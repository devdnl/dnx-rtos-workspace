--[[----------------------------------------------------------------------------
  BT817/BT818 (Bridgetek EVE3 video controller) model: SPI host interface,
  memory map, registers, command FIFO with interrupts and rendering of the
  display list to a board panel LCD. Datasheet: DS_BT817_8
  (BRT_000302), coprocessor commands: BT81x Series Programmers Guide.

  The chip is a memory mapped SPI device (mode 0 only, MSB first):
    read  = 0b00 + 22-bit address + dummy byte + data ...
    write = 0b10 + 22-bit address + data ...
    cmd   = 0b01 + 6-bit command code (+ parameters); ACTIVE is a transaction
            of three zero bytes (a memory read from address 0 wakes too)

  Usage:
    local bt817 = require("models.bt817")
    local eve = bt817.new{id = "eve", width = 480, height = 272, int = "PA8"}
    eve:attach("SPI:2", "PB3")             -- bus, CS pin (active low)
    panel.on_touch(function(x, y, p) eve:touch(x, y, p) end, "eve")

  Options of new():
    id          LCD id of the panel (default "bt817")
    width       width of the display in pixels (default 480)
    height      height of the display in pixels (default 272)
    format      pixel format of the panel LCD (default "rgb565")
    part        "bt817" or "bt818" (default "bt817"): chip ID bytes at 0C0000h
                (08 17 01 00 / 08 18 01 00)
    int         system input pin connected to INT_N (optional, open drain,
                active low, enabled by REG_INT_EN/REG_INT_MASK)
    pd          system output pin connected to PD_N (optional, active low: the
                chip is powered down while it is low and does not respond; the
                pin needs a pull-up or an output high from the system)
    tags        function tags(x, y) -> tag of REG_TOUCH_TAG (optional, overrides
                the tag buffer when it returns a number)

  Memory map (Table 5-1): RAM_G 000000..0FFFFF (sparse, writable chip ID at
  0C0000), ROM 200000, RAM_DL 300000 (8 kB), RAM_REG 302000, RAM_CMD 308000
  (4 kB FIFO ring: a continuous access wraps from 308FFF to 308000, as the
  host libraries write commands to RAM_CMD + REG_CMD_WRITE without splitting
  at the end of the ring). Registers are little endian and byte accessible, reserved
  addresses read as 0.

  Host commands (Table 4-5): ACTIVE, STANDBY, SLEEP, PWRDOWN (core off, SPI
  alive), CLKEXT, CLKSEL (Byte2[5:0] is the multiplier of the oscillator, 0 is
  the default clock, REG_FREQUENCY follows it), RST_PULSE (as POR except the
  power mode), CLKINT and PINDRIVE (accepted and ignored). The chip boots in
  sleep mode; the panel LCD is shown in ACTIVE state when the display is
  started (REG_PCLK is not 0). After PWRDOWN or PD_N the registers and memories
  are lost (reset values after ACTIVE).

  Registers: registers of Table 5-2 with their reset values. REG_ID reads 7Ch,
  REG_CLOCK counts from reset, REG_FRAMES counts frames while the display runs
  (REG_PCLK not 0, frame rate REG_FREQUENCY / (REG_PCLK * REG_HCYCLE *
  REG_VCYCLE), 60 Hz without timing), REG_INT_FLAGS clears at the end of the
  transaction which read it, REG_CMD_READ/WRITE and REG_CMDB_SPACE/
  REG_CMDB_WRITE implement the command FIFO (processed at the end of the
  transaction which wrote REG_CMD_WRITE or REG_CMDB_WRITE; a burst write to
  REG_CMDB_WRITE keeps the address; when the FIFO is full during a write to
  REG_CMDB_WRITE, it is processed during the transfer as by the chip, data
  without space are lost with warning), write to REG_DLSWAP renders the display
  list, sets the SWAP flag and clears REG_DLSWAP, REG_CPURESET bit 0 resets the
  coprocessor (FIFO pointers, fault), REG_PLAYBACK_PLAY ends after length/freq seconds and
  sets the PLAYBACK flag, REG_SPI_WIDTH keeps its value but Dual/Quad channel
  mode is not supported (warning once, Single mode kept), REG_GPIO/GPIOX keep
  their bits (bit 7/15 = DISP pin - not connected to the LCD of the panel).

  Command FIFO: display list instructions are collected in RAM_DL (REG_CMD_DL),
  coprocessor commands CMD_MEMZERO/MEMSET/MEMCPY/MEMWRITE/APPEND, CMD_GETPTR/
  REGREAD/MEMCRC (standard CRC-32; results are written to the result argument
  in the command buffer, as by the chip), CMD_INFLATE (zlib stream following in
  the FIFO, padded to words, decoded in C by gfx.inflater - resumed at deflate
  blocks, Adler-32 mismatch is reported once; GETPTR returns the end of
  inflated data),
  CMD_INTERRUPT (CMDFLAG interrupt after ms), CMD_WAIT (us, processing of the
  rest of the FIFO is delayed), CMD_SWAP are executed; MEMWRITE and INFLATE data
  may be longer than the FIFO (they are processed while they are written).
  CMDEMPTY flag is set when the FIFO is processed. The display list management
  commands CMD_NEWLIST/CMD_ENDLIST/CMD_CALLLIST/CMD_RETURN are parsed with
  correct argument counts but all lists are collected into one buffer.
  Widgets CMD_BUTTON, CMD_KEYS, CMD_TOGGLE, CMD_SLIDER, CMD_PROGRESS,
  CMD_SCROLLBAR, CMD_DIAL, CMD_GAUGE, CMD_CLOCK and CMD_GRADIENT (average color)
  are drawn as simple shapes in CMD_FGCOLOR/CMD_BGCOLOR (instructions added to
  the display list in SAVE/RESTORE_CONTEXT, without text and 3D effects): the
  widget area has the tag of the context (TAG before the widget), keys of
  CMD_KEYS have the tags of their characters and the pressed key (options) is
  lighter. CMD_TRACK registers the tracking area of a tag (linear: w > h
  horizontal, otherwise vertical; rotary: w = h = 1, angle clockwise from
  straight down; w = h = 0 stops it), REG_TRACKER (309000h, REG_TRACKER_1..4
  in extended mode) is (value << 16) | tag while the touched tag is tracked,
  else 0. CMD_TEXT, CMD_NUMBER, CMD_SPINNER and other commands with known
  arguments are parsed but not executed (warning once; CMD_CALIBRATE returns
  success),
  CMD_LOADIMAGE with data in the FIFO skips the JPEG/PNG image (the end is
  found by the image structure, it is not decoded, warning once),
  commands of the media engine and flash memory and errors (e.g. invalid
  CMD_INFLATE stream) cause coprocessor fault as BT81x: REG_CMD_READ reads FFFh
  (REG_CMDB_SPACE has low bits set), the message is in RAM_ERR_REPORT (309800h),
  the FIFO is not processed until REG_CPURESET (warning).

  Display list: DISPLAY ends the list, instructions are CLEAR/CLEAR_COLOR_A/RGB,
  COLOR_RGB/A, TAG/TAG_MASK/CLEAR_TAG (tag buffer: drawn pixels get the tag
  when TAG_MASK is 1, also transparent ones - ALPHA_FUNC is not modeled; CLEAR
  with the tag bit sets CLEAR_TAG, pixels not cleared have tag 0),
  SCISSOR_XY/SIZE,
  VERTEX_FORMAT (default 4: 1/16 px)/VERTEX_TRANSLATE_X/Y, POINT_SIZE (radius)
  and LINE_WIDTH (half width, units of 1/16 px), BITMAP_SOURCE/HANDLE/LAYOUT
  (line stride in bytes)/SIZE (+_H)/PALETTE_SOURCE, CELL and the cell of
  VERTEX2II (cell n starts at source + n * stride * layout height, e.g. glyphs
  of own fonts), CMD_SETBITMAP (layout instructions of the handle), BEGIN/END with
  primitives BITMAPS(1), POINTS(2, circles), LINES(3), LINE_STRIP(4, round
  ends), RECTS(9, corners rounded by LINE_WIDTH > 1 px: radius LINE_WIDTH, the
  rectangle grows by LINE_WIDTH - 1 px) anti-aliased (edges of POINTS, LINES
  and rounded RECTS),
  and bitmap formats ARGB1555, ARGB4, ARGB2, RGB565, RGB332, L8, L4, L2, L1
  (luminance is alpha of COLOR_RGB), PALETTED565/4444/8 (palette at
  PALETTE_SOURCE), colors of bitmaps are multiplied by COLOR_RGB/COLOR_A;
  CALL/JUMP (instruction index)/RETURN, MACRO (executes REG_MACRO_0/1) and
  SAVE/RESTORE_CONTEXT, COLOR_MASK without color channels (only tags and
  alpha are drawn, e.g. alpha masks: nothing visible, CLEAR keeps colors; a
  partial mask is drawn as all channels, warning once). ALPHA_FUNC/STENCIL_* are
  accepted without effect, BLEND_FUNC other than the default is drawn as default
  (warning once; there is no alpha channel in the frame), bitmap transforms/swizzle
  ignored (warning once), EDGE_STRIP_* and ROM fonts are not drawn. The display
  list is interpreted in Lua, pixels are drawn in C (board API gfx: RAM_G and
  the command FIFO are gfx.memory buffers, the frame with the tag buffer is a
  gfx.canvas, alpha blended) and the changed rows are copied to the panel LCD
  on swap; a swap of the same display list (and macros) without RAM_G changes
  is not rendered again.

  Touch: method touch(x, y [, pressed]) as in the FT5336 model updates the
  first touch point, touch_slot(n, x, y [, pressed]) (n = 0..4) any of the five
  points of the capacitive touch panel. In FT800 compatibility mode
  (REG_CTOUCH_EXTENDED bit 0 set, default) the registers REG_TOUCH_RAW_XY/
  SCREEN_XY/TAG_XY/TAG follow the first point (no calibration: coordinates are
  pixels, REG_TOUCH_TRANSFORM_A..F are not used). In extended mode
  (REG_CTOUCH_EXTENDED = 0) the points are read from REG_CTOUCH_TOUCH0..3_XY
  and REG_CTOUCH4_X/Y, their tags from REG_TOUCH_TAG/TAG_XY and
  REG_TOUCH_TAG1..4/TAG1..4_XY. The tag is looked up in the tag buffer of the
  displayed frame at the touch point (after a touch change and after every
  swap), the optional function opts.tags(x, y) overrides it. No touch reads
  FFFFh (raw) / 8000h (screen) and tag 0, inactive slots of extended mode read
  8000:8000. Touch sets the TOUCH interrupt flag.

  Methods:
    eve:attach(bus, cs)           connect to SPI bus
    eve:touch(x, y [, pressed])   first touch point in pixels of the display
                                  (pressed false/nil releases it)
    eve:touch_slot(n, x, y [, p]) set/release touch point n (0..4, extended mode)
    eve:set_tag(fn)               fn(x, y) -> tag for REG_TOUCH_TAG (nil: tag buffer)
    eve:tag_at(x, y)              tag of the displayed frame at the pixel
    eve:pin(n [, level])          chip GPIO pin 0..3: drive input (level) or
                                  read its level (output: value driven by chip)
    eve:on_gpio(fn)               call fn(pin, level) when an output changes
    eve:backlight()               backlight PWM duty REG_PWM_DUTY (0..255)
    eve:frame()                   frame counter (REG_FRAMES)
    eve:reset()                   reset as POR (registers and memories)
    eve:on_swap(fn)               call fn(frame) after display list is rendered

  Field:
    eve.lcd                       LCD object of the panel (panel.lcd())
------------------------------------------------------------------------------]]

