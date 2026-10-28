--[[----------------------------------------------------------------------------
  I2C test executed in dnx RTOS (lua program) with board boards/test.lua.
  Devices are created by modprobe (I2C driver, configuration string), data
  are transferred by I2C device files (file position is sub-address).
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect = T.test, T.expect

local DELAY = 50     -- ms, time for board reaction
local WRITE_TIME = 6 -- ms, EEPROM write cycle (model: 5 ms)

-- I2C devices: path, major (bus), minor, configuration
local DEVICES = {
    {"/dev/i2c-io",   0, 10, "address=0x40"},                   -- PCF8574
    {"/dev/i2c-temp", 0, 11, "address=0x90,sub_addr_mode=1"},   -- LM75
    {"/dev/i2c-ee",   0, 12, "address=0xA2,sub_addr_mode=2"},   -- 24C32
    {"/dev/i2c-none", 0, 13, "address=0xAE,sub_addr_mode=2"},   -- not connected
    {"/dev/i2c2-ee",  1, 10, "address=0xA0,sub_addr_mode=1"},   -- 24C02 on I2C2
}

local function exists(path)
    local f = io.open(path, "rb")
    if f then f:close() end
    return f ~= nil
end

local function open(path)
    local f, err = io.open(path, "r+b")
    if not f then error(path .. ": " .. tostring(err), 3) end
    f:setvbuf("no")
    return f
end

local function dev_write(path, pos, data)
    local f = open(path)
    if pos then f:seek("set", pos) end
    local ok, err = f:write(data)
    f:close()
    return ok ~= nil, err
end

local function dev_read(path, pos, n)
    local f = open(path)
    if pos then f:seek("set", pos) end
    local data, err = f:read(n)
    f:close()
    return data, err
end

local function hex(s)
    return (tostring(s):gsub(".", function(c) return string.format("%02X ", c:byte()) end))
end

test("Create I2C devices (modprobe -c)", function()
    for _, d in ipairs(DEVICES) do
        if not exists(d[1]) then
            local cmd = string.format("modprobe -i I2C %d %d %s -c %s", d[2], d[3], d[1], d[4])
            local ok, out = T.run(cmd)
            expect(ok, cmd .. ": " .. out)
        end
    end
end)

test("Not connected device (address NACK)", function()
    local data, err = dev_read("/dev/i2c-none", 0, 1)
    expect(data == nil, "data read from not connected device")
    expect(tostring(err):find("No such device or address"), "unexpected error: " .. tostring(err))
end)

test("EEPROM write and read", function()
    local data = "dnx RTOS I2C test"
    local ok, err = dev_write("/dev/i2c-ee", 0x123, data)
    expect(ok, "write error: " .. tostring(err))
    T.msleep(WRITE_TIME)
    local rd = dev_read("/dev/i2c-ee", 0x123, #data)
    expect(rd == data, "read: " .. hex(rd))
end)

test("EEPROM page write roll-over", function()
    -- 32 B page: bytes after page end are written at page begin
    local ok = dev_write("/dev/i2c-ee", 0x3FC, "ABCDEFGH")
    expect(ok, "write error")
    T.msleep(WRITE_TIME)
    expect(dev_read("/dev/i2c-ee", 0x3FC, 4) == "ABCD", "page end")
    expect(dev_read("/dev/i2c-ee", 0x3E0, 4) == "EFGH", "page begin")
    expect(dev_read("/dev/i2c-ee", 0x400, 4) ~= "EFGH", "next page written")
end)

test("EEPROM sequential read across pages", function()
    local data = {}
    for i = 0, 99 do data[#data + 1] = string.char((i * 7) & 0xFF) end
    data = table.concat(data)
    for off = 0, #data - 1, 32 do
        expect(dev_write("/dev/i2c-ee", 0x800 + off, data:sub(off + 1, off + 32)), "write error")
        T.msleep(WRITE_TIME)
    end
    expect(dev_read("/dev/i2c-ee", 0x800, #data) == data, "data mismatch")
end)

test("EEPROM on second bus (I2C2, 1-byte sub-address)", function()
    expect(dev_write("/dev/i2c2-ee", 0x10, "I2C2"), "write error")
    T.msleep(WRITE_TIME)
    expect(dev_read("/dev/i2c2-ee", 0x10, 4) == "I2C2", "data mismatch")
end)

test("PCF8574 outputs and inputs (P0..P3 -> P4..P7)", function()
    for _, v in ipairs({0x05, 0x0A, 0x0F, 0x00}) do
        expect(dev_write("/dev/i2c-io", nil, string.char(0xF0 | v)), "write error")
        local rd = dev_read("/dev/i2c-io", nil, 1)
        local expected = (v << 4) | v
        expect(rd and rd:byte() == expected,
               string.format("written 0x%02X, read %s, expected 0x%02X", 0xF0 | v, hex(rd), expected))
    end
end)

test("PCF8574 interrupt output (INT -> PA13)", function()
    dev_read("/dev/i2c-io", nil, 1)
    T.msleep(DELAY)
    expect(T.pin_get(13) == true, "INT active without input change")
    expect(dev_write("/dev/i2c-io", nil, string.char(0xF3)), "write error")
    T.msleep(DELAY)
    expect(T.pin_get(13) == false, "INT not active after input change")
    dev_read("/dev/i2c-io", nil, 1)
    T.msleep(DELAY)
    expect(T.pin_get(13) == true, "INT not released by read")
end)

test("LM75 temperature register", function()
    local rd = dev_read("/dev/i2c-temp", 0, 2)
    expect(rd == "\x15\x80", "temperature 21.5 C expected, read: " .. hex(rd))
end)

test("LM75 limit registers write and read", function()
    expect(dev_write("/dev/i2c-temp", 2, "\x28\x00"), "THYST write error")
    expect(dev_write("/dev/i2c-temp", 3, "\x32\x80"), "TOS write error")
    expect(dev_read("/dev/i2c-temp", 2, 2) == "\x28\x00", "THYST read")
    expect(dev_read("/dev/i2c-temp", 3, 2) == "\x32\x80", "TOS read")
end)

test("LM75 OS output (PA3 heats sensor, OS -> PA14)", function()
    T.pin_set(3, false)
    T.msleep(DELAY)
    expect(T.pin_get(14) == true, "OS active at 21.5 C")
    T.pin_set(3, true)
    T.msleep(DELAY)
    expect(dev_read("/dev/i2c-temp", 0, 2) == "\x5A\x00", "temperature 90 C expected")
    expect(T.pin_get(14) == false, "OS not active at 90 C")
    T.pin_set(3, false)
    T.msleep(DELAY)
    expect(T.pin_get(14) == true, "OS not released at 21.5 C")
end)

T.summary()
