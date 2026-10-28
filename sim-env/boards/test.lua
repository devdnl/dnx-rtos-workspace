--[[----------------------------------------------------------------------------
  Board used by automatic tests (tools/simtest.py, test programs tests/gpio.lua,
  tests/i2c.lua, tests/i2cee.lua, tests/spi.lua, tests/spiee.lua,
  tests/sdcard.lua).

  GPIO connections (system outputs -> system inputs):
    PA0  -> PA8   inverter
    PA1  -> PA9   wire
    PA10          pull-up (always high)
    PA11          pulse generator (toggled every 20 ms)
    PA2  -> PA12  T flip-flop (toggled by rising edge of PA2)

  I2C1:
    0x20  PCF8574, P0..P3 connected to P4..P7, INT -> PA13
    0x48  LM75 (21.5 C), OS -> PA14, PA3 = 1 heats sensor to 90 C
    0x50  24C256 (I2CEE driver and eefs)
    0x51  24C32 (raw access tests)
  I2C2:
    0x50  24C02

  SPI1 (CS pins, active low):
    PA4   test memory A: returns data of the last write transaction (mode 0)
    PA6   test memory B (mode 0)
    PA7   25LC256 EEPROM (SPIEE driver)
    PA15  SD card SDHC 16 MiB (SDSPI driver)
    PA17  SD card SD v1 2 MiB (SDSPI driver)
  SPI2:
    PA16  74HC595 (CS = RCLK), outputs QA..QH -> PA20..PA27
  SPI3:   no devices
------------------------------------------------------------------------------]]

local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")
local spiee = require("models.25xx")
local sdcard = require("models.sdcard")
local hc595 = require("models.74hc595")

gpio.connect("PA0", "PA8", true)
gpio.connect("PA1", "PA9")

gpio.set("PA10", true)

local pulse = false
host.timer(20, function()
    pulse = not pulse
    gpio.set("PA11", pulse)
end, true)

local q = false
gpio.set("PA12", q)
gpio.on_change("PA2", function(level)
    if level then
        q = not q
        gpio.set("PA12", q)
    end
end)

-- I2C1
local io8 = pcf8574.new{int = "PA13"}:attach("I2C1", 0x20)
io8:on_change(function(latch)
    for n = 0, 3 do io8:drive(n + 4, (latch >> n) & 1 == 1) end
end)

local sensor = lm75.new{temperature = 21.5, os = "PA14"}:attach("I2C1", 0x48)
gpio.on_change("PA3", function(level)
    sensor:set_temperature(level and 90 or 21.5)
end)

eeprom.new{type = "24c256"}:attach("I2C1", 0x50)
eeprom.new{type = "24c32"}:attach("I2C1", 0x51)

-- I2C2
eeprom.new{type = "24c02"}:attach("I2C2", 0x50)

-- SPI1: test memory, returns data of the last transaction that contains other
-- bytes than 0x00 and 0xFF (read transaction does not overwrite data)
local function spi_memory()
    local d = {mode = 0, buf = ""}
    function d:select() self.rx, self.n = {}, 0 end
    function d:transfer(byte)
        self.n = self.n + 1
        self.rx[self.n] = string.char(byte)
        return self.buf:byte(self.n) or 0xFF
    end
    function d:deselect()
        local data = table.concat(self.rx or {})
        if data:find("[^\0\xFF]") then self.buf = data end
    end
    return d
end

spi.attach("SPI1", "PA4", spi_memory())
spi.attach("SPI1", "PA6", spi_memory())
spiee.new{type = "25lc256"}:attach("SPI1", "PA7")
sdcard.new{type = "sdhc", size = 16 * 1024 * 1024}:attach("SPI1", "PA15")
sdcard.new{type = "sdv1", size = 2 * 1024 * 1024}:attach("SPI1", "PA17")

-- SPI2
hc595.new{outputs = {"PA20", "PA21", "PA22", "PA23", "PA24", "PA25", "PA26", "PA27"}}:attach("SPI2", "PA16")

print("test board loaded")
