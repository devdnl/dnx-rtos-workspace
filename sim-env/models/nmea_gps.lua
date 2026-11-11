--[[----------------------------------------------------------------------------
  NMEA GPS receiver model (MediaTek MT3339 style, e.g. PA6H/PA1616 modules)
  connected to UART of the board.

  Usage:
    local nmea_gps = require("models.nmea_gps")
    local gps = nmea_gps.new{lat = 52.2297, lon = 21.0122, alt = 110}
    gps:attach("UART:1")                      -- /dev/ttyS1 of dnx RTOS
    gps:set_motion(12.5, 90)                  -- 12.5 knots to east
    gps:set_fix(false)                        -- no fix (e.g. signal lost)

  Options of new():
    lat, lon, alt   position (degrees, negative: S/W; meters), default
                    52.2297, 21.0122, 100
    speed, course   speed over ground (knots) and course (degrees), default 0,
                    position is moved by speed and course at each fix
    fix             position fix (default true)
    satellites      number of satellites used in fix (default 8, 4..12)
    hdop            horizontal dilution of precision (default 0.9)
    baud            baud rate of the receiver (default 9600, 8N1), the system
                    has to configure UART to the same baud rate and no parity,
                    otherwise it receives unreadable data
    rate            fix interval in ms (default 1000, PMTK220)
    sentences       output of sentences: table name -> output every n fixes
                    (0: off), default {GGA = 1, GSA = 1, GSV = 1, RMC = 1,
                    VTG = 1}, also GLL, ZDA (PMTK314)
    talker          talker ID (default "GP")
    start_time      UTC time (seconds since 1970) at board start (default:
                    host time), the time advances with host time
    ttff            time to the first fix after restart (ms): {hot = , warm =
                    , cold = }, default 1000, 5000, 15000 (PMTK101..104)
    boot            boot messages at start and restart ($PMTK011,MTKGPS and
                    $PMTK010,001), default true

  Output: sentences of each fix (GGA, GSA, GSV, RMC, VTG, GLL, ZDA) with CRLF.
  Without fix the time is sent (date and time are known), position fields
  are empty, RMC status is V, GGA quality 0.

  Commands sent by the system ($PMTK<n>,...*<checksum><CR><LF>, sentences with
  wrong checksum are ignored), acknowledge $PMTK001,<n>,<flag> (3: success,
  2: failed, 1: unsupported):
    PMTK000                     test
    PMTK101/102/103/104         hot/warm/cold/full cold restart (no fix for
                                ttff time, boot message)
    PMTK220,<ms>                fix interval (100..10000 ms)
    PMTK251,<baud>              baud rate (4800..115200, 0: default 9600),
                                acknowledge is sent at the old baud rate
    PMTK314,<f0>,...,<f18>      sentence output (0: GLL, 1: RMC, 2: VTG, 3: GGA,
                                4: GSA, 5: GSV, 17: ZDA), PMTK314,-1: default
    PMTK605                     firmware release ($PMTK705,...)

  Methods:
    gps:attach(uart [, opts])   connect to UART (opts of uart.attach())
    gps:set_position(lat, lon [, alt])
    gps:position()              lat, lon, alt
    gps:set_motion(speed, course)  knots, degrees
    gps:set_fix(on)             position fix
    gps:fix()                   fix is reported
    gps:set_satellites(n)
    gps:set_enabled(on)         output of sentences (receiver power)
    gps:restart(kind)           "hot", "warm", "cold"
    gps:on_sentence(fn)         fn(sentence) for each sent sentence (without
                                CRLF)
------------------------------------------------------------------------------]]

local M = {}

local GPS = {}
GPS.__index = GPS

local DEFAULT_SENTENCES = {GGA = 1, GSA = 1, GSV = 1, RMC = 1, VTG = 1, GLL = 0, ZDA = 0}
local ORDER = {"GGA", "GLL", "GSA", "GSV", "RMC", "VTG", "ZDA"}
local PMTK314_FIELDS = {[0] = "GLL", "RMC", "VTG", "GGA", "GSA", "GSV", [17] = "ZDA"}
local BAUDS = {[4800] = true, [9600] = true, [14400] = true, [19200] = true, [38400] = true,
               [57600] = true, [115200] = true}
