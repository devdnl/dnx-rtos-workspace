--[[----------------------------------------------------------------------------
  MCP23S17 test executed in dnx RTOS (lua program) with board boards/test.lua:
  two expanders on SPI:2 with one CS (PB2, hardware addresses 0 and 1), signals
  of expanders are connected to GPIO port C (see board description). Registers
  are accessed by full duplex transfers (dnx.spi.transceive): opcode, register
  address and data in one transfer (CS active).
------------------------------------------------------------------------------]]

local T = require("testlib")
local spi = require("dnx.spi")
local test, expect = T.test, T.expect

local DEV = "/dev/spi-mcp"
local GPIOC = "/dev/gpioc"
local DELAY = 20 -- ms, time for board reaction on GPIO changes

-- registers (IOCON.BANK = 0)
local IODIRA, IODIRB, IPOLA, IPOLB, GPINTENA, GPINTENB = 0x00, 0x01, 0x02, 0x03, 0x04, 0x05
local DEFVALA, DEFVALB, INTCONA, INTCONB, IOCON, GPPUA = 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0C
local INTFA, INTFB, INTCAPA, INTCAPB, GPIOA, GPIOB, OLATA, OLATB = 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15

-- IOCON bits
local BANK, MIRROR, SEQOP, HAEN, ODR, INTPOL = 0x80, 0x40, 0x20, 0x08, 0x04, 0x02

-- port C pins
local RESET, INT0A, INT0B, INT1B = 16, 17, 18, 19  -- INT0B: INTB of #0 and INTA of #1

local function hex(s)
    return (tostring(s):gsub(".", function(c) return string.format("%02X ", c:byte()) end))
end

-- port C ----------------------------------------------------------------------
local pc_out = 0

local function pc_write(v)
    pc_out = v
    local f = assert(io.open(GPIOC, "r+b"))
    f:setvbuf("no")
    f:write(string.pack("<I4", v))
    f:close()
end

local function pc_read()
    local f = assert(io.open(GPIOC, "rb"))
    f:setvbuf("no")
    local s = assert(f:read(4))
    f:close()
    return (string.unpack("<I4", s))
end

local function pc_set(n, level)
    pc_write(level and (pc_out | (1 << n)) or (pc_out & ~(1 << n)))
end

local function pc_pin(n)
    return (pc_read() >> n) & 1 == 1
end

-- inputs of #0 port B (PC8..PC15)
local function inputs(v)
    pc_write((pc_out & ~0xFF00) | ((v & 0xFF) << 8))
end

-- outputs of #0 port A (PC0..PC7)
local function outputs()
    return pc_read() & 0xFF
end

-- SPI -------------------------------------------------------------------------
local function transfer(data)
    local r, err = spi.transceive(DEV, data)
    if not r then error("transceive: " .. tostring(err), 3) end
    return r
end

-- write registers from reg (sequential mode)
local function wr(hw, reg, ...)
    transfer(string.char(0x40 | (hw << 1), reg, ...))
end

-- read n registers from reg (default 1), returns values
local function rd(hw, reg, n)
    n = n or 1
    local r = transfer(string.char(0x41 | (hw << 1), reg) .. string.rep("\0", n))
    return r:byte(3, 2 + n)
end

local function expect_reg(hw, reg, value, msg)
    local v = rd(hw, reg)
    expect(v == value, string.format("%s: #%d register 0x%02X = 0x%02X, expected 0x%02X", msg, hw, reg, v, value))
end

local function reset()
    pc_set(RESET, false)
    T.msleep(DELAY)
    pc_set(RESET, true)
    T.msleep(DELAY)
end

-- both devices: HAEN set (IOCON written at address 0, HAEN cleared after reset)
local function enable_haen(extra)
    wr(0, IOCON, HAEN | (extra or 0))
end

-- tests ---------------------------------------------------------------------------
test("Create devices (modprobe -c)", function()
    T.spi_devices()
    T.modprobe("GPIO", 2, 0, GPIOC)
    pc_write(1 << RESET)
    reset()
end)

