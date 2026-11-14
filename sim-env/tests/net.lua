--[[----------------------------------------------------------------------------
  Ethernet test executed in dnx RTOS (lua program) with board boards/test.lua:
  network of the simulator connected to models of the board (eth.attach(),
  models/netdev.lua): ping in both directions, UDP (echo, unicast from the
  model, UDP sensors of models/udp_sensor.lua: broadcast, unicast, format
  function, transmission after change and on request, disabled sensor), TCP
  (echo, transfer in both directions, connection refused, connection from the
  model to the system). The system uses programs ping and nc, models are
  controlled by the test control device (I2C:0 0x30); udp_sensor.panel() is
  tested by the panel protocol test of tools/simtest.py.
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local test, expect = T.test, T.expect

local DEV = "192.168.77.50"         -- netdev of test board
local SYS = "192.168.77.2"          -- system (app-romfs/netmgr.conf)

-- test control device ------------------------------------------------------------
local ctl

local function control(cmd)
    ctl:seek("set", 0)
    assert(ctl:write(cmd))
    ctl:seek("set", 0)
    local r = assert(ctl:read(128)):match("^[^%z]*")
    if r:find("^error") then error("control '" .. cmd .. "': " .. r, 2) end
    return r
end

-- wait until fn() returns value (or timeout), returns the last value
local function wait_for(fn, timeout)
    local t0 = dnx.sys.uptime()
    local v
    repeat
        v = fn()
        if v then return v end
        dnx.sys.msleep(50)
    until dnx.sys.uptime() - t0 > timeout
    return v
end

local function read_file(path)
    local f = io.open(path, "rb")
    if not f then return nil end
    local s = f:read("a")
    f:close()
    return s
end

-- start program in background, output to file
local function spawn(cmd, out)
    return assert(dnx.proc.spawn(cmd, {stdout = out, stderr = out}))
end

local function wait(pid, timeout)
    local status, err = dnx.proc.wait(pid, timeout or 10000)
    if not status then
        dnx.proc.kill(pid)
        dnx.proc.wait(pid, 1000)
        error("program does not finish: " .. tostring(err), 2)
    end
    return status
end

-- test pattern: byte i = i % 251 (the same as TCP source of the model), files are
-- written and checked in blocks (small heap of the system)
local BLOCK = {}
for i = 0, 250 do BLOCK[#BLOCK + 1] = string.char(i) end
BLOCK = table.concat(BLOCK):rep(16)        -- 4016 bytes (multiple of 251)

local function write_pattern(path, n)
    local f = assert(io.open(path, "wb"))
    while n > 0 do
        local k = math.min(n, #BLOCK)
        assert(f:write(BLOCK:sub(1, k)))
        n = n - k
    end
    f:close()
end

-- file contains pattern of n bytes: true or nil, message
local function check_pattern(path, n)
    local f = io.open(path, "rb")
    if not f then return nil, "no file " .. path end
    local pos = 0
    while true do
        local s = f:read(#BLOCK)
        if not s then break end
        if s ~= BLOCK:sub(1, #s) then
            f:close()
            return nil, "data differ in block at " .. pos
        end
        pos = pos + #s
    end
    f:close()
    if pos ~= n then return nil, "size " .. pos .. ", expected " .. n end
    return true
end

-- sum32 of pattern of n bytes (as TCP sink of the model)
local function pattern_sum(n)
    local sum = 0
    for i = 0, n - 1 do sum = sum + i % 251 end
    return sum & 0xFFFFFFFF
end

-- tests --------------------------------------------------------------------------------
test("Open control device", function()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = assert(io.open("/dev/i2c-ow-ctl", "r+b"))
    ctl:setvbuf("no")
end)

test("Network is configured", function()
    local ok = wait_for(function()
        local _, out = T.run("ifconfig")
        return out:find("STATIC IP", 1, true) ~= nil
    end, 10000)
    expect(ok, "inet interface is not configured (netmgr)")
end)

test("Ping: model -> system", function()
    control("net ping " .. SYS)
    local r = wait_for(function()
        local s = control("net pingres")
        return s ~= "pending" and s
    end, 5000)
    expect(r and r:find("^ok"), "ping result: " .. tostring(r))
end)

test("Ping: system -> model", function()
    local pid = spawn("ping " .. DEV, "/tmp/net_ping")
    dnx.sys.msleep(2500)
    dnx.proc.kill(pid)
    dnx.proc.wait(pid, 2000)
    local out = read_file("/tmp/net_ping") or ""
    local _, replies = out:gsub("bytes from " .. DEV:gsub("%.", "%%."), "")
    expect(replies >= 2, "replies: " .. replies .. "\n" .. out)
end)

test("UDP echo", function()
    local ok, out = T.run("nc -u -w 3 -c 1 -s hello " .. DEV .. " 7")
    expect(ok and out:find("HELLO", 1, true), "nc output: " .. tostring(out))
end)

test("UDP: model -> system (unicast)", function()
    local pid = spawn("nc -u -l -w 5 -c 1 6000", "/tmp/net_udp")
    dnx.sys.msleep(300)
    control("net udp " .. SYS .. " 6000 datagram from model")
    local status = wait(pid)
    local out = read_file("/tmp/net_udp") or ""
    expect(status == 0 and out:find("datagram from model", 1, true), "nc output: " .. out)
end)

test("UDP broadcast of sensor", function()
    local tx0 = tonumber(control("net sensor_tx"))
    control("net sensor 23.4")
    control("net sensor_en 1")
    local ok, out = T.run("nc -v -u -l -w 5 -c 2 5000")
    control("net sensor_en 0")
    expect(ok, "nc failed: " .. out)
    local _, n = out:gsub("192%.168%.77%.51:5000 T=23%.4", "")
    expect(n == 2, "datagrams: " .. out)
    local tx = tonumber(control("net sensor_tx"))
    expect(tx >= tx0 + 2, "transmissions: " .. tx0 .. " -> " .. tx)
end)

test("UDP sensor disabled: no transmissions", function()
    local tx0 = control("net sensor_tx")
    expect(control("net sensor_send") == "false", "send() of disabled sensor returned true")
    dnx.sys.msleep(1200)                    -- more than 2 periods (500 ms)
    expect(control("net sensor_tx") == tx0, "transmissions: " .. tx0 .. " -> " .. control("net sensor_tx"))
end)

-- receive datagrams of sensor 192.168.77.52 (unicast to port 5001) while fn() is
-- executed, returns lines of nc output (may be empty) and exit status of nc
local function sensor2_receive(count, timeout, fn)
    os.remove("/tmp/net_s2")
    local pid = spawn("nc -v -u -l -w " .. timeout .. " -c " .. count .. " 5001", "/tmp/net_s2")
    dnx.sys.msleep(300)
    fn()
    local status = wait(pid, (timeout + 5) * 1000)
    local lines = {}
    for l in (read_file("/tmp/net_s2") or ""):gmatch("[^\n]+") do
        if l:find("^192%.168%.77%.52:5001 ") then lines[#lines + 1] = l:sub(20) end
    end
    return lines, status
end

test("UDP sensor: unicast, format function, send after change", function()
    local tx0 = tonumber(control("net sensor2_tx"))
    local lines, status = sensor2_receive(1, 5, function() control("net sensor2 12.5") end)
    expect(status == 0 and #lines == 1, "datagrams: " .. table.concat(lines, " | "))
    expect(lines[1] == string.format("S52 12.50 #%d", tx0 + 1), "datagram: " .. tostring(lines[1]))
    expect(tonumber(control("net sensor2_tx")) == tx0 + 1, "transmissions")
end)

test("UDP sensor: quick changes are sent once", function()
    local tx0 = tonumber(control("net sensor2_tx"))
    local lines = sensor2_receive(2, 2, function()
        control("net sensor2 1")
        control("net sensor2 2")
        control("net sensor2 3.25")
    end)
    expect(#lines == 1, "datagrams: " .. table.concat(lines, " | "))
    expect(lines[1] == string.format("S52 3.25 #%d", tx0 + 1), "datagram: " .. tostring(lines[1]))
    expect(tonumber(control("net sensor2_tx")) == tx0 + 1, "transmissions: " .. control("net sensor2_tx"))
end)

test("UDP sensor: send on request", function()
    local tx0 = tonumber(control("net sensor2_tx"))
    local lines, status = sensor2_receive(1, 5, function()
        expect(control("net sensor2_send") == "true", "send() returned false")
    end)
    expect(status == 0 and lines[1] == string.format("S52 3.25 #%d", tx0 + 1),
           "datagrams: " .. table.concat(lines, " | "))
end)

test("TCP echo", function()
    local ok, out = T.run("nc -w 5 -h -s tcp-echo-text " .. DEV .. " 7")
    expect(ok and out == "tcp-echo-text", "nc output: '" .. tostring(out) .. "'")
end)

local ECHO_SIZE = 65536

test("TCP echo of 64 KiB (both directions)", function()
    write_pattern("/tmp/net_in", ECHO_SIZE)
    os.remove("/tmp/net_out")
    local ok = T.run("nc -w 10 -h -f /tmp/net_in -o /tmp/net_out " .. DEV .. " 7")
    expect(ok, "nc failed")
    local good, err = check_pattern("/tmp/net_out", ECHO_SIZE)
    expect(good, err)
end)

test("TCP: model sends 200 KB", function()
    local n = 200000
    control("net src " .. n)
    os.remove("/tmp/net_src")
    local ok = T.run("nc -w 10 -o /tmp/net_src " .. DEV .. " 19")
    expect(ok, "nc failed")
    local good, err = check_pattern("/tmp/net_src", n)
    expect(good, err)
end)

test("TCP: system sends 64 KiB to model", function()
    local ok = T.run("nc -w 10 -h -f /tmp/net_in " .. DEV .. " 9")
    expect(ok, "nc failed")
    local expected = string.format("%d %d 1", ECHO_SIZE, pattern_sum(ECHO_SIZE))
    local r = wait_for(function()
        local s = control("net sink")
        return s == expected and s
    end, 3000)
    expect(r, "sink: " .. control("net sink") .. ", expected: " .. expected)
end)

test("TCP connection refused", function()
    local ok = T.run("nc -w 2 " .. DEV .. " 81")
    expect(not ok, "connection to closed port succeeded")
end)

test("TCP: model connects to system", function()
    local pid = spawn("nc -l -w 5 -o /tmp/net_srv 7000", "/tmp/net_srv_log")
    dnx.sys.msleep(300)
    control("net connect " .. SYS .. " 7000 hello from model")
    local status = wait(pid)
    expect(status == 0, "nc failed: " .. (read_file("/tmp/net_srv_log") or ""))
    expect(read_file("/tmp/net_srv") == "hello from model", "received: " .. tostring(read_file("/tmp/net_srv")))
    local r = wait_for(function()
        local s = control("net client")
        return s:find("closed$") and s
    end, 3000)
    expect(r, "client state: " .. control("net client"))
end)

test("TCP: model connects to closed port", function()
    control("net connect " .. SYS .. " 7001 x")
    local r = wait_for(function()
        local s = control("net client")
        return s ~= "connecting" and s
    end, 3000)
    expect(r == "refused", "client state: " .. tostring(r))
end)

test("No open connections in model", function()
    local r = wait_for(function()
        return control("net conns") == "0"
    end, 3000)
    expect(r, "open connections: " .. control("net conns"))
end)

test("Close", function()
    ctl:close()
    for _, f in ipairs({"net_ping", "net_udp", "net_in", "net_out", "net_src", "net_srv", "net_srv_log", "net_s2"}) do
        os.remove("/tmp/" .. f)
    end
end)

T.summary()
