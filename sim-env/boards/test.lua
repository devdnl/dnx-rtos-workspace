--[[----------------------------------------------------------------------------
  Board used by automatic tests (tools/simtest.py, test programs tests/gpio.lua,
  tests/i2c.lua, tests/i2cee.lua, tests/spi.lua, tests/spiee.lua,
  tests/sdcard.lua, tests/ds2482.lua, tests/mcp23s17.lua).

  GPIO connections (system outputs -> system inputs):
    PA0  -> PA8   inverter
    PA1  -> PA9   wire
    PA10          pull-up (always high)
    PA11          pulse generator (toggled every 20 ms)
    PA2  -> PA12  T flip-flop (toggled by rising edge of PA2)

  I2C1:
    0x18  DS2482-100, 1-Wire bus "a": DS18B20 sensors 1..7 (6: parasite
          power, 7: family 0x10), sensor 8 is connected by test (hot-plug),
          tests/ds2482.lua
    0x19  DS2482-100, 1-Wire bus "b": DS18B20 sensor 9
    0x20  PCF8574, P0..P3 connected to P4..P7, INT -> PA13
    0x30  1-Wire test control: write of text command, read of response
          (text terminated by 0): "init", "powercycle", "list",
          "temp <n> <C>", "attach <n>", "detach <n>", "short <a|b> <0|1>",
          "fault <n> <crc|rom_crc|mute> <0|1>"
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
  SPI3 (/dev/spi-none: no devices on its CS):
    PB2   MCP23S17 #0 (address 0) and #1 (address 1) on one CS, RESET <- PC16,
          tests/mcp23s17.lua:
          #0: GPA0..GPA7 -> PC0..PC7, PC8..PC15 -> GPB0..GPB7, INTA -> PC17,
              INTB -> PC18
          #1: GPA0..GPA7 -> GPB0..GPB7 (wire), INTA -> PC18 (shared with INTB of
              #0, wired-AND), INTB -> PC19
  SPI4:
    PB1   ST7789 LCD 240x320 (LCD "tft" of the panel), D/C -> PB0,
          tests/spilcd.lua

  Panel (tools/simtest.py, test of panel protocol, tests/panel.lua):
    P_OUT  shows PA18 (system output)
    P_IN   drives PA19 (system input)
    P_VAL  value, P_CTL control (P_VAL = 2 * P_CTL)
    touch of LCD "ltdc" (LTDC driver) -> FT5336 (I2C1 0x38, INT -> PA28), tests/lcd.lua
    touch of LCD "tft" (ST7789) -> P_TOUCH value: "<x> <y> <0|1> <lcd id>"
------------------------------------------------------------------------------]]

local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")
local spiee = require("models.25xx")
local sdcard = require("models.sdcard")
local hc595 = require("models.74hc595")
local ft5336 = require("models.ft5336")
local st7789 = require("models.st7789")
local ds2482 = require("models.ds2482")
local ds18b20 = require("models.ds18b20")
local mcp23s17 = require("models.mcp23s17")

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

-- I2C1: DS2482-100 bridges with DS18B20 sensors, ROM codes are selected for
-- SEARCH ROM: discrepancies in the family code, in the first and last bits of
-- serial number, sensors that differ only in CRC
local ow = {a = ds2482.new{}:attach("I2C1", 0x18), b = ds2482.new{}:attach("I2C1", 0x19)}

local OW_SENSORS = {
    {bus = "a", serial = 0x000000000001, temperature = 25.0625},
    {bus = "a", serial = 0x000000000003, temperature = -10.5},
    {bus = "a", serial = 0x800000000001, temperature = 0},
    {bus = "a", serial = 0x800000000000, temperature = 125},
    {bus = "a", serial = 0x7FFFFFFFFFFF, temperature = -55},
    {bus = "a", serial = 0x0000A1B2C3D4, temperature = 21.5, parasite = true},
    {bus = "a", serial = 0x000000000001, temperature = 30, family = 0x10},
    {bus = "a", serial = 0x123456789ABC, temperature = 40, unplugged = true},
    {bus = "b", serial = 0x00000000BEEF, temperature = 22.25},
}

local sensors = {}

