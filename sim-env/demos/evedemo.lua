--[[----------------------------------------------------------------------------
  EVE (BT817) demo executed in dnx RTOS (lua program) in the POSIX simulator
  with the example board and the board panel:
    make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua --hostfs host=sim-env"
    lua /host/demos/evedemo.lua [seconds]

  Drives the BT817 model (480x272, LCD "eve" of the panel) connected to SPI:3
  (/dev/spi-eve, CS PD0) like an EVE host driver: wakes the chip (host command
  ACTIVE), sets the display timing, uploads an L8 bitmap to RAM_G
  (CMD_MEMWRITE) and draws frames by coprocessor commands written to
  REG_CMDB_WRITE: color bars (RECTS), bouncing balls (POINTS), a sine wave
  (LINE_STRIP), the bitmap tinted by COLOR_RGB and a ring at the touch point
  (REG_TOUCH_SCREEN_XY, touch the display in the panel). Touch tags: button
  "pause" (TAG 1, CMD_BUTTON), keys R/G/B (CMD_KEYS, tags are the characters)
  set the color of the wave, the slider (TAG 3, CMD_SLIDER with CMD_TRACK,
  REG_TRACKER) sets the speed. After the given time
  (default 20 s) the display is stopped (REG_PCLK = 0) and the chip is put to
  sleep.
------------------------------------------------------------------------------]]

local dnx = require("dnx")
local spi = require("dnx.spi")
local lfs = require("lfs")

local W, H = 480, 272
local DEV = "/dev/spi-eve"
local SECONDS = tonumber(arg and arg[1]) or 20

-- registers
local RAM_G, REG = 0x000000, 0x302000
local REG_ID, REG_FRAMES = REG + 0x00, REG + 0x04
local REG_HCYCLE, REG_HOFFSET, REG_HSIZE, REG_HSYNC0, REG_HSYNC1 =
    REG + 0x2C, REG + 0x30, REG + 0x34, REG + 0x38, REG + 0x3C
local REG_VCYCLE, REG_VOFFSET, REG_VSIZE, REG_VSYNC0, REG_VSYNC1 =
    REG + 0x40, REG + 0x44, REG + 0x48, REG + 0x4C, REG + 0x50
local REG_SWIZZLE, REG_PCLK_POL, REG_CSPREAD, REG_PCLK = REG + 0x64, REG + 0x6C, REG + 0x68, REG + 0x70
local REG_GPIO_DIR, REG_GPIO, REG_PWM_DUTY = REG + 0x90, REG + 0x94, REG + 0xD4
local REG_CMD_READ, REG_CMD_WRITE = REG + 0xF8, REG + 0xFC
local REG_TOUCH_SCREEN_XY, REG_TOUCH_TAG = REG + 0x124, REG + 0x12C
local REG_TRACKER = 0x309000
local REG_CMDB_SPACE, REG_CMDB_WRITE = REG + 0x574, REG + 0x578

-- display list instructions
local function CLEAR_COLOR_RGB(r, g, b)
    return 0x02000000 | (r << 16) | (g << 8) | b
end
local function CLEAR(c, s, t)
    return 0x26000000 | (c << 2) | (s << 1) | t
end
local function COLOR_RGB(r, g, b)
    return 0x04000000 | (r << 16) | (g << 8) | b
end
local function COLOR_A(a)
    return 0x10000000 | a
end
local function BEGIN(p)
    return 0x1F000000 | p
end
local function VERTEX2F(x, y) -- 1/16 px (VERTEX_FORMAT 4)
    return 0x40000000 | ((math.floor(x * 16) & 0x7FFF) << 15) | (math.floor(y * 16) & 0x7FFF)
end
local function POINT_SIZE(r) -- radius in px
    return 0x0D000000 | math.floor(r * 16)
end
local function LINE_WIDTH(w) -- half width in px
    return 0x0E000000 | math.floor(w * 16)
end
local function TAG(t)
    return 0x03000000 | t
end
local function TAG_MASK(m)
    return 0x14000000 | m
end
local function BITMAP_SOURCE(a)
    return 0x01000000 | a
end
local function BITMAP_LAYOUT(f, ls, h)
    return 0x07000000 | (f << 19) | (ls << 9) | h
end
local function BITMAP_SIZE(w, h)
    return 0x08000000 | (w << 9) | h
end
local END, DISPLAY = 0x21000000, 0x00000000
local BITMAPS, POINTS, LINE_STRIP, RECTS = 1, 2, 4, 9
local L8 = 3

-- coprocessor commands
local DLSTART, SWAP, MEMWRITE = 0xFFFFFF00, 0xFFFFFF01, 0xFFFFFF1A
local FGCOLOR, BGCOLOR, BUTTON, KEYS = 0xFFFFFF0A, 0xFFFFFF09, 0xFFFFFF0D, 0xFFFFFF0E
local SLIDER, TRACK = 0xFFFFFF10, 0xFFFFFF2C
local function XY(x, y) -- packed 16-bit arguments
    return (x & 0xFFFF) | ((y & 0xFFFF) << 16)
