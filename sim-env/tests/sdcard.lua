--[[----------------------------------------------------------------------------
  SDSPI driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: SDHC card 16 MiB (CS PA15) and SD v1 card 2 MiB (CS PA17)
  on SPI1, ext2 file system on SDHC card (mkext). The card is left formatted
  (tools/simtest.py executes fstest on it).
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local SD     = "/dev/sdcard"
local SD_V1  = "/dev/sdcard-v1"
local MNT    = "/mnt/sdcard"
local SECTOR = 512

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

local function dev_rw(path, sector, data)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    f:seek("set", sector * SECTOR)
    local ok, err = f:write(data)
    if not ok then
        f:close()
        error("write error: " .. tostring(err), 2)
    end
    f:seek("set", sector * SECTOR)
    local rd, rerr = f:read(#data)
    f:close()
    if not rd then error("read error: " .. tostring(rerr), 2) end
    return rd
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

local FILES = {
    {MNT .. "/small.txt", "Hello from SD card!\n"},
    {MNT .. "/big.bin", pattern(20000, 7)},
    {MNT .. "/dir/nested.bin", pattern(1500, 8)},
}

test("Create SPI and SDSPI devices (modprobe -c)", function()
    T.spi_devices()
    T.modprobe("SDSPI", 0, 0, SD, "spi_path=/dev/spi-sd,timeout=1000")
    T.modprobe("SDSPI", 1, 0, SD_V1, "spi_path=/dev/spi-sdv1,timeout=1000")
end)

test("SDHC card initialization (storinit) and size", function()
    run("storinit " .. SD)
    local size = lfs.attributes(SD, "size")
    expect(size == 16 * 1024 * 1024, "card size: " .. tostring(size))
end)

test("SDHC single sector write and read", function()
    local data = pattern(SECTOR, 1)
    expect(dev_rw(SD, 100, data) == data, "data mismatch")
end)

test("SDHC multiple sectors write and read", function()
    local data = pattern(8 * SECTOR, 2)
    expect(dev_rw(SD, 1000, data) == data, "data mismatch")
    expect(dev_rw(SD, 32767, data:sub(1, SECTOR)) == data:sub(1, SECTOR), "last sector")
end)

test("SD v1 card initialization, size and sectors", function()
    run("storinit " .. SD_V1)
    local size = lfs.attributes(SD_V1, "size")
    expect(size == 2 * 1024 * 1024, "card size: " .. tostring(size))
    local data = pattern(3 * SECTOR, 3)
    expect(dev_rw(SD_V1, 10, data) == data, "data mismatch")
end)

test("Format ext2 (mkext) and mount", function()
    run("mkext -i " .. SD)
    run("mkdir " .. MNT)
    run("mount " .. SD .. " ext4fs " .. MNT)
end)

test("Write and read files", function()
    run("mkdir " .. MNT .. "/dir")
    for _, f in ipairs(FILES) do write_file(f[1], f[2]) end
    for _, f in ipairs(FILES) do expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch") end
end)

test("Remount and read files", function()
    run("umount " .. MNT)
    run("mount " .. SD .. " ext4fs " .. MNT)
    for _, f in ipairs(FILES) do expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch") end
end)

test("Unmount", function()
    run("umount " .. MNT)
end)

T.summary()
