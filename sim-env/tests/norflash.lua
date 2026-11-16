--[[----------------------------------------------------------------------------
  NORFLASH driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: NOR flash memories on SPI:0, W25Q16 (CS PB3, SFDP),
  MX25L8005 (CS PB4, no SFDP, deep power-down at start), W25Q256 (CS PB5,
  4-byte address), SST26VF016 (CS PB6, protected at power-up), no memory on
  CS PB7. Sector erases and page programs are counted by the board (control
  device I2C:0 0x30).
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local F0, F1, F2, F3 = "/dev/flash0", "/dev/flash1", "/dev/flash2", "/dev/flash3"
local MNT = "/mnt/norflash"
local SECTOR = 4096
local MiB = 1024 * 1024

local ctl

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. tostring(out), 2)
    end
    return out
end

local function open(path)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    return f
end

local function control(cmd)
    ctl:seek("set", 0)
    assert(ctl:write(cmd))
    ctl:seek("set", 0)
    return assert(ctl:read(128)):match("^[^%z]*")
end

-- sector erases and page programs of chip (CS PB<n>)
local function stats(n)
    local e, p = control("nor " .. n .. " stats"):match("^(%d+) (%d+)")
    return tonumber(e), tonumber(p)
end

local function pattern(n, seed)
    local t, x = {}, seed
    for i = 1, n do
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        t[i] = string.char((x >> 16) & 0xFF)
    end
    return table.concat(t)
end

local function dev_write(path, pos, data)
    local f = open(path)
    f:seek("set", pos)
    local ok, err = f:write(data)
    f:close()
    return ok, err
end

local function dev_read(path, pos, n)
    local f = open(path)
    f:seek("set", pos)
    local data = f:read(n)
    f:close()
    return data
end

local function write_file(path, data)
    local f = assert(io.open(path, "wb"))
    assert(f:write(data))
    f:close()
end

local function read_file(path)
    local f = io.open(path, "rb")
    if not f then
        return nil
    end
    local data = f:read("a")
    f:close()
    return data
end

-- tests ------------------------------------------------------------------------------
test("Create devices (modprobe -c), identification", function()
    T.spi_devices()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    T.modprobe("NORFLASH", 0, 0, F0, "path=/dev/spi-nor-a")
    T.modprobe("NORFLASH", 1, 0, F1, "path=/dev/spi-nor-b")
    T.modprobe("NORFLASH", 2, 0, F2, "path=/dev/spi-nor-c")
    T.modprobe("NORFLASH", 3, 0, F3, "path=/dev/spi-nor-d")
end)

test("Detected size (SFDP, JEDEC ID, deep power-down)", function()
    local SIZES = { [F0] = 2 * MiB, [F1] = 1 * MiB, [F2] = 32 * MiB, [F3] = 2 * MiB }
    for path, size in pairs(SIZES) do
        local a = assert(lfs.attributes(path))
        expect(a.size == size, path .. ": size " .. tostring(a.size))
    end
end)

test("No memory: configuration fails, device without memory", function()
    T.modprobe("NORFLASH", 4, 0, "/dev/flash4")
    local ok = T.run("modprobe -c /dev/flash4 path=/dev/spi-nor-none")
    expect(not ok, "configuration without memory succeeded")
    ok = T.run("modprobe -c /dev/flash4 path=/dev/spi-nor-a,interface=usb")
    expect(not ok, "invalid interface accepted")
    expect(io.open("/dev/flash4", "rb"):read(16) == nil, "read of not configured device")
end)

test("Erased memory reads 0xFF", function()
    expect(dev_read(F0, 0, SECTOR) == string.rep("\xFF", SECTOR), "not erased")
end)

test("Write erased sector: program without erase", function()
    local e0, p0 = stats(3)
    local data = pattern(SECTOR, 1)
    assert(dev_write(F0, 2 * SECTOR, data))
    local e1, p1 = stats(3)
    expect(dev_read(F0, 2 * SECTOR, SECTOR) == data, "data mismatch")
    expect(e1 == e0, "sector erased: " .. (e1 - e0))
    expect(p1 - p0 == 16, "page programs: " .. (p1 - p0))
end)

test("Write the same data: nothing programmed", function()
    local e0, p0 = stats(3)
    assert(dev_write(F0, 2 * SECTOR, pattern(SECTOR, 1)))
    local e1, p1 = stats(3)
    expect(e1 == e0 and p1 == p0, string.format("erases %d, programs %d", e1 - e0, p1 - p0))
end)

test("Overwrite: sector erase, pages with 0xFF are not programmed", function()
    local e0, p0 = stats(3)
    local data = pattern(SECTOR, 2):sub(1, 1000) .. string.rep("\xFF", SECTOR - 1000)
    assert(dev_write(F0, 2 * SECTOR, data))
    local e1, p1 = stats(3)
    expect(dev_read(F0, 2 * SECTOR, SECTOR) == data, "data mismatch")
    expect(e1 - e0 == 1, "sector erases: " .. (e1 - e0))
    expect(p1 - p0 == 4, "page programs: " .. (p1 - p0))
end)

test("Clear bits only: programmed without erase", function()
    local old = dev_read(F0, 2 * SECTOR, SECTOR)
    local t = {}
    for i = 1, SECTOR do
        t[i] = string.char(old:byte(i) & 0x5A)
    end
    local data = table.concat(t)
    local e0 = stats(3)
    assert(dev_write(F0, 2 * SECTOR, data))
    expect(stats(3) == e0, "sector erased")
    expect(dev_read(F0, 2 * SECTOR, SECTOR) == data, "data mismatch")
end)

test("Unaligned write across sectors (read-modify-write)", function()
    local base = 8 * SECTOR
    local before = pattern(3 * SECTOR, 3)
    assert(dev_write(F0, base, before))
    local data = pattern(5000, 4)
    assert(dev_write(F0, base + 1234, data))
    local exp = before:sub(1, 1234) .. data .. before:sub(1234 + #data + 1)
    expect(dev_read(F0, base, 3 * SECTOR) == exp, "data mismatch")
end)

test("Write and read at the end of memory, beyond the end", function()
    local data = pattern(SECTOR, 5)
    assert(dev_write(F1, MiB - SECTOR, data))
    expect(dev_read(F1, MiB - SECTOR, SECTOR) == data, "data mismatch")
    expect(dev_read(F1, MiB, 16) == nil, "read beyond the end")
end)

test("4-byte address (32 MiB memory)", function()
    local data = pattern(SECTOR, 6)
    assert(dev_write(F2, 20 * MiB, data))
    expect(dev_read(F2, 20 * MiB, SECTOR) == data, "data mismatch")
    expect(dev_read(F2, 4 * MiB, SECTOR) == string.rep("\xFF", SECTOR), "address wrapped at 16 MiB")
end)

test("Protected memory: write error, unprotect", function()
    local ok = dev_write(F3, 0, pattern(SECTOR, 7))
    expect(not ok, "write of protected memory succeeded")
    run("modprobe -c " .. F3 .. " path=/dev/spi-nor-d,unprotect=1")
    local data = pattern(SECTOR, 7)
    assert(dev_write(F3, 0, data))
    expect(dev_read(F3, 0, SECTOR) == data, "data mismatch")
end)

test("Configured size", function()
    run("modprobe -c " .. F1 .. " path=/dev/spi-nor-b,size=524288")
    expect(lfs.attributes(F1).size == 524288, "size not changed")
    expect(not T.run("modprobe -c " .. F1 .. " path=/dev/spi-nor-b,size=1000"), "invalid size accepted")
    run("modprobe -c " .. F1 .. " path=/dev/spi-nor-b")
    expect(lfs.attributes(F1).size == MiB, "size not detected")
end)

local FILES = {
    { MNT .. "/small.txt", "Hello from NOR flash!\n" },
    { MNT .. "/big.bin", pattern(20000, 8) },
    { MNT .. "/dir/nested.bin", pattern(777, 9) },
}

test("flatfs: format, write files, remount, read", function()
    run("mkflatfs " .. F0 .. " 64")
    run("mkdir " .. MNT)
    run("mount " .. F0 .. " flatfs " .. MNT)
    run("mkdir " .. MNT .. "/dir")
    for _, f in ipairs(FILES) do
        write_file(f[1], f[2])
    end
    run("umount " .. MNT)
    run("mount " .. F0 .. " flatfs " .. MNT)
    for _, f in ipairs(FILES) do
        expect(read_file(f[1]) == f[2], f[1] .. ": data mismatch")
    end
    for i = #FILES, 1, -1 do
        expect(os.remove(FILES[i][1]), "remove " .. FILES[i][1])
    end
    expect(os.remove(MNT .. "/dir"), "remove dir")
    run("umount " .. MNT)
end)

T.summary()
