--[[----------------------------------------------------------------------------
  TFT display demo executed in dnx RTOS (lua program) in the POSIX simulator
  with the example board and the board panel:
    make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua --hostfs sim-env"
    lua /host/demos/tftdemo.lua [seconds]

  Draws on the ST7789 display model (240x320, RGB565) connected to SPI3
  (/dev/spi-tft, CS PB1), the D/C pin (PB0) is controlled by /dev/gpiob:
  color bars and a moving square for the given time (default 10 s), then the
  display is switched off.
------------------------------------------------------------------------------]]

local dnx = require("dnx")
local lfs = require("lfs")

local W, H = 240, 320
local DC, CS = 0x01, 0x02     -- PB0, PB1 (CS is controlled by the SPI driver)
local BOX = 24

local function run(cmd)
    local p = assert(io.popen(cmd, "r"))
    local out = p:read("a")
    if not p:close() then error(cmd .. ": " .. out) end
end

if not lfs.attributes("/dev/gpiob") then
    run("modprobe -i GPIO 1 0 /dev/gpiob")
end
if not lfs.attributes("/dev/spi-tft") then
    run("modprobe -i SPI 2 0 /dev/spi-tft -c cs_port=1,cs_pin=1")
end

local gpio = assert(io.open("/dev/gpiob", "r+b"))
gpio:setvbuf("no")
local spi = assert(io.open("/dev/spi-tft", "r+b"))
spi:setvbuf("no")

-- D/C pin: low - command, high - data (other pins of port B: CS high)
local function dc(level)
    gpio:seek("set", 0)
    gpio:write(string.pack("<I4", CS | (level and DC or 0)))
end

local function cmd(c, data)
    dc(false)
    spi:write(string.char(c))
    if data then
        dc(true)
        spi:write(data)
    end
end

local function color(r, g, b)
    return string.pack(">I2", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
end

local function rect(x, y, w, h, c)
    cmd(0x2A, string.pack(">I2I2", x, x + w - 1))     -- CASET
    cmd(0x2B, string.pack(">I2I2", y, y + h - 1))     -- RASET
    cmd(0x2C, c:rep(w * h))                           -- RAMWR
end

-- initialization
cmd(0x01)                   -- SWRESET
dnx.sys.msleep(5)
cmd(0x11)                   -- SLPOUT
cmd(0x3A, "\x55")           -- COLMOD: 16-bit pixels
cmd(0x36, "\x00")           -- MADCTL: portrait
cmd(0x21)                   -- INVON (IPS panels)

local BG = color(16, 18, 24)
local BARS = {
    color(255, 255, 255), color(255, 255, 0), color(0, 255, 255), color(0, 255, 0),
    color(255, 0, 255), color(255, 0, 0), color(0, 0, 255), color(0, 0, 0),
}

rect(0, 0, W, H, BG)
for i, c in ipairs(BARS) do rect((i - 1) * W // #BARS, 0, W // #BARS, 80, c) end
cmd(0x29)                   -- DISPON

-- moving square below the bars
local time = tonumber(arg[1]) or 10
local x, y, dx, dy = 10, 100, 3, 2
local square = color(59, 130, 246)
local t0 = os.time()
print("Moving square on the TFT display for " .. time .. " s.")

while os.time() - t0 < time do
    rect(x, y, BOX, BOX, BG)
    x = math.max(0, math.min(W - BOX, x + dx))
    y = math.max(82, math.min(H - BOX, y + dy))
    if x == 0 or x == W - BOX then dx = -dx end
    if y == 82 or y == H - BOX then dy = -dy end
    rect(x, y, BOX, BOX, square)
    dnx.sys.msleep(20)
end

cmd(0x28)                   -- DISPOFF
cmd(0x10)                   -- SLPIN
spi:close()
gpio:close()
print("bye")
