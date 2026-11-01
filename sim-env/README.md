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
  controller), the description of options and methods is in the model file,
- `tests/` – test programs executed in dnx RTOS (e.g. `lua /host/gpio.lua`),
- `demos/` – demo programs executed in dnx RTOS, e.g. `lcdpaint.lua` (drawing
  on the LCD of the board panel, see [LCD and touch](#lcd-and-touch)),
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
| `panel.on_touch(fn)` | call `fn(x, y, pressed)` when the LCD of the panel is touched (`pressed`: `false` – released), see [LCD and touch](#lcd-and-touch) |

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
| `panel.onLcd(fn)` | call `fn(lcd)` when LCD is changed, `lcd`: `{w, h, fmt, bg}` or `null` (LTDC driver not active) |
| `panel.touch(x, y, pressed)` | touch of LCD (pixels of LCD), as touch of `<canvas data-lcd>` |

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

The HTTP interface of `simpanel.py`: `/api/state` (GET, JSON: signals and
values), `/api/set` (POST, JSON `{"id": "...", "value": ...}`), `/api/touch`
(POST, JSON `{"x": ..., "y": ..., "pressed": true}`), `/events` (server-sent
events), e.g. `curl -d '{"id":"BUTTON","value":true}' localhost:8080/api/set`.

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
The LCD is shown by `<canvas data-lcd>` (the generic page shows it
automatically). Touch of the canvas (mouse or touch screen) is sent to the
board script (`panel.on_touch()`), usually to the model of touch panel
controller which is read by the system, e.g. FT5336 (as on STM32F746G-DISCO):
```lua
local ft5336 = require("models.ft5336")
local ts = ft5336.new{int = "PA9"}:attach("I2C1", 0x38)
panel.on_touch(function(x, y, pressed) ts:touch(x, y, pressed) end)
```
The controller is read in dnx RTOS by the `I2C` driver, registers 0x02..0x06:
number of points, X and Y (12 bits), e.g. in the demo:
```
make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua --hostfs sim-env"
lua /host/demos/lcdpaint.lua
```
