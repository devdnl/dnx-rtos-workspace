--[[----------------------------------------------------------------------------
  SPI test executed in dnx RTOS (lua program) with board boards/test.lua.
  Devices are created by modprobe (SPI driver, configuration string), the CS
  pin is controlled by the SPI driver (GPIO port A), data are transferred by
  SPI device files (write: transmit, read: receive, CS active during access).
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect = T.test, T.expect

local DELAY = 50 -- ms, time for board reaction

local function dev_write(path, data)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    local ok, err = f:write(data)
    f:close()
    return ok ~= nil, err
end

local function dev_read(path, n)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    local data, err = f:read(n)
    f:close()
    return data, err
end

local function hex(s)
    return (tostring(s):gsub(".", function(c) return string.format("%02X ", c:byte()) end))
end

local function rev(s)
    return (s:gsub(".", function(c)
        local v, r = c:byte(), 0
        for b = 0, 7 do if (v >> b) & 1 == 1 then r = r | (1 << (7 - b)) end end
        return string.char(r)
    end))
end

test("Create SPI devices (modprobe -c, CS inactive)", function()
    T.spi_devices()
    T.msleep(DELAY)
    local port = T.port_read()
    for _, pin in ipairs({4, 6, 7, 15, 16, 17}) do
        expect((port >> pin) & 1 == 1, "CS PA" .. pin .. " is not inactive (high)")
    end
end)

test("Write and read (CS selects device)", function()
    expect(dev_write("/dev/spi-a", "hello SPI"), "write error")
    local rd = dev_read("/dev/spi-a", 9)
    expect(rd == "hello SPI", "read: " .. hex(rd))
end)

test("Two devices on bus (separate CS)", function()
    expect(dev_write("/dev/spi-b", "device B"), "write error")
    expect(dev_read("/dev/spi-a", 9) == "hello SPI", "device A data changed")
    expect(dev_read("/dev/spi-b", 8) == "device B", "device B data")
end)

test("CS inactive after transfer", function()
    dev_read("/dev/spi-a", 4)
    T.msleep(DELAY)
    expect((T.port_read() >> 4) & 1 == 1, "CS PA4 is active")
end)

test("LSB first bit order", function()
    expect(dev_write("/dev/spi-b-lsb", "LSB"), "write error")
    local rd = dev_read("/dev/spi-b", 3)
    expect(rd == rev("LSB"), "MSB first read of LSB first data: " .. hex(rd))
    expect(dev_read("/dev/spi-b-lsb", 3) == "LSB", "LSB first read")
end)

test("Not supported SPI mode (no response)", function()
    local rd = dev_read("/dev/spi-a-mode1", 4)
    expect(rd == "\xFF\xFF\xFF\xFF", "data received in mode 1: " .. hex(rd))
end)

test("Bus without devices (MISO high)", function()
    expect(dev_write("/dev/spi-none", "xyz"), "write error")
    local rd = dev_read("/dev/spi-none", 3)
    expect(rd == "\xFF\xFF\xFF", "read: " .. hex(rd))
end)

test("74HC595 outputs (SPI2, outputs -> PA20..PA27)", function()
    for _, v in ipairs({0xA5, 0x3C, 0x00, 0xFF}) do
        expect(dev_write("/dev/spi-595", string.char(v)), "write error")
        T.msleep(DELAY)
        local out = (T.port_read() >> 20) & 0xFF
        expect(out == v, string.format("written 0x%02X, outputs 0x%02X", v, out))
    end
end)

T.summary()