local M = {}

-- memory map (Table 5-1)
local RAM_G_TOP = 0x0FFFFF
local CHIP_ID = 0x0C0000
local DL_BASE, DL_SIZE = 0x300000, 0x2000
local REG_BASE, REG_SIZE = 0x302000, 0x1000
local CMD_BASE, CMD_SIZE = 0x308000, 0x1000

-- register offsets (dword index from RAM_REG)
local R_ID, R_FRAMES, R_CLOCK, R_FREQUENCY = 0x00, 0x01, 0x02, 0x03
local R_CPURESET = 0x08
local R_HCYCLE, R_VCYCLE = 0x0B, 0x10
local R_DLSWAP, R_PCLK = 0x15, 0x1C
local R_GPIO_DIR, R_GPIO, R_GPIOX_DIR, R_GPIOX = 0x24, 0x25, 0x26, 0x27
local R_INT_FLAGS, R_INT_EN, R_INT_MASK = 0x2A, 0x2B, 0x2C
local R_PLAYBACK_LENGTH, R_PLAYBACK_FREQ = 0x2E, 0x30
local R_PLAYBACK_FORMAT, R_PLAYBACK_PLAY = 0x31, 0x33
local R_PWM_DUTY = 0x35
local R_MACRO_0, R_MACRO_1 = 0x36, 0x37
local R_CMD_READ, R_CMD_WRITE, R_CMD_DL = 0x3E, 0x3F, 0x40
local R_TOUCH_MODE, R_CTOUCH_EXT = 0x41, 0x42
local R_RAW_XY, R_SCREEN_XY = 0x47, 0x49
local R_TAG_XY, R_TOUCH_TAG = 0x4A, 0x4B
local R_TAG1_XY = 0x4C -- REG_TOUCH_TAG1_XY, TAG1, ... TAG4_XY, TAG4 (extended mode)
local TRACKER_BASE = 0x309000 -- REG_TRACKER, REG_TRACKER_1..4
local ERR_REPORT, ERR_REPORT_SIZE = 0x309800, 128 -- RAM_ERR_REPORT: message of coprocessor fault
local R_RZTHRESH = 0x46
local R_CTOUCH4_Y, R_CTOUCH4_X = 0x48, 0x5B
local R_CTOUCH2_XY, R_CTOUCH3_XY = 0x63, 0x64
local R_SPI_WIDTH = 0x62
local R_CMDB_SPACE, R_CMDB_WRITE = 0x15D, 0x15E

-- interrupt flags (Table 4-6)
local F_SWAP, F_TOUCH, F_PLAYBACK, F_CMDEMPTY, F_CMDFLAG = 0, 1, 4, 5, 6

-- coprocessor commands (low byte of word FFFFFFxx)
local C_DLSTART, C_SWAP, C_INTERRUPT = 0x00, 0x01, 0x02
local C_BGCOLOR, C_FGCOLOR, C_GRADIENT = 0x09, 0x0A, 0x0B
local C_TEXT, C_BUTTON, C_KEYS, C_PROGRESS = 0x0C, 0x0D, 0x0E, 0x0F
local C_SLIDER, C_SCROLLBAR, C_TOGGLE, C_GAUGE = 0x10, 0x11, 0x12, 0x13
local C_CLOCK, C_CALIBRATE, C_SPINNER, C_STOP = 0x14, 0x15, 0x16, 0x17
local C_MEMCRC, C_REGREAD, C_MEMWRITE, C_MEMSET = 0x18, 0x19, 0x1A, 0x1B
local C_MEMZERO, C_MEMCPY, C_APPEND, C_SNAPSHOT = 0x1C, 0x1D, 0x1E, 0x1F
local C_GETPTR, C_INFLATE, C_LOADIMAGE, C_LOADIDENTITY = 0x23, 0x22, 0x24, 0x26
local C_TRANSLATE, C_SCALE, C_ROTATE, C_SETMATRIX = 0x27, 0x28, 0x29, 0x2A
local C_SETFONT, C_TRACK, C_DIAL, C_NUMBER = 0x2B, 0x2C, 0x2D, 0x2E
local C_SCREENSAVER, C_SKETCH, C_LOGO, C_COLDSTART = 0x2F, 0x30, 0x31, 0x32
local C_GETMATRIX, C_SETROTATE, C_ROMFONT, C_SETBASE = 0x33, 0x36, 0x3F, 0x38
local C_GETPROPS, C_GRADCOLOR, C_SETSCRATCH, C_SYNC = 0x25, 0x34, 0x3C, 0x42
local C_SNAPSHOT2, C_SETFONT2, C_SETBITMAP = 0x37, 0x3B, 0x43
local C_ROTATEAROUND, C_RESETFONTS = 0x51, 0x52
local C_FILLWIDTH = 0x58
local C_NOP = 0x5B
local C_CALIBRATESUB, C_TESTCARD, C_APILEVEL = 0x60, 0x61, 0x63
local C_WAIT, C_RETURN, C_CALLLIST = 0x65, 0x66, 0x67
local C_NEWLIST, C_ENDLIST = 0x68, 0x69

-- argument words after the command word (16-bit arguments are packed in pairs,
-- "S": trailing string, terminated by NUL and padded to a whole word); commands
-- of the media engine are not listed (their payload length is unknown) and
-- cause coprocessor fault
local COPRO_ARGS = {
    [C_TEXT] = "2S",
    [C_BUTTON] = "3S",
    [C_KEYS] = "3S",
    [C_TOGGLE] = "3S",
    [C_NUMBER] = 3,
    [C_GAUGE] = 4,
    [C_CLOCK] = 4,
    [C_SCROLLBAR] = 4,
    [C_SLIDER] = 4,
    [C_PROGRESS] = 4,
    [C_DIAL] = 3,
    [C_GRADIENT] = 4,
    [C_SPINNER] = 2,
    [C_TRACK] = 3,
    [C_TRANSLATE] = 2,
    [C_SCALE] = 2,
    [C_ROTATE] = 1,
    [C_ROTATEAROUND] = 4,
    [C_GETMATRIX] = 6,
    [C_SETFONT] = 2,
    [C_ROMFONT] = 2,
    [C_SETFONT2] = 3,
    [C_SETBITMAP] = 3,
    [C_SKETCH] = 4,
    [C_SNAPSHOT] = 1,
    [C_SNAPSHOT2] = 4,
    [C_SETROTATE] = 1,
    [C_BGCOLOR] = 1,
    [C_FGCOLOR] = 1,
    [C_GRADCOLOR] = 1,
    [C_CALIBRATE] = 1,
    [C_CALIBRATESUB] = 3,
    [C_STOP] = 0,
    [C_LOGO] = 0,
    [C_SCREENSAVER] = 0,
    [C_COLDSTART] = 0,
    [C_LOADIDENTITY] = 0,
    [C_SETMATRIX] = 0,
    [C_FILLWIDTH] = 1,
    [C_NOP] = 0,
    [C_SETBASE] = 1,
    [C_SETSCRATCH] = 1,
    [C_GETPROPS] = 3,
    [C_SYNC] = 0,
    [C_TESTCARD] = 0,
    [C_APILEVEL] = 1,
    [C_RESETFONTS] = 0,
    [C_NEWLIST] = 1,
    [C_ENDLIST] = 0,
    [C_CALLLIST] = 1,
    [C_RETURN] = 0,
}

-- commands executed by the model: (argument words, word of result or nil)
local COPRO_EXEC = {
    [C_DLSTART] = { 0 },
    [C_SWAP] = { 0 },
    [C_INTERRUPT] = { 1 },
    [C_WAIT] = { 1 },
    [C_GETPTR] = { 1, 1 },
    [C_MEMZERO] = { 2 },
    [C_MEMSET] = { 3 },
    [C_MEMCPY] = { 3 },
    [C_MEMCRC] = { 3, 3 },
    [C_REGREAD] = { 2, 2 },
    [C_APPEND] = { 2 },
    [C_MEMWRITE] = { 2 },
    [C_INFLATE] = { 1 },
    [C_LOADIMAGE] = { 2 },
}

local PARTS = { bt817 = 0x17, bt818 = 0x18 }

-- primitives of BEGIN (BT81x Programmers Guide)
local P_BITMAPS, P_POINTS, P_LINES, P_LINE_STRIP, P_RECTS = 1, 2, 3, 4, 9

-- bitmap formats
local F_ARGB1555, F_L1, F_L4, F_L8, F_RGB332, F_ARGB2, F_ARGB4, F_RGB565 = 0, 1, 2, 3, 4, 5, 6, 7
local F_PALETTED, F_PALETTED565, F_PALETTED4444, F_PALETTED8, F_L2 = 8, 14, 15, 16, 17

local DEV = {}
DEV.__index = DEV

local function s15(v) -- sign extend 15 bits (VERTEX2F coordinates)
    if v >= 0x4000 then
        return v - 0x8000
    end
    return v
end

local function s17(v) -- sign extend 17 bits (VERTEX_TRANSLATE, units of 1/16 px)
    if v >= 0x10000 then
        return v - 0x20000
    end
    return v
end

-- standard CRC-32 (result of CMD_MEMCRC)
local crc_table = {}
for i = 0, 255 do
    local c = i
    for _ = 1, 8 do
        if c & 1 == 1 then
            c = (c >> 1) ~ 0xEDB88320
        else
            c = c >> 1
        end
    end
    crc_table[i] = c
end

local function crc32(bytes)
    local crc = 0xFFFFFFFF
    for _, b in ipairs(bytes) do
        crc = (crc >> 8) ~ crc_table[(crc ~ b) & 0xFF]
    end
    return ~crc & 0xFFFFFFFF
end

--------------------------------------------------------------------------------
-- construction and reset
--------------------------------------------------------------------------------

function M.new(opts)
    opts = opts or {}
    local d = setmetatable({}, DEV)
    d.mode = { [0] = true } -- SPI mode 0 only (datasheet)
    d.width = opts.width or 480
    d.height = opts.height or 272
    d.part = opts.part or "bt817"
    assert(PARTS[d.part], "bt817: unknown part '" .. tostring(d.part) .. "'")
    d.tags_cb = opts.tags

    d.lcd = panel.lcd(opts.id or "bt817", d.width, d.height, { format = opts.format, enabled = false })

    d.int_pin = opts.int
    if d.int_pin then
        gpio.set(d.int_pin, true)
    end
    if opts.pd then
        d.pd_pin = opts.pd
        gpio.on_change(opts.pd, function(level)
            if not level then
                d:power_down()
            elseif d.state == "off" then
                print("bt817: PD_N released, power on (sleep)")
                d.state = "sleep" -- core boots after POR
            end
        end)
    end

    d:reset()
    if opts.pd and not gpio.get(opts.pd) then
        d.state = "off"
    end
    return d
end

-- memories are cleared (power on, core off): RAM_G in C (gfx.memory)
function DEV:clear_memory()
    if not self.ram_g then
        self.ram_g = gfx.memory(RAM_G_TOP + 1)
    else
        self.ram_g:fill(0, RAM_G_TOP + 1, 0)
    end
    if not self.ram_cmd then
        self.ram_cmd = gfx.memory(CMD_SIZE) -- command FIFO (ring buffer)
    else
        self.ram_cmd:fill(0, CMD_SIZE, 0)
    end
    self.regs, self.ram_dl = {}, {}
    self.last_render = nil -- memory lost: next swap renders
    self.ram_g:write(CHIP_ID, string.char(0x08, PARTS[self.part], 0x01, 0x00)) -- chip ID (writable)
