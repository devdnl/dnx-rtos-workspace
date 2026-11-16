--[[----------------------------------------------------------------------------
  W5500 Ethernet controller model (WIZnet), SPI mode 0 or 3, variable data
  length mode. Only the MACRAW mode of socket 0 is modeled (hardware TCP/IP
  stack is not): the chip is connected to the Ethernet of the simulator
  (eth.attach()) as other device models, frames sent by SEND command go to
  the network, frames received from the network are stored in the RX memory
  of socket 0 (2-byte length header, MAC filter MFEN: own MAC address,
  broadcast, multicast if MMB = 0).

  Usage:
    local w5500 = require("models.w5500")
    local chip = w5500.new{reset = "PB10"}:attach("SPI:0", "PB9")
    chip:set_link(false)                       -- cable disconnected

  Options of new():
    reset       GPIO pin connected to RSTn (low: chip in reset), optional
    link        link state at start (default true, 100 Mbps full duplex)
    segment     Ethernet segment (default "lan", see eth.attach())

  Registers: common (MR with software reset, SHAR, PHYCFGR with link state,
  speed and duplex of configured mode, VERSIONR = 0x04), socket registers
  (all 8 sockets are stored, buffer sizes Sn_RXBUF_SIZE/Sn_TXBUF_SIZE),
  socket 0 commands OPEN, CLOSE, SEND (SEND_OK in Sn_IR), RECV, Sn_TX_FSR,
  Sn_RX_RSR, pointers (RX/TX memory address wraps in the socket buffer).

  Methods:
    chip:attach(bus, cs)        connect to SPI bus and Ethernet
    chip:set_link(on)           link state
    chip:stats()                frames sent, received, dropped (RX memory
                                full or MAC filter)
------------------------------------------------------------------------------]]

local M = {}

local CHIP = {}
CHIP.__index = CHIP

local MEM = 16384

function M.new(opts)
    opts = opts or {}
    local c = setmetatable({}, CHIP)
    c.mode = { [0] = true, [3] = true }
    c.link = opts.link ~= false
    c.segment = opts.segment
    c.tx_frames, c.rx_frames, c.dropped = 0, 0, 0
    c:reset()
    if opts.reset then
        c.reset_pin = opts.reset
        gpio.on_change(opts.reset, function(level)
            c.in_reset = not level
            if not level then
                c:reset()
            end
        end)
    end
    return c
end

function CHIP:reset()
    self.common = {}
    for i = 0, 0x39 do
        self.common[i] = 0
    end
    self.common[0x2E] = 0xB8 -- PHYCFGR: RST = 1, OPMD = 0, OPMDC = 111
    self.common[0x39] = 0x04
    self.sock = {}
    for s = 0, 7 do
        local r = {}
        for i = 0, 0x2F do
            r[i] = 0
        end
        r[0x1E], r[0x1F] = 2, 2 -- 2 KiB RX/TX buffers
        r[0x20], r[0x21] = 0x08, 0x00 -- TX_FSR = 2048
        self.sock[s] = r
    end
    self.tx = {}
    self.rx = {}
    self.rx_rd_commit = 0
    self.open = false
end

function CHIP:set_link(on)
    self.link = on
end

function CHIP:stats()
    return self.tx_frames, self.rx_frames, self.dropped
end

-- socket registers ------------------------------------------------------------
local function get16(r, a)
    return (r[a] << 8) | r[a + 1]
end

local function set16(r, a, v)
    r[a] = (v >> 8) & 0xFF
    r[a + 1] = v & 0xFF
end

function CHIP:buf_size(dir)
    local kb = self.sock[0][dir == "rx" and 0x1E or 0x1F]
    return kb * 1024
end

function CHIP:update_sizes()
    local r = self.sock[0]
    local txsize, rxsize = self:buf_size("tx"), self:buf_size("rx")
    set16(r, 0x20, txsize - ((get16(r, 0x24) - get16(r, 0x22)) & 0xFFFF)) -- TX_FSR
    set16(r, 0x26, (get16(r, 0x2A) - self.rx_rd_commit) & 0xFFFF) -- RX_RSR
    if txsize == 0 then
        set16(r, 0x20, 0)
    end
    if rxsize == 0 then
        set16(r, 0x26, 0)
    end
end

function CHIP:phycfgr()
    local v = self.common[0x2E] & 0xF8
    local opmdc = (v >> 3) & 7
    local spd, dpx = 1, 1
    if v & 0x40 ~= 0 and opmdc < 4 then -- forced mode
        spd, dpx = (opmdc >> 1) & 1, opmdc & 1
    elseif v & 0x40 ~= 0 and opmdc == 4 then
        spd, dpx = 1, 0
    end
    if not self.link then
        return v
    end
    return v | (dpx << 2) | (spd << 1) | 1
end

