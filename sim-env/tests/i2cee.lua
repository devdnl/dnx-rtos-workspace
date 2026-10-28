--[[----------------------------------------------------------------------------
  I2CEE driver and eefs test executed in dnx RTOS (lua program) with board
  boards/test.lua (24C256 on I2C1, address 0x50). The full fstest is not
  executed on this device: eefs reads blocks very often and every block is an
  I2C transaction, the test would take more than an hour.
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect = T.test, T.expect

local I2C_DEV = "/dev/i2c-24c256"
local EE_DEV  = "/dev/24c256"
local MNT     = "/mnt/i2cee"
local SIZE    = 32768

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. out, 2) end
    return out
end

local function exists(path)
    local f = io.open(path, "rb")
    if f then f:close() end
    return f ~= nil
end

-- deterministic test data
local function pattern(n, seed)
    local t, x = {}, seed
    for i = 1, n do
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        t[i] = string.char((x >> 16) & 0xFF)
    end
    return table.concat(t)
end

local function write_file(path, data)
    local f = assert(io.open(path, "wb"))
    assert(f:write(data))
    f:close()
end

local function read_file(path)
    local f, err = io.open(path, "rb")
    if not f then return nil, err end
    local data = f:read("a")
    f:close()
    return data
end

local FILES = {
    {MNT .. "/small.txt", "Hello from I2C EEPROM!\n"},
    {MNT .. "/block.bin", pattern(128, 1)},
    {MNT .. "/multi.bin", pattern(3000, 2)},
    {MNT .. "/dir/nested.bin", pattern(777, 3)},
}

test("Create I2C and I2CEE devices (modprobe -c)", function()
    if not exists(I2C_DEV) then
        run("modprobe -i I2C 0 0 " .. I2C_DEV .. " -c address=0xA0,sub_addr_mode=2")
    end
    if not exists(EE_DEV) then
        run("modprobe -i I2CEE 0 0 " .. EE_DEV .. " -c i2c_path=" .. I2C_DEV
            .. ",memory_size=" .. SIZE .. ",page_size=64,page_prog_time_ms=5")
    end
end)

test("Unaligned write across pages and read", function()
    local data = pattern(1000, 4)
    local f = assert(io.open(EE_DEV, "r+b"))
    f:setvbuf("no")
    f:seek("set", 1001)
    assert(f:write(data))
    f:seek("set", 1001)
    local rd = f:read(#data)
    f:close()
    expect(rd == data, "data mismatch")
end)

test("Write at the end of memory", function()
    local f = assert(io.open(EE_DEV, "r+b"))
    f:setvbuf("no")
    f:seek("set", SIZE - 10)
    assert(f:write("0123456789"))
    f:seek("set", SIZE - 10)
    local rd = f:read(10)
    f:close()
    expect(rd == "0123456789", "data mismatch: " .. tostring(rd))
end)

test("Format eefs and mount", function()
    run("mkeefs " .. EE_DEV)
    run("mkdir " .. MNT)
    run("mount " .. EE_DEV .. " eefs " .. MNT)
end)

test("Write files", function()
    run("mkdir " .. MNT .. "/dir")
    for _, f in ipairs(FILES) do write_file(f[1], f[2]) end
end)

test("Read files", function()
    for _, f in ipairs(FILES) do
        expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch")
    end
end)

test("Remount and read files", function()
    run("umount " .. MNT)
    run("mount " .. EE_DEV .. " eefs " .. MNT)
    for _, f in ipairs(FILES) do
        expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch")
    end
end)

test("Remove files and free space", function()
    for i = #FILES, 1, -1 do expect(os.remove(FILES[i][1]), "remove " .. FILES[i][1]) end
    expect(os.remove(MNT .. "/dir"), "remove dir")
    -- space of removed files can be used again
    local big = pattern(20000, 5)
    write_file(MNT .. "/big.bin", big)
    expect(read_file(MNT .. "/big.bin") == big, "big file: data mismatch")
    expect(os.remove(MNT .. "/big.bin"), "remove big file")
end)

test("Unmount", function()
    run("umount " .. MNT)
end)

T.summary()
