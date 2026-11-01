--[[----------------------------------------------------------------------------
  Block layer test executed in dnx RTOS (lua program): host disk 4 (simulator
  option --disk 4=<file>) with emulated hardware constraints (HOSTDISK driver):
  block size 512 B and data buffer alignment 32 B. The driver rejects requests
  that do not meet constraints, so all accesses below pass only if the kernel
  splits unaligned requests (read-modify-write) and uses bounce buffers.
  The disk is left formatted by mkext (tools/simtest.py executes fstest on it).
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local DEV    = "/dev/blk"
local MNT    = "/mnt/blk"
local SECTOR = 512
local BASE   = 100 * SECTOR -- tested area
local AREA   = 16 * SECTOR  -- tested area size

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

local function dev_write(f, pos, data)
    f:seek("set", pos)
    local ok, err = f:write(data)
    if not ok then error("write error at " .. pos .. ": " .. tostring(err), 2) end
end

local function dev_read(f, pos, len)
    f:seek("set", pos)
    local data, err = f:read(len)
    if not data then error("read error at " .. pos .. ": " .. tostring(err), 2) end
    return data
end

-- returns iostat counters: requests, bytes, driver calls, unaligned, bounce, errors
local function iostat(dir)
    local out = run("iostat " .. DEV)
    local v = {out:match(dir .. "%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)")}
    if #v == 0 then error("iostat output: " .. out, 2) end
    for i = 1, #v do v[i] = tonumber(v[i]) end
    return table.unpack(v)
end

local model -- expected content of tested area

test("Create block device with emulated constraints", function()
    T.modprobe("HOSTDISK", 4, 0, DEV, "block_size=512,buffer_align=32")
    expect(lfs.attributes(DEV, "mode") == "block device", "not a block device")
    run("iostat -e " .. DEV)
end)

test("Aligned write and read", function()
    model = pattern(AREA, 1)
    local f = assert(io.open(DEV, "r+b"))
    f:setvbuf("no")
    dev_write(f, BASE, model)
    local rd = dev_read(f, BASE, AREA)
    f:close()
    expect(rd == model, "data mismatch")
end)

test("Unaligned writes (read-modify-write)", function()
    local f = assert(io.open(DEV, "r+b"))
    f:setvbuf("no")

    -- head, middle and tail parts; the same source buffer is written from two
    -- different offsets, so its middle part can not be aligned in both cases
    local data = pattern(1300, 2)
    for _, off in ipairs({100, 2600}) do
        dev_write(f, BASE + off, data)
        model = model:sub(1, off) .. data .. model:sub(off + #data + 1)
    end

    -- small writes inside one sector and across sector boundary
    for i, w in ipairs({{3, 1}, {511, 2}, {1023, 3}, {5000, 700}, {7000, 1}}) do
        local off, len = w[1], w[2]
        local d = pattern(len, 10 + i)
        dev_write(f, BASE + off, d)
        model = model:sub(1, off) .. d .. model:sub(off + len + 1)
    end

    local rd = dev_read(f, BASE, AREA)
    f:close()
    expect(rd == model, "data mismatch")
end)

test("Unaligned reads", function()
    local f = assert(io.open(DEV, "rb"))
    f:setvbuf("no")
    for _, r in ipairs({{0, 1}, {1, 511}, {100, 1300}, {511, 2}, {700, 3000}, {AREA - 5, 5}}) do
        local off, len = r[1], r[2]
        local rd = dev_read(f, BASE + off, len)
        expect(rd == model:sub(off + 1, off + len), "data mismatch at " .. off .. ", size " .. len)
    end
    f:close()
end)

test("Statistics: unaligned requests, bounce buffers and no errors", function()
    local _, _, rdrv, runal, rbounce, rerr = iostat("read")
    local _, _, wdrv, wunal, wbounce, werr = iostat("write")
    expect(rerr == 0 and werr == 0, "errors: " .. rerr .. "/" .. werr)
    expect(runal > 0 and wunal > 0, "unaligned requests: " .. runal .. "/" .. wunal)
    expect(wbounce > 0, "write requests through bounce buffer: " .. wbounce)
    expect(rdrv > 0 and wdrv > 0, "driver calls: " .. rdrv .. "/" .. wdrv)
    run("iostat -d " .. DEV)
end)

test("Format ext2 (mkext) and mount", function()
    run("mkext -i " .. DEV)
    run("mkdir " .. MNT)
    run("mount " .. DEV .. " ext4fs " .. MNT)
    local data = pattern(20000, 3)
    local f = assert(io.open(MNT .. "/file.bin", "wb"))
    assert(f:write(data))
    f:close()
    run("umount " .. MNT)
    run("mount " .. DEV .. " ext4fs " .. MNT)
    f = assert(io.open(MNT .. "/file.bin", "rb"))
    local rd = f:read("a")
    f:close()
    expect(rd == data, "file data mismatch")
    run("umount " .. MNT)
end)

T.summary()
