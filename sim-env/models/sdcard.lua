--[[----------------------------------------------------------------------------
  SD card model (SPI mode), SPI mode 0, MSB first.

  Usage:
    local sdcard = require("models.sdcard")
    local sd = sdcard.new{file = "sd.img"}      -- host image file
    sd:attach("SPI1", "PA15")                    -- bus, CS pin (active low)

  Options of new():
    type        "sdhc" (default, SD v2, block addressing), "sdsc" (SD v2,
                byte addressing) or "sdv1" (SD v1, byte addressing, CMD8 is
                not supported)
    file        host image file: card size is the file size (the file is
                created with the size option if not exists); without the file
                the card content is kept in memory
    size        card size in bytes (default 16 MiB), multiple of 512 KiB
                (sdhc) or 256 KiB (sdsc, sdv1, max 1 GiB)
    init_delay  number of ACMD41 commands which return busy (default 2)

  Supported commands: CMD0, CMD8, CMD9 (CSD), CMD10 (CID), CMD12, CMD13,
  CMD16 (512 B only), CMD17, CMD18, CMD24, CMD25, CMD55, CMD58, ACMD23,
  ACMD41. CRC is not checked. Other commands: illegal command (R1 bit 2).

  Methods:
    sd:attach(bus, cs)          connect to bus
    sd:read_sector(n)           read sector (512 B string, host side)
    sd:write_sector(n, data)    write sector (host side)
    sd:close()                  close image file
------------------------------------------------------------------------------]]

local M = {}

local SECTOR = 512
local ZERO = string.rep("\0", SECTOR)

local R1_IDLE      = 0x01
local R1_ILLEGAL   = 0x04
local R1_ADDR_ERR  = 0x20
local R1_PARAM_ERR = 0x40

local SD = {}
SD.__index = SD

function M.new(opts)
    opts = opts or {}
    local sd = setmetatable({}, SD)
    sd.type = opts.type or "sdhc"
    sd.mode = 0
    sd.init_delay = opts.init_delay or 2

    if sd.type ~= "sdhc" and sd.type ~= "sdsc" and sd.type ~= "sdv1" then
        error("sdcard: unknown type: " .. tostring(sd.type), 2)
    end

    local unit = (sd.type == "sdhc") and (512 * 1024) or (256 * 1024)
    local size = opts.size or (16 * 1024 * 1024)

    if opts.file then
        local f = io.open(opts.file, "r+b")
        if not f then
            f = assert(io.open(opts.file, "w+b"))
            f:seek("set", size - 1)
            f:write("\0")
        end
        size = f:seek("end")
        sd.f = f
        host.on_exit(function() sd:close() end)
    else
        sd.sectors = {}
    end

    size = size - (size % unit)
    if size == 0 or (sd.type ~= "sdhc" and size > 1024 * 1024 * 1024) then
        error("sdcard: invalid card size: " .. tostring(size), 2)
    end

    sd.size = size
    sd.blocks = size // SECTOR
    sd.csd = sd:make_csd(unit)
    sd.cid = "\x03SDDNXSD\x10\x12\x34\x56\x78\x01\x9A\x01"

    sd:reset()
    return sd
end

function SD:reset()
    self.idle = true
    self.app = false
    self.init_cnt = 0
    self.out = {}
    self.out_pos = 1
    self.cmdbuf = nil
    self.state = "cmd"
    self.multi_read = nil
    self.multi_write = false
end

