--[[----------------------------------------------------------------------------
  FT5336 capacitive touch panel controller model (I2C, address 0x38, one
  touch point). Touch events are given by the board, e.g. from the LCD of the
  board panel (panel.on_touch()).

  Usage:
    local ft5336 = require("models.ft5336")
    local ts = ft5336.new{int = "PA9"}:attach("I2C1", 0x38)
    panel.on_touch(function(x, y, pressed) ts:touch(x, y, pressed) end)

  Options:
    int         INT output pin (optional): low while panel is touched
    swap_xy     X and Y registers are swapped (as on STM32F746G-DISCO)

  Methods:
    ts:touch(x, y, pressed)     touch (pressed = true) or release (false); short
                                touch is reported until it is read (TD_STATUS)

  Registers (address is incremented after each read and write byte):
    0x00 DEV_MODE, 0x01 GEST_ID (0), 0x02 TD_STATUS (number of points: 0/1),
    0x03 P1_XH (bits 7-6 event: 0 press down, 1 lift up, 2 contact; bits
    3-0 X[11:8]), 0x04 P1_XL, 0x05 P1_YH (bits 7-4 touch ID: 0, bits 3-0
    Y[11:8]), 0x06 P1_YL, 0x07 P1_WEIGHT, 0x08 P1_MISC, 0x80 TH_GROUP,
    0xA3 CIPHER, 0xA4 G_MODE, 0xA6 FIRMID, 0xA8 FOCALTECH_ID (0x51).
------------------------------------------------------------------------------]]

local M = {}

local FT = {}
FT.__index = FT

local EVENT_DOWN, EVENT_UP, EVENT_CONTACT = 0, 1, 2

function M.new(opts)
    opts = opts or {}
    local dev = setmetatable({}, FT)
    dev.int = opts.int
    dev.swap_xy = opts.swap_xy and true or false
    dev.reg = {[0x80] = 0x16, [0xA3] = 0x0A, [0xA4] = 0x00, [0xA6] = 0x10, [0xA8] = 0x51}
    dev.ptr = 0
    dev.points = 0
    dev.event = EVENT_UP
    dev.x, dev.y = 0, 0
    if dev.int then gpio.set(dev.int, true) end
    return dev
end

function FT:release()
    self.event = EVENT_UP
    self.points = 0
    self.release_pending = false
    if self.int then gpio.set(self.int, true) end
end

function FT:touch(x, y, pressed)
    if pressed then
        self.event = (self.points == 0) and EVENT_DOWN or EVENT_CONTACT
        self.points = 1
        self.reported = false
        self.release_pending = false
        self.x, self.y = math.tointeger(x) or 0, math.tointeger(y) or 0
        if self.int then gpio.set(self.int, false) end
    elseif self.points > 0 and not self.reported then
        self.release_pending = true -- touch was not read yet (short touch)
    else
        self:release()
    end
end

-- value of register
function FT:value(r)
    local x, y = self.x, self.y
    if self.swap_xy then x, y = y, x end
    if r == 0x02 then
        if self.points > 0 then self.reported = true end
        return self.points
    elseif r == 0x03 then return (self.event << 6) | ((x >> 8) & 0x0F)
    elseif r == 0x04 then return x & 0xFF
    elseif r == 0x05 then return (y >> 8) & 0x0F
    elseif r == 0x06 then return y & 0xFF
    elseif r == 0x07 then return self.points > 0 and 0x20 or 0
    end
    return self.reg[r] or 0
end

-- bus interface
function FT:select(read)
    if self.release_pending and self.reported then self:release() end
    self.first = not read
    return true
end

function FT:write(byte)
    if self.first then
        self.ptr = byte
        self.first = false
    else
        self.reg[self.ptr] = byte
        self.ptr = (self.ptr + 1) & 0xFF
    end
    return true
end

function FT:read()
    local v = self:value(self.ptr)
    self.ptr = (self.ptr + 1) & 0xFF
    return v
end

function FT:attach(bus, address)
    i2c.attach(bus, address or 0x38, self)
    return self
end

return M