end
local function str(s) -- string argument: NUL terminated, padded to words
    s = s .. "\0"
    return s .. string.rep("\0", (4 - #s % 4) % 4)
end

-- SPI host interface
local function xfer(data)
    return assert(spi.transceive(DEV, data))
end
local function addr_bytes(a, write)
    return string.char(((a >> 16) & 0x3F) | (write and 0x80 or 0), (a >> 8) & 0xFF, a & 0xFF)
end
local function wr(a, data)
    xfer(addr_bytes(a, true) .. data)
end
local function rd32(a)
    return (string.unpack("<I4", xfer(addr_bytes(a) .. "\0\0\0\0\0"):sub(5)))
end
local function wr8(a, v)
    wr(a, string.char(v & 0xFF))
end
local function wr16(a, v)
    wr(a, string.pack("<I2", v))
end
local function host_cmd(c, p)
    xfer(string.char(c, p or 0, 0))
end

local function words(t) -- numbers: 32-bit words, strings: data
    local s = {}
    for i, w in ipairs(t) do
        s[i] = type(w) == "string" and w or string.pack("<I4", w & 0xFFFFFFFF)
    end
    return table.concat(s)
end

-- coprocessor data by REG_CMDB_WRITE bursts
local function cmd(data)
    local pos = 1
    while pos <= #data do
        local space = rd32(REG_CMDB_SPACE) & ~3
        if space > 0 then
            local n = math.min(space, #data - pos + 1)
            wr(REG_CMDB_WRITE, data:sub(pos, pos + n - 1))
            pos = pos + n
        else
            dnx.sys.msleep(2)
        end
    end
end

local function wait_idle()
    for _ = 1, 500 do
        if rd32(REG_CMD_READ) == rd32(REG_CMD_WRITE) then
            return
        end
        dnx.sys.msleep(2)
    end
    error("coprocessor does not respond")
end

-- device file (SPI:3, CS PD0)
if not lfs.attributes(DEV) then
    local p = assert(io.popen("modprobe -i SPI 3 0 " .. DEV .. " -c cs_port=3,cs_pin=0,mode=0", "r"))
    p:read("a")
    p:close()
end

-- wake up: ACTIVE, REG_ID = 7Ch after boot
host_cmd(0x00)
local t0 = dnx.sys.uptime()
while rd32(REG_ID) & 0xFF ~= 0x7C do
    if dnx.sys.uptime() - t0 > 1000 then
        error("BT817 does not respond (REG_ID)")
    end
    dnx.sys.msleep(10)
end

-- display timing of a 480x272 panel
wr16(REG_HCYCLE, 548)
wr16(REG_HOFFSET, 43)
wr16(REG_HSYNC0, 0)
wr16(REG_HSYNC1, 41)
wr16(REG_VCYCLE, 292)
wr16(REG_VOFFSET, 12)
wr16(REG_VSYNC0, 0)
wr16(REG_VSYNC1, 10)
wr16(REG_HSIZE, W)
wr16(REG_VSIZE, H)
wr8(REG_SWIZZLE, 0)
wr8(REG_PCLK_POL, 1)
wr8(REG_CSPREAD, 0)
cmd(words({ DLSTART, CLEAR_COLOR_RGB(0, 0, 0), CLEAR(1, 1, 1), DISPLAY, SWAP }))
wait_idle()
wr8(REG_GPIO_DIR, 0x80)
wr8(REG_GPIO, 0x80) -- DISP
wr8(REG_PWM_DUTY, 128) -- backlight
wr8(REG_PCLK, 5) -- start the display

-- L8 bitmap 64x64: radial gradient (luminance is the alpha of COLOR_RGB)
local BMP = 64
local px = {}
for y = 0, BMP - 1 do
    for x = 0, BMP - 1 do
        local d = math.sqrt((x - 31.5) ^ 2 + (y - 31.5) ^ 2) / 32
        px[#px + 1] = string.char(math.max(0, math.floor(255 * (1 - d))))
    end
end
cmd(words({ MEMWRITE, RAM_G, BMP * BMP }) .. table.concat(px))
wait_idle()

-- animation
local balls = {
    { x = 60, y = 60, dx = 3.1, dy = 2.3, r = 14, c = { 255, 80, 80 } },
    { x = 200, y = 150, dx = -2.4, dy = 3.0, r = 10, c = { 80, 255, 120 } },
    { x = 350, y = 90, dx = 2.0, dy = -2.7, r = 18, c = { 90, 140, 255 } },
}
local BARS = { { 255, 255, 255 }, { 255, 255, 0 }, { 0, 255, 255 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 0 } }

-- controls: slider tracked by REG_TRACKER (tag 3)
local SL_X, SL_Y, SL_W, SL_H = 250, 22, 70, 10
cmd(words({ TRACK, XY(SL_X, SL_Y), XY(SL_W, SL_H), 3 }))
local WAVE = { R = { 255, 80, 80 }, G = { 80, 255, 120 }, B = { 90, 160, 255 } }
local wave_key, paused, last_tag, speed = "R", false, 0, 0.5

local frame = 0
local phase = 0
local start = dnx.sys.uptime()
while dnx.sys.uptime() - start < SECONDS * 1000 do
    -- touch tags of the last frame
    local tag = rd32(REG_TOUCH_TAG) & 0xFF
    if tag ~= last_tag then
        if tag == 1 then
            paused = not paused
        elseif WAVE[string.char(tag)] then
            wave_key = string.char(tag)
        end
        last_tag = tag
    end
    local tracker = rd32(REG_TRACKER)
    if tracker & 0xFF == 3 then
        speed = (tracker >> 16) / 65535
    end
    local k = paused and 0 or (0.2 + speed * 2.8)
    phase = phase + 6 * k

    local dl = { DLSTART, CLEAR_COLOR_RGB(16, 18, 32), CLEAR(1, 1, 1) }
    local function add(...)
        for _, w in ipairs({ ... }) do
            dl[#dl + 1] = w
        end
    end

    -- color bars at the bottom
    add(BEGIN(RECTS))
    local bw = W / #BARS
    for i, c in ipairs(BARS) do
        add(COLOR_RGB(c[1], c[2], c[3]), VERTEX2F((i - 1) * bw, H - 24), VERTEX2F(i * bw - 1, H - 1))
    end
    add(END)

    -- sine wave
    local wc = WAVE[wave_key]
    add(COLOR_RGB(wc[1], wc[2], wc[3]), LINE_WIDTH(1.5), BEGIN(LINE_STRIP))
    for x = 0, W, 8 do
        add(VERTEX2F(x, 140 + 40 * math.sin((x + phase) / 40)))
    end
    add(END)

    -- tinted bitmaps (L8: alpha of COLOR_RGB)
    add(BITMAP_SOURCE(RAM_G), BITMAP_LAYOUT(L8, BMP, BMP), BITMAP_SIZE(BMP, BMP), BEGIN(BITMAPS))
    add(COLOR_RGB(255, 120, 0), VERTEX2F(W - 150, 20))
    add(COLOR_RGB(0, 200, 255), VERTEX2F(W - 80, 20))
    add(END)

    -- balls (semi-transparent)
    add(COLOR_A(200), BEGIN(POINTS))
    for _, b in ipairs(balls) do
        b.x, b.y = b.x + b.dx * k, b.y + b.dy * k
        if b.x < b.r or b.x > W - b.r then
            b.dx = -b.dx
        end
        if b.y < b.r or b.y > H - 24 - b.r then
            b.dy = -b.dy
        end
        add(COLOR_RGB(b.c[1], b.c[2], b.c[3]), POINT_SIZE(b.r), VERTEX2F(b.x, b.y))
    end
    add(END, COLOR_A(255))

    -- controls (widgets of the coprocessor, tags for touch)
    add(FGCOLOR, paused and 0x806020 or 0x205080, BGCOLOR, 0x303848)
    add(TAG(1), BUTTON, XY(10, 10), XY(80, 32), XY(27, 0))
    dl[#dl + 1] = str(paused and "run" or "pause")
    add(FGCOLOR, 0x205080, KEYS, XY(100, 10), XY(130, 32), XY(27, wave_key:byte()))
    dl[#dl + 1] = str("RGB")
    add(TAG(3), SLIDER, XY(SL_X, SL_Y), XY(SL_W, SL_H), XY(0, math.floor(speed * 100)), 100, TAG(255))

    -- touch: ring at REG_TOUCH_SCREEN_XY (8000h: no touch)
    local xy = rd32(REG_TOUCH_SCREEN_XY)
    local tx, ty = (xy >> 16) & 0xFFFF, xy & 0xFFFF
    if tx ~= 0x8000 and tx < W and ty < H then
        add(TAG_MASK(0), BEGIN(POINTS), COLOR_RGB(255, 255, 255), POINT_SIZE(22), VERTEX2F(tx, ty))
        add(COLOR_RGB(16, 18, 32), POINT_SIZE(18), VERTEX2F(tx, ty), END, TAG_MASK(1))
    end

    add(DISPLAY, SWAP)
    cmd(words(dl))
    wait_idle()
    frame = frame + 1
    dnx.sys.msleep(30)
end

local frames = rd32(REG_FRAMES)
print(string.format("%d frames drawn, REG_FRAMES = %d", frame, frames))

-- stop the display and put the chip to sleep
wr8(REG_PCLK, 0)
wr8(REG_PWM_DUTY, 0)
host_cmd(0x42) -- SLEEP
