--[[----------------------------------------------------------------------------
  MAX310X driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: MAX3109 (chip 0) on SPI:1 (CS PA29, RST PA30) with serial
  lines of UARTs connected to UART:4 and UART:5 (/dev/ttyS4, /dev/ttyS5),
  MAX3107 (chip 1) on I2C:1 (0x2C, crystal), MAX3108 (chip 2) on SPI:1 (CS
  PA31), MAX3109 (chip 3) on I2C:1 (0x6C, UART 1 0x5C), chip with wrong
  revision ID on I2C:1 (0x2D). Registers and format programmed by the driver
  are checked by the test control device (I2C:0 0x30).
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local test, expect = T.test, T.expect

local M0, M1 = "/dev/ttyM0", "/dev/ttyM1" -- MAX3109 (SPI) UART 0, 1
local M2 = "/dev/ttyM2" -- MAX3107 (I2C)
local M3 = "/dev/ttyM3" -- MAX3108 (SPI)
local M4, M5 = "/dev/ttyM4", "/dev/ttyM5" -- MAX3109 (I2C) UART 0, 1
local S4, S5 = "/dev/ttyS4", "/dev/ttyS5"
local CHIP0 = "path=/dev/spi-max3109,chip=max3109,clock=3686400,reset_port=0,reset_pin=30"

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. tostring(out), 2)
    end
    return out
end

local function configure(path, cfg)
    return T.run(string.format("modprobe -c %s %s", path, cfg))
end

local function open(path)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    return f
end

local function pattern(n, seed)
    local t, x = {}, seed
    for i = 1, n do
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        t[i] = string.char((x >> 16) & 0xFF)
    end
    return table.concat(t)
end

-- write data to one file and read it from other (or the same) file
local function transfer(wpath, rpath, data)
    local w = open(wpath)
    local r = (rpath == wpath) and w or open(rpath)
    assert(w:write(data))
    local rd = r:read(#data)
    w:close()
    if r ~= w then
        r:close()
    end
    return rd
end

-- test control device ------------------------------------------------------------
local ctl

local function control(cmd)
    ctl:seek("set", 0)
    assert(ctl:write(cmd))
    ctl:seek("set", 0)
    local r = assert(ctl:read(128)):match("^[^%z]*")
    if r:find("^error") then
        error("control '" .. cmd .. "': " .. r, 2)
    end
    return r
end

local function format(chip, n)
    return control(string.format("max310x %d %d format", chip, n))
end

local function reg(chip, n, addr)
    return tonumber(control(string.format("max310x %d %d reg %d", chip, n, addr)))
end

-- tests ------------------------------------------------------------------------------
test("Create devices (modprobe -c), chip reset and detection", function()
    T.spi_devices()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    T.modprobe("I2C", 1, 20, "/dev/i2c-max3107", "address=0x2C,sub_addr_mode=1")
    T.modprobe("I2C", 1, 21, "/dev/i2c-max3107-bad", "address=0x2D,sub_addr_mode=1")
    T.modprobe("I2C", 1, 22, "/dev/i2c-max3109-0", "address=0x6C,sub_addr_mode=1")
    T.modprobe("I2C", 1, 23, "/dev/i2c-max3109-1", "address=0x5C,sub_addr_mode=1")
    ctl = open("/dev/i2c-ow-ctl")
    T.modprobe("MAX310X", 0, 0, M0, CHIP0 .. ",baud=115200,loopback=1")
    T.modprobe("MAX310X", 0, 1, M1, "baud=57600,loopback=1")
    T.modprobe("MAX310X", 1, 0, M2,
        "path=/dev/i2c-max3107,interface=i2c,chip=max3107,clock=1843200,crystal=1,baud=9600,loopback=1")
    T.modprobe("MAX310X", 2, 0, M3, "path=/dev/spi-max3108,chip=max3108,clock=3686400,baud=38400,loopback=1")
    T.modprobe("MAX310X", 3, 0, M4,
        "path=/dev/i2c-max3109-0,interface=i2c,chip=max3109,clock=3686400,baud=19200,loopback=1")
    T.modprobe("MAX310X", 3, 1, M5, "path=/dev/i2c-max3109-1,interface=i2c,baud=4800,loopback=1")
    local expected = {
        { 0, 0, "115200 8 none 1" },
        { 0, 1, "57600 8 none 1" },
        { 1, 0, "9600 8 none 1" },
        { 2, 0, "38400 8 none 1" },
        { 3, 0, "19200 8 none 1" },
        { 3, 1, "4800 8 none 1" },
    }
    for _, e in ipairs(expected) do
        local f = format(e[1], e[2])
        expect(f == e[3], string.format("format of chip %d UART %d: %s", e[1], e[2], f))
    end
end)

test("Internal loopback of all UARTs (SPI, I2C, MAX3107/3108/3109)", function()
    for i, path in ipairs({ M0, M1, M2, M3, M4, M5 }) do
        local data = pattern(100, i)
        expect(transfer(path, path, data) == data, path .. ": data mismatch")
    end
end)

-- data larger than FIFO: 38400 baud, the driver without IRQ reads the chip periodically (FIFO
-- filled in 33 ms), at 115200 baud (11 ms) delays of threads of the simulator (host load) cause
-- overrun of RX FIFO (lost data)
test("Data larger than FIFO (buffers of driver)", function()
    local data = pattern(1000, 7)
    expect(configure(M0, CHIP0 .. ",baud=38400,loopback=1,rx_buffer=1024,tx_buffer=256"), "configuration failed")
    expect(transfer(M0, M0, data) == data, "data mismatch")
end)

test("Data received when device is not read", function()
    run("modprobe -c " .. S4 .. " baud=38400,parity=off")
    expect(configure(M0, CHIP0 .. ",baud=38400,rx_buffer=512"), "configuration failed")
    local data = pattern(400, 8) -- more than FIFO of chip
    local s = open(S4)
    assert(s:write(data))
    s:close()
    dnx.sys.msleep(300)
    local f = open(M0)
    local rd = f:read(#data)
    f:close()
    expect(rd == data, "data mismatch")
end)

test("UART 1 is configured again after configuration of UART 0", function()
    expect(format(0, 1) == "57600 8 none 1", "format of UART 1: " .. format(0, 1))
    local data = pattern(50, 9)
    expect(transfer(M1, M1, data) == data, "UART 1: data mismatch")
end)

test("Serial lines to system UARTs: 19200 8E1 and 9600 8N2", function()
    run("modprobe -c " .. S4 .. " baud=19200,parity=even,stop_bits=1")
    run("modprobe -c " .. S5 .. " baud=9600,parity=off,stop_bits=2")
    expect(configure(M0, CHIP0 .. ",baud=19200,parity=even"), "configuration of UART 0 failed")
    expect(configure(M1, "path=/dev/spi-max3109,baud=9600,stop_bits=2"), "configuration of UART 1 failed")
    expect(format(0, 0) == "19200 8 even 1", "format of UART 0: " .. format(0, 0))
    expect(format(0, 1) == "9600 8 none 2", "format of UART 1: " .. format(0, 1))
    expect(transfer(M0, S4, "UART0 -> S4") == "UART0 -> S4", "UART 0 -> S4 mismatch")
    expect(transfer(S4, M0, "S4 -> UART0") == "S4 -> UART0", "S4 -> UART 0 mismatch")
    expect(transfer(M1, S5, "UART1 -> S5") == "UART1 -> S5", "UART 1 -> S5 mismatch")
    expect(transfer(S5, M1, "S5 -> UART1") == "S5 -> UART1", "S5 -> UART 1 mismatch")
end)

test("Baud rates of generator (1x, 2x, 4x mode) and format", function()
    local list = {
        { "baud=300", "300 8 none 1" },
        { "baud=57600,stop_bits=2", "57600 8 none 2" },
        { "baud=460800,parity=odd", "460800 8 odd 1" },
        { "baud=921600,word_length=7", "921600 7 none 1" },
    }
    for _, c in ipairs(list) do
        expect(configure(M1, "path=/dev/spi-max3109," .. c[1]), c[1] .. ": configuration failed")
        local f = format(0, 1)
        expect(f == c[2], c[1] .. ": format " .. f)
    end
end)

test("Different baud rate gives unreadable data", function()
    run("modprobe -c " .. S5 .. " baud=19200,parity=off,stop_bits=1")
    expect(configure(M1, "path=/dev/spi-max3109,baud=9600"), "configuration failed")
    expect(transfer(M1, S5, "abc") == "\xFF\xFF\xFF", "data should be unreadable")
end)

test("Hardware flow control and RS485 registers", function()
    expect(configure(M1, "path=/dev/spi-max3109,baud=115200,hw_flow_ctrl=1,flow_halt=64,flow_resume=16,rs485=1"),
        "configuration failed")
    expect(reg(0, 1, 0x13) == 0x03, "FlowCtrl: " .. reg(0, 1, 0x13))
    expect(reg(0, 1, 0x0F) == 0x28, "FlowLvl: " .. reg(0, 1, 0x0F))
    expect(reg(0, 1, 0x09) == 0x10, "MODE1: " .. reg(0, 1, 0x09))
    -- configuration without these options resets registers (UART reset)
    expect(configure(M1, "path=/dev/spi-max3109,baud=115200"), "configuration failed")
    expect(reg(0, 1, 0x13) == 0x00 and reg(0, 1, 0x09) == 0x00, "registers of previous configuration")
end)

test("AutoCTS: transmission waits for CTS", function()
    run("modprobe -c " .. S5 .. " baud=115200,parity=off,stop_bits=1")
    expect(configure(M1, "path=/dev/spi-max3109,baud=115200,hw_flow_ctrl=1"), "configuration failed")
    control("max310x 0 1 cts 0")
    local w = open(M1)
    assert(w:write("cts"))
    w:close()
    dnx.sys.msleep(100)
    local level = reg(0, 1, 0x11)
    control("max310x 0 1 cts 1")
    expect(level == 3, "TX FIFO level with inactive CTS: " .. tostring(level))
    local r = open(S5)
    local rd = r:read(3)
    r:close()
    expect(rd == "cts", "data after CTS activation")
end)

test("Disabled transmitter and receiver", function()
    expect(configure(M1, "path=/dev/spi-max3109,baud=115200,tx_enable=0,rx_enable=0"), "configuration failed")
    local f = open(M1)
    expect(not f:write("x"), "write with disabled transmitter")
    expect(not f:read(1), "read with disabled receiver")
    f:close()
end)

test("Invalid configuration is rejected", function()
    local bad = {
        { M0, "clock=3686400,baud=115200" },
        { M0, "path=/dev/spi-max3109,chip=max3109,clock=0" },
        { M1, "path=/dev/spi-max3109,baud=2000000" },
        { M1, "path=/dev/spi-max3109,baud=0" },
        { M1, "path=/dev/spi-max3109,word_length=9" },
        { M1, "path=/dev/spi-max3109,hw_flow_ctrl=1,flow_halt=32,flow_resume=32" },
        { M1, "path=/dev/spi-max3109,parity=mark" },
        { M1, "path=/dev/spi-max3109,interface=i2c" },
        { M0, "path=/dev/spi-max3109,chip=max3107,clock=3686400" }, -- UART 1 exists
        { M0, "path=/dev/spi-max3109,chip=max3110" },
    }
    for _, b in ipairs(bad) do
        expect(not configure(b[1], b[2]), "accepted: " .. b[1] .. " " .. b[2])
    end
    expect(configure(M0, CHIP0 .. ",baud=115200,loopback=1"), "valid configuration after errors")
    expect(configure(M1, "path=/dev/spi-max3109,baud=115200,loopback=1"), "valid configuration after errors")
    expect(transfer(M1, M1, "ok") == "ok", "device works after errors")
end)

test("Detection of chip type", function()
    local ok = T.run("modprobe -i MAX310X 4 0 /dev/ttyM6 -c path=/dev/i2c-max3107-bad,interface=i2c,clock=3686400")
    expect(not ok, "chip with wrong revision ID accepted")
    expect(not configure(M2, "path=/dev/i2c-max3107,interface=i2c,chip=max3109,clock=1843200,crystal=1"),
        "MAX3107 accepted as MAX3109")
    expect(not configure(M4, "path=/dev/i2c-max3109-0,interface=i2c,chip=max3107,clock=3686400"),
        "MAX3109 accepted as MAX3107")
    expect(not T.run("modprobe -i MAX310X 1 1 /dev/ttyM7 -c path=/dev/i2c-max3107,interface=i2c"),
        "UART 1 of MAX3107 accepted")
end)

test("Release of UARTs", function()
    expect(not T.run("modprobe -r MAX310X 0 0"), "UART 0 released before UART 1")
    run("modprobe -r MAX310X 0 1")
    run("modprobe -r MAX310X 0 0")
    expect(not io.open(M0, "r+b"), "released device can be opened")
end)

T.summary()
