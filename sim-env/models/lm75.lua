--[[----------------------------------------------------------------------------
  LM75 I2C temperature sensor model (9-bit, 0.5 C resolution).

  Usage:
    local lm75 = require("models.lm75")
    local sensor = lm75.new{temperature = 21.5, os = "PA6"}  -- optional OS output
    sensor:attach("I2C1", 0x48)
    sensor:set_temperature(30)

  Registers (selected by pointer byte):
    0 temperature (read only, 2 bytes), 1 configuration (1 byte),
    2 THYST (2 bytes, default 75 C), 3 TOS (2 bytes, default 80 C).
  Configuration: bit 0 shutdown (temperature is not updated), bit 1
  interrupt mode, bit 2 OS polarity (1: active high), bits 3-4 fault queue
  (ignored). Comparator mode: OS active when T > TOS, inactive when
  T < THYST. Interrupt mode: OS active when T > TOS (or T < THYST after
  TOS event), inactive after read of any register.
------------------------------------------------------------------------------]]

local M = {}

local LM75 = {}
LM75.__index = LM75

local function to_reg(c)
    local v = math.floor(c * 2 + 0.5)
    v = math.max(-110, math.min(250, v))   -- -55 .. 125 C
    return (v & 0x1FF) << 7
end

local function from_reg(r)
    local v = (r >> 7) & 0x1FF
    if v & 0x100 ~= 0 then v = v - 0x200 end
    return v / 2
end

function M.new(opts)
    opts = opts or {}
    local dev = setmetatable({}, LM75)
    dev.os = opts.os
    dev.reg = {[0] = to_reg(opts.temperature or 25), [1] = 0, [2] = to_reg(75), [3] = to_reg(80)}
    dev.ptr = 0
    dev.alarm = false
    dev.armed_low = false
    dev:update_os()
    return dev
end

function LM75:temperature()
    return from_reg(self.reg[0])
end

function LM75:set_temperature(c)
    if self.reg[1] & 1 == 0 then
        self.reg[0] = to_reg(c)
        self:check_limits()
    end
end

function LM75:check_limits()
    local t, tos, thyst = from_reg(self.reg[0]), from_reg(self.reg[3]), from_reg(self.reg[2])
    if self.reg[1] & 2 == 0 then
        -- comparator mode
        if t > tos then
            self.alarm = true
        elseif t < thyst then
            self.alarm = false
        end
    else
        -- interrupt mode
        if not self.armed_low and t > tos then
            self.alarm, self.armed_low = true, true
        elseif self.armed_low and t < thyst then
            self.alarm, self.armed_low = true, false
        end
    end
    self:update_os()
end

function LM75:update_os()
    if self.os then
        local active_high = self.reg[1] & 4 ~= 0
        gpio.set(self.os, self.alarm == active_high)
    end
end

-- bus interface
function LM75:select(read)
    self.wcnt = 0
    self.rcnt = 0
    if read and self.reg[1] & 2 ~= 0 then
        self.alarm = false -- interrupt mode: read clears OS
        self:update_os()
    end
    return true
end

function LM75:write(byte)
    if self.wcnt == 0 then
        self.ptr = byte & 3
        self.wdata = 0
    else
        self.wdata = (self.wdata << 8) | byte
    end
    self.wcnt = self.wcnt + 1
    return true
end

function LM75:read()
    local v
    if self.ptr == 1 then
        v = self.reg[1]
    else
        local r = self.reg[self.ptr]
        v = (self.rcnt % 2 == 0) and (r >> 8) or r
    end
    self.rcnt = self.rcnt + 1
    return v & 0xFF
end

function LM75:stop()
    local n = (self.wcnt or 0) - 1
    if self.ptr == 1 and n >= 1 then
        self.reg[1] = self.wdata & 0x1F
        self:check_limits()
    elseif (self.ptr == 2 or self.ptr == 3) and n >= 2 then
        self.reg[self.ptr] = (self.wdata >> ((n - 2) * 8)) & 0xFF80
        self:check_limits()
    end
    self.wcnt = 0
end

function LM75:attach(bus, address)
    i2c.attach(bus, address, self)
    return self
end

return M
