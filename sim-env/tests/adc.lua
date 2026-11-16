--[[----------------------------------------------------------------------------
  ADC driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: voltages of channels of ADC:0 and ADC:1 are set by the
  board (control device I2C:0 0x30). Converter is used by read of the device
  file (position / 2 is a channel) and by luadnx module dnx.adc (ioctl).
------------------------------------------------------------------------------]]

local T = require("testlib")
local adc = require("dnx.adc")
local test, expect = T.test, T.expect

local A0, A1 = "/dev/adc0", "/dev/adc1"

local ctl

local function open(path, mode)
    local f = assert(io.open(path, mode or "r+b"))
    f:setvbuf("no")
    return f
end

local function control(cmd)
    ctl:seek("set", 0)
    assert(ctl:write(cmd))
    ctl:seek("set", 0)
    local r = assert(ctl:read(128)):match("^[^%z]*")
    expect(r == "ok", "control '" .. cmd .. "': " .. r)
end

local function check(...)
    local r, msg = ...
    if r == nil then
        error(msg, 2)
    end
    return ...
end

local function configure(path, cfg)
    return T.run(string.format("modprobe -c %s %s", path, cfg))
end

-- samples of channels by read of file
local function samples(path, first, n)
    local f = open(path, "rb")
    f:seek("set", first * 2)
    local data = f:read(n * 2)
    f:close()
    if not data then
        return nil
    end
    local t = {}
    for i = 1, #data // 2 do
        t[i] = string.unpack("<I2", data, i * 2 - 1)
    end
    return t
end

local function raw(path, ch)
    return (check(adc.read(path, ch)))
end

-- tests ------------------------------------------------------------------------------
test("Create devices (modprobe -c), not existing converter", function()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    control("adc 0 init")
    T.modprobe("ADC", 0, 0, A0)
    T.modprobe("ADC", 1, 0, A1, "resolution=10")
    expect(not T.run("modprobe -i ADC 4 0 /dev/adc4"), "converter 4 created")
end)

test("Information: resolution, channels, internal channels, sample time", function()
    local i = check(adc.info(A0))
    expect(i.resolution_max == 12 and i.resolution == 12, "resolution " .. i.resolution)
    expect(i.channels == 16 and i.temp and i.vrefint, "channels")
    expect(i.vref_mv == 3300 and i.oversampling == 1 and not i.hw_oversampling, "defaults")
    expect(i.sample_time_ns == 13333, "sample time " .. i.sample_time_ns)
    expect(check(adc.info(A1)).resolution == 10, "resolution of ADC 1")
end)

test("Read of file: one channel, channels 0..3", function()
    expect(samples(A0, 1, 1)[1] == 2048, "channel 1")
    local s = samples(A0, 0, 4)
    expect(#s == 4 and s[1] == 0 and s[2] == 2048 and s[3] == 4095 and s[4] == 1241,
        "channels: " .. table.concat(s, " "))
    expect(samples(A1, 0, 1)[1] == 155, "ADC 1 channel 0 (10 bits)")
end)

test("Read beyond the last channel, odd position", function()
    expect(samples(A0, 16, 1) == nil, "channel 16 read")
    local s = samples(A0, 14, 4)
    expect(#s == 2, "read across the last channel: " .. #s)
    local f = open(A0, "rb")
    f:seek("set", 3)
    expect(f:read(2) == nil, "read at odd position")
    f:close()
end)

test("ioctl: raw value and voltage", function()
    local r, v = check(adc.read(A0, 3))
    expect(r == 1241, "raw " .. r)
    expect(math.abs(v - 1.0) < 0.001, "voltage " .. v)
    expect(adc.read(A0, 16) == nil, "channel 16 read")
end)

test("Resolution 8 and 6 bits, invalid resolution", function()
    assert(configure(A0, "resolution=8"))
    expect(raw(A0, 1) == 128 and raw(A0, 2) == 255, "8 bits")
    assert(configure(A0, "resolution=6"))
    expect(raw(A0, 2) == 63, "6 bits")
    expect(not configure(A0, "resolution=7"), "7 bits accepted")
    expect(not configure(A0, "resolution=14"), "14 bits accepted")
    assert(configure(A0, "resolution=0"))
    expect(check(adc.info(A0)).resolution == 12, "maximal resolution")
end)

test("Oversampling (software), invalid values", function()
    assert(configure(A0, "oversampling=16"))
    local i = check(adc.info(A0))
    expect(i.oversampling == 16 and not i.hw_oversampling, "oversampling")
    expect(raw(A0, 1) == 2048, "averaged value")
    expect(not configure(A0, "oversampling=3"), "oversampling 3 accepted")
    expect(not configure(A0, "oversampling=2048"), "oversampling 2048 accepted")
    expect(not configure(A0, "oversampling=0"), "oversampling 0 accepted")
    assert(configure(A0, "oversampling=1"))
end)

test("Sample time selection", function()
    assert(configure(A0, "sample_time_ns=0"))
    expect(check(adc.info(A0)).sample_time_ns == 83, "shortest")
    assert(configure(A0, "sample_time_ns=100"))
    expect(check(adc.info(A0)).sample_time_ns == 416, "100 ns")
    assert(configure(A0, "sample_time_ns=1000"))
    expect(check(adc.info(A0)).sample_time_ns == 1555, "1000 ns")
    assert(configure(A0, "sample_time_ns=1000000"))
    expect(check(adc.info(A0)).sample_time_ns == 13333, "longest")
end)

test("Reference voltage of calculation", function()
    assert(configure(A0, "vref_mv=3000"))
    local _, v = check(adc.read(A0, 1))
    assert(configure(A0, "vref_mv=3300"))
    expect(math.abs(v - 1.5) < 0.001, "voltage " .. v)
    expect(not configure(A0, "vref_mv=0"), "vref 0 accepted")
end)

test("Voltage changed by board", function()
    control("adc 0 5 2.5")
    expect(raw(A0, 5) == 3102, "channel 5: " .. raw(A0, 5))
    control("adc 0 5 4.0")
    expect(raw(A0, 5) == 4095, "saturation")
end)

test("Internal channels: temperature, reference", function()
    local t = check(adc.temperature(A0))
    expect(math.abs(t - 25) < 0.5, "temperature " .. t)
    control("adc 0 temp 60")
    t = check(adc.temperature(A0))
    expect(math.abs(t - 60) < 0.5, "temperature " .. t)
    control("adc 0 temp 25")
    local _, v = check(adc.read(A0, "vref"))
    expect(math.abs(v - 1.21) < 0.002, "reference " .. v)
end)

T.summary()
