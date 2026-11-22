--[[----------------------------------------------------------------------------
  Partition test executed in dnx RTOS (lua program): partition nodes created
  by the PART driver from GPT (disk /dev/sdb: /dev/sdb1..4 and named nodes
  /dev/sdb-<name>), fdisk program on empty disk (simulator --disk 5, PART disk
  instance /dev/disk5 with nodes /dev/disk5p<n> and /dev/disk5-<name>): new GPT,
  partitions, names, types, delete, refused write of mounted disk, damaged
  primary GPT (backup GPT used by driver, repaired by fdisk), damaged
  protective MBR (GPT used by driver, repaired by fdisk), MBR and release of
  disk instance with partition nodes.
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local RAW = "/dev/hostdisk5"
local DISK = "/dev/disk5"
local MNT = "/mnt/parttest"

-- command as root
local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. out, 2)
    end
    return out
end

local function size(path)
    return lfs.attributes(path, "size")
end

local function exists(path)
    return lfs.attributes(path) ~= nil
end

local function read(path, offset, len)
    local f = assert(io.open(path, "rb"))
    f:seek("set", offset)
    local data = f:read(len)
    f:close()
    return data
end

test("GPT disk: partition and name nodes", function()
    for n, name in ipairs({ "flatfs", "eefs", "ext2", "ext3" }) do
        local part, named = "/dev/sdb" .. n, "/dev/sdb-" .. name
        expect(exists(part) and exists(named), "nodes of partition " .. n)
        expect(size(part) == size(named) and size(part) > 0, "size of " .. named)
    end
    expect(read("/dev/sdb-ext3", 1024, 512) == read("/dev/sdb4", 1024, 512), "data of named partition")
    expect(not exists("/dev/sdb5"), "partition 5 not exist")
end)

test("fdisk: disk without partition table", function()
    T.modprobe("HOSTDISK", 5, 0, RAW)
    T.modprobe("PART", 5, 0, DISK, "path=" .. RAW .. ",nodes=" .. DISK)
    expect(size(DISK) == 8 * 1024 * 1024, "disk size")
    local out = run("fdisk " .. DISK)
    expect(out:find("no partition table"), out)
    local ok
    ok, out = T.run("fdisk -n 0:: " .. DISK)
    expect(not ok and out:find("use %-g"), out)
end)

test("fdisk: new GPT", function()
    local out = run("fdisk -g -n 0::+2M -t 1:fat -c 1:boot -n 0:: -c 2:data " .. DISK)
    expect(out:find("GPT") and out:find("fat") and out:find("boot") and out:find("data"), out)
    expect(size(DISK .. "p1") == 2 * 1024 * 1024 and exists(DISK .. "-boot"), "partition 1")
    expect(size(DISK .. "-data") == size(DISK .. "p2"), "partition 2")
    -- 1 MiB alignment, backup GPT (33 sectors) at the end of disk
    expect(size(DISK .. "p2") == (8 * 1024 * 1024 - 3 * 1024 * 1024 - 33 * 512), "size of partition 2")
    local ok
    ok, out = T.run("fdisk -g " .. DISK)
    expect(not ok and out:find("use %-f"), "existing table: " .. out)
end)

test("fdisk: mounted partition", function()
    run("mkflatfs " .. DISK .. "-data 64")
    lfs.mkdir(MNT)
    run("mount " .. DISK .. "-data flatfs " .. MNT)
    local f = assert(io.open(MNT .. "/file", "wb"))
    f:write("partition data")
    f:close()
    local ok, out = T.run("fdisk -x 1 " .. DISK)
    expect(not ok and out:find("is mounted"), out)
    run("umount " .. MNT)
end)

test("fdisk: delete and rename", function()
    run("fdisk -x 1 -c 2:store " .. DISK)
    expect(not exists(DISK .. "p1") and not exists(DISK .. "-boot"), "partition 1 deleted")
    expect(exists(DISK .. "p2") and exists(DISK .. "-store") and not exists(DISK .. "-data"), "renamed")
    run("mount " .. DISK .. "-store flatfs " .. MNT)
    local f = assert(io.open(MNT .. "/file", "rb"))
    expect(f:read("a") == "partition data", "data after rename")
    f:close()
    run("umount " .. MNT)
end)

test("damaged primary GPT", function()
    local f = assert(io.open(RAW, "r+b"))
    f:seek("set", 512)
    f:write(string.rep("\0", 512))
    f:close()

    -- driver: backup GPT
    run("modprobe -r PART 5 0")
    expect(not exists(DISK .. "p2") and not exists(DISK .. "-store"), "nodes released with disk")
    os.remove(DISK)
    T.modprobe("PART", 5, 0, DISK, "path=" .. RAW .. ",nodes=" .. DISK)
    expect(exists(DISK .. "p2") and exists(DISK .. "-store"), "partitions from backup GPT")

    -- fdisk: repair
    local out = run("fdisk " .. DISK)
    expect(out:find("backup GPT used"), out)
    run("fdisk -r " .. DISK)
    expect(read(RAW, 512, 8) == "EFI PART", "primary GPT repaired")
    out = run("fdisk " .. DISK)
    expect(not out:find("damaged") and out:find("store"), out)
end)

test("damaged protective MBR", function()
    local f = assert(io.open(RAW, "r+b"))
    f:seek("set", 0)
    f:write(string.rep("\0", 512))
    f:close()

    -- driver: GPT without protective MBR
    run("modprobe -r PART 5 0")
    os.remove(DISK)
    T.modprobe("PART", 5, 0, DISK, "path=" .. RAW .. ",nodes=" .. DISK)
    expect(exists(DISK .. "p2") and exists(DISK .. "-store"), "partitions of GPT")

    -- fdisk: repair
    local out = run("fdisk " .. DISK)
    expect(out:find("Protective MBR damaged") and out:find("store"), out)
    run("fdisk -r " .. DISK)
    expect(read(RAW, 446 + 4, 1) == "\xee" and read(RAW, 510, 2) == "\x55\xaa", "protective MBR repaired")
    out = run("fdisk " .. DISK)
    expect(not out:find("damaged") and out:find("GPT"), out)
end)

test("fdisk: MBR", function()
    local out = run("fdisk -m -f -n 1::+1M -t 1:dnx -n 2:: -t 2:0c " .. DISK)
    expect(out:find("MBR") and out:find("dnx") and out:find("fat"), out)
    expect(size(DISK .. "p1") == 1024 * 1024 and size(DISK .. "p2") > 0, "partitions")
    expect(not exists(DISK .. "-store"), "name nodes removed")
    expect(read(RAW, 446 + 4, 1) == "\xda", "MBR type")
end)

test("release of disk", function()
    run("modprobe -r PART 5 0")
    expect(not exists(DISK .. "p1") and not exists(DISK .. "p2"), "partition nodes released")
    os.remove(DISK)
    run("modprobe -r HOSTDISK 5 0")
    os.remove(RAW)
    os.remove(MNT)
end)

T.summary()
