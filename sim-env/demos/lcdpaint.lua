--[[----------------------------------------------------------------------------
  LCD paint demo executed in dnx RTOS (lua program) in the POSIX simulator
  with the example board and the board panel:
    make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua --hostfs sim-env"
    lua /host/demos/lcdpaint.lua

  Draws on the LCD (LTDC driver, /dev/lcd, RGB565) where the LCD of the board
  panel (web browser) is touched. Touch is read from the FT5336 touch panel
  controller (I2C1, address 0x38) by the I2C driver (/dev/i2c-ts). Top line:
  colors, C clears the screen, X exits.
------------------------------------------------------------------------------]]

local dnx = require("dnx")
local lfs = require("lfs")

local W = 480       -- __LTDC_WIDTH__ of configuration
local BOX = 40      -- size of palette box
local BRUSH = 6

local function run(cmd)
    local p = assert(io.popen(cmd, "r"))
    local out = p:read("a")
    if not p:close() then error(cmd .. ": " .. out) end
end

if not lfs.attributes("/dev/lcd") then
    run("modprobe -i LTDC 0 0 /dev/lcd -c pixel_format=rgb565")
end
if not lfs.attributes("/dev/i2c-ts") then
    run("modprobe -i I2C 0 5 /dev/i2c-ts -c address=0x38,sub_addr_mode=1")
end

local H = lfs.attributes("/dev/lcd", "size") // (W * 2)

local lcd = assert(io.open("/dev/lcd", "r+b"))
lcd:setvbuf("no")
local ts = assert(io.open("/dev/i2c-ts", "r+b"))
ts:setvbuf("no")

local function color(r, g, b)
    return string.pack("<I2", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
end

local function rect(x, y, w, h, c)
    if x < 0 then w, x = w + x, 0 end
    if y < 0 then h, y = h + y, 0 end
    w, h = math.min(w, W - x), math.min(h, H - y)
    if w <= 0 or h <= 0 then return end
    local row = c:rep(w)
    for j = y, y + h - 1 do
        lcd:seek("set", (j * W + x) * 2)
        lcd:write(row)
    end
end

-- touch point (x, y) or nil
local function touch()
    ts:seek("set", 2)
    local d = ts:read(5)
    if not d or #d < 5 or (d:byte(1) & 0x0F) == 0 then return nil end
    return ((d:byte(2) & 0x0F) << 8) | d:byte(3), ((d:byte(4) & 0x0F) << 8) | d:byte(5)
end

local BG = color(16, 18, 24)
local PALETTE = {
    color(255, 255, 255), color(239, 68, 68), color(245, 158, 11), color(234, 179, 8),
    color(34, 197, 94), color(59, 130, 246), color(168, 85, 247), color(236, 72, 153),
}
local GRAY, DARK = color(100, 100, 110), color(40, 40, 48)

local function letter(x, y, pattern)
    -- 5x5 bitmap scaled by 4
    for j, line in ipairs(pattern) do
        for i = 1, #line do
            if line:sub(i, i) == "#" then rect(x + (i - 1) * 4, y + (j - 1) * 4, 4, 4, color(230, 230, 230)) end
        end
    end
end

local function screen()
    rect(0, 0, W, H, BG)
    for i, c in ipairs(PALETTE) do rect((i - 1) * BOX + 2, 2, BOX - 4, BOX - 4, c) end
    rect(W - 2 * BOX + 2, 2, BOX - 4, BOX - 4, DARK)
    letter(W - 2 * BOX + 10, 10, {".###.", "#....", "#....", "#....", ".###."})
    rect(W - BOX + 2, 2, BOX - 4, BOX - 4, color(127, 29, 29))
    letter(W - BOX + 10, 10, {"#...#", ".#.#.", "..#..", ".#.#.", "#...#"})
    rect(0, BOX, W, 1, GRAY)
end

screen()
print("Touch the LCD in the board panel (C: clear, X: exit).")

local current = PALETTE[1]
local last = nil
local pressed = false
local running = true

while running do
    local x, y = touch()
    local new_press = x and not pressed
    pressed = x ~= nil
    if x and y < BOX then
        if new_press then
            local i = x // BOX + 1
            if x >= W - BOX then
                running = false
            elseif x >= W - 2 * BOX then
                screen()
            elseif PALETTE[i] then
                current = PALETTE[i]
            end
        end
        last = nil
    elseif x then
        -- line from the last point (fast movement)
        local lx, ly = last and last[1] or x, last and last[2] or y
        local steps = math.max(1, math.max(math.abs(x - lx), math.abs(y - ly)) // (BRUSH // 2))
        for s = 1, steps do
            local px = lx + (x - lx) * s // steps
            local py = math.max(ly + (y - ly) * s // steps, BOX + BRUSH // 2 + 1)
            rect(px - BRUSH // 2, py - BRUSH // 2, BRUSH, BRUSH, current)
        end
        last = {x, y}
    else
        last = nil
    end
    dnx.sys.msleep(10)
end

rect(0, 0, W, H, BG)
lcd:close()
ts:close()
print("bye")
