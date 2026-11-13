--[[----------------------------------------------------------------------------
  Network device for board models: minimal IPv4 stack on the Ethernet of the
  simulator (eth.attach()): ARP, IPv4 (no fragmentation), ICMP echo, UDP and
  TCP. Device models use it to talk to the system (and to the host by TAP
  interface: --tap, devices are visible from the host, e.g. ping, Wireshark).

  Network of the simulator (app-romfs/netmgr.conf): 192.168.77.0/24, the
  system is 192.168.77.2, host TAP interface is usually 192.168.77.1.

  Usage:
    local netdev = require("models.netdev")
    local dev = netdev.new{ip = "192.168.77.50"}:attach()

    -- UDP
    local sock = dev:udp(5000)                       -- port (nil: any free)
    sock:on_receive(function(data, ip, port) ... end)
    sock:send("192.168.77.2", 5000, "text")
    sock:send("255.255.255.255", 5000, "text")       -- broadcast

    -- TCP server
    dev:tcp_listen(7, function(conn)
        conn:on_data(function(data) conn:send(data) end)   -- echo
    end)

    -- TCP client
    dev:tcp_connect("192.168.77.2", 1883, function(conn, err)
        if not conn then print(err) return end
        conn:send("hello")
        conn:close()
    end)

    dev:ping("192.168.77.2", function(ok, ms) ... end)

  Options of new():
    ip          IPv4 address (required)
    netmask     default "255.255.255.0"
    gateway     default nil (only local network is reachable)
    mac         MAC address "02:00:00:00:00:32", default 02:00 + IP bytes
    name        name in messages (default IP address)
    trace       print sent and received packets (default false)

  Device methods:
    dev:attach()                connect to Ethernet of the simulator
    dev:detach()
    dev:udp([port])             UDP socket:
      sock:send(ip, port, data)     broadcast: 255.255.255.255 or subnet
                                    broadcast address
      sock:on_receive(fn)           fn(data, ip, port)
      sock:close()
      sock.port
    dev:tcp_listen(port, fn)    TCP server, fn(conn) for each accepted
                                connection, returns server with close()
    dev:tcp_connect(ip, port, fn [, timeout])
                                TCP client, fn(conn) when connected or
                                fn(nil, "refused"|"timeout"|"unreachable")
    dev:ping(ip, fn [, timeout])  ICMP echo, fn(ok, ms)

  TCP connection:
    conn:send(data)             queue data (sent by window of receiver)
    conn:close()                close after queued data are sent (FIN)
    conn:abort()                reset connection (RST)
    conn:on_data(fn)            fn(data) data received
    conn:on_end(fn)             fn() remote side closed sending (FIN); without
                                handler the connection is closed
    conn:on_close(fn)           fn(reason) connection removed: "closed",
                                "reset", "timeout"
    conn:pending()              bytes not acknowledged by remote side
    conn.remote_ip, conn.remote_port, conn.local_port, conn.state

  Limitations: no IP fragmentation and options, TCP without congestion
  control and out of order queue (segments out of order are dropped and
  retransmitted by sender), fixed receive window.
------------------------------------------------------------------------------]]

local M = {}

local BROADCAST_MAC = string.rep("\xFF", 6)
local ETH_IP, ETH_ARP = 0x0800, 0x0806
local PROTO_ICMP, PROTO_TCP, PROTO_UDP = 1, 6, 17

local FIN, SYN, RST, PSH, ACK = 0x01, 0x02, 0x04, 0x08, 0x10

local TCP_MSS = 1460          -- own MSS (Ethernet)
local TCP_WINDOW = 8760       -- receive window (data are delivered immediately)
local TCP_RTO = 300           -- initial retransmission timeout (ms)
local TCP_RTO_MAX = 4000
local TCP_RETRIES = 10
local TCP_TIME_WAIT = 1000    -- shortened TIME-WAIT (ms)
local ARP_RETRY = 500         -- ms
local ARP_RETRIES = 3

