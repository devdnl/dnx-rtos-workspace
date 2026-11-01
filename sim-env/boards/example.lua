--[[----------------------------------------------------------------------------
  Example board: LEDs, buttons and simple logic connected to the system GPIO.

  Usage:
    make run-sim SIM_OPTS="--board sim-env/boards/example.lua"
    make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua"

  The board panel (PANEL=1, web browser: http://localhost:8080) shows page
  sim-env/panel/example/: LEDs, button, temperature, LEDs of the I/O expander
  and shift register.

  Pins (port A):
    PA0  LED (output of the system, active low)
    PA1  button (input of the system, active low, pressed by the panel)
    PA2  -> PA3  inverter
    PA4  heartbeat (input of the system, toggled every 500 ms)

  I2C1:
    0x20  PCF8574 with 8 LEDs (printed when changed), INT -> PA5
    0x48  LM75 temperature sensor (slowly changing temperature or set by the
          panel), OS -> PA6
    0x50  24C256 EEPROM, content is kept in the host file
          build/example-24c256.bin (build directory of the workspace)

  SPI1 (CS active low):
    PA7   25LC256 EEPROM, content is kept in the host file build/example-25lc256.bin
    PA15  SD card (SDHC), image in the host file build/example-sd.img (64 MiB)
  SPI2:
    PA16  74HC595 with 8 LEDs (printed when changed)

  dnx RTOS (shell), I2C EEPROM:
    modprobe -i I2C 0 0 /dev/i2c-ee -c address=0xA0,sub_addr_mode=2
    modprobe -i I2CEE 0 0 /dev/ee0 -c i2c_path=/dev/i2c-ee,memory_size=32768,page_size=64,page_prog_time_ms=5
    mkeefs /dev/ee0   (only once, EEPROM content is kept in the file)
    mkdir /mnt/ee; mount /dev/ee0 eefs /mnt/ee

  dnx RTOS (shell), SD card (all SPI1 CS pins should be configured first,
  not configured CS pin is low: device is selected):
    modprobe -i SPI 0 0 /dev/spi-ee -c cs_port=0,cs_pin=7
    modprobe -i SPI 0 1 /dev/spi-sd -c cs_port=0,cs_pin=15
    modprobe -i SDSPI 0 0 /dev/sdcard -c spi_path=/dev/spi-sd
    storinit /dev/sdcard
    mkext -i /dev/sdcard   (only once, card content is kept in the file)
    mkdir /mnt/sd; mount /dev/sdcard ext4fs /mnt/sd

  dnx RTOS (shell), SPI EEPROM and LEDs:
    modprobe -i SPIEE 0 0 /dev/ee1 -c spi_path=/dev/spi-ee,memory_size=32768,page_size=64,address_size=2
    modprobe -i SPI 1 0 /dev/leds2 -c cs_port=0,cs_pin=16
------------------------------------------------------------------------------]]

local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")
local spiee = require("models.25xx")
local sdcard = require("models.sdcard")
local hc595 = require("models.74hc595")

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

-- I2C1: EEPROM
-- files with content of memories are kept in the build directory of the
-- workspace (sim-env/boards/../../build), removed by make clean
local DATA = host.dir .. "/../../build/"

eeprom.new({ type = "24c256", file = DATA .. "example-24c256.bin" }):attach("I2C1", 0x50)

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

print("example board loaded")