end

function DEV:reset()
    self:clear_memory()
    self.state = "sleep" -- power on: sleep until ACTIVE
    self.core_lost = false
    self.frames_count, self.display_on, self.fps = 0, false, 60
    self.swaps = 0
    self.t0 = host.time_ms()
    self.cmd_read, self.cmd_write = 0, 0
    self.stream, self.fifo_pending, self.inflate_end = nil, nil, 0
    self.dl_off, self.dl_top = 0, 0
    self.flags, self.int_en, self.int_mask = 0, 0, 0xFF
    self.pins = { [0] = false, false, false, false } -- levels driven to chip inputs
    self.gpio_out = {} -- levels driven by chip outputs
    self.touches = {} -- touch points [n] = {x, y}
    self.trackers = {} -- CMD_TRACK: [tag] = {x, y, w, h}
    self.tracker_regs = { 0, 0, 0, 0, 0 } -- REG_TRACKER, REG_TRACKER_1..4
    self.fgcolor, self.bgcolor = 0x003870, 0x002040 -- CMD_FGCOLOR/CMD_BGCOLOR
    self.copro_wait = nil -- CMD_WAIT: process FIFO after this time
    self.warnings = {}
    if self.play_timer then
        self.play_timer:stop()
    end
    self.play_timer = nil

    local def = { -- reset values (Table 5-2)
        [R_FREQUENCY] = 60000000,
        [R_INT_MASK] = 0xFF,
        [R_GPIO_DIR] = 0x80,
        [R_GPIOX_DIR] = 0x8000,
        [R_GPIOX] = 0x0080,
        [R_PLAYBACK_FREQ] = 0x8000,
        [R_PWM_DUTY] = 128,
        [R_RAW_XY] = 0xFFFFFFFF,
        [R_SCREEN_XY] = 0x80008000,
        [R_TAG_XY] = 0x80008000,
        [R_TOUCH_TAG] = 0,
    }
    def[R_TOUCH_MODE] = 3 -- continuous screening
    def[R_CTOUCH_EXT] = 1 -- FT800 compatibility mode
    def[R_RZTHRESH] = 0xFFFF
    for _, a in ipairs({ 0x54, 0x58 }) do
        def[a] = 0x00010000
    end -- transform A, E
    for a, v in pairs(def) do
        self.regs[a] = v
    end

    self:update_touch_regs()

    -- frame and tag buffer (gfx.canvas: drawing in C), black
    self.canvas = self.canvas or gfx.canvas(self.width, self.height)
    self.canvas:reset()
    self.lcd:enable(false)
end

-- the LCD of the panel shows the display while the chip is active and the
-- display is started (REG_PCLK); frames are counted while the display runs
function DEV:update_display()
    local on = self.state == "active" and ((self.regs[R_PCLK] or 0) & 0xFF) ~= 0
    if on == self.display_on then
        return
    end
    if on then
        local pclk, hc, vc = (self.regs[R_PCLK] or 0) & 0xFF, self.regs[R_HCYCLE] or 0, self.regs[R_VCYCLE] or 0
        local freq = self.regs[R_FREQUENCY] or 60000000
        self.fps = (pclk > 0 and hc > 0 and vc > 0) and freq / (pclk * hc * vc) or 60
        self.frame_t0 = host.time_ms()
    else
        self.frames_count = self:frame()
    end
    self.display_on = on
    self.lcd:enable(on)
end

-- core switched off (PD_N or PWRDOWN): registers, RAM and FIFO are lost
function DEV:core_off()
    self:clear_memory()
    self.cmd_read, self.cmd_write, self.dl_off, self.dl_top = 0, 0, 0, 0
    self.stream, self.fifo_pending = nil, nil
    self.flags = 0
    self.core_lost = true -- reset values after ACTIVE
end

function DEV:power_down()
    if self.state == "off" then
        return
    end
    print("bt817: power down (PD_N)")
    self.state = "off"
    self:core_off()
    self:update_int()
    self:update_display()
end

function DEV:warn_once(key, msg)
    if not self.warnings[key] then
        self.warnings[key] = true
        print("bt817: " .. msg)
    end
end

--------------------------------------------------------------------------------
-- memory and registers (little endian, byte accessible)
--------------------------------------------------------------------------------

function DEV:mem_read(addr)
    local a = addr & 0x3FFFFF
    if a <= RAM_G_TOP then
        return self.ram_g:get(a)
    end
    if a < DL_BASE + DL_SIZE then
        return self.ram_dl[a - DL_BASE] or 0
    end
    if a < REG_BASE + REG_SIZE then
        local v = self:reg_read((a - REG_BASE) >> 2, false)
        return (v >> ((a & 3) * 8)) & 0xFF
    end
    if a < CMD_BASE + CMD_SIZE then
        return self.ram_cmd:get(a - CMD_BASE)
    end
    if a >= ERR_REPORT and a < ERR_REPORT + ERR_REPORT_SIZE then -- message of coprocessor fault
        local msg = self.err_report or ""
        local i = a - ERR_REPORT + 1
        return i <= #msg and msg:byte(i) or 0
    end
    if a >= TRACKER_BASE and a < TRACKER_BASE + 20 then -- REG_TRACKER..REG_TRACKER_4
        local v = self.tracker_regs[((a - TRACKER_BASE) >> 2) + 1] or 0
        return (v >> ((a & 3) * 8)) & 0xFF
    end
    return 0
end

function DEV:mem_write(addr, byte)
    local a = addr & 0x3FFFFF
    if a <= RAM_G_TOP then
        self.ram_g:set(a, byte)
        self.mem_gen = (self.mem_gen or 0) + 1 -- RAM_G changed (render of the same display list)
    elseif a < DL_BASE + DL_SIZE then
        local o = a - DL_BASE
        self.ram_dl[o] = byte & 0xFF
        if o + 1 > self.dl_top then
            self.dl_top = o + 1
        end
    elseif a < REG_BASE + REG_SIZE then
        local off, sh = (a - REG_BASE) >> 2, ((a & 3) * 8)
        if off == R_CMDB_WRITE then -- FIFO port: every byte access appends one byte
            self:fifo_write(string.char(byte & 0xFF)) -- processed at the end of the transaction
        else
            local cur = self:reg_read(off, true) or 0
            self:reg_write(off, (cur & ~(0xFF << sh)) | ((byte & 0xFF) << sh))
        end
    elseif a < CMD_BASE + CMD_SIZE then
        self.ram_cmd:set(a - CMD_BASE, byte)
    end
end

function DEV:reg_read(off, raw)
    if off == R_ID then
        return 0x7C
    end
    if off == R_FRAMES then
        return self:frame() & 0xFFFFFFFF
    end
    if off == R_CLOCK then
        return math.floor((host.time_ms() - self.t0) * (self.regs[R_FREQUENCY] or 60000000) / 1000)
    end
    if off == R_CMDB_SPACE then
        if self.fault then -- coprocessor fault: REG_CMD_READ is FFFh (space has low bits set)
            return (0xFFF - self.cmd_write - 4) & 0xFFF
        end
        local pending = (self.cmd_write - self.cmd_read) & (CMD_SIZE - 1)
        return math.max(CMD_SIZE - pending - 4, 0)
    end
    if off == R_CMD_READ then
        return self.fault and 0xFFF or self.cmd_read
    end -- live FIFO pointers
    if off == R_CMD_WRITE then
        return self.cmd_write
    end
    if off == R_GPIO and not raw then -- input pins reflect external drive
        local v, dir = self.regs[off] or 0, self.regs[R_GPIO_DIR] or 0x80
        for n = 0, 3 do
            if (dir >> n) & 1 == 0 then
                v = (v & ~(1 << n)) | ((self.pins[n] and 1 or 0) << n)
            end
        end
        return v
    end
    local v = self.regs[off] or 0
    if off == R_INT_FLAGS then -- live flags, cleared at end of reading transaction
        if not raw then
            self.flags_read = true
        end
        return self.flags
    end
    return v
end

function DEV:reg_write(off, v)
    v = v & 0xFFFFFFFF
    if off == R_ID or off == R_FRAMES or off == R_CLOCK then
        return
    end -- r/o
    self.regs[off] = v

    if off == R_DLSWAP and (v == 1 or v == 2) then
        self:swap()
    elseif off == R_INT_FLAGS then -- write 1 clears the flag bit
        self.flags = self.flags & ~v & 0xFFFFFFFF
        self:update_int()
    elseif off == R_INT_EN or off == R_INT_MASK then
        self.int_en, self.int_mask = self.regs[R_INT_EN] or 0, self.regs[R_INT_MASK] or 0xFF
        self:update_int()
    elseif off == R_CMD_WRITE then -- processed at the end of the transaction
        self.cmd_write = v & (CMD_SIZE - 1)
        self.fifo_pending = true
    elseif off == R_CMD_READ then
        self.cmd_read = v & (CMD_SIZE - 1)
    elseif off == R_CPURESET and (v & 1) == 1 then -- coprocessor reset: FIFO is empty, fault cleared
        self.cmd_read, self.cmd_write, self.stream, self.copro_wait = 0, 0, nil, nil
        self.dl_off = 0
        self.fault, self.err_report = nil, nil
    elseif off == R_PCLK then
        self:update_display()
    elseif off == R_CMD_DL then
        self.dl_off = v
    elseif off == R_PLAYBACK_PLAY and v == 1 then
        self:start_playback()
    elseif off == R_SPI_WIDTH and (v & 3) ~= 0 then
        self:warn_once("spi_width", "Dual/Quad channel mode is not supported, Single mode kept")
    elseif off == R_CTOUCH_EXT or off == R_TOUCH_MODE then
        self:update_touch_regs() -- touch registers follow the mode
    elseif off == R_GPIO or off == R_GPIOX or off == R_GPIO_DIR then
        self:update_gpio_outputs()
    end
end

--------------------------------------------------------------------------------
-- interrupts (INT_N open drain, active low)
--------------------------------------------------------------------------------

function DEV:set_flag(f)
    self.flags = self.flags | (1 << f)
    self:update_int()
end

function DEV:clear_flags_read() -- REG_INT_FLAGS clears when read
    if self.flags_read then
        self.flags_read = nil
        self.flags = 0
        self:update_int()
    end
end

function DEV:update_int()
    if not self.int_pin then
        return
    end
    local on = (self.int_en & 1) == 1 and (self.flags & ~self.int_mask & 0xFFFFFFFF) ~= 0
    gpio.set(self.int_pin, not on) -- open drain: released = high
end

--------------------------------------------------------------------------------
-- SPI interface (mode 0): select/transfer/deselect
--------------------------------------------------------------------------------

function DEV:select()
    self.tx = "first" -- state of the transaction
    self.wr = false
    self.addr_part = nil
end

