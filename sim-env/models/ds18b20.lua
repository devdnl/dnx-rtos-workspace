--[[----------------------------------------------------------------------------
  DS18B20 1-Wire digital thermometer model (also DS18S20/DS1822 compatible
  family codes, protocol and scratchpad of DS18B20).

  Usage:
    local ds2482 = require("models.ds2482")
    local ds18b20 = require("models.ds18b20")
    local ow = ds2482.new{}:attach("I2C1", 0x18)
    local s1 = ds18b20.new{serial = 0x00000A1B2C3D, temperature = 21.5}:attach(ow)
    local s2 = ds18b20.new{serial = 0x00000A1B2C3E, parasite = true}:attach(ow)
    s1:set_temperature(-10.125)

  Options of new():
    serial        48-bit serial number (default: next number of module,
                  starting from 0x000000000001)
    family        family code (default 0x28)
    rom           ROM code given directly: table of 8 bytes in bus order
                  {family, serial (LSB first, 6 bytes), CRC}, CRC is not
                  checked (invalid ROM codes can be tested)
    temperature   temperature (C, default 25), measured by CONVERT T
    parasite      parasite power (default false: external power)
    th, tl, resolution
                  content of EEPROM (default 75 C, 70 C, 12 bits)
    file          host file with EEPROM content (3 bytes: TH, TL, config),
                  saved by COPY SCRATCHPAD
    timing        conversion and EEPROM write time (default true), false:
                  commands are finished immediately

  ROM commands: SEARCH ROM (F0), ALARM SEARCH (EC), READ ROM (33), MATCH ROM
  (55), SKIP ROM (CC); overdrive commands are not supported (device does not
  respond at overdrive speed). Function commands: CONVERT T (44, conversion
  time 93.75/187.5/375/750 ms for 9..12 bits, read slots return 0 while
  converting), WRITE SCRATCHPAD (4E: TH, TL, config), READ SCRATCHPAD (BE: 9
  bytes with CRC, then 1s), COPY SCRATCHPAD (48: 10 ms), RECALL E2 (B8),
  READ POWER SUPPLY (B4: 0 parasite power). The temperature register is 85 C
  after power on. The alarm flag is set by conversion when T >= TH or
  T <= TL (integer part of temperature, as in the datasheet).

  Parasite power: the conversion and the EEPROM write need the strong
  pull-up of the master (DS2482: SPU bit set before the command), when the
  strong pull-up is not active the operation fails: the temperature register
  (or EEPROM) is not changed and a warning is printed.

  Methods:
    s:attach(bus)              connect to 1-Wire bus (onewire bus or master
                               model with field "ow", e.g. DS2482)
    s:detach()                 disconnect (hot-plug)
    s:set_temperature(c)       set measured temperature (C)
    s:temperature()            measured temperature (C)
    s:register()               temperature register (C, last conversion)
    s:rom()                    ROM code: table of 8 bytes in bus order
    s:rom_string()             ROM code as "28-00000a1b2c3d" (Linux style)
    s:alarm()                  alarm flag
    s:power_cycle()            power on state (scratchpad from EEPROM,
                               temperature register 85 C, no alarm)
    s:set_fault(name, on)      fault injection: "crc" (wrong CRC of
                               scratchpad), "rom_crc" (wrong CRC of ROM
                               code sent by search and READ ROM, MATCH ROM
                               expects the wrong code), "mute" (no response,
                               device stays connected)
------------------------------------------------------------------------------]]

local M = {}

local DS = {}
DS.__index = DS

local next_serial = 1

local CONV_TIME = {[9] = 93.75, [10] = 187.5, [11] = 375, [12] = 750}  -- ms
local COPY_TIME = 10                                                    -- ms

-- Maxim/Dallas CRC8 (x^8 + x^5 + x^4 + 1)
function M.crc8(bytes, n)
    local crc = 0
    for i = 1, n or #bytes do
        local b = bytes[i]
        for _ = 1, 8 do
            local mix = (crc ~ b) & 1
            crc = crc >> 1
            if mix ~= 0 then crc = crc ~ 0x8C end
            b = b >> 1
        end
    end
    return crc
end

local function to_reg(c, resolution)
    local v = math.floor(c * 16 + 0.5)
    v = math.max(-55 * 16, math.min(125 * 16, v))
    v = v & ~((1 << (12 - resolution)) - 1)
    return v & 0xFFFF
end

local function from_reg(r)
    if r & 0x8000 ~= 0 then r = r - 0x10000 end
    return r / 16
end

local function to_s8(v)
    if v & 0x80 ~= 0 then return v - 0x100 end
    return v
end

local function config_byte(resolution)
    return ((resolution - 9) << 5) | 0x1F
end

