--[[----------------------------------------------------------------------------
  1-Wire bus model: the line shared by 1-Wire devices and connected to the
  1-Wire master model (e.g. DS2482, models/ds2482.lua).

  Usage:
    local onewire = require("models.onewire")
    local bus = onewire.new{name = "ow0"}     -- or: ds2482.new{}.ow
    local s = ds18b20.new{serial = 0x1234}:attach(bus)
    bus:set_short(true)                        -- line shorted to ground

  The bus is modeled by time slots, not by bytes: a slot started by the
  master writes one bit (bit 1 is the read slot), each device can pull the
  line low and the master and all devices sample the resulting level
  (wired-AND). The master does not know anything about commands of devices,
  so SEARCH ROM, ALARM SEARCH, READ ROM with more devices (collision) and
  other commands work as on the real line, also when the master sends single
  bits (DS2482 1WSB) instead of triplets.

  Master interface (used by master models):
    bus:reset(overdrive)    reset pulse, returns presence (boolean) and
                            short (boolean); overdrive: reset at overdrive
                            speed (devices without overdrive do not respond)
    bus:slot(bit)           time slot, master writes bit (1: read slot),
                            returns sampled level (0 or 1)
    bus:power(on)           strong pull-up on/off (power of parasite devices)
    bus:level()             line level without activity (0: shorted)

  Board interface:
    bus:add(dev)            connect device (also by dev:attach(bus))
    bus:remove(dev)         disconnect device (hot-plug)
    bus:devices()           list of connected devices
    bus:set_short(on)       line shorted to ground (reset: short, no
                            presence; all slots are read as 0)
    bus.name                name of bus (messages)

  Device interface (methods of device object):
    dev:ow_reset(overdrive) reset pulse, returns presence (boolean)
    dev:ow_drive()          level driven by device in this slot (0: pull
                            low, 1 or nil: release)
    dev:ow_sample(level)    level of the line sampled at the end of slot
    dev:ow_power(on)        strong pull-up changed (optional)
------------------------------------------------------------------------------]]

local M = {}

local BUS = {}
BUS.__index = BUS

local count = 0

function M.new(opts)
    opts = opts or {}
    count = count + 1
    local bus = setmetatable({}, BUS)
    bus.name = opts.name or ("onewire" .. count)
    bus.list = {}
    bus.short = false
    bus.powered = false
    return bus
end

function BUS:add(dev)
    assert(type(dev) == "table" and dev.ow_reset and dev.ow_drive and dev.ow_sample,
           "onewire: device object expected")
    for _, d in ipairs(self.list) do
        if d == dev then return dev end
    end
    table.insert(self.list, dev)
    return dev
end

function BUS:remove(dev)
    for i, d in ipairs(self.list) do
        if d == dev then
            table.remove(self.list, i)
            return true
        end
    end
    return false
end

function BUS:devices()
    return {table.unpack(self.list)}
end

function BUS:set_short(on)
    self.short = on and true or false
end

function BUS:level()
    return self.short and 0 or 1
end

function BUS:reset(overdrive)
    if self.powered then self:power(false) end
    local presence = false
    for _, d in ipairs(self.list) do
        if d:ow_reset(overdrive and true or false) then presence = true end
    end
    if self.short then
        return false, true
    end
    return presence, false
end

function BUS:slot(bit)
    if self.powered then self:power(false) end
    local level = (bit ~= 0 and bit ~= false) and 1 or 0
    if self.short then level = 0 end
    for _, d in ipairs(self.list) do
        if d:ow_drive() == 0 then level = 0 end
    end
    for _, d in ipairs(self.list) do
        d:ow_sample(level)
    end
    return level
end

function BUS:power(on)
    on = on and true or false
    if self.powered ~= on then
        self.powered = on
        for _, d in ipairs(self.list) do
            if d.ow_power then d:ow_power(on) end
        end
    end
end

return M
