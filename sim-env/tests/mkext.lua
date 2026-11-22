--[[----------------------------------------------------------------------------
  mkext test executed in dnx RTOS (lua program): host disk 6 (simulator option
  --disk 6=<file>) divided into areas (PART driver, manual geometry) of sizes
  not aligned to block group size, each formatted by mkext with different
  options (ext2/3/4, journal, block size), mounted by ext4fs and modified.
  Arguments: <offset_sectors>:<sectors>:<mkext options separated by commas>.
  Areas are checked by e2fsck after test (tools/simtest.py), the last area
  stays mounted for fstest (/mnt/mkext).
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect = T.test, T.expect

local RAW = "/dev/hostdisk6"
local MNT = "/mnt/mkext"

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. out, 2)
    end
    return out
end

local function write_file(path, data)
    local f = assert(io.open(path, "wb"))
    assert(f:write(data))
    f:close()
end

local function read_file(path)
    local f = assert(io.open(path, "rb"))
    local data = f:read("a")
    f:close()
    return data
end

T.modprobe("HOSTDISK", 6, 0, RAW)
os.execute("mkdir " .. MNT)

local AREAS = { ... }

for n, area in ipairs(AREAS) do
    local offset, size, opts = area:match("^(%d+):(%d+):(.*)$")
    opts = opts:gsub(",", " ")
    local dev = "/dev/mkext" .. n

    test(opts .. " (" .. size .. " sectors)", function()
        T.modprobe("PART", 6, n, dev,
            "path=" .. RAW .. ",offset_blocks=" .. offset .. ",total_blocks=" .. size)
        T.run("umount " .. MNT) -- previous area left mounted on failure
        local out = run("mkext " .. opts .. " -l area" .. n .. " -i " .. dev)
        expect(out:find("Created ext"), out)

        run("mount " .. dev .. " ext4fs " .. MNT)
        run("mkdir " .. MNT .. "/dir")
        local data = string.rep("mkext" .. n, 4000)
        write_file(MNT .. "/dir/file.bin", data)
        for i = 1, 40 do
            write_file(MNT .. "/dir/f" .. i, tostring(i))
        end
        for i = 1, 40, 2 do
            os.remove(MNT .. "/dir/f" .. i)
        end
        run("umount " .. MNT)

        run("mount " .. dev .. " ext4fs " .. MNT)
        expect(read_file(MNT .. "/dir/file.bin") == data, "data after remount")
        expect(read_file(MNT .. "/dir/f2") == "2", "small file")
        if n < #AREAS then
            run("umount " .. MNT)
        end
    end)
end

T.summary()
