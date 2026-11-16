--[[----------------------------------------------------------------------------
  Board used by automatic tests (tools/simtest.py, test programs tests/gpio.lua,
  tests/i2c.lua, tests/i2cee.lua, tests/spi.lua, tests/spiee.lua,
  tests/sdcard.lua, tests/norflash.lua, tests/onewire.lua, tests/w5500.lua, tests/inet2.lua, tests/adc.lua, tests/ds2482.lua, tests/mcp23s17.lua, tests/uart.lua,
  tests/net.lua).

  GPIO connections (system outputs -> system inputs):
    PA0  -> PA8   inverter
    PA1  -> PA9   wire
    PA10          pull-up (always high)
    PA11          pulse generator (toggled every 20 ms)
    PA2  -> PA12  T flip-flop (toggled by rising edge of PA2)

  I2C:0:
    0x18  DS2482-100, 1-Wire bus "a": DS18B20 sensors 1..7 (6: parasite
          power, 7: family 0x10), sensor 8 is connected by test (hot-plug),
          tests/ds2482.lua
    0x19  DS2482-100, 1-Wire bus "b": DS18B20 sensor 9
    0x20  PCF8574, P0..P3 connected to P4..P7, INT -> PA13
    0x30  1-Wire and GPS test control: write of text command, read of response
          (text terminated by 0): "init", "powercycle", "list",
          "temp <n> <C>", "attach <n>", "detach <n>", "short <a|b> <0|1>",
          "fault <n> <crc|rom_crc|mute> <0|1>", "gps fix <0|1>",
          "gps pos <lat> <lon> <alt>", "gps motion <knots> <course>",
          "gps power <0|1>", "gps time" (board time in ms), "net ..."
          (Ethernet, see below)
    0x48  LM75 (21.5 C), OS -> PA14, PA3 = 1 heats sensor to 90 C
    0x50  24C256 (I2CEE driver and eefs)
    0x51  24C32 (raw access tests)
  I2C:1:
    0x2C  MAX3107, chip 1 (crystal 1.8432 MHz, tests/max310x.lua)
    0x2D  MAX3107 with wrong revision ID 0x00 (chip detection test)
    0x6C  MAX3109, chip 3 (UART 1 at 0x5C, external clock 3.6864 MHz)
    0x50  24C02

  UART (UART:1 /dev/ttyS1 is used by tools/simtest.py --uart 1=<pty>):
    UART:2 /dev/ttyS2  echo (transmission timing), "?cfg\n" is answered by
                      configuration: "<baud> <parity> <stop bits>\n"
    UART:3 /dev/ttyS3  NMEA GPS (38400 8N1, fix every 200 ms, start time
                      2026-01-01 12:00:00 UTC, 52.2297 N 21.0122 E 100 m)
    UART:4 /dev/ttyS4  serial line of MAX3109 (chip 0) UART 0 (SPI:1)
    UART:5 /dev/ttyS5  serial line of MAX3109 (chip 0) UART 1 (SPI:1)
    UART:6 /dev/ttyS6  1-Wire master (ONEWIRE driver, tests/onewire.lua): DS18B20
                      sensors 1..3 (3: parasite power), strong pull-up <- PB8;
                      control commands "owu temp <n> <C>", "owu short <0|1>",
                      "owu attach <n>", "owu detach <n>"

  Ethernet (network of the simulator 192.168.77.0/24, system 192.168.77.2),
  tests/net.lua:
    192.168.77.50  netdev: ping, UDP echo port 7 (answer in upper case), TCP
                   echo port 7, TCP source port 19 (sends "net src <n>" bytes
                   of pattern: byte i = i % 251, then closes), TCP sink port 9
                   (counts received bytes)
    192.168.77.51  UDP sensor: broadcast "T=<value>" to port 5000 (500 ms,
                   enabled by test)
    192.168.77.52  UDP sensor: unicast to the system port 5001, text by format
                   function "S52 <value> #<datagram number>", no periodic
                   transmissions (only after change and on request), value
                   also by the panel (P_ETH)
    control commands (I2C:0 0x30): "net sensor <value>", "net sensor_en <0|1>",
    "net sensor_tx" (number of datagrams), "net sensor_send", "net sensor2
    <value>", "net sensor2_send", "net sensor2_tx", "net src <n>", "net sink" (last
    connection: "<bytes> <sum32> <closed 0|1>"), "net connect <ip> <port> <text>"
    (TCP client sends text and closes), "net client" (state of client:
    "connecting", "sent", "refused", ...), "net udp <ip> <port> <text>",
    "net ping <ip>", "net pingres" ("ok <ms>", "fail", "pending"), "net conns"
    (open TCP connections of 192.168.77.50)

  SPI:0 (CS pins, active low):
    PA4   test memory A: returns data of the last write transaction (mode 0)
    PA6   test memory B (mode 0)
    PA7   25LC256 EEPROM (SPIEE driver)
    PA15  SD card SDHC 16 MiB (SDSPI driver)
    PA17  SD card SD v1 2 MiB (SDSPI driver)
    PB3   W25Q16 NOR flash 2 MiB, SFDP (NORFLASH driver, tests/norflash.lua)
    PB4   MX25L8005 NOR flash 1 MiB, no SFDP, deep power-down at start
    PB5   W25Q256 NOR flash 32 MiB, 4-byte address instructions
    PB6   SST26VF016 NOR flash 2 MiB, block protection set at power-up
    PB7   no device (NOR flash detection test)
    PB9   W5500 Ethernet controller (W5500 driver, tests/w5500.lua,
          tests/inet2.lua), RSTn <- PB10, connected to Ethernet segment "lan2"
          (192.168.78.0/24, separated from the network of /dev/eth):
          192.168.78.50 netdev (ping, UDP echo port 7 in upper case, TCP echo
          port 7), 192.168.78.51 netdev (ping); control commands "w5500 link
          <0|1>", "w5500 stats" ("<sent> <received> <dropped>"), "net2 ping
          <ip>" (ping from 192.168.78.50), "net2 pingres"
          control command "nor <n> stats" (n: 3..6 = PB3..PB6): "<sector
          erases> <page programs>"
  SPI:1:
    PA16  74HC595 (CS = RCLK), outputs QA..QH -> PA20..PA27
    PA29  MAX3109, chip 0 (external clock 3.6864 MHz), RST <- PA30, serial
          lines of UARTs -> UART:4, UART:5, tests/max310x.lua; control commands
          "max310x <chip> <uart> format" ("<baud> <data bits> <parity> <stop
          bits>" or "none"), "max310x <chip> <uart> reg <addr>" (register
          value), "max310x <chip> <uart> cts <0|1>" (CTS input), "max310x
          <chip> <uart> rts" (RTS output: 0|1)
    PA31  MAX3108, chip 2 (external clock 3.6864 MHz)
  SPI:2 (/dev/spi-none: no devices on its CS):
    PB2   MCP23S17 #0 (address 0) and #1 (address 1) on one CS, RESET <- PC16,
          tests/mcp23s17.lua:
          #0: GPA0..GPA7 -> PC0..PC7, PC8..PC15 -> GPB0..GPB7, INTA -> PC17,
              INTB -> PC18
          #1: GPA0..GPA7 -> GPB0..GPB7 (wire), INTA -> PC18 (shared with INTB of
              #0, wired-AND), INTB -> PC19
  SPI:3:
    PB1   ST7789 LCD 240x320 (LCD "tft" of the panel), D/C -> PB0,
          tests/spilcd.lua
    PD0   BT817 EVE3 video controller 480x272 (LCD "eve" of the panel), INT_N
          -> PD1, tests/bt817.lua; control commands "eve pixel <x> <y>" (pixel
          of LCD, RGB565 hex), "eve touch <x> <y> <0|1>", "eve touch_slot <n> <x>
          <y> <0|1>", "eve skipped" (swaps without render), "eve shown" (LCD
          shown in the panel)

  ADC (ADC driver, tests/adc.lua): ADC:0 channels 0..3: 0 V, 1.65 V, 3.3 V,
  1.0 V, ADC:1 channel 0: 0.5 V, temperature sensors 25 C; control commands
  "adc <n> <channel> <volts>", "adc <n> temp <C>"

  Panel (tools/simtest.py, test of panel protocol, tests/panel.lua):
    P_OUT  shows PA18 (system output)
    P_IN   drives PA19 (system input)
    P_VAL  value, P_CTL control (P_VAL = 2 * P_CTL)
    P_ETH* controls of UDP sensor 192.168.77.52 (udp_sensor.panel())
    touch of LCD "ltdc" (LTDC driver) -> FT5336 (I2C:0 0x38, INT -> PA28), tests/lcd.lua
    touch of LCD "tft" (ST7789) -> P_TOUCH value: "<x> <y> <0|1> <lcd id>"
