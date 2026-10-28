--[[----------------------------------------------------------------------------
  74HC595 shift register model (8-bit serial in, parallel out) connected to
  SPI: MOSI -> SER, SCK -> SRCLK, CS -> RCLK (outputs are latched at the
  rising edge of CS), QH' -> MISO. Chained registers are supported (chips).

  Usage:
    local hc595 = require("models.74hc595")
    local leds = hc595.new{outputs = {"PA20", "PA21"}}   -- optional GPIO outputs
    leds:attach("SPI2", "PA16")                          -- bus, CS/RCLK pin
    leds:on_change(function(state, changed) ... end)

  Options of new():
    chips       number of chained registers (default 1), the first byte
                transferred is shifted to the last register
    outputs     list of system input pins driven by outputs QA, QB, ...
                (connection of register outputs to GPIO pins)

  Methods:
    sr:attach(bus, cs)          connect to bus
    sr:state()                  latched outputs (integer, QA of first chip is bit 0)
    sr:output(n)                output n level (boolean)
    sr:on_change(fn)            fn(state, changed) called when outputs change
------------------------------------------------------------------------------]]

local M = {}

local HC595 = {}
HC595.__index = HC595

function M.new(opts)
    opts = opts or {}
    local sr = setmetatable({}, HC595)
    sr.chips = opts.chips or 1
    sr.mode = {[0] = true, [3] = true}
    sr.shift = {}
    for i = 1, sr.chips do sr.shift[i] = 0 end
    sr.latched = 0
    sr.outputs = opts.outputs or {}
    sr.handlers = {}
    sr:update_pins()
    return sr
end

function HC595:state()
    return self.latched
end

function HC595:output(n)
    return (self.latched >> n) & 1 == 1
end

function HC595:on_change(fn)
    assert(type(fn) == "function", "74hc595: function expected")
    table.insert(self.handlers, fn)
end

function HC595:update_pins()
    for i, name in ipairs(self.outputs) do
        gpio.set(name, self:output(i - 1))
    end
end

-- SPI interface: byte is shifted through all chips, QH' of the last chip is returned
function HC595:transfer(byte)
    local out = self.shift[self.chips]
    for i = self.chips, 2, -1 do self.shift[i] = self.shift[i - 1] end
    self.shift[1] = byte
    return out
end

function HC595:deselect()
    local state = 0
    for i = 1, self.chips do state = state | (self.shift[i] << (8 * (i - 1))) end
    local changed = state ~ self.latched
    self.latched = state
    if changed ~= 0 then
        self:update_pins()
        for _, fn in ipairs(self.handlers) do fn(state, changed) end
    end
end

function HC595:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
