--[[----------------------------------------------------------------------------
  DS2482-100 and DS18B20 test executed in dnx RTOS (lua program) with board
  boards/test.lua. The DS2482 is used by the I2C driver (device files without
  sub-address), the 1-Wire protocol (search, ROM and function commands) is
  implemented here, as it is done by a driver of the DS2482. The test board
  is controlled by the test control device (I2C1 0x30): sensors are
  attached and detached, temperatures, faults and short of the line are set.
------------------------------------------------------------------------------]]

local T = require("testlib")
local dnx = require("dnx")
local test, expect = T.test, T.expect

-- status register bits
local ST_1WB, ST_PPD, ST_SD, ST_LL, ST_RST, ST_SBR, ST_TSB, ST_DIR = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
local CFG_APU, CFG_SPU, CFG_1WS = 0x01, 0x04, 0x08

local BUSY_TIMEOUT = 100  -- ms

local function uptime()
    return dnx.sys.uptime()
end

local function hex(t)
    local s = {}
    for i, b in ipairs(t) do s[i] = string.format("%02X", b) end
    return table.concat(s)
end

local function crc8(bytes, n)
    local crc = 0
    for i = 1, n or #bytes do
        local b = bytes[i]
        for _ = 1, 8 do
            local mix = (crc ~ b) & 1
            crc = crc >> 1
            if mix ~= 0 then crc = crc ~ 0x8C end
            b = b >> 1
        end
    end
    return crc
end

local function rom_crc_ok(rom)
    return crc8(rom, 7) == rom[8]
end

-- I2C device file -------------------------------------------------------------
local Dev = {}
Dev.__index = Dev

local function device(path)
    local f, err = io.open(path, "r+b")
    if not f then error(path .. ": " .. tostring(err), 2) end
    f:setvbuf("no")
    return setmetatable({f = f, path = path}, Dev)
end

function Dev:write(...)
    self.f:seek("set", 0)
    local ok, err = self.f:write(string.char(...))
    return ok ~= nil, err
end

function Dev:read(n)
    self.f:seek("set", 0)
    local s, err = self.f:read(n or 1)
    if not s then error(self.path .. ": read: " .. tostring(err), 2) end
    return s
end

function Dev:close()
    self.f:close()
end

-- test control device of the board
local ctl

local function control(cmd)
    local ok, err = ctl:write(cmd:byte(1, -1))
    expect(ok, "control: " .. tostring(err))
    local r = ctl:read(1024)
    r = r:match("^[^%z]*")
    if r:find("^error") then error("control '" .. cmd .. "': " .. r, 2) end
    return r
end