local function ow_init()
    for _, s in ipairs(sensors) do s:detach() end
    for n, o in ipairs(OW_SENSORS) do
        sensors[n] = ds18b20.new{serial = o.serial, family = o.family, temperature = o.temperature,
                                 parasite = o.parasite}
        if not o.unplugged then sensors[n]:attach(ow[o.bus]) end
    end
    ow.a.ow:set_short(false)
    ow.b.ow:set_short(false)
end

ow_init()

local function sensor(n)
    return assert(sensors[tonumber(n)], "no sensor " .. tostring(n)), OW_SENSORS[tonumber(n)]
end

local OW_COMMANDS = {
    init = function() ow_init() end,
    powercycle = function()
        for _, s in ipairs(sensors) do s:power_cycle() end
    end,
    list = function()
        local t = {}
        for n, s in ipairs(sensors) do
            local rom = {}
            for i, b in ipairs(s:rom()) do rom[i] = string.format("%02X", b) end
            t[#t + 1] = string.format("%d %s %d %s %.4f %d", n, OW_SENSORS[n].bus, s.bus and 1 or 0,
                                      table.concat(rom), s:temperature(), s.parasite and 1 or 0)
        end
        return table.concat(t, "\n")
    end,
    temp = function(n, c) sensor(n):set_temperature(assert(tonumber(c), "temperature")) end,
    attach = function(n) local s, o = sensor(n); s:attach(ow[o.bus]) end,
    detach = function(n) sensor(n):detach() end,
    short = function(bus, on) assert(ow[bus], "no bus " .. tostring(bus)).ow:set_short(on == "1") end,
    fault = function(n, name, on) sensor(n):set_fault(name, on == "1") end,
}

local ow_ctl = {resp = "", pos = 1}
function ow_ctl:select(read)
    if not read then self.cmd = {} end
    self.pos = 1
    return true
end
function ow_ctl:write(byte) table.insert(self.cmd, string.char(byte)) end
function ow_ctl:read()
    local b = self.resp:byte(self.pos) or 0
    self.pos = self.pos + 1
    return b
end
function ow_ctl:stop()
    if not self.cmd then return end
    local args = {}
    for w in table.concat(self.cmd):gmatch("%S+") do args[#args + 1] = w end
    self.cmd = nil
    local fn = OW_COMMANDS[args[1] or ""]
    if not fn then
        self.resp = "error: unknown command " .. tostring(args[1])
        return
    end
    local ok, r = pcall(fn, table.unpack(args, 2))
    self.resp = ok and (r or "ok") or ("error: " .. tostring(r))
end
i2c.attach("I2C1", 0x30, ow_ctl)

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

-- SPI3: two MCP23S17 on one CS (hardware addresses 0 and 1)
local mcp0 = mcp23s17.new{address = 0, int_a = "PC17", int_b = "PC18", reset = "PC16"}:attach("SPI3", "PB2")
local mcp1 = mcp23s17.new{address = 1, int_a = "PC18", int_b = "PC19", reset = "PC16"}:attach("SPI3", "PB2")
for i = 0, 7 do
    mcp0:connect_output(i, "PC" .. i)
    mcp0:connect_input(8 + i, "PC" .. (8 + i))
end
local function mcp1_wire(_, changed)
    for i = 0, 7 do
        if changed == nil or (changed >> i) & 1 == 1 then mcp1:set_input(8 + i, mcp1:pin(i)) end
    end
end
mcp1:on_change(mcp1_wire)
mcp1_wire()

-- SPI2
hc595.new{outputs = {"PA20", "PA21", "PA22", "PA23", "PA24", "PA25", "PA26", "PA27"}}:attach("SPI2", "PA16")

-- panel
panel.output("P_OUT", "PA18")
panel.input("P_IN", "PA19")
panel.value("P_VAL", 0)
panel.control("P_CTL", function(v) panel.set("P_VAL", v * 2) end)

local ts = ft5336.new{int = "PA28"}:attach("I2C1", 0x38)
panel.on_touch(function(x, y, pressed) ts:touch(x, y, pressed) end, "ltdc")

-- SPI4: LCD of board model
panel.value("P_TOUCH", "")
st7789.new{id = "tft", width = 240, height = 320, dc = "PB0"}:attach("SPI4", "PB1")
panel.on_touch(function(x, y, pressed, id)
    if id ~= "ltdc" then panel.set("P_TOUCH", string.format("%d %d %d %s", x, y, pressed and 1 or 0, id)) end
end)

print("test board loaded")
