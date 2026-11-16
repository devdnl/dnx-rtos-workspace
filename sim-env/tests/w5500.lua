--[[----------------------------------------------------------------------------
  W5500 driver test executed in dnx RTOS (lua program) with board
  boards/test.lua: W5500 on SPI:0 (CS PB9, RSTn PB10) connected to Ethernet
  segment "lan2" of the simulator. Frames are sent and received by luadnx
  module dnx.eth (interface of class ETHIF, not used by the TCP/IP stack):
  ARP, ICMP echo and UDP echo of device 192.168.78.50 (netdev).
------------------------------------------------------------------------------]]

local T = require("testlib")
local eth = require("dnx.eth")
local dnx = require("dnx")
local test, expect = T.test, T.expect

local DEV = "/dev/w5500"
local MAC = "02:00:00:00:55:00"
local IP = "192.168.78.99"
local PEER = "192.168.78.50"

local ctl
local peer_mac

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

local function check(...)
    local r, msg = ...
    if r == nil then
        error(msg, 2)
    end
    return ...
end

local function mac_bytes(s)
    return (s:gsub("(%x%x):?", function(h)
        return string.char(tonumber(h, 16))
    end))
end

local function ip_bytes(s)
    local a, b, c, d = s:match("^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
    return string.char(tonumber(a), tonumber(b), tonumber(c), tonumber(d))
end

local MY_MAC, MY_IP, PEER_IP = mac_bytes(MAC), ip_bytes(IP), ip_bytes(PEER)

local function checksum(data)
    if #data % 2 == 1 then
        data = data .. "\0"
    end
    local sum = 0
    for i = 1, #data, 2 do
        sum = sum + string.unpack(">I2", data, i)
    end
    while sum > 0xFFFF do
        sum = (sum & 0xFFFF) + (sum >> 16)
    end
    return (~sum) & 0xFFFF
end

local function pattern(n, seed)
    local t, x = {}, seed
    for i = 1, n do
        x = (x * 1103515245 + 12345) & 0x7FFFFFFF
        t[i] = string.char(0x61 + ((x >> 16) % 26))
    end
    return table.concat(t)
end

local function ipv4(proto, payload)
    local h = string.pack(">BBI2I2I2BBI2", 0x45, 0, 20 + #payload, 1, 0, 64, proto, 0) .. MY_IP .. PEER_IP
    h = h:sub(1, 10) .. string.pack(">I2", checksum(h)) .. h:sub(13)
    return peer_mac .. MY_MAC .. "\x08\x00" .. h .. payload
end

local function icmp_echo(id, seq, data)
    local m = string.pack(">BBI2I2I2", 8, 0, 0, id, seq) .. data
    m = m:sub(1, 2) .. string.pack(">I2", checksum(m)) .. m:sub(5)
    return ipv4(1, m)
end

local function udp(sport, dport, data)
    return ipv4(17, string.pack(">I2I2I2I2", sport, dport, 8 + #data, 0) .. data)
end

-- receive frame accepted by function (other frames are skipped)
local function receive(pred, timeout)
    local deadline = dnx.sys.uptime() + (timeout or 2000)
    while true do
        local left = deadline - dnx.sys.uptime()
        if left <= 0 then
            return nil
        end
        local f = eth.receive(DEV, left)
        if f and pred(f) then
            return f
        end
    end
end

local function is_echo_reply(id, seq)
    return function(f)
        return f:sub(1, 6) == MY_MAC and f:sub(13, 14) == "\x08\x00" and f:byte(24) == 1 and f:byte(35) == 0
            and string.unpack(">I2", f, 39) == id and string.unpack(">I2", f, 41) == seq
    end
end

local function ping(id, seq, data)
    check(eth.send(DEV, icmp_echo(id, seq, data)))
end

-- tests ------------------------------------------------------------------------------
test("Create devices (modprobe -c), no chip", function()
    T.spi_devices()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    T.modprobe("W5500", 1, 0, "/dev/w5500-none", "spi_path=/dev/spi-nor-none")
    expect(not eth.start("/dev/w5500-none", MAC), "chip detected without chip")
    T.modprobe("W5500", 0, 0, DEV, "spi_path=/dev/spi-w5500,reset_port=1,reset_pin=10")
end)

test("Start, status of interface (link, speed, MAC)", function()
    check(eth.start(DEV, MAC))
    local st = check(eth.status(DEV))
    expect(st.state == "ready" and st.configured, "state " .. st.state)
    expect(st.link == "connected", "link " .. st.link)
    expect(st.speed == 100 and st.full_duplex, "mode " .. st.speed)
    expect(st.mac == MAC, "MAC " .. st.mac)
end)

test("ARP request and reply", function()
    local req = string.rep("\xFF", 6) .. MY_MAC .. "\x08\x06" .. string.pack(">I2I2BBI2", 1, 0x0800, 6, 4, 1)
        .. MY_MAC .. MY_IP .. string.rep("\0", 6) .. PEER_IP
    check(eth.send(DEV, req))
    local r = receive(function(f)
        return f:sub(13, 14) == "\x08\x06" and string.unpack(">I2", f, 21) == 2 and f:sub(29, 32) == PEER_IP
    end)
    expect(r, "no ARP reply")
    expect(r:sub(1, 6) == MY_MAC, "reply not to own MAC")
    peer_mac = r:sub(23, 28)
end)

test("ICMP echo (1000 bytes)", function()
    local data = pattern(1000, 1)
    ping(0x55, 1, data)
    local r = receive(is_echo_reply(0x55, 1))
    expect(r, "no echo reply")
    expect(r:sub(43) == data, "echo data mismatch")
end)

test("UDP echo of maximal frame (1514 bytes)", function()
    local data = pattern(1472, 2)
    local frame = udp(40000, 7, data)
    expect(#frame == 1514, "frame size " .. #frame)
    check(eth.send(DEV, frame))
    local r = receive(function(f)
        return f:sub(13, 14) == "\x08\x00" and f:byte(24) == 17 and string.unpack(">I2", f, 37) == 40000
    end)
    expect(r, "no UDP echo")
    expect(r:sub(43) == data:upper(), "UDP data mismatch")
end)

test("Frames queued in RX memory (memory wrap)", function()
    local data = pattern(1000, 3)
    for round = 1, 3 do
        for seq = 1, 10 do
            ping(0x60 + round, seq, data)
        end
        T.msleep(100)
        for seq = 1, 10 do
            local r = receive(is_echo_reply(0x60 + round, seq), 1000)
            expect(r and r:sub(43) == data, string.format("round %d: reply %d", round, seq))
        end
    end
end)

test("MAC filter: frames of other devices are not received", function()
    control("net2 ping 192.168.78.51") -- frames between other devices of the segment
    local deadline = dnx.sys.uptime() + 500
    while dnx.sys.uptime() < deadline do
        local f = eth.receive(DEV, 100)
        if f then
            local dst = f:sub(1, 6)
            expect(dst == MY_MAC or dst:byte(1) & 1 == 1, "frame to other MAC received")
        end
    end
end)

test("Link down and up", function()
    control("w5500 link 0")
    local st = check(eth.status(DEV))
    control("w5500 link 1")
    expect(st.link == "disconnected", "link " .. st.link)
    expect(check(eth.status(DEV)).link == "connected", "link not restored")
end)

test("Stop and restart (chip reset)", function()
    check(eth.stop(DEV))
    expect(check(eth.status(DEV)).state == "reset", "not stopped")
    expect(not eth.receive(DEV, 0), "receive of stopped interface")
    check(eth.start(DEV, MAC))
    ping(0x70, 1, "restart")
    expect(receive(is_echo_reply(0x70, 1)), "no echo after restart")
    local st = check(eth.status(DEV))
    expect(st.tx_packets >= 1 and st.rx_packets >= 1, "statistics")
    check(eth.stop(DEV))
end)

T.summary()
