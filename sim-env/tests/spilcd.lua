--[[----------------------------------------------------------------------------
  Helper of the SPI LCD test (ST7789 model, LCD "tft" of the panel) executed
  by tools/simtest.py with board boards/test.lua: SPI:3, CS PB1, D/C PB0.
    lua spilcd.lua init     creates /dev/gpiob and /dev/spi-lcd, initializes
                            the controller, prints ID=<RDDID bytes> and
                            PM=<RDDPM> (display on: 0x9C)
    lua spilcd.lua draw     line 10 red (RGB565), pixels (5..6, 20..21) blue
                            written with MADCTL MV (rows and columns
                            exchanged), 18-bit pixel (7, 30) green, prints DRAWN
    lua spilcd.lua off      DISPOFF, prints OFF
------------------------------------------------------------------------------]]

local T = require("testlib")

local W = 240
local DC = 0x01       -- PB0
local CS = 0x02       -- PB1 (controlled by SPI driver, kept high between transfers)

T.modprobe("GPIO", 1, 0, "/dev/gpiob")
T.modprobe("SPI", 3, 0, "/dev/spi-lcd", "cs_port=1,cs_pin=1,mode=0")

local gpio = assert(io.open("/dev/gpiob", "r+b"))
gpio:setvbuf("no")
local spi = assert(io.open("/dev/spi-lcd", "r+b"))
spi:setvbuf("no")

local function dc(level)
    gpio:seek("set", 0)
    assert(gpio:write(string.pack("<I4", CS | (level and DC or 0))))
end

local function cmd(c, data)
    dc(false)
    assert(spi:write(string.char(c)))
    if data and #data > 0 then
        dc(true)
        assert(spi:write(data))
    end
end

local function read(c, n)
    dc(false)
    assert(spi:write(string.char(c)))
    dc(true)
    return assert(spi:read(n))
end

local function window(x0, y0, x1, y1)
    cmd(0x2A, string.pack(">I2I2", x0, x1))
    cmd(0x2B, string.pack(">I2I2", y0, y1))
end

if arg[1] == "init" then
    cmd(0x01)                   -- SWRESET
    cmd(0x11)                   -- SLPOUT
    cmd(0x3A, "\x55")           -- COLMOD: 16-bit
    cmd(0x36, "\x00")           -- MADCTL
    cmd(0x21)                   -- INVON (ignored)
    window(0, 0, W - 1, 319)
    cmd(0x2C, string.rep("\0", W * 320 * 2))
    cmd(0x29)                   -- DISPON
    local id = read(0x04, 3)
    print(string.format("ID=%02X%02X%02X PM=0x%02X", id:byte(1), id:byte(2), id:byte(3), read(0x0A, 1):byte()))

elseif arg[1] == "draw" then
    window(0, 10, W - 1, 10)
    cmd(0x2C, string.pack(">I2", 0xF800):rep(W))
    cmd(0x36, "\x20")           -- MV: column address is Y, row address is X
    window(20, 5, 21, 6)
    cmd(0x2C, string.pack(">I2", 0x001F):rep(4))
    cmd(0x36, "\x00")
    cmd(0x3A, "\x66")           -- 18-bit pixel
    window(7, 30, 7, 30)
    cmd(0x2C, "\x00\xFC\x00")
    cmd(0x3A, "\x55")
    print("DRAWN")

elseif arg[1] == "off" then
    cmd(0x28)
    print("OFF")
end

spi:close()
gpio:close()
