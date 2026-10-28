--[[----------------------------------------------------------------------------
  Example board: LEDs, buttons and simple logic connected to the system GPIO.

  Usage:
    make run-sim SIM_OPTS="--board sim-env/boards/example.lua"

  Pins (port A):
    PA0  LED (output of the system, active low)
    PA1  button (input of the system, pressed every 2 s for 200 ms, active low)
    PA2  -> PA3  inverter
    PA4  heartbeat (input of the system, toggled every 500 ms)

  I2C1:
    0x20  PCF8574 with 8 LEDs (printed when changed), INT -> PA5
    0x48  LM75 temperature sensor (slowly changing temperature), OS -> PA6
    0x50  24C256 EEPROM, content is kept in the host file example-24c256.bin
          (in the current directory)

  SPI1 (CS active low):
    PA7   25LC256 EEPROM, content is kept in the host file example-25lc256.bin
    PA15  SD card (SDHC), image in the host file example-sd.img (64 MiB)
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

-- button: idle state is high (pull-up), pressed state is low
gpio.set("PA1", true)
host.timer(2000, function()
    gpio.set("PA1", false)
    host.timer(200, function()
        gpio.set("PA1", true)
    end)
end, true)

-- logic gate between pins of the system
gpio.connect("PA2", "PA3", true)

-- heartbeat signal
local hb = false
host.timer(500, function()
    hb = not hb
    gpio.set("PA4", hb)
end, true)

-- I2C1: LEDs on I/O expander (active low)
local leds = pcf8574.new({ int = "PA5" }):attach("I2C1", 0x20)
leds:on_change(function(latch)
    local t = {}
    for n = 7, 0, -1 do
        t[#t + 1] = (latch >> n) & 1 == 0 and "*" or "."
    end
    print("LEDs: " .. table.concat(t))
end)

-- I2C1: temperature sensor, temperature changes between 20 and 30 C
local sensor = lm75.new({ temperature = 20, os = "PA6" }):attach("I2C1", 0x48)
host.timer(1000, function()
    sensor:set_temperature(25 + 5 * math.sin(host.time_ms() / 60000 * 2 * math.pi))
end, true)

-- I2C1: EEPROM
eeprom.new({ type = "24c256", file = "example-24c256.bin" }):attach("I2C1", 0x50)

-- SPI1: EEPROM and SD card
spiee.new({ type = "25lc256", file = "example-25lc256.bin" }):attach("SPI1", "PA7")
sdcard.new({ file = "example-sd.img", size = 64 * 1024 * 1024 }):attach("SPI1", "PA15")

-- SPI2: LEDs on shift register (active high)
local leds2 = hc595.new():attach("SPI2", "PA16")
leds2:on_change(function(state)
    local t = {}
    for n = 7, 0, -1 do
        t[#t + 1] = (state >> n) & 1 == 1 and "*" or "."
    end
    print("LEDs2: " .. table.concat(t))
end)

print("example board loaded")
