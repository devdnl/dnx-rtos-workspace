--[[----------------------------------------------------------------------------
  ST7789 SPI LCD controller model (4-wire SPI: SCL, SDA, CS, D/C), also
  compatible with ILI9341, ST7735 and other MIPI DCS controllers. SPI mode 0
  or 3, MSB first. The display is shown by the board panel as LCD <id>
  (<canvas data-lcd="<id>">, panel.lcd() of board API).

  Usage:
    local st7789 = require("models.st7789")
    local lcd = st7789.new{id = "tft", width = 240, height = 320, dc = "PB0"}
    lcd:attach("SPI:2", "PB1")                  -- bus, CS pin (active low)
    lcd:on_touch(function(x, y, pressed) ... end)  -- touch of the panel LCD

  Options of new():
    id          LCD id of the panel (default "st7789")
    width       width of the display in pixels (default 240)
    height      height of the display in pixels (default 320)
    dc          D/C pin (output of the system): low - command, high - data
    reset       RESET pin (output of the system, active low, optional)
    format      pixel format of the panel LCD (default "rgb565"), "rgb888"
                shows 18-bit colors without loss
    id_bytes    bytes returned by RDDID (default {0x85, 0x85, 0x52})

  Instructions: SWRESET (0x01), RDDID (0x04, 3 bytes without dummy byte),
  RDDPM (0x0A), SLPIN (0x10), SLPOUT (0x11), DISPOFF (0x28), DISPON (0x29),
  CASET (0x2A), RASET (0x2B), RAMWR (0x2C), RAMWRC (0x3C), MADCTL (0x36:
  MY mirrors rows, MX mirrors columns, MV exchanges rows and columns, BGR
  swaps red and blue), COLMOD (0x3A: 16-bit RGB565 or 18-bit RGB666 pixels).
  Other instructions (e.g. INVON, gamma, power settings) are accepted and
  ignored. The display is shown after SLPOUT and DISPON (initial state: sleep,
  display off); in sleep or display off mode the panel LCD is not active.

  Methods:
    lcd:attach(bus, cs)         connect to bus
    lcd:on_touch(fn)            call fn(x, y, pressed) when LCD of the panel
                                is touched (x, y: pixels of the display)
    lcd:pixel(x, y)             color of display pixel (host side, e.g. tests)
    lcd:reset()                 hardware reset

  Field:
    lcd.lcd                     LCD object of the panel (panel.lcd())
------------------------------------------------------------------------------]]

local M = {}

local SWRESET, RDDID, RDDPM, SLPIN, SLPOUT = 0x01, 0x04, 0x0A, 0x10, 0x11
local DISPOFF, DISPON, CASET, RASET, RAMWR = 0x28, 0x29, 0x2A, 0x2B, 0x2C
local MADCTL, COLMOD, RAMWRC = 0x36, 0x3A, 0x3C

local DEV = {}
DEV.__index = DEV

function M.new(opts)
    opts = opts or {}
    assert(opts.dc, "st7789: D/C pin is not defined")

    local d = setmetatable({}, DEV)
    d.width = opts.width or 240
    d.height = opts.height or 320
    d.id_bytes = opts.id_bytes or {0x85, 0x85, 0x52}
    d.mode = {[0] = true, [3] = true}
    d.lcd = panel.lcd(opts.id or "st7789", d.width, d.height, {format = opts.format, enabled = false})
    d.rgb565 = (d.lcd.format == "rgb565")      -- 16-bit pixels are written without conversion

    d.dc = gpio.get(opts.dc)
    gpio.on_change(opts.dc, function(level) d.dc = level end)

    if opts.reset then
        gpio.on_change(opts.reset, function(level) if not level then d:reset() end end)
    end

    d:reset()
    return d
end

-- state after reset (memory content is not changed)
function DEV:reset()
    self.sleep = true
    self.on = false
    self.madctl = 0
    self.bpp = 2                -- bytes per pixel (COLMOD)
    self.cmd = nil
    self:window(0, 0, self.width - 1, self.height - 1)
    self:update()
end

