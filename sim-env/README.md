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
  file), `74hc595` (SPI shift register), the description of options and
  methods is in the model file,
- `tests/` – test programs executed in dnx RTOS (e.g. `lua /host/gpio.lua`).

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
`I2C16`. The I2C driver of the simulator supports master mode only, the
device address of driver configuration is 8-bit (e.g. `0xA0` is the 7-bit
address `0x50` of the board model).

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
modprobe -i I2C 0 0 /dev/i2c-ee -c address=0xA0,sub_addr_mode=2
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
