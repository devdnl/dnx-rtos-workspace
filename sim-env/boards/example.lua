--[[----------------------------------------------------------------------------
  Example board: LEDs, buttons and simple logic connected to the system GPIO.

  Usage:
    make run-sim SIM_OPTS="--board sim-env/boards/example.lua"
    make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua"

  The board panel (PANEL=1, web browser: http://localhost:8080) shows page
  sim-env/panel/example/: LEDs, button, temperature, DS18B20 sensors on
  1-Wire bus (DS2482), LEDs of the I/O expander and shift register, LCD
  (LTDC driver) with touch panel, TFT display (ST7789 model on SPI3).

  Pins (port A):
    PA0  LED (output of the system, active low)
    PA1  button (input of the system, active low, pressed by the panel)
    PA2  -> PA3  inverter
    PA4  heartbeat (input of the system, toggled every 500 ms)

  I2C1:
    0x18  DS2482-100 1-Wire master with 3 DS18B20 sensors (temperatures set
          by the panel, sensor 3 with parasite power), EEPROM of sensors is
          kept in the host files build/example-ds18b20-<n>.bin
    0x20  PCF8574 with 8 LEDs (printed when changed), INT -> PA5
    0x38  FT5336 touch panel controller (touch of the LCD in the panel),
          INT -> PA9
    0x48  LM75 temperature sensor (slowly changing temperature or set by the
          panel), OS -> PA6
    0x50  24C256 EEPROM, content is kept in the host file
          build/example-24c256.bin (build directory of the workspace)

  SPI1 (CS active low):
    PA7   25LC256 EEPROM, content is kept in the host file build/example-25lc256.bin
    PA15  SD card (SDHC), image in the host file build/example-sd.img (64 MiB)
  SPI2:
    PA16  74HC595 with 8 LEDs (printed when changed)
  SPI3:
    PB1   ST7789 TFT display 240x320 (LCD "tft" of the panel), D/C -> PB0

  dnx RTOS (shell), I2C EEPROM:
    modprobe -i I2C 0 0 /dev/i2c-ee -c address=0x50,sub_addr_mode=2
    modprobe -i I2CEE 0 0 /dev/ee0 -c i2c_path=/dev/i2c-ee,memory_size=32768,page_size=64,page_prog_time_ms=5
    mkeefs /dev/ee0   (only once, EEPROM content is kept in the file)
    mkdir /mnt/ee; mount /dev/ee0 eefs /mnt/ee

  dnx RTOS (shell), DS2482 (1-Wire protocol is implemented by the program
  or driver that uses the device file):
    modprobe -i I2C 0 1 /dev/i2c-ow -c address=0x18

  dnx RTOS (shell), SD card (all SPI1 CS pins should be configured first,
  not configured CS pin is low: device is selected):
    modprobe -i SPI 0 0 /dev/spi-ee -c cs_port=0,cs_pin=7
    modprobe -i SPI 0 1 /dev/spi-sd -c cs_port=0,cs_pin=15
    modprobe -i SDSPI 0 0 /dev/sdcard -c spi_path=/dev/spi-sd
    storinit /dev/sdcard
    mkext -i /dev/sdcard   (only once, card content is kept in the file)
    mkdir /mnt/sd; mount /dev/sdcard ext4fs /mnt/sd

  dnx RTOS (shell), LCD and touch panel (demo sim-env/demos/lcdpaint.lua,
  simulator option --hostfs sim-env):
    modprobe -i LTDC 0 0 /dev/lcd -c pixel_format=rgb565
    modprobe -i I2C 0 5 /dev/i2c-ts -c address=0x38,sub_addr_mode=1
    lua /host/demos/lcdpaint.lua

  dnx RTOS (shell), TFT display (demo sim-env/demos/tftdemo.lua, simulator
  option --hostfs sim-env, D/C pin is controlled by /dev/gpiob):
    modprobe -i GPIO 1 0 /dev/gpiob
    modprobe -i SPI 2 0 /dev/spi-tft -c cs_port=1,cs_pin=1
    lua /host/demos/tftdemo.lua

  dnx RTOS (shell), SPI EEPROM and LEDs:
    modprobe -i SPIEE 0 0 /dev/ee1 -c spi_path=/dev/spi-ee,memory_size=32768,page_size=64,address_size=2
    modprobe -i SPI 1 0 /dev/leds2 -c cs_port=0,cs_pin=16
------------------------------------------------------------------------------]]

local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")
local ds2482 = require("models.ds2482")
local ds18b20 = require("models.ds18b20")
local spiee = require("models.25xx")
local ft5336 = require("models.ft5336")
local sdcard = require("models.sdcard")
local hc595 = require("models.74hc595")
local st7789 = require("models.st7789")

-- LED: print state when system changes the output
gpio.on_change("PA0", function(level)
    print(string.format("%8.1f ms: LED %s", host.time_ms(), level and "off" or "on"))
end)
panel.output("LED", "PA0", {active_low = true, label = "LED (PA0)", color = "#22c55e"})

-- button on the panel: idle state is high (pull-up), pressed state is low
panel.input("BUTTON", "PA1", {active_low = true, label = "Button (PA1)"})

-- logic gate between pins of the system
gpio.connect("PA2", "PA3", true)
panel.output("PA2", "PA2", {label = "Inverter input (PA2)"})
panel.output("PA3", "PA3", {label = "Inverter output (PA3)"})

-- heartbeat signal
local hb = false
host.timer(500, function()
    hb = not hb
    gpio.set("PA4", hb)
end, true)
panel.output("HEARTBEAT", "PA4", {label = "Heartbeat (PA4)", color = "#3b82f6"})

-- I2C1: LEDs on I/O expander (active low)
local leds = pcf8574.new({ int = "PA5" }):attach("I2C1", 0x20)
panel.value("LEDS1", leds.latch, {label = "PCF8574 LEDs (active low)", format = "0x%02X"})
leds:on_change(function(latch)
    local t = {}
    for n = 7, 0, -1 do
        t[#t + 1] = (latch >> n) & 1 == 0 and "*" or "."
    end
    print("LEDs: " .. table.concat(t))
    panel.set("LEDS1", latch)
end)

-- I2C1: temperature sensor, temperature changes between 20 and 30 C
-- (automatic mode) or is set by the panel
local sensor = lm75.new({ temperature = 20, os = "PA6" }):attach("I2C1", 0x48)
local auto = true
host.timer(1000, function()
    if auto then
        sensor:set_temperature(25 + 5 * math.sin(host.time_ms() / 60000 * 2 * math.pi))
    end
end, true)
panel.value("TEMP", function() return sensor:temperature() end,
            {period = 200, label = "LM75 temperature", unit = "°C", format = "%.1f"})
panel.control("TEMP_AUTO", function(v) auto = v end, {value = true, label = "Automatic temperature"})
panel.control("TEMP_SET", function(v)
    auto = false
    panel.set("TEMP_AUTO", false)
    sensor:set_temperature(v)
end, {value = 25, min = -20, max = 100, step = 0.5, label = "Set temperature", unit = "°C"})
panel.output("OS", "PA6", {active_low = true, label = "LM75 OS (PA6)", color = "#f59e0b"})

-- I2C1: touch panel controller of the LCD (LTDC driver), touch of the LCD in
-- the board panel
local ts = ft5336.new({ int = "PA9" }):attach("I2C1", 0x38)
panel.on_touch(function(x, y, pressed) ts:touch(x, y, pressed) end, "ltdc")
panel.output("TOUCH", "PA9", {active_low = true, label = "Touch INT (PA9)", color = "#a855f7"})

-- I2C1: EEPROM
-- files with content of memories are kept in the build directory of the
-- workspace (sim-env/boards/../../build), removed by make clean
local DATA = host.dir .. "/../../build/"

eeprom.new({ type = "24c256", file = DATA .. "example-24c256.bin" }):attach("I2C1", 0x50)

-- I2C1: DS2482-100 with three DS18B20 sensors on 1-Wire bus (sensor 3:
-- parasite power), temperatures are set by the panel, sensors can be
-- disconnected (hot-plug) and the 1-Wire line shorted
local ow = ds2482.new():attach("I2C1", 0x18)
local OW_SENSORS = {
    {serial = 0x00000A1B2C01, temperature = 21.5},
    {serial = 0x00000A1B2C02, temperature = 4.0},
    {serial = 0x00000A1B2C03, temperature = -12.25, parasite = true},
}
for n, o in ipairs(OW_SENSORS) do
    local s = ds18b20.new({ serial = o.serial, temperature = o.temperature, parasite = o.parasite,
                            file = DATA .. "example-ds18b20-" .. n .. ".bin" }):attach(ow)
    local id = "OW" .. n
    panel.value(id .. "_ROM", s:rom_string(), {label = "DS18B20 " .. n .. " ROM"})
    panel.value(id .. "_REG", function() return s:register() end,
                {period = 250, label = "DS18B20 " .. n .. " last conversion", unit = "°C", format = "%.4f"})
    panel.control(id .. "_SET", function(v) s:set_temperature(v) end,
                  {value = o.temperature, min = -55, max = 125, step = 0.25,
                   label = "DS18B20 " .. n .. " temperature", unit = "°C"})
    panel.control(id .. "_PLUG", function(v)
        if v then s:attach(ow) else s:detach() end
        print(string.format("DS18B20 %s %s", s:rom_string(), v and "connected" or "disconnected"))
    end, {value = true, label = "DS18B20 " .. n .. " connected"})
end
panel.control("OW_SHORT", function(v) ow.ow:set_short(v) end, {value = false, label = "1-Wire line shorted"})

-- SPI1: EEPROM and SD card
spiee.new({ type = "25lc256", file = DATA .. "example-25lc256.bin" }):attach("SPI1", "PA7")
sdcard.new({ file = DATA .. "example-sd.img", size = 64 * 1024 * 1024 }):attach("SPI1", "PA15")

-- SPI2: LEDs on shift register (active high)
local leds2 = hc595.new():attach("SPI2", "PA16")
panel.value("LEDS2", leds2:state(), {label = "74HC595 LEDs", format = "0x%02X"})
leds2:on_change(function(state)
    local t = {}
    for n = 7, 0, -1 do
        t[#t + 1] = (state >> n) & 1 == 1 and "*" or "."
    end
    print("LEDs2: " .. table.concat(t))
    panel.set("LEDS2", state)
end)

-- SPI3: TFT display (LCD "tft" of the panel), touch of the display in the
-- board panel is only shown
local tft = st7789.new({ id = "tft", width = 240, height = 320, dc = "PB0" }):attach("SPI3", "PB1")
panel.value("TFT_TOUCH", "", {label = "TFT touch"})
tft:on_touch(function(x, y, pressed) panel.set("TFT_TOUCH", pressed and string.format("%d, %d", x, y) or "") end)

print("example board loaded")
