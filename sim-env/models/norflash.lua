--[[----------------------------------------------------------------------------
  Serial NOR flash memory model (W25Qxx, MX25Lxx and similar), SPI mode 0 or 3,
  MSB first. Memory is stored sparsely (only programmed 4 KiB sectors), so big
  memories can be simulated.

  Usage:
    local norflash = require("models.norflash")
    local flash = norflash.new{type = "w25q16"}
    flash:attach("SPI:0", "PB3")                -- bus, CS pin (active low)

  Options of new():
    type          memory type (table TYPES), or explicit parameters:
    id            JEDEC ID (3 bytes, string)
    size          memory size in bytes (multiple of 4 KiB)
    sfdp          SFDP table is available (JEDEC basic flash parameters)
    addr4         4-byte address instructions (0x13, 0x12, 0x21, 0x0C)
    protected     block protection set at power-up (BP2..BP0 = 111, whole
                  memory protected, as SST26/AT25 memories)
    power_down    memory in deep power-down at start (only RDP 0xAB accepted)
    page_time     page program time in ms (default 0.5)
    erase_time    sector erase time in ms (default 3)

  Instructions: RDID (0x9F), SFDP (0x5A), READ (0x03, 0x13), FAST READ (0x0B,
  0x0C), PP (0x02, 0x12, page roll-over), SE 4 KiB (0x20, 0x21), BE 64 KiB
  (0xD8), CE (0xC7, 0x60), WREN (0x06), WRDI (0x04), RDSR (0x05), WRSR (0x01),
  global block unlock (0x98, clears BP bits), deep power-down (0xB9), release
  from deep power-down (0xAB). Programming only clears bits (AND). During
  program/erase only RDSR is accepted.

  Methods:
    flash:attach(bus, cs)        connect to bus
    flash:read(addr, n)          read memory (host side)
    flash:write(addr, data)      write memory without erase (host side)
    flash:erase_all()            erase memory (host side)
    flash:stats()                number of sector erases, page programs
------------------------------------------------------------------------------]]

local M = {}

local SECTOR = 4096
local PAGE = 256
local ERASED = string.rep("\xFF", SECTOR)

M.TYPES = {
    w25q16 = { id = "\xEF\x40\x15", size = 2 * 1024 * 1024, sfdp = true },
    w25q256 = { id = "\xEF\x40\x19", size = 32 * 1024 * 1024, sfdp = true, addr4 = true },
    mx25l8005 = { id = "\xC2\x20\x14", size = 1024 * 1024, sfdp = false },
    sst26vf016 = { id = "\xBF\x26\x41", size = 2 * 1024 * 1024, sfdp = true, protected = true },
}

local FLASH = {}
FLASH.__index = FLASH