function SD:make_csd(unit)
    local c = {}
    for i = 0, 15 do c[i] = 0 end
    if self.type == "sdhc" then
        local csize = self.size // unit - 1
        c[0] = 0x40                      -- CSD v2
        c[1], c[2], c[3] = 0x0E, 0x00, 0x32
        c[4], c[5], c[6] = 0x5B, 0x59, 0x00
        c[7] = (csize >> 16) & 0x3F
        c[8] = (csize >> 8) & 0xFF
        c[9] = csize & 0xFF
        c[10], c[11], c[12], c[13], c[14] = 0x7F, 0x80, 0x0A, 0x40, 0x00
    else
        -- CSD v1: READ_BL_LEN = 9, C_SIZE_MULT = 7: unit 256 KiB
        local csize = self.size // unit - 1
        c[0] = 0x00
        c[1], c[2], c[3] = 0x26, 0x00, 0x32
        c[4] = 0x5F
        c[5] = 0x50 | 9                  -- CCC, READ_BL_LEN
        c[6] = 0x80 | ((csize >> 10) & 3)
        c[7] = (csize >> 2) & 0xFF
        c[8] = ((csize & 3) << 6) | 0x2D
        c[9] = 0xB4 | 3                  -- C_SIZE_MULT[2:1]
        c[10] = 0x80 | 0x7F              -- C_SIZE_MULT[0]
        c[11], c[12], c[13], c[14] = 0x80, 0x0A, 0x40, 0x00
    end
    c[15] = 0x01
    local t = {}
    for i = 0, 15 do t[#t + 1] = string.char(c[i]) end
    return table.concat(t)
end

function SD:read_sector(n)
    if self.f then
        self.f:seek("set", n * SECTOR)
        local data = self.f:read(SECTOR) or ""
        if #data < SECTOR then data = data .. string.rep("\0", SECTOR - #data) end
        return data
    end
    return self.sectors[n] or ZERO
end

function SD:write_sector(n, data)
    assert(#data == SECTOR, "sdcard: invalid sector size")
    if self.f then
        self.f:seek("set", n * SECTOR)
        self.f:write(data)
        self.f:flush()
    else
        self.sectors[n] = data
    end
end

function SD:close()
    if self.f then
        self.f:close()
        self.f = nil
    end
end

-- output queue (bytes sent to system)
function SD:push(s)
    self.out[#self.out + 1] = s
end

function SD:pop()
    local s = self.out[1]
    if not s then return 0xFF end
    local v = s:byte(self.out_pos)
    self.out_pos = self.out_pos + 1
    if self.out_pos > #s then
        table.remove(self.out, 1)
        self.out_pos = 1
    end
    return v
end

function SD:clear_out()
    self.out = {}
    self.out_pos = 1
end

-- response after one byte delay (NCR)
function SD:respond(...)
    self:push("\xFF" .. string.char(...))
end

function SD:block_addr(arg)
    if self.type == "sdhc" then return arg end
    return arg // SECTOR
end

function SD:data_block(n)
    self:push("\xFF\xFE" .. self:read_sector(n) .. "\xFF\xFF")
end

function SD:command(cmd, arg)
    local app = self.app
    self.app = false
    local r1 = self.idle and R1_IDLE or 0

    if app and cmd == 41 then                                   -- ACMD41
        if self.init_cnt < self.init_delay then
            self.init_cnt = self.init_cnt + 1
            self:respond(R1_IDLE)
        else
            self.idle = false
            self:respond(0)
        end
    elseif app and cmd == 23 then                               -- ACMD23
        self:respond(r1)
    elseif cmd == 0 then                                        -- GO_IDLE_STATE
        self:reset()
        self:respond(R1_IDLE)
    elseif cmd == 8 then                                        -- SEND_IF_COND
        if self.type == "sdv1" then
            self:respond(r1 | R1_ILLEGAL)
        else
            self:respond(r1, 0x00, 0x00, (arg >> 8) & 0x0F, arg & 0xFF)
        end
    elseif cmd == 55 then                                       -- APP_CMD
        self.app = true
        self:respond(r1)
    elseif cmd == 58 then                                       -- READ_OCR
        local ocr0 = (self.idle and 0 or 0x80) | ((self.type == "sdhc") and 0x40 or 0)
        self:respond(r1, ocr0, 0xFF, 0x80, 0x00)
    elseif self.idle then
        self:respond(r1 | R1_ILLEGAL)
    elseif cmd == 9 or cmd == 10 then                           -- SEND_CSD, SEND_CID
        self:respond(0)
        self:push("\xFF\xFE" .. (cmd == 9 and self.csd or self.cid) .. "\xFF\xFF")
    elseif cmd == 12 then                                       -- STOP_TRANSMISSION
        self.multi_read = nil
        self:clear_out()
        self:push("\xFF\x00\x00\x00")
    elseif cmd == 13 then                                       -- SEND_STATUS
        self:respond(0, 0)
    elseif cmd == 16 then                                       -- SET_BLOCKLEN
        self:respond(arg == SECTOR and 0 or R1_PARAM_ERR)
    elseif cmd == 17 or cmd == 18 or cmd == 24 or cmd == 25 then
        local n = self:block_addr(arg)
        if n >= self.blocks or (self.type ~= "sdhc" and arg % SECTOR ~= 0) then
            self:respond(R1_ADDR_ERR)
        elseif cmd == 17 then                                   -- READ_SINGLE_BLOCK
            self:respond(0)
            self:data_block(n)
        elseif cmd == 18 then                                   -- READ_MULTIPLE_BLOCK
            self:respond(0)
            self:data_block(n)
            self.multi_read = n + 1
        else                                                    -- WRITE_BLOCK, WRITE_MULTIPLE_BLOCK
            self:respond(0)
            self.wr_block = n
            self.multi_write = (cmd == 25)
            self.state = "wr_wait"
        end
    else
        self:respond(r1 | R1_ILLEGAL)
    end
end

function SD:receive(byte)
    local state = self.state

    if state == "cmd" then
        if self.cmdbuf then
            local b = self.cmdbuf
            b[#b + 1] = byte
            if #b == 6 then
                self.cmdbuf = nil
                self:command(b[1] & 0x3F, (b[2] << 24) | (b[3] << 16) | (b[4] << 8) | b[5])
            end
        elseif byte & 0xC0 == 0x40 then
            self.cmdbuf = {byte}
        end

    elseif state == "wr_wait" then
        if byte == 0xFE or (self.multi_write and byte == 0xFC) then
            self.state = "wr_data"
            self.wr_data = {}
        elseif self.multi_write and byte == 0xFD then               -- stop token
            self.multi_write = false
            self.state = "cmd"
            self:push("\xFF\x00\x00")
        end

    elseif state == "wr_data" then
        local d = self.wr_data
        d[#d + 1] = string.char(byte)
        if #d == SECTOR + 2 then                                    -- data + CRC
            if self.wr_block < self.blocks then
                self:write_sector(self.wr_block, table.concat(d, "", 1, SECTOR))
                self:push("\xE5\x00\x00")                           -- data accepted, busy
            else
                self:push("\xED\x00")                               -- write error
            end
            self.wr_block = self.wr_block + 1
            self.wr_data = nil
            self.state = self.multi_write and "wr_wait" or "cmd"
        end
    end
end

-- SPI interface
function SD:transfer(byte)
    if self.multi_read and #self.out == 0 then
        if self.multi_read < self.blocks then
            self:data_block(self.multi_read)
            self.multi_read = self.multi_read + 1
        end
    end

    local r = self:pop()
    self:receive(byte)
    return r
end

function SD:select()
end

function SD:deselect()
    self:clear_out()
    self.cmdbuf = nil
    self.multi_read = nil
    if self.state == "wr_data" then
        self.state = "cmd"
    end
end

function SD:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
