--[[----------------------------------------------------------------------------
  UART test executed in dnx RTOS (lua program) with board boards/test.lua:
  UARTs connected to board models (uart.attach()), echo on /dev/ttyS2 (UART:2)
  and NMEA GPS on /dev/ttyS3 (UART:3). The GPS is controlled by the test
  control device (I2C:0 0x30). Reads of UART are blocking, so the test reads
  only data that has to come.
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local test, expect = T.test, T.expect

local ECHO = "/dev/ttyS2"
local GPS = "/dev/ttyS3"
local GPS_START = 12 * 3600 * 1000          -- 12:00:00.000 UTC (start_time of test board)

local function uptime()
    return dnx.sys.uptime()
end

local function configure(path, cfg)
    local cmd = string.format("modprobe -c %s %s", path, cfg)
    local ok, out = T.run(cmd)
    if not ok then error(cmd .. ": " .. tostring(out), 2) end
end

local function open(path)
    local f = assert(io.open(path, "r+b"))
    f:setvbuf("no")
    return f
end

local function read(f, n)
    local s = f:read(n)
    if not s then error("read failed", 2) end
    return s
end

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

-- NMEA -----------------------------------------------------------------------------
local function checksum(body)
    local cs = 0
    for i = 1, #body do cs = cs ~ body:byte(i) end
    return string.format("%02X", cs)
end

local function pmtk(body)
    return "$" .. body .. "*" .. checksum(body) .. "\r\n"
end

-- valid sentence: fields (fields[1]: type, e.g. "GPRMC") or nil
local function parse(line)
    local body, cs = line:match("^%$([^*$]+)%*(%x%x)\r?$")
    if not body or checksum(body) ~= cs then return nil end
    local f = {}
    for v in (body .. ","):gmatch("([^,]*),") do f[#f + 1] = v end
    return f
end

-- time of sentence (ms from GPS start) or nil
local function stime(f)
    local h, m, s, ms = (f[2] or ""):match("^(%d%d)(%d%d)(%d%d)%.(%d%d%d)$")
    if not h then return nil end
    return ((tonumber(h) * 60 + tonumber(m)) * 60 + tonumber(s)) * 1000 + tonumber(ms) - GPS_START
end

local gps

-- board time (ms): time base of GPS model. The system uptime is not used as
-- reference because simulated time can drift from host time.
local function board_time()
    return tonumber(control("gps time"))
end

-- next valid sentence of selected type (with time: RMC, GGA) generated after t
-- (board time, default: now)
local function next_sentence(kind, t, max)
    local limit = t or board_time()
    for _ = 1, max or 300 do
        local f = parse(assert(gps:read("l")))
        if f and f[1] == kind then
            local st = stime(f)
            if st and st > limit then return f end
        end
    end
    error("sentence " .. kind .. " not received", 2)
end

-- sentences with time after t (types without time follow sentences with time)
local function read_lines(n, t)
    t = t or board_time()
    local list, fresh = {}, false
    while #list < n do
        local line = assert(gps:read("l"))
        local f = parse(line)
        if f then
            local st = stime(f)
            if st then fresh = st > t end
            if fresh then list[#list + 1] = f end
        end
    end
    return list
end

-- wait for acknowledge of PMTK command
local function wait_ack(cmd, max)
    for _ = 1, max or 100 do
        local f = parse(assert(gps:read("l")))
        if f and f[1] == "PMTK001" and tonumber(f[2]) == cmd then return tonumber(f[3]) end
    end
    return nil
end

local function nmea_coord(v, deg_digits)
    local a = math.abs(v)
    local d = math.floor(a)
    return string.format("%0" .. deg_digits .. "d%07.4f", d, (a - d) * 60)
end

-- tests ---------------------------------------------------------------------------
local echo

test("Open devices", function()
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    ctl = open("/dev/i2c-ow-ctl")
    echo = open(ECHO)
    gps = open(GPS)
    control("gps power 1")
end)

test("Echo model and configuration (9600 8N1)", function()
    configure(ECHO, "baud=9600,parity=off,stop_bits=1")
    echo:write("?cfg\n")
    local r = read(echo, 12)
    expect(r == "9600 none 1\n", "configuration seen by model: " .. r)
    echo:write("hello, board\n")
    r = read(echo, 13)
    expect(r == "hello, board\n", "echo: " .. r)
end)

test("Transmission time (9600 vs 115200 baud)", function()
    local data = string.rep("0123456789", 20)   -- 200 bytes
    local t0 = uptime()
    echo:write(data)
    local r = read(echo, #data)
    local t9600 = uptime() - t0
    expect(r == data, "echo of 200 bytes at 9600")
    expect(t9600 >= 190 and t9600 < 800, "200 bytes at 9600 baud: " .. t9600 .. " ms (expected ~210 ms)")

    configure(ECHO, "baud=115200")
    t0 = uptime()
    echo:write(data)
    r = read(echo, #data)
    local t115200 = uptime() - t0
    expect(r == data, "echo of 200 bytes at 115200")
    expect(t115200 < t9600 / 3, string.format("115200 baud: %d ms, 9600 baud: %d ms", t115200, t9600))
end)

test("Parity and stop bits seen by model", function()
    configure(ECHO, "baud=19200,parity=even,stop_bits=2")
    echo:write("?cfg\n")
    local r = read(echo, 13)
    expect(r == "19200 even 2\n", "configuration: " .. r)
    configure(ECHO, "baud=115200,parity=off,stop_bits=1")
end)

test("GPS sentences (38400 8N1)", function()
    configure(GPS, "baud=38400,parity=off,stop_bits=1")
    local list = read_lines(12)
    local seen = {}
    for _, f in ipairs(list) do seen[f[1]] = true end
    for _, k in ipairs({"GPGGA", "GPGSA", "GPGSV", "GPRMC", "GPVTG"}) do
        expect(seen[k], k .. " not received")
    end
    local rmc = next_sentence("GPRMC")
    expect(rmc[3] == "A", "RMC status " .. tostring(rmc[3]))
    expect(rmc[4] == nmea_coord(52.2297, 2) and rmc[5] == "N", "RMC latitude " .. rmc[4] .. rmc[5])
    expect(rmc[6] == nmea_coord(21.0122, 3) and rmc[7] == "E", "RMC longitude " .. rmc[6] .. rmc[7])
    expect(rmc[10] == "010126", "RMC date " .. tostring(rmc[10]))
    local gga = next_sentence("GPGGA")
    expect(gga[7] == "1" and gga[10] == "100.0", "GGA quality/altitude " .. tostring(gga[7]) .. " " .. tostring(gga[10]))
end)

test("GPS time follows board time", function()
    local a = next_sentence("GPRMC")
    local ta = board_time()
    expect(math.abs(stime(a) - ta) < 500, string.format("GPS time %d ms, board time %.0f ms", stime(a), ta))
    local b = next_sentence("GPRMC")
    local d = stime(b) - stime(a)
    expect(d >= 150 and d <= 250, "fix interval " .. d .. " ms (expected 200 ms)")
end)

test("UART baud rate mismatch (unreadable data)", function()
    configure(GPS, "baud=9600")
    local t0 = board_time()
    local valid = 0
    local data = read(gps, 600)
    for line in data:gmatch("[^\n]+") do
        local f = parse(line)
        local st = f and stime(f)
        if st and st > t0 + 50 then valid = valid + 1 end
    end
    expect(valid == 0, valid .. " valid sentences received at wrong baud rate")
    configure(GPS, "baud=38400,parity=even")
    t0 = board_time()
    valid = 0
    data = read(gps, 600)
    for line in data:gmatch("[^\n]+") do
        local f = parse(line)
        local st = f and stime(f)
        if st and st > t0 + 50 then valid = valid + 1 end
    end
    expect(valid == 0, valid .. " valid sentences received with wrong parity")
    configure(GPS, "baud=38400,parity=off")
    next_sentence("GPRMC")
end)

test("PMTK220 fix interval and PMTK314 sentence output", function()
    gps:write(pmtk("PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"))
    expect(wait_ack(314) == 3, "PMTK314 not acknowledged")
    gps:write(pmtk("PMTK220,500"))
    expect(wait_ack(220) == 3, "PMTK220 not acknowledged")
    local list = read_lines(4)
    for _, f in ipairs(list) do expect(f[1] == "GPRMC", "sentence " .. f[1] .. " (only RMC selected)") end
    local d = stime(list[4]) - stime(list[3])
    expect(d >= 450 and d <= 550, "fix interval " .. d .. " ms (expected 500 ms)")
    gps:write(pmtk("PMTK220,10"))
    expect(wait_ack(220) == 2, "invalid interval accepted")
    gps:write(pmtk("PMTK220,200"))
    expect(wait_ack(220) == 3, "PMTK220 not acknowledged")
    gps:write(pmtk("PMTK314,-1"))
    expect(wait_ack(314) == 3, "PMTK314,-1 not acknowledged")
end)

test("PMTK command with wrong checksum is ignored", function()
    gps:write("$PMTK000*00\r\n")
    expect(wait_ack(0, 25) == nil, "acknowledge of command with wrong checksum")
    gps:write(pmtk("PMTK000"))
    expect(wait_ack(0) == 3, "PMTK000 not acknowledged")
    gps:write(pmtk("PMTK999"))
    expect(wait_ack(999) == 1, "unknown command not reported as unsupported")
end)

test("PMTK251 baud rate change", function()
    gps:write(pmtk("PMTK251,115200"))
    expect(wait_ack(251) == 3, "PMTK251 not acknowledged (at old baud rate)")
    configure(GPS, "baud=115200")
    local f = next_sentence("GPRMC")
    expect(f[3] == "A", "RMC at 115200 baud")
    gps:write(pmtk("PMTK251,38400"))
    expect(wait_ack(251) == 3, "PMTK251 not acknowledged")
    configure(GPS, "baud=38400")
    next_sentence("GPRMC")
end)

test("Fix lost and position changed by board", function()
    control("gps fix 0")
    local rmc = next_sentence("GPRMC")
    local gga = next_sentence("GPGGA")
    expect(rmc[3] == "V" and rmc[4] == "", "RMC without fix: " .. rmc[3])
    expect(gga[7] == "0" and gga[3] == "", "GGA without fix: quality " .. gga[7])
    control("gps pos -33.8688 151.2093 58")
    control("gps fix 1")
    gga = next_sentence("GPGGA")
    expect(gga[3] == "3352.1280" and gga[4] == "S", "latitude " .. gga[3] .. gga[4])
    expect(gga[5] == "15112.5580" and gga[6] == "E", "longitude " .. gga[5] .. gga[6])
    expect(gga[10] == "58.0", "altitude " .. gga[10])
end)

test("Motion (speed and course)", function()
    control("gps pos 0 0 0")
    control("gps motion 194.384 90")             -- 100 m/s to east
    local a = next_sentence("GPRMC")
    local b = next_sentence("GPRMC")
    control("gps motion 0 0")
    expect(a[8] == "194.38" and a[9] == "90.00", "RMC speed/course " .. a[8] .. " " .. a[9])
    local function lon(f) local d = tonumber(f[6]:sub(1, 3)) + tonumber(f[6]:sub(4)) / 60 return d end
    local m = (lon(b) - lon(a)) * 111320
    expect(m > 15 and m < 25, string.format("moved %.1f m in 200 ms (expected 20 m)", m))
    expect(a[4] == "0000.0000", "latitude changed: " .. a[4])
end)

test("PMTK101 restart (no fix until TTFF)", function()
    control("gps pos 52.2297 21.0122 100")
    gps:write(pmtk("PMTK101"))
    expect(wait_ack(101) == 3, "PMTK101 not acknowledged")
    local rmc = next_sentence("GPRMC")
    expect(rmc[3] == "V", "fix after restart")
    local t0 = uptime()
    repeat rmc = next_sentence("GPRMC") until rmc[3] == "A" or uptime() - t0 > 3000
    expect(rmc[3] == "A", "no fix after TTFF")
end)

test("Close devices", function()
    control("gps power 0")
    echo:close()
    gps:close()
    ctl:close()
end)

T.summary()
