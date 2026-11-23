--[[----------------------------------------------------------------------------
  MAX310x SPI/I2C UART model: MAX3107, MAX3108 (1 UART), MAX3109 (2 UARTs),
  128-word FIFOs.

  Usage:
    local max310x = require("models.max310x")
    local chip = max310x.new{type = "max3109", clock = 3686400, reset = "PA30"}
    chip:attach("SPI:1", "PA29")                 -- SPI bus, CS pin (active low)
    -- or chip:attach("I2C:1", 0x6C)             -- I2C bus, address of UART 0
    local u = chip:uart(0)                       -- UART of the chip (0, 1)
    u:connect_uart("UART:4")                     -- serial line to UART of the system
    u:on_transmit(function(data) ... end)        -- data transmitted by the UART
    u:receive("text")                            -- data received by the UART

  Options of new():
    type        "max3107" (default), "max3108", "max3109"
    clock       frequency of reference clock on XIN [Hz] (default 3686400), the
                clock works when CLKSource selects external clock (ExtClock)
                or crystal (Crystal)
    reset       system output pin connected to RST (optional, active low: the
                chip is reset and does not respond while the pin is low, the
                pin is followed from the first change done by the system)
    revid       revision ID (default: MAX3107 0xA1, MAX3109 0xC1; MAX3108 has
                no revision ID)

  Registers of UART 0x00..0x1E (datasheet names), reset values 0 except DIVLSB
  = 0x01 and CLKSource = 0x18 (ExtClock, PLLBypass). Implemented: RHR/THR
  (FIFOs), LSR (RxOverrun, CTS state; read clears RxOverrun), MODE1
  (RxDisabl, TxDisabl, TrnscvCtrl), MODE2 (RST: soft reset of UART while set,
  FIFORst, Loopback), LCR (word length, stop bits, parity), FlowLvl, FlowCtrl
  (AutoRTS, AutoCTS), TxFIFOLvl, RxFIFOLvl, BRGConfig, DIVLSB, DIVMSB (baud rate
  generator, 2x and 4x modes), CLKSource (ExtClock, Crystal; MAX3109: register
  of UART 0 is used by both UARTs), STSInt (ClockReady when crystal is
  selected). Register 0x1F: MAX3107 RevID; MAX3109 write: GlobalCommand (0xCE
  enables, 0xCD disables extended registers: RevID at 0x05 by SPI, 0x25 by
  I2C), read: GlobalIRQ (0xFF). Other registers are stored without effect;
  EchoSuprs, IrDA inversion and interrupts have no effect.

  SPI (mode 0 or 3): command byte (bit 7: 1 write, 0 read; bits 6..5: UART;
  bits 4..0: register) and data, the register address is incremented after
  each byte except of RHR/THR (FIFO). I2C: address of UART 0 is given to
  attach(), UART 1 of MAX3109 has address lower by 0x10; first written byte
  is register address, next bytes are written to registers, read returns
  registers (the same increment).

  Transmission: bytes of TX FIFO are transmitted with the time of character
  (start bit, data bits, parity, stop bits at the baud rate of the generator),
  not with disabled transmitter, without clock or with AutoCTS and inactive
  CTS. With Loopback the bytes are received by the UART, otherwise they are
  passed to on_transmit() and to the connected UART. Received bytes are stored
  in RX FIFO when the receiver is enabled, bytes received when FIFO is full are
  lost (LSR RxOverrun). Connected UART of the system: data of different format
  (baud rate differs more than 3 %, parity, stop bits, data bits other than 8)
  are changed to 0xFF (unreadable data).

  Methods of chip:
    chip:attach(bus, cs_or_address)   connect to SPI ("SPI:n", CS pin) or I2C bus ("I2C:n", address)
    chip:uart(n)                      UART n of the chip
    chip:reset()                      reset (as RST pin)
  Methods of UART:
    u:connect_uart(bus)               serial line to system UART ("UART:n")
    u:on_transmit(fn)                 fn(data) for transmitted data (string)
    u:receive(data)                   data received by the UART (string)
    u:set_cts(active)                 CTS input (true: active, transmission allowed, default)
    u:rts()                           RTS output with AutoRTS (true: active, receive allowed)
    u:format()                        {baud, data_bits, parity ("none", "odd", "even"), stop_bits}
                                      or nil without clock
    u:register(addr)                  register value without side effects
------------------------------------------------------------------------------]]

