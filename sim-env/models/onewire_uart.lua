--[[----------------------------------------------------------------------------
  1-Wire master on UART (UART TX/RX connected to the 1-Wire line by open
  drain buffer, as DS9097 adapters or UART in single wire mode). Every
  character sent by the system is received back with the state of the line:
    baud rate below 38400: reset pulse, character with presence pulse is
    changed (0xF0 -> 0xE0), line shorted: 0x00
    other baud rates: time slot, bit 0 of character is the written bit
    (0xFF: write 1 / read slot, 0x00: write 0 slot), slot read as 0 changes
    the character (0xFF -> 0xF8)
  Characters of other format than 8N1 are not echoed (line is not driven).

  Usage:
    local onewire_uart = require("models.onewire_uart")
    local ds18b20 = require("models.ds18b20")
    local ow = onewire_uart.new{spu = "PB8"}:attach("UART:6")
    ds18b20.new{serial = 0x1234}:attach(ow)

  Options of new():
    spu         GPIO pin of strong pull-up (high: on), power of parasite
                devices (optional)

  Fields: ow (1-Wire bus, models/onewire.lua).
------------------------------------------------------------------------------]]

local onewire = require("models.onewire")

local M = {}

local MASTER = {}
MASTER.__index = MASTER

local RESET_BAUD_MAX = 38400

function M.new(opts)
    opts = opts or {}
    local m = setmetatable({}, MASTER)
    m.ow = onewire.new({ name = opts.name })
    if opts.spu then
        gpio.on_change(opts.spu, function(level)
            m.ow:power(level)
        end)
    end
    return m
end

function MASTER:receive(data)
    local cfg = uart.config(self.port)
    if cfg.parity ~= "none" or cfg.stop_bits ~= 1 then
        return
    end

    local res = {}
    for i = 1, #data do
        local c = data:byte(i)
        if cfg.baud < RESET_BAUD_MAX then
            local presence, short = self.ow:reset(false)
            if short then
                c = 0x00
            elseif presence then
                c = c & 0xE0
            end
        else
            local level = self.ow:slot(c & 1)
            if level == 0 then
                c = c & 0xF8
            end
        end
        res[i] = string.char(c)
    end
    uart.send(self.port, table.concat(res))
end

function MASTER:attach(port)
    self.port = port
    uart.attach(port, self)
    return self
end

return M
