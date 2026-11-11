--[[----------------------------------------------------------------------------
  Test of driver configuration strings (IOCTL_DEVICE__CONFIGURE_STR) executed
  in dnx RTOS (lua program). Devices are created and configured by modprobe:
    modprobe -i <module> <major> <minor> <path> -c <config>
    modprobe -c <path> <config>
  UART configuration (/dev/ttyS1) is checked by tools/simtest.py on the host
  side (UART:1 is connected to the host pseudo terminal). TTY is not tested: all
  terminals share one input/output, reconfiguration moves the system console.
------------------------------------------------------------------------------]]

local T = require("testlib")
local lfs = require("lfs")
local test, expect = T.test, T.expect

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. out, 2) end
    return out
end

local function fails(cmd)
    local ok = T.run(cmd)
    return not ok
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

local function pattern(n, seed)
    local t, x = {}, seed
    for i = 1, n do
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        t[i] = string.char((x >> 16) & 0xFF)
    end
    return table.concat(t)
end

test("PART on file (path, block_size, offset_blocks, total_blocks)", function()
    local data = pattern(8192, 1)
    write_file("/tmp/part.bin", data)
    T.modprobe("PART", 0, 10, "/dev/part-test",
               "path=/tmp/part.bin,block_size=512,offset_blocks=4,total_blocks=8")
    expect(lfs.attributes("/dev/part-test", "size") == 4096, "partition size")
    expect(read_file("/dev/part-test") == data:sub(2049, 6144), "partition data")
end)

test("PART write inside partition", function()
    local f = assert(io.open("/dev/part-test", "r+b"))
    f:setvbuf("no")
    f:seek("set", 512)
    assert(f:write(string.rep("P", 512)))
    f:close()
    local data = read_file("/tmp/part.bin")
    expect(data:sub(2048 + 513, 2048 + 1024) == string.rep("P", 512), "data not written at partition offset")
end)

test("FBRIDGE (read_path, write_path)", function()
    write_file("/tmp/fb_in", "input data")
    write_file("/tmp/fb_out", "")
    T.modprobe("FBRIDGE", 0, 0, "/dev/fbr-test", "read_path=/tmp/fb_in,write_path=/tmp/fb_out")
    local f = assert(io.open("/dev/fbr-test", "r+b"))
    f:setvbuf("no")
    local rd = f:read(10)
    f:seek("set", 0)
    assert(f:write("output"))
    f:close()
    expect(rd == "input data", "read: " .. tostring(rd))
    expect(read_file("/tmp/fb_out") == "output", "written data")
end)

test("UART configuration (modprobe -c existing device)", function()
    run("modprobe -c /dev/ttyS1 baud=9600,parity=even,stop_bits=2")
end)

test("MEMBLOCK configuration (block_size, access)", function()
    T.modprobe("MEMBLOCK", 0, 10, "/dev/mb-test", "block_size=512,access=rdonly")
    run("modprobe -c /dev/mb-test access=rdwr")
end)

test("Invalid values are rejected", function()
    expect(fails("modprobe -c /dev/ttyS1 parity=bad"), "UART parity=bad accepted")
    expect(fails("modprobe -c /dev/ttyS1 stop_bits=3"), "UART stop_bits=3 accepted")
    expect(fails("modprobe -c /dev/mb-test access=execute"), "MEMBLOCK access=execute accepted")
    expect(fails("modprobe -c /dev/fbr-test read_path=/tmp/fb_in"), "FBRIDGE without write_path accepted")
end)

T.summary()
