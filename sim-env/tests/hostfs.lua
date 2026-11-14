--[[----------------------------------------------------------------------------
  hostfs test executed in dnx RTOS (lua program): host directories selected by
  the simulator options --hostfs host=<dir> (mounted by siminit in /host) and
  --hostfs aux=<dir> (created by tools/simtest.py, contains file aux.txt with
  text "aux" and file sub/sub.txt with text "sub").
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local MNT    = "/mnt/aux"
local MNTSUB = "/mnt/auxsub"

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. out, 2) end
    return out
end

local function read_file(path)
    local f, err = io.open(path, "rb")
    if not f then error(err, 2) end
    local data = f:read("a")
    f:close()
    return data
end

test("Mount directory selected by name", function()
    lfs.mkdir(MNT)
    run("mount host://aux hostfs " .. MNT)
    expect(read_file(MNT .. "/aux.txt") == "aux", "aux.txt content")
    expect(read_file(MNT .. "/sub/sub.txt") == "sub", "sub/sub.txt content")
    expect(not lfs.attributes("/host/aux.txt"), "aux.txt visible in /host")
end)

test("Write file in second directory", function()
    local f = assert(io.open(MNT .. "/new.txt", "wb"))
    assert(f:write("new"))
    f:close()
    expect(read_file(MNT .. "/new.txt") == "new", "new.txt content")
    expect(os.remove(MNT .. "/new.txt"), "remove new.txt")
end)

test("Mount subdirectory read only", function()
    lfs.mkdir(MNTSUB)
    run("mount host://aux/sub hostfs " .. MNTSUB .. " ro")
    expect(read_file(MNTSUB .. "/sub.txt") == "sub", "sub.txt content")
    expect(not io.open(MNTSUB .. "/new.txt", "wb"), "file created in read only file system")
    run("umount " .. MNTSUB)
end)

test("Invalid sources are rejected", function()
    expect(not T.run("mount host://unknown hostfs " .. MNTSUB), "unknown name mounted")
    expect(not T.run("mount host://aux/.. hostfs " .. MNTSUB), "path outside directory mounted")
    expect(not T.run("mount host://aux/aux.txt hostfs " .. MNTSUB), "file mounted")
    expect(not T.run("mount aux hostfs " .. MNTSUB), "source without protocol mounted")
    expect(not T.run("mount host:// hostfs " .. MNTSUB), "source without name mounted")
    lfs.rmdir(MNTSUB)
end)

test("Unmount", function()
    run("umount " .. MNT)
    expect(not lfs.attributes(MNT .. "/aux.txt"), "aux.txt visible after umount")
    lfs.rmdir(MNT)
end)

T.summary()
