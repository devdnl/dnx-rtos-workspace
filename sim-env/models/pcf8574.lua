--[[----------------------------------------------------------------------------
  PCF8574 I2C 8-bit I/O expander model (quasi-bidirectional pins).

  Usage:
    local pcf8574 = require("models.pcf8574")
    local io8 = pcf8574.new{int = "PA5"}   -- optional INT output (active low)
    io8:attach("I2C1", 0x20)

  Pin level = output latch AND external level: pin with latch 1 is weak high
  (input), it can be pulled low by external device (drive()). Write to device
  sets latch, read returns pin levels. INT output is activated when input
  level changes and it is released by read or write of device.

  Methods:
    io8:attach(bus, address)    connect to bus
    io8:drive(pin, level)       drive pin by external device (level false: low)
    io8:release(pin)            stop driving pin
    io8:pins()                  pin levels (byte)
    io8:pin(n)                  pin level (boolean)
    io8:on_change(fn)           fn(latch, changed) called when latch changes
------------------------------------------------------------------------------]]

local M = {}

local PCF = {}
PCF.__index = PCF

function M.new(opts)
    opts = opts or {}
    local dev = setmetatable({}, PCF)
    dev.latch = 0xFF
    dev.ext_mask = 0
    dev.ext_level = 0
    dev.int = opts.int
    dev.handlers = {}
    dev.last = dev:pins()
    if dev.int then gpio.set(dev.int, true) end
    return dev
end

function PCF:pins()
    local ext = (self.ext_level & self.ext_mask) | (~self.ext_mask & 0xFF)
    return self.latch & ext
end

function PCF:pin(n)
    return (self:pins() >> n) & 1 == 1
end

function PCF:update_int()
    if self.int then
        gpio.set(self.int, self:pins() == self.last)
    end
end

function PCF:drive(n, level)
    assert(math.type(n) == "integer" and n >= 0 and n <= 7, "pcf8574: invalid pin")
    self.ext_mask = self.ext_mask | (1 << n)
    if level and level ~= 0 then
        self.ext_level = self.ext_level | (1 << n)
    else
        self.ext_level = self.ext_level & ~(1 << n)
    end
    self:update_int()
end

function PCF:release(n)
    assert(math.type(n) == "integer" and n >= 0 and n <= 7, "pcf8574: invalid pin")
    self.ext_mask = self.ext_mask & ~(1 << n)
    self:update_int()
end

function PCF:on_change(fn)
    assert(type(fn) == "function", "pcf8574: function expected")
    table.insert(self.handlers, fn)
end

-- bus interface
function PCF:select()
    return true
end

function PCF:write(byte)
    local changed = self.latch ~ byte
    self.latch = byte
    self.last = self:pins()
    self:update_int()
    if changed ~= 0 then
        for _, fn in ipairs(self.handlers) do fn(byte, changed) end
    end
    return true
end

function PCF:read()
    local v = self:pins()
    self.last = v
    self:update_int()
    return v
end

function PCF:attach(bus, address)
    i2c.attach(bus, address, self)
    return self
end

return M