function M.new(opts)
    opts = opts or {}
    local d = setmetatable({}, DS)

    if opts.rom then
        assert(type(opts.rom) == "table" and #opts.rom == 8, "ds18b20: rom must be table of 8 bytes")
        d.rom_code = {}
        for i = 1, 8 do d.rom_code[i] = opts.rom[i] & 0xFF end
    else
        local serial = opts.serial
        if serial == nil then
            serial = next_serial
            next_serial = next_serial + 1
        end
        assert(math.type(serial) == "integer" and serial >= 0 and serial < (1 << 48),
               "ds18b20: serial must be 48-bit integer")
        d.rom_code = {opts.family or 0x28}
        for i = 0, 5 do d.rom_code[i + 2] = (serial >> (8 * i)) & 0xFF end
        d.rom_code[8] = M.crc8(d.rom_code, 7)
    end

    local resolution = opts.resolution or 12
    assert(CONV_TIME[resolution], "ds18b20: resolution must be 9..12")
    d.ee = {th = (opts.th or 75) & 0xFF, tl = (opts.tl or 70) & 0xFF, cfg = config_byte(resolution)}
    d.file = opts.file
    if d.file then d:load() end

    d.parasite = opts.parasite and true or false
    d.timing = opts.timing ~= false
    d.temp = opts.temperature or 25
    d.faults = {}
    d.alarm_flag = false
    d.treg = to_reg(85, 12)
    d.sp = {th = d.ee.th, tl = d.ee.tl, cfg = d.ee.cfg}
    d.busy = nil
    d.co = nil
    d.drive = 1

    if d.file then
        host.on_exit(function() d:save() end)
    end

    return d
end

-- EEPROM file -----------------------------------------------------------------
function DS:load()
    local f = io.open(self.file, "rb")
    if f then
        local s = f:read(3)
        f:close()
        if s and #s == 3 then
            self.ee.th, self.ee.tl = s:byte(1), s:byte(2)
            self.ee.cfg = (s:byte(3) & 0x60) | 0x1F
        end
    end
end

function DS:save()
    if self.file and self.dirty then
        local f = io.open(self.file, "wb")
        if f then
            f:write(string.char(self.ee.th, self.ee.tl, self.ee.cfg))
            f:close()
            self.dirty = false
        else
            print("ds18b20: cannot write " .. self.file)
        end
    end
end

-- host interface --------------------------------------------------------------
function DS:attach(bus)
    if type(bus) == "table" and bus.ow then bus = bus.ow end
    assert(type(bus) == "table" and bus.add, "ds18b20: 1-Wire bus expected")
    if self.bus then self.bus:remove(self) end
    self.bus = bus
    bus:add(self)
    self.co = nil
    return self
end

function DS:detach()
    if self.bus then
        self.bus:remove(self)
        self.bus = nil
    end
    self.co = nil
    self.busy = nil
end

function DS:set_temperature(c)
    assert(type(c) == "number", "ds18b20: temperature must be number")
    self.temp = c
end

function DS:temperature()
    return self.temp
end

function DS:register()
    self:update()
    return from_reg(self.treg)
end

function DS:alarm()
    self:update()
    return self.alarm_flag
end

function DS:rom()
    return {table.unpack(self.rom_code)}
end

function DS:rom_string()
    local s = string.format("%02x-", self.rom_code[1])
    for i = 7, 2, -1 do s = s .. string.format("%02x", self.rom_code[i]) end
    return s
end

-- power on state: scratchpad from EEPROM, temperature register 85 C
function DS:power_cycle()
    self.co = nil
    self.drive = 1
    self.busy = nil
    self.alarm_flag = false
    self.treg = to_reg(85, 12)
    self.sp = {th = self.ee.th, tl = self.ee.tl, cfg = self.ee.cfg}
end

function DS:set_fault(name, on)
    assert(name == "crc" or name == "rom_crc" or name == "mute", "ds18b20: unknown fault " .. tostring(name))
    self.faults[name] = on ~= false and true or nil
end

function DS:resolution()
    return ((self.sp.cfg >> 5) & 3) + 9
end

-- ROM code as sent on the bus (fault rom_crc: CRC inverted)
function DS:bus_rom()
    local r = self:rom()
    if self.faults.rom_crc then r[8] = r[8] ~ 0xFF end
    return r
end

-- busy operations (conversion, EEPROM write) ----------------------------------
function DS:now()
    return host.time_ms()
end

-- finish operation when its time elapsed (timing off: immediately, parasite
-- power: when strong pull-up is activated or at next activity of the line),
-- returns true when operation is in progress
function DS:update(force)
    local b = self.busy
    if not b then return false end
    local due
    if self.timing then
        due = force or self:now() >= b.done
    else
        due = force or not self.parasite or b.powered
    end
    if due then
        self.busy = nil
        if self.parasite and (not b.powered or b.lost) then
            print(string.format("ds18b20 %s: %s failed, no strong pull-up (parasite power)", self:rom_string(), b.name))
        elseif b.name == "conversion" then
            self.treg = b.value
            local t = from_reg(self.treg)
            local tint = math.floor(t)
            self.alarm_flag = (tint >= to_s8(self.sp.th)) or (tint <= to_s8(self.sp.tl))
        elseif b.name == "copy" then
            self.ee.th, self.ee.tl, self.ee.cfg = self.sp.th, self.sp.tl, self.sp.cfg
            self.dirty = true
            if self.file then
                if not self.save_timer then
                    self.save_timer = host.timer(500, function() self.save_timer = nil; self:save() end)
                end
            end
        end
    end
    return self.busy ~= nil
end

function DS:start(name, time, value)
    self.busy = {name = name, done = self:now() + time, value = value, powered = false, lost = false}
    self:update()
end

-- activity of the line during operation of parasite powered device: timing
-- off, operation is finished (without strong pull-up it fails)
function DS:activity()
    if self.busy and self.parasite and not self.timing then
        self:update(true)
    end
end

-- bit level protocol ----------------------------------------------------------
-- the transaction is a coroutine resumed by each time slot: yield(drive)
-- returns the sampled level of the slot
local function slot(drive)
    return coroutine.yield(drive)
end

local function rx_byte()
    local v = 0
    for i = 0, 7 do
        if slot(1) == 1 then v = v | (1 << i) end
    end
    return v
end

local function tx_byte(v)
    for i = 0, 7 do slot((v >> i) & 1) end
end

local function rom_bit(rom, n)
    return (rom[(n >> 3) + 1] >> (n & 7)) & 1
end

function DS:transaction()
    local cmd = rx_byte()

    if cmd == 0xF0 or cmd == 0xEC then
        if cmd == 0xEC and not self:alarm() then return end
        local rom = self:bus_rom()
        for n = 0, 63 do
            local b = rom_bit(rom, n)
            slot(b)
            slot(b ~ 1)
            if slot(1) ~= b then return end  -- other direction: device is deselected
        end
    elseif cmd == 0x33 then
        for _, b in ipairs(self:bus_rom()) do tx_byte(b) end
    elseif cmd == 0x55 then
        local rom = self:bus_rom()
        local match = true
        for n = 0, 63 do
            if slot(1) ~= rom_bit(rom, n) then match = false end
        end
        if not match then return end
    elseif cmd ~= 0xCC then
        return  -- unknown or overdrive command (not supported)
    end

    self:function_command(rx_byte())
end

function DS:function_command(cmd)
    self:update()

    if cmd == 0x44 then                          -- CONVERT T
        if self.busy then return end
        local res = self:resolution()
        self:start("conversion", CONV_TIME[res], to_reg(self.temp, res))
        self:busy_slots()

    elseif cmd == 0xBE then                      -- READ SCRATCHPAD
        local sp = {self.treg & 0xFF, self.treg >> 8, self.sp.th, self.sp.tl, self.sp.cfg, 0xFF, 0x0C, 0x10}
        sp[9] = M.crc8(sp, 8)
        if self.faults.crc then sp[9] = sp[9] ~ 0xFF end
        for _, b in ipairs(sp) do tx_byte(b) end
        while true do slot(1) end

    elseif cmd == 0x4E then                      -- WRITE SCRATCHPAD
        self.sp.th = rx_byte()
        self.sp.tl = rx_byte()
        self.sp.cfg = (rx_byte() & 0x60) | 0x1F

    elseif cmd == 0x48 then                      -- COPY SCRATCHPAD
        if self.busy then return end
        self:start("copy", COPY_TIME)
        self:busy_slots()

    elseif cmd == 0xB8 then                      -- RECALL E2
        self.sp.th, self.sp.tl, self.sp.cfg = self.ee.th, self.ee.tl, self.ee.cfg

    elseif cmd == 0xB4 then                      -- READ POWER SUPPLY
        while true do slot(self.parasite and 0 or 1) end
    end
end

-- read slots return 0 while the operation is in progress (external power),
-- device with parasite power cannot signal it (line is held by the strong
-- pull-up)
function DS:busy_slots()
    local drive = function()
        if self.parasite then return 1 end
        return self:update() and 0 or 1
    end
    while true do slot(drive) end
end

-- device interface of the 1-Wire bus --------------------------------------
function DS:ow_reset(overdrive)
    self:update()
    self:activity()
    self.co = nil
    self.drive = 1
    if self.faults.mute or overdrive or (self.bus and self.bus.short) then
        return false
    end
    self.co = coroutine.create(function() self:transaction() end)
    local ok, v = coroutine.resume(self.co)
    if not ok then self.co = nil; error(v) end
    self.drive = v
    return true
end

function DS:ow_drive()
    local v = self.drive
    if type(v) == "function" then v = v() end
    return v
end

function DS:ow_sample(level)
    self:activity()
    if not self.co then return end
    local ok, v = coroutine.resume(self.co, level)
    if not ok then self.co = nil; self.drive = 1; error(v) end
    if coroutine.status(self.co) == "dead" then
        self.co = nil
        self.drive = 1
    else
        self.drive = v
    end
end

-- strong pull-up: power of parasite device during operation, the pull-up
-- switched off before the end of operation breaks it
function DS:ow_power(on)
    local b = self.busy
    if not (b and self.parasite) then return end
    if on then
        b.powered = true
        self:update()
    elseif self:update() then
        b.lost = true
        if not self.timing then self:update(true) end
    end
end

return M
