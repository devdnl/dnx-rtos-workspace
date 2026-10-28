--[[----------------------------------------------------------------------------
  Common functions of tests executed in dnx RTOS (lua program), the SUMMARY
  line is printed in the fstest format (checked by tools/simtest.py).
------------------------------------------------------------------------------]]

local dnx = require("dnx")

local T = {executed = 0, passed = 0, failed = 0, skipped = 0}

-- run test function, error message is printed on failure
function T.test(name, fn)
    T.executed = T.executed + 1
    io.write(string.format("TEST %2d: %s... ", T.executed, name))
    local ok, err = pcall(fn)
    if ok then
        T.passed = T.passed + 1
        print("[SUCCESS]")
    else
        T.failed = T.failed + 1
        print("[FAILURE]")
        print("=> " .. tostring(err))
    end
end

function T.expect(cond, msg)
    if not cond then error(msg, 2) end
end

function T.msleep(ms)
    dnx.sys.msleep(ms)
end

function T.summary()
    print(string.format("\nSUMMARY: executed: %d, passed: %d, failed: %d, skipped: %d",
                        T.executed, T.passed, T.failed, T.skipped))
end

-- GPIO port A (/dev/gpio, port state as 32-bit word)
local GPIO = "/dev/gpio"
local out = 0

function T.port_read()
    local f = assert(io.open(GPIO, "rb"))
    f:setvbuf("no")
    local s = assert(f:read(4))
    f:close()
    return (string.unpack("<I4", s))
end

function T.port_write(v)
    out = v
    local f = assert(io.open(GPIO, "r+b"))
    f:setvbuf("no")
    f:write(string.pack("<I4", v))
    f:close()
end

function T.pin_set(n, level)
    if level then T.port_write(out | (1 << n)) else T.port_write(out & ~(1 << n)) end
end

function T.pin_get(n)
    return (T.port_read() >> n) & 1 == 1
end

-- execute shell command, returns true on success and output
function T.run(cmd)
    local p = assert(io.popen(cmd, "r"))
    local output = p:read("a")
    local ok = p:close()
    return ok, output
end

-- create device file if not exists (modprobe), raises error on failure
function T.modprobe(module, major, minor, path, config)
    if require("lfs").attributes(path) then
        return -- exists (can be opened by other driver)
    end
    local cmd = string.format("modprobe -i %s %d %d %s", module, major, minor, path)
    if config then cmd = cmd .. " -c " .. config end
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. out, 2) end
end

-- SPI devices of test board: all CS pins are configured (CS inactive) before
-- the first transfer, otherwise not configured devices are selected (CS low)
T.SPI_DEVICES = {
    {"/dev/spi-a",        0, 10, "cs_port=0,cs_pin=4"},
    {"/dev/spi-b",        0, 11, "cs_port=0,cs_pin=6"},
    {"/dev/spi-b-lsb",    0, 12, "cs_port=0,cs_pin=6,msb_first=0"},
    {"/dev/spi-a-mode1",  0, 13, "cs_port=0,cs_pin=4,mode=1"},
    {"/dev/spi-25lc256",  0, 14, "cs_port=0,cs_pin=7"},
    {"/dev/spi-sd",       0, 15, "cs_port=0,cs_pin=15"},
    {"/dev/spi-sdv1",     0, 16, "cs_port=0,cs_pin=17"},
    {"/dev/spi-595",      1, 10, "cs_port=0,cs_pin=16"},
    {"/dev/spi-none",     2, 10, nil},
}

function T.spi_devices()
    for _, d in ipairs(T.SPI_DEVICES) do
        T.modprobe("SPI", d[2], d[3], d[1], d[4])
    end
end

return T
