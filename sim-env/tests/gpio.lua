--[[----------------------------------------------------------------------------
  GPIO test executed in dnx RTOS (lua program) with board boards/test.lua.
------------------------------------------------------------------------------]]

local T = require("testlib")
local test, expect, pin_set, pin_get = T.test, T.expect, T.pin_set, T.pin_get

local DELAY = 50 -- ms, time for board reaction

local function wait()
    T.msleep(DELAY)
end

test("Inverter PA0 -> PA8", function()
    pin_set(0, false); wait()
    expect(pin_get(8) == true, "PA0=0: PA8 is not high")
    pin_set(0, true); wait()
    expect(pin_get(8) == false, "PA0=1: PA8 is not low")
    pin_set(0, false); wait()
    expect(pin_get(8) == true, "PA0=0: PA8 is not high (again)")
end)

test("Wire PA1 -> PA9", function()
    pin_set(1, true); wait()
    expect(pin_get(9) == true, "PA1=1: PA9 is not high")
    pin_set(1, false); wait()
    expect(pin_get(9) == false, "PA1=0: PA9 is not low")
end)

test("Pull-up PA10 (output state ignored)", function()
    pin_set(10, false); wait()
    expect(pin_get(10) == true, "PA10 is not high")
end)

test("Pulse generator PA11", function()
    local changes, last = 0, pin_get(11)
    for _ = 1, 20 do
        T.msleep(10)
        local v = pin_get(11)
        if v ~= last then changes = changes + 1 end
        last = v
    end
    expect(changes >= 5, "only " .. changes .. " changes in 200 ms (expected about 10)")
end)

test("T flip-flop PA2 -> PA12", function()
    local q0 = pin_get(12)
    for i = 1, 4 do
        pin_set(2, true); wait()
        pin_set(2, false); wait()
        local expected = (i % 2 == 1) ~= q0
        expect(pin_get(12) == expected, "edge " .. i .. ": PA12 is " .. tostring(pin_get(12)))
    end
end)

test("Not connected pin keeps output state", function()
    pin_set(5, true)
    expect(pin_get(5) == true, "PA5 output 1 is not read back")
    pin_set(5, false)
    expect(pin_get(5) == false, "PA5 output 0 is not read back")
end)

T.summary()