-- SFDP table: header, parameter header of JEDEC basic table (9 DWORDs at 0x30)
local function sfdp_table(size)
    local t = string.rep("\xFF", 0x30 + 36)
    local function put(off, s)
        t = t:sub(1, off) .. s .. t:sub(off + #s + 1)
    end
    put(0, "SFDP\x06\x01\x00\xFF")
    put(8, "\x00\x06\x01\x09\x30\x00\x00\xFF")
    local bits = size * 8
    local dw1 = 0xFFF920E5 -- 4 KiB erase (0x20), 3 or 4 address bytes
    local dw2
    if bits > 0x80000000 then
        dw2 = 0x80000000 | (math.floor(math.log(bits, 2) + 0.5))
    else
        dw2 = bits - 1
    end
    put(0x30, string.pack("<I4I4", dw1, dw2))
    return t
end

function M.new(opts)
    opts = opts or {}
    local t = opts.type and M.TYPES[string.lower(opts.type)]
    if opts.type and not t then
        error("norflash: unknown type: " .. tostring(opts.type), 2)
    end
    t = t or {}

    local f = setmetatable({}, FLASH)
    f.id = opts.id or t.id
    f.size = opts.size or t.size
    f.addr4 = opts.addr4 or t.addr4 or false
    local sfdp = opts.sfdp
    if sfdp == nil then
        sfdp = t.sfdp
    end
    f.sfdp = sfdp and sfdp_table(f.size) or nil
    f.page_time = opts.page_time or 0.5
    f.erase_time = opts.erase_time or 3
    f.mode = { [0] = true, [3] = true }

    assert(type(f.id) == "string" and #f.id == 3, "norflash: invalid JEDEC ID")
    assert(math.type(f.size) == "integer" and f.size % SECTOR == 0, "norflash: invalid size")

    f.mem = {}
    f.busy_until = 0
    f.wel = false
    local protected = opts.protected
    if protected == nil then
        protected = t.protected
    end
    f.bp = protected and 0x1C or 0
    f.deep = opts.power_down or false
    f.erases, f.programs = 0, 0
    f.state = "idle"
    return f
end

function FLASH:read(addr, n)
    local res = {}
    while n > 0 do
        local s = addr // SECTOR
        local off = addr % SECTOR
        local m = math.min(n, SECTOR - off)
        res[#res + 1] = (self.mem[s] or ERASED):sub(off + 1, off + m)
        addr = (addr + m) % self.size
        n = n - m
    end
    return table.concat(res)
end

-- program data (AND with memory content), data in one sector
function FLASH:program(addr, data)
    local s = addr // SECTOR
    local off = addr % SECTOR
    local old = self.mem[s] or ERASED
    local b = { old:byte(off + 1, off + #data) }
    for i = 1, #data do
        b[i] = b[i] & data:byte(i)
    end
    self.mem[s] = old:sub(1, off) .. string.char(table.unpack(b)) .. old:sub(off + #data + 1)
end

function FLASH:write(addr, data)
    local i = 1
    while i <= #data do
        local m = math.min(#data - i + 1, SECTOR - addr % SECTOR)
        self:program(addr, data:sub(i, i + m - 1))
        addr = addr + m
        i = i + m
    end
end

function FLASH:erase_all()
    self.mem = {}
end

function FLASH:stats()
    return self.erases, self.programs
end

function FLASH:busy()
    return host.time_ms() < self.busy_until
end

function FLASH:status()
    return self.bp | (self.wel and 2 or 0) | (self:busy() and 1 or 0)
end

-- SPI interface
local ADDR_CMDS = {
    [0x03] = 3, [0x13] = 4, [0x0B] = 3, [0x0C] = 4, [0x02] = 3, [0x12] = 4,
    [0x20] = 3, [0x21] = 4, [0xD8] = 3, [0x5A] = 3,
}
local DUMMY_CMDS = { [0x0B] = 1, [0x0C] = 1, [0x5A] = 1 }

function FLASH:select()
    self.state = "cmd"
    self.cmd = nil
end

function FLASH:transfer(byte)
    local state = self.state

    if state == "cmd" then
        self.cmd = byte
        local nbytes = ADDR_CMDS[byte]
        if self.deep and byte ~= 0xAB then
            self.state = "ignore"
        elseif self:busy() and byte ~= 0x05 then
            self.state = "ignore"
        elseif byte == 0x05 then
            self.state = "rdsr"
        elseif byte == 0x9F then
            self.state, self.ptr = "rdid", 0
        elseif nbytes and (nbytes == 3 or self.addr4) and (byte ~= 0x5A or self.sfdp) then
            self.state = "addr"
            self.addr, self.addr_cnt, self.addr_len = 0, 0, nbytes
            self.dummy = DUMMY_CMDS[byte] or 0
        elseif byte == 0x01 then
            self.state = "wrsr"
        else
            self.state = "ignore" -- executed at CS rise or unknown
        end
        return 0xFF
    elseif state == "rdsr" then
        return self:status()
    elseif state == "rdid" then
        self.ptr = self.ptr + 1
        return self.id:byte(self.ptr) or 0x00
    elseif state == "addr" then
        if self.addr_cnt < self.addr_len then
            self.addr = (self.addr << 8) | byte
            self.addr_cnt = self.addr_cnt + 1
        else
            self.dummy = self.dummy - 1
        end
        if self.addr_cnt == self.addr_len and self.dummy == 0 then
            local cmd = self.cmd
            if cmd == 0x5A then
                self.state, self.ptr = "sfdp", self.addr
            elseif cmd == 0x02 or cmd == 0x12 then
                self.state = "pp"
                self.ptr = self.addr % self.size
                self.data = {}
            elseif cmd == 0x20 or cmd == 0x21 or cmd == 0xD8 then
                self.state = "erase"
            else
                self.state = "read"
                self.ptr = self.addr % self.size
            end
        end
        return 0xFF
    elseif state == "read" then
        local v = self:read(self.ptr, 1):byte()
        self.ptr = (self.ptr + 1) % self.size
        return v
    elseif state == "sfdp" then
        self.ptr = self.ptr + 1
        return self.sfdp:byte(self.ptr) or 0xFF
    elseif state == "pp" then
        self.data[#self.data + 1] = string.char(byte)
        return 0xFF
    elseif state == "wrsr" then
        self.new_sr = byte
        self.state = "ignore"
        return 0xFF
    end

    return 0xFF
end

-- whole SPI transfer (board engine): data of READ and PP in bulk
function FLASH:transfer_block(data)
    local n, i, res = #data, 1, {}
    while i <= n do
        local state = self.state
        if state == "read" then
            res[#res + 1] = self:read(self.ptr, n - i + 1)
            self.ptr = (self.ptr + n - i + 1) % self.size
            break
        elseif state == "pp" then
            self.data[#self.data + 1] = data:sub(i)
            res[#res + 1] = string.rep("\xFF", n - i + 1)
            break
        end
        res[#res + 1] = string.char((self:transfer(data:byte(i)) or 0xFF) & 0xFF)
        i = i + 1
    end
    return table.concat(res)
end

function FLASH:protected(addr)
    return self.bp & 0x1C ~= 0 -- whole memory protected by any BP bit
end

function FLASH:deselect()
    local cmd, state = self.cmd, self.state
    self.cmd = nil
    self.state = "idle"

    if state == "idle" or self:busy() then
        return
    end

    if self.deep then
        if cmd == 0xAB then
            self.deep = false
        end
        return
    end

    local now = host.time_ms()
    if cmd == 0x06 then
        self.wel = true
    elseif cmd == 0x04 then
        self.wel = false
    elseif cmd == 0xB9 then
        self.deep = true
    elseif cmd == 0x98 and self.wel then
        self.bp = 0
        self.wel = false
    elseif (cmd == 0x02 or cmd == 0x12) and state == "pp" and self.wel then
        local d = table.concat(self.data)
        if #d > PAGE then
            d = d:sub(-PAGE) -- only last 256 bytes are programmed
        end
        if not self:protected(self.ptr) and #d > 0 then
            local base = self.ptr - self.ptr % PAGE
            local off = self.ptr % PAGE
            local page = self:read(base, PAGE)
            local first = d:sub(1, PAGE - off)
            local wrap = d:sub(PAGE - off + 1)
            local buf = page:sub(1, off) .. first .. page:sub(off + #first + 1)
            if #wrap > 0 then
                buf = wrap .. buf:sub(#wrap + 1)
            end
            -- buf: new bytes merged, program only changed area (AND)
            self:program(base, buf)
            self.programs = self.programs + 1
        end
        self.busy_until = now + self.page_time
        self.wel = false
    elseif (cmd == 0x20 or cmd == 0x21 or cmd == 0xD8) and state == "erase" and self.wel then
        local size = (cmd == 0xD8) and 65536 or SECTOR
        local base = (self.addr % self.size) - (self.addr % self.size) % size
        if not self:protected(base) then
            for s = base // SECTOR, (base + size) // SECTOR - 1 do
                self.mem[s] = nil
            end
            self.erases = self.erases + 1
        end
        self.busy_until = now + self.erase_time * size / SECTOR
        self.wel = false
    elseif (cmd == 0xC7 or cmd == 0x60) and self.wel then
        if not self:protected(0) then
            self.mem = {}
        end
        self.busy_until = now + self.erase_time * 10
        self.wel = false
    elseif cmd == 0x01 and self.new_sr and self.wel then
        self.bp = self.new_sr & 0x9C
        self.busy_until = now + 1
        self.wel = false
    end

    self.new_sr = nil
    self.data = nil
end

function FLASH:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