local M = {}

local CHIP = {}
CHIP.__index = CHIP

local UART = {}
UART.__index = UART

local FIFO_SIZE = 128
local MAX_LAG = 2 -- ms, transmitter does not catch up longer delay of board thread (see UART:service)

local TYPES = {
    max3107 = { ports = 1, revid = 0xA1 },
    max3108 = { ports = 1 },
    max3109 = { ports = 2, revid = 0xC1 },
}

local RHR = 0x00
local LSR = 0x04
local STSINT = 0x08
local MODE1 = 0x09
local MODE2 = 0x0A
local LCR = 0x0B
local FLOWLVL = 0x0F
local TXFIFOLVL = 0x11
local RXFIFOLVL = 0x12
local FLOWCTRL = 0x13
local BRGCONFIG = 0x1B
local DIVLSB = 0x1C
local DIVMSB = 0x1D
local CLKSOURCE = 0x1E
local REG_1F = 0x1F

local function bit(v, n)
    return (v >> n) & 1 == 1
end

function M.new(opts)
    opts = opts or {}
    local t = TYPES[opts.type or "max3107"]
    assert(t, "max310x: unknown type " .. tostring(opts.type))
    local c = setmetatable({}, CHIP)
    c.type = opts.type or "max3107"
    c.mode = { [0] = true, [3] = true } -- supported SPI modes
    c.clock = opts.clock or 3686400
    c.revid = opts.revid or t.revid
    c.ports = {}
    assert(c.clock > 0, "max310x: invalid clock")
    for n = 0, t.ports - 1 do
        c.ports[n] = setmetatable({ chip = c, n = n, cts = true, handlers = {} }, UART)
    end
    c:reset()

    if opts.reset then
        gpio.on_change(opts.reset, function(level)
            c.in_reset = not level
            if c.in_reset then
                c:reset()
            end
        end)
    end

    return c
end

function CHIP:uart(n)
    return assert(self.ports[n], "max310x: no UART " .. tostring(n))
end

function CHIP:reset()
    self.extreg = false
    for _, u in pairs(self.ports) do
        u:reset()
    end
end

-- UART: reset values of registers, empty FIFOs
function UART:reset()
    self.regs = {}
    for a = 0, 0x1F do
        self.regs[a] = 0
    end
    self.regs[DIVLSB] = 0x01
    self.regs[CLKSOURCE] = 0x18
    self.rx, self.tx = {}, {}
    self.overrun = false
    self.rts_active = true
    self.tx_time = nil
    self:stop_timer()
end

function UART:register(addr)
    return self:peek(addr)
end

function UART:set_cts(active)
    self.cts = active and true or false
    self:service()
end

function UART:rts()
    return self.rts_active
end

