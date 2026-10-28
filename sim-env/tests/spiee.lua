--[[----------------------------------------------------------------------------
  SPIEE driver and eefs test executed in dnx RTOS (lua program) with board
  boards/test.lua (25LC256 on SPI1, CS PA7).
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect = T.test, T.expect

local EE_DEV = "/dev/25lc256"
local MNT    = "/mnt/spiee"
local SIZE   = 32768

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. out, 2) end
    return out
end

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
    local f = io.open(path, "rb")
    if not f then return nil end
    local data = f:read("a")
    f:close()
    return data
end

local function dev_rw(pos, data)
    local f = assert(io.open(EE_DEV, "r+b"))
    f:setvbuf("no")
    f:seek("set", pos)
    assert(f:write(data))
    f:seek("set", pos)
    local rd = f:read(#data)
    f:close()
    return rd
end

local FILES = {
    {MNT .. "/small.txt", "Hello from SPI EEPROM!\n"},
    {MNT .. "/multi.bin", pattern(3000, 2)},
    {MNT .. "/dir/nested.bin", pattern(777, 3)},
}

test("Create SPI and SPIEE devices (modprobe -c)", function()
    T.spi_devices()
    T.modprobe("SPIEE", 0, 0, EE_DEV, "spi_path=/dev/spi-25lc256,memory_size=" .. SIZE
               .. ",page_size=64,address_size=2")
end)

test("Unaligned write across pages and read", function()
    local data = pattern(1000, 4)
    expect(dev_rw(1001, data) == data, "data mismatch")
end)

test("Write at the end of memory", function()
    expect(dev_rw(SIZE - 10, "0123456789") == "0123456789", "data mismatch")
end)

test("Format eefs and mount", function()
    run("mkeefs " .. EE_DEV)
    run("mkdir " .. MNT)
    run("mount " .. EE_DEV .. " eefs " .. MNT)
end)

test("Write and read files", function()
    run("mkdir " .. MNT .. "/dir")
    for _, f in ipairs(FILES) do write_file(f[1], f[2]) end
    for _, f in ipairs(FILES) do expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch") end
end)

test("Remount and read files", function()
    run("umount " .. MNT)
    run("mount " .. EE_DEV .. " eefs " .. MNT)
    for _, f in ipairs(FILES) do expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch") end
end)

test("Remove files and unmount", function()
    for i = #FILES, 1, -1 do expect(os.remove(FILES[i][1]), "remove " .. FILES[i][1]) end
    expect(os.remove(MNT .. "/dir"), "remove dir")
    run("umount " .. MNT)
end)

T.summary()
