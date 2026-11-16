--[[----------------------------------------------------------------------------
  Second network interface of the TCP/IP stack (lwIP) executed in dnx RTOS
  (lua program) with board boards/test.lua: network "inet" (/dev/eth,
  192.168.77.0/24) and network "inet1" (W5500 on SPI:0, segment "lan2"
  192.168.78.0/24), both configured by netmgr (app-romfs/netmgr.conf: inet1
  is configured when the test creates /dev/eth-w5500). Both networks are used
  at the same time: ping from devices of both segments, TCP echo by nc,
  routing of socket opened on other network than destination, link of W5500
  disconnected and connected again (netmgr).
------------------------------------------------------------------------------]]

local T = require("testlib")
local eth = require("dnx.eth")
local test, expect = T.test, T.expect

local W5500 = "/dev/eth-w5500"
local NET2 = "inet1"

local function run(cmd)
    local ok, out = T.run(cmd)
    if not ok then
        error(cmd .. ": " .. tostring(out), 2)
    end
    return out
end

local ctl
local networks -- number of networks before test

local function count_networks()
    return tonumber(run("ifconfig"):match("Number of networks: (%d+)"))
end

-- wait for state of network (ifconfig output contains text, or not)
local function wait_network(netname, text, present, timeout)
    for _ = 1, (timeout or 10000) // 250 do
        local ok, out = T.run("ifconfig " .. netname)
        if ((ok and out:find(text, 1, true)) ~= nil) == present then
            return true
        end
        T.msleep(250)
    end
    return false
end

local function open(path)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    return f
end

local function control(cmd)
    ctl:seek("set", 0)
    assert(ctl:write(cmd))
    ctl:seek("set", 0)
    return assert(ctl:read(128)):match("^[^%z]*")
end

-- ping from model: cmd "net" (192.168.77.50) or "net2" (192.168.78.50)
local function model_ping(cmd, ip)
    control(cmd .. " ping " .. ip)
    for _ = 1, 40 do
        local r = control(cmd .. " pingres")
        if r ~= "pending" then
            return r
        end
        T.msleep(50)
    end
    return "timeout"
end

local function tcp_echo(text, ip, netname)
    local cmd = string.format("nc -w 5 -h -s %s %s%s 7", text, netname and ("-n " .. netname .. " ") or "", ip)
    local ok, out = T.run(cmd)
    return ok and out:find(text, 1, true) ~= nil, out
end

-- tests ------------------------------------------------------------------------------
test("Network inet1 configured by netmgr when interface file is created", function()
    T.spi_devices()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    networks = count_networks()
    T.modprobe("W5500", 2, 0, W5500, "spi_path=/dev/spi-w5500,reset_port=1,reset_pin=10")
    expect(wait_network(NET2, "192.168.78.2", true), "inet1 not configured by netmgr")
    expect(count_networks() == networks + 1, "number of networks")
    expect(run("ifconfig inet"):find("192.168.77.2", 1, true), "address of inet")
end)

test("Ping of the system from devices of both segments", function()
    local r2 = model_ping("net2", "192.168.78.2")
    local r1 = model_ping("net", "192.168.77.2")
    expect(r2:find("^ok"), "inet1: " .. r2)
    expect(r1:find("^ok"), "inet: " .. r1)
end)

test("TCP echo through both interfaces", function()
    local ok2, out2 = tcp_echo("via-w5500", "192.168.78.50")
    local ok1, out1 = tcp_echo("via-eth", "192.168.77.50")
    expect(ok2, "inet1: " .. out2)
    expect(ok1, "inet: " .. out1)
end)

test("Socket of network inet1, destination in network inet (routing by address)", function()
    local ok, out = tcp_echo("cross-route", "192.168.77.50", NET2)
    expect(ok, out)
end)

test("Statistics of W5500 interface", function()
    local st = assert(eth.status(W5500))
    expect(st.tx_packets > 0 and st.rx_packets > 0, "packets " .. st.tx_packets .. " " .. st.rx_packets)
end)

test("Link of W5500 disconnected: inet1 down (netmgr), inet works", function()
    control("w5500 link 0")
    local down = wait_network(NET2, "192.168.78.2", false)
    local r1 = model_ping("net", "192.168.77.2")
    expect(down, "inet1 not disabled after link loss")
    expect(r1:find("^ok"), "inet: " .. r1)
end)

test("Link connected: inet1 configured again (netmgr)", function()
    control("w5500 link 1")
    expect(wait_network(NET2, "192.168.78.2", true), "inet1 not configured after link")
    local r2 = model_ping("net2", "192.168.78.2")
    expect(r2:find("^ok"), "inet1: " .. r2)
end)

test("ifconfig: invalid family of added network", function()
    expect(not T.run("ifconfig inet9 add=xyz:/dev/eth"), "invalid family accepted")
    expect(not T.run("ifconfig " .. NET2 .. " add=inet:" .. W5500), "network added twice")
end)

test("Link disconnected at the end (W5500 does not load SPI:0 in next tests)", function()
    control("w5500 link 0")
    expect(wait_network(NET2, "192.168.78.2", false), "inet1 not disabled")
end)

T.summary()