function UART:on_transmit(fn)
    self.handlers[#self.handlers + 1] = fn
end

-- baud rate of the generator or nil (no clock: CLKSource of UART 0)
function UART:baud()
    local clk = self.chip.ports[0].regs[CLKSOURCE]
    if not bit(clk, 4) and not bit(clk, 1) then
        return nil -- ExtClock and Crystal disabled
    end
    local div = (self.regs[DIVMSB] << 8) | self.regs[DIVLSB]
    local fract = self.regs[BRGCONFIG] & 0x0F
    local mult = bit(self.regs[BRGCONFIG], 5) and 4 or (bit(self.regs[BRGCONFIG], 4) and 2 or 1)
    if div == 0 then
        return nil
    end
    return self.chip.clock * mult / (16 * div + fract)
end

function UART:format()
    local baud = self:baud()
    if not baud then
        return nil
    end
    local lcr = self.regs[LCR]
    local parity = bit(lcr, 3) and (bit(lcr, 4) and "even" or "odd") or "none"
    return { baud = baud, data_bits = 5 + (lcr & 3), parity = parity, stop_bits = bit(lcr, 2) and 2 or 1 }
end

-- time of character [ms]
function UART:char_time()
    local f = self:format()
    if not f then
        return nil
    end
    local bits = 1 + f.data_bits + (f.parity == "none" and 0 or 1) + f.stop_bits
    return bits * 1000 / f.baud
end

-- format of connected UART differs (data unreadable)
function UART:format_mismatch()
    local f = self:format()
    if not f or not self.uart_bus then
        return true
    end
    local c = uart.config(self.uart_bus)
    return math.abs(f.baud - c.baud) > c.baud * 0.03 or f.parity ~= c.parity or f.stop_bits ~= c.stop_bits
        or f.data_bits ~= 8
end

-- FIFOs -------------------------------------------------------------------------
function UART:rx_push(byte)
    if bit(self.regs[MODE1], 0) or bit(self.regs[MODE2], 0) then
        return -- receiver disabled or UART in soft reset
    end
    if #self.rx >= FIFO_SIZE then
        self.overrun = true
    else
        self.rx[#self.rx + 1] = byte
    end
    self:update_rts()
end

function UART:update_rts()
    if bit(self.regs[FLOWCTRL], 0) then -- AutoRTS
        local halt = (self.regs[FLOWLVL] & 0x0F) * 8
        local resume = (self.regs[FLOWLVL] >> 4) * 8
        if #self.rx >= halt then
            self.rts_active = false
        elseif #self.rx <= resume then
            self.rts_active = true
        end
    end
end

function UART:receive(data)
    for i = 1, #data do
        self:rx_push(data:byte(i))
    end
end

function UART:can_transmit()
    if bit(self.regs[MODE1], 1) or bit(self.regs[MODE2], 0) then
        return false -- transmitter disabled or soft reset
    end
    if bit(self.regs[FLOWCTRL], 1) and not self.cts then
        return false -- AutoCTS: CTS inactive
    end
    return self:char_time() ~= nil
end

-- transmit bytes of TX FIFO whose time elapsed
function UART:service()
    local now = host.time_ms()
    local out = {}
    while #self.tx > 0 and self:can_transmit() do
        local ct = self:char_time()
        if not self.tx_time then
            self.tx_time = now + ct
        elseif self.tx_time < now - MAX_LAG then
            -- board thread or simulator was not running (host load): bytes are not sent at once
            -- (the loopback would overflow RX FIFO before the driver is able to read it)
            self.tx_time = now - MAX_LAG
        end
        if now < self.tx_time then
            break
        end
        local byte = table.remove(self.tx, 1)
        self.tx_time = (#self.tx > 0) and (self.tx_time + ct) or nil
        if bit(self.regs[MODE2], 5) then
            self:rx_push(byte) -- Loopback
        else
            out[#out + 1] = string.char(byte)
        end
    end
    if #self.tx > 0 and self:can_transmit() then
        self:start_timer()
    else
        self.tx_time = nil
        self:stop_timer()
    end
    if #out > 0 then
        local data = table.concat(out)
        for _, fn in ipairs(self.handlers) do
            fn(data)
        end
        if self.uart_bus then
            uart.send(self.uart_bus, self:format_mismatch() and string.rep("\xFF", #data) or data)
        end
    end
end

function UART:start_timer()
    if not self.timer then
        self.timer = host.timer(1, function()
            self:service()
        end, true)
    end
end

function UART:stop_timer()
    if self.timer then
        self.timer:stop()
        self.timer = nil
    end
end

-- registers ----------------------------------------------------------------------
function UART:peek(addr)
    if addr == RHR then
        return self.rx[1] or 0
    elseif addr == LSR then
        return (self.regs[LSR] & 0x7D) | (self.overrun and 0x02 or 0) | (self.cts and 0 or 0x80)
    elseif addr == TXFIFOLVL then
        return #self.tx
    elseif addr == RXFIFOLVL then
        return #self.rx
    elseif addr == STSINT then
        local cryst = bit(self.chip.ports[0].regs[CLKSOURCE], 1)
        return (self.regs[STSINT] & ~0x20) | (cryst and 0x20 or 0)
    end
    return self.regs[addr] or 0
end

function UART:read_reg(addr, i2c)
    local chip = self.chip
    if chip.type == "max3109" and chip.extreg and self.n == 0 and addr == (i2c and 0x25 or 0x05) then
        return chip.revid -- extended register: revision ID
    elseif addr == REG_1F then
        if chip.type == "max3107" then
            return chip.revid
        elseif chip.type == "max3109" then
            return 0xFF -- GlobalIRQ: no interrupts (active low)
        end
        return 0
    end
    self:service()
    local v = self:peek(addr)
    if addr == RHR then
        if #self.rx > 0 then
            table.remove(self.rx, 1)
            self:update_rts()
        end
    elseif addr == LSR then
        self.overrun = false
    end
    return v
end

function UART:write_reg(addr, v)
    local chip = self.chip
    if addr == REG_1F then
        if chip.type == "max3109" then -- GlobalCommand
            if v == 0xCE then
                chip.extreg = true
            elseif v == 0xCD then
                chip.extreg = false
            end
        end
        return
    elseif addr > REG_1F then
        return
    end
    self:service()
    if addr == RHR then
        if #self.tx < FIFO_SIZE then
            self.tx[#self.tx + 1] = v
            self:start_timer()
        end
        return
    elseif addr == TXFIFOLVL or addr == RXFIFOLVL then
        return -- read only
    end
    if addr == MODE2 then
        if bit(v, 0) then -- soft reset: registers have reset values while RST is set
            self:reset()
            self.regs[MODE2] = v
            return
        end
        if bit(v, 1) then -- FIFORst
            self.rx, self.tx = {}, {}
            self.tx_time = nil
        end
    end
    self.regs[addr] = v
    self:update_rts()
    if #self.tx > 0 then
        self:start_timer()
    end
end

-- register pointer (SPI and I2C transfers)
function UART:next_addr()
    if self.addr ~= RHR then
        self.addr = (self.addr + 1) & 0xFF
    end
end

-- SPI ------------------------------------------------------------------------------
function CHIP:select()
    self.state = "cmd"
end

function CHIP:deselect()
    self.state = nil
end

function CHIP:transfer(byte)
    if self.in_reset then
        return 0xFF
    end
    if self.state == "cmd" then
        self.wr = byte & 0x80 ~= 0
        self.cur = self.ports[(byte >> 5) & 0x03]
        if self.cur then
            self.cur.addr = byte & 0x1F
        end
        self.state = "data"
        return 0x00
    elseif self.state == "data" and self.cur then
        local u, v = self.cur, 0x00
        if self.wr then
            u:write_reg(u.addr, byte)
        else
            v = u:read_reg(u.addr)
        end
        u:next_addr()
        return v
    end
    return 0xFF
end

function CHIP:transfer_block(data)
    if self.in_reset or self.state ~= "data" then
        return nil -- command byte: transfer()
    end
    local out = {}
    for i = 1, #data do
        out[i] = string.char(self:transfer(data:byte(i)))
    end
    return table.concat(out)
end

-- I2C device of UART ---------------------------------------------------------------
local function i2c_device(u)
    local first = false
    return {
        select = function(_, read)
            if u.chip.in_reset then
                return false
            end
            first = not read
            return true
        end,
        write = function(_, byte)
            if first then
                u.addr = byte
                first = false
            else
                u:write_reg(u.addr, byte)
                u:next_addr()
            end
        end,
        read = function()
            local v = u:read_reg(u.addr, true)
            u:next_addr()
            return v
        end,
    }
end

-- connection -----------------------------------------------------------------------
function CHIP:attach(bus, where)
    if bus:match("^I2C:") then
        for n, u in pairs(self.ports) do
            i2c.attach(bus, where - n * 0x10, i2c_device(u))
        end
    else
        spi.attach(bus, where, self)
    end
    return self
end

function UART:connect_uart(bus)
    local u = self
    self.uart_bus = bus
    uart.attach(bus, {
        receive = function(_, data)
            if u:format_mismatch() then
                data = string.rep("\xFF", #data)
            end
            u:receive(data)
        end,
    })
    return self
end

return M
