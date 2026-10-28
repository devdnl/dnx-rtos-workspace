--[[----------------------------------------------------------------------------
  25xx SPI EEPROM model (25AA010 .. 25LC1024), SPI mode 0 or 3, MSB first.

  Usage:
    local eeprom = require("models.25xx")
    local ee = eeprom.new{type = "25lc256"}     -- 32 KiB, 64 B page
    ee:attach("SPI1", "PA7")                    -- bus, CS pin (active low)

  Options of new():
    type        memory type (table TYPES), or explicit parameters:
    size        memory size in bytes
    page        page size in bytes
    addr_bytes  number of address bytes (1, 2 or 3)
    write_time  write cycle time in ms (default 5), during write cycle only
                RDSR instruction is accepted (status WIP = 1)
    file        host file with memory content: loaded at start (if exists),
                saved after writes and at simulator exit
    fill        initial value of erased memory (default 0xFF)

  Instructions: READ (0x03), WRITE (0x02, page write with roll-over in page,
  write enable latch required), WREN (0x06), WRDI (0x04), RDSR (0x05),
  WRSR (0x01, BP and WPEN bits are stored, protection is not simulated).

  Methods:
    ee:attach(bus, cs)          connect to bus
    ee:read_byte(addr)          read memory (host side, e.g. in tests)
    ee:write_byte(addr, value)  write memory (host side)
    ee:fill(value)              fill memory
    ee:save([file])             save memory to file
------------------------------------------------------------------------------]]

local M = {}

M.TYPES = {
    ["25aa010"]  = {size = 128,    page = 16,  addr_bytes = 1},
    ["25aa020"]  = {size = 256,    page = 16,  addr_bytes = 1},
    ["25lc080"]  = {size = 1024,   page = 16,  addr_bytes = 2},
    ["25lc160"]  = {size = 2048,   page = 16,  addr_bytes = 2},
    ["25lc320"]  = {size = 4096,   page = 32,  addr_bytes = 2},
    ["25lc640"]  = {size = 8192,   page = 32,  addr_bytes = 2},
    ["25lc128"]  = {size = 16384,  page = 64,  addr_bytes = 2},
    ["25lc256"]  = {size = 32768,  page = 64,  addr_bytes = 2},
    ["25lc512"]  = {size = 65536,  page = 128, addr_bytes = 2},
    ["25lc1024"] = {size = 131072, page = 256, addr_bytes = 3},
}

local CMD_WRSR, CMD_WRITE, CMD_READ, CMD_WRDI, CMD_RDSR, CMD_WREN = 0x01, 0x02, 0x03, 0x04, 0x05, 0x06

local EEPROM = {}
EEPROM.__index = EEPROM

function M.new(opts)
    opts = opts or {}
    local t = opts.type and M.TYPES[string.lower(opts.type)]
    if opts.type and not t then error("25xx: unknown type: " .. tostring(opts.type), 2) end
    t = t or {}

    local ee = setmetatable({}, EEPROM)
    ee.size       = opts.size or t.size
    ee.page       = opts.page or t.page
    ee.addr_bytes = opts.addr_bytes or t.addr_bytes
    ee.write_time = opts.write_time or 5
    ee.mode       = {[0] = true, [3] = true}

    assert(math.type(ee.size) == "integer" and ee.size > 0, "25xx: invalid size")
    assert(math.type(ee.page) == "integer" and ee.page > 0, "25xx: invalid page size")
    assert(ee.addr_bytes and ee.addr_bytes >= 1 and ee.addr_bytes <= 3, "25xx: invalid number of address bytes")

    ee.mem = {}
    ee:fill(opts.fill or 0xFF)
    ee.busy_until = 0
    ee.wel = false
    ee.sr_bits = 0  -- BP1, BP0, WPEN
    ee.state = "idle"

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
    local f = assert(io.open(file or self.file, "wb"))
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

function EEPROM:busy()
    return host.time_ms() < self.busy_until
end

function EEPROM:status()
    return self.sr_bits | (self.wel and 2 or 0) | (self:busy() and 1 or 0)
end

-- SPI interface
function EEPROM:select()
    self.state = "cmd"
    self.cmd = nil
end

function EEPROM:transfer(byte)
    local state = self.state

    if state == "cmd" then
        self.cmd = byte
        if self:busy() and byte ~= CMD_RDSR then
            self.state = "ignore"
        elseif byte == CMD_RDSR then
            self.state = "rdsr"
        elseif byte == CMD_READ or byte == CMD_WRITE then
            self.state = "addr"
            self.addr, self.addr_cnt = 0, 0
        elseif byte == CMD_WRSR then
            self.state = "wrsr"
        else
            self.state = "ignore" -- WREN, WRDI (executed at CS rise), unknown
        end
        return 0xFF

    elseif state == "rdsr" then
        return self:status()

    elseif state == "addr" then
        self.addr = (self.addr << 8) | byte
        self.addr_cnt = self.addr_cnt + 1
        if self.addr_cnt == self.addr_bytes then
            self.ptr = self.addr % self.size
            if self.cmd == CMD_READ then
                self.state = "read"
            else
                self.state = "write"
                self.page_base = self.ptr - (self.ptr % self.page)
                self.offset = self.ptr % self.page
                self.data = {}
                self.count = 0
            end
        end
        return 0xFF

    elseif state == "read" then
        local v = self.mem[self.ptr]
        self.ptr = (self.ptr + 1) % self.size
        return v

    elseif state == "write" then
        self.data[self.offset] = byte
        self.offset = (self.offset + 1) % self.page
        self.count = self.count + 1
        return 0xFF

    elseif state == "wrsr" then
        self.new_sr = byte & 0x8C
        self.state = "ignore"
        return 0xFF
    end

    return 0xFF
end

function EEPROM:deselect()
    local cmd = self.cmd
    self.cmd = nil

    if self.state == "idle" or self:busy() then
        self.state = "idle"
        return
    end

    if cmd == CMD_WREN then
        self.wel = true
    elseif cmd == CMD_WRDI then
        self.wel = false
    elseif cmd == CMD_WRITE and self.state == "write" and self.count > 0 and self.wel then
        for off, v in pairs(self.data) do self.mem[self.page_base + off] = v end
        self.busy_until = host.time_ms() + self.write_time
        self.wel = false
        self:mark_dirty()
    elseif cmd == CMD_WRSR and self.new_sr and self.wel then
        self.sr_bits = self.new_sr
        self.busy_until = host.time_ms() + self.write_time
        self.wel = false
    end

    self.new_sr = nil
    self.data = nil
    self.state = "idle"
end

function EEPROM:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
