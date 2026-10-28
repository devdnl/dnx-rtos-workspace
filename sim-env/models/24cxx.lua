--[[----------------------------------------------------------------------------
  24Cxx I2C EEPROM model (24C01 .. 24C512, 24M01, M24M02).

  Usage:
    local eeprom = require("models.24cxx")
    local ee = eeprom.new{type = "24c256"}         -- 32 KiB, 64 B page
    ee:attach("I2C1", 0x50)                         -- 7-bit base address

  Options of new():
    type        memory type (table TYPES), or explicit parameters:
    size        memory size in bytes
    page        page size in bytes
    addr_bytes  number of address bytes (1 or 2)
    write_time  write cycle time in ms (default 5), the device does not
                acknowledge its address during write cycle (as real device)
    file        host file with memory content: loaded at start (if exists),
                saved after writes and at simulator exit
    fill        initial value of erased memory (default 0xFF)

  Memory bigger than address range (e.g. 24C16, M24M02) occupies several bus
  addresses (block bits in device address), base address must be aligned.

  Methods:
    ee:attach(bus, address)     connect to bus
    ee:read_byte(addr)          read memory (host side, e.g. in tests)
    ee:write_byte(addr, value)  write memory (host side)
    ee:fill(value)              fill memory
    ee:save([file])             save memory to file
------------------------------------------------------------------------------]]

local M = {}

M.TYPES = {
    ["24c01"]   = {size = 128,    page = 8,   addr_bytes = 1},
    ["24c02"]   = {size = 256,    page = 8,   addr_bytes = 1},
    ["24c04"]   = {size = 512,    page = 16,  addr_bytes = 1},
    ["24c08"]   = {size = 1024,   page = 16,  addr_bytes = 1},
    ["24c16"]   = {size = 2048,   page = 16,  addr_bytes = 1},
    ["24c32"]   = {size = 4096,   page = 32,  addr_bytes = 2},
    ["24c64"]   = {size = 8192,   page = 32,  addr_bytes = 2},
    ["24c128"]  = {size = 16384,  page = 64,  addr_bytes = 2},
    ["24c256"]  = {size = 32768,  page = 64,  addr_bytes = 2},
    ["24c512"]  = {size = 65536,  page = 128, addr_bytes = 2},
    ["24m01"]   = {size = 131072, page = 256, addr_bytes = 2},
    ["m24m02"]  = {size = 262144, page = 256, addr_bytes = 2},
}

local EEPROM = {}
EEPROM.__index = EEPROM

function M.new(opts)
    opts = opts or {}
    local t = opts.type and M.TYPES[string.lower(opts.type)]
    if opts.type and not t then error("24cxx: unknown type: " .. tostring(opts.type), 2) end
    t = t or {}

    local ee = setmetatable({}, EEPROM)
    ee.size       = opts.size or t.size
    ee.page       = opts.page or t.page
    ee.addr_bytes = opts.addr_bytes or t.addr_bytes
    ee.write_time = opts.write_time or 5
    ee.name       = opts.name or opts.type or "24cxx"

    assert(math.type(ee.size) == "integer" and ee.size > 0, "24cxx: invalid size")
    assert(math.type(ee.page) == "integer" and ee.page > 0, "24cxx: invalid page size")
    assert(ee.addr_bytes == 1 or ee.addr_bytes == 2, "24cxx: invalid number of address bytes")

    local range = 1 << (8 * ee.addr_bytes)
    ee.blocks = math.max(1, ee.size // range)
    ee.block_size = math.min(ee.size, range)

    ee.mem = {}
    ee:fill(opts.fill or 0xFF)
    ee.ptr = 0
    ee.busy_until = 0

    if opts.file then
        ee.file = opts.file
        ee:load(ee.file)
        host.on_exit(function() ee:flush() end)
    end

    return ee
end

function EEPROM:fill(value)
    for i = 0, self.size - 1 do self.mem[i] = value & 0xFF end
    self:mark_dirty()
end

function EEPROM:read_byte(addr)
    return self.mem[addr % self.size]
end

function EEPROM:write_byte(addr, value)
    self.mem[addr % self.size] = value & 0xFF
    self:mark_dirty()
end

function EEPROM:load(file)
    local f = io.open(file, "rb")
    if not f then return false end
    local data = f:read("a")
    f:close()
    for i = 1, math.min(#data, self.size) do self.mem[i - 1] = data:byte(i) end
    return true
end

function EEPROM:save(file)
    file = file or self.file
    local f = assert(io.open(file, "wb"))
    local chunk = {}
    for i = 0, self.size - 1 do
        chunk[#chunk + 1] = string.char(self.mem[i])
        if #chunk == 4096 then
            f:write(table.concat(chunk))
            chunk = {}
        end
    end
    f:write(table.concat(chunk))
    f:close()
end

-- content is saved with delay (many write cycles are saved once)
function EEPROM:mark_dirty()
    if not self.file or self.dirty then return end
    self.dirty = true
    host.timer(500, function() self:flush() end)
end

function EEPROM:flush()
    if self.dirty then
        self.dirty = false
        self:save()
    end
end

-- bus interface of one block (bus address)
local function block_device(ee, block)
    local dev = {}

    function dev:select(read)
        if host.time_ms() < ee.busy_until then
            return false -- write cycle in progress
        end

        if not read then
            self.addr = 0
            self.addr_cnt = 0
            self.data = nil
        end

        return true
    end

    function dev:write(byte)
        if self.addr_cnt < ee.addr_bytes then
            self.addr = (self.addr << 8) | byte
            self.addr_cnt = self.addr_cnt + 1
            if self.addr_cnt == ee.addr_bytes then
                ee.ptr = (block * ee.block_size + (self.addr % ee.block_size)) % ee.size
                self.page_base = ee.ptr - (ee.ptr % ee.page)
                self.offset = ee.ptr % ee.page
                self.data = {}
                self.count = 0
            end
        else
            -- page buffer: address rolls over inside page
            self.data[self.offset] = byte
            self.offset = (self.offset + 1) % ee.page
            self.count = self.count + 1
        end
        return true
    end

    function dev:read()
        local v = ee.mem[ee.ptr]
        ee.ptr = (ee.ptr + 1) % ee.size
        return v
    end

    function dev:stop()
        if self.data and self.count > 0 then
            for off, v in pairs(self.data) do
                ee.mem[self.page_base + off] = v
            end
            ee.ptr = self.page_base + self.offset
            ee.busy_until = host.time_ms() + ee.write_time
            ee:mark_dirty()
        end
        self.data = nil
    end

    return dev
end

function EEPROM:attach(bus, address)
    if address % self.blocks ~= 0 then
        error(string.format("24cxx: address 0x%02X is not aligned to %u blocks", address, self.blocks), 2)
    end

    for b = 0, self.blocks - 1 do
        i2c.attach(bus, address + b, block_device(self, b))
    end

    return self
end

return M
