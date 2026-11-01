--[[----------------------------------------------------------------------------
  Helper of the LCD test (LTDC driver, touch panel controller FT5336) executed
  by tools/simtest.py with board boards/test.lua:
    lua lcd.lua init    creates /dev/lcd (RGB565) and /dev/i2c-ts (FT5336),
                        line 10 is filled with red color, prints LCD=<size>
    lua lcd.lua touch   prints state of touch controller and INT pin (PA28):
                        TOUCH n=<points> x=<x> y=<y> ev=<event> int=<0|1>
    lua lcd.lua release releases /dev/lcd (framebuffer memory is needed by
                        next tests), prints RELEASED
------------------------------------------------------------------------------]]

local T = require("testlib")
local W = 480 -- __LTDC_WIDTH__

if arg[1] == "init" then
    T.modprobe("LTDC", 0, 0, "/dev/lcd", "pixel_format=rgb565")
    T.modprobe("I2C", 0, 14, "/dev/i2c-ts", "address=0x38,sub_addr_mode=1")
    local f = assert(io.open("/dev/lcd", "r+b"))
    f:setvbuf("no")
    f:seek("set", 10 * W * 2)
    assert(f:write(string.pack("<I2", 0xF800):rep(W)))
    f:close()
    print("LCD=" .. require("lfs").attributes("/dev/lcd", "size"))

elseif arg[1] == "release" then
    local ok, out = T.run("modprobe -R /dev/lcd")
    print(ok and "RELEASED" or out)

elseif arg[1] == "touch" then
    local f = assert(io.open("/dev/i2c-ts", "r+b"))
    f:setvbuf("no")
    f:seek("set", 0xA8)
    local id = f:read(1):byte()
    f:seek("set", 2)
    local d = assert(f:read(5))
    f:close()
    local x = ((d:byte(2) & 0x0F) << 8) | d:byte(3)
    local y = ((d:byte(4) & 0x0F) << 8) | d:byte(5)
    print(string.format("TOUCH n=%d x=%d y=%d ev=%d int=%d id=0x%02X", d:byte(1) & 0x0F, x, y, d:byte(2) >> 6,
                        T.pin_get(28) and 1 or 0, id))
end