function DEV:transfer(byte)
    if self.state == "off" then
        return 0xFF
    end -- PD_N low: no response

    local t = self.tx
    if t == "cmd" then -- host command parameters
        table.insert(self.cmd_bytes, byte)
        if #self.cmd_bytes >= 3 then
            local b1, b2 = self.cmd_bytes[1], self.cmd_bytes[2]
            self.cmd_bytes, self.tx = nil, nil
            self:run_host_command(b1, b2)
        end
        return 0xFF
    end

    if t == "first" then
        local prefix = (byte >> 6) & 3
        if prefix == 1 then -- host command 0b01CCCCCC
            self.cmd_bytes = { byte }
            self.tx = "cmd"
        elseif prefix == 2 then -- memory write 0b10AAAAAA
            self.wr, self.addr_part, self.tx = true, (byte & 0x3F) << 16, "addr2"
        elseif prefix == 0 then -- memory read 0b00AAAAAA
            if byte == 0 and self.state ~= "active" then
                self:activate() -- dummy read from address 0 wakes up
            end
            self.wr, self.addr_part, self.tx = false, (byte & 0x3F) << 16, "addr2"
        else -- prefix 0b11 undefined
            self.tx = nil
        end
        return 0xFF
    end

    if t == "addr2" then
        self.addr_part = self.addr_part | ((byte & 0xFF) << 8)
        self.tx = "addr3"
        return 0xFF
    elseif t == "addr3" then
        self.addr = self.addr_part | (byte & 0xFF)
        self.tx = self.wr and "data" or "dummy"
        return 0xFF
    elseif t == "dummy" then -- dummy byte before read data
        self.tx = "data"
        return 0xFF
    end

    if t == "data" then -- streaming write/read
        local a = self.addr
        if a >= CMD_BASE and a < CMD_BASE + CMD_SIZE then -- RAM_CMD: address wraps within the ring
            self.addr = CMD_BASE + ((a - CMD_BASE + 1) & (CMD_SIZE - 1))
        elseif not (self.wr and (a & ~3) == REG_BASE + R_CMDB_WRITE * 4) then -- burst to CMDB_WRITE: same address
            self.addr = (a + 1) & 0x3FFFFF -- continuous address
        end
        if self.wr then
            self:mem_write(a, byte)
            return 0xFF
        end
        return self:mem_read(a)
    end
    return 0xFF
end