-- panel LCD is active in normal mode with display on
function DEV:update()
    self.lcd:enable(self.on and not self.sleep)
end

-- logical size (MV exchanges rows and columns)
function DEV:size()
    if self.madctl & 0x20 ~= 0 then return self.height, self.width end
    return self.width, self.height
end

function DEV:window(xs, ys, xe, ye)
    self.xs, self.ys, self.xe, self.ye = xs, ys, xe, ye
end

-- write pixel (value in LCD format) at the address pointer and move the pointer
function DEV:put(v)
    local w, h = self:size()
    local x, y = self.x, self.y
    if x < w and y < h then
        local m = self.madctl
        if m & 0x40 ~= 0 then x = w - 1 - x end     -- MX
        if m & 0x80 ~= 0 then y = h - 1 - y end     -- MY
        if m & 0x20 ~= 0 then x, y = y, x end       -- MV
        self.lcd:pixel(x, y, v)
    end
    if self.x >= self.xe then
        self.x = self.xs
        self.y = (self.y >= self.ye) and self.ys or self.y + 1
    else
        self.x = self.x + 1
    end
end

function DEV:pixel(x, y)
    return self.lcd:pixel(x, y)
end

function DEV:on_touch(fn)
    self.lcd:on_touch(fn)
    return self
end

-- SPI interface (state is kept between transfers: CS can be released between
-- command and its parameters)
function DEV:command(cmd)
    self.cmd, self.args, self.rd = cmd, {}, nil

    if cmd == SWRESET then
        self:reset()
    elseif cmd == SLPIN or cmd == SLPOUT then
        self.sleep = (cmd == SLPIN)
        self:update()
    elseif cmd == DISPOFF or cmd == DISPON then
        self.on = (cmd == DISPON)
        self:update()
    elseif cmd == RAMWR then
        self.x, self.y, self.n = self.xs, self.ys, 0
    elseif cmd == RAMWRC then
        self.cmd, self.n = RAMWR, 0
    elseif cmd == RDDID then
        self.rd = {table.unpack(self.id_bytes)}
    elseif cmd == RDDPM then
        self.rd = {0x08 | (self.sleep and 0 or 0x90) | (self.on and 0x04 or 0)}  -- booster, sleep out, normal, on
    end
end

function DEV:data_byte(byte)
    local cmd = self.cmd

    if cmd == RAMWR then
        local n = self.n + 1
        if n < self.bpp then
            self[n], self.n = byte, n   -- first bytes of pixel: self[1], self[2]
            return
        end
        self.n = 0
        local bgr = self.madctl & 0x08 ~= 0
        if self.bpp == 2 then
            local v = (self[1] << 8) | byte
            if bgr then v = ((v & 0x1F) << 11) | (v & 0x07E0) | (v >> 11) end
            if not self.rgb565 then
                local r, g, b = (v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F
                v = self.lcd:color((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))
            end
            self:put(v)
        else
            local r, g, b = self[1] & 0xFC, self[2] & 0xFC, byte & 0xFC
            if bgr then r, b = b, r end
            self:put(self.lcd:color(r | (r >> 6), g | (g >> 6), b | (b >> 6)))
        end

    elseif cmd == CASET or cmd == RASET then
        local a = self.args
        a[#a + 1] = byte
        if #a == 4 then
            local s, e = (a[1] << 8) | a[2], (a[3] << 8) | a[4]
            if cmd == CASET then self.xs, self.xe = s, e else self.ys, self.ye = s, e end
        end

    elseif cmd == MADCTL then
        self.madctl = byte
        self.cmd = nil

    elseif cmd == COLMOD then
        self.bpp = ((byte & 0x07) == 0x06) and 3 or 2  -- 0x66/0x06: 18-bit, other: 16-bit
        self.cmd = nil
    end
end

function DEV:transfer(byte)
    if not self.dc then
        self:command(byte)
        return 0xFF
    end

    if self.rd then
        return table.remove(self.rd, 1) or 0xFF
    end

    if self.cmd then self:data_byte(byte) end
    return 0xFF
end

function DEV:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