------------------------------------------------------------------------------]]

local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")
local spiee = require("models.25xx")
local sdcard = require("models.sdcard")
local norflash = require("models.norflash")
local hc595 = require("models.74hc595")
local ft5336 = require("models.ft5336")
local st7789 = require("models.st7789")
local ds2482 = require("models.ds2482")
local ds18b20 = require("models.ds18b20")
local mcp23s17 = require("models.mcp23s17")
local nmea_gps = require("models.nmea_gps")
local netdev = require("models.netdev")
local udp_sensor = require("models.udp_sensor")
local bt817 = require("models.bt817")
local max310x = require("models.max310x")
local onewire_uart = require("models.onewire_uart")
local w5500 = require("models.w5500")

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

-- I2C:0
local io8 = pcf8574.new({ int = "PA13" }):attach("I2C:0", 0x20)
io8:on_change(function(latch)
    for n = 0, 3 do
        io8:drive(n + 4, (latch >> n) & 1 == 1)
    end
end)

local sensor = lm75.new({ temperature = 21.5, os = "PA14" }):attach("I2C:0", 0x48)
gpio.on_change("PA3", function(level)
    sensor:set_temperature(level and 90 or 21.5)
end)

eeprom.new({ type = "24c256" }):attach("I2C:0", 0x50)
eeprom.new({ type = "24c32" }):attach("I2C:0", 0x51)