-- whole SPI transfer at once (called by the board engine instead of
-- transfer() for every byte): data phases of RAM_G writes/reads and of the
-- REG_CMDB_WRITE burst are handled in bulk, other bytes by transfer()
local CHUNK = 4096
function DEV:transfer_block(data)
    local n, i, res = #data, 1, {}
    while i <= n do
        if self.tx == "data" and self.state ~= "off" then
            local cnt, a = n - i + 1, self.addr
            if self.wr and (a & ~3) == REG_BASE + R_CMDB_WRITE * 4 then -- FIFO burst
                self:fifo_write(i == 1 and data or data:sub(i))
                res[#res + 1] = string.rep("\xFF", cnt)
                break
            elseif a >= CMD_BASE and a < CMD_BASE + CMD_SIZE then -- RAM_CMD (address wraps within the ring)
                local pos = a - CMD_BASE
                if self.wr then -- data only, the coprocessor sees it after REG_CMD_WRITE update
                    pos = self:fifo_put(pos, i == 1 and data or data:sub(i))
                    res[#res + 1] = string.rep("\xFF", cnt)
                else
                    res[#res + 1] = self:fifo_get(pos, cnt)
                    pos = (pos + cnt) & (CMD_SIZE - 1)
                end
                self.addr = CMD_BASE + pos
                break
            elseif a + cnt - 1 <= RAM_G_TOP then -- RAM_G (continuous address)
                if self.wr then
                    self.ram_g:write(a, i == 1 and data or data:sub(i))
                    self.mem_gen = (self.mem_gen or 0) + 1
                    res[#res + 1] = string.rep("\xFF", cnt)
                else
                    res[#res + 1] = self.ram_g:read(a, cnt)
                end
                self.addr = (a + cnt) & 0x3FFFFF
                break
            end
        end
        res[#res + 1] = string.char(self:transfer(data:byte(i)) or 0xFF) -- header, registers, ...
        i = i + 1
    end
    return table.concat(res)
end

function DEV:deselect()
    if self.cmd_bytes then -- command with early CS release
        while #self.cmd_bytes < 3 do
            table.insert(self.cmd_bytes, 0)
        end
        local b1, b2 = self.cmd_bytes[1], self.cmd_bytes[2]
        self.cmd_bytes, self.tx = nil, nil
        self:run_host_command(b1, b2)
    else
        self.tx = nil
    end
    if self.fifo_pending then
        self:process_fifo()
    end
    self:clear_flags_read() -- read of REG_INT_FLAGS clears
end

--------------------------------------------------------------------------------
-- host commands (Table 4-5)
--------------------------------------------------------------------------------

function DEV:activate()
    if self.state ~= "active" then
        if self.core_lost then
            self:reset()
        end -- after PWRDOWN/PD_N: reset values
        print("bt817: active")
        self.state = "active"
        self:update_display()
    end
end

function DEV:run_host_command(b1, b2)
    local cmd = b1 & 0xFF
    if cmd == 0x00 then -- ACTIVE (transaction of zeros)
        self:activate()
    elseif cmd == 0x41 then -- STANDBY
        self.state = "standby"
        self:update_display()
    elseif cmd == 0x42 then -- SLEEP
        self.state = "sleep"
        self:update_display()
    elseif cmd == 0x43 or cmd == 0x50 then -- PWRDOWN (core off, SPI alive)
        print("bt817: power down (PWRDOWN)")
        self:core_off() -- ACTIVE wakes up, registers lost
        self.state = "pwrdown"
        self:update_int()
        self:update_display()
    elseif cmd == 0x44 then -- CLKEXT: external crystal oscillator
        self.clock_ext = true
    elseif cmd == 0x48 then -- CLKINT: internal oscillator
        self.clock_ext = false
    elseif cmd == 0x61 or cmd == 0x62 then -- CLKSEL: Byte2[5:0] is the multiplier of
        local m = b2 & 0x3F -- the oscillator (0 default, 2..6 times)
        self.regs[R_FREQUENCY] = (m >= 2 and m <= 6) and m * 12000000 or 60000000
    elseif cmd == 0x68 then -- RST_PULSE: as POR, power mode kept
        local st = self.state
        self:reset()
        self.state = st
        self:update_display()
    elseif cmd == 0x70 or cmd == 0x71 then -- PINDRIVE, PIN_PD_STATE: ignored
    else -- PIN_PD_STATE and others
        self:warn_once("hcmd" .. cmd, string.format("host command %02Xh ignored", cmd))
    end
end

--------------------------------------------------------------------------------
-- command FIFO (RAM_CMD) and coprocessor commands
--------------------------------------------------------------------------------

function DEV:fifo_u32(pos)
    pos = pos & (CMD_SIZE - 1)
    if pos <= CMD_SIZE - 4 then
        return string.unpack("<I4", self.ram_cmd:read(pos, 4))
    end
    local v = 0
    for i = 0, 3 do
        v = v | (self.ram_cmd:get((pos + i) & (CMD_SIZE - 1)) << (i * 8))
    end
    return v
end

-- data to the FIFO ring at position pos, returns the next position
function DEV:fifo_put(pos, data)
    local k, n = 1, #data
    while k <= n do
        local m = math.min(n - k + 1, CMD_SIZE - pos)
        self.ram_cmd:write(pos, data:sub(k, k + m - 1))
        pos, k = (pos + m) & (CMD_SIZE - 1), k + m
    end
    return pos
end

-- n bytes of the FIFO ring from position pos
function DEV:fifo_get(pos, n)
    local parts = {}
    while n > 0 do
        local m = math.min(n, CMD_SIZE - pos)
        parts[#parts + 1] = self.ram_cmd:read(pos, m)
        pos, n = (pos + m) & (CMD_SIZE - 1), n - m
    end
    return table.concat(parts)
end

function DEV:fifo_string(pos) -- string argument (without NUL)
    local t = {}
    for n = 0, CMD_SIZE - 1 do
        local b = self.ram_cmd:get((pos + n) & (CMD_SIZE - 1))
        if b == 0 then
            break
        end
        t[#t + 1] = string.char(b)
    end
    return table.concat(t)
end

function DEV:dl_word(w) -- append instruction to RAM_DL (REG_CMD_DL)
    for i = 0, 3 do
        if self.dl_off < DL_SIZE then
            self.ram_dl[self.dl_off] = (w >> (i * 8)) & 0xFF
            self.dl_off = self.dl_off + 1
            if self.dl_off > self.dl_top then
                self.dl_top = self.dl_off
            end
        end
    end
end

function DEV:fifo_skip_string(pos) -- bytes until NUL, padded to word
    local n = 0
    repeat
        n = n + 1
    until self.ram_cmd:get((pos + n - 1) & (CMD_SIZE - 1)) == 0 or n > CMD_SIZE
    return (n + 3) & ~3
end

function DEV:fifo_avail() -- bytes pending in FIFO
    return (self.cmd_write - self.cmd_read) & (CMD_SIZE - 1)
end

-- data written to REG_CMDB_WRITE: when the FIFO is full, the coprocessor
-- processes it during the transfer (as the chip does, e.g. a burst of
-- CMD_INFLATE data longer than REG_CMDB_SPACE), data without space are lost
function DEV:fifo_write(data)
    if self.fault then
        return -- coprocessor stopped: data are ignored
    end
    local k, n = 1, #data
    while k <= n do
        local free = CMD_SIZE - 4 - self:fifo_avail()
        if free <= 0 then
            self:process_fifo()
            free = CMD_SIZE - 4 - self:fifo_avail()
            if self.fault then
                return
            end
        end
        if free <= 0 then
            self:warn_once("overflow", "command FIFO overflow: data written without REG_CMDB_SPACE are lost")
            return
        end
        local m = math.min(free, n - k + 1)
        self.cmd_write = self:fifo_put(self.cmd_write, data:sub(k, k + m - 1))
        self.fifo_pending = true
        k = k + m
    end
end

-- coprocessor fault (as BT81x): REG_CMD_READ reads FFFh, message in
-- RAM_ERR_REPORT, the FIFO is not processed until REG_CPURESET
function DEV:copro_fault(msg)
    self.fault, self.stream = true, nil
    self.err_report = msg:sub(1, ERR_REPORT_SIZE - 1)
    print("bt817: coprocessor fault: " .. msg .. " (REG_CMD_READ = FFFh, recovery by REG_CPURESET)")
end

-- bytes of a coprocessor command including its word (MEMWRITE/INFLATE: header,
-- their data are processed as a stream), nil when the command is not written
-- completely yet, second value true for unknown commands (coprocessor fault)
function DEV:fifo_cmd_size(c)
    local avail = self:fifo_avail()
    local exec = COPRO_EXEC[c]
    if exec then
        local need = 4 + exec[1] * 4
        if avail < need then
            return nil
        end
        return need
    end
    local spec = COPRO_ARGS[c]
    if not spec then
        return nil, true
    end -- unknown/media command: fault
    local nargs = type(spec) == "number" and spec or tonumber((spec:sub(1, 1)))
    local need = 4 + nargs * 4
    if avail < need then
        return nil
    end
    if type(spec) == "string" then -- trailing string must be terminated
        local base = (self.cmd_read + need) & (CMD_SIZE - 1) -- first byte of the string
        for i = 0, avail - need - 1 do -- look for NUL byte in written area
            if self.ram_cmd:get((base + i) & (CMD_SIZE - 1)) == 0 then
                local total = need + ((i + 4) & ~3)
                if avail < total then
                    return nil
                end -- padding not written yet
                return total
            end
        end
        return nil -- string not complete yet: wait
    end
    return need
end

-- data of MEMWRITE/INFLATE following in the FIFO: processes written bytes,
-- returns false when more data are needed
function DEV:stream_step()
    local st = self.stream
    if st.kind == "memwrite" then
        local n = math.min(st.left, self:fifo_avail())
        if n > 0 and st.ptr >= 0 and st.ptr + n - 1 <= RAM_G_TOP then -- bulk copy FIFO -> RAM_G
            local ptr = st.ptr
            self.ram_g:write(ptr, self:fifo_get(self.cmd_read, n))
            self.cmd_read = (self.cmd_read + n) & (CMD_SIZE - 1)
            st.ptr, st.left, st.consumed = ptr + n, st.left - n, st.consumed + n
            self.mem_gen = (self.mem_gen or 0) + 1
        end
        while st.left > 0 do
            if self:fifo_avail() == 0 then
                return false
            end
            self:mem_write(st.ptr, self.ram_cmd:get(self.cmd_read))
            st.ptr, st.left, st.consumed = st.ptr + 1, st.left - 1, st.consumed + 1
            self.cmd_read = (self.cmd_read + 1) & (CMD_SIZE - 1)
        end
        st.kind = "pad"
    elseif st.kind == "inflate" then
        -- available bytes are decoded (C: gfx.inflater), bytes of the stream are
        -- consumed; the end of the stream is known after decoding
        local avail = self:fifo_avail()
        if avail == 0 then
            return false
        end
        local state, used, out, adler_ok = st.z:feed(self.ram_g, self:fifo_get(self.cmd_read, avail))
        if out > st.out then
            st.out = out
            self.mem_gen = (self.mem_gen or 0) + 1
        end
        if state == "error" then
            self:copro_fault("inflate: " .. tostring(used))
            return false
        end
        self.cmd_read = (self.cmd_read + used) & (CMD_SIZE - 1)
        st.consumed = st.consumed + used
        if state == "more" then
            return false
        end
        if not adler_ok then
            self:warn_once("adler", "CMD_INFLATE: Adler-32 of the stream does not match")
        end
        self.inflate_end = st.ptr + st.out
        st.kind = "pad"
    elseif st.kind == "image" then -- CMD_LOADIMAGE: the image is skipped
        local ok, err = self:image_step(st)
        if err then
            self:copro_fault("loadimage: " .. err)
            return false
        elseif not ok then
            return false
        end
        st.kind = "pad"
    end
    while st.consumed % 4 ~= 0 do -- data are padded to words
        if self:fifo_avail() == 0 then
            return false
        end
        self.cmd_read = (self.cmd_read + 1) & (CMD_SIZE - 1)
        st.consumed = st.consumed + 1
    end
    self.stream = nil
    return true
end

-- JPEG/PNG data of CMD_LOADIMAGE following in the FIFO: the end of the image
-- is found by its structure (PNG chunks to IEND, JPEG segments and entropy
-- coded data to EOI), the image is not decoded; returns true at the end, false
-- when more data are needed, nil and message for invalid data
function DEV:image_step(st)
    local function avail()
        return self:fifo_avail()
    end
    local function peek(n)
        return self:fifo_get(self.cmd_read, n)
    end
    local function consume(n)
        self.cmd_read = (self.cmd_read + n) & (CMD_SIZE - 1)
        st.consumed = st.consumed + n
    end
    while true do
        if st.skip and st.skip > 0 then -- bytes of chunk/segment
            local n = math.min(st.skip, avail())
            if n == 0 then
                return false
            end
            consume(n)
            st.skip = st.skip - n
            if st.skip > 0 then
                return false
            end
        end
        local ps = st.ps
        if ps == "sig" then
            if avail() < 8 then
                return false
            end
            local h = peek(8)
            if h == "\x89PNG\r\n\x1A\n" then
                consume(8)
                st.ps = "png"
            elseif h:sub(1, 2) == "\xFF\xD8" then
                consume(2)
                st.ps = "jpeg"
            else
                return nil, "unknown image format"
            end
        elseif ps == "png" then -- chunk: length, type, data, CRC
            if avail() < 8 then
                return false
            end
            local len, typ = string.unpack(">I4c4", peek(8))
            consume(8)
            st.skip = len + 4
            if typ == "IEND" then
                st.ps = "end"
            end
        elseif ps == "jpeg" then -- marker
            if avail() < 2 then
                return false
            end
            local b1, b2 = peek(2):byte(1, 2)
            if b1 ~= 0xFF then
                return nil, "invalid JPEG marker"
            end
            if b2 == 0xFF then -- fill byte
                consume(1)
            elseif b2 == 0xD9 then -- EOI
                consume(2)
                return true
            elseif b2 == 0x01 or (b2 >= 0xD0 and b2 <= 0xD8) then -- markers without length
                consume(2)
            else
                if avail() < 4 then
                    return false
                end
                local len = string.unpack(">I2", peek(4), 3)
                if len < 2 then
                    return nil, "invalid JPEG segment"
                end
                consume(4)
                st.skip = len - 2
                if b2 == 0xDA then -- SOS: entropy coded data follow
                    st.ps = "scan"
                end
            end
        elseif ps == "scan" then -- entropy coded data to marker (not FF00, RSTn)
            local n = avail()
            if n == 0 then
                return false
            end
            local data = peek(n)
            local i, found = 1, false
            while true do
                local k = data:find("\xFF", i, true)
                if not k then
                    consume(n)
                    return false
                end
                if k == n then -- marker byte not written yet
                    consume(k - 1)
                    return false
                end
                local b = data:byte(k + 1)
                if b == 0x00 or (b >= 0xD0 and b <= 0xD7) then
                    i = k + 2
                else
                    consume(k - 1)
                    found = true
                    break
                end
            end
            if found then
                st.ps = "jpeg"
            end
        else -- "end" (PNG after IEND)
            return true
        end
    end
end

function DEV:start_inflate(ptr)
    self.stream = { kind = "inflate", ptr = ptr, out = 0, consumed = 0, z = gfx.inflater(ptr) }
end

function DEV:process_fifo()
    self.fifo_pending = nil
    if self.fault then
        return -- stopped until REG_CPURESET
    end
    if self.copro_wait then -- CMD_WAIT delays the rest of the FIFO
        if host.time_ms() < self.copro_wait then
            if not self.wait_scheduled then
                self.wait_scheduled = true
                local ms = math.max(self.copro_wait - host.time_ms(), 1)
                host.timer(ms, function()
                    self.wait_scheduled = nil
                    self:process_fifo()
                end)
            end
            return
        end
        self.copro_wait = nil
    end
    local guard = 0
    while guard < 100000 do
        guard = guard + 1
        if self.stream then
            if not self:stream_step() then
                break
            end
        else
            if self:fifo_avail() < 4 then
                break
            end -- partial word: wait for more
            local pos = self.cmd_read -- byte position of the command
            local w = self:fifo_u32(pos)
            if (w & 0xFFFFFF00) == 0xFFFFFF00 then
                local c = w & 0xFF
                local need, drain_now = self:fifo_cmd_size(c)
                if drain_now then -- unusable command: coprocessor fault
                    self:copro_fault(string.format("unsupported command %02Xh", c))
                    break
                elseif need then
                    self:coprocessor(c, pos) -- consumes its words from FIFO
                    if self.fault then
                        break
                    end
                    if self.copro_wait then -- rest of the FIFO after CMD_WAIT
                        self.regs[R_CMD_DL] = self.dl_off
                        self:process_fifo()
                        return
                    end
                else -- arguments incomplete: wait
                    break
                end
            else -- display list instruction -> RAM_DL
                self:dl_word(w)
                self.cmd_read = (self.cmd_read + 4) & (CMD_SIZE - 1)
            end
        end
    end
    self.regs[R_CMD_DL] = self.dl_off
    if self.cmd_read == self.cmd_write and not self.stream then
        self:set_flag(F_CMDEMPTY)
    end
    self:clear_flags_read() -- REGREAD could read INT_FLAGS
end

-- widgets of the coprocessor are drawn as simple shapes (no fonts, no 3D
-- effects): the instructions are added to the display list in
-- SAVE_CONTEXT/RESTORE_CONTEXT, so they have the tag of the context (TAG
-- before the widget) and touch of the widget area is reported by REG_TOUCH_TAG;
-- CMD_KEYS sets the tag of every key to its character as the chip
local function s16(v)
    v = v & 0xFFFF
    return v >= 0x8000 and v - 0x10000 or v
end

local FONT_H = { -- heights of ROM fonts 16..34 (toggle height)
    [16] = 8,
    [17] = 8,
    [18] = 16,
    [19] = 16,
    [20] = 13,
    [21] = 17,
    [22] = 20,
    [23] = 22,
    [24] = 29,
    [25] = 38,
    [26] = 16,
    [27] = 20,
    [28] = 25,
    [29] = 28,
    [30] = 36,
    [31] = 49,
    [32] = 63,
    [33] = 83,
    [34] = 108,
}

local function lighter(rgb)
    local r, g, b = (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF
    return (math.min(r + 64, 255) << 16) | (math.min(g + 64, 255) << 8) | math.min(b + 64, 255)
end

local Draw = {} -- display list builder of widgets
Draw.__index = Draw

local function vtx(x, y) -- VERTEX2F in 1/16 px
    return 0x40000000 | ((math.floor(x * 16 + 0.5) & 0x7FFF) << 15) | (math.floor(y * 16 + 0.5) & 0x7FFF)
end

function Draw:begin()
    self.dev:dl_word(0x22000000) -- SAVE_CONTEXT
    self.dev:dl_word(0x27000004) -- VERTEX_FORMAT(4)
    return self
end
function Draw:finish()
    self.dev:dl_word(0x23000000) -- RESTORE_CONTEXT
end
function Draw:color(rgb)
    self.dev:dl_word(0x04000000 | (rgb & 0xFFFFFF))
end
function Draw:tag(t)
    self.dev:dl_word(0x03000000 | (t & 0xFF))
end
function Draw:rect(x0, y0, x1, y1, rgb) -- inclusive corners in pixels
    self:color(rgb)
    self.dev:dl_word(0x1F000000 | P_RECTS)
    self.dev:dl_word(vtx(x0, y0))
    self.dev:dl_word(vtx(x1, y1))
    self.dev:dl_word(0x21000000)
end
function Draw:disc(x, y, r, rgb)
    self:color(rgb)
    self.dev:dl_word(0x0D000000 | (math.floor(r * 16) & 0x1FFF)) -- POINT_SIZE
    self.dev:dl_word(0x1F000000 | P_POINTS)
    self.dev:dl_word(vtx(x, y))
    self.dev:dl_word(0x21000000)
end
function Draw:line(x0, y0, x1, y1, half, rgb)
    self:color(rgb)
    self.dev:dl_word(0x0E000000 | (math.floor(half * 16) & 0xFFF)) -- LINE_WIDTH
    self.dev:dl_word(0x1F000000 | P_LINES)
    self.dev:dl_word(vtx(x0, y0))
    self.dev:dl_word(vtx(x1, y1))
    self.dev:dl_word(0x21000000)
end

-- point at angle a (0..1 of full turn) clockwise from straight down, distance r
local function polar(x, y, a, r)
    local t = a * 2 * math.pi
    return x - math.sin(t) * r, y + math.cos(t) * r
end

-- bar of slider/progress/scrollbar: from..to are parts 0..1 of the length
local function bar(d, x, y, w, h, from, to, rgb)
    if w >= h then
        d:rect(x + w * from, y, x + w * to - 1, y + h - 1, rgb)
    else
        d:rect(x, y + h * from, x + w - 1, y + h * to - 1, rgb)
    end
end

function DEV:widget(c, a, str)
    local fg, bg = self.fgcolor, self.bgcolor
    local d = setmetatable({ dev = self }, Draw):begin()
    local x, y = s16(a[1]), s16(a[1] >> 16)
    if c == C_BUTTON then -- x, y, w, h, font, options, string
        local w, h = s16(a[2]), s16(a[2] >> 16)
        d:rect(x, y, x + w - 1, y + h - 1, fg)
    elseif c == C_KEYS then -- x, y, w, h, font, options (low byte: pressed key), string
        local w, h, pressed = s16(a[2]), s16(a[2] >> 16), (a[3] >> 16) & 0xFF
        local n = #str
        if n > 0 then
            local gap = w >= n * 4 and 3 or 0
            local kw = (w - gap * (n - 1)) / n
            for k = 1, n do
                local ch = str:byte(k)
                local kx = x + (k - 1) * (kw + gap)
                d:tag(ch)
                d:rect(kx, y, kx + kw - 1, y + h - 1, ch == pressed and lighter(fg) or fg)
            end
        end
    elseif c == C_TOGGLE then -- x, y, w, font, options, state, string
        local w, font, state = s16(a[2]), a[2] >> 16, (a[3] >> 16) & 0xFFFF
        local h = FONT_H[font] or 16
        d:rect(x, y, x + w - 1, y + h - 1, bg)
        d:disc(x + w * state / 65535, y + h / 2, h / 2 + 1, fg)
    elseif c == C_SLIDER or c == C_PROGRESS then -- x, y, w, h, options, val, range
        local w, h, val, range = s16(a[2]), s16(a[2] >> 16), (a[3] >> 16) & 0xFFFF, a[4] & 0xFFFF
        local f = range > 0 and math.min(val / range, 1) or 0
        bar(d, x, y, w, h, 0, 1, bg)
        bar(d, x, y, w, h, 0, f, fg)
        if c == C_SLIDER then -- knob
            local r = math.min(w, h)
            if w >= h then
                d:disc(x + w * f, y + h / 2, r, fg)
            else
                d:disc(x + w / 2, y + h * f, r, fg)
            end
        end
    elseif c == C_SCROLLBAR then -- x, y, w, h, options, val, size, range
        local w, h, val = s16(a[2]), s16(a[2] >> 16), (a[3] >> 16) & 0xFFFF
        local size, range = a[4] & 0xFFFF, (a[4] >> 16) & 0xFFFF
        bar(d, x, y, w, h, 0, 1, bg)
        if range > 0 then
            bar(d, x, y, w, h, math.min(val / range, 1), math.min((val + size) / range, 1), fg)
        end
    elseif c == C_DIAL then -- x, y, r, options, val
        local r, val = s16(a[2]), a[3] & 0xFFFF
        d:disc(x, y, r, fg)
        local px, py = polar(x, y, val / 65536, r * 0.8)
        d:line(x, y, px, py, math.max(r / 16, 1), bg)
    elseif c == C_GAUGE then -- x, y, r, options, major, minor, val, range
        local r, val, range = s16(a[2]), a[4] & 0xFFFF, (a[4] >> 16) & 0xFFFF
        d:disc(x, y, r, bg)
        local f = range > 0 and math.min(val / range, 1) or 0
        local px, py = polar(x, y, 0.125 + 0.75 * f, r * 0.85)
        d:line(x, y, px, py, math.max(r / 24, 1), fg)
    elseif c == C_CLOCK then -- x, y, r, options, h, m, s, ms
        local r, hh, mm = s16(a[2]), a[3] & 0xFFFF, (a[3] >> 16) & 0xFFFF
        d:disc(x, y, r, bg)
        local hx, hy = polar(x, y, 0.5 + ((hh % 12) + mm / 60) / 12, r * 0.5)
        local mx, my = polar(x, y, 0.5 + mm / 60, r * 0.8)
        d:line(x, y, hx, hy, math.max(r / 16, 1), fg)
        d:line(x, y, mx, my, math.max(r / 24, 1), fg)
    elseif c == C_GRADIENT then -- x0, y0, rgb0, x1, y1, rgb1: average color in the scissor area
        local c0, c1 = a[2], a[4]
        local avg = 0
        for sh = 0, 16, 8 do
            avg = avg | (((((c0 >> sh) & 0xFF) + ((c1 >> sh) & 0xFF)) // 2) << sh)
        end
        d:rect(0, 0, 1023, 1023, avg)
    end
    d:finish()
end

-- widgets drawn by DEV:widget() and commands executed without drawing
local WIDGETS = {
    [C_BUTTON] = true,
    [C_KEYS] = true,
    [C_TOGGLE] = true,
    [C_SLIDER] = true,
    [C_PROGRESS] = true,
    [C_SCROLLBAR] = true,
    [C_DIAL] = true,
    [C_GAUGE] = true,
    [C_CLOCK] = true,
    [C_GRADIENT] = true,
}

function DEV:coprocessor(c, pos)
    local read = { pos = (pos + 4) & (CMD_SIZE - 1) } -- next argument word
    local function arg()
        local v = self:fifo_u32(read.pos)
        read.pos = (read.pos + 4) & (CMD_SIZE - 1)
        return v
    end
    local function finish() -- consume the processed words
        self.cmd_read = read.pos
    end
    local function result(n, v) -- write result to argument word n (1..)
        local p = pos + n * 4
        for i = 0, 3 do
            self.ram_cmd:set((p + i) & (CMD_SIZE - 1), (v >> (i * 8)) & 0xFF)
        end
    end

    if c == C_DLSTART then
        self.dl_off = 0
        finish()
    elseif c == C_SWAP then
        finish()
        self:swap()
    elseif c == C_INTERRUPT then
        local ms = arg()
        finish()
        host.timer(math.max(ms, 1), function()
            self:set_flag(F_CMDFLAG)
        end)
    elseif c == C_WAIT then -- delay of the rest of the FIFO (us in BT817)
        local us = arg()
        finish()
        self.copro_wait = host.time_ms() + us / 1000
    elseif c == C_MEMZERO or c == C_MEMSET then -- (ptr, num) / (ptr, value, num) - value in low byte
        local ptr, vn = arg(), arg()
        local n = vn
        if c == C_MEMSET then
            n = arg()
        else
            vn = 0
        end
        finish()
        if ptr >= 0 and ptr + n - 1 <= RAM_G_TOP then
            self.ram_g:fill(ptr, n, vn & 0xFF)
            self.mem_gen = (self.mem_gen or 0) + 1
        else
            for i = 0, n - 1 do
                self:mem_write(ptr + i, vn & 0xFF)
            end
        end
    elseif c == C_MEMCPY then
        local dst, src, n = arg(), arg(), arg()
        finish()
        if dst >= 0 and src >= 0 and dst + n - 1 <= RAM_G_TOP and src + n - 1 <= RAM_G_TOP then
            self.ram_g:copy(dst, src, n)
            self.mem_gen = (self.mem_gen or 0) + 1
        else
            for i = 0, n - 1 do
                self:mem_write(dst + i, self:mem_read(src + i))
            end
        end
    elseif c == C_MEMWRITE then -- data follow in the FIFO stream
        local ptr, n = arg(), arg()
        finish()
        self.stream = { kind = "memwrite", ptr = ptr, left = n, consumed = 0 }
    elseif c == C_INFLATE then -- zlib data follow in the FIFO stream
        local ptr = arg()
        finish()
        self:start_inflate(ptr)
    elseif c == C_LOADIMAGE then -- JPEG/PNG data follow in the FIFO stream (skipped)
        local ptr, opts = arg(), arg()
        finish()
        if (opts & 0x50) ~= 0 then -- OPT_MEDIAFIFO, OPT_FLASH
            self:copro_fault("CMD_LOADIMAGE from media FIFO or flash is not supported")
            return
        end
        self:warn_once("loadimage", "CMD_LOADIMAGE: JPEG/PNG image is skipped (not decoded)")
        self.inflate_end = ptr
        self.stream = { kind = "image", ptr = ptr, ps = "sig", consumed = 0 }
    elseif c == C_APPEND then -- memory to display list
        local ptr, n = arg(), arg()
        finish()
        for i = 0, n - 1 do
            if self.dl_off < DL_SIZE then
                self.ram_dl[self.dl_off] = self:mem_read(ptr + i)
                self.dl_off = self.dl_off + 1
                if self.dl_off > self.dl_top then
                    self.dl_top = self.dl_off
                end
            end
        end
    elseif c == C_GETPTR then -- end of data inflated by CMD_INFLATE
        arg()
        finish()
        result(1, self.inflate_end)
    elseif c == C_REGREAD then -- (ptr, result): 32-bit value of register
        local ptr = arg()
        arg()
        finish()
        local v = 0
        for i = 0, 3 do
            v = v | (self:mem_read(ptr + i) << (i * 8))
        end
        result(2, v)
    elseif c == C_MEMCRC then -- (ptr, num, result)
        local ptr, n = arg(), arg()
        arg()
        finish()
        if ptr >= 0 and ptr + n - 1 <= RAM_G_TOP then
            result(3, self.ram_g:crc32(ptr, n))
        else
            local bytes = {}
            for i = 0, n - 1 do
                bytes[#bytes + 1] = self:mem_read(ptr + i)
            end
            result(3, crc32(bytes))
        end
    elseif c == C_SETBITMAP then -- source, fmt, width, height: layout instructions of the handle
        local src, a2, a3 = arg(), arg(), arg()
        finish()
        local fmt, bw, bh = a2 & 0xFFFF, (a2 >> 16) & 0xFFFF, a3 & 0xFFFF
        local stride
        if fmt == F_L1 then
            stride = (bw + 7) // 8
        elseif fmt == F_L2 then
            stride = (bw + 3) // 4
        elseif fmt == F_L4 then
            stride = (bw + 1) // 2
        elseif fmt == F_ARGB1555 or fmt == F_ARGB4 or fmt == F_RGB565 then
            stride = bw * 2
        else
            stride = bw
        end
        self:dl_word(0x01000000 | (src & 0x3FFFFF)) -- BITMAP_SOURCE
        self:dl_word(0x07000000 | ((fmt & 0x1F) << 19) | ((stride & 0x3FF) << 9) | (bh & 0x1FF)) -- BITMAP_LAYOUT
        self:dl_word(0x28000000 | (((stride >> 10) & 3) << 2) | ((bh >> 9) & 3)) -- BITMAP_LAYOUT_H
        self:dl_word(0x08000000 | ((bw & 0x1FF) << 9) | (bh & 0x1FF)) -- BITMAP_SIZE (NEAREST, BORDER)
        self:dl_word(0x29000000 | (((bw >> 9) & 3) << 2) | ((bh >> 9) & 3)) -- BITMAP_SIZE_H
    elseif c == C_FGCOLOR or c == C_BGCOLOR then
        local rgb = arg() & 0xFFFFFF
        finish()
        if c == C_FGCOLOR then
            self.fgcolor = rgb
        else
            self.bgcolor = rgb
        end
    elseif c == C_TRACK then -- x, y, w, h, tag: REG_TRACKER (w = h = 0 stops tracking)
        local a1, a2, a3 = arg(), arg(), arg()
        finish()
        local t = a3 & 0xFF
        local w, h = s16(a2), s16(a2 >> 16)
        if w == 0 and h == 0 then
            self.trackers[t] = nil
        else
            self.trackers[t] = { x = s16(a1), y = s16(a1 >> 16), w = w, h = h }
        end
        self:update_touch_regs()
    elseif WIDGETS[c] then
        local spec = COPRO_ARGS[c]
        local nargs = type(spec) == "number" and spec or tonumber((spec:sub(1, 1)))
        local a = {}
        for k = 1, nargs do
            a[k] = arg()
        end
        local str = ""
        if type(spec) == "string" then
            str = self:fifo_string(read.pos)
            read.pos = (read.pos + self:fifo_skip_string(read.pos)) & (CMD_SIZE - 1)
        end
        finish()
        self:widget(c, a, str)
        if str ~= "" and c ~= C_KEYS then
            self:warn_once("wtext", "text of widgets is not drawn (no fonts)")
        end
    else
        local spec = COPRO_ARGS[c] -- known but not executed
        local nargs = type(spec) == "number" and spec or tonumber((spec:sub(1, 1)))
        for _ = 1, nargs do
            arg()
        end
        if type(spec) == "string" then -- trailing string argument
            read.pos = (read.pos + self:fifo_skip_string(read.pos)) & (CMD_SIZE - 1)
        end
        finish()
        if c == C_CALIBRATE then
            result(1, 1) -- success (touch is in pixels)
        end
        if c ~= C_NOP and c ~= C_NEWLIST and c ~= C_ENDLIST and c ~= C_CALLLIST and c ~= C_RETURN then
            self:warn_once("copro" .. c, string.format("coprocessor command %02Xh is not executed", c))
        end
    end
end

function DEV:start_playback()
    if self.play_timer then
        self.play_timer:stop()
    end
    local freq = (self.regs[R_PLAYBACK_FREQ] or 8000) & 0xFFFF
    local length = (self.regs[R_PLAYBACK_LENGTH] or 0) & 0xFFFFFF
    local format = (self.regs[R_PLAYBACK_FORMAT] or 0) & 3
    local samples = (format == 2) and length * 2 or length -- ADPCM: 4 bit
    if freq < 1 then
        freq = 8000
    end
    self.play_timer = host.timer(math.max(math.floor(samples / freq * 1000), 1), function()
        self:set_flag(F_PLAYBACK)
    end)
end

--------------------------------------------------------------------------------
-- display list rendering (REG_DLSWAP or CMD_SWAP)
--------------------------------------------------------------------------------

function DEV:swap()
    self.regs[R_DLSWAP] = 0 -- swap done
    if self.state ~= "active" then
        return
    end -- core off in sleep/standby/power down
    self.swaps = self.swaps + 1

    -- the same display list (and macros) without RAM_G changes renders the same frame
    local dl_raw = self.ram_dl
    local top = math.max(self.dl_top or 0, 4)
    local key_parts, t = {}, {}
    for k = 0, top - 1, CHUNK do
        local m = math.min(CHUNK, top - k)
        for j = 1, m do
            t[j] = dl_raw[k + j - 1] or 0
        end
        key_parts[#key_parts + 1] = string.char(table.unpack(t, 1, m))
    end
    local key = table.concat(key_parts)
        .. string.pack("<I4I4", (self.regs[R_MACRO_0] or 0) & 0xFFFFFFFF, (self.regs[R_MACRO_1] or 0) & 0xFFFFFFFF)
    local last = self.last_render
    if last and last.key == key and last.gen == (self.mem_gen or 0) and last.lcd == self.lcd then
        self.skipped = (self.skipped or 0) + 1
        self:update_touch_regs()
        self:set_flag(F_SWAP)
        if self.on_swap_cb then
            self.on_swap_cb(self:frame())
        end
        return
    end
    self.last_render = { key = key, gen = self.mem_gen or 0, lcd = self.lcd }
    local lcd, dl, cv, ram = self.lcd, self.ram_dl, self.canvas, self.ram_g

    -- graphics state (reset values at the beginning of the display list)
    local sx, sy, sw, sh = 0, 0, 2048, 2048 -- scissor
    local cr, cg, cb, ca = 0, 0, 0, 0 -- clear color
    local fr, fg, fbc, fa = 255, 255, 255, 255 -- current color
    local frac = 4 -- VERTEX_FORMAT: 1/16 px
    local tx, ty = 0, 0 -- VERTEX_TRANSLATE (1/16 px)
    local point_size, line_width = 16, 16 -- 1/16 px (radius, half width)
    local prim, verts = nil, {}
    local handles, palette, cur_handle, cur_cell = {}, 0, 0, 0
    local tag, tag_mask, clear_tag = 255, 1, 0 -- TAG, TAG_MASK, CLEAR_TAG
    local cmask = 0xF -- COLOR_MASK (bits r, g, b, a): without r, g, b only tags are drawn

    -- scissor and tag of the canvas (C) follow the state before drawing
    local cv_sx, cv_sy, cv_sw, cv_sh, cv_tag, cv_mask
    local function sync()
        if sx ~= cv_sx or sy ~= cv_sy or sw ~= cv_sw or sh ~= cv_sh then
            cv:scissor(sx, sy, sw, sh)
            cv_sx, cv_sy, cv_sw, cv_sh = sx, sy, sw, sh
        end
        if tag ~= cv_tag or tag_mask ~= cv_mask then
            cv:tag(tag, tag_mask ~= 0)
            cv_tag, cv_mask = tag, tag_mask
        end
    end

    local function handle(n)
        local b = handles[n]
        if not b then
            b = { source = 0, fmt = 0, ls = 0, ls_hi = 0, lh = 0, lh_hi = 0, sw = 0, sh = 0, sw_hi = 0, sh_hi = 0 }
            handles[n] = b
        end
        return b
    end

    local function color()
        return (fr << 16) | (fg << 8) | fbc
    end

    local function alpha() -- COLOR_A, nothing visible when COLOR_MASK disables colors
        return (cmask & 0xE) ~= 0 and fa or 0
    end

    local function fill(x0, y0, x1, y1) -- filled rectangle of current color, inclusive corners
        sync()
        cv:rect(x0, y0, x1, y1, color(), alpha())
    end

    local function rects(x0, y0, x1, y1) -- RECTS: corners rounded by LINE_WIDTH (radius)
        sync()
        if line_width > 16 then
            cv:rrect(x0, y0, x1, y1, line_width / 16, color(), alpha())
        else
            cv:rect(x0, y0, x1, y1, color(), alpha())
        end
    end

    local function disc(px, py, rad) -- filled circle of current color
        sync()
        cv:disc(px, py, rad, color(), alpha())
    end

    local function line(x0, y0, x1, y1) -- line of width 2 * LINE_WIDTH
        sync()
        cv:line(x0, y0, x1, y1, line_width / 16, color(), alpha())
    end

    local function tight_bytes(fmt, bw) -- bytes per row, packed
        if fmt == F_L1 then
            return (bw + 7) // 8
        end
        if fmt == F_L2 then
            return (bw + 3) // 4
        end
        if fmt == F_L4 then
            return (bw + 1) // 2
        end
        if fmt == F_ARGB1555 or fmt == F_ARGB4 or fmt == F_RGB565 then
            return bw * 2
        end
        return bw -- 8-bit formats
    end

    local function blit(x, y, hd, cell) -- one bitmap (cell) of BEGIN BITMAPS
        local b = handle(hd)
        if not b.layout then -- not configured handle (e.g. ROM fonts 16..31)
            self:warn_once("handle" .. hd, "bitmap handle " .. hd .. " without BITMAP_LAYOUT is not drawn")
            return
        end
        local fmt = b.fmt
        if fmt >= 9 and fmt <= 13 or fmt >= 18 then
            self:warn_once("fmt" .. fmt, "bitmap format " .. fmt .. " is not drawn")
            return
        end
        local bw = ((b.sw_hi or 0) << 9) | (b.sw & 0x1FF)
        local bh = ((b.sh_hi or 0) << 9) | (b.sh & 0x1FF)
        if bw == 0 then
            bw = 2048
        end
        if bh == 0 then
            bh = 2048
        end
        local lh = ((b.lh_hi or 0) << 9) | (b.lh & 0x1FF) -- rows of bitmap data (no wrap: border)
        local row = ((b.ls_hi or 0) << 10) | (b.ls & 0x3FF) -- line stride in bytes (BITMAP_LAYOUT)
        if row == 0 then
            row = tight_bytes(fmt, bw)
        end
        if lh == 0 then
            lh = bh
        end
        local base = b.source + (cell or 0) * row * lh -- cells follow each other (row * layout height)
        sync()
        cv:bitmap(ram, base, fmt, row, bw, math.min(bh, lh), x, y, color(), alpha(), palette)
    end

    local stack = {} -- SAVE/RESTORE_CONTEXT
    local function save_state()
        return {
            sx,
            sy,
            sw,
            sh,
            cr,
            cg,
            cb,
            ca,
            fr,
            fg,
            fbc,
            fa,
            frac,
            tx,
            ty,
            point_size,
            line_width,
            cur_handle,
            palette,
            tag,
            tag_mask,
            clear_tag,
            cur_cell,
            cmask,
        }
    end
    local function restore_state(st)
        if not st then
            return
        end
        sx, sy, sw, sh, cr, cg, cb, ca = st[1], st[2], st[3], st[4], st[5], st[6], st[7], st[8]
        fr, fg, fbc, fa, frac, tx, ty = st[9], st[10], st[11], st[12], st[13], st[14], st[15]
        point_size, line_width, cur_handle, palette = st[16], st[17], st[18], st[19]
        tag, tag_mask, clear_tag, cur_cell, cmask = st[20], st[21], st[22], st[23], st[24]
    end

    local function vertex(px, py, hd, cell)
        if prim == P_RECTS then
            verts[#verts + 1] = { px, py }
            if #verts == 2 then
                rects(verts[1][1], verts[1][2], verts[2][1], verts[2][2])
                verts = {}
            end
        elseif prim == P_POINTS then
            disc(px, py, point_size / 16)
        elseif prim == P_LINES then
            verts[#verts + 1] = { px, py }
            if #verts == 2 then
                line(verts[1][1], verts[1][2], verts[2][1], verts[2][2])
                verts = {}
            end
        elseif prim == P_LINE_STRIP then
            if #verts >= 1 then
                line(verts[1][1], verts[1][2], px, py)
            end
            verts = { { px, py } }
        elseif prim == P_BITMAPS then
            blit(px, py, hd or cur_handle, cell or cur_cell)
        elseif prim and prim >= 5 and prim <= 8 then
            self:warn_once("prim" .. prim, "EDGE_STRIP primitive is not drawn")
        end
    end

    -- execute one instruction, returns the next instruction index (jump) or nil
    local call_stack = {}
    local function exec(wd, i, depth)
        local hi = (wd >> 30) & 3
        local op = (wd >> 24) & 0xFF
        if hi == 1 then -- VERTEX2F
            local scale = 1 << frac
            vertex(s15((wd >> 15) & 0x7FFF) / scale + tx / 16, s15(wd & 0x7FFF) / scale + ty / 16)
        elseif hi == 2 then -- VERTEX2II
            vertex(((wd >> 21) & 0x1FF) + tx / 16, ((wd >> 12) & 0x1FF) + ty / 16, (wd >> 7) & 0x1F, wd & 0x7F)
        elseif op == 0x01 then
            handle(cur_handle).source = wd & 0x3FFFFF -- BITMAP_SOURCE
        elseif op == 0x02 then
            cr, cg, cb = (wd >> 16) & 0xFF, (wd >> 8) & 0xFF, wd & 0xFF -- CLEAR_COLOR_RGB
        elseif op == 0x03 then
            tag = wd & 0xFF -- TAG
        elseif op == 0x04 then
            fr, fg, fbc = (wd >> 16) & 0xFF, (wd >> 8) & 0xFF, wd & 0xFF -- COLOR_RGB
        elseif op == 0x05 then
            cur_handle = wd & 0x1F
            handle(cur_handle) -- BITMAP_HANDLE
        elseif op == 0x06 then
            cur_cell = wd & 0x7F -- CELL (VERTEX2F bitmaps)
        elseif op == 0x07 then -- BITMAP_LAYOUT
            local b = handle(cur_handle)
            b.fmt, b.ls, b.lh = (wd >> 19) & 0x1F, (wd >> 9) & 0x3FF, wd & 0x1FF
            b.layout = true
        elseif op == 0x08 then -- BITMAP_SIZE
            local b = handle(cur_handle)
            b.sw, b.sh = (wd >> 9) & 0x1FF, wd & 0x1FF
        elseif op == 0x0D then
            point_size = wd & 0x1FFF -- POINT_SIZE
        elseif op == 0x0E then
            line_width = wd & 0xFFF -- LINE_WIDTH
        elseif op == 0x0F then
            ca = wd & 0xFF -- CLEAR_COLOR_A
        elseif op == 0x10 then
            fa = wd & 0xFF -- COLOR_A
        elseif op == 0x12 then
            clear_tag = wd & 0xFF -- CLEAR_TAG
        elseif op == 0x14 then
            tag_mask = wd & 1 -- TAG_MASK
        elseif op == 0x1B then
            sx, sy = (wd >> 11) & 0x7FF, wd & 0x7FF -- SCISSOR_XY
        elseif op == 0x1C then
            sw, sh = (wd >> 12) & 0xFFF, wd & 0xFFF -- SCISSOR_SIZE
        elseif op == 0x1D then
            call_stack[#call_stack + 1] = i + 1
            return wd & 0xFFFF -- CALL
        elseif op == 0x1E then
            return wd & 0xFFFF -- JUMP
        elseif op == 0x1F then
            prim = wd & 0xF
            verts = {} -- BEGIN
        elseif op == 0x21 then
            prim, verts = nil, {} -- END
        elseif op == 0x22 then
            stack[#stack + 1] = save_state() -- SAVE_CONTEXT
        elseif op == 0x23 then
            restore_state(table.remove(stack)) -- RESTORE_CONTEXT
        elseif op == 0x24 then -- RETURN
            local pc = table.remove(call_stack)
            if pc then
                return pc
            end
        elseif op == 0x25 then -- MACRO
            if depth == 0 then
                return exec(self.regs[((wd & 1) == 0) and R_MACRO_0 or R_MACRO_1] or 0, i, 1)
            end
        elseif op == 0x26 then -- CLEAR (color/stencil/tag) in the scissor rectangle
            local c, t = (wd & 4) ~= 0 and (cmask & 0xE) ~= 0, (wd & 1) ~= 0
            if c or t then
                sync()
                cv:clear(c and ((cr << 16) | (cg << 8) | cb) or nil, t and clear_tag or nil)
            end
        elseif op == 0x27 then
            frac = wd & 7 -- VERTEX_FORMAT
        elseif op == 0x28 then -- BITMAP_LAYOUT_H
            local b = handle(cur_handle)
            b.ls_hi, b.lh_hi = (wd >> 2) & 3, wd & 3
        elseif op == 0x29 then -- BITMAP_SIZE_H
            local b = handle(cur_handle)
            b.sw_hi, b.sh_hi = (wd >> 2) & 3, wd & 3
        elseif op == 0x2A then
            palette = wd & 0x3FFFFF -- PALETTE_SOURCE
        elseif op == 0x2B then
            tx = s17(wd & 0x1FFFF) -- VERTEX_TRANSLATE_X
        elseif op == 0x2C then
            ty = s17(wd & 0x1FFFF) -- VERTEX_TRANSLATE_Y
        elseif op == 0x20 then -- COLOR_MASK
            cmask = wd & 0xF
            if (cmask & 0xE) ~= 0 and (cmask & 0xE) ~= 0xE then
                self:warn_once("cmask", "COLOR_MASK of some color channels is drawn as all channels")
            end
        elseif op == 0x0B then -- BLEND_FUNC
            if (wd & 0x3F) ~= ((2 << 3) | 4) then
                self:warn_once("blend", "BLEND_FUNC other than (SRC_ALPHA, ONE_MINUS_SRC_ALPHA) is drawn as default")
            end
        elseif (op >= 0x09 and op <= 0x0C) or op == 0x11 or op == 0x13 or op == 0x2D or op == 0x30 then
            -- ALPHA_FUNC, STENCIL_FUNC, STENCIL_OP, CLEAR_STENCIL, STENCIL_MASK,
            -- NOP, INT_FRR: accepted without effect
        elseif (op >= 0x15 and op <= 0x1A) or op == 0x2E or op == 0x2F then
            self:warn_once("bmtransform", "bitmap transform/swizzle is ignored") -- BITMAP_TRANSFORM_*, EXT_FORMAT, SWIZZLE
        else
            self:warn_once("dl" .. op, string.format("display instruction %02Xh ignored", op))
        end
        return nil
    end

    -- execute the display list words until DISPLAY or end of RAM_DL
    local i, limit = 0, DL_SIZE >> 2
    local guard = 0
    while i < limit and guard < 100000 do
        guard = guard + 1
        local wd = (dl[i * 4] or 0)
            | ((dl[i * 4 + 1] or 0) << 8)
            | ((dl[i * 4 + 2] or 0) << 16)
            | ((dl[i * 4 + 3] or 0) << 24)
        if wd == 0 then
            break
        end -- DISPLAY: end of list
        local nxt = exec(wd, i, 0)
        i = nxt or (i + 1)
    end

    -- canvas to the panel LCD (converted in C, only changed rows)
    cv:push(lcd)

    self:update_touch_regs()
    self:set_flag(F_SWAP)
    if self.on_swap_cb then
        self.on_swap_cb(self:frame())
    end
end

function DEV:on_swap(fn)
    self.on_swap_cb = fn
    return self
end

--------------------------------------------------------------------------------
-- touch (FT800 compatibility mode follows the first point, extended mode the
-- registers of all five points of the capacitive touch panel)
--------------------------------------------------------------------------------

local CTOUCH_XY = { [0] = R_SCREEN_XY, R_RAW_XY, R_CTOUCH2_XY, R_CTOUCH3_XY } -- TOUCH0..3

function DEV:extended()
    return (self.regs[R_CTOUCH_EXT] or 1) & 1 == 0
end

-- write the touch registers according to the mode and stored points
-- tag of the displayed frame at pixel x, y (opts.tags / set_tag() overrides)
function DEV:tag_at(x, y)
    if self.tags_cb then
        local t = self.tags_cb(x, y)
        if t ~= nil then
            return t & 0xFF
        end
    end
    if x < 0 or y < 0 or x >= self.width or y >= self.height then
        return 0
    end
    return self.canvas:tag_at(x, y)
end

-- REG_TRACKER value of touch point p with tag t: (value << 16) | tag, 0 when not tracked
function DEV:tracker_value(p, t)
    local tr = p and t ~= 0 and self.trackers[t]
    if not tr then
        return 0
    end
    local v
    if tr.w == 1 and tr.h == 1 then -- rotary: angle clockwise from straight down
        local a = math.atan(-(p.x - tr.x), p.y - tr.y)
        v = math.floor(a / (2 * math.pi) * 65536) & 0xFFFF
    elseif tr.w > tr.h then -- horizontal: 0 at left, 65535 at right
        v = math.floor(math.max(0, math.min(1, (p.x - tr.x) / tr.w)) * 65535)
    else -- vertical: 0 at top
        v = math.floor(math.max(0, math.min(1, (p.y - tr.y) / tr.h)) * 65535)
    end
    return (v << 16) | t
end

function DEV:update_touch_regs()
    local regs = self.regs
    if not regs then
        return
    end
    local function xy(p)
        return p and ((p.x << 16) | (p.y & 0xFFFF)) or 0x80008000
    end
    local tags = {}
    for n = 0, 4 do
        local p = self.touches[n]
        tags[n] = p and self:tag_at(p.x, p.y) or 0
    end
    local p0 = self.touches[0]
    regs[R_TAG_XY], regs[R_TOUCH_TAG] = xy(p0), tags[0]
    self.tracker_regs[1] = self:tracker_value(p0, tags[0])
    if self:extended() then
        for n = 0, 3 do
            regs[CTOUCH_XY[n]] = xy(self.touches[n])
        end
        local p4 = self.touches[4]
        regs[R_CTOUCH4_X] = p4 and p4.x or 0x8000
        regs[R_CTOUCH4_Y] = p4 and p4.y or 0x8000
        for n = 1, 4 do -- REG_TOUCH_TAGn_XY, REG_TOUCH_TAGn, REG_TRACKER_n
            regs[R_TAG1_XY + (n - 1) * 2] = xy(self.touches[n])
            regs[R_TAG1_XY + (n - 1) * 2 + 1] = tags[n]
            self.tracker_regs[n + 1] = self:tracker_value(self.touches[n], tags[n])
        end
    else
        regs[R_RAW_XY] = p0 and xy(p0) or 0xFFFFFFFF
        regs[R_SCREEN_XY] = xy(p0)
        for n = 2, 5 do
            self.tracker_regs[n] = 0
        end
    end
end

function DEV:touch(x, y, pressed)
    return self:touch_slot(0, x, y, pressed)
end

-- touch point in pixels of the display (pressed false/nil releases the point)
function DEV:touch_slot(n, x, y, pressed)
    if n < 0 or n > 4 then
        error("bt817: touch slot must be 0..4")
    end
    if pressed == nil then
        pressed = true
    end
    if not pressed or x == nil then
        self.touches[n] = nil
    else
        self.touches[n] = { x = math.floor(x), y = math.floor(y) }
    end
    self:update_touch_regs()
    self:set_flag(F_TOUCH)
end

function DEV:set_tag(fn)
    self.tags_cb = fn
    return self
end

--------------------------------------------------------------------------------
-- chip GPIO pins
--------------------------------------------------------------------------------

function DEV:pin(n, level)
    if n < 0 or n > 3 then
        error("bt817: pin must be 0..3")
    end
    if level ~= nil then
        self.pins[n] = not (level == false or level == 0)
    end
    local dir = ((self.regs[R_GPIO_DIR] or 0x80) >> n) & 1
    if dir == 1 then
        return self.gpio_out[n]
    else
        return self.pins[n]
    end
end

function DEV:on_gpio(fn)
    self.gpio_cb = fn
    return self
end

function DEV:update_gpio_outputs()
    local reg, dir = (self.regs[R_GPIO] or 0), (self.regs[R_GPIO_DIR] or 0x80)
    for n = 0, 3 do
        if (dir >> n) & 1 == 1 then
            local lvl = (reg >> n) & 1 == 1
            if self.gpio_out[n] ~= lvl then
                self.gpio_out[n] = lvl
                if self.gpio_cb then
                    self.gpio_cb(n, lvl)
                end
            end
        end
    end
end

function DEV:backlight()
    return (self.regs[R_PWM_DUTY] or 0) & 0xFF
end
function DEV:frame()
    if self.display_on then
        return (self.frames_count or 0) + math.floor((host.time_ms() - self.frame_t0) * self.fps / 1000)
    end
    return self.frames_count or 0
end

--------------------------------------------------------------------------------
-- attach
--------------------------------------------------------------------------------

function DEV:attach(bus, cs)
    spi.attach(bus, cs, self)
    return self
end

return M
