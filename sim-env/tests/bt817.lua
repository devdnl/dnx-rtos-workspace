--[[----------------------------------------------------------------------------
  BT817 (EVE3) test executed in dnx RTOS (lua program) with board
  boards/test.lua: models/bt817.lua on SPI:3, CS PD0, INT_N -> PD1. The test
  works like an EVE host driver (dnx.spi.transceive: memory read/write, host
  commands, command FIFO by REG_CMDB_WRITE bursts) and checks pixels of the
  LCD "eve" and touch registers by the test control device (I2C:0 0x30,
  "eve ..." commands).
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local spi = require("dnx.spi")
local test, expect = T.test, T.expect

local DEV = "/dev/spi-eve"
local GPIOD = "/dev/gpiod"
local INT_PIN = 1 -- PD1

-- memory map and registers
local RAM_G, RAM_DL, RAM_CMD = 0x000000, 0x300000, 0x308000
local REG = 0x302000
local REG_ID, REG_FRAMES, REG_FREQUENCY = REG + 0x00, REG + 0x04, REG + 0x0C
local REG_CPURESET = REG + 0x20
local REG_HCYCLE, REG_HOFFSET, REG_HSIZE, REG_HSYNC0, REG_HSYNC1 =
    REG + 0x2C, REG + 0x30, REG + 0x34, REG + 0x38, REG + 0x3C
local REG_VCYCLE, REG_VOFFSET, REG_VSIZE, REG_VSYNC0, REG_VSYNC1 =
    REG + 0x40, REG + 0x44, REG + 0x48, REG + 0x4C, REG + 0x50
local REG_DLSWAP, REG_PCLK = REG + 0x54, REG + 0x70
local REG_INT_FLAGS, REG_INT_EN, REG_INT_MASK = REG + 0xA8, REG + 0xAC, REG + 0xB0
local REG_MACRO_0 = REG + 0xD8
local REG_CMD_READ, REG_CMD_WRITE, REG_CMD_DL = REG + 0xF8, REG + 0xFC, REG + 0x100
local REG_TOUCH_SCREEN_XY, REG_TOUCH_TAG_XY, REG_TOUCH_TAG = REG + 0x124, REG + 0x128, REG + 0x12C
local REG_TOUCH_TAG1_XY, REG_TOUCH_TAG1 = REG + 0x130, REG + 0x134
local REG_CTOUCH_EXTENDED = REG + 0x108
local REG_TRACKER, REG_TRACKER_1 = 0x309000, 0x309004
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
local function BEGIN(p)
    return 0x1F000000 | p
end
local END, DISPLAY, RETURN = 0x21000000, 0x00000000, 0x24000000
local function VERTEX2F(x, y)
    return 0x40000000 | ((x & 0x7FFF) << 15) | (y & 0x7FFF)
end
local function VERTEX2II(x, y, h, c)
    return 0x80000000 | (x << 21) | (y << 12) | ((h or 0) << 7) | (c or 0)
end
local function VERTEX_FORMAT(f)
    return 0x27000000 | f
end
local function POINT_SIZE(s)
    return 0x0D000000 | s
end
local function LINE_WIDTH(w)
    return 0x0E000000 | w
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
local function TAG(t)
    return 0x03000000 | t
end
local function CLEAR_TAG(t)
    return 0x12000000 | t
end
local function TAG_MASK(m)
    return 0x14000000 | m
end
local function BITMAP_HANDLE(n)
    return 0x05000000 | n
end
local function CELL(n)
    return 0x06000000 | n
end
local function JUMP(d)
    return 0x1E000000 | d
end
local function CALL(d)
    return 0x1D000000 | d
end
local function MACRO(m)
    return 0x25000000 | m
end
local BITMAPS, POINTS, LINES, RECTS = 1, 2, 3, 9
local RGB565, L8, L4, L2 = 7, 3, 2, 17

-- coprocessor commands
local function CMD(c)
    return 0xFFFFFF00 | c
end
local DLSTART, SWAP, TEXT, BUTTON, SLIDER, GAUGE = CMD(0x00), CMD(0x01), CMD(0x0C), CMD(0x0D), CMD(0x10), CMD(0x13)
local MEMCRC, REGREAD, MEMWRITE, APPEND, INFLATE = CMD(0x18), CMD(0x19), CMD(0x1A), CMD(0x1E), CMD(0x22)
local GETPTR, NUMBER, SETBASE = CMD(0x23), CMD(0x2E), CMD(0x38)
local FGCOLOR, KEYS, TRACK, DIAL = CMD(0x0A), CMD(0x0E), CMD(0x2C), CMD(0x2D)
local SETBITMAP = CMD(0x43)

local ZLIB = {
    dyn = {
        data = "\x78\xDA\xED\x95\x4D\x6A\xC3\x40\x0C\x85\xAF\x32\x47\x18\xFF\xDB\xDB\x42\xD7\x85\x36\x74\x1F\xE2\x40\x03\x6E\x1C\xE2\x04\xD2\xDB\x77\xF4\xA4\xD1\x28\xD3\x13\x14\x66\xFD\x1E\xF3\xF1\x61\x59\x9A\xCF\x0F\xEF\xB6\xD3\xF7\x7D\xD9\xDF\xD6\x6B\xE5\xE6\xD3\x76\x59\xF6\x3F\xB5\x7B\xD9\x8D\xD5\xD0\xB8\xDB\x7A\x3F\x7C\xB5\xEE\xB0\x5E\xAE\xEB\xE1\xB8\x6D\xEB\xB5\x8B\x9D\xDE\xBD\xEF\xDE\x3E\x06\x77\xD9\x9F\x8F\xCB\x68\x2B\x93\x7B\xFD\x7C\xAD\xBC\x9B\xCF\x8F\xAA\xE2\xBC\xAA\xDD\x72\xDA\x6E\x9E\xDF\xAD\x28\xAA\x13\xB7\x89\x6F\xB6\x9C\x77\xCC\xED\xED\xA3\x43\xEC\x8C\xE0\x4E\xF2\xAE\xB7\x9D\x40\x23\x72\x4D\xCF\x7B\x29\x80\xAB\x3E\x21\x68\x13\xD7\xB8\x20\x1F\x98\x9B\xC9\x48\x27\xA0\x08\x6C\x8C\x4C\xCB\x03\xCC\x5A\x88\x1B\x70\xD5\x27\x04\x7D\xE2\x1A\x17\xE4\x13\x73\xFF\xC8\x44\x72\x0D\xB2\x1A\x99\x52\x4D\x5C\xD1\x42\xDA\x81\xAB\x3E\x21\x18\x13\xD7\xBA\xF0\x87\xA8\x84\x9C\xC9\xC4\x1A\xB8\x6A\x64\x2A\x2D\x71\x45\x0B\xE9\x00\xAE\xFA\xD0\xA7\xB7\x83\x65\x5D\x50\xF1\x02\x7E\x96\x91\x52\x03\xAE\x1A\x99\x4A\x4F\x5C\xD1\x42\x3A\x81\x6B\x7C\x88\x6C\x46\xCB\xB8\xA0\x51\x33\x37\x93\x91\x4E\x07\xAE\x1A\x99\xCA\x48\xDC\xA8\xC5\x9F\x81\x27\x2B\xF9\x50\x96\xB8\xC6\x05\x79\xCB\xDC\x4C\x46\x3A\x03\xB8\x6A\x64\x87\x80\x07\x2B\x6A\xA1\xE0\x19\x1C\x7D\x42\xD2\x24\xAE\x71\x41\xDE\x33\x37\x93\x91\xCE\xC4\x13\x9D\x8C\x2C\x19\xA3\x25\x5A\xC8\xF9\x17\x56\x9F\x10\x74\x89\x6B\x5C\x90\x8F\xCC\xCD\x65\xE2\xC7\xE0\xC9\x4A\x46\xB6\x45\x5C\xD1\x42\xDA\x82\xAB\x3E\x21\x18\x12\xD7\xB8\xF0\x04\xC4\xC1\xCA\x65\xA4\x26\x3F\x71\x34\x32\x9D\x86\xB8\xA2\x85\xB4\x07\x57\x7D\x42\x30\x99\x89\xB6\x2E\x4C\x96\xD1\xCA\x64\x74\xA3\x12\x57\x8D\x9E\x16\x6A\xE0\x8A\x96\xEC\x52\xE2\x26\x1F\x5E\xA3\x89\x6C\x5C\x64\x9B\x82\x9B\xC9\xE8\x46\x25\xAE\x1A\x3D\x2D\xD4\xC0\x15\xAD\xB8\x4B\x31\x58\xC9\x67\x2E\xD7\xA1\x5C\x87\x72\x1D\xCA\x75\x28\xD7\xA1\x5C\x87\x72\x1D\xCA\x75\x28\xD7\xA1\x5C\x87\x72\x1D\xCA\x75\x28\xD7\xA1\x5C\x87\x72\x1D\xFE\xF9\x75\xF8\x05\x33\x49\x4A\x94",
        len = 5628,
        crc = 0x488991C5,
    },
    fixed = {
        data = "\x78\x01\x73\x74\x72\x76\x44\x42\x0A\x19\xA9\x39\x39\xF9\x28\xA4\x81\xA1\x91\xB1\x89\xA9\x99\xB9\x85\x25\x00\x20\xAD\x0C\xA8",
        len = 44,
        crc = 0x68E40431,
    },
    stored = {
        data = "\x78\x01\x01\x1C\x00\xE3\xFF\x73\x74\x6F\x72\x65\x64\x20\x62\x6C\x6F\x63\x6B\x20\x64\x61\x74\x61\x20\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x8F\x6C\x08\xA4",
        len = 28,
        crc = 0x85EC40E5,
    },
}

-- SPI host interface -----------------------------------------------------------
local function xfer(data)
    local r, err = spi.transceive(DEV, data)
    if not r then
        error("transceive: " .. tostring(err), 3)
    end
    return r
end

local function addr_bytes(a, write)
    return string.char(((a >> 16) & 0x3F) | (write and 0x80 or 0), (a >> 8) & 0xFF, a & 0xFF)
end

local function wr(a, data)
    xfer(addr_bytes(a, true) .. data)
end
local function rd(a, n)
    return xfer(addr_bytes(a) .. "\0" .. string.rep("\0", n)):sub(5)
end
local function wr8(a, v)
    wr(a, string.char(v & 0xFF))
end
local function wr16(a, v)
    wr(a, string.pack("<I2", v))
end
local function wr32(a, v)
    wr(a, string.pack("<I4", v))
end
local function rd32(a)
    return (string.unpack("<I4", rd(a, 4)))
end
local function host_cmd(c, p)
    xfer(string.char(c, p or 0, 0))
end

local function words(...)
    local t = {}
    for i, w in ipairs({ ... }) do
        t[i] = string.pack("<I4", w & 0xFFFFFFFF)
    end
    return table.concat(t)
end

local function padded(s) -- string or data padded to whole words
    return s .. string.rep("\0", (4 - #s % 4) % 4)
end

-- write coprocessor data by REG_CMDB_WRITE bursts (free space by REG_CMDB_SPACE)
local function cmd(data)
    local pos = 1
    local t0 = dnx.sys.uptime()
    while pos <= #data do
        local space = rd32(REG_CMDB_SPACE) & ~3
        if space > 0 then
            local n = math.min(space, #data - pos + 1)
            wr(REG_CMDB_WRITE, data:sub(pos, pos + n - 1))
            pos = pos + n
        elseif dnx.sys.uptime() - t0 > 3000 then
            error("command FIFO full", 2)
        else
            dnx.sys.msleep(5)
        end
    end
end

local function wait_idle()
    local t0 = dnx.sys.uptime()
    repeat
        local r, w = rd32(REG_CMD_READ), rd32(REG_CMD_WRITE)
        if r == w then
            return true
        end
        dnx.sys.msleep(5)
    until dnx.sys.uptime() - t0 > 3000
    return false
end

-- result word of the last command (written by the coprocessor before REG_CMD_WRITE)
local function result()
    return rd32(RAM_CMD + ((rd32(REG_CMD_WRITE) - 4) & 0xFFF))
end

-- test control device ------------------------------------------------------------
local ctl

local function control(c)
    ctl:seek("set", 0)
    assert(ctl:write(c))
    ctl:seek("set", 0)
    local r = assert(ctl:read(128)):match("^[^%z]*")
    if r:find("^error") then
        error("control '" .. c .. "': " .. r, 2)
    end
    return r
end

local function pixel(x, y)
    return control(string.format("eve pixel %d %d", x, y))
end

local function int_pin()
    local f = assert(io.open(GPIOD, "rb"))
    f:setvbuf("no")
    local s = assert(f:read(4))
    f:close()
    return (string.unpack("<I4", s) >> INT_PIN) & 1 == 1
end

local RED, GREEN, BLUE, YELLOW, WHITE = "F800", "07E0", "001F", "FFE0", "FFFF"
local BG = "0008" -- CLEAR_COLOR_RGB(0, 0, 64)

local function expect_pixels(list)
    for _, p in ipairs(list) do
        local v = pixel(p[1], p[2])
        expect(v == p[3], string.format("pixel %d,%d: %s, expected %s", p[1], p[2], v, p[3]))
    end
end

-- tests --------------------------------------------------------------------------------
test("Create devices", function()
    T.spi_devices()
    T.modprobe("GPIO", 3, 0, GPIOD)
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = assert(io.open("/dev/i2c-ow-ctl", "r+b"))
    ctl:setvbuf("no")
end)

test("Power on: ACTIVE, REG_ID, chip ID, CPURESET", function()
    expect(control("eve shown") == "false", "LCD shown before ACTIVE")
    host_cmd(0x00) -- ACTIVE
    dnx.sys.msleep(50)
    expect(rd32(REG_ID) & 0xFF == 0x7C, "REG_ID " .. rd32(REG_ID))
    expect(rd(0x0C0000, 4) == "\x08\x17\x01\x00", "chip ID")
    expect(rd32(REG_CPURESET) == 0, "REG_CPURESET")
    expect(rd32(REG_FREQUENCY) == 60000000, "REG_FREQUENCY " .. rd32(REG_FREQUENCY))
end)

test("Display timing: LCD shown after REG_PCLK, frames counted", function()
    for _, r in ipairs({
        { REG_HCYCLE, 548 },
        { REG_HOFFSET, 43 },
        { REG_HSIZE, 480 },
        { REG_HSYNC0, 0 },
        { REG_HSYNC1, 41 },
        { REG_VCYCLE, 292 },
        { REG_VOFFSET, 12 },
        { REG_VSIZE, 272 },
        { REG_VSYNC0, 0 },
        { REG_VSYNC1, 10 },
    }) do
        wr16(r[1], r[2])
    end
    expect(control("eve shown") == "false", "LCD shown before REG_PCLK")
    wr8(REG_PCLK, 5)
    expect(control("eve shown") == "true", "LCD not shown after REG_PCLK")
    local f0 = rd32(REG_FRAMES)
    dnx.sys.msleep(200)
    local f1 = rd32(REG_FRAMES)
    expect(f1 > f0 + 5, "REG_FRAMES " .. f0 .. " -> " .. f1)
end)

test("Display list in RAM_DL, REG_DLSWAP returns to 0", function()
    wr(
        RAM_DL,
        words(
            CLEAR_COLOR_RGB(0, 0, 64),
            CLEAR(1, 1, 1),
            COLOR_RGB(255, 0, 0),
            BEGIN(RECTS),
            VERTEX2F(10 * 16, 10 * 16),
            VERTEX2F(49 * 16, 29 * 16),
            END,
            VERTEX_FORMAT(0),
            COLOR_RGB(0, 255, 0),
            BEGIN(RECTS),
            VERTEX2F(60, 10),
            VERTEX2F(99, 29),
            END,
            DISPLAY
        )
    )
    wr8(REG_DLSWAP, 2)
    local t0 = dnx.sys.uptime()
    while rd32(REG_DLSWAP) ~= 0 and dnx.sys.uptime() - t0 < 1000 do
        dnx.sys.msleep(5)
    end
    expect(rd32(REG_DLSWAP) == 0, "REG_DLSWAP is not cleared after swap")
    expect_pixels({ { 5, 5, BG }, { 20, 20, RED }, { 40, 25, RED }, { 70, 20, GREEN }, { 105, 20, BG } })
end)

test("Coprocessor: display list by REG_CMDB_WRITE burst (points, lines)", function()
    cmd(
        words(
            DLSTART,
            CLEAR_COLOR_RGB(0, 0, 64),
            CLEAR(1, 1, 1),
            COLOR_RGB(0, 0, 255),
            POINT_SIZE(10 * 16),
            BEGIN(POINTS),
            VERTEX2II(100, 100),
            END,
            COLOR_RGB(255, 255, 0),
            LINE_WIDTH(16),
            BEGIN(LINES),
            VERTEX2II(150, 50),
            VERTEX2II(250, 50),
            END,
            DISPLAY,
            SWAP
        )
    )
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMDB_SPACE) == 4092, "REG_CMDB_SPACE " .. rd32(REG_CMDB_SPACE))
    expect_pixels({
        { 100, 100, BLUE },
        { 109, 100, BLUE },
        { 100, 91, BLUE },
        { 108, 108, BG },
        { 200, 50, YELLOW },
        { 200, 53, BG },
        { 20, 20, BG },
    })
end)

test("Coprocessor results: MEMCRC, REGREAD, REG_CMD_DL", function()
    cmd(words(MEMWRITE, 0x1000, 12) .. "hello world!")
    cmd(words(MEMCRC, 0x1000, 12, 0))
    expect(wait_idle(), "FIFO not processed")
    expect(result() == 0x03B4C26D, string.format("MEMCRC %08X", result()))
    cmd(words(REGREAD, REG_ID, 0))
    expect(wait_idle() and result() & 0xFF == 0x7C, string.format("REGREAD %08X", result()))
    cmd(words(DLSTART, CLEAR(1, 1, 1)))
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMD_DL) == 4, "REG_CMD_DL " .. rd32(REG_CMD_DL))
end)

test("Widgets (not rendered) keep the FIFO in sync", function()
    cmd(
        words(DLSTART, CLEAR(1, 1, 1))
            .. words(TEXT, (10 << 16) | 10, (0 << 16) | 28)
            .. padded("Hello EVE\0")
            .. words(SETBASE, 16, NUMBER, (40 << 16) | 10, 28, 0x1234)
            .. words(BUTTON, (60 << 16) | 10, (30 << 16) | 100, (0 << 16) | 27)
            .. padded("OK\0")
            .. words(GAUGE, (100 << 16) | 100, (0 << 16) | 50, (4 << 16) | 10, (100 << 16) | 30)
            .. words(SLIDER, (150 << 16) | 10, (20 << 16) | 100, (40 << 16) | 0, 100)
            .. words(MEMWRITE, 0x2000, 4)
            .. "SYNC"
    )
    expect(wait_idle(), "FIFO not processed (FIFO out of sync)")
    expect(rd(0x2000, 4) == "SYNC", "MEMWRITE after widgets not executed")
end)

test("MEMWRITE of 6000 bytes (more than the FIFO)", function()
    local t = {}
    for i = 0, 5999 do
        t[#t + 1] = string.char((i * 13 + (i >> 8)) & 0xFF)
    end
    local data = table.concat(t)
    cmd(words(MEMWRITE, 0x4000, #data) .. data)
    expect(wait_idle(), "FIFO not processed")
    expect(rd(0x4000, #data) == data, "RAM_G content differs")
end)

for _, name in ipairs({ "stored", "fixed", "dyn" }) do
    test("CMD_INFLATE (zlib, " .. name .. " block)", function()
        local v = ZLIB[name]
        local ptr = 0x10000
        cmd(words(INFLATE, ptr) .. padded(v.data))
        expect(wait_idle(), "FIFO not processed")
        cmd(words(MEMCRC, ptr, v.len, 0))
        expect(wait_idle() and result() == v.crc, string.format("CRC %08X, expected %08X", result(), v.crc))
        cmd(words(GETPTR, 0))
        expect(wait_idle() and result() == ptr + v.len, "GETPTR " .. result() .. ", expected " .. (ptr + v.len))
    end)
end

-- zlib stream of data: blocks of literals coded by fixed Huffman codes and
-- stored blocks alternately (block of `block` bytes), adler: Adler-32 of trailer
local function zlib_stream(data, block, adler)
    local out, bytes, acc, nacc = { "\x78\x01" }, {}, 0, 0
    local function flush() -- bytes of bit writer to strings (memory of system)
        for k = 1, #bytes, 4096 do
            out[#out + 1] = string.char(table.unpack(bytes, k, math.min(k + 4095, #bytes)))
        end
        bytes = {}
    end
    local function put(v, n) -- n bits, LSB first
        acc, nacc = acc | (v << nacc), nacc + n
        while nacc >= 8 do
            bytes[#bytes + 1] = acc & 0xFF
            acc, nacc = acc >> 8, nacc - 8
        end
    end
    local function code(c, len) -- Huffman code, MSB first
        local r = 0
        for _ = 1, len do
            r, c = (r << 1) | (c & 1), c >> 1
        end
        put(r, len)
    end
    local nblocks = (#data + block - 1) // block
    for b = 1, nblocks do
        local chunk = data:sub((b - 1) * block + 1, b * block)
        local last = (b == nblocks) and 1 or 0
        if b % 2 == 0 then -- stored
            put(last, 1)
            put(0, 2)
            if nacc > 0 then
                put(0, 8 - nacc)
            end
            flush()
            out[#out + 1] = string.pack("<I2I2", #chunk, #chunk ~ 0xFFFF) .. chunk
        else -- fixed codes, literals
            put(last, 1)
            put(1, 2)
            for i = 1, #chunk do
                local lit = chunk:byte(i)
                if lit < 144 then
                    code(0x30 + lit, 8)
                else
                    code(0x190 + lit - 144, 9)
                end
            end
            code(0, 7) -- end of block (256)
            flush()
        end
    end
    if nacc > 0 then
        put(0, 8 - nacc)
    end
    flush()
    if not adler then
        local a, b = 1, 0
        for i = 1, #data do
            a = (a + data:byte(i)) % 65521
            b = (b + a) % 65521
        end
        adler = (b << 16) | a
    end
    out[#out + 1] = string.pack(">I4", adler)
    return table.concat(out)
end

test("CMD_INFLATE: long stream (blocks interrupted by FIFO), next command, Adler-32", function()
    local parts, t, x = {}, {}, 1
    for _ = 1, 24 do
        for i = 1, 1000 do
            x = (x * 1103515245 + 12345) & 0x7FFFFFFF
            t[i] = (x >> 16) & 0xFF
        end
        parts[#parts + 1] = string.char(table.unpack(t, 1, 1000))
    end
    local data = table.concat(parts)
    local z = zlib_stream(data, 3000)
    expect(#z > 4 * 4096, "stream is too short: " .. #z)
    -- the stream and the next command in one transfer: MEMWRITE after padding
    cmd(words(INFLATE, 0x20000) .. padded(z) .. words(MEMWRITE, 0x30000, 4) .. "next")
    expect(wait_idle(), "FIFO not processed")
    expect(rd(0x20000, #data) == data, "inflated data differ")
    expect(rd(0x30000, 4) == "next", "command after the stream: " .. rd(0x30000, 4))
    cmd(words(GETPTR, 0))
    expect(wait_idle() and result() == 0x20000 + #data, "GETPTR " .. result())
    -- wrong Adler-32: data are written, the end is correct
    local small = data:sub(1, 1000)
    cmd(words(INFLATE, 0x40000) .. padded(zlib_stream(small, 400, 0x12345678)) .. words(GETPTR, 0))
    expect(wait_idle() and result() == 0x40000 + #small, "GETPTR after wrong Adler-32 " .. result())
    expect(rd(0x40000, #small) == small, "data of stream with wrong Adler-32")
end)

test("Burst to REG_CMDB_WRITE longer than the FIFO (consumed during the transfer)", function()
    local parts, t, x = {}, {}, 7
    for _ = 1, 12 do
        for i = 1, 1000 do
            x = (x * 1103515245 + 12345) & 0x7FFFFFFF
            t[i] = (x >> 16) & 0xFF
        end
        parts[#parts + 1] = string.char(table.unpack(t, 1, 1000))
    end
    local data = table.concat(parts)
    local z = zlib_stream(data, 2500)
    expect(#z > 2 * 4096, "stream is too short: " .. #z)
    wr(REG_CMDB_WRITE, words(INFLATE, 0x50000) .. padded(z) .. words(MEMWRITE, 0x60000, #data) .. data)
    expect(wait_idle(), "FIFO not processed")
    expect(rd(0x50000, #data) == data, "inflated data differ")
    expect(rd(0x60000, #data) == data, "MEMWRITE data differ")
    expect(rd32(REG_CMD_READ) ~= 0xFFF, "coprocessor fault")
end)

-- write by RAM_CMD + REG_CMD_WRITE (FT81x HAL: Ft_Gpu_Hal_WrCmdBuf); data are not
-- split at the end of the ring, the chip wraps the address within RAM_CMD
local function ramcmd_write(data)
    local pos = 1
    local t0 = dnx.sys.uptime()
    while pos <= #data do
        local wp, rp = rd32(REG_CMD_WRITE), rd32(REG_CMD_READ)
        local free = 4092 - ((wp - rp) & 0xFFF)
        if free > 0 then
            local n = math.min(free, #data - pos + 1)
            wr(RAM_CMD + wp, data:sub(pos, pos + n - 1))
            wr16(REG_CMD_WRITE, ((wp + n + 3) & 0xFFC) & 0xFFF)
            pos = pos + n
        elseif dnx.sys.uptime() - t0 > 3000 then
            error("command FIFO full", 2)
        else
            dnx.sys.msleep(5)
        end
    end
end

test("Write to RAM_CMD + REG_CMD_WRITE across the end of the ring (FT81x HAL)", function()
    -- move the FIFO pointers near the end of the ring
    local fill = (0xF00 - rd32(REG_CMD_WRITE) - 12) & 0xFFC
    cmd(words(MEMWRITE, 0x7F000, fill) .. string.rep("\0", fill))
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMD_WRITE) == 0xF00, "write pointer " .. rd32(REG_CMD_WRITE))
    -- INFLATE command and stream by separate writes, as transfer_image() of SGL
    local parts, t, x = {}, {}, 11
    for _ = 1, 10 do
        for i = 1, 1000 do
            x = (x * 1103515245 + 12345) & 0x7FFFFFFF
            t[i] = (x >> 16) & 0x0F -- L4-like data (compressible)
        end
        parts[#parts + 1] = string.char(table.unpack(t, 1, 1000))
    end
    local data = table.concat(parts)
    local z = zlib_stream(data, 1800)
    ramcmd_write(words(INFLATE, 0x80000))
    ramcmd_write(z) -- odd length: padding by REG_CMD_WRITE alignment
    ramcmd_write(words(MEMWRITE, 0x8F000, 4) .. "wrap")
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMD_READ) ~= 0xFFF, "coprocessor fault")
    expect(rd(0x80000, #data) == data, "inflated data differ")
    expect(rd(0x8F000, 4) == "wrap", "command after the stream: " .. rd(0x8F000, 4))
    -- one long write (frame of SGL, up to 4000 bytes) across the end of the ring
    local wp = rd32(REG_CMD_WRITE)
    local n = 4000 - (4000 % 4)
    local blob = string.rep("\x5A", n - 12)
    ramcmd_write(words(MEMWRITE, 0x8E000, #blob) .. blob)
    expect(wait_idle(), "FIFO not processed")
    expect(((wp + n) & 0xFFF) < wp, "write did not cross the end of the ring")
    expect(rd(0x8E000, #blob) == blob, "MEMWRITE data across the end of the ring differ")
    -- reading RAM_CMD across the end of the ring
    local back = rd(RAM_CMD + 0xFFC, 8)
    expect(back == rd(RAM_CMD + 0xFFC, 4) .. rd(RAM_CMD, 4), "RAM_CMD read does not wrap")
end)

test("CMD_LOADIMAGE: JPEG/PNG in the FIFO are skipped, next command executed", function()
    local LOADIMAGE = CMD(0x24)
    local function chunk(typ, data)
        return string.pack(">I4", #data) .. typ .. data .. "\0\0\0\0"
    end
    local png = "\x89PNG\r\n\x1A\n"
        .. chunk("IHDR", string.pack(">I4I4BBBBB", 4, 2, 8, 2, 0, 0, 0))
        .. chunk("IDAT", string.rep("\xAB", 999))
        .. chunk("IEND", "")
    local function seg(m, data)
        return "\xFF" .. string.char(m) .. string.pack(">I2", #data + 2) .. data
    end
    local jpeg = "\xFF\xD8"
        .. seg(0xE0, "JFIF\0\1\1\0\0\1\0\1\0\0")
        .. seg(0xDB, string.rep("\1", 65))
        .. seg(0xC0, "\8\0\2\0\4\1\1\x11\0")
        .. seg(0xDA, "\1\1\0\0\x3F\0")
        .. "\x12\xFF\x00\x34\xFF\xD0\x56\xFF\xFF\xD9" -- FF00, RST0, fill byte, EOI
    -- the image in one transfer
    cmd(words(LOADIMAGE, 0x90000, 0) .. padded(png) .. words(MEMWRITE, 0x91000, 4) .. "png!")
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMD_READ) ~= 0xFFF, "coprocessor fault (PNG)")
    expect(rd(0x91000, 4) == "png!", "command after PNG: " .. rd(0x91000, 4))
    -- the image in small pieces (structure across writes)
    local data = words(LOADIMAGE, 0x92000, 0) .. padded(jpeg) .. words(MEMWRITE, 0x93000, 4) .. "jpg!"
    for k = 1, #data, 8 do
        wr(REG_CMDB_WRITE, data:sub(k, k + 7))
    end
    expect(wait_idle(), "FIFO not processed")
    expect(rd32(REG_CMD_READ) ~= 0xFFF, "coprocessor fault (JPEG)")
    expect(rd(0x93000, 4) == "jpg!", "command after JPEG: " .. rd(0x93000, 4))
end)

-- coprocessor recovery after fault (sequence of EVE host libraries)
local function cpu_reset()
    wr32(REG_CPURESET, 1)
    wr32(REG_CMD_READ, 0)
    wr32(REG_CMD_WRITE, 0)
    wr32(REG_CMD_DL, 0)
    wr32(REG_CPURESET, 0)
end

test("Coprocessor fault: REG_CMD_READ FFFh, RAM_ERR_REPORT, recovery by REG_CPURESET", function()
    local RAM_ERR_REPORT = 0x309800
    -- invalid deflate block (type 3) after valid zlib header, then a command
    cmd(words(INFLATE, 0x70000) .. padded("\x78\x01\x07\x00") .. words(MEMWRITE, 0x70100, 4) .. "FAIL")
    dnx.sys.msleep(20)
    expect(rd32(REG_CMD_READ) == 0xFFF, string.format("REG_CMD_READ %X", rd32(REG_CMD_READ)))
    expect(rd32(REG_CMDB_SPACE) & 3 ~= 0, string.format("REG_CMDB_SPACE %X", rd32(REG_CMDB_SPACE)))
    local msg = rd(RAM_ERR_REPORT, 128):match("^[^%z]*")
    expect(msg:find("inflate", 1, true) == 1, "RAM_ERR_REPORT: " .. msg)
    expect(rd(0x70100, 4) ~= "FAIL", "command after fault is executed")
    cpu_reset()
    expect(rd32(REG_CMD_READ) == 0, "REG_CMD_READ after reset " .. rd32(REG_CMD_READ))
    cmd(words(MEMWRITE, 0x70100, 4) .. "OK!!")
    expect(wait_idle() and rd(0x70100, 4) == "OK!!", "coprocessor after recovery")
    -- unknown command
    cmd(words(0xFFFFFF7F))
    dnx.sys.msleep(20)
    expect(rd32(REG_CMD_READ) == 0xFFF, "unknown command: no fault")
    cpu_reset()
    expect(rd32(REG_CMD_READ) == 0 and rd(RAM_ERR_REPORT, 1) == "\0", "fault cleared")
end)

test("CMD_APPEND and bitmaps (RGB565, L8 colored by COLOR_RGB)", function()
    local bmp = string.rep(string.pack("<I2", 0xF800), 4) .. string.rep(string.pack("<I2", 0x07E0), 4)
    cmd(words(MEMWRITE, 0x3000, #bmp) .. bmp)
    cmd(words(MEMWRITE, 0x3100, 4) .. "\xFF\xFF\x00\x00") -- L8 2x2: white row, black row
    local dl = words(CLEAR_COLOR_RGB(0, 0, 64), CLEAR(1, 1, 1))
    cmd(words(MEMWRITE, 0x3200, #dl) .. dl)
    cmd(
        words(
            DLSTART,
            APPEND,
            0x3200,
            #dl,
            BITMAP_SOURCE(0x3000),
            BITMAP_LAYOUT(RGB565, 8, 2),
            BITMAP_SIZE(4, 2),
            BEGIN(BITMAPS),
            VERTEX2II(300, 200),
            END,
            BITMAP_SOURCE(0x3100),
            BITMAP_LAYOUT(L8, 2, 2),
            BITMAP_SIZE(2, 2),
            COLOR_RGB(0, 0, 255),
            BEGIN(BITMAPS),
            VERTEX2II(320, 200),
            END,
            DISPLAY,
            SWAP
        )
    )
    expect(wait_idle(), "FIFO not processed")
    expect_pixels({
        { 5, 5, BG },
        { 300, 200, RED },
        { 303, 200, RED },
        { 300, 201, GREEN },
        { 304, 200, BG },
        { 320, 200, BLUE },
        { 321, 200, BLUE },
        { 320, 201, BG },
    })
end)

test("CALL, JUMP, RETURN, MACRO", function()
    wr32(REG_MACRO_0, COLOR_RGB(255, 255, 255))
    wr(
        RAM_DL,
        words(
            CLEAR_COLOR_RGB(0, 0, 64),
            CLEAR(1, 1, 1), -- 0, 1
            JUMP(5), -- 2
            COLOR_RGB(255, 0, 0),
            DISPLAY, -- 3, 4 (skipped)
            CALL(11), -- 5: subroutine sets color
            BEGIN(RECTS),
            VERTEX2II(10, 100),
            VERTEX2II(19, 109),
            END, -- 6..9
            JUMP(13), -- 10
            COLOR_RGB(0, 255, 0),
            RETURN, -- 11, 12
            MACRO(0),
            BEGIN(RECTS),
            VERTEX2II(30, 100),
            VERTEX2II(39, 109),
            END,
            DISPLAY
        )
    ) -- 13..18
    wr8(REG_DLSWAP, 2)
    dnx.sys.msleep(20)
    expect_pixels({ { 15, 105, GREEN }, { 35, 105, WHITE }, { 25, 105, BG } })
end)

test("Interrupt: SWAP flag drives INT_N, cleared by read", function()
    rd32(REG_INT_FLAGS)
    wr8(REG_INT_MASK, 0x01) -- SWAP
    wr8(REG_INT_EN, 1)
    expect(int_pin(), "INT_N low before swap")
    cmd(words(DLSTART, CLEAR(1, 1, 1), DISPLAY, SWAP))
    expect(wait_idle(), "FIFO not processed")
    expect(not int_pin(), "INT_N not low after swap")
    expect(rd32(REG_INT_FLAGS) & 1 == 1, "SWAP flag not set")
    expect(int_pin(), "INT_N not released after read of REG_INT_FLAGS")
    wr8(REG_INT_EN, 0)
end)

test("Touch registers", function()
    control("eve touch 123 45 1")
    expect(rd32(REG_TOUCH_SCREEN_XY) == (123 << 16) | 45, string.format("touch %08X", rd32(REG_TOUCH_SCREEN_XY)))
    control("eve touch 0 0 0")
    expect(rd32(REG_TOUCH_SCREEN_XY) == 0x80008000, string.format("no touch %08X", rd32(REG_TOUCH_SCREEN_XY)))
end)

test("L4/L2 bitmaps: partial bytes, scissor, edge alpha", function()
    -- L4 5x1: 15, 0, 8, 15, 15 (stride 3); L2 6x1: 3, 3, 0, 3, 2, 3 (stride 2)
    wr(0x7800, "\xF0\x8F\xF0" .. "\xF3\xB0")
    local function scissor(x, y, w, h)
        return 0x1B000000 | (x << 11) | y, 0x1C000000 | (w << 12) | h
    end
    cmd(
        words(DLSTART, CLEAR_COLOR_RGB(0, 0, 64), CLEAR(1, 1, 1), COLOR_RGB(255, 255, 0), BITMAP_HANDLE(0))
            .. words(BITMAP_SOURCE(0x7800), BITMAP_LAYOUT(L4, 3, 1), BITMAP_SIZE(5, 1), BEGIN(BITMAPS))
            .. words(VERTEX2II(300, 100, 0, 0))
            .. words(scissor(311, 0, 3, 272)) -- clips first pixel and the last one (310..314 -> 311..313)
            .. words(VERTEX2II(310, 100, 0, 0), scissor(0, 0, 2048, 2048))
            .. words(BITMAP_SOURCE(0x7803), BITMAP_LAYOUT(L2, 2, 1), BITMAP_SIZE(6, 1), VERTEX2II(300, 120, 0, 0))
            .. words(END, DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect_pixels({
        { 300, 100, YELLOW },
        { 301, 100, BG },
        { 302, 100, "8C43" }, -- luminance 8/15: alpha 136 over background
        { 303, 100, YELLOW },
        { 304, 100, YELLOW },
        { 305, 100, BG },
        { 310, 100, BG }, -- scissor
        { 311, 100, BG },
        { 312, 100, "8C43" },
        { 313, 100, YELLOW },
        { 314, 100, BG }, -- scissor
        { 300, 120, YELLOW },
        { 301, 120, YELLOW },
        { 302, 120, BG },
        { 303, 120, YELLOW },
        { 304, 120, "AD42" }, -- luminance 2/3: alpha 170
        { 305, 120, YELLOW },
        { 306, 120, BG },
    })
end)

test("Bitmap cells (own font glyphs): VERTEX2II cell, CELL, CMD_SETBITMAP", function()
    -- L1 glyphs 8x8: cell 0 empty, cell 1 full, cell 2 left half
    wr(0x2000, string.rep("\0", 8) .. string.rep("\xFF", 8) .. string.rep("\xF0", 8))
    wr(0x3000, string.rep(string.pack("<I2", 0xF800), 16)) -- RGB565 4x4 red
    cmd(
        words(DLSTART, CLEAR_COLOR_RGB(0, 0, 64), CLEAR(1, 1, 1), COLOR_RGB(255, 255, 0), BITMAP_HANDLE(0))
            .. words(BITMAP_SOURCE(0x2000), BITMAP_LAYOUT(1, 1, 8), BITMAP_SIZE(8, 8), BEGIN(BITMAPS))
            .. words(VERTEX2II(10, 10, 0, 2), CELL(1), VERTEX2F(30 * 16, 10 * 16), VERTEX2II(50, 10, 0, 0), END)
            .. words(BITMAP_HANDLE(1), SETBITMAP, 0x3000, (4 << 16) | RGB565, 4, COLOR_RGB(255, 255, 255))
            .. words(BEGIN(BITMAPS), VERTEX2II(100, 10, 1, 0), VERTEX2II(120, 10, 20, 0), END, DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect_pixels({
        { 11, 11, YELLOW },
        { 16, 11, BG },
        { 33, 13, YELLOW },
        { 37, 17, YELLOW },
        { 53, 13, BG },
        { 101, 11, RED },
        { 103, 13, RED },
        { 105, 11, BG },
        { 121, 11, BG },
    })
end)

local function touch_tag(x, y)
    control(string.format("eve touch %d %d 1", x, y))
    return rd32(REG_TOUCH_TAG)
end

test("Tags: TAG, TAG_MASK, CLEAR_TAG, REG_TOUCH_TAG", function()
    cmd(
        words(DLSTART, CLEAR_COLOR_RGB(0, 0, 8), CLEAR_TAG(0), CLEAR(1, 1, 1), BEGIN(RECTS))
            .. words(TAG(10), VERTEX2F(10 * 16, 10 * 16), VERTEX2F(50 * 16, 50 * 16))
            .. words(TAG_MASK(0), TAG(20), VERTEX2F(60 * 16, 10 * 16), VERTEX2F(100 * 16, 50 * 16), TAG_MASK(1))
            .. words(BEGIN(POINTS), POINT_SIZE(20 * 16), TAG(30), VERTEX2F(200 * 16, 100 * 16), END, DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect(touch_tag(30, 30) == 10, "tag of rectangle " .. rd32(REG_TOUCH_TAG))
    expect(rd32(REG_TOUCH_TAG_XY) == (30 << 16) | 30, string.format("REG_TOUCH_TAG_XY %08X", rd32(REG_TOUCH_TAG_XY)))
    expect(touch_tag(80, 30) == 0, "TAG_MASK(0): tag " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(205, 105) == 30, "tag of point " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(300, 200) == 0, "background tag " .. rd32(REG_TOUCH_TAG))
    control("eve touch 0 0 0")
    expect(rd32(REG_TOUCH_TAG) == 0, "no touch: tag " .. rd32(REG_TOUCH_TAG))
    -- background tag by CLEAR_TAG, tag lookup follows the new frame
    control("eve touch 300 200 1")
    cmd(words(DLSTART, CLEAR_TAG(5), CLEAR(0, 0, 1), DISPLAY, SWAP))
    expect(wait_idle() and rd32(REG_TOUCH_TAG) == 5, "CLEAR_TAG: tag " .. rd32(REG_TOUCH_TAG))
    control("eve touch 0 0 0")
end)

test("COLOR_MASK without colors: alpha mask of SGL progress bar (only tags drawn)", function()
    local function COLOR_MASK(r, g, b, a)
        return 0x20000000 | (r << 3) | (g << 2) | (b << 1) | a
    end
    local BLEND_FUNC = 0x0B000000 | (3 << 3) | 5 -- DST_ALPHA, ONE_MINUS_DST_ALPHA
    cmd(
        words(DLSTART, CLEAR_COLOR_RGB(0, 0, 64), CLEAR_TAG(0), CLEAR(1, 1, 1), BEGIN(RECTS))
            .. words(COLOR_RGB(255, 0, 0), VERTEX2F(10 * 16, 10 * 16), VERTEX2F(50 * 16, 50 * 16))
            .. words(COLOR_RGB(255, 255, 255), COLOR_MASK(0, 0, 0, 1), CLEAR_COLOR_RGB(0, 255, 0), CLEAR(1, 0, 0))
            .. words(TAG(40), BEGIN(POINTS), POINT_SIZE(10 * 16), VERTEX2F(30 * 16, 30 * 16))
            .. words(COLOR_MASK(1, 1, 1, 1), BLEND_FUNC, 0x0B000000 | (2 << 3) | 4, END, DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect_pixels({
        { 30, 30, RED }, -- point in alpha only
        { 12, 12, RED }, -- CLEAR(1, 0, 0) of alpha only
        { 100, 100, BG },
    })
    expect(touch_tag(30, 30) == 40, "tag of masked point " .. rd32(REG_TOUCH_TAG))
    control("eve touch 0 0 0")
end)

test("RECTS with LINE_WIDTH: rounded corners, anti-aliased edges, tags", function()
    cmd(
        words(DLSTART, CLEAR_COLOR_RGB(0, 0, 64), CLEAR_TAG(0), CLEAR(1, 1, 1), COLOR_RGB(255, 0, 0))
            .. words(
                TAG(77),
                LINE_WIDTH(10 * 16),
                BEGIN(RECTS),
                VERTEX2F(50 * 16, 50 * 16),
                VERTEX2F(150 * 16, 100 * 16)
            )
            .. words(LINE_WIDTH(16), VERTEX2F(200 * 16, 50 * 16), VERTEX2F(210 * 16, 60 * 16), END, DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect_pixels({
        { 41, 75, RED }, -- rectangle grows by LINE_WIDTH - 1 px
        { 40, 75, BG },
        { 159, 75, RED },
        { 160, 75, BG },
        { 100, 41, RED },
        { 100, 40, BG },
        { 41, 41, BG }, -- rounded corner
        { 44, 44, RED },
        { 200, 50, RED }, -- LINE_WIDTH 1 px: sharp rectangle on the vertices
        { 210, 60, RED },
        { 199, 50, BG },
        { 211, 60, BG },
    })
    local v = tonumber(pixel(43, 44), 16) -- edge of the corner: partial coverage
    local r5, g6 = v >> 11, (v >> 5) & 0x3F
    expect(r5 > 4 and r5 < 28 and g6 == 0, "anti-aliased edge " .. pixel(43, 44))
    expect(touch_tag(44, 44) == 77, "tag inside the corner " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(41, 41) == 0, "tag outside the corner " .. rd32(REG_TOUCH_TAG))
    control("eve touch 0 0 0")
end)

test("Widgets: BUTTON with tag, KEYS tags are characters, FGCOLOR", function()
    cmd(
        words(DLSTART, CLEAR(1, 1, 1), FGCOLOR, 0xFF0000, TAG(1))
            .. words(BUTTON, (150 << 16) | 20, (40 << 16) | 100, (0 << 16) | 27)
            .. padded("OK\0")
            .. words(FGCOLOR, 0x0000FF, KEYS, (150 << 16) | 150, (40 << 16) | 200, (string.byte("B") << 16) | 27)
            .. padded("ABC\0")
            .. words(DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    expect(pixel(70, 170) == RED, "button color " .. pixel(70, 170))
    expect(touch_tag(70, 170) == 1, "button tag " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(160, 170) == string.byte("A"), "key A tag " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(250, 170) == string.byte("B"), "key B tag " .. rd32(REG_TOUCH_TAG))
    expect(touch_tag(340, 170) == string.byte("C"), "key C tag " .. rd32(REG_TOUCH_TAG))
    expect(pixel(160, 170) == BLUE, "key color " .. pixel(160, 170))
    expect(pixel(250, 170) ~= BLUE, "pressed key not highlighted")
    control("eve touch 0 0 0")
end)

test("CMD_TRACK: REG_TRACKER of linear and rotary tracking", function()
    cmd(
        words(DLSTART, CLEAR(1, 1, 1), TRACK, (220 << 16) | 100, (20 << 16) | 200, 5)
            .. words(TAG(5), SLIDER, (220 << 16) | 100, (20 << 16) | 200, (50 << 16) | 0, 100)
            .. words(TRACK, (60 << 16) | 400, (1 << 16) | 1, 6, TAG(6), DIAL, (60 << 16) | 400, (0 << 16) | 30, 0)
            .. words(DISPLAY, SWAP)
    )
    expect(wait_idle(), "FIFO not processed")
    touch_tag(200, 230)
    local v = rd32(REG_TRACKER)
    expect(v & 0xFF == 5 and math.abs((v >> 16) - 32767) < 400, string.format("slider tracker %08X", v))
    touch_tag(150, 230)
    v = rd32(REG_TRACKER)
    expect(v & 0xFF == 5 and math.abs((v >> 16) - 16383) < 400, string.format("slider tracker %08X", v))
    touch_tag(380, 60) -- left of the dial center: quarter turn clockwise from bottom
    v = rd32(REG_TRACKER)
    expect(v & 0xFF == 6 and math.abs((v >> 16) - 16384) < 200, string.format("dial tracker %08X", v))
    control("eve touch 0 0 0")
    expect(rd32(REG_TRACKER) == 0, string.format("no touch: tracker %08X", rd32(REG_TRACKER)))
end)

test("Extended touch mode: REG_TOUCH_TAG1, REG_TRACKER_1", function()
    wr8(REG_CTOUCH_EXTENDED, 0)
    control("eve touch_slot 1 30 30 1")
    expect(rd32(REG_TOUCH_TAG1_XY) == (30 << 16) | 30, string.format("TAG1_XY %08X", rd32(REG_TOUCH_TAG1_XY)))
    expect(rd32(REG_TOUCH_TAG1) == 0, "tag 1 outside widgets " .. rd32(REG_TOUCH_TAG1))
    control("eve touch_slot 1 200 230 1")
    expect(rd32(REG_TOUCH_TAG1) == 5, "tag 1 on slider " .. rd32(REG_TOUCH_TAG1))
    expect(rd32(REG_TRACKER_1) & 0xFF == 5, string.format("REG_TRACKER_1 %08X", rd32(REG_TRACKER_1)))
    control("eve touch_slot 1 0 0 0")
    expect(rd32(REG_TOUCH_TAG1) == 0, "tag 1 released " .. rd32(REG_TOUCH_TAG1))
    wr8(REG_CTOUCH_EXTENDED, 1)
end)

test("Unchanged frame is not rendered again, RAM_G change renders", function()
    wr(0x7000, string.rep(string.pack("<I2", 0xF800), 16)) -- RGB565 4x4 red
    local body = words(DLSTART, CLEAR_COLOR_RGB(0, 0, 64), CLEAR(1, 1, 1), BITMAP_HANDLE(0))
        .. words(BITMAP_SOURCE(0x7000), BITMAP_LAYOUT(RGB565, 8, 4), BITMAP_SIZE(4, 4), BEGIN(BITMAPS))
        .. words(VERTEX2II(200, 200, 0, 0), END, DISPLAY, SWAP)
    cmd(body)
    expect(wait_idle() and pixel(201, 201) == RED, "bitmap " .. pixel(201, 201))
    local skipped = tonumber(control("eve skipped"))
    cmd(body)
    expect(wait_idle(), "FIFO not processed")
    expect(tonumber(control("eve skipped")) == skipped + 1, "same frame rendered again")
    expect(pixel(201, 201) == RED, "bitmap after skipped render " .. pixel(201, 201))
    wr(0x7000, string.rep(string.pack("<I2", 0x07E0), 16)) -- bitmap changed to green
    cmd(body)
    expect(wait_idle() and pixel(201, 201) == GREEN, "changed bitmap " .. pixel(201, 201))
    expect(tonumber(control("eve skipped")) == skipped + 1, "frame with changed RAM_G skipped")
    expect(rd32(REG_DLSWAP) == 0, "REG_DLSWAP not cleared")
end)

test("Performance: upload and rendering (printed)", function()
    collectgarbage() -- data of previous tests (memory of the system)
    local function now()
        return dnx.sys.uptime()
    end
    local size = 480 * 272 * 2 -- full screen RGB565, sent in chunks (memory of the system)
    local chunk = string.rep(string.pack("<I2", 0x07E0), 8192)
    local t = now()
    cmd(words(MEMWRITE, 0x10000, size))
    for _ = 1, size // #chunk do
        cmd(chunk)
    end
    cmd(chunk:sub(1, size % #chunk))
    expect(wait_idle(), "FIFO not processed")
    local dt = math.max(now() - t, 1)
    print(string.format("  upload by MEMWRITE %d B: %d ms (%.0f kB/s)", size, dt, size / 1024 / (dt / 1000)))
    -- text-like images 400x40 (L8, L4, L2): 70 % transparent, 20 % opaque, 10 % edges
    local function text_image(bits)
        local per, max, x, out, acc, n = 8 // bits, (1 << bits) - 1, 12345, {}, 0, 0
        for _ = 1, 400 * 40 do
            x = (x * 1103515245 + 12345) & 0x7FFFFFFF
            local r = (x >> 16) % 10
            local l = r < 7 and 0 or r < 9 and max or (max + 1) // 2
            acc, n = (acc << bits) | l, n + 1 -- first pixel in the high bits
            if n == per then
                out[#out + 1] = string.char(acc)
                acc, n = 0, 0
            end
        end
        return table.concat(out)
    end
    local TEXT = { { "L8", L8, 400, 0x50000 }, { "L4", L4, 200, 0x58000 }, { "L2", L2, 100, 0x5C000 } }
    for _, tx in ipairs(TEXT) do
        local img = text_image(tx[2] == L8 and 8 or tx[2] == L4 and 4 or 2)
        for off = 0, #img - 1, 4096 do
            wr(tx[4] + off, img:sub(off + 1, off + 4096))
        end
    end
    -- full screen L4 image 480x272 (same structure)
    local l4row = text_image(4):sub(1, 240)
    for y = 0, 271 do
        wr(0xA0000 + y * 240, l4row)
    end

    local function frame(name, body, same)
        local n = 10
        local t0 = now()
        for k = 1, n do
            local c = same and 64 or k % 2 -- different clear color: every frame is rendered
            cmd(words(DLSTART, CLEAR_COLOR_RGB(0, 0, c), CLEAR(1, 1, 1)) .. body .. words(DISPLAY, SWAP))
            expect(wait_idle(), "FIFO not processed")
        end
        local ms = (now() - t0) / n
        print(string.format("  %-40s %6.1f ms/frame (%.1f fps)", name, ms, 1000 / math.max(ms, 0.1)))
        expect(ms < 1000, name .. ": " .. ms .. " ms/frame")
    end

    t = now()
    for off = 0, size - 1, #chunk do
        wr(0x60000 + off, chunk:sub(1, math.min(#chunk, size - off)))
    end
    dt = math.max(now() - t, 1)
    print(string.format("  direct write to RAM_G %d B: %d ms (%.0f kB/s)", size, dt, size / 1024 / (dt / 1000)))

    collectgarbage()
    do -- inflate of 32 kB stream (fixed/stored blocks of 4 kB)
        local data = string.rep("\x12\x34\x56\x78\x9A\xBC\xDE\xF0", 4096)
        local z = zlib_stream(data, 4096)
        local t0 = now()
        cmd(words(INFLATE, 0x80000))
        for off = 1, #z, 8192 do
            cmd(z:sub(off, off + 8191))
        end
        cmd(string.rep("\0", (4 - #z % 4) % 4))
        expect(wait_idle(), "FIFO not processed")
        local dt = math.max(now() - t0, 1)
        print(
            string.format(
                "  inflate %d B -> %d B: %d ms (%.0f kB/s of output)",
                #z,
                #data,
                dt,
                #data / 1024 / (dt / 1000)
            )
        )
    end
    frame("clear", "")
    frame(
        "clear + full screen RGB565 bitmap",
        words(BITMAP_SOURCE(0x10000), BITMAP_LAYOUT(RGB565, 960 & 0x3FF, 272), 0x28000000 | ((960 >> 10) << 2))
            .. words(BITMAP_SIZE(480, 272), BEGIN(BITMAPS), VERTEX2II(0, 0, 0, 0), END)
    )
    for _, tx in ipairs(TEXT) do
        local txt = words(BITMAP_SOURCE(tx[4]), BITMAP_LAYOUT(tx[2], tx[3], 40), BITMAP_SIZE(400, 40))
            .. words(BEGIN(BITMAPS))
        for i = 0, 4 do
            txt = txt .. words(VERTEX2II(40, 10 + i * 50, 0, 0))
        end
        frame("clear + 5 text images " .. tx[1] .. " 400x40", txt .. words(END))
    end
    frame(
        "clear + full screen L4 image",
        words(BITMAP_SOURCE(0xA0000), BITMAP_LAYOUT(L4, 240, 272), BITMAP_SIZE(480, 272), BEGIN(BITMAPS))
            .. words(VERTEX2II(0, 0, 0, 0), END)
    )
    local shapes = words(BEGIN(RECTS))
    for i = 0, 9 do
        shapes = shapes
            .. words(COLOR_RGB(255, i * 20, 0), VERTEX2F(i * 40 * 16, 20 * 16), VERTEX2F((i * 40 + 30) * 16, 60 * 16))
    end
    shapes = shapes .. words(BEGIN(POINTS), POINT_SIZE(15 * 16))
    for i = 0, 9 do
        shapes = shapes .. words(VERTEX2F((20 + i * 45) * 16, 150 * 16))
    end
    frame("clear + 10 rects 30x40 + 10 points r=15", shapes .. words(END))
    frame(
        "unchanged frame (full screen bitmap)",
        words(BITMAP_SOURCE(0x10000), BITMAP_LAYOUT(RGB565, 960 & 0x3FF, 272))
            .. words(0x28000000 | ((960 >> 10) << 2), BITMAP_SIZE(480, 272), BEGIN(BITMAPS), VERTEX2II(0, 0, 0, 0), END),
        true
    )
end)

test("PWRDOWN: LCD hidden, registers lost, ACTIVE wakes", function()
    host_cmd(0x43)
    expect(control("eve shown") == "false", "LCD shown after PWRDOWN")
    host_cmd(0x00)
    dnx.sys.msleep(50)
    expect(rd32(REG_ID) & 0xFF == 0x7C, "REG_ID after ACTIVE")
    expect(rd32(REG_PCLK) == 0, "REG_PCLK kept after PWRDOWN")
    expect(control("eve shown") == "false", "LCD shown without REG_PCLK")
end)

test("Close", function()
    ctl:close()
end)

T.summary()
