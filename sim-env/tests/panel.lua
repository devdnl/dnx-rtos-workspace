--[[----------------------------------------------------------------------------
  Helper of the board panel test (tools/simpanel.py protocol) executed by
  tools/simtest.py with board boards/test.lua:
    lua panel.lua [level]
  Sets PA18 (shown by the panel signal P_OUT) to level (0/1, other pins are not
  changed) and prints level of PA19 (driven by the panel signal P_IN): PA19=<0|1>
------------------------------------------------------------------------------]]

local GPIO = "/dev/gpio"

local function port_read(f)
    f:seek("set", 0)
    return (string.unpack("<I4", assert(f:read(4))))
end

local f = assert(io.open(GPIO, "r+b"))
f:setvbuf("no")

local level = tonumber(arg[1])
if level then
    local v = port_read(f)
    v = (level ~= 0) and (v | (1 << 18)) or (v & ~(1 << 18))
    f:seek("set", 0)
    assert(f:write(string.pack("<I4", v)))
end

print("PA19=" .. ((port_read(f) >> 19) & 1))
f:close()
