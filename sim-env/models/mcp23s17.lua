--[[----------------------------------------------------------------------------
  MCP23S17 16-bit I/O expander model (SPI, ports A and B, interrupt outputs).

  Usage:
    local mcp23s17 = require("models.mcp23s17")
    local exp = mcp23s17.new{address = 0, int_a = "PB3", int_b = "PB4"}
    exp:attach("SPI:2", "PB2")                   -- bus, CS pin (active low)
    exp:connect_output("A0", "PB8")              -- GPA0 drives system input PB8
    exp:connect_input("B0", "PB16")              -- system output PB16 drives GPB0
    exp:set_input("B7", false)                   -- external level of pin (button)
    exp:on_change(function(state, changed) ... end)

  Options of new():
    address     hardware address A2..A0 (0..7, default 0), used when HAEN is set
                (IOCON), devices with different addresses can share one CS pin
                (attach all of them to the same pin)
    int_a       system input pin connected to INTA (optional)
    int_b       system input pin connected to INTB (optional)
    reset       system output pin connected to RESET (optional, active low:
                the device is reset and does not respond while the pin is low,
                the pin is followed from the first change done by the system)
    floating    level of input pin without external drive and pull-up
                (default false)

  Registers (IOCON.BANK = 0 addresses, BANK = 1: port A 0x00..0x0A, port B
  0x10..0x1A in the same order): IODIR 00/01, IPOL 02/03, GPINTEN 04/05,
  DEFVAL 06/07, INTCON 08/09, IOCON 0A/0B (one register), GPPU 0C/0D, INTF
  0E/0F, INTCAP 10/11, GPIO 12/13, OLAT 14/15. Unimplemented addresses are read
  as 0, writes are ignored. Power-on and reset: IODIR = 0xFF, other registers 0.

  SPI (mode 0 or 3): opcode 0100 A2 A1 A0 R/W, register address, data. The
  device responds when the opcode address is equal to its address pins and
  IOCON.HAEN is set, or to address 0 when HAEN is cleared (so IOCON written at
  address 0 enables HAEN in all devices on the CS pin). Sequential mode
  (SEQOP = 0): the address is incremented after each data byte (BANK = 0: 0x15
  -> 0x00; BANK = 1: 0x0A -> 0x10, 0x1A -> 0x00); byte mode (SEQOP = 1): BANK = 0
  toggles between the A/B registers of a pair, BANK = 1 keeps the address.
  MISO is not driven during opcode and address bytes (0xFF).

  Pins: output (IODIR = 0) has the level of OLAT, input has the level driven
  externally (set_input, connect_input), the pull-up level (GPPU) or the level
  of option floating. The GPIO register returns pin levels, input bits are
  inverted by IPOL; writes to GPIO write OLAT.

  Interrupts (input pins with GPINTEN set): INTCON = 0 - change of the pin,
  INTCON = 1 - pin level differs from DEFVAL. The first interrupt event of the
  port sets INTF (pins that caused it) and copies GPIO to INTCAP; next events
  are ignored until GPIO or INTCAP of the port is read (then a condition of
  INTCON = 1 that still exists causes a new interrupt immediately). INT pins:
  MIRROR = 1 - both pins signal interrupt of any port; ODR = 1 - open drain,
  active low (INTPOL ignored), ODR = 0 - push-pull, INTPOL sets active level.
  INT lines of more devices connected to one system pin are wired-AND with
  pull-up (open drain outputs can share the line, a push-pull output high
  against other output low is reported). DISSLW has no effect.

  Methods (pin: 0..15, "A0".."A7", "B0".."B7" or "GPA0".."GPB7"; port: "A" or "B"):
    exp:attach(bus, cs)                connect to SPI bus
    exp:set_input(pin, level)          drive pin externally (true/false, nil: release)
    exp:release(pin)                   stop driving pin
    exp:pin(pin)                       pin level (boolean)
    exp:state()                        levels of all pins (GPA0 is bit 0, GPB0 bit 8)
    exp:port(port)                     levels of port pins (8 bits)
    exp:register(name [, port])        register value without side effects (e.g.
                                       "IODIR", "A"; "IOCON")
    exp:interrupt(port)                interrupt of port is active (INTF != 0)
    exp:reset()                        reset (as RESET pin)
    exp:on_change(fn)                  fn(state, changed) when pin levels change
    exp:connect_output(pin, sys [, invert])   pin drives system input pin
    exp:connect_input(pin, sys [, invert])    system output pin drives pin
------------------------------------------------------------------------------]]

local M = {}

local DEV = {}
DEV.__index = DEV

-- registers in address order (BANK = 0: index * 2 + port, BANK = 1: port * 0x10 + index)
local REGS = {"IODIR", "IPOL", "GPINTEN", "DEFVAL", "INTCON", "IOCON", "GPPU", "INTF", "INTCAP", "GPIO", "OLAT"}
local REG_INDEX = {}
for i, name in ipairs(REGS) do REG_INDEX[name] = i - 1 end

local IOCON_BANK, IOCON_MIRROR, IOCON_SEQOP = 0x80, 0x40, 0x20
local IOCON_HAEN, IOCON_ODR, IOCON_INTPOL = 0x08, 0x04, 0x02

