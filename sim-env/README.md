# Simulation environment (sim-env)

The simulation environment describes the hardware connected to the
microcontroller when dnx RTOS is started in the POSIX simulator. The whole
board (devices and connections between them) is described by a single Lua
script selected by the simulator option `--board`:
```
make run-sim SIM_OPTS="--board sim-env/boards/example.lua"
build/dnx-sim --disk 0=build/sim-romfs.bin:ro --board sim-env/boards/example.lua
```

Directories:
- `boards/` – board descriptions (`example.lua`, `test.lua` used by
  `tools/simtest.py`),
- `models/` – library of device models used by boards (`require("models.<name>")`):
  `24cxx` (I2C EEPROM), `pcf8574` (I2C I/O expander), `lm75` (I2C temperature
  sensor), `25xx` (SPI EEPROM), `sdcard` (SD card in SPI mode, host image
  file), `74hc595` (SPI shift register), `ft5336` (I2C touch panel
  controller), `st7789` (SPI TFT LCD controller, ST7789/ILI9341 compatible,
  shown by the board panel), `ds2482` (I2C to 1-Wire bridge DS2482-100),
  `ds18b20` (1-Wire thermometer), `onewire` (1-Wire bus used by them, see
  [1-Wire](#1-wire)), `mcp23s17` (SPI 16-bit I/O expander, see
  [MCP23S17](#mcp23s17)), `nmea_gps` (NMEA GPS receiver on UART, see
  [uart](#uart)), the description of options and methods is in
  the model file, see [Writing device models](#writing-device-models),
- `tests/` – test programs executed in dnx RTOS (e.g. `lua /host/gpio.lua`),
- `testappset/` – test appset linked with the simulator by `make test-sim`
  (`SIM_APPSETS`, program `appsettest`),
- `demos/` – demo programs executed in dnx RTOS, e.g. `lcdpaint.lua` (drawing
  on the LCD of the board panel, see [LCD and touch](#lcd-and-touch)),
  `tftdemo.lua` (drawing on the SPI TFT display, ST7789 model),
- `panel/` – pages of the board panel shown in a web browser (`default/` –
  generic page, `example/` – page of `boards/example.lua`), see
  [Board panel](#board-panel).

The board engine is a part of the simulator in the kernel
(`kernel/src/cpu/posix/host/board.c`, Lua 5.5 in
`kernel/src/cpu/posix/host/lua`).

## How it works
The script is executed in a separate host thread that
owns the Lua state. Events of the system (e.g. GPIO output changes) are queued
and handled in this thread one by one, so the behavior of the board is
repeatable. The script is loaded before the system start; an error in the
script stops the simulator, an error in a callback is printed and the board
works further.

Messages of the board (`print()`) are printed to the standard error output of
the simulator with the `[board]` prefix. Modules are searched in the script
directory and in its parent directory (`sim-env`).

## API

### host
| Function | Description |
|----------|-------------|
| `host.time_ms()` | time from board start in milliseconds |
| `host.timer(ms, fn [, periodic])` | call `fn(timer)` after `ms` (every `ms` if `periodic`), returns timer, `timer:stop()` stops it |
| `host.log(...)`, `print(...)` | print message |
| `host.on_exit(fn)` | call `fn()` at simulator exit or restart (e.g. save model state) |
| `host.script`, `host.dir` | path of board script and its directory (absolute), e.g. for files of models: relative paths are relative to the current directory of the simulator |

### gpio
Pins are named in STM32 style: `PA0` (port A, pin 0) .. `PZ31`. The port index
is the major number of the `GPIO` driver (`/dev/gpio` is port A). Levels are
booleans (`true` – high); numbers `0`/`1` are accepted too.

| Function | Description |
|----------|-------------|
| `gpio.set(pin, level)` | drive input of the system (the system reads this level) |
| `gpio.release(pin)` | stop driving, the system reads its own output state |
| `gpio.get(pin)` | level seen by the system |
| `gpio.on_change(pin, fn)` | call `fn(level, pin)` when the system changes output state of pin |
| `gpio.connect(out, in [, invert])` | connect output of the system to its input (wire or inverter) |

Example:
```lua
gpio.connect("PA2", "PA3", true)                  -- inverter PA2 -> PA3
gpio.on_change("PA0", function(level)             -- LED
    print("LED", level and "off" or "on")
end)
host.timer(500, function() gpio.set("PA4", not gpio.get("PA4")) end, true)
```

### i2c
Buses are named in STM32 style: `I2C1` (major 0 of the `I2C` driver) ..
`I2C16`. The I2C driver of the simulator supports master mode only. Device
addresses are 7-bit (as in datasheets), the same in the board model and in
the driver configuration (e.g. `0x50`).

| Function | Description |
|----------|-------------|
| `i2c.attach(bus, address, dev [, ten_bit])` | connect device model (7-bit address, 10-bit if `ten_bit`), returns `dev` |
| `i2c.detach(bus, address [, ten_bit])` | disconnect device |
| `i2c.device(bus, address [, ten_bit])` | device connected to address |

Device model is a table with optional methods (called with the device as
`self`). An error in a method is printed and the transfer is not acknowledged.

| Method | Description |
|--------|-------------|
| `dev:select(read)` | address acknowledged (START + address), `read`: read transfer; `false` result: NACK (e.g. busy device) |
| `dev:write(byte)` | byte received from the system; `false` result: NACK (transfer is stopped) |
| `dev:read(last)` | byte sent to the system (integer, `nil`: 0xFF); `last`: last byte of transfer |
| `dev:stop()` | STOP condition (called once for each device selected in transaction) |

Repeated START is not passed to the device: the next `select()` is called.
Without the board model buses are empty (devices do not acknowledge address).

Example – register based device:
```lua
local dev = {regs = {[0] = 0x12, [1] = 0x34}, ptr = 0}

function dev:select(read) self.first = not read; return true end
function dev:write(byte)
    if self.first then self.ptr = byte; self.first = false
    else self.regs[self.ptr] = byte; self.ptr = self.ptr + 1 end
end
function dev:read() local v = self.regs[self.ptr]; self.ptr = self.ptr + 1; return v end

i2c.attach("I2C1", 0x3C, dev)
```

Example – models:
```lua
local eeprom = require("models.24cxx")
local pcf8574 = require("models.pcf8574")
local lm75 = require("models.lm75")

eeprom.new{type = "24c256", file = "eeprom.bin"}:attach("I2C1", 0x50)
local leds = pcf8574.new{int = "PA5"}:attach("I2C1", 0x20)
local sensor = lm75.new{temperature = 21.5, os = "PA6"}:attach("I2C1", 0x48)
host.timer(1000, function() sensor:set_temperature(20 + math.random() * 5) end, true)
```

Devices are created in dnx RTOS by the `I2C` driver, e.g. by `modprobe` with
configuration string (driver `ioctl()` `IOCTL_DEVICE__CONFIGURE_STR`):
```
modprobe -i I2C 0 0 /dev/i2c-ee -c address=0x50,sub_addr_mode=2
modprobe -i I2CEE 0 0 /dev/ee0 -c i2c_path=/dev/i2c-ee,memory_size=32768,page_size=64,page_prog_time_ms=5
mkeefs /dev/ee0
mkdir /mnt/ee && mount /dev/ee0 eefs /mnt/ee
```
File position of I2C device file is the sub-address (register or memory
address) when `sub_addr_mode` is set (1..4 bytes).

### spi
Buses are named in STM32 style: `SPI1` (major 0 of the `SPI` driver) ..
`SPI16`. Device is selected by its CS pin: the SPI driver of dnx RTOS
controls CS by the GPIO driver (configuration `CS_port_idx`, `CS_pin_idx`),
the board receives CS changes as GPIO events before the transfer, so CS
timing is the same as on the hardware. Not configured CS pin is low (output
state of GPIO port), so all SPI devices of the board should be configured in
dnx before the first transfer (or CS should be active high).

| Function | Description |
|----------|-------------|
| `spi.attach(bus, cs, dev [, opts])` | connect device model selected by `cs` pin (active low, `opts.cs_active_high`: active high, `nil`: always selected), returns `dev` |

Device model is a table with optional fields and methods:

| Field / method | Description |
|----------------|-------------|
| `dev.mode` | supported SPI mode (0..3) or set of modes (`{[0] = true, [3] = true}`), other mode: device does not respond (warning is printed) |
| `dev.lsb_first` | device bit order is LSB first (data are bit reversed when the transfer uses other order) |
| `dev:select()` | CS activated |
| `dev:transfer(byte)` | byte received from the system, returns byte sent to the system (`nil`: 0xFF) |
| `dev:deselect()` | CS deactivated |

When more devices are selected, received bytes are combined by AND (bus
conflict). Without devices the system receives 0xFF.

Example – shift register:
```lua
local sr = {state = 0}
function sr:transfer(byte) self.next = byte end
function sr:deselect() if self.next then self.state = self.next; print("outputs", self.state) end end
spi.attach("SPI2", "PA16", sr)
```

Devices are created in dnx RTOS by the `SPI` driver, e.g. by `modprobe` with
configuration string (`clk_divider`, `mode`, `msb_first`, `cs_port`,
`cs_pin`, `cs_reverse`):
```
modprobe -i SPI 0 0 /dev/spi-sd -c cs_port=0,cs_pin=15
modprobe -i SDSPI 0 0 /dev/sdcard -c spi_path=/dev/spi-sd
storinit /dev/sdcard
modprobe -i SPI 0 1 /dev/spi-ee -c cs_port=0,cs_pin=7,mode=0
modprobe -i SPIEE 0 0 /dev/ee1 -c spi_path=/dev/spi-ee,memory_size=32768,page_size=64,address_size=2
```

### uart
UARTs of the system not connected by the simulator option `--uart` (and not
UART1, the simulator terminal) can be connected to board models. UART names
are in STM32 style: `UART1` is `/dev/ttyS0` (option `--uart 0`), `UART2` is
`/dev/ttyS1` and so on. `--uart <n>=board` connects UART explicitly (a warning
is printed when the board script does not attach a model).
```lua
local dev = {}
function dev:receive(data)                    -- data sent by the system (string)
    uart.send("UART2", data:upper())          -- data sent to the system
end
function dev:configure(cfg)                   -- configuration set by the system
    print(cfg.baud, cfg.parity, cfg.stop_bits) -- 9600 none 1 (parity: none, odd, even)
end
if uart.available("UART2") then               -- not connected by option --uart
    uart.attach("UART2", dev)                 -- opts.timing = false: no transmission time
end
print(uart.config("UART2").baud, uart.pending("UART2"))
```
Data is transferred with transmission time of characters (start bit, 8 data
bits, parity, stop bits at the baud rate configured by the system): the model
gets bytes sent by the system when they are transmitted, bytes sent by the
model are available to the system at the time of their reception, so the
system sees the real throughput of the line (option `timing = false`: data is
transferred immediately). Bytes received by the system and not read in 200 ms
are lost (receiver overrun, reported once); data sent by model faster than the
baud rate allows is lost when the buffer (4096 bytes) is full. The model
should check the configuration (`dev:configure()`, `uart.config()`): a model of
a device with fixed baud rate sends unreadable data when the system uses
another baud rate or parity (as `nmea_gps`).

`models/nmea_gps.lua` is a GPS receiver (MediaTek MT3339 style): sentences
GGA, GSA, GSV, RMC, VTG (GLL, ZDA) of each fix, position, fix, speed and
course set by the board, PMTK commands (fix interval, sentence output, baud
rate, restart, firmware query):
```lua
local nmea_gps = require("models.nmea_gps")

local gps = nmea_gps.new{lat = 52.2297, lon = 21.0122, alt = 110, baud = 9600}
gps:attach("UART2")
gps:set_motion(12.5, 90)                      -- knots, course
gps:set_fix(false)
```
In dnx RTOS:
```
modprobe -c /dev/ttyS1 baud=9600
cat /dev/ttyS1
```
`tests/uart.lua` tests UARTs of `boards/test.lua` (echo with configuration
query on UART3, GPS on UART4).

### MCP23S17
`models/mcp23s17.lua` is the 16-bit I/O expander with SPI (mode 0 or 3): all
registers in both address maps (IOCON.BANK), sequential and byte mode
(SEQOP), hardware addresses (HAEN: up to 8 devices on one CS pin, attach all
of them to the same pin), input polarity, pull-ups, interrupt-on-change
(INTCON/DEFVAL, INTF, INTCAP cleared by read of GPIO or INTCAP) and INT pins
(MIRROR, ODR, INTPOL; INT lines of more devices on one system pin are
wired-AND, so open drain outputs can share an interrupt line), RESET pin.
Pins are connected to the system GPIO or driven by the board:
```lua
local mcp23s17 = require("models.mcp23s17")

local exp = mcp23s17.new{address = 0, int_a = "PB3", int_b = "PB4", reset = "PB5"}
exp:attach("SPI3", "PB2")
for i = 0, 7 do exp:connect_output("A" .. i, "PC" .. i) end   -- GPA0..7 -> PC0..7
exp:connect_input("B0", "PC8")                                 -- system output -> GPB0
exp:set_input("B7", false)                                     -- button pressed (low)
exp:release("B7")                                              -- released (pull-up GPPU)
exp:on_change(function(state, changed) print(string.format("pins 0x%04X", state)) end)
```
A register is read in one transfer while CS is active (opcode, address, data),
so the SPI device file in dnx RTOS is used by full duplex transfers: in Lua by
`dnx.spi.transceive(path, frame [, frame ...])` (`luadnx`, all frames in one
CS activation, returns received frames), in C by `IOCTL_SPI__TRANSCEIVE`.
`tests/mcp23s17.lua` contains register access functions and tests of two
expanders on one CS (`boards/test.lua`):
```lua
local spi = require("dnx.spi")
local DEV = "/dev/spi-mcp"   -- modprobe -i SPI 2 11 /dev/spi-mcp -c cs_port=1,cs_pin=2

local function wr(hw, reg, ...) spi.transceive(DEV, string.char(0x40 | (hw << 1), reg, ...)) end
local function rd(hw, reg)
    local r = spi.transceive(DEV, string.char(0x41 | (hw << 1), reg, 0))
    return r:byte(3)
end

wr(0, 0x00, 0x00)            -- IODIRA: port A outputs
wr(0, 0x14, 0xA5)            -- OLATA
print(rd(0, 0x13))           -- GPIOB
```

### 1-Wire
1-Wire devices are connected to the 1-Wire bus (`models/onewire.lua`) of the
master model, the DS2482-100 I2C bridge (`models/ds2482.lua`, addresses
0x18..0x1B). The bus is modeled by time slots, not by bytes: in each slot the
master writes one bit (bit 1 is the read slot), every device can pull the line
low and all of them sample the resulting level (wired-AND). The master does
not know commands of devices, so SEARCH ROM and ALARM SEARCH work as on the
real line for any number of devices and any ROM codes, also when the search
is done by single bits (DS2482 `1WSB`) instead of triplets (`1WT`), and READ
ROM with more devices returns the wired-AND of their codes.
```lua
local ds2482 = require("models.ds2482")
local ds18b20 = require("models.ds18b20")

local ow = ds2482.new{}:attach("I2C1", 0x18)
local s1 = ds18b20.new{serial = 0x00000A1B2C01, temperature = 21.5}:attach(ow)
local s2 = ds18b20.new{serial = 0x00000A1B2C02, parasite = true}:attach(ow)
s1:set_temperature(-10.125)
s2:detach()                       -- hot-plug: disconnected device
ow.ow:set_short(true)             -- 1-Wire line shorted to ground
s1:set_fault("crc", true)         -- wrong CRC of scratchpad
```
The DS2482 model sets the busy bit (`1WB`) for the time of the 1-Wire
operation (reset about 1.15 ms, time slot 72.5 µs) and its results (PPD, SBR,
TSB, DIR, read data) are visible after the operation is finished, so the
driver has to poll the status register as on the hardware (option `timing =
false`: operations are finished immediately). Commands sent while the line is
busy are ignored (a warning is printed). The DS18B20 model supports all ROM
and function commands of the device: conversion time depends on resolution
(read slots return 0 during conversion), the temperature register is 85 °C
after power on, EEPROM (TH, TL, configuration) can be kept in the host file.
Devices with parasite power need the strong pull-up of the master (DS2482
`SPU` bit set before CONVERT T or COPY SCRATCHPAD), otherwise the operation
fails (temperature register is not changed, a warning is printed). Faults:
`set_fault("crc" | "rom_crc" | "mute", on)` of the sensor, short of the line.
Other 1-Wire devices or masters (e.g. DS2482-800, bit-banged GPIO master) can
be written for the same bus interface (described in `models/onewire.lua`).

In dnx RTOS the DS2482 is used by the I2C device without sub-address, the
1-Wire protocol is implemented by the program or driver (`tests/ds2482.lua`
contains a Lua implementation: reset, byte and bit transfers, triplets,
search by Maxim AN187, MATCH ROM, conversion, scratchpad with CRC):
```
modprobe -i I2C 0 1 /dev/i2c-ow -c address=0x18
```

## Board panel
The board can be visualized in a web browser: LEDs, buttons, switches, values
of device models, etc. The board script defines *signals* of the panel (what
is shown and controlled), the look of the panel is defined by HTML pages in
`panel/<page>/` (fully defined by the user: images, CSS, JavaScript).
```
make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua"
```
then open `http://localhost:8080` (the Docker container uses the host
network on Linux). `PANEL=1` starts `tools/simpanel.py` in background (log:
`build/simpanel.log`) and the simulator with option `--panel
build/dnx-panel.sock`. Options: `SIM_PANEL_PORT=<port>` (default 8080),
`PANEL_OPTS="..."` (options of `simpanel.py`, e.g. `--bind 0.0.0.0` to open
the panel from other computers, `--page <name>`).

The panel can be also started manually (e.g. in other terminal):
```
build/dnx-sim --disk 0=build/sim-romfs.bin:ro --board sim-env/boards/example.lua --panel /tmp/dnx-panel.sock
tools/simpanel.py --socket /tmp/dnx-panel.sock [--port 8080] [--bind 127.0.0.1] [--page <name>] [-v]
```
The panel reconnects automatically after simulator restart (`reboot`); more
browsers can show the panel at the same time.

```
dnx-sim (board script) --Unix socket, JSON lines--> simpanel.py --HTTP, SSE--> browser
                       <------- control changes ---             <--- POST ---
```

### panel (board script)
Signal identifiers contain letters, digits, `_`, `.` and `-`. Values are
booleans, numbers or strings. Options (`opts` table) are sent to the page
(e.g. `label`, `color`, `unit`, `format`, `min`, `max`, `step`), so the page
can use them; options used by the board are described below.

| Function | Description |
|----------|-------------|
| `panel.output(id, pin [, opts])` | shows level of pin seen by the system (e.g. LED), `opts.active_low`: inverted |
| `panel.input(id, pin [, opts])` | control of the panel drives input pin of the system (button, switch), `opts.active_low`: inverted, `opts.value`: initial state |
| `panel.value(id, v [, opts])` | value shown by the panel: initial value or function polled every `opts.period` ms (default 500) |
| `panel.control(id, fn [, opts])` | control of the panel (slider, field, checkbox): `fn(value, id)` is called when the value is changed by the panel, `opts.value`: initial value |
| `panel.set(id, v)` | set value of signal (e.g. in model callback) |
| `panel.get(id)` | current value of signal |
| `panel.page(name)` | page of the panel: directory `panel/<name>/` (default: name of board script, e.g. `example` for `boards/example.lua`) |
| `panel.connected()` | `true` when the panel is connected |
| `panel.on_touch(fn [, lcd_id])` | call `fn(x, y, pressed, lcd_id)` when an LCD of the panel is touched (`pressed`: `false` – released), `lcd_id`: only touch of this LCD (default: all LCDs), see [LCD and touch](#lcd-and-touch) |
| `panel.lcd(id, width, height [, opts])` | LCD of board model (e.g. model of LCD controller) shown by the panel, returns LCD object, see [LCDs of board models](#lcds-of-board-models) |

Changes are sent to the panel at most every 20 ms (the last value of signal
is sent), so very short pulses are not visible.

Example:
```lua
panel.output("LED", "PA0", {active_low = true, color = "#22c55e"})
panel.input("BUTTON", "PA1", {active_low = true})
panel.value("TEMP", function() return sensor:temperature() end, {period = 200, unit = "°C"})
panel.control("TEMP_SET", function(v) sensor:set_temperature(v) end, {min = -20, max = 100, value = 25})
leds:on_change(function(latch) panel.set("LEDS", latch) end)     -- value from model
```

### Pages
The page `panel/<page>/index.html` is served with its files (images, CSS,
scripts) at `/p/<page>/`. Board without its own page is shown by the generic
page `panel/default/` (all signals, open `/p/default/?all` to see it also for
a board with its own page). The page includes the panel client script:
```html
<script src="/panel.js"></script>
```
Elements are connected to signals by attributes (dynamically created elements
are connected by `panel.bind(element)`):

| Attribute | Description |
|-----------|-------------|
| `data-signal="<id>"` | shows value: class `on` (value is true, not zero, not empty), attribute `data-value`; `<img>` with `data-on`/`data-off` changes image; `data-format` sets text (printf-like format, e.g. `%.1f °C`, `0x%02X`, empty: value) |
| `data-lcd="<lcd id>"` | `<canvas>` shows LCD (`ltdc`: LTDC driver, other: LCDs of board models), pointer (mouse, touch screen) is the touch of LCD |
| `data-input="<id>"` | control: button or other element (`data-mode="push"`: `true` while pressed (default), `"toggle"`: changed by click), checkbox (boolean), slider and number field (number), text field and select (string, `data-type="number"`: number); shows current value too |

Classes of `<html>`: `panel-connected`, `panel-disconnected`; elements with
`data-panel-status` show connection state.

JavaScript API (object `panel`):

| Function | Description |
|----------|-------------|
| `panel.on(id, fn)` | call `fn(value, id)` when signal is changed (and immediately with current value), `"*"`: all signals |
| `panel.set(id, value)` | send value of control to the board (`panel.input()`, `panel.control()`) |
| `panel.get(id)` | current value |
| `panel.onInit(fn)` | call `fn(signals)` when board definition is received (also after restart), `signals`: `[{id, kind, ui}]` (`kind`: `output`, `input`, `value`, `control`; `ui`: options) |
| `panel.onStatus(fn)` | call `fn(connected)` when connection with simulator is changed |
| `panel.format(fmt, value)` | printf-like formatting |
| `panel.bind(root)` | connect `data-signal`/`data-input`/`data-lcd` elements of `root` |
| `panel.onLcd(fn)` | call `fn(lcd, id)` when LCD `id` is changed (and immediately for all active LCDs), `lcd`: `{id, w, h, fmt, bg}` or `null` (not active: LTDC driver not initialized, display of model is off) |
| `panel.lcds` | active LCDs: `{[id] = {id, w, h, fmt, bg}}` |
| `panel.touch(lcd_id, x, y, pressed)` | touch of LCD (pixels of LCD), as touch of `<canvas data-lcd="<lcd id>">` |

Example (`panel/example/`):
```html
<img src="img/led-off.svg" data-signal="LED" data-on="img/led-on.svg" data-off="img/led-off.svg">
<button data-input="BUTTON">Button</button>
<span data-signal="TEMP" data-format="%.1f °C"></span>
<input type="range" min="-20" max="100" data-input="TEMP_SET">
<script>
    panel.on("LEDS", function (v) { /* draw 8 LEDs from bits of v */ });
</script>
```

The HTTP interface of `simpanel.py`: `/api/state` (GET, JSON: signals,
values and active LCDs), `/api/set` (POST, JSON `{"id": "...", "value": ...}`),
`/api/touch` (POST, JSON `{"lcd": "tft", "x": ..., "y": ..., "pressed": true}`),
`/events` (server-sent events), e.g.
`curl -d '{"id":"BUTTON","value":true}' localhost:8080/api/set`.

### LCD and touch
The `LTDC` driver of the simulator (enabled in `config/sim-kernel`, size of
the display: `__LTDC_WIDTH__`, `__LTDC_HEIGHT__` in
`config/sim-kernel/arch/posix/ltdc_flags.h`, default 480×272) allocates the
framebuffer in the system memory, the simulator sends changed lines of the
framebuffer to the panel every 50 ms. The API is the same as on STM32F7/H7:
the framebuffer is written by `write()` (file position is offset in the
framebuffer) or directly by its address (`IOCTL_LTDC__GET_FEATURES`), all pixel
formats are shown (`l8` without CLUT: gray scale, alpha is blended with the
background color):
```
modprobe -i LTDC 0 0 /dev/lcd -c pixel_format=rgb565
```
The panel can show more LCDs, all of them are handled the same way and are
identified by id: the LCD of the LTDC driver has id `ltdc`, other LCDs are
defined by the board script (models of displays connected by SPI, I2C,
parallel bus, etc.), see [LCDs of board models](#lcds-of-board-models). The
LCD is shown by `<canvas data-lcd="<id>">` (e.g. `<canvas data-lcd="ltdc">`),
more canvases can show the same LCD (the generic page shows all active LCDs
automatically). Touch of the canvas (mouse or touch screen) is
sent to the board script (`panel.on_touch()`), usually to the model of touch
panel controller which is read by the system, e.g. FT5336 (as on
STM32F746G-DISCO):
```lua
local ft5336 = require("models.ft5336")
local ts = ft5336.new{int = "PA9"}:attach("I2C1", 0x38)
panel.on_touch(function(x, y, pressed) ts:touch(x, y, pressed) end, "ltdc")
```
The controller is read in dnx RTOS by the `I2C` driver, registers 0x02..0x06:
number of points, X and Y (12 bits), e.g. in the demo:
```
make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua --hostfs sim-env"
lua /host/demos/lcdpaint.lua
```

### LCDs of board models
`panel.lcd(id, width, height [, opts])` creates a framebuffer in the
simulator and shows it in the panel as LCD `id` (letters, digits, `_`, `.`,
`-`, `ltdc` is used by the LTDC driver, at most 7 LCDs). The framebuffer has
the same layout as the framebuffer of the LTDC driver (lines from the top,
pixels little endian), changed lines are sent to the panel every 50 ms by the
simulator (not by Lua), so the model only writes pixels. The LCD exists until
the board is stopped.

| Option | Description |
|--------|-------------|
| `format` | pixel format: `rgb565` (default), `argb8888`, `rgb888`, `argb1555`, `argb4444`, `l8` (gray scale), `al44`, `al88` |
| `bg` | `{r, g, b}` background color blended with alpha of pixels (default black) |
| `enabled` | `false`: LCD is not shown (e.g. display is off after reset) |

Methods and fields of LCD object (colors are pixel values in the LCD format,
pixels out of LCD are not written):

| Method / field | Description |
|----------------|-------------|
| `lcd:pixel(x, y [, value])` | write pixel (if `value` is given), returns pixel value (`nil`: out of LCD) |
| `lcd:fill(value [, x, y, w, h])` | fill whole LCD or rectangle |
| `lcd:color(r, g, b [, a])` | pixel value of color (components 0..255, alpha default 255) |
| `lcd:write(offset, data)` | write raw bytes (string) at byte offset of framebuffer, returns number of written bytes |
| `lcd:read(offset, len)` | read raw bytes of framebuffer |
| `lcd:enable(on)` | show (`true`) or hide LCD in the panel (display off, sleep), content is kept |
| `lcd:on_touch(fn)` | call `fn(x, y, pressed, id)` when the LCD is touched in the panel |
| `lcd.id`, `lcd.width`, `lcd.height`, `lcd.format`, `lcd.pixel_size`, `lcd.enabled` | properties |

Example (`boards/example.lua`, page `panel/example/`, demo
`demos/tftdemo.lua`) – ST7789 TFT display on SPI3:
```lua
local st7789 = require("models.st7789")
local tft = st7789.new{id = "tft", width = 240, height = 320, dc = "PB0"}:attach("SPI3", "PB1")
tft:on_touch(function(x, y, pressed) panel.set("TFT_TOUCH", pressed and x .. ", " .. y or "") end)
```
```html
<canvas data-lcd="tft"></canvas>
<script>
    panel.onLcd(function (lcd, id) { if (id === "tft") { /* lcd === null: display off */ } });
</script>
```
In dnx RTOS the display is used by the SPI device and the D/C pin of GPIO port
B (`/dev/gpiob`, the file is the 32-bit port state):
```
modprobe -i GPIO 1 0 /dev/gpiob
modprobe -i SPI 2 0 /dev/spi-tft -c cs_port=1,cs_pin=1
lua /host/demos/tftdemo.lua
```

## Writing device models
A device model is a Lua module in `models/` loaded by the board script by
`require("models.<name>")`. The board engine calls methods of the device
(I2C: `select`, `write`, `read`, `stop`; SPI: `select`, `transfer`,
`deselect`, see [i2c](#i2c) and [spi](#spi)), GPIO lines of the device (INT,
D/C, RESET, chip enable, ...) are handled by the [gpio](#gpio) API, timing
(write cycle, conversion time) by `host.time_ms()` and `host.timer()`, and
the device can be shown or controlled by the [board panel](#board-panel).

### Structure of model
The convention of existing models:
- the file starts with a comment: usage, options of `new()`, supported
  commands/registers, methods and fields (this is the documentation of the
  model),
- the module returns table `M` with `M.new(opts)`: creates the device object
  (table with metatable), options are checked (`error()`/`assert()` with the
  model name in the message),
- `dev:attach(bus, address_or_cs)` connects the device to the bus and returns
  the device (`local d = model.new{...}:attach("SPI1", "PA7")`),
- protocol methods are called by the engine, host side methods are used by
  the board script and tests (e.g. `set_temperature()`, `touch()`,
  `read_byte()`), callbacks of changes are registered by `dev:on_change(fn)`,
- state kept in host files: loaded in `new()`, saved after changes (delayed
  by `host.timer()`, not after every byte) and at exit (`host.on_exit()`),
  paths relative to the board script are built from `host.dir`,
- no global variables: the board can contain more instances of the model.

Skeleton of SPI device with INT output (open drain, active low):
```lua
local M = {}
local DEV = {}
DEV.__index = DEV

function M.new(opts)
    opts = opts or {}
    local d = setmetatable({}, DEV)
    d.mode = {[0] = true, [3] = true}   -- supported SPI modes
    d.int = opts.int                    -- INT pin (input of the system), optional
    d.regs = {}
    if d.int then gpio.set(d.int, true) end
    return d
end

function DEV:select() self.state = "cmd" end      -- CS activated: new command

function DEV:transfer(byte)
    if self.state == "cmd" then
        self.cmd, self.state = byte, "data"
        return 0xFF                     -- byte sent to the system
    end
    -- data of command ...
    return self.regs[self.cmd] or 0
end

function DEV:deselect() self.state = nil end      -- CS deactivated: execute command

function DEV:interrupt(active)
    if self.int then gpio.set(self.int, not active) end
end

function DEV:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
```

Rules of the engine that the model can rely on:
- all callbacks (bus methods, GPIO changes, timers, panel controls) are
  called one by one in the board thread, so the model does not need locks,
- GPIO changes of the system are handled before a bus transfer that the
  system started later (e.g. D/C or CS pin set before the SPI transfer), so
  the model can keep pin levels from `gpio.on_change()` (initial level:
  `gpio.get()`),
- a bus transfer of the system waits for the model, so a model method should
  be fast (no loops waiting for time): timing is simulated by time stamps
  (e.g. busy until `host.time_ms() + write_time`) or timers,
- the system can release CS between a command and its data (e.g. `write()`
  of the command and `read()` of the result are two transfers of the SPI
  driver), so models of devices that keep the command state across CS (LCD
  controllers) do not reset it in `select()`,
- an error in a method is printed with traceback and the board works
  further (I2C: transfer is not acknowledged).

### Model of display
A display model joins the bus protocol of the controller with an LCD of the
panel (`models/st7789.lua`):
- `new()` creates the LCD by `panel.lcd(opts.id, width, height, {enabled =
  false})`, so the panel shows the display only after initialization by the
  system (e.g. SLPOUT and DISPON), commands that switch the display off call
  `lcd:enable(false)`,
- the D/C pin is followed by `gpio.on_change()`: in `transfer()` low level
  means command, high level data (parameters, pixels),
- the address window (CASET/RASET or similar commands) and the write pointer
  are kept by the model, pixels of RAMWR are converted to the LCD format
  (`lcd:color()`, or directly when the formats are the same) and written by
  `lcd:pixel()`; the orientation (MADCTL) is applied to coordinates, the LCD
  in the panel has the physical orientation of the display,
- reads of the controller (ID, status) return data from `transfer()`,
- touch of the LCD in the panel is passed to the model of the touch
  controller (`lcd:on_touch()` or `panel.on_touch(fn, id)`), the system reads
  it by its bus (e.g. FT5336 by I2C, see [LCD and touch](#lcd-and-touch)).

A display with own memory layout (monochrome OLED with pages, e.g. SSD1306)
keeps its memory in Lua and writes changed pixels to the LCD
(`lcd:pixel(x, y, on and white or black)`), a display with the framebuffer in
the LCD format can write whole lines by `lcd:write(offset, data)`.

### Testing models
Behavior of the model can be checked in the board script (calls of the
methods, `print()`), by the generic panel page (`/p/default/?all`) and by test
programs executed in dnx RTOS. Automatic tests (`make test-sim`,
`tools/simtest.py`) use the board `boards/test.lua`: a new model is attached
there, a test program in `tests/` uses the device by the driver of dnx RTOS
(e.g. `tests/spilcd.lua` for the ST7789 model) and `tools/simtest.py` checks
its output (and data received by the panel socket, e.g. pixels of the LCD).