-- sensors of the board: {n, bus, attached, rom (table), temp, parasite}
local function sensors()
    local list = {}
    for line in control("list"):gmatch("[^\n]+") do
        local n, bus, att, rom, temp, par = line:match("^(%d+) (%a) (%d) (%x+) (%S+) (%d)$")
        local r = {}
        for b in rom:gmatch("%x%x") do r[#r + 1] = tonumber(b, 16) end
        list[tonumber(n)] = {n = tonumber(n), bus = bus, attached = att == "1", rom = r,
                             temp = tonumber(temp), parasite = par == "1"}
    end
    return list
end

local function roms_of(list, bus)
    local t = {}
    for _, s in ipairs(list) do
        if s.bus == bus and s.attached then t[hex(s.rom)] = s end
    end
    return t
end

-- DS2482 ------------------------------------------------------------------------
local OW = {}
OW.__index = OW

local function master(path)
    return setmetatable({dev = device(path)}, OW)
end

function OW:cmd(...)
    local ok, err = self.dev:write(...)
    if not ok then error(string.format("command 0x%02X: %s", (...), tostring(err)), 2) end
end

function OW:rd()
    return self.dev:read(1):byte()
end

-- read pointer is the status register after 1-Wire commands
function OW:wait()
    local t0 = uptime()
    while true do
        local st = self:rd()
        if st & ST_1WB == 0 then return st end
        if uptime() - t0 > BUSY_TIMEOUT then error("1-Wire busy timeout", 2) end
    end
end

function OW:status()
    self:cmd(0xE1, 0xF0)
    return self:rd()
end

function OW:device_reset()
    self:cmd(0xF0)
    return self:rd()
end

-- write configuration, returns read back value
function OW:config(cfg)
    self:cmd(0xD2, ((~cfg & 0x0F) << 4) | cfg)
    return self:rd()
end

function OW:reset()
    self:cmd(0xB4)
    local st = self:wait()
    return st & ST_PPD ~= 0, st
end

function OW:write_byte(b)
    self:cmd(0xA5, b)
    self:wait()
end

function OW:write_bytes(t)
    for _, b in ipairs(t) do self:write_byte(b) end
end

function OW:read_byte()
    self:cmd(0x96)
    self:wait()
    self:cmd(0xE1, 0xE1)
    return self:rd()
end

function OW:read_bytes(n)
    local t = {}
    for i = 1, n do t[i] = self:read_byte() end
    return t
end

function OW:bit(b)
    self:cmd(0x87, b ~= 0 and 0x80 or 0)
    return self:wait() & ST_SBR ~= 0 and 1 or 0
end

function OW:triplet(dir)
    self:cmd(0x78, dir ~= 0 and 0x80 or 0)
    local st = self:wait()
    return st & ST_SBR ~= 0 and 1 or 0, st & ST_TSB ~= 0 and 1 or 0, st & ST_DIR ~= 0 and 1 or 0
end

--[[
  Search of ROM codes (Maxim AN187). opts.cmd: 0xF0 SEARCH ROM (default),
  0xEC ALARM SEARCH; opts.bits: search by single bits (1WSB) instead of
  triplets (1WT); opts.family: target setup, returns first device of the
  family (or of the next family). Returns list of ROM codes (tables of 8
  bytes) or nil and error message.
]]
function OW:search(opts)
    opts = opts or {}
    local rom = {0, 0, 0, 0, 0, 0, 0, 0}
    local last_disc, last_device = 0, false
    local found = {}

    if opts.family then
        rom[1] = opts.family
        last_disc = 64
    end

    repeat
        if not self:reset() then
            return found, "no presence"
        end
        self:write_byte(opts.cmd or 0xF0)

        local last_zero = 0
        for n = 1, 64 do
            local i, mask = ((n - 1) >> 3) + 1, 1 << ((n - 1) & 7)
            local dir
            if n < last_disc then
                dir = rom[i] & mask ~= 0 and 1 or 0
            else
                dir = n == last_disc and 1 or 0
            end

            local id, cmp
            if opts.bits then
                id, cmp = self:bit(1), self:bit(1)
                if id ~= cmp then dir = id end
                if not (id == 1 and cmp == 1) then self:bit(dir) end
            else
                id, cmp, dir = self:triplet(dir)
            end

            if id == 1 and cmp == 1 then
                return found, string.format("no device responds at bit %d", n)
            end
            if id == 0 and cmp == 0 and dir == 0 then last_zero = n end

            if dir ~= 0 then rom[i] = rom[i] | mask else rom[i] = rom[i] & ~mask end
        end

        last_disc = last_zero
        if last_disc == 0 then last_device = true end
        found[#found + 1] = {table.unpack(rom)}
        if #found > 64 then return found, "too many devices (search loop)" end
    until last_device or opts.family

    return found
end

-- select device: MATCH ROM (rom) or SKIP ROM (nil), returns presence
function OW:select(rom)
    if not self:reset() then return false end
    if rom then
        self:write_byte(0x55)
        self:write_bytes(rom)
    else
        self:write_byte(0xCC)
    end
    return true
end

-- read scratchpad, returns bytes and CRC result
function OW:scratchpad(rom)
    expect(self:select(rom), "no presence")
    self:write_byte(0xBE)
    local sp = self:read_bytes(9)
    return sp, crc8(sp, 8) == sp[9]
end

local function sp_temperature(sp)
    local v = sp[1] | (sp[2] << 8)
    if v & 0x8000 ~= 0 then v = v - 0x10000 end
    return v / 16
end

-- read temperature, returns temperature or error
function OW:temperature(rom)
    local sp, ok = self:scratchpad(rom)
    if not ok then return nil, "scratchpad CRC error: " .. hex(sp) end
    return sp_temperature(sp)
end

-- CONVERT T, wait for end of conversion (read slots), returns time (ms)
function OW:convert(rom, spu)
    expect(self:select(rom), "no presence")
    if spu then expect(self:config(CFG_SPU) == CFG_SPU, "SPU not set") end
    local t0 = uptime()
    self:write_byte(0x44)
    if spu then
        T.msleep(800)       -- parasite power: device cannot signal end of conversion
    else
        while self:read_byte() == 0 do
            expect(uptime() - t0 < 2000, "conversion timeout")
        end
    end
    return uptime() - t0
end

function OW:write_scratchpad(rom, th, tl, cfg)
    expect(self:select(rom), "no presence")
    self:write_bytes({0x4E, th & 0xFF, tl & 0xFF, cfg})
end

local function set_of(list)
    local t, n = {}, 0
    for _, r in ipairs(list) do
        local k = hex(r)
        expect(not t[k], "ROM found twice: " .. k)
        t[k] = r
        n = n + 1
    end
    return t, n
end

local function compare_sets(found, expected)
    local fset, fn = set_of(found)
    local en = 0
    for k in pairs(expected) do
        en = en + 1
        expect(fset[k], "ROM not found: " .. k)
    end
    for k in pairs(fset) do
        expect(expected[k], "unexpected ROM found: " .. k)
    end
    expect(fn == en, string.format("found %d devices, expected %d", fn, en))
end

-- tests ---------------------------------------------------------------------------
local a, b
local board

test("Create I2C devices (modprobe -c)", function()
    T.modprobe("I2C", 0, 20, "/dev/i2c-ow-a", "address=0x18")
    T.modprobe("I2C", 0, 21, "/dev/i2c-ow-b", "address=0x19")
    T.modprobe("I2C", 0, 22, "/dev/i2c-ow-ctl", "address=0x30")
    a = master("/dev/i2c-ow-a")
    b = master("/dev/i2c-ow-b")
    ctl = device("/dev/i2c-ow-ctl")
    control("init")
    board = sensors()
    expect(#board == 9, "board sensors: " .. #board)
end)

test("Device reset (DRST)", function()
    local st = a:device_reset()
    expect(st == ST_RST | ST_LL, string.format("status after DRST 0x%02X", st))
    expect(b:device_reset() & ~ST_LL == ST_RST, "bus b: RST not set")
end)

test("Configuration register", function()
    expect(a:config(CFG_APU) == CFG_APU, "APU not set")
    expect(a:status() & ST_RST == 0, "RST not cleared by WCFG")
    a:cmd(0xD2, 0x11)  -- upper nibble is not complement: ignored
    expect(a:rd() == CFG_APU, "invalid configuration byte accepted")
    expect(a:config(0) == 0, "configuration not cleared")
end)

test("Not acknowledged commands", function()
    local ok = a.dev:write(0xE1, 0x55)
    expect(not ok, "invalid read pointer acknowledged")
    ok = a.dev:write(0x12)
    expect(not ok, "unknown command acknowledged")
end)

test("1-Wire reset, presence and busy bit", function()
    local busy_seen = false
    for _ = 1, 5 do
        a:cmd(0xB4)
        if a:rd() & ST_1WB ~= 0 then busy_seen = true end
        local st = a:wait()
        expect(st & ST_PPD ~= 0, "no presence")
        expect(st & ST_SD == 0, "short detected")
        expect(st & ST_LL ~= 0, "line level low")
    end
    expect(busy_seen, "1WB never set after 1WRS")
end)

test("SEARCH ROM (triplets)", function()
    local found, err = a:search()
    expect(not err, err)
    compare_sets(found, roms_of(board, "a"))
    for _, r in ipairs(found) do expect(rom_crc_ok(r), "ROM CRC error: " .. hex(r)) end
end)

test("SEARCH ROM (single bits)", function()
    local found, err = a:search{bits = true}
    expect(not err, err)
    compare_sets(found, roms_of(board, "a"))
end)

test("SEARCH ROM repeated (stable order)", function()
    local f1 = a:search()
    local f2 = a:search()
    expect(#f1 == #f2, "different number of devices")
    for i = 1, #f1 do expect(hex(f1[i]) == hex(f2[i]), "different order at " .. i) end
end)

test("SEARCH ROM target setup (family 0x10)", function()
    local found, err = a:search{family = 0x10}
    expect(not err, err)
    expect(#found == 1 and found[1][1] == 0x10, "family 0x10 not found first: " .. hex(found[1] or {}))
    expect(hex(found[1]) == hex(board[7].rom), "wrong device")
end)

test("SEARCH ROM single device (bus b)", function()
    local found, err = b:search()
    expect(not err, err)
    compare_sets(found, roms_of(board, "b"))
end)

test("READ ROM (single device, collision)", function()
    expect(b:reset(), "no presence")
    b:write_byte(0x33)
    local rom = b:read_bytes(8)
    expect(hex(rom) == hex(board[9].rom), "READ ROM: " .. hex(rom))

    expect(a:reset(), "no presence")
    a:write_byte(0x33)
    rom = a:read_bytes(8)
    local wired_and = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
    for _, s in pairs(roms_of(board, "a")) do
        for i = 1, 8 do wired_and[i] = wired_and[i] & s.rom[i] end
    end
    expect(hex(rom) == hex(wired_and), "collision is not wired-AND: " .. hex(rom))
end)

test("MATCH ROM, scratchpad after power on", function()
    control("powercycle")
    for _, s in pairs(roms_of(board, "a")) do
        local sp, ok = a:scratchpad(s.rom)
        expect(ok, "CRC error " .. hex(sp))
        expect(sp_temperature(sp) == 85, string.format("sensor %d: %.4f C after power on", s.n, sp_temperature(sp)))
        expect(sp[5] == 0x7F, string.format("sensor %d: config 0x%02X", s.n, sp[5]))
    end
end)

test("MATCH ROM of not connected device", function()
    expect(a:select(board[8].rom), "no presence")
    a:write_byte(0xBE)
    local sp = a:read_bytes(9)
    expect(hex(sp) == "FFFFFFFFFFFFFFFFFF", "response of not selected device: " .. hex(sp))
end)

test("READ POWER SUPPLY", function()
    expect(a:select(nil), "no presence")
    a:write_byte(0xB4)
    expect(a:bit(1) == 0, "SKIP ROM: parasite device not reported")
    expect(a:select(board[1].rom), "no presence")
    a:write_byte(0xB4)
    expect(a:bit(1) == 1, "sensor 1 reported as parasite")
    expect(a:select(board[6].rom), "no presence")
    a:write_byte(0xB4)
    expect(a:bit(1) == 0, "sensor 6 not reported as parasite")
end)

test("CONVERT T 12 bit (busy read slots)", function()
    local t = a:convert(board[1].rom)
    expect(t >= 700 and t < 1500, "conversion time " .. t .. " ms")
    local c, err = a:temperature(board[1].rom)
    expect(c == 25.0625, err or string.format("temperature %.4f", c))
end)

test("CONVERT T 9 bit", function()
    local rom = board[1].rom
    a:write_scratchpad(rom, 75, 70, 0x1F)
    local t = a:convert(rom)
    expect(t >= 80 and t < 400, "conversion time " .. t .. " ms")
    local c, err = a:temperature(rom)
    expect(c == 25.0, err or string.format("temperature %.4f", c))
    a:write_scratchpad(rom, 75, 70, 0x7F)
end)

test("Parasite power without strong pull-up", function()
    local rom = board[6].rom
    expect(a:select(rom), "no presence")
    a:write_byte(0x44)
    T.msleep(800)
    local c, err = a:temperature(rom)
    expect(c == 85, err or string.format("temperature %.4f (conversion should fail)", c))
end)

test("Parasite power with strong pull-up (SPU)", function()
    local rom = board[6].rom
    a:convert(rom, true)
    expect(a:reset(), "no presence")
    expect(a:status() & ST_RST == 0, "unexpected RST")
    a:cmd(0xE1, 0xC3)
    expect(a:rd() & CFG_SPU == 0, "SPU not cleared after strong pull-up")
    local c, err = a:temperature(rom)
    expect(c == 21.5, err or string.format("temperature %.4f", c))
end)

test("SKIP ROM CONVERT T, all temperatures", function()
    control("temp 3 -0.0625")
    board = sensors()
    a:convert(nil, true)
    for _, s in pairs(roms_of(board, "a")) do
        local c, err = a:temperature(s.rom)
        expect(c == s.temp, err or string.format("sensor %d: %.4f C, expected %.4f C", s.n, c, s.temp))
    end
end)

test("ALARM SEARCH", function()
    a:write_scratchpad(nil, 30, -5, 0x7F)  -- all devices: TH = 30 C, TL = -5 C
    a:convert(nil, true)
    local expected = {}
    for _, s in pairs(roms_of(board, "a")) do
        local tint = math.floor(s.temp)
        if tint >= 30 or tint <= -5 then expected[hex(s.rom)] = s end
    end
    local found, err = a:search{cmd = 0xEC}
    expect(not err, err)
    compare_sets(found, expected)
    found, err = a:search{cmd = 0xEC, bits = true}
    expect(not err, err)
    compare_sets(found, expected)
end)

test("COPY SCRATCHPAD and RECALL E2", function()
    local rom = board[3].rom
    a:write_scratchpad(rom, 0x11, 0x22, 0x5F)
    expect(a:select(rom), "no presence")
    local t0 = uptime()
    a:write_byte(0x48)
    while a:read_byte() == 0 do expect(uptime() - t0 < 100, "copy timeout") end
    a:write_scratchpad(rom, 0x33, 0x44, 0x1F)
    expect(a:select(rom), "no presence")
    a:write_byte(0xB8)
    local sp = a:scratchpad(rom)
    expect(sp[3] == 0x11 and sp[4] == 0x22 and sp[5] == 0x5F, "RECALL: " .. hex(sp))
    control("powercycle")
    sp = a:scratchpad(rom)
    expect(sp[3] == 0x11 and sp[4] == 0x22 and sp[5] == 0x5F, "after power on: " .. hex(sp))
end)

test("ALARM SEARCH without alarms (triplet: no response)", function()
    -- after power on alarm flags are cleared: presence, but no device responds
    expect(a:reset(), "no presence")
    a:write_byte(0xEC)
    local id, cmp, dir = a:triplet(0)
    expect(id == 1 and cmp == 1 and dir == 1, string.format("triplet: %d %d %d, expected 1 1 1", id, cmp, dir))
    local found, err = a:search{cmd = 0xEC}
    expect(#found == 0 and err, "devices found by ALARM SEARCH")
end)

test("Hot-plug (detach and attach)", function()
    control("detach 3")
    control("attach 8")
    local list = sensors()
    local found, err = a:search()
    expect(not err, err)
    compare_sets(found, roms_of(list, "a"))
    expect(roms_of(list, "a")[hex(board[8].rom)], "sensor 8 not attached")
    control("detach 8")
    control("attach 3")
    found = a:search()
    compare_sets(found, roms_of(board, "a"))
end)

test("Fault: ROM CRC", function()
    control("fault 4 rom_crc 1")
    local found, err = a:search()
    expect(not err, err)
    local bad = 0
    for _, r in ipairs(found) do
        if not rom_crc_ok(r) then bad = bad + 1 end
    end
    control("fault 4 rom_crc 0")
    expect(#found == 7, "found " .. #found .. " devices")
    expect(bad == 1, bad .. " ROM codes with CRC error")
end)

test("Fault: scratchpad CRC", function()
    control("fault 2 crc 1")
    local sp, ok = a:scratchpad(board[2].rom)
    control("fault 2 crc 0")
    expect(not ok, "CRC error not detected: " .. hex(sp))
    sp, ok = a:scratchpad(board[2].rom)
    expect(ok, "CRC error after fault removal")
end)

test("Fault: short of 1-Wire line", function()
    control("short a 1")
    local presence, st = a:reset()
    local found, err = a:search()
    control("short a 0")
    expect(not presence, "presence on shorted line")
    expect(st & ST_SD ~= 0, string.format("SD not set, status 0x%02X", st))
    expect(st & ST_LL == 0, "LL is high on shorted line")
    expect(#found == 0 and err, "devices found on shorted line")
    presence, st = a:reset()
    expect(presence and st & ST_SD == 0, "no presence after short removal")
end)

test("Empty bus", function()
    control("fault 9 mute 1")
    local presence = b:reset()
    local found, err = b:search()
    control("fault 9 mute 0")
    expect(not presence, "presence on empty bus")
    expect(#found == 0 and err == "no presence", "search on empty bus: " .. tostring(err))
end)

test("Overdrive speed (DS18B20 not supported)", function()
    expect(a:config(CFG_1WS) == CFG_1WS, "1WS not set")
    local presence = a:reset()
    a:config(0)
    expect(not presence, "presence at overdrive speed")
    expect(a:reset(), "no presence at standard speed")
end)

test("Release devices", function()
    control("init")
    a.dev:close()
    b.dev:close()
    ctl:close()
end)

T.summary()