-- INT lines: system pin -> {drivers = {[key] = level or nil}}, wired-AND with pull-up
local lines = {}

local function line_drive(pin, key, level)
    local line = lines[pin]
    if not line then
        line = {drivers = {}}
        lines[pin] = line
    end
    line.drivers[key] = level
    local value, high = true, false
    for _, v in pairs(line.drivers) do
        if v == false then value = false end
        if v == true then high = true end   -- push-pull output (open drain is low or released)
    end
    local conflict = high and not value
    if conflict and not line.conflict then
        print(string.format("mcp23s17: INT line %s: push-pull output high and other output low (use ODR)", pin))
    end
    line.conflict = conflict
    if line.value ~= value then
        line.value = value
        gpio.set(pin, value)
    end
end

local function bit(v)
    return v and 1 or 0
end

local function to_level(level)
    if level == nil then return nil end
    if level == 0 then return false end
    if level == 1 then return true end
    return level and true or false
end

-- pin number (0..15) from number or name
local function pin_index(pin)
    if math.type(pin) == "integer" and pin >= 0 and pin <= 15 then
        return pin
    end
    if type(pin) == "string" then
        local p, n = pin:upper():match("^G?P?([AB])([0-7])$")
        if p then
            return (p == "B" and 8 or 0) + tonumber(n)
        end
    end
    error("mcp23s17: invalid pin: " .. tostring(pin), 3)
end

local function port_index(port)
    if port == "A" or port == "a" or port == 0 then return 0 end
    if port == "B" or port == "b" or port == 1 then return 1 end
    error("mcp23s17: invalid port: " .. tostring(port), 3)
end

local count = 0

function M.new(opts)
    opts = opts or {}
    count = count + 1
    local d = setmetatable({}, DEV)
    d.address = opts.address or 0
    assert(math.type(d.address) == "integer" and d.address >= 0 and d.address <= 7,
           "mcp23s17: address must be 0..7")
    d.name = string.format("mcp23s17 #%d (address %d)", count, d.address)
    d.int_pins = {opts.int_a, opts.int_b}
    d.reset_pin = opts.reset
    d.floating = opts.floating and true or false
    d.mode = {[0] = true, [3] = true}
    d.ext = {}
    d.handlers = {}
    d.warned = {}
    d.held = false
    d.state_spi = "idle"
    d.levels = 0
    d:reset()

    if d.reset_pin then
        gpio.on_change(d.reset_pin, function(level)
            d.held = not level
            if d.held then d:reset() end
        end)
    end

    return d
end

-- registers ---------------------------------------------------------------------
function DEV:reset()
    self.r = {}
    for p = 0, 1 do
        self.r[p] = {IODIR = 0xFF, IPOL = 0, GPINTEN = 0, DEFVAL = 0, INTCON = 0, GPPU = 0,
                     INTF = 0, INTCAP = 0, OLAT = 0}
    end
    self.iocon = 0
    self.state_spi = "ignore"
    self:update()
end

-- register of address: name, port (nil: unimplemented)
function DEV:decode(addr)
    if self.iocon & IOCON_BANK == 0 then
        if addr <= 0x15 then return REGS[(addr >> 1) + 1], addr & 1 end
    else
        local idx = addr & 0x0F
        if (addr & 0xE0) == 0 and idx <= 0x0A then return REGS[idx + 1], (addr >> 4) & 1 end
    end
    return nil
end

function DEV:next_address(addr)
    local bank1 = self.iocon & IOCON_BANK ~= 0
    if self.iocon & IOCON_SEQOP ~= 0 then
        if not bank1 and addr <= 0x15 then return addr ~ 1 end
        return addr
    end
    if bank1 then
        if addr == 0x0A then return 0x10 end
        if addr == 0x1A then return 0x00 end
    elseif addr == 0x15 then
        return 0x00
    end
    return (addr + 1) & 0xFF
end

-- port pin levels (8 bits)
function DEV:pins(p)
    return (self.levels >> (8 * p)) & 0xFF
end

-- value of GPIO register: pin levels, input bits inverted by IPOL
function DEV:gpio_value(p)
    local r = self.r[p]
    return self:pins(p) ~ (r.IPOL & r.IODIR)
end

function DEV:read_reg(addr)
    local name, p = self:decode(addr)
    if not name then return 0 end
    if name == "IOCON" then return self.iocon end
    local r = self.r[p]
    if name == "GPIO" then
        local v = self:gpio_value(p)
        self:clear_interrupt(p)
        return v
    elseif name == "INTCAP" then
        local v = r.INTCAP
        self:clear_interrupt(p)
        return v
    end
    return r[name]
end

function DEV:write_reg(addr, value)
    local name, p = self:decode(addr)
    if not name or name == "INTF" or name == "INTCAP" then return end
    if name == "IOCON" then
        self.iocon = value & 0xFE
    elseif name == "GPIO" then
        self.r[p].OLAT = value
    else
        self.r[p][name] = value
    end
    self:update()
end

