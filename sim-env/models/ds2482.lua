--[[----------------------------------------------------------------------------
  DS2482-100 I2C to 1-Wire bridge model (single channel 1-Wire master).

  Usage:
    local ds2482 = require("models.ds2482")
    local ds18b20 = require("models.ds18b20")
    local ow = ds2482.new{}:attach("I2C:0", 0x18)    -- AD1 = AD0 = 0
    ds18b20.new{serial = 0x0102030405}:attach(ow)     -- devices on ow.ow
    ow.ow:set_short(true)                             -- 1-Wire line shorted

  Options of new():
    bus       1-Wire bus (models/onewire.lua), default: new bus
    timing    busy time of 1-Wire operations (default true): the 1WB bit of
              the status register is set and results of the operation are
              not visible until the operation is finished, as on the real
              device (reset 1.15 ms, slot 72.5 us; overdrive 0.15 ms,
              10.6 us), false: operations are finished immediately

  Addresses: 0x18..0x1B (AD1, AD0 pins).

  Registers (read pointer codes): status F0, read data E1, configuration C3.
  Status: bit 0 1WB (busy), 1 PPD (presence), 2 SD (short), 3 LL (line
  level), 4 RST (device reset), 5 SBR (single bit result), 6 TSB (triplet
  second bit), 7 DIR (branch direction). Configuration: bit 0 APU, 1 PPM,
  2 SPU (strong pull-up), 3 1WS (overdrive); the written byte is accepted
  only when its upper nibble is the one's complement of the lower nibble,
  the read value contains only the lower nibble.

  Commands (write transaction: command byte, optional parameter byte):
    F0     DRST   device reset (config cleared, RST set, 1-Wire activity
                  and strong pull-up ended), read pointer: status
    E1 p   SRP    set read pointer (F0, E1, C3; other value: NACK)
    D2 c   WCFG   write configuration, read pointer: configuration
    B4     1WRS   1-Wire reset, result: PPD, SD
    87 b   1WSB   single bit (bit 7 of b), result: SBR
    A5 d   1WWB   write byte
    96     1WRB   read byte, result in read data register (SRP E1 to read)
    78 d   1WT    triplet: two read slots (SBR, TSB) and write of direction
                  (DIR): SBR != TSB: SBR, both 0: bit 7 of d, both 1: 1
  1-Wire commands set the read pointer to the status register. A command
  that needs the idle 1-Wire line (all except DRST and SRP) is ignored when
  1WB is set (a warning is printed: the master should poll the status).
  Unknown command codes are not acknowledged. SPU: the strong pull-up is
  activated after the next 1WWB, 1WSB or 1WT and it ends (SPU bit is
  cleared) by the next 1-Wire command, DRST or write of SPU = 0.

  Methods and fields:
    ow:attach(bus, address)   connect to I2C bus
    ow.ow                     1-Wire bus (add devices, short)
    ow:status()               status register (host view)
------------------------------------------------------------------------------]]

local onewire = require("models.onewire")

local M = {}

local DEV = {}
DEV.__index = DEV

local ST_1WB, ST_PPD, ST_SD, ST_LL, ST_RST, ST_SBR, ST_TSB, ST_DIR = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
local CFG_SPU, CFG_1WS = 0x04, 0x08

local T_RESET = {[false] = 1.148, [true] = 0.146}   -- ms (tRSTL + tRSTH)
local T_SLOT  = {[false] = 0.0725, [true] = 0.0106} -- ms

local PARAM = {[0xE1] = true, [0xD2] = true, [0x87] = true, [0xA5] = true, [0x78] = true}
local NO_PARAM = {[0xF0] = true, [0xB4] = true, [0x96] = true}

function M.new(opts)
    opts = opts or {}
    local d = setmetatable({}, DEV)
    d.ow = opts.bus or onewire.new()
    d.timing = opts.timing ~= false
    d.cfg = 0
    d.st = ST_RST        -- status bits except 1WB and LL
    d.data = 0xFF
    d.ptr = 0xF0
    d.busy_until = 0
    d.pending = nil
    d.spu_active = false
    return d
end

function DEV:now()
    return host.time_ms()
end

-- apply results of finished 1-Wire operation
function DEV:sync()
    if self.pending and self:now() >= self.busy_until then
        local p = self.pending
        self.pending = nil
        self.st = (self.st & ~p.mask) | (p.bits & p.mask)
        if p.data then self.data = p.data end
        if p.spu then
            self.ow:power(true)
            self.spu_active = true
        end
    end
end

function DEV:busy()
    self:sync()
    return self.pending ~= nil
end

function DEV:status()
    self:sync()
    local s = self.st & ~(ST_1WB | ST_LL)
    if self.pending then s = s | ST_1WB end
    if self.ow:level() ~= 0 then s = s | ST_LL end
    return s