function CHIP:command(cmd)
    local r = self.sock[0]
    if cmd == 0x01 then -- OPEN
        if r[0x00] & 0x0F == 0x04 then
            r[0x03] = 0x42 -- SOCK_MACRAW
            self.open = true
            set16(r, 0x22, 0)
            set16(r, 0x24, 0)
            set16(r, 0x28, 0)
            set16(r, 0x2A, 0)
            self.rx_rd_commit = 0
        end
    elseif cmd == 0x10 then -- CLOSE
        r[0x03] = 0x00
        self.open = false
    elseif cmd == 0x20 and self.open then -- SEND
        local size = self:buf_size("tx")
        local rd, wr = get16(r, 0x22), get16(r, 0x24)
        local n = (wr - rd) & 0xFFFF
        local t = {}
        for i = 0, n - 1 do
            t[#t + 1] = string.char(self.tx[(rd + i) % size] or 0)
        end
        local frame = table.concat(t)
        if #frame >= 14 and #frame <= 1514 and self.link then
            eth.send(frame, self)
            self.tx_frames = self.tx_frames + 1
        end
        set16(r, 0x22, wr)
        r[0x02] = r[0x02] | 0x10 -- SEND_OK
    elseif cmd == 0x40 and self.open then -- RECV
        self.rx_rd_commit = get16(r, 0x28)
    end
    r[0x01] = 0
    self:update_sizes()
end

-- Ethernet ---------------------------------------------------------------------
function CHIP:receive(frame)
    if not self.open or self.in_reset or not self.link then
        return
    end
    local r = self.sock[0]
    local dst = frame:sub(1, 6)
    local mac = string.char(table.unpack(self.common, 0x09, 0x0E))
    local bcast = dst == "\xFF\xFF\xFF\xFF\xFF\xFF"
    local mcast = dst:byte(1) & 1 == 1
    if r[0x00] & 0x80 ~= 0 then -- MFEN
        if not (dst == mac or bcast or (mcast and r[0x00] & 0x20 == 0)) then
            return
        end
    end
    if bcast and r[0x00] & 0x40 ~= 0 then -- BCASTB
        return
    end
    self:update_sizes()
    local size = self:buf_size("rx")
    local used = get16(r, 0x26)
    local len = #frame + 2
    if used + len > size then
        self.dropped = self.dropped + 1
        return
    end
    local wr = get16(r, 0x2A)
    local data = string.char(len >> 8, len & 0xFF) .. frame
    for i = 1, #data do
        self.rx[(wr + i - 1) % size] = data:byte(i)
    end
    set16(r, 0x2A, (wr + len) & 0xFFFF)
    r[0x02] = r[0x02] | 0x04 -- RECV
    self.rx_frames = self.rx_frames + 1
    self:update_sizes()
end

-- SPI interface ----------------------------------------------------------------
function CHIP:select()
    self.hdr = {}
end

function CHIP:access(byte)
    local blk, write, addr = self.blk, self.write, self.addr
    local v = 0
    if blk == 0 then
        if write then
            if addr == 0x00 and byte & 0x80 ~= 0 then
                self:reset()
                return 0
            end
            if addr <= 0x38 then
                self.common[addr] = byte
            end
        elseif addr == 0x2E then
            v = self:phycfgr()
        else
            v = self.common[addr] or 0
        end
    else
        local s = (blk - 1) // 4
        local kind = (blk - 1) % 4
        if kind == 0 then
            local r = self.sock[s]
            if write then
                if addr == 0x02 then
                    r[0x02] = r[0x02] & ~byte -- Sn_IR: write 1 clears
                elseif addr <= 0x2F then
                    r[addr] = byte
                    if s == 0 and addr == 0x01 then
                        self:command(byte)
                    elseif s == 0 then
                        self:update_sizes()
                    end
                end
            else
                v = r[addr] or 0
            end
        elseif s == 0 and kind == 1 then
            local size = self:buf_size("tx")
            if write and size > 0 then
                self.tx[addr % size] = byte
            end
        elseif s == 0 and kind == 2 then
            local size = self:buf_size("rx")
            if not write and size > 0 then
                v = self.rx[addr % size] or 0
            end
        end
    end
    self.addr = (addr + 1) & 0xFFFF
    return v
end

function CHIP:transfer(byte)
    if self.in_reset then
        return 0xFF
    end
    local hdr = self.hdr
    if #hdr < 3 then
        hdr[#hdr + 1] = byte
        if #hdr == 3 then
            self.addr = (hdr[1] << 8) | hdr[2]
            self.blk = hdr[3] >> 3
            self.write = hdr[3] & 0x04 ~= 0
        end
        return 0
    end
    return self:access(byte)
end

function CHIP:deselect()
    self.hdr = nil
end

function CHIP:attach(bus, cs)
    spi.attach(bus, cs, self)
    eth.attach(self, self.segment)
    return self
end

return M