-- pins and interrupts -------------------------------------------------------------
function DEV:pin_level(n)
    local r = self.r[n >> 3]
    local mask = 1 << (n & 7)
    local ext = self.ext[n]
    if r.IODIR & mask == 0 then
        local out = r.OLAT & mask ~= 0
        if ext ~= nil and ext ~= out then
            if not self.warned[n] then
                self.warned[n] = true
                print(string.format("%s: GP%s%d is output (%d), driven externally to %d",
                                    self.name, n < 8 and "A" or "B", n & 7, bit(out), bit(ext)))
            end
        else
            self.warned[n] = nil
        end
        return out
    end
    self.warned[n] = nil
    if ext ~= nil then return ext end
    if r.GPPU & mask ~= 0 then return true end
    return self.floating
end

-- interrupt conditions of port, changed: pins changed since last update
function DEV:check_interrupt(p, changed)
    local r = self.r[p]
    if r.INTF ~= 0 then return end
    local en = r.GPINTEN & r.IODIR
    local trig = (changed & en & ~r.INTCON) | ((self:pins(p) ~ r.DEFVAL) & en & r.INTCON)
    trig = trig & 0xFF
    if trig ~= 0 then
        r.INTF = trig
        r.INTCAP = self:gpio_value(p)
    end
end

function DEV:clear_interrupt(p)
    if self.r[p].INTF ~= 0 then
        self.r[p].INTF = 0
        self:check_interrupt(p, 0)
        self:update_int()
    end
end

function DEV:update_int()
    local a, b = self.r[0].INTF ~= 0, self.r[1].INTF ~= 0
    if self.iocon & IOCON_MIRROR ~= 0 then
        a, b = a or b, a or b
    end
    local odr = self.iocon & IOCON_ODR ~= 0
    local pol = self.iocon & IOCON_INTPOL ~= 0
    for i, active in ipairs({a, b}) do
        local pin = self.int_pins[i]
        if pin then
            local level
            if odr then
                if active then level = false end    -- open drain: low or released (nil)
            else
                level = (active and pol) or (not active and not pol)
            end
            line_drive(pin, self, level)
        end
    end
end

-- recompute pin levels, interrupts and outputs
function DEV:update()
    local levels = 0
    for n = 0, 15 do
        if self:pin_level(n) then levels = levels | (1 << n) end
    end
    local changed = levels ~ self.levels
    self.levels = levels

    self:check_interrupt(0, changed & 0xFF)
    self:check_interrupt(1, (changed >> 8) & 0xFF)
    self:update_int()

    if changed ~= 0 then
        for _, fn in ipairs(self.handlers) do fn(levels, changed) end
    end
end

-- SPI interface -----------------------------------------------------------------
function DEV:select()
    self.state_spi = "opcode"
end

function DEV:deselect()
    self.state_spi = "idle"
end

function DEV:transfer(byte)
    if self.held then return nil end
    local s = self.state_spi
    if s == "opcode" then
        local addr = (self.iocon & IOCON_HAEN ~= 0) and self.address or 0
        if (byte >> 4) == 0x4 and ((byte >> 1) & 7) == addr then
            self.reading = byte & 1 == 1
            self.state_spi = "address"
        else
            self.state_spi = "ignore"
        end
    elseif s == "address" then
        self.ptr = byte
        self.state_spi = "data"
    elseif s == "data" then
        local addr = self.ptr
        local v
        if self.reading then
            v = self:read_reg(addr)
        else
            self:write_reg(addr, byte)
        end
        self.ptr = self:next_address(addr)
        return v
    end
    return nil
end

function DEV:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

-- host interface ----------------------------------------------------------------
function DEV:set_input(pin, level)
    self.ext[pin_index(pin)] = to_level(level)
    self:update()
end

function DEV:release(pin)
    self:set_input(pin, nil)
end

function DEV:pin(pin)
    return (self.levels >> pin_index(pin)) & 1 == 1
end

function DEV:state()
    return self.levels
end

function DEV:port(port)
    return self:pins(port_index(port))
end

function DEV:register(name, port)
    name = tostring(name):upper()
    assert(REG_INDEX[name], "mcp23s17: unknown register: " .. name)
    if name == "IOCON" then return self.iocon end
    local p = port_index(port or "A")
    if name == "GPIO" then return self:gpio_value(p) end
    return self.r[p][name]
end

function DEV:interrupt(port)
    return self.r[port_index(port)].INTF ~= 0
end

function DEV:on_change(fn)
    assert(type(fn) == "function", "mcp23s17: function expected")
    table.insert(self.handlers, fn)
end

function DEV:connect_output(pin, sys, invert)
    local n = pin_index(pin)
    local function set()
        gpio.set(sys, self:pin(n) ~= (invert and true or false))
    end
    self:on_change(function(_, changed)
        if (changed >> n) & 1 == 1 then set() end
    end)
    set()
end

function DEV:connect_input(pin, sys, invert)
    local n = pin_index(pin)
    gpio.on_change(sys, function(level)
        self:set_input(n, level ~= (invert and true or false))
    end)
    self:set_input(n, gpio.get(sys) ~= (invert and true or false))
end

return M