local SAT_PRN = {1, 3, 6, 9, 11, 14, 17, 19, 22, 24, 28, 31}

-- NMEA checksum (XOR of characters between $ and *)
function M.checksum(body)
    local cs = 0
    for i = 1, #body do cs = cs ~ body:byte(i) end
    return string.format("%02X", cs)
end

local function sentence(body)
    return "$" .. body .. "*" .. M.checksum(body)
end

local function copy(t)
    local c = {}
    for k, v in pairs(t) do c[k] = v end
    return c
end

function M.new(opts)
    opts = opts or {}
    local g = setmetatable({}, GPS)
    g.lat = opts.lat or 52.2297
    g.lon = opts.lon or 21.0122
    g.alt = opts.alt or 100
    g.speed = opts.speed or 0
    g.course = opts.course or 0
    g.fix_on = opts.fix ~= false
    g.sats = opts.satellites or 8
    g.hdop = opts.hdop or 0.9
    g.baud = opts.baud or 9600
    g.rate = opts.rate or 1000
    g.default_sentences = copy(DEFAULT_SENTENCES)
    for k, v in pairs(opts.sentences or {}) do g.default_sentences[k] = v end
    g.sentences = copy(g.default_sentences)
    g.talker = opts.talker or "GP"
    g.start_time = opts.start_time or os.time()
    g.ttff = {hot = 1000, warm = 5000, cold = 15000}
    for k, v in pairs(opts.ttff or {}) do g.ttff[k] = v end
    g.boot = opts.boot ~= false
    g.enabled = true
    g.no_fix_until = 0
    g.fixes = 0
    g.rx = ""
    g.handlers = {}
    g.mismatch = nil
    assert(BAUDS[g.baud], "nmea_gps: unsupported baud rate " .. tostring(g.baud))
    return g
end

-- UART ------------------------------------------------------------------------------
-- configuration of system UART differs from receiver (data unreadable)
function GPS:mismatched()
    local cfg = uart.config(self.uart)
    local bad = (cfg.baud ~= self.baud) or (cfg.parity ~= "none")
    local key = string.format("%d %s %d", cfg.baud, cfg.parity, self.baud)
    if bad and self.mismatch ~= key then
        print(string.format("nmea_gps %s: UART configured to %d baud, parity %s; receiver uses %d 8N1: data unreadable",
                            self.uart, cfg.baud, cfg.parity, self.baud))
    end
    self.mismatch = bad and key or nil
    return bad
end

-- send text to the system (unreadable bytes on configuration mismatch)
function GPS:send(text)
    if self:mismatched() then
        local t = {}
        for i = 1, #text do
            self.noise = ((self.noise or 0x5A) * 75 + 74) % 65537
            t[i] = string.char(self.noise & 0xFF)
        end
        text = table.concat(t)
    end
    uart.send(self.uart, text)
end

function GPS:send_sentence(body)
    local s = sentence(body)
    for _, fn in ipairs(self.handlers) do fn(s) end
    self:send(s .. "\r\n")
end

function GPS:receive(data)
    if self:mismatched() then
        self.rx = ""
        return
    end
    self.rx = self.rx .. data
    while true do
        local line, rest = self.rx:match("^([^\n]*)\n(.*)$")
        if not line then break end
        self.rx = rest
        self:command((line:gsub("\r$", "")))
    end
    if #self.rx > 256 then self.rx = "" end
end

function GPS:configure()
    self:mismatched()
end

function GPS:attach(name, opts)
    self.uart = name
    uart.attach(name, self, opts)
    self.t0 = host.time_ms()
    if self.boot then self:boot_messages() end
    self:start_timer()
    return self
end

function GPS:start_timer()
    if self.timer then self.timer:stop() end
    self.timer = host.timer(self.rate, function() self:tick() end, true)
end