-- address helpers ---------------------------------------------------------------
function M.ip_parse(s)
    if type(s) == "number" then return s & 0xFFFFFFFF end
    local a, b, c, d = string.match(tostring(s), "^(%d+)%.(%d+)%.(%d+)%.(%d+)$")
    a, b, c, d = tonumber(a), tonumber(b), tonumber(c), tonumber(d)
    if not a or a > 255 or b > 255 or c > 255 or d > 255 then
        error("netdev: invalid IPv4 address: " .. tostring(s), 3)
    end
    return a << 24 | b << 16 | c << 8 | d
end

function M.ip_str(n)
    return string.format("%d.%d.%d.%d", n >> 24 & 255, n >> 16 & 255, n >> 8 & 255, n & 255)
end

function M.mac_parse(s)
    local t = {}
    for h in string.gmatch(tostring(s), "%x%x") do t[#t + 1] = string.char(tonumber(h, 16)) end
    if #t ~= 6 then error("netdev: invalid MAC address: " .. tostring(s), 3) end
    return table.concat(t)
end

function M.mac_str(m)
    return (m:gsub(".", function(c) return string.format("%02X:", c:byte()) end):sub(1, -2))
end

-- Internet checksum (ones' complement sum of 16-bit words)
function M.checksum(data, sum)
    sum = sum or 0
    local n = #data
    for i = 1, n - 1, 2 do
        local a, b = data:byte(i, i + 1)
        sum = sum + (a << 8 | b)
    end
    if n % 2 == 1 then sum = sum + (data:byte(n) << 8) end
    while sum > 0xFFFF do sum = (sum & 0xFFFF) + (sum >> 16) end
    return ~sum & 0xFFFF
end

local function pseudo_sum(src, dst, proto, len)
    return (src >> 16) + (src & 0xFFFF) + (dst >> 16) + (dst & 0xFFFF) + proto + len
end

local function seq_add(a, b) return (a + b) & 0xFFFFFFFF end

-- signed difference a - b of sequence numbers
local function seq_diff(a, b)
    local d = (a - b) & 0xFFFFFFFF
    return d >= 0x80000000 and d - 0x100000000 or d
end

local function random32()
    return math.random(0, 0xFFFF) << 16 | math.random(0, 0xFFFF)
end

-- device -------------------------------------------------------------------------
local DEV = {}
DEV.__index = DEV

function M.new(opts)
    opts = opts or {}
    local d = setmetatable({}, DEV)
    d.ip = M.ip_parse(opts.ip or error("netdev: ip expected", 2))
    d.mask = M.ip_parse(opts.netmask or "255.255.255.0")
    d.gateway = opts.gateway and M.ip_parse(opts.gateway) or nil
    d.mac = opts.mac and M.mac_parse(opts.mac)
            or ("\x02\x00" .. string.pack(">I4", d.ip))
    d.bcast = (d.ip & d.mask) | (~d.mask & 0xFFFFFFFF)
    d.name = opts.name or M.ip_str(d.ip)
    d.trace = opts.trace
    d.arp = {}               -- ip -> mac
    d.arp_wait = {}          -- ip -> {packets = {}, tries, timer}
    d.udp_socks = {}         -- port -> socket
    d.listeners = {}         -- port -> server
    d.conns = {}             -- key -> connection
    d.pings = {}             -- id:seq -> {fn, t0, timer}
    d.ip_id = math.random(0, 0xFFFF)
    d.next_port = math.random(49152, 60000)
    d.ping_id = math.random(0, 0xFFFF)
    d.ping_seq = 0
    return d
end

function DEV:log(fmt, ...)
    print(string.format("netdev %s: " .. fmt, self.name, ...))
end

function DEV:attach()
    eth.attach(self)
    -- gratuitous ARP: the system and the host learn MAC address
    self:arp_send(1, BROADCAST_MAC, self.ip, string.rep("\0", 6), self.ip)
    return self
end

function DEV:detach()
    eth.detach(self)
end

function DEV:free_port()
    for _ = 1, 20000 do
        local p = self.next_port
        self.next_port = self.next_port < 65535 and self.next_port + 1 or 49152
        if not self.udp_socks[p] and not self.listeners[p] then
            local used = false
            for _, c in pairs(self.conns) do
                if c.local_port == p then used = true break end
            end
            if not used then return p end
        end
    end
    error("netdev: no free port", 3)
end

-- Ethernet ---------------------------------------------------------------------------
function DEV:eth_send(dst_mac, etype, payload)
    local frame = dst_mac .. self.mac .. string.pack(">I2", etype) .. payload
    if #frame < 60 then frame = frame .. string.rep("\0", 60 - #frame) end
    eth.send(frame, self)
end

function DEV:receive(frame)
    if #frame < 14 then return end
    local dst = frame:sub(1, 6)
    if dst ~= self.mac and dst ~= BROADCAST_MAC then return end
    local etype = string.unpack(">I2", frame, 13)
    if etype == ETH_ARP then
        self:arp_input(frame:sub(15))
    elseif etype == ETH_IP then
        self:ip_input(frame:sub(15), frame:sub(7, 12))
    end
end

-- ARP ---------------------------------------------------------------------------------
function DEV:arp_send(op, dst_mac, target_ip, target_mac, sender_ip)
    local p = string.pack(">I2I2BBI2", 1, ETH_IP, 6, 4, op) .. self.mac
              .. string.pack(">I4", sender_ip or self.ip) .. target_mac .. string.pack(">I4", target_ip)
    self:eth_send(dst_mac, ETH_ARP, p)
end

function DEV:arp_input(p)
    if #p < 28 then return end
    local htype, ptype, hlen, plen, op = string.unpack(">I2I2BBI2", p)
    if htype ~= 1 or ptype ~= ETH_IP or hlen ~= 6 or plen ~= 4 then return end
    local sha, spa = p:sub(9, 14), string.unpack(">I4", p, 15)
    local tpa = string.unpack(">I4", p, 25)
    if spa ~= 0 and (tpa == self.ip or self.arp[spa]) then
        self:arp_learn(spa, sha)
    end
    if op == 1 and tpa == self.ip and spa ~= self.ip then
        self:arp_send(2, sha, spa, sha)
    end
end

function DEV:arp_learn(ip, mac)
    self.arp[ip] = mac
    local w = self.arp_wait[ip]
    if w then
        self.arp_wait[ip] = nil
        if w.timer then w.timer:stop() end
        for _, packet in ipairs(w.packets) do self:eth_send(mac, ETH_IP, packet) end
    end
end

function DEV:arp_resolve(ip, packet)
    local w = self.arp_wait[ip]
    if w then
        if #w.packets < 16 then table.insert(w.packets, packet) end
        return
    end
    w = {packets = {packet}, tries = 0}
    self.arp_wait[ip] = w
    local function try()
        if self.arp_wait[ip] ~= w then return end
        w.tries = w.tries + 1
        if w.tries > ARP_RETRIES then
            self.arp_wait[ip] = nil
            if self.trace then self:log("ARP: %s does not respond, packets dropped", M.ip_str(ip)) end
            self:unreachable(ip)
            return
        end
        self:arp_send(1, BROADCAST_MAC, ip, string.rep("\0", 6))
        w.timer = host.timer(ARP_RETRY, try)
    end
    try()
end

-- IPv4 --------------------------------------------------------------------------------
function DEV:is_broadcast(ip)
    return ip == 0xFFFFFFFF or ip == self.bcast
end

function DEV:ip_send(dst, proto, payload)
    self.ip_id = (self.ip_id + 1) & 0xFFFF
    local h = string.pack(">BBI2I2I2BBI2I4I4", 0x45, 0, 20 + #payload, self.ip_id, 0x4000, 64, proto, 0,
                          self.ip, dst)
    h = h:sub(1, 10) .. string.pack(">I2", M.checksum(h)) .. h:sub(13)
    local packet = h .. payload
    if self.trace then self:log("TX %s -> %s proto %d, %d B", M.ip_str(self.ip), M.ip_str(dst), proto, #payload) end

    if self:is_broadcast(dst) then
        self:eth_send(BROADCAST_MAC, ETH_IP, packet)
        return true
    end

    local hop = dst
    if (dst & self.mask) ~= (self.ip & self.mask) then
        if not self.gateway then return false end
        hop = self.gateway
    end
    local mac = self.arp[hop]
    if mac then
        self:eth_send(mac, ETH_IP, packet)
    else
        self:arp_resolve(hop, packet)
    end
    return true
end

function DEV:ip_input(p, src_mac)
    if #p < 20 then return end
    local vihl, _, total, _, frag, _, proto, _, src, dst = string.unpack(">BBI2I2I2BBI2I4I4", p)
    local ihl = (vihl & 0x0F) * 4
    if vihl >> 4 ~= 4 or ihl < 20 or total < ihl or total > #p then return end
    if M.checksum(p:sub(1, ihl)) ~= 0 then return end
    if dst ~= self.ip and not self:is_broadcast(dst) then return end
    if (frag & 0x3FFF) ~= 0 then
        if self.trace then self:log("IP fragment from %s dropped", M.ip_str(src)) end
        return
    end
    if src ~= 0 and (src & self.mask) == (self.ip & self.mask) and not self.arp[src] then
        self.arp[src] = src_mac
    end
    local header, data = p:sub(1, ihl), p:sub(ihl + 1, total)
    if self.trace then self:log("RX %s -> %s proto %d, %d B", M.ip_str(src), M.ip_str(dst), proto, #data) end
    if proto == PROTO_ICMP then
        self:icmp_input(src, dst, data)
    elseif proto == PROTO_UDP then
        self:udp_input(src, dst, data, header)
    elseif proto == PROTO_TCP and dst == self.ip then
        self:tcp_input(src, data)
    end
end

-- ICMP --------------------------------------------------------------------------------
function DEV:icmp_send(dst, typ, code, rest)
    local m = string.pack(">BBI2", typ, code, 0) .. rest
    m = m:sub(1, 2) .. string.pack(">I2", M.checksum(m)) .. m:sub(5)
    self:ip_send(dst, PROTO_ICMP, m)
end

function DEV:icmp_input(src, dst, m)
    if #m < 8 or M.checksum(m) ~= 0 then return end
    local typ = m:byte(1)
    if typ == 8 and dst == self.ip then
        self:icmp_send(src, 0, 0, m:sub(5))
    elseif typ == 0 then
        local id, seq = string.unpack(">I2I2", m, 5)
        local key = id .. ":" .. seq
        local p = self.pings[key]
        if p then
            self.pings[key] = nil
            p.timer:stop()
            p.fn(true, host.time_ms() - p.t0)
        end
    end
end

-- ICMP destination unreachable (port: code 3)
function DEV:icmp_unreachable(src, code, header, data)
    self:icmp_send(src, 3, code, "\0\0\0\0" .. header .. data:sub(1, 8))
end

function DEV:ping(ip, fn, timeout)
    local dst = M.ip_parse(ip)
    self.ping_seq = (self.ping_seq + 1) & 0xFFFF
    local key = self.ping_id .. ":" .. self.ping_seq
    local p = {fn = fn, t0 = host.time_ms()}
    p.timer = host.timer(timeout or 2000, function()
        if self.pings[key] == p then
            self.pings[key] = nil
            fn(false)
        end
    end)
    self.pings[key] = p
    self:icmp_send(dst, 8, 0, string.pack(">I2I2", self.ping_id, self.ping_seq) .. "netdev ping")
end

-- UDP ---------------------------------------------------------------------------------
local SOCK = {}
SOCK.__index = SOCK

function DEV:udp(port)
    port = port or self:free_port()
    if self.udp_socks[port] then error("netdev: UDP port in use: " .. port, 2) end
    local s = setmetatable({dev = self, port = port}, SOCK)
    self.udp_socks[port] = s
    return s
end

function SOCK:on_receive(fn)
    self.handler = fn
end

function SOCK:send(ip, port, data)
    local dev = self.dev
    local dst = M.ip_parse(ip)
    data = tostring(data)
    local len = 8 + #data
    local u = string.pack(">I2I2I2I2", self.port, port, len, 0) .. data
    local sum = M.checksum(u, pseudo_sum(dev.ip, dst, PROTO_UDP, len))
    if sum == 0 then sum = 0xFFFF end
    u = u:sub(1, 6) .. string.pack(">I2", sum) .. u:sub(9)
    return dev:ip_send(dst, PROTO_UDP, u)
end

function SOCK:close()
    if self.dev.udp_socks[self.port] == self then self.dev.udp_socks[self.port] = nil end
end

function DEV:udp_input(src, dst, u, header)
    if #u < 8 then return end
    local sport, dport, len, sum = string.unpack(">I2I2I2I2", u)
    if len < 8 or len > #u then return end
    u = u:sub(1, len)
    if sum ~= 0 and M.checksum(u, pseudo_sum(src, dst, PROTO_UDP, len)) ~= 0 then return end
    local s = self.udp_socks[dport]
    if s then
        if s.handler then
            local ok, err = xpcall(s.handler, debug.traceback, u:sub(9), M.ip_str(src), sport)
            if not ok then print(err) end
        end
    elseif dst == self.ip then
        self:icmp_unreachable(src, 3, header, u)
    end
end

-- TCP ---------------------------------------------------------------------------------
local CONN = {}
CONN.__index = CONN

local function conn_key(ip, rport, lport)
    return ip .. ":" .. rport .. ":" .. lport
end

-- send TCP segment
function DEV:tcp_segment(src_port, dst, dst_port, seq, ack, flags, window, data, mss)
    data = data or ""
    local opts = mss and string.pack(">BBI2", 2, 4, mss) or ""
    local off = (20 + #opts) // 4
    local t = string.pack(">I2I2I4I4BBI2I2I2", src_port, dst_port, seq, ack, off << 4, flags, window, 0, 0)
              .. opts .. data
    local sum = M.checksum(t, pseudo_sum(self.ip, dst, PROTO_TCP, #t))
    t = t:sub(1, 16) .. string.pack(">I2", sum) .. t:sub(19)
    self:ip_send(dst, PROTO_TCP, t)
end

function DEV:new_conn(ip, rport, lport)
    local c = setmetatable({dev = self, rip = ip, remote_ip = M.ip_str(ip), remote_port = rport,
                            local_port = lport, sndbuf = "", snd_wnd = 0, mss = 536,
                            rto = TCP_RTO, retries = 0, handlers = {}}, CONN)
    c.iss = random32()
    c.snd_una = c.iss
    c.snd_nxt = seq_add(c.iss, 1)
    self.conns[conn_key(ip, rport, lport)] = c
    return c
end

local function parse_mss(t, off)
    local i = 21
    while i < off do
        local kind = t:byte(i)
        if kind == 0 then break end
        if kind == 1 then
            i = i + 1
        else
            local len = t:byte(i + 1) or 2
            if kind == 2 and len == 4 then return string.unpack(">I2", t, i + 2) end
            i = i + math.max(len, 2)
        end
    end
    return 536
end

function DEV:tcp_input(src, t)
    if #t < 20 then return end
    local sum = M.checksum(t, pseudo_sum(src, self.ip, PROTO_TCP, #t))
    if sum ~= 0 then return end
    local sport, dport, seq, ack, offb, flags, wnd = string.unpack(">I2I2I4I4BBI2", t)
    local off = (offb >> 4) * 4
    if off < 20 or off > #t then return end
    local data = t:sub(off + 1)
    local c = self.conns[conn_key(src, sport, dport)]

    if not c then
        if flags & RST ~= 0 then return end
        local srv = self.listeners[dport]
        if srv and flags & SYN ~= 0 and flags & ACK == 0 then
            c = self:new_conn(src, sport, dport)
            c.server = srv
            c.state = "SYN_RCVD"
            c.irs = seq
            c.rcv_nxt = seq_add(seq, 1)
            c.snd_wnd = wnd
            c.mss = math.min(parse_mss(t, off), TCP_MSS)
            c:send_syn()
            return
        end
        -- no connection: reset
        if flags & ACK ~= 0 then
            self:tcp_segment(dport, src, sport, ack, 0, RST, 0)
        else
            local len = #data + ((flags & SYN ~= 0) and 1 or 0) + ((flags & FIN ~= 0) and 1 or 0)
            self:tcp_segment(dport, src, sport, 0, seq_add(seq, len), RST | ACK, 0)
        end
        return
    end

    c:input(seq, ack, flags, wnd, data, t, off)
end

function DEV:tcp_listen(port, fn)
    if self.listeners[port] then error("netdev: TCP port in use: " .. port, 2) end
    local srv = {port = port, fn = fn, dev = self}
    function srv:close() if self.dev.listeners[self.port] == self then self.dev.listeners[self.port] = nil end end
    self.listeners[port] = srv
    return srv
end

function DEV:tcp_connect(ip, port, fn, timeout)
    local dst = M.ip_parse(ip)
    local c = self:new_conn(dst, port, self:free_port())
    c.state = "SYN_SENT"
    c.connect_fn = fn
    c.connect_timer = host.timer(timeout or 5000, function()
        if c.state == "SYN_SENT" then c:finish("timeout") end
    end)
    c:send_syn()
    return c
end

-- host unreachable (ARP): connections in SYN_SENT fail
function DEV:unreachable(ip)
    for _, c in pairs(self.conns) do
        if c.rip == ip and c.state == "SYN_SENT" then c:finish("unreachable") end
    end
end

function CONN:segment(flags, seq, data, mss)
    self.dev:tcp_segment(self.local_port, self.rip, self.remote_port, seq, self.rcv_nxt or 0,
                         flags, TCP_WINDOW, data, mss)
end

function CONN:send_syn()
    if self.state == "SYN_SENT" then
        self:segment(SYN, self.iss, nil, TCP_MSS)
    else
        self:segment(SYN | ACK, self.iss, nil, TCP_MSS)
    end
    self:arm_timer()
end

function CONN:send_ack()
    self:segment(ACK, self.snd_nxt)
end

function CONN:on_data(fn) self.handlers.data = fn end
function CONN:on_end(fn) self.handlers["end"] = fn end
function CONN:on_close(fn) self.handlers.close = fn end

function CONN:call(name, ...)
    local fn = self.handlers[name]
    if fn then
        local ok, err = xpcall(fn, debug.traceback, ...)
        if not ok then print(err) end
    end
    return fn ~= nil
end

function CONN:pending()
    return #self.sndbuf
end

function CONN:send(data)
    data = tostring(data)
    if self.closing or self.state == "CLOSED" then error("netdev: TCP connection is closed", 2) end
    self.sndbuf = self.sndbuf .. data
    self:transmit()
    return true
end

function CONN:close()
    if self.closing or self.state == "CLOSED" then return end
    self.closing = true
    if self.state == "SYN_SENT" then
        self:finish("closed")
        return
    end
    self:transmit()
end

function CONN:abort()
    if self.state ~= "CLOSED" then
        self:segment(RST | ACK, self.snd_nxt)
        self:finish("reset")
    end
end

-- remove connection
function CONN:finish(reason)
    if self.state == "CLOSED" then return end
    local was_connecting = self.state == "SYN_SENT"
    self.state = "CLOSED"
    if self.timer then self.timer:stop() self.timer = nil end
    if self.connect_timer then self.connect_timer:stop() end
    local key = conn_key(self.rip, self.remote_port, self.local_port)
    if self.dev.conns[key] == self then self.dev.conns[key] = nil end
    if was_connecting and self.connect_fn then
        local ok, err = xpcall(self.connect_fn, debug.traceback, nil, reason == "reset" and "refused" or reason)
        if not ok then print(err) end
    elseif self.accepted or self.connected then
        self:call("close", reason)
    end
end

function CONN:arm_timer()
    if self.timer then return end
    self.timer = host.timer(self.rto, function()
        self.timer = nil
        self:timeout()
    end)
end

function CONN:restart_timer()
    if self.timer then self.timer:stop() self.timer = nil end
    if seq_diff(self.snd_nxt, self.snd_una) > 0 then self:arm_timer() end
end

-- retransmission (go-back-N) or zero window probe
function CONN:timeout()
    if self.state == "CLOSED" then return end
    if seq_diff(self.snd_nxt, self.snd_una) <= 0 and not self.probe then return end
    self.retries = self.retries + 1
    if self.retries > TCP_RETRIES then
        self:segment(RST | ACK, self.snd_nxt)
        self:finish("timeout")
        return
    end
    self.rto = math.min(self.rto * 2, TCP_RTO_MAX)
    if self.state == "SYN_SENT" or self.state == "SYN_RCVD" then
        self:send_syn()
        return
    end
    self.snd_nxt = self.snd_una
    if self.fin_sent and not self.fin_acked then self.fin_sent = false end
    self:transmit(true)
end

-- send data allowed by window, FIN after data when closing
function CONN:transmit(retransmit)
    local st = self.state
    if st ~= "ESTABLISHED" and st ~= "CLOSE_WAIT" and st ~= "FIN_WAIT_1" and st ~= "CLOSING"
       and st ~= "LAST_ACK" then
        return
    end
    local inflight = seq_diff(self.snd_nxt, self.snd_una)
    local window = self.snd_wnd
    self.probe = false
    if window == 0 and inflight == 0 and #self.sndbuf > 0 then
        -- zero window probe: one byte by timer
        if retransmit then window = 1 else self.probe = true self:arm_timer() return end
    end
    while true do
        local sent = seq_diff(self.snd_nxt, self.snd_una)
        local unsent = #self.sndbuf - sent
        local usable = window - sent
        if unsent <= 0 or usable <= 0 then break end
        local n = math.min(unsent, usable, self.mss)
        self:segment(ACK | PSH, self.snd_nxt, self.sndbuf:sub(sent + 1, sent + n))
        self.snd_nxt = seq_add(self.snd_nxt, n)
    end
    local sent = seq_diff(self.snd_nxt, self.snd_una)
    if self.closing and not self.fin_sent and sent >= #self.sndbuf then
        self.fin_sent = true
        self.fin_seq = self.snd_nxt
        self:segment(FIN | ACK, self.snd_nxt)
        self.snd_nxt = seq_add(self.snd_nxt, 1)
        if self.state == "ESTABLISHED" then
            self.state = "FIN_WAIT_1"
        elseif self.state == "CLOSE_WAIT" then
            self.state = "LAST_ACK"
        end
    end
    if seq_diff(self.snd_nxt, self.snd_una) > 0 then self:arm_timer() end
end

function CONN:time_wait()
    self.state = "TIME_WAIT"
    if self.timer then self.timer:stop() self.timer = nil end
    host.timer(TCP_TIME_WAIT, function()
        if self.state == "TIME_WAIT" then self:finish("closed") end
    end)
end

function CONN:established()
    self.state = "ESTABLISHED"
    self.retries = 0
    self.rto = TCP_RTO
    if self.timer then self.timer:stop() self.timer = nil end
end

function CONN:input(seq, ack, flags, wnd, data, t, off)
    if flags & RST ~= 0 then
        if self.state == "SYN_SENT" then
            if flags & ACK ~= 0 and ack == self.snd_nxt then self:finish("reset") end
        else
            self:finish("reset")
        end
        return
    end

    if self.state == "SYN_SENT" then
        if flags & ACK ~= 0 and ack ~= self.snd_nxt then
            self.dev:tcp_segment(self.local_port, self.rip, self.remote_port, ack, 0, RST, 0)
            return
        end
        if flags & SYN ~= 0 and flags & ACK ~= 0 then
            self.irs = seq
            self.rcv_nxt = seq_add(seq, 1)
            self.snd_una = ack
            self.snd_wnd = wnd
            self.mss = math.min(parse_mss(t, off), TCP_MSS)
            self:established()
            if self.connect_timer then self.connect_timer:stop() end
            self:send_ack()
            self.connected = true
            if self.connect_fn then
                local ok, err = xpcall(self.connect_fn, debug.traceback, self)
                if not ok then print(err) end
            end
            self:transmit()
        end
        return
    end

    if self.state == "SYN_RCVD" and flags & SYN ~= 0 and flags & ACK == 0 then
        self:send_syn() -- SYN retransmitted by remote side
        return
    end

    if flags & ACK == 0 then return end

    if self.state == "SYN_RCVD" then
        if ack ~= self.snd_nxt then
            self.dev:tcp_segment(self.local_port, self.rip, self.remote_port, ack, 0, RST, 0)
            return
        end
        self.snd_una = ack
        self.snd_wnd = wnd
        self:established()
        self.accepted = true
        local ok, err = xpcall(self.server.fn, debug.traceback, self)
        if not ok then print(err) end
        if self.state == "CLOSED" then return end
    end

    -- acknowledgment
    local acked = seq_diff(ack, self.snd_una)
    if acked > 0 and seq_diff(ack, self.snd_nxt) <= 0 then
        local data_acked = acked
        if self.fin_sent and seq_diff(ack, self.fin_seq) > 0 then
            data_acked = data_acked - 1
            self.fin_acked = true
        end
        self.sndbuf = self.sndbuf:sub(data_acked + 1)
        self.snd_una = ack
        self.retries = 0
        self.rto = TCP_RTO
        self:restart_timer()
    end
    if acked >= 0 and seq_diff(ack, self.snd_nxt) <= 0 then
        self.snd_wnd = wnd
    end

    if self.fin_acked then
        if self.state == "FIN_WAIT_1" then
            self.state = "FIN_WAIT_2"
        elseif self.state == "CLOSING" then
            self:time_wait()
        elseif self.state == "LAST_ACK" then
            self:finish("closed")
            return
        end
    end

    -- data and FIN
    local fin = flags & FIN ~= 0
    if #data > 0 or fin then
        local d = seq_diff(seq, self.rcv_nxt)
        if d < 0 and -d < #data then
            data = data:sub(-d + 1) -- part already received
            d = 0
        end
        if d == 0 then
            local st = self.state
            if #data > 0 and (st == "ESTABLISHED" or st == "FIN_WAIT_1" or st == "FIN_WAIT_2") then
                self.rcv_nxt = seq_add(self.rcv_nxt, #data)
                self:call("data", data)
                if self.state == "CLOSED" then return end
            elseif #data > 0 then
                self.rcv_nxt = seq_add(self.rcv_nxt, #data) -- data after own close: ignored
            end
            if fin and not self.fin_received then
                self.fin_received = true
                self.rcv_nxt = seq_add(self.rcv_nxt, 1)
                self:send_ack()
                if self.state == "ESTABLISHED" then
                    self.state = "CLOSE_WAIT"
                    if not self:call("end") then self:close() end
                elseif self.state == "FIN_WAIT_1" then
                    if self.fin_acked then self:time_wait() else self.state = "CLOSING" end
                elseif self.state == "FIN_WAIT_2" then
                    self:time_wait()
                end
            else
                self:send_ack()
            end
        else
            self:send_ack() -- out of order or duplicate
        end
    end

    self:transmit()
end

return M