end

-- end of strong pull-up: by 1-Wire command, DRST or SPU = 0
function DEV:end_spu()
    if self.spu_active then
        self.ow:power(false)
        self.spu_active = false
        self.cfg = self.cfg & ~CFG_SPU
    end
end

-- 1-Wire operation: fn(od) returns status bits (mask, bits) and data,
-- slots: duration in time slots (or reset)
function DEV:operation(name, slots, reset, use_spu, fn)
    self:sync()
    if self.pending then
        print(string.format("ds2482: %s ignored, 1-Wire busy (1WB = 1)", name))
        return
    end
    self:end_spu()
    local od = self.cfg & CFG_1WS ~= 0
    local mask, bits, data = fn(od)
    local p = {mask = mask or 0, bits = bits or 0, data = data}
    if use_spu and self.cfg & CFG_SPU ~= 0 then p.spu = true end
    self.ptr = 0xF0
    if self.timing then
        self.busy_until = self:now() + (reset and T_RESET[od] or 0) + slots * T_SLOT[od]
    else
        self.busy_until = self:now()
    end
    self.pending = p
    self:sync()
end

function DEV:execute(cmd, param)
    local ow = self.ow

    if cmd == 0xF0 then                          -- DRST
        self.pending = nil
        self:end_spu()
        self.cfg = 0
        self.st = ST_RST
        self.ptr = 0xF0
        return true

    elseif cmd == 0xE1 then                      -- SRP
        if param ~= 0xF0 and param ~= 0xE1 and param ~= 0xC3 then
            return false
        end
        self.ptr = param
        return true

    elseif cmd == 0xD2 then                      -- WCFG
        if self:busy() then
            print("ds2482: WCFG ignored, 1-Wire busy (1WB = 1)")
            return true
        end
        if (param >> 4) == (~param & 0x0F) then
            self.cfg = param & 0x0F
            self.st = self.st & ~ST_RST
            if self.cfg & CFG_SPU == 0 then self:end_spu() end
        else
            print(string.format("ds2482: WCFG 0x%02X ignored (upper nibble is not complement of lower)", param))
        end
        self.ptr = 0xC3
        return true

    elseif cmd == 0xB4 then                      -- 1WRS
        self:operation("1WRS", 0, true, false, function(od)
            local presence, short = ow:reset(od)
            return ST_PPD | ST_SD, (presence and ST_PPD or 0) | (short and ST_SD or 0)
        end)
        return true

    elseif cmd == 0x87 then                      -- 1WSB
        self:operation("1WSB", 1, false, true, function()
            local v = ow:slot((param >> 7) & 1)
            return ST_SBR, v ~= 0 and ST_SBR or 0
        end)
        return true

    elseif cmd == 0xA5 then                      -- 1WWB
        self:operation("1WWB", 8, false, true, function()
            for i = 0, 7 do ow:slot((param >> i) & 1) end
            return 0, 0
        end)
        return true

    elseif cmd == 0x96 then                      -- 1WRB
        self:operation("1WRB", 8, false, false, function()
            local v = 0
            for i = 0, 7 do
                if ow:slot(1) ~= 0 then v = v | (1 << i) end
            end
            return 0, 0, v
        end)
        return true

    elseif cmd == 0x78 then                      -- 1WT
        self:operation("1WT", 3, false, true, function()
            local id = ow:slot(1)
            local cmp = ow:slot(1)
            local dir
            if id ~= cmp then
                dir = id
            elseif id == 0 then
                dir = (param >> 7) & 1
            else
                dir = 1
            end
            ow:slot(dir)
            return ST_SBR | ST_TSB | ST_DIR,
                   (id ~= 0 and ST_SBR or 0) | (cmp ~= 0 and ST_TSB or 0) | (dir ~= 0 and ST_DIR or 0)
        end)
        return true
    end

    return false
end

-- I2C bus interface ---------------------------------------------------------
function DEV:select(read)
    self.cmd = nil
    return true
end

function DEV:write(byte)
    if self.cmd then
        local cmd = self.cmd
        self.cmd = nil
        return self:execute(cmd, byte)
    elseif PARAM[byte] then
        self.cmd = byte
        return true
    elseif NO_PARAM[byte] then
        return self:execute(byte)
    end
    return false  -- unknown command: NACK
end

function DEV:read()
    if self.ptr == 0xE1 then
        self:sync()
        return self.data
    elseif self.ptr == 0xC3 then
        return self.cfg & 0x0F
    end
    return self:status()
end

function DEV:stop()
    self.cmd = nil  -- command without parameter is not executed
end

function DEV:attach(bus, address)
    assert(address >= 0x18 and address <= 0x1B, "ds2482: address must be 0x18..0x1B")
    i2c.attach(bus, address, self)
    return self
end

return M