function GPS:boot_messages()
    self:send_sentence("PMTK011,MTKGPS")
    self:send_sentence("PMTK010,001")
end

-- commands ----------------------------------------------------------------------
function GPS:ack(cmd, flag)
    self:send_sentence(string.format("PMTK001,%d,%d", cmd, flag))
end

function GPS:command(line)
    local body, cs = line:match("^%$([^*]+)%*(%x%x)$")
    if not body or M.checksum(body) ~= cs:upper() then return end
    local fields = {}
    for f in (body .. ","):gmatch("([^,]*),") do fields[#fields + 1] = f end
    local cmd = tonumber((fields[1]:match("^PMTK(%d+)$")))
    if not cmd then return end
    local arg = function(i) return tonumber(fields[i + 1]) end

    if cmd == 0 then
        self:ack(cmd, 3)
    elseif cmd >= 101 and cmd <= 104 then
        self:ack(cmd, 3)
        self:restart(({[101] = "hot", [102] = "warm", [103] = "cold", [104] = "cold"})[cmd])
    elseif cmd == 220 then
        local ms = arg(1)
        if ms and ms >= 100 and ms <= 10000 then
            self.rate = math.floor(ms)
            self:start_timer()
            self:ack(cmd, 3)
        else
            self:ack(cmd, 2)
        end
    elseif cmd == 251 then
        local baud = arg(1)
        if baud == 0 then baud = 9600 end
        if baud and BAUDS[baud] then
            self:ack(cmd, 3)              -- sent at the old baud rate
            self.baud = baud
        else
            self:ack(cmd, 2)
        end
    elseif cmd == 314 then
        if arg(1) == -1 then
            self.sentences = copy(self.default_sentences)
        else
            for i, name in pairs(PMTK314_FIELDS) do
                self.sentences[name] = math.tointeger(arg(i + 1)) or 0
            end
        end
        self:ack(cmd, 3)
    elseif cmd == 605 then
        self:send_sentence("PMTK705,AXN_SIM_1.0,0001,SIMGPS,1.0")
    else
        self:ack(cmd, 1)
    end
end

-- fix -------------------------------------------------------------------------------
function GPS:utc()
    return self.start_time + host.time_ms() / 1000
end

function GPS:fix()
    return self.fix_on and host.time_ms() >= self.no_fix_until
end

local function nmea_lat(v)
    local a = math.abs(v)
    local d = math.floor(a)
    return string.format("%02d%07.4f", d, (a - d) * 60), v >= 0 and "N" or "S"
end

local function nmea_lon(v)
    local a = math.abs(v)
    local d = math.floor(a)
    return string.format("%03d%07.4f", d, (a - d) * 60), v >= 0 and "E" or "W"
end

function GPS:move(dt)
    if self.speed > 0 then
        local d = self.speed * 0.514444 * dt
        local c = math.rad(self.course)
        self.lat = self.lat + d * math.cos(c) / 111320
        self.lon = self.lon + d * math.sin(c) / (111320 * math.max(math.cos(math.rad(self.lat)), 0.01))
        if self.lon > 180 then self.lon = self.lon - 360 elseif self.lon < -180 then self.lon = self.lon + 360 end
        self.lat = math.max(-89.9999, math.min(89.9999, self.lat))
    end
end

function GPS:tick()
    if not self.enabled then return end
    self:move(self.rate / 1000)
    self.fixes = self.fixes + 1

    local t = self:utc()
    local sec = math.floor(t)
    local d = os.date("!*t", sec)
    local hms = string.format("%02d%02d%02d.%03d", d.hour, d.min, d.sec, math.floor((t - sec) * 1000 + 0.5) % 1000)
    local date = string.format("%02d%02d%02d", d.day, d.month, d.year % 100)
    local fix = self:fix()
    local lat, ns = nmea_lat(self.lat)
    local lon, ew = nmea_lon(self.lon)
    if not fix then lat, ns, lon, ew = "", "", "", "" end
    local sats = fix and math.max(4, math.min(12, self.sats)) or 0
    local tk = self.talker

    local out = {
        GGA = function()
            if fix then
                return string.format("%sGGA,%s,%s,%s,%s,%s,1,%02d,%.2f,%.1f,M,0.0,M,,", tk, hms, lat, ns, lon, ew,
                                     sats, self.hdop, self.alt)
            end
            return string.format("%sGGA,%s,,,,,0,00,,,M,,M,,", tk, hms)
        end,
        GLL = function()
            return string.format("%sGLL,%s,%s,%s,%s,%s,%s,%s", tk, lat, ns, lon, ew, hms, fix and "A" or "V",
                                 fix and "A" or "N")
        end,
        GSA = function()
            local prn = {}
            for i = 1, 12 do prn[i] = (i <= sats) and string.format("%02d", SAT_PRN[i]) or "" end
            local pdop, vdop = self.hdop * 1.4, self.hdop * 1.1
            if fix then
                return string.format("%sGSA,A,3,%s,%.2f,%.2f,%.2f", tk, table.concat(prn, ","), pdop, self.hdop, vdop)
            end
            return string.format("%sGSA,A,1,%s,,,", tk, table.concat(prn, ","))
        end,
        GSV = function()
            local n = math.max(4, math.min(12, self.sats))
            local total = (n + 3) // 4
            local list = {}
            for m = 1, total do
                local f = {string.format("%sGSV,%d,%d,%02d", tk, total, m, n)}
                for i = (m - 1) * 4 + 1, math.min(m * 4, n) do
                    local snr = fix and string.format("%02d", 20 + (i * 7) % 25) or ""
                    f[#f + 1] = string.format("%02d,%02d,%03d,%s", SAT_PRN[i], 15 + (i * 13) % 70, (i * 47) % 360, snr)
                end
                list[#list + 1] = table.concat(f, ",")
            end
            return list
        end,
        RMC = function()
            if fix then
                return string.format("%sRMC,%s,A,%s,%s,%s,%s,%.2f,%.2f,%s,,,A", tk, hms, lat, ns, lon, ew,
                                     self.speed, self.course, date)
            end
            return string.format("%sRMC,%s,V,,,,,0.00,0.00,%s,,,N", tk, hms, date)
        end,
        VTG = function()
            if fix then
                return string.format("%sVTG,%.2f,T,,M,%.2f,N,%.2f,K,A", tk, self.course, self.speed, self.speed * 1.852)
            end
            return string.format("%sVTG,,T,,M,,N,,K,N", tk)
        end,
        ZDA = function()
            return string.format("%sZDA,%s,%02d,%02d,%04d,,", tk, hms, d.day, d.month, d.year)
        end,
    }

    for _, name in ipairs(ORDER) do
        local every = self.sentences[name] or 0
        if every > 0 and (self.fixes % every == 0) then
            local s = out[name]()
            if type(s) == "table" then
                for _, b in ipairs(s) do self:send_sentence(b) end
            else
                self:send_sentence(s)
            end
        end
    end
end

-- host interface ----------------------------------------------------------------
function GPS:set_position(lat, lon, alt)
    assert(type(lat) == "number" and type(lon) == "number", "nmea_gps: position expected")
    self.lat, self.lon = lat, lon
    if alt then self.alt = alt end
end

function GPS:position()
    return self.lat, self.lon, self.alt
end

function GPS:set_motion(speed, course)
    self.speed = math.max(0, speed or 0)
    self.course = (course or 0) % 360
end

function GPS:set_fix(on)
    self.fix_on = on and true or false
end

function GPS:set_satellites(n)
    self.sats = math.max(0, math.floor(n))
end

function GPS:set_enabled(on)
    self.enabled = on and true or false
end

function GPS:restart(kind)
    local tmo = self.ttff[kind or "hot"] or self.ttff.hot
    self.no_fix_until = host.time_ms() + tmo
    if self.boot then self:send_sentence("PMTK010,001") end
end

function GPS:on_sentence(fn)
    assert(type(fn) == "function", "nmea_gps: function expected")
    table.insert(self.handlers, fn)
end

return M