-- I2C:0: DS2482-100 bridges with DS18B20 sensors, ROM codes are selected for
-- SEARCH ROM: discrepancies in the family code, in the first and last bits of
-- serial number, sensors that differ only in CRC
local ow = { a = ds2482.new({}):attach("I2C:0", 0x18), b = ds2482.new({}):attach("I2C:0", 0x19) }

local OW_SENSORS = {
    { bus = "a", serial = 0x000000000001, temperature = 25.0625 },
    { bus = "a", serial = 0x000000000003, temperature = -10.5 },
    { bus = "a", serial = 0x800000000001, temperature = 0 },
    { bus = "a", serial = 0x800000000000, temperature = 125 },
    { bus = "a", serial = 0x7FFFFFFFFFFF, temperature = -55 },
    { bus = "a", serial = 0x0000A1B2C3D4, temperature = 21.5, parasite = true },
    { bus = "a", serial = 0x000000000001, temperature = 30, family = 0x10 },
    { bus = "a", serial = 0x123456789ABC, temperature = 40, unplugged = true },
    { bus = "b", serial = 0x00000000BEEF, temperature = 22.25 },
}

local sensors = {}

local function ow_init()
    for _, s in ipairs(sensors) do
        s:detach()
    end
    for n, o in ipairs(OW_SENSORS) do
        sensors[n] = ds18b20.new({
            serial = o.serial,
            family = o.family,
            temperature = o.temperature,
            parasite = o.parasite,
        })
        if not o.unplugged then
            sensors[n]:attach(ow[o.bus])
        end
    end
    ow.a.ow:set_short(false)
    ow.b.ow:set_short(false)
end

ow_init()

local function sensor(n)
    return assert(sensors[tonumber(n)], "no sensor " .. tostring(n)), OW_SENSORS[tonumber(n)]
end

local OW_COMMANDS = {
    init = function()
        ow_init()
    end,
    powercycle = function()
        for _, s in ipairs(sensors) do
            s:power_cycle()
        end
    end,
    list = function()
        local t = {}
        for n, s in ipairs(sensors) do
            local rom = {}
            for i, b in ipairs(s:rom()) do
                rom[i] = string.format("%02X", b)
            end
            t[#t + 1] = string.format(
                "%d %s %d %s %.4f %d",
                n,
                OW_SENSORS[n].bus,
                s.bus and 1 or 0,
                table.concat(rom),
                s:temperature(),
                s.parasite and 1 or 0
            )
        end
        return table.concat(t, "\n")
    end,
    temp = function(n, c)
        sensor(n):set_temperature(assert(tonumber(c), "temperature"))
    end,
    attach = function(n)
        local s, o = sensor(n)
        s:attach(ow[o.bus])
    end,
    detach = function(n)
        sensor(n):detach()
    end,
    short = function(bus, on)
        assert(ow[bus], "no bus " .. tostring(bus)).ow:set_short(on == "1")
    end,
    fault = function(n, name, on)
        sensor(n):set_fault(name, on == "1")
    end,
}