test("dnx.spi.transceive: frames in one transfer (CS active)", function()
    local r1 = transfer(string.char(0x41, IODIRA, 0, 0))
    local a, b = spi.transceive(DEV, string.char(0x41, IODIRA), "\0\0")
    expect(a and b, "two frames: " .. tostring(b))
    expect(#a == 2 and #b == 2, "lengths of received frames")
    expect(a == "\xFF\xFF", "MISO driven during opcode and address: " .. hex(a))
    expect(b == r1:sub(3), "two frames: " .. hex(b) .. ", one frame: " .. hex(r1))
    local r, err, code = spi.transceive("/dev/no-spi-device", "\0")
    expect(r == nil and type(err) == "string" and math.type(code) == "integer", "missing device")
end)

test("Power-on state (IODIR = 0xFF, other registers 0)", function()
    local regs = {rd(0, IODIRA, 22)}
    for addr = 0, 21 do
        local exp = (addr <= 1) and 0xFF or 0x00
        expect(regs[addr + 1] == exp, string.format("register 0x%02X = 0x%02X", addr, regs[addr + 1]))
    end
end)

test("HAEN cleared: devices respond to address 0 only", function()
    wr(0, OLATB, 0x5A)                   -- both devices
    expect(rd(1, OLATB) == 0xFF, "address 1 responds without HAEN")
    enable_haen()
    expect_reg(0, OLATB, 0x5A, "write at address 0 without HAEN")
    expect_reg(1, OLATB, 0x5A, "write at address 0 without HAEN")
end)

test("HAEN set: hardware addresses (one CS)", function()
    wr(1, OLATB, 0x11)
    wr(0, OLATB, 0x22)
    expect_reg(0, OLATB, 0x22, "address 0")
    expect_reg(1, OLATB, 0x11, "address 1")
    expect(rd(2, OLATB) == 0xFF, "address 2 responds")
    expect_reg(0, IOCON, HAEN, "IOCON")
    expect_reg(0, IOCON + 1, HAEN, "IOCON (second address)")
end)

test("Not valid opcode is ignored", function()
    transfer(string.char(0x30, OLATB, 0x99))
    expect_reg(0, OLATB, 0x22, "opcode 0x30")
end)

test("Outputs (IODIR, OLAT, write of GPIO)", function()
    wr(0, IODIRA, 0x00)
    wr(0, OLATA, 0xA5)
    expect(outputs() == 0xA5, string.format("PC0..PC7 = 0x%02X", outputs()))
    wr(0, GPIOA, 0x3C)
    expect(outputs() == 0x3C, string.format("write GPIOA: PC0..PC7 = 0x%02X", outputs()))
    expect_reg(0, OLATA, 0x3C, "OLATA after write of GPIOA")
    expect_reg(0, GPIOA, 0x3C, "GPIOA of outputs")
    wr(0, IPOLA, 0xFF)
    expect_reg(0, GPIOA, 0x3C, "IPOL inverted output")
    wr(0, IPOLA, 0x00)
end)

test("Inputs and polarity (IPOL)", function()
    inputs(0x5A)
    expect_reg(0, GPIOB, 0x5A, "GPIOB")
    wr(0, IPOLB, 0xF0)
    expect_reg(0, GPIOB, 0xAA, "GPIOB with IPOLB = 0xF0")
    wr(0, IPOLB, 0x00)
    inputs(0xC3)
    expect_reg(0, GPIOB, 0xC3, "GPIOB")
end)

test("Pull-up of not driven inputs (GPPU)", function()
    wr(0, IODIRA, 0xFF)
    expect_reg(0, GPIOA, 0x00, "floating inputs")
    wr(0, GPPUA, 0x0F)
    expect_reg(0, GPIOA, 0x0F, "inputs with pull-up")
    T.msleep(DELAY)
    expect(outputs() == 0x0F, string.format("pins PC0..PC7 = 0x%02X", outputs()))
    wr(0, GPPUA, 0x00)
end)

test("Sequential write and read (SEQOP = 0, address roll-over)", function()
    wr(1, DEFVALA, 0x01, 0x02, 0x03, 0x04)  -- DEFVALA, DEFVALB, INTCONA, INTCONB
    local a, b, c, d = rd(1, DEFVALA, 4)
    expect(a == 0x01 and b == 0x02 and c == 0x03 and d == 0x04,
           string.format("read %02X %02X %02X %02X", a, b, c, d))
    wr(1, OLATB, 0x77, 0xF0, 0xFF)      -- OLATB, roll-over: IODIRA, IODIRB
    local olatb, iodira, iodirb = rd(1, OLATB, 3)
    expect(olatb == 0x77 and iodira == 0xF0 and iodirb == 0xFF,
           string.format("roll-over: %02X %02X %02X", olatb, iodira, iodirb))
    wr(1, IODIRA, 0xFF)
    wr(1, DEFVALA, 0x00, 0x00, 0x00, 0x00)
end)

test("Byte mode (SEQOP = 1, BANK = 0: A/B pair)", function()
    wr(0, OLATA, 0x81)
    wr(0, OLATB, 0x42)
    wr(0, IOCON, HAEN | SEQOP)
    local v = {rd(0, OLATA, 4)}
    expect(v[1] == 0x81 and v[2] == 0x42 and v[3] == 0x81 and v[4] == 0x42,
           string.format("read %02X %02X %02X %02X", v[1], v[2], v[3], v[4]))
    wr(0, IOCON, HAEN)
end)

test("BANK = 1 register addresses", function()
    wr(0, IOCON, HAEN | BANK)
    expect_reg(0, 0x05, HAEN | BANK, "IOCON at 0x05")
    expect_reg(0, 0x15, HAEN | BANK, "IOCON at 0x15")
    expect_reg(0, 0x0A, 0x81, "OLATA at 0x0A")
    expect_reg(0, 0x1A, 0x42, "OLATB at 0x1A")
    expect_reg(0, 0x10, 0xFF, "IODIRB at 0x10")
    expect_reg(0, 0x0B, 0x00, "not implemented address 0x0B")
    local olata, iodirb = rd(0, 0x0A, 2)
    expect(olata == 0x81 and iodirb == 0xFF, "sequential 0x0A -> 0x10")
    wr(0, 0x05, HAEN | BANK | SEQOP)     -- IOCON in BANK = 1
    local x, y = rd(0, 0x0A, 2)
    expect(x == 0x81 and y == 0x81, "byte mode BANK = 1 (same address)")
    wr(0, 0x05, HAEN)                    -- IOCON in BANK = 1: back to BANK = 0
    expect_reg(0, IOCON, HAEN, "IOCON BANK = 0")
end)

test("Interrupt on change (INTCON = 0), INTF and INTCAP", function()
    wr(1, IOCON, HAEN | ODR)             -- INTA of #1 does not drive shared line (INTB of #0)
    inputs(0x00)
    rd(0, GPIOB)
    wr(0, GPINTENB, 0xFF)
    T.msleep(DELAY)
    expect(pc_pin(INT0B), "INTB low (active) before change")
    inputs(0x01)
    T.msleep(DELAY)
    expect(not pc_pin(INT0B), "INTB high (not active) after change")
    expect_reg(0, INTFB, 0x01, "INTFB")
    inputs(0x03)                         -- next change: INTF, INTCAP not changed
    expect_reg(0, INTFB, 0x01, "INTFB after second change")
    expect_reg(0, INTCAPB, 0x01, "INTCAPB")  -- read clears interrupt
    T.msleep(DELAY)
    expect(pc_pin(INT0B), "INTB low (active) after read of INTCAP")
    expect_reg(0, INTFB, 0x00, "INTFB after read of INTCAP")
    expect_reg(0, GPIOB, 0x03, "GPIOB")
    wr(0, GPINTENB, 0x00)
end)

test("Interrupt compared with DEFVAL (INTCON = 1)", function()
    inputs(0x00)
    wr(0, DEFVALB, 0x00)
    wr(0, INTCONB, 0x01)
    wr(0, GPINTENB, 0x01)
    expect_reg(0, INTFB, 0x00, "INTFB without condition")
    inputs(0x01)
    expect_reg(0, INTFB, 0x01, "INTFB")
    expect_reg(0, GPIOB, 0x01, "GPIOB")      -- clears, condition still active
    expect_reg(0, INTFB, 0x01, "INTFB when pin still differs from DEFVAL")
    inputs(0x00)
    rd(0, GPIOB)
    expect_reg(0, INTFB, 0x00, "INTFB after end of condition")
    wr(0, GPINTENB, 0x00)
    wr(0, INTCONB, 0x00)
end)

test("Interrupt only from inputs", function()
    wr(0, IODIRA, 0x00)
    wr(0, GPINTENA, 0xFF)
    wr(0, OLATA, 0x55)
    expect_reg(0, INTFA, 0x00, "INTFA after change of outputs")
    wr(0, GPINTENA, 0x00)
    wr(0, IODIRA, 0xFF)
end)

test("MIRROR and INTPOL (active high)", function()
    wr(0, IOCON, HAEN | MIRROR | INTPOL)
    T.msleep(DELAY)
    expect(not pc_pin(INT0A) and not pc_pin(INT0B), "INT pins high (active) without interrupt")
    inputs(0x00)
    rd(0, GPIOB)
    wr(0, GPINTENB, 0x80)
    inputs(0x80)
    T.msleep(DELAY)
    expect(pc_pin(INT0A), "INTA does not signal interrupt of port B (MIRROR)")
    expect(pc_pin(INT0B), "INTB low (not active) during interrupt")
    rd(0, GPIOB)
    T.msleep(DELAY)
    expect(not pc_pin(INT0A) and not pc_pin(INT0B), "INT pins high (active) after clear")
    wr(0, GPINTENB, 0x00)
    wr(0, IOCON, HAEN)
end)

test("Open drain INT shared by two devices (ODR)", function()
    -- #1: GPA outputs drive GPB inputs, interrupt of port B on INTA (MIRROR)
    wr(1, IOCON, HAEN | MIRROR | ODR | INTPOL)  -- open drain: active low, INTPOL ignored
    wr(1, IODIRA, 0x00)
    wr(1, OLATA, 0x00)
    rd(1, GPIOB)
    wr(1, GPINTENB, 0xFF)
    wr(0, IOCON, HAEN | ODR)
    inputs(0x00)
    rd(0, GPIOB)
    wr(0, GPINTENB, 0xFF)
    T.msleep(DELAY)
    expect(pc_pin(INT0B), "shared line low (active) before interrupts")

    wr(1, OLATA, 0x10)                   -- interrupt of #1
    T.msleep(DELAY)
    expect(not pc_pin(INT0B) and not pc_pin(INT1B), "interrupt of #1 not signaled")
    expect_reg(1, INTFB, 0x10, "INTFB of #1")
    inputs(0x04)                         -- interrupt of #0
    rd(1, INTCAPB)                       -- clear #1, line still low (#0)
    T.msleep(DELAY)
    expect(not pc_pin(INT0B), "shared line released by #1 during interrupt of #0")
    expect(pc_pin(INT1B), "INTB of #1 low (active) after clear")
    rd(0, GPIOB)
    T.msleep(DELAY)
    expect(pc_pin(INT0B), "shared line low (active) after clear of both devices")
    wr(0, GPINTENB, 0x00)
    wr(1, GPINTENB, 0x00)
end)

test("RESET pin", function()
    wr(0, IODIRA, 0x00)
    wr(0, OLATA, 0xFF)
    T.msleep(DELAY)
    expect(outputs() == 0xFF, "outputs before reset")
    reset()
    expect(outputs() == 0x00, string.format("PC0..PC7 after reset = 0x%02X", outputs()))
    expect_reg(0, IOCON, 0x00, "IOCON after reset")
    expect(rd(1, IOCON) == 0xFF, "address 1 responds after reset (HAEN not cleared)")
    expect_reg(0, IODIRA, 0xFF, "IODIRA after reset")
    expect_reg(0, OLATA, 0x00, "OLATA after reset")
end)

T.summary()
