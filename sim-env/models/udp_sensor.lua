--[[----------------------------------------------------------------------------
  Ethernet sensor sending its value as text in UDP datagrams (broadcast by
  default), e.g. temperature sensor. Example of device model on netdev.

  Usage:
    local udp_sensor = require("models.udp_sensor")
    local s = udp_sensor.new{ip = "192.168.77.51", port = 5000, value = 21.5}:attach()
    s:set_value(22.0)
    udp_sensor.panel(s, "ETH_TEMP", {label = "Temperature (UDP)", min = -40, max = 85})

  Options of new():
    ip          IPv4 address of the sensor (required)
    port        destination UDP port (default 5000), the same source port
    dst         destination address (default "255.255.255.255": broadcast;
                unicast e.g. "192.168.77.2")
    value       initial value (number or string, default 0)
    format      text of datagram: format string of string.format() with value
                (default "%.2f" for numbers, "%s" for strings) or function
                fn(value, sensor) returning text
    interval    transmission interval in ms (default 10000)
    delay       first transmission after attach in ms (default 1000)
    send_on_change  transmit 300 ms after the last change of value (default
                true; false: only periodic transmissions)
    enabled     sensor transmits (default true)
    netmask, gateway, mac, name, trace   options of netdev.new()

  Methods:
    s:attach()              connect to Ethernet of the simulator, start
    s:set_value(v)          number or string
    s:value()
    s:text()                text of the next datagram
    s:set_enabled(on)       transmissions on/off (sensor disconnected)
    s:enabled()
    s:send()                transmit now (if enabled)
    s:transmissions()       number of sent datagrams
    s:on_send(fn)           fn(sensor, text) after each transmission
    s.dev                   netdev device (e.g. ping, other services)

  Panel:
    udp_sensor.panel(s, id [, opts])  controls: <id> value (slider: opts.min,
      max, step, unit; text field for string values), <id>_EN transmits,
      <id>_SEND transmit now (button), <id>_TX number of transmissions,
      <id>_TEXT text of the last datagram
------------------------------------------------------------------------------]]

local netdev = require("models.netdev")

local M = {}

local S = {}
S.__index = S

function M.new(opts)
    opts = opts or {}
    local s = setmetatable({}, S)
    s.dev = netdev.new{ip = opts.ip, netmask = opts.netmask, gateway = opts.gateway, mac = opts.mac,
                       name = opts.name, trace = opts.trace}
    s.port = opts.port or 5000
    s.dst = opts.dst or "255.255.255.255"
    s.val = opts.value or 0
    s.format = opts.format
    s.interval = opts.interval or 10000
    s.delay = opts.delay or 1000
    s.send_on_change = opts.send_on_change ~= false
    s.on = opts.enabled ~= false
    s.count = 0
    s.handlers = {}
    s.last = ""
    return s
end

function S:attach()
    self.dev:attach()
    self.sock = self.dev:udp(self.port)
    host.timer(self.delay, function()
        self:send()
        self.timer = host.timer(self.interval, function() self:send() end, true)
    end)
    return self
end

function S:text()
    local f = self.format
    if type(f) == "function" then return tostring(f(self.val, self)) end
    if f == nil then f = type(self.val) == "number" and "%.2f" or "%s" end
    return string.format(f, self.val)
end

function S:send()
    if not self.on or not self.sock then return false end
    local text = self:text()
    self.sock:send(self.dst, self.port, text)
    self.count = self.count + 1
    self.last = text
    for _, fn in ipairs(self.handlers) do fn(self, text) end
    return true
end

function S:changed()
    if not self.send_on_change or not self.sock then return end
    if self.change_timer then self.change_timer:stop() end
    self.change_timer = host.timer(300, function()
        self.change_timer = nil
        self:send()
    end)
end

function S:set_value(v)
    self.val = v
    self:changed()
end

function S:value()
    return self.val
end

function S:set_enabled(on)
    self.on = on and true or false
end

function S:enabled()
    return self.on
end

function S:transmissions()
    return self.count
end

function S:on_send(fn)
    table.insert(self.handlers, fn)
end

function M.panel(s, id, opts)
    opts = opts or {}
    local label = opts.label or id
    if type(s:value()) == "number" then
        panel.control(id, function(v) s:set_value(tonumber(v) or s:value()) end,
                      {value = s:value(), min = opts.min or 0, max = opts.max or 100, step = opts.step or 0.1,
                       unit = opts.unit, label = label})
    else
        panel.control(id, function(v) s:set_value(tostring(v)) end,
                      {value = s:value(), type = "string", label = label})
    end
    panel.control(id .. "_EN", function(v) s:set_enabled(v == true) end,
                  {value = s:enabled(), type = "bool", label = label .. ": transmits"})
    panel.control(id .. "_SEND", function(v) if v == true then s:send() end end,
                  {value = false, type = "bool", label = label .. ": send now"})
    panel.value(id .. "_TX", 0, {label = label .. ": transmissions"})
    panel.value(id .. "_TEXT", "", {label = label .. ": datagram"})
    s:on_send(function(_, text)
        panel.set(id .. "_TX", s:transmissions())
        panel.set(id .. "_TEXT", text)
    end)
    return s
end

return M