-- UART:2: echo, UART:3: GPS (UARTs connected by simulator option --uart are not used)
local echo = { pending = "" }
function echo:receive(data)
    local out = {}
    for ch in data:gmatch(".") do
        local p = self.pending .. ch
        if p == "?cfg\n" then
            local c = uart.config("UART:2")
            out[#out + 1] = string.format("%d %s %d\n", c.baud, c.parity, c.stop_bits)
            p = ""
        elseif ("?cfg\n"):sub(1, #p) ~= p then
            out[#out + 1] = p -- not a query: echo
            p = ""
        end
        self.pending = p
    end
    if #out > 0 then
        uart.send("UART:2", table.concat(out))
    end
end
if uart.available("UART:2") then
    uart.attach("UART:2", echo)
end

local gps = nmea_gps.new({
    baud = 38400,
    rate = 200,
    start_time = 1767268800,
    lat = 52.2297,
    lon = 21.0122,
    alt = 100,
    ttff = { hot = 600, warm = 600, cold = 600 },
})
-- GPS is powered on by the UART test only: data of unread port is lost (receiver
-- overrun reported by board) and it is not needed by other tests
gps:set_enabled(false)
if uart.available("UART:3") then
    gps:attach("UART:3")
end

OW_COMMANDS.gps = function(what, a, b, c)
    if what == "power" then
        gps:set_enabled(a == "1")
    elseif what == "fix" then
        gps:set_fix(a == "1")
    elseif what == "pos" then
        gps:set_position(assert(tonumber(a)), assert(tonumber(b)), tonumber(c))
    elseif what == "motion" then
        gps:set_motion(assert(tonumber(a)), assert(tonumber(b)))
    elseif what == "time" then
        return string.format("%.0f", host.time_ms()) -- board time (GPS time = start_time + board time)
    else
        error("gps: unknown command " .. tostring(what))
    end
end

-- Ethernet devices
local net = netdev.new({ ip = "192.168.77.50" }):attach()
local net_state = { src = 0, sink = "0 0 0", client = "none", ping = "none" }

local udp_echo = net:udp(7)
udp_echo:on_receive(function(data, ip, port)
    udp_echo:send(ip, port, data:upper())
end)

net:tcp_listen(7, function(conn)
    conn:on_data(function(data)
        conn:send(data)
    end)
end)

net:tcp_listen(19, function(conn)
    local n = net_state.src
    local t = {}
    for i = 0, n - 1 do
        t[#t + 1] = string.char(i % 251)
    end
    conn:send(table.concat(t))
    conn:close()
end)

net:tcp_listen(9, function(conn)
    local bytes, sum = 0, 0
    net_state.sink = "0 0 0"
    conn:on_data(function(data)
        bytes = bytes + #data
        for i = 1, #data do
            sum = (sum + data:byte(i)) & 0xFFFFFFFF
        end
        net_state.sink = string.format("%d %d 0", bytes, sum)
    end)
    conn:on_close(function()
        net_state.sink = string.format("%d %d 1", bytes, sum)
    end)
end)

local sensor_eth = udp_sensor
    .new({
        ip = "192.168.77.51",
        port = 5000,
        value = 21.5,
        format = "T=%.1f",
        interval = 500,
        delay = 500,
        enabled = false,
    })
    :attach()

local sensor_eth2 = udp_sensor
    .new({
        ip = "192.168.77.52",
        port = 5001,
        dst = "192.168.77.2",
        value = 10,
        interval = 3600000,
        delay = 3600000,
        format = function(v, s)
            return string.format("S52 %.2f #%d", v, s:transmissions() + 1)
        end,
    })
    :attach()

-- SPI:3: BT817
local eve = bt817.new({ id = "eve", width = 480, height = 272, int = "PD1" })
eve:attach("SPI:3", "PD0")

OW_COMMANDS.eve = function(what, a, b, c, d)
    if what == "pixel" then
        return string.format("%04X", eve.lcd:pixel(assert(tonumber(a)), assert(tonumber(b))) or 0)
    elseif what == "touch" then
        eve:touch(assert(tonumber(a)), assert(tonumber(b)), c == "1")
    elseif what == "touch_slot" then -- "eve touch_slot <n> <x> <y> <0|1>"
        eve:touch_slot(assert(tonumber(a)), assert(tonumber(b)), assert(tonumber(c)), d == "1")
    elseif what == "skipped" then -- "eve skipped": swaps without render (unchanged frame)
        return tostring(eve.skipped or 0)
    elseif what == "shown" then
        return tostring(eve.lcd.enabled)
    else
        error("eve: unknown command " .. tostring(what))
    end
end

OW_COMMANDS.net = function(what, a, b, ...)
    if what == "sensor" then
        sensor_eth:set_value(assert(tonumber(a)))
    elseif what == "sensor_en" then
        sensor_eth:set_enabled(a == "1")
    elseif what == "sensor_tx" then
        return tostring(sensor_eth:transmissions())
    elseif what == "sensor_send" then
        return tostring(sensor_eth:send())
    elseif what == "sensor2" then
        sensor_eth2:set_value(assert(tonumber(a)))
    elseif what == "sensor2_send" then
        return tostring(sensor_eth2:send())
    elseif what == "sensor2_tx" then
        return tostring(sensor_eth2:transmissions())
    elseif what == "src" then
        net_state.src = assert(tonumber(a))
    elseif what == "sink" then
        return net_state.sink
    elseif what == "connect" then
        local text = table.concat({ ... }, " ")
        net_state.client = "connecting"
        net:tcp_connect(a, assert(tonumber(b)), function(conn, err)
            if not conn then
                net_state.client = err
                return
            end
            net_state.client = "connected"
            conn:on_close(function(reason)
                net_state.client = net_state.client .. " " .. reason
            end)
            conn:send(text)
            conn:close()
            net_state.client = "sent"
        end)
    elseif what == "client" then
        return net_state.client
    elseif what == "udp" then
        udp_echo:send(a, assert(tonumber(b)), table.concat({ ... }, " "))
    elseif what == "ping" then
        net_state.ping = "pending"
        net:ping(a, function(ok, ms)
            net_state.ping = ok and string.format("ok %.0f", ms) or "fail"
        end)
    elseif what == "pingres" then
        return net_state.ping
    elseif what == "conns" then
        local n = 0
        for _ in pairs(net.conns) do
            n = n + 1
        end
        return tostring(n)
    else
        error("net: unknown command " .. tostring(what))
    end
end

local ow_ctl = { resp = "", pos = 1 }
function ow_ctl:select(read)
    if not read then
        self.cmd = {}
    end
    self.pos = 1
    return true
end
function ow_ctl:write(byte)
    table.insert(self.cmd, string.char(byte))
end
function ow_ctl:read()
    local b = self.resp:byte(self.pos) or 0
    self.pos = self.pos + 1
    return b
end
function ow_ctl:stop()
    if not self.cmd then
        return
    end
    local args = {}
    for w in table.concat(self.cmd):gmatch("%S+") do
        args[#args + 1] = w
    end
    self.cmd = nil
    local fn = OW_COMMANDS[args[1] or ""]
    if not fn then
        self.resp = "error: unknown command " .. tostring(args[1])
        return
    end
    local ok, r = pcall(fn, table.unpack(args, 2))
    self.resp = ok and (r or "ok") or ("error: " .. tostring(r))
end
i2c.attach("I2C:0", 0x30, ow_ctl)

-- I2C:1
eeprom.new({ type = "24c02" }):attach("I2C:1", 0x50)

-- SPI:0: test memory, returns data of the last transaction that contains other
-- bytes than 0x00 and 0xFF (read transaction does not overwrite data);
-- block: transfers by dev:transfer_block()
local function spi_memory(block)
    local d = { mode = 0, buf = "" }
    if block then
        function d:transfer_block(data)
            local r = self.buf:sub(self.n + 1, self.n + #data)
            self.rx[#self.rx + 1] = data
            self.n = self.n + #data
            return r -- missing bytes: 0xFF
        end
    end
    function d:select()
        self.rx, self.n = {}, 0
    end
    function d:transfer(byte)
        self.n = self.n + 1
        self.rx[self.n] = string.char(byte)
        return self.buf:byte(self.n) or 0xFF
    end
    function d:deselect()
        local data = table.concat(self.rx or {})
        if data:find("[^\0\xFF]") then
            self.buf = data
        end
    end
    return d
end

spi.attach("SPI:0", "PA4", spi_memory())
spi.attach("SPI:0", "PA6", spi_memory(true))
-- second device on PA6: block transfer refused (nil: dev:transfer() for every
-- byte), sends 0xFF (data of PA6 memory combined by AND are not changed)
local spi_fallback = { mode = 0, count = 0 }
function spi_fallback:transfer_block()
    return nil
end
function spi_fallback:transfer()
    self.count = self.count + 1
    return 0xFF
end
spi.attach("SPI:0", "PA6", spi_fallback)
OW_COMMANDS.spi = function(what) -- "spi fallback": bytes received by transfer() of second PA6 device
    if what == "fallback" then
        return tostring(spi_fallback.count)
    end
end
spiee.new({ type = "25lc256" }):attach("SPI:0", "PA7")
sdcard.new({ type = "sdhc", size = 16 * 1024 * 1024 }):attach("SPI:0", "PA15")
sdcard.new({ type = "sdv1", size = 2 * 1024 * 1024 }):attach("SPI:0", "PA17")

-- UART:6: 1-Wire master on UART (ONEWIRE driver), tests/onewire.lua
local owu = onewire_uart.new({ spu = "PB8" }):attach("UART:6")
local owu_sensors = {
    ds18b20.new({ serial = 0x0000000A0001, temperature = 23.5 }),
    ds18b20.new({ serial = 0x8000000A0002, temperature = -5.25 }),
    ds18b20.new({ serial = 0x0000000A0003, temperature = 60.125, parasite = true }),
}
for _, s in ipairs(owu_sensors) do
    s:attach(owu)
end
OW_COMMANDS.owu = function(what, a, b)
    local s = owu_sensors[tonumber(a)]
    if what == "temp" and s then
        s:set_temperature(tonumber(b))
        return "ok"
    elseif what == "short" then
        owu.ow:set_short(a == "1")
        return "ok"
    elseif what == "attach" and s then
        s:attach(owu)
        return "ok"
    elseif what == "detach" and s then
        s:detach()
        return "ok"
    end
end

-- ADC (ADC driver), tests/adc.lua
local function adc_init()
    for ch, v in pairs({ [0] = 0, 1.65, 3.3, 1.0 }) do
        adc.set("ADC:0", ch, v)
    end
    adc.set("ADC:1", 0, 0.5)
end
adc_init()
OW_COMMANDS.adc = function(n, ch, v)
    local name = "ADC:" .. tostring(n)
    if ch == "temp" then
        adc.set_temperature(name, tonumber(v))
    elseif ch == "init" then
        adc_init()
    else
        adc.set(name, tonumber(ch), tonumber(v))
    end
    return "ok"
end

-- W5500 Ethernet controller (W5500 driver), tests/w5500.lua, tests/inet2.lua: Ethernet
-- segment "lan2" (192.168.78.0/24) with netdev models 192.168.78.50 (ping, UDP echo port 7
-- in upper case, TCP echo port 7) and 192.168.78.51 (ping)
local eth_w5500 = w5500.new({ reset = "PB10", segment = "lan2" }):attach("SPI:0", "PB9")
local net2 = netdev.new({ ip = "192.168.78.50", segment = "lan2" }):attach()
netdev.new({ ip = "192.168.78.51", segment = "lan2" }):attach()
local net2_udp = net2:udp(7)
net2_udp:on_receive(function(data, ip, port)
    net2_udp:send(ip, port, data:upper())
end)
net2:tcp_listen(7, function(conn)
    conn:on_data(function(data)
        conn:send(data)
    end)
end)
local net2_ping = "none"
OW_COMMANDS.net2 = function(what, a)
    if what == "ping" then
        net2_ping = "pending"
        net2:ping(a, function(ok, ms)
            net2_ping = ok and string.format("ok %.0f", ms) or "fail"
        end)
        return "ok"
    elseif what == "pingres" then
        return net2_ping
    end
end
OW_COMMANDS.w5500 = function(what, a)
    if what == "link" then
        eth_w5500:set_link(a == "1")
        return "ok"
    elseif what == "stats" then
        return string.format("%d %d %d", eth_w5500:stats())
    end
end

-- NOR flash memories (NORFLASH driver), tests/norflash.lua
local nor_chips = {
    [3] = norflash.new({ type = "w25q16" }):attach("SPI:0", "PB3"),
    [4] = norflash.new({ type = "mx25l8005", power_down = true }):attach("SPI:0", "PB4"),
    [5] = norflash.new({ type = "w25q256" }):attach("SPI:0", "PB5"),
    [6] = norflash.new({ type = "sst26vf016" }):attach("SPI:0", "PB6"),
}
OW_COMMANDS.nor = function(n, what)
    local chip = nor_chips[tonumber(n)]
    if chip and what == "stats" then
        return string.format("%d %d", chip:stats())
    end
end

-- SPI:2: two MCP23S17 on one CS (hardware addresses 0 and 1)
local mcp0 = mcp23s17.new({ address = 0, int_a = "PC17", int_b = "PC18", reset = "PC16" }):attach("SPI:2", "PB2")
local mcp1 = mcp23s17.new({ address = 1, int_a = "PC18", int_b = "PC19", reset = "PC16" }):attach("SPI:2", "PB2")
for i = 0, 7 do
    mcp0:connect_output(i, "PC" .. i)
    mcp0:connect_input(8 + i, "PC" .. (8 + i))
end
local function mcp1_wire(_, changed)
    for i = 0, 7 do
        if changed == nil or (changed >> i) & 1 == 1 then
            mcp1:set_input(8 + i, mcp1:pin(i))
        end
    end
end
mcp1:on_change(mcp1_wire)
mcp1_wire()

-- SPI:1
hc595.new({ outputs = { "PA20", "PA21", "PA22", "PA23", "PA24", "PA25", "PA26", "PA27" } }):attach("SPI:1", "PA16")

-- MAX310x: chips of tests/max310x.lua, chip with wrong revision ID
local max310x_chips = {
    [0] = max310x.new({ type = "max3109", clock = 3686400, reset = "PA30" }):attach("SPI:1", "PA29"),
    [1] = max310x.new({ type = "max3107", clock = 1843200 }):attach("I2C:1", 0x2C),
    [2] = max310x.new({ type = "max3108", clock = 3686400 }):attach("SPI:1", "PA31"),
    [3] = max310x.new({ type = "max3109", clock = 3686400 }):attach("I2C:1", 0x6C),
}
max310x.new({ revid = 0x00 }):attach("I2C:1", 0x2D)
if uart.available("UART:4") then
    max310x_chips[0]:uart(0):connect_uart("UART:4")
end
if uart.available("UART:5") then
    max310x_chips[0]:uart(1):connect_uart("UART:5")
end

OW_COMMANDS.max310x = function(c, n, what, a)
    local chip = assert(max310x_chips[tonumber(c)], "max310x: unknown chip")
    local u = chip:uart(tonumber(n))
    if what == "format" then
        local f = u:format()
        return f and string.format("%.0f %d %s %d", f.baud, f.data_bits, f.parity, f.stop_bits) or "none"
    elseif what == "reg" then
        return tostring(u:register(assert(tonumber(a))))
    elseif what == "cts" then
        u:set_cts(a == "1")
    elseif what == "rts" then
        return u:rts() and "1" or "0"
    else
        error("max310x: unknown command " .. tostring(what))
    end
end

-- panel
panel.output("P_OUT", "PA18")
panel.input("P_IN", "PA19")
panel.value("P_VAL", 0)
panel.control("P_CTL", function(v)
    panel.set("P_VAL", v * 2)
end)

local ts = ft5336.new({ int = "PA28" }):attach("I2C:0", 0x38)
panel.on_touch(function(x, y, pressed)
    ts:touch(x, y, pressed)
end, "ltdc")

-- SPI:3: LCD of board model
panel.value("P_TOUCH", "")
st7789.new({ id = "tft", width = 240, height = 320, dc = "PB0" }):attach("SPI:3", "PB1")
panel.on_touch(function(x, y, pressed, id)
    if id ~= "ltdc" then
        panel.set("P_TOUCH", string.format("%d %d %d %s", x, y, pressed and 1 or 0, id))
    end
end)

-- controls of UDP sensor 192.168.77.52: P_ETH, P_ETH_EN, P_ETH_SEND, P_ETH_TX, P_ETH_TEXT
udp_sensor.panel(sensor_eth2, "P_ETH", { label = "UDP sensor", min = -40, max = 85 })

print("test board loaded")
