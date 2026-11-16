--[[----------------------------------------------------------------------------
  ONEWIRE driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: 1-Wire master on UART:6 (/dev/ttyS6) with DS18B20 sensors
  1..3 (3: parasite power), strong pull-up pin PB8. Bus is used by luadnx
  module dnx.onewire (ioctl) and by read/write of the device file. Sensors
  are controlled by the test control device (I2C:0 0x30).
------------------------------------------------------------------------------]]

local T = require("testlib")
local ow = require("dnx.onewire")
local test, expect = T.test, T.expect

local DEV = "/dev/ow0"

local ctl

local function open(path)
    local f = assert(io.open(path, "r+b"))
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

local function crc8(s)
    local crc = 0
    for i = 1, #s do
        crc = crc ~ s:byte(i)
        for _ = 1, 8 do
            crc = (crc & 1 == 1) and ((crc >> 1) ~ 0x8C) or (crc >> 1)
        end
    end
    return crc
end

-- ROM code (hex, bus order) of DS18B20 with serial number
local function rom_of(serial)
    local b = string.char(0x28) .. string.pack("<I6", serial)
    b = b .. string.char(crc8(b))
    return (b:gsub(".", function(c)
        return string.format("%02X", c:byte())
    end))
end

local ROMS = { rom_of(0x0000000A0001), rom_of(0x8000000A0002), rom_of(0x0000000A0003) }

local function sorted(t)
    local c = { table.unpack(t) }
    table.sort(c)
    return table.concat(c, ",")
end

local function check(...)
    local r, msg = ...
    if r == nil then
        error(msg, 2)
    end
    return ...
end

-- temperature of sensor (scratchpad read by MATCH ROM), TH, TL
local function temperature(rom)
    local sp = check(ow.transfer(DEV, rom, "\xBE", 9))
    expect(crc8(sp:sub(1, 8)) == sp:byte(9), "scratchpad CRC error")
    local raw = string.unpack("<i2", sp)
    return raw / 16, string.unpack("i1", sp, 3), string.unpack("i1", sp, 4)
end

local function convert(spu_ms)
    check(ow.transfer(DEV, nil, "\x44", 0, spu_ms))
    if spu_ms == 0 then
        T.msleep(800)
    end
end

-- tests ------------------------------------------------------------------------------
test("Create device (modprobe -c), invalid configuration", function()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    T.modprobe("ONEWIRE", 1, 0, "/dev/ow1")
    expect(not T.run("modprobe -c /dev/ow1 spu_pin=8"), "configuration without path accepted")
    expect(ow.reset("/dev/ow1") == nil, "reset of not configured bus")
    T.modprobe("ONEWIRE", 0, 0, DEV, "path=/dev/ttyS6,spu_port=1,spu_pin=8")
end)

test("Reset: presence pulse", function()
    expect(check(ow.reset(DEV)) == true, "no presence")
end)

test("SEARCH ROM: all sensors with valid ROM codes", function()
    local roms = check(ow.search(DEV))
    expect(sorted(roms) == sorted(ROMS), "found: " .. sorted(roms))
end)

test("Conversion with strong pull-up, scratchpad of each sensor", function()
    convert(750)
    local EXP = { 23.5, -5.25, 60.125 }
    for i, rom in ipairs(ROMS) do
        local t = temperature(rom)
        expect(t == EXP[i], "sensor " .. i .. ": " .. t)
    end
end)

test("Parasite sensor without strong pull-up: conversion fails", function()
    control("owu temp 1 30.5")
    control("owu temp 3 10")
    convert(0)
    expect(temperature(ROMS[1]) == 30.5, "sensor 1 not converted")
    expect(temperature(ROMS[3]) == 60.125, "parasite sensor converted without strong pull-up")
    convert(750)
    expect(temperature(ROMS[3]) == 10, "parasite sensor not converted")
end)

test("ALARM SEARCH (TH, TL written by SKIP ROM)", function()
    check(ow.transfer(DEV, nil, "\x4E\x32\xF6\x7F")) -- TH 50, TL -10, 12 bits
    local _, th, tl = temperature(ROMS[2])
    expect(th == 50 and tl == -10, "TH/TL not written: " .. th .. " " .. tl)
    control("owu temp 3 70")
    convert(750)
    local roms = check(ow.search(DEV, true))
    expect(sorted(roms) == ROMS[3], "alarm: " .. sorted(roms))
end)

test("Device file: read and write of bytes (MATCH ROM, READ SCRATCHPAD)", function()
    expect(check(ow.reset(DEV)), "no presence")
    local f = open(DEV)
    local rom = ROMS[1]:gsub("%x%x", function(h)
        return string.char(tonumber(h, 16))
    end)
    assert(f:write("\x55" .. rom .. "\xBE"))
    local sp = assert(f:read(9))
    f:close()
    expect(crc8(sp:sub(1, 8)) == sp:byte(9), "scratchpad CRC error")
    expect(string.unpack("<i2", sp) / 16 == 30.5, "temperature")
end)

test("Hot-plug: detached sensor is not found", function()
    control("owu detach 2")
    local roms = check(ow.search(DEV))
    control("owu attach 2")
    expect(sorted(roms) == sorted({ ROMS[1], ROMS[3] }), "found: " .. sorted(roms))
    expect(#check(ow.search(DEV)) == 3, "attached sensor not found")
end)

test("No devices: no presence, transfer fails, empty search", function()
    for i = 1, 3 do
        control("owu detach " .. i)
    end
    local presence = ow.reset(DEV)
    local r, _, errno = ow.transfer(DEV, nil, "\x44")
    local roms = ow.search(DEV)
    for i = 1, 3 do
        control("owu attach " .. i)
    end
    expect(presence == false, "presence without devices")
    expect(r == nil and errno ~= nil, "transfer without devices")
    expect(roms and #roms == 0, "search without devices")
end)

test("Line shorted: bus error", function()
    control("owu short 1")
    local r = ow.reset(DEV)
    local roms = ow.search(DEV)
    control("owu short 0")
    expect(r == nil and roms == nil, "error not reported")
    expect(check(ow.reset(DEV)) == true, "no presence after short")
end)

T.summary()
