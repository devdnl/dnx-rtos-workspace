/*==============================================================================
@file    tui.c

@author  Daniel Zorychta

@brief   Text user interface library (VT100/xterm terminals).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

        Permission is hereby granted, free of charge, to any person obtaining
        a copy of this software and associated documentation files (the
        "Software"), to deal in the Software without restriction, including
        without limitation the rights to use, copy, modify, merge, publish,
        distribute, sublicense, and/or sell copies of the Software, and to
        permit persons to whom the Software is furnished to do so, subject to
        the following conditions:

        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
        OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include "tui.h"
#include "tui_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define OBUF_SIZE             512
#define IBUF_SIZE             64
#define PRINTF_BUF_SIZE       256
#define DEFAULT_COLS          80
#define DEFAULT_ROWS          24
#define MIN_COLS              10
#define MIN_ROWS              2
#define MAX_COLS              512
#define MAX_ROWS              200
#define SIZE_QUERY_TIMEOUT_MS 500
#define DEFAULT_ESC_DELAY_MS  60
#define DEFAULT_ESC_PREFIX_MS 1000
#define CSI_MAX_LEN           16
#define CSI_MAX_PARAMS        4
#define REPRINT_LIMIT         4 /* max. gap reprinted instead of cursor move */
#define ERASE_MIN             4 /* min. blank line end erased instead of printed */
#define ERASE_ATTR_CANDIDATES 8 /* attributes counted to select screen clear color */
#define KEY_IGNORE            (-2)
#define ESC                   "\033"

/* terminal control sequences */
#define SEQ_SIZE_QUERY  ESC "7" ESC "[999;999H" ESC "[6n" ESC "8"
#define SEQ_INIT        ESC "[?1049h" ESC "[?7l"
#define SEQ_EXIT        ESC "[0m" ESC "[2J" ESC "[H" ESC "[?7h" ESC "[?25h" ESC "[?1049l"
#define SEQ_CLEAR       ESC "[H" ESC "[2J"
#define SEQ_ERASE_EOL   ESC "[K"
#define SEQ_CURSOR_HIDE ESC "[?25l"
#define SEQ_CURSOR_SHOW ESC "[?25h"

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        uint16_t ch;
        uint16_t attr;
} cell_t;

struct tui {
        void *port;

        /* screen buffers: back is drawn by program, front is shown by terminal */
        int cols;
        int rows;
        cell_t *back;
        cell_t *front;

        /* drawing state */
        int x;
        int y;
        uint16_t attr;
        int cursor_x;
        int cursor_y;
        bool ascii_lines;
        bool bce; /* terminal erases with current background color */

        /* terminal output state */
        int out_x; /* -1: position not known */
        int out_y;
        uint16_t out_attr;
        bool out_attr_valid;
        bool out_cursor_visible;
        bool full_redraw;
        size_t refresh_bytes;
        size_t olen;
        char obuf[OBUF_SIZE];

        /* input state */
        bool size_pending;
        int esc_delay;
        int esc_prefix;
        size_t ilen;
        size_t ipos;
        uint8_t ibuf[IBUF_SIZE];
};

typedef struct {
        int key;
        const char *name;
} key_name_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void out_flush(tui_t *tui);
static void out_str(tui_t *tui, const char *str, size_t len);
static void out_cstr(tui_t *tui, const char *str);
static void out_char(tui_t *tui, uint16_t ch);
static void out_move(tui_t *tui, int x, int y);
static void out_attr(tui_t *tui, uint16_t attr);
static bool set_size(tui_t *tui, int cols, int rows);
static void request_size(tui_t *tui);
static int read_byte(tui_t *tui, int timeout_ms);
static int decode_key(tui_t *tui, int c);
static int decode_esc(tui_t *tui);
static int decode_csi(tui_t *tui);
static int decode_utf8(tui_t *tui, int c);
static int xterm_mods(int param);

/*==============================================================================
  Local object definitions
==============================================================================*/
/* line characters: horizontal, vertical, top-left, top-right, bottom-left, bottom-right */
static const uint16_t LINES_UTF8[2][6] = {
        {0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518},
        {0x2550, 0x2551, 0x2554, 0x2557, 0x255A, 0x255D},
};

static const uint16_t LINES_ASCII[2][6] = {
        {'-', '|', '+', '+', '+', '+'},
        {'=', '|', '+', '+', '+', '+'},
};

static const key_name_t KEY_NAMES[] = {
        {TUI_KEY_UP, "Up"},
        {TUI_KEY_DOWN, "Down"},
        {TUI_KEY_LEFT, "Left"},
        {TUI_KEY_RIGHT, "Right"},
        {TUI_KEY_HOME, "Home"},
        {TUI_KEY_END, "End"},
        {TUI_KEY_INSERT, "Insert"},
        {TUI_KEY_DELETE, "Delete"},
        {TUI_KEY_PGUP, "PgUp"},
        {TUI_KEY_PGDN, "PgDn"},
        {TUI_KEY_SHIFT_TAB, "Shift+Tab"},
        {TUI_KEY_RESIZE, "Resize"},
        {TUI_KEY_UNKNOWN, "Unknown"},
        {TUI_KEY_TAB, "Tab"},
        {TUI_KEY_ENTER, "Enter"},
        {TUI_KEY_ESC, "Esc"},
        {TUI_KEY_BACKSPACE, "Backspace"},
        {' ', "Space"},
};

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Send buffered output to terminal.
 *
 * @param  tui          TUI object
 */
//==============================================================================
static void out_flush(tui_t *tui)
{
        if (tui->olen) {
                tui_port_write(tui->port, tui->obuf, tui->olen);
                tui->refresh_bytes += tui->olen;
                tui->olen = 0;
        }
}

//==============================================================================
/**
 * @brief  Add bytes to output buffer.
 *
 * @param  tui          TUI object
 * @param  str          bytes
 * @param  len          number of bytes
 */
//==============================================================================
static void out_str(tui_t *tui, const char *str, size_t len)
{
        while (len) {
                if (tui->olen == sizeof(tui->obuf)) {
                        out_flush(tui);
                }

                size_t n = sizeof(tui->obuf) - tui->olen;
                if (n > len) {
                        n = len;
                }

                memcpy(&tui->obuf[tui->olen], str, n);
                tui->olen += n;
                str += n;
                len -= n;
        }
}

//==============================================================================
/**
 * @brief  Add string to output buffer.
 *
 * @param  tui          TUI object
 * @param  str          string
 */
//==============================================================================
static void out_cstr(tui_t *tui, const char *str)
{
        out_str(tui, str, strlen(str));
}

//==============================================================================
/**
 * @brief  Add character to output buffer (UTF-8 encoded).
 *
 * @param  tui          TUI object
 * @param  ch           character
 */
//==============================================================================
static void out_char(tui_t *tui, uint16_t ch)
{
        char utf8[4];
        int n = tui_utf8_encode(ch, utf8);
        out_str(tui, utf8, n);
}

//==============================================================================
/**
 * @brief  Move terminal cursor.
 *
 * @param  tui          TUI object
 * @param  x            column
 * @param  y            row
 */
//==============================================================================
static void out_move(tui_t *tui, int x, int y)
{
        char seq[16];
        int n = snprintf(seq, sizeof(seq), ESC "[%d;%dH", y + 1, x + 1);
        out_str(tui, seq, n);

        tui->out_x = x;
        tui->out_y = y;
}

//==============================================================================
/**
 * @brief  Set terminal attribute (SGR).
 *
 * @param  tui          TUI object
 * @param  attr         attribute
 */
//==============================================================================
static void out_attr(tui_t *tui, uint16_t attr)
{
        char seq[32];
        int n = snprintf(seq, sizeof(seq), ESC "[0");
        int fg = TUI_ATTR_FG(attr);
        int bg = TUI_ATTR_BG(attr);

        /*
         * Bright colors (SGR 90-97, 100-107) are not supported by VT100/VT102
         * terminals (e.g. minicom): bright foreground is sent as bold (shown
         * bright by terminals), bright background as normal color.
         */
        if ((attr & TUI_BOLD) || ((fg >= 8) && (fg < 16))) {
                n += snprintf(&seq[n], sizeof(seq) - n, ";1");
        }

        if (attr & TUI_UNDERLINE) {
                n += snprintf(&seq[n], sizeof(seq) - n, ";4");
        }

        if (attr & TUI_REVERSE) {
                n += snprintf(&seq[n], sizeof(seq) - n, ";7");
        }

        if (fg < 16) {
                n += snprintf(&seq[n], sizeof(seq) - n, ";%d", 30 + (fg & 7));
        }

        if (bg < 16) {
                n += snprintf(&seq[n], sizeof(seq) - n, ";%d", 40 + (bg & 7));
        }

        n += snprintf(&seq[n], sizeof(seq) - n, "m");

        out_str(tui, seq, n);

        tui->out_attr = attr;
        tui->out_attr_valid = true;
}

//==============================================================================
/**
 * @brief  Set screen size and allocate buffers. Screen content is cleared.
 *
 * @param  tui          TUI object
 * @param  cols         number of columns
 * @param  rows         number of rows
 *
 * @return true if size was changed, false if size is the same or on error
 *         (old buffers are kept).
 */
//==============================================================================
static bool set_size(tui_t *tui, int cols, int rows)
{
        cols = (cols > MAX_COLS) ? MAX_COLS : (cols < MIN_COLS) ? MIN_COLS : cols;
        rows = (rows > MAX_ROWS) ? MAX_ROWS : (rows < MIN_ROWS) ? MIN_ROWS : rows;

        if (tui->back && (cols == tui->cols) && (rows == tui->rows)) {
                return false;
        }

        size_t cells = (size_t)cols * rows;
        cell_t *back = malloc(cells * sizeof(cell_t));
        cell_t *front = malloc(cells * sizeof(cell_t));

        if (!back || !front) {
                free(back);
                free(front);
                return false;
        }

        for (size_t i = 0; i < cells; i++) {
                back[i].ch = ' ';
                back[i].attr = TUI_ATTR_DEFAULT;
        }

        memcpy(front, back, cells * sizeof(cell_t));

        free(tui->back);
        free(tui->front);

        tui->back = back;
        tui->front = front;
        tui->cols = cols;
        tui->rows = rows;
        tui->x = 0;
        tui->y = 0;
        tui->full_redraw = true;

        return true;
}

//==============================================================================
/**
 * @brief  Ask terminal about its size. The answer (cursor position report) is
 *         handled by key decoder.
 *
 * @param  tui          TUI object
 */
//==============================================================================
static void request_size(tui_t *tui)
{
        out_cstr(tui, SEQ_SIZE_QUERY);
        out_flush(tui);

        /* cursor position is restored by terminal, but it is not known here */
        tui->out_x = -1;
        tui->size_pending = true;
}

//==============================================================================
/**
 * @brief  Initialize TUI on stdin/stdout: raw mode, terminal size, screen buffers.
 *
 * @return TUI object or NULL on error (errno is set).
 */
//==============================================================================
tui_t *tui_init(void)
{
        return tui_init_stream(stdin, stdout);
}

//==============================================================================
/**
 * @brief  Initialize TUI on selected streams (e.g. output to stderr when
 *         stdout is used for program result).
 *
 * @param  in           terminal input stream
 * @param  out          terminal output stream
 *
 * @return TUI object or NULL on error (errno is set).
 */
//==============================================================================
tui_t *tui_init_stream(FILE *in, FILE *out)
{
        tui_t *tui = calloc(1, sizeof(tui_t));
        if (!tui) {
                errno = ENOMEM;
                return NULL;
        }

        if (tui_port_open(&tui->port, fileno(in), fileno(out)) != 0) {
                int err = errno;
                free(tui);
                errno = err;
                return NULL;
        }

        tui->attr = TUI_ATTR_DEFAULT;
        tui->bce = true;
        tui->esc_delay = DEFAULT_ESC_DELAY_MS;
        tui->esc_prefix = DEFAULT_ESC_PREFIX_MS;
        tui->cursor_x = -1;
        tui->out_x = -1;
        tui->out_y = -1;
        tui->out_cursor_visible = true;

        out_cstr(tui, SEQ_INIT);
        request_size(tui);

        /* wait for terminal answer, other input is dropped */
        uint32_t tref = tui_port_time_ms();
        uint32_t elapsed;

        while (tui->size_pending && ((elapsed = tui_port_time_ms() - tref) < SIZE_QUERY_TIMEOUT_MS)) {
                tui_getkey(tui, SIZE_QUERY_TIMEOUT_MS - elapsed);
        }

        tui->size_pending = false;

        if (!tui->back && !set_size(tui, DEFAULT_COLS, DEFAULT_ROWS)) {
                tui_deinit(tui);
                errno = ENOMEM;
                return NULL;
        }

        return tui;
}

//==============================================================================
/**
 * @brief  Restore terminal and release TUI object.
 *
 * @param  tui          TUI object
 */
//==============================================================================
void tui_deinit(tui_t *tui)
{
        if (tui) {
                out_cstr(tui, SEQ_EXIT);
                out_flush(tui);
                tui_port_close(tui->port);
                free(tui->back);
                free(tui->front);
                free(tui);
        }
}

//==============================================================================
/**
 * @brief  Restore normal terminal mode (e.g. before other program is started).
 *         Screen content is kept in buffers, see tui_resume().
 *
 * @param  tui          TUI object
 */
//==============================================================================
void tui_suspend(tui_t *tui)
{
        out_cstr(tui, SEQ_EXIT);
        out_flush(tui);
        tui_port_set_raw(tui->port, false);
}

//==============================================================================
/**
 * @brief  Return to TUI mode after tui_suspend(). Whole screen is drawn on
 *         next refresh, terminal size is checked (see tui_redraw()).
 *
 * @param  tui          TUI object
 */
//==============================================================================
void tui_resume(tui_t *tui)
{
        tui_port_set_raw(tui->port, true);

        tui->out_x = -1;
        tui->out_y = -1;
        tui->out_attr_valid = false;
        tui->out_cursor_visible = true;
        tui->ilen = 0;
        tui->ipos = 0;

        out_cstr(tui, SEQ_INIT);
        tui_redraw(tui);
}

//==============================================================================
/**
 * @brief  Return number of columns.
 */
//==============================================================================
int tui_cols(tui_t *tui)
{
        return tui->cols;
}

//==============================================================================
/**
 * @brief  Return number of rows.
 */
//==============================================================================
int tui_rows(tui_t *tui)
{
        return tui->rows;
}

//==============================================================================
/**
 * @brief  Set current attribute used by drawing functions.
 */
//==============================================================================
void tui_set_attr(tui_t *tui, uint16_t attr)
{
        tui->attr = attr;
}

//==============================================================================
/**
 * @brief  Return current attribute.
 */
//==============================================================================
uint16_t tui_get_attr(tui_t *tui)
{
        return tui->attr;
}

//==============================================================================
/**
 * @brief  Set drawing position (can be outside of screen, output is clipped).
 */
//==============================================================================
void tui_move(tui_t *tui, int x, int y)
{
        tui->x = x;
        tui->y = y;
}

//==============================================================================
/**
 * @brief  Put character at drawing position and move position right.
 *         Characters outside of screen are clipped. Control characters and
 *         characters out of Basic Multilingual Plane are shown as '?'.
 *
 * @param  tui          TUI object
 * @param  ch           Unicode character
 */
//==============================================================================
void tui_putc(tui_t *tui, uint32_t ch)
{
        if ((ch < 0x20) || ((ch >= 0x7F) && (ch < 0xA0)) || (ch > 0xFFFF)) {
                ch = '?';
        }

        if ((tui->x >= 0) && (tui->x < tui->cols) && (tui->y >= 0) && (tui->y < tui->rows)) {
                cell_t *cell = &tui->back[tui->y * tui->cols + tui->x];
                cell->ch = ch;
                cell->attr = tui->attr;
        }

        tui->x++;
}

//==============================================================================
/**
 * @brief  Put UTF-8 string at drawing position.
 *
 * @param  tui          TUI object
 * @param  utf8         string
 *
 * @return Number of columns.
 */
//==============================================================================
int tui_puts(tui_t *tui, const char *utf8)
{
        int n = 0;

        if (utf8) {
                uint32_t ch;
                while ((ch = tui_utf8_decode(&utf8)) != 0) {
                        tui_putc(tui, ch);
                        n++;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief  Put formatted string at drawing position (max. 255 bytes).
 *
 * @return Number of columns.
 */
//==============================================================================
int tui_vprintf(tui_t *tui, const char *fmt, va_list args)
{
        char buf[PRINTF_BUF_SIZE];
        vsnprintf(buf, sizeof(buf), fmt, args);
        return tui_puts(tui, buf);
}

//==============================================================================
/**
 * @brief  Put formatted string at drawing position (max. 255 bytes).
 *
 * @return Number of columns.
 */
//==============================================================================
int tui_printf(tui_t *tui, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        int n = tui_vprintf(tui, fmt, args);
        va_end(args);

        return n;
}

//==============================================================================
/**
 * @brief  Put string at selected position with selected attribute. The
 *         attribute becomes current attribute.
 *
 * @return Number of columns.
 */
//==============================================================================
int tui_print_at(tui_t *tui, int x, int y, uint16_t attr, const char *utf8)
{
        tui->attr = attr;
        tui_move(tui, x, y);
        return tui_puts(tui, utf8);
}

//==============================================================================
/**
 * @brief  Fill rectangle with character using current attribute.
 */
//==============================================================================
void tui_fill(tui_t *tui, int x, int y, int w, int h, uint32_t ch)
{
        for (int row = y; row < y + h; row++) {
                tui_move(tui, x, row);

                for (int col = 0; col < w; col++) {
                        tui_putc(tui, ch);
                }
        }

        tui_move(tui, x, y);
}

//==============================================================================
/**
 * @brief  Clear screen using current attribute.
 */
//==============================================================================
void tui_clear(tui_t *tui)
{
        tui_fill(tui, 0, 0, tui->cols, tui->rows, ' ');
}

//==============================================================================
/**
 * @brief  Select ASCII line characters (for terminals without UTF-8).
 */
//==============================================================================
void tui_set_ascii_lines(tui_t *tui, bool ascii)
{
        tui->ascii_lines = ascii;
}

//==============================================================================
/**
 * @brief  Return line character.
 *
 * @param  tui          TUI object
 * @param  style        TUI_BOX_SINGLE or TUI_BOX_DOUBLE
 * @param  idx          character index (see LINES_UTF8)
 */
//==============================================================================
static uint16_t line_char(tui_t *tui, int style, int idx)
{
        style = (style == TUI_BOX_DOUBLE) ? 1 : 0;
        return tui->ascii_lines ? LINES_ASCII[style][idx] : LINES_UTF8[style][idx];
}

//==============================================================================
/**
 * @brief  Draw horizontal line using current attribute.
 */
//==============================================================================
void tui_hline(tui_t *tui, int x, int y, int w, int style)
{
        tui_move(tui, x, y);

        for (int i = 0; i < w; i++) {
                tui_putc(tui, line_char(tui, style, 0));
        }
}

//==============================================================================
/**
 * @brief  Draw vertical line using current attribute.
 */
//==============================================================================
void tui_vline(tui_t *tui, int x, int y, int h, int style)
{
        for (int i = 0; i < h; i++) {
                tui_move(tui, x, y + i);
                tui_putc(tui, line_char(tui, style, 1));
        }
}

//==============================================================================
/**
 * @brief  Draw box frame using current attribute (interior is not changed).
 */
//==============================================================================
void tui_box(tui_t *tui, int x, int y, int w, int h, int style)
{
        if ((w < 2) || (h < 2)) {
                return;
        }

        tui_hline(tui, x + 1, y, w - 2, style);
        tui_hline(tui, x + 1, y + h - 1, w - 2, style);
        tui_vline(tui, x, y + 1, h - 2, style);
        tui_vline(tui, x + w - 1, y + 1, h - 2, style);

        tui_move(tui, x, y);
        tui_putc(tui, line_char(tui, style, 2));
        tui_move(tui, x + w - 1, y);
        tui_putc(tui, line_char(tui, style, 3));
        tui_move(tui, x, y + h - 1);
        tui_putc(tui, line_char(tui, style, 4));
        tui_move(tui, x + w - 1, y + h - 1);
        tui_putc(tui, line_char(tui, style, 5));
}

//==============================================================================
/**
 * @brief  Set cursor position shown after refresh (x < 0 hides cursor).
 */
//==============================================================================
void tui_set_cursor(tui_t *tui, int x, int y)
{
        tui->cursor_x = x;
        tui->cursor_y = y;
}

//==============================================================================
/**
 * @brief  Scroll rows of terminal (whole width) instead of sending them again.
 *         Program informs that content of rows top..bottom was moved by n
 *         rows (n > 0: up, n < 0: down) and draws screen as usual. Terminal
 *         is scrolled on next refresh, so only new rows are sent.
 *
 * @param  tui          TUI object
 * @param  top          first row of region
 * @param  bottom       last row of region
 * @param  n            number of rows (positive: content moves up)
 */
//==============================================================================
void tui_scroll(tui_t *tui, int top, int bottom, int n)
{
        int height = bottom - top + 1;

        if (tui->full_redraw || (top < 0) || (bottom >= tui->rows) || (height < 2) || (n == 0) ||
                (n >= height) || (-n >= height)) {
                return;
        }

        if (tui->out_cursor_visible) {
                out_cstr(tui, SEQ_CURSOR_HIDE);
                tui->out_cursor_visible = false;
        }

        /* new rows are filled with current background */
        if (!tui->out_attr_valid || (tui->out_attr != TUI_ATTR_DEFAULT)) {
                out_attr(tui, TUI_ATTR_DEFAULT);
        }

        char seq[24];
        int len = snprintf(seq, sizeof(seq), ESC "[%d;%dr", top + 1, bottom + 1);
        out_str(tui, seq, len);

        /* index at bottom margin scrolls up, reverse index at top margin scrolls down */
        out_move(tui, 0, (n > 0) ? bottom : top);
        for (int i = 0; i < ((n > 0) ? n : -n); i++) {
                out_cstr(tui, (n > 0) ? ESC "D" : ESC "M");
        }

        /* reset scroll region (cursor position is not known) */
        out_cstr(tui, ESC "[r");
        tui->out_x = -1;
        tui->out_y = -1;

        /* front buffer follows terminal */
        int cols = tui->cols;
        int m = (n > 0) ? n : -n;
        cell_t *f = tui->front;

        if (n > 0) {
                memmove(&f[top * cols], &f[(top + m) * cols], (size_t)(height - m) * cols * sizeof(cell_t));
        } else {
                memmove(&f[(top + m) * cols], &f[top * cols], (size_t)(height - m) * cols * sizeof(cell_t));
        }

        int blank = (n > 0) ? bottom - m + 1 : top;
        for (int i = blank * cols; i < (blank + m) * cols; i++) {
                f[i].ch = ' ';
                f[i].attr = TUI_ATTR_DEFAULT;
        }
}

//==============================================================================
/**
 * @brief  Check if blank cells of selected attribute can be created by erase
 *         sequences (screen clear, erase to end of line). Erased cells have
 *         background color of current attribute (BCE terminals) or default
 *         color. Underline and reverse are not used by erase.
 *
 * @param  tui          TUI object
 * @param  attr         attribute of blank cells
 *
 * @return true if erase can be used.
 */
//==============================================================================
static bool can_erase(tui_t *tui, uint16_t attr)
{
        if (attr & (TUI_UNDERLINE | TUI_REVERSE)) {
                return false;
        }

        return tui->bce || (TUI_ATTR_BG(attr) == TUI_DEFAULT);
}

//==============================================================================
/**
 * @brief  Clear whole screen with the most common attribute of blank cells,
 *         so these cells are not sent again.
 *
 * @param  tui          TUI object
 */
//==============================================================================
static void clear_screen(tui_t *tui)
{
        uint16_t attr[ERASE_ATTR_CANDIDATES];
        size_t count[ERASE_ATTR_CANDIDATES];
        int n = 0;

        size_t cells = (size_t)tui->cols * tui->rows;

        for (size_t i = 0; i < cells; i++) {
                if ((tui->back[i].ch != ' ') || !can_erase(tui, tui->back[i].attr)) {
                        continue;
                }

                int k = 0;
                while ((k < n) && (attr[k] != tui->back[i].attr)) {
                        k++;
                }

                if (k == n) {
                        if (n == ERASE_ATTR_CANDIDATES) {
                                continue;
                        }

                        attr[n] = tui->back[i].attr;
                        count[n++] = 0;
                }

                count[k]++;
        }

        uint16_t clear_attr = TUI_ATTR_DEFAULT;
        size_t best = 0;

        for (int k = 0; k < n; k++) {
                if (count[k] > best) {
                        best = count[k];
                        clear_attr = attr[k];
                }
        }

        out_cstr(tui, SEQ_CURSOR_HIDE);
        out_attr(tui, clear_attr);
        out_cstr(tui, SEQ_CLEAR);

        tui->out_cursor_visible = false;
        tui->out_x = 0;
        tui->out_y = 0;

        for (size_t i = 0; i < cells; i++) {
                tui->front[i].ch = ' ';
                tui->front[i].attr = clear_attr;
        }
}

//==============================================================================
/**
 * @brief  Send changed part of screen to terminal.
 *
 * @param  tui          TUI object
 */
//==============================================================================
void tui_refresh(tui_t *tui)
{
        tui->refresh_bytes = 0;

        if (tui->full_redraw) {
                clear_screen(tui);
                tui->full_redraw = false;
        }

        for (int y = 0; y < tui->rows; y++) {
                cell_t *back = &tui->back[y * tui->cols];
                cell_t *front = &tui->front[y * tui->cols];

                /* blank end of line (the same attribute) can be erased */
                int cols = tui->cols;
                int tail = cols;
                uint16_t tail_attr = back[cols - 1].attr;

                if ((back[cols - 1].ch == ' ') && can_erase(tui, tail_attr)) {
                        while ((tail > 0) && (back[tail - 1].ch == ' ') &&
                                (back[tail - 1].attr == tail_attr)) {
                                tail--;
                        }
                }

                for (int x = 0; x < tui->cols; x++) {
                        if ((back[x].ch == front[x].ch) && (back[x].attr == front[x].attr)) {
                                continue;
                        }

                        bool erase = (x >= tail) && ((cols - x) >= ERASE_MIN);

                        if (tui->out_cursor_visible) {
                                out_cstr(tui, SEQ_CURSOR_HIDE);
                                tui->out_cursor_visible = false;
                        }

                        if ((tui->out_x != x) || (tui->out_y != y)) {
                                /*
                                 * Short gap of not changed cells with the same
                                 * attribute is reprinted (cheaper than move).
                                 */
                                bool reprint = tui->out_attr_valid && (tui->out_y == y) &&
                                        (tui->out_x >= 0) && (x > tui->out_x) &&
                                        ((x - tui->out_x) <= REPRINT_LIMIT);

                                for (int i = tui->out_x; reprint && (i < x); i++) {
                                        reprint = (back[i].attr == tui->out_attr);
                                }

                                if (reprint) {
                                        for (int i = tui->out_x; i < x; i++) {
                                                out_char(tui, back[i].ch);
                                        }

                                        tui->out_x = x;
                                } else {
                                        out_move(tui, x, y);
                                }
                        }

                        if (!tui->out_attr_valid || (back[x].attr != tui->out_attr)) {
                                out_attr(tui, back[x].attr);
                        }

                        if (erase) {
                                out_cstr(tui, SEQ_ERASE_EOL);
                                memcpy(&front[x], &back[x], (size_t)(cols - x) * sizeof(cell_t));
                                break;
                        }

                        out_char(tui, back[x].ch);
                        front[x] = back[x];

                        /* auto wrap is disabled, cursor stays in the last column */
                        tui->out_x = (x + 1 < tui->cols) ? (x + 1) : -1;
                }
        }

        if ((tui->cursor_x >= 0) && (tui->cursor_x < tui->cols) && (tui->cursor_y >= 0) &&
                (tui->cursor_y < tui->rows)) {

                if ((tui->out_x != tui->cursor_x) || (tui->out_y != tui->cursor_y)) {
                        out_move(tui, tui->cursor_x, tui->cursor_y);
                }

                if (!tui->out_cursor_visible) {
                        out_cstr(tui, SEQ_CURSOR_SHOW);
                        tui->out_cursor_visible = true;
                }

        } else if (tui->out_cursor_visible) {
                out_cstr(tui, SEQ_CURSOR_HIDE);
                tui->out_cursor_visible = false;
        }

        out_flush(tui);
}

//==============================================================================
/**
 * @brief  Redraw whole screen on next refresh and ask terminal about its size.
 *         If size is changed tui_getkey() returns TUI_KEY_RESIZE (screen is
 *         cleared, program should draw it again).
 *
 * @param  tui          TUI object
 */
//==============================================================================
void tui_redraw(tui_t *tui)
{
        tui->full_redraw = true;
        request_size(tui);
}

//==============================================================================
/**
 * @brief  Select if terminal erases screen with current background color
 *         (BCE, enabled by default: xterm, Konsole, minicom, PuTTY, tmux).
 *         When disabled, erase sequences are used only for default background.
 *
 * @param  tui          TUI object
 * @param  enable       true: terminal supports BCE
 */
//==============================================================================
void tui_set_bce(tui_t *tui, bool enable)
{
        tui->bce = enable;
}

//==============================================================================
/**
 * @brief  Return number of bytes sent by last refresh.
 */
//==============================================================================
size_t tui_last_refresh_bytes(tui_t *tui)
{
        return tui->refresh_bytes;
}

//==============================================================================
/**
 * @brief  Set time of waiting for the rest of escape sequence. Esc key is
 *         recognized after this time.
 */
//==============================================================================
void tui_set_esc_delay(tui_t *tui, int delay_ms)
{
        tui->esc_delay = (delay_ms > 0) ? delay_ms : 1;
}

//==============================================================================
/**
 * @brief  Set time of waiting for the next key after Esc pressed by user
 *         (Esc + digit: function key, Esc + key: Alt + key, Esc Esc: Esc).
 *         Zero: Esc key is returned without waiting.
 */
//==============================================================================
void tui_set_esc_prefix_timeout(tui_t *tui, int timeout_ms)
{
        tui->esc_prefix = (timeout_ms > 0) ? timeout_ms : 0;
}

//==============================================================================
/**
 * @brief  Read input byte.
 *
 * @param  tui          TUI object
 * @param  timeout_ms   timeout (< 0 forever)
 *
 * @return Byte or -1 on timeout or error.
 */
//==============================================================================
static int read_byte(tui_t *tui, int timeout_ms)
{
        if (tui->ipos >= tui->ilen) {
                tui->ipos = 0;
                tui->ilen = 0;

                int n = tui_port_read(tui->port, tui->ibuf, sizeof(tui->ibuf), timeout_ms);
                if (n <= 0) {
                        return -1;
                }

                tui->ilen = n;
        }

        return tui->ibuf[tui->ipos++];
}

//==============================================================================
/**
 * @brief  Convert xterm modifier parameter to TUI_MOD_* flags.
 */
//==============================================================================
static int xterm_mods(int param)
{
        int mods = 0;
        int bits = param - 1;

        if (bits > 0) {
                if (bits & 1) {
                        mods |= TUI_MOD_SHIFT;
                }

                if (bits & (2 | 8)) {
                        mods |= TUI_MOD_ALT;
                }

                if (bits & 4) {
                        mods |= TUI_MOD_CTRL;
                }
        }

        return mods;
}

//==============================================================================
/**
 * @brief  Decode rest of UTF-8 character.
 *
 * @param  tui          TUI object
 * @param  c            first byte
 *
 * @return Unicode character or U+FFFD on error.
 */
//==============================================================================
static int decode_utf8(tui_t *tui, int c)
{
        int len;
        uint32_t ch;

        if ((c & 0xE0) == 0xC0) {
                len = 1;
                ch = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
                len = 2;
                ch = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
                len = 3;
                ch = c & 0x07;
        } else {
                return 0xFFFD;
        }

        for (int i = 0; i < len; i++) {
                int b = read_byte(tui, tui->esc_delay);
                if ((b < 0) || ((b & 0xC0) != 0x80)) {
                        return 0xFFFD;
                }

                ch = (ch << 6) | (b & 0x3F);
        }

        return (ch > 0x10FFFF) ? 0xFFFD : (int)ch;
}

//==============================================================================
/**
 * @brief  Decode CSI sequence (ESC [ already received).
 *
 * @param  tui          TUI object
 *
 * @return Key.
 */
//==============================================================================
static int decode_csi(tui_t *tui)
{
        int c = read_byte(tui, tui->esc_delay);

        /* Linux console F1-F5: ESC [ [ A..E */
        if (c == '[') {
                c = read_byte(tui, tui->esc_delay);
                return ((c >= 'A') && (c <= 'E')) ? TUI_KEY_F(1 + c - 'A') : TUI_KEY_UNKNOWN;
        }

        int param[CSI_MAX_PARAMS] = {0};
        int nparam = 0;
        bool final = false;

        for (int i = 0; (i < CSI_MAX_LEN) && (c >= 0); i++) {
                if ((c >= '0') && (c <= '9')) {
                        if (nparam == 0) {
                                nparam = 1;
                        }

                        if (param[nparam - 1] < 10000) {
                                param[nparam - 1] = param[nparam - 1] * 10 + (c - '0');
                        }

                } else if (c == ';') {
                        if (nparam == 0) {
                                nparam = 1;
                        }

                        if (nparam < CSI_MAX_PARAMS) {
                                nparam++;
                        }

                } else if ((c >= 0x40) && (c <= 0x7E)) {
                        final = true;
                        break;
                }

                c = read_byte(tui, tui->esc_delay);
        }

        if (!final) {
                return TUI_KEY_UNKNOWN;
        }

        int mods = (nparam >= 2) ? xterm_mods(param[1]) : 0;

        switch (c) {
        case 'A':
                return TUI_KEY_UP | mods;
        case 'B':
                return TUI_KEY_DOWN | mods;
        case 'C':
                return TUI_KEY_RIGHT | mods;
        case 'D':
                return TUI_KEY_LEFT | mods;
        case 'H':
                return TUI_KEY_HOME | mods;
        case 'F':
                return TUI_KEY_END | mods;
        case 'Z':
                return TUI_KEY_SHIFT_TAB;
        case 'P':
        case 'Q':
        case 'S':
                return TUI_KEY_F(1 + c - 'P') | mods;

        case 'R':
                /* cursor position report (terminal size) or F3 with modifiers */
                if (tui->size_pending && (nparam == 2) && (param[0] >= MIN_ROWS) && (param[1] >= MIN_COLS)) {
                        tui->size_pending = false;
                        return set_size(tui, param[1], param[0]) ? TUI_KEY_RESIZE : KEY_IGNORE;
                }

                return TUI_KEY_F(3) | mods;

        case '~':
                switch (param[0]) {
                case 1:
                case 7:
                        return TUI_KEY_HOME | mods;
                case 2:
                        return TUI_KEY_INSERT | mods;
                case 3:
                        return TUI_KEY_DELETE | mods;
                case 4:
                case 8:
                        return TUI_KEY_END | mods;
                case 5:
                        return TUI_KEY_PGUP | mods;
                case 6:
                        return TUI_KEY_PGDN | mods;
                case 11 ... 15:
                        return TUI_KEY_F(param[0] - 10) | mods;
                case 16: /* minicom */
                        return TUI_KEY_F(5) | mods;
                case 17 ... 21:
                        return TUI_KEY_F(param[0] - 11) | mods;
                case 23:
                case 24:
                        return TUI_KEY_F(param[0] - 12) | mods;
                default:
                        return TUI_KEY_UNKNOWN;
                }

        default:
                return TUI_KEY_UNKNOWN;
        }
}

//==============================================================================
/**
 * @brief  Decode escape sequence (ESC already received).
 *
 * @param  tui          TUI object
 *
 * @return Key.
 */
//==============================================================================
static int decode_esc(tui_t *tui)
{
        int c = read_byte(tui, tui->esc_delay);

        if (c < 0) {
                /*
                 * Esc pressed by user: wait for the next key (as in mc).
                 * Esc + digit is function key, Esc + key is Alt + key.
                 */
                c = (tui->esc_prefix > 0) ? read_byte(tui, tui->esc_prefix) : -1;

                if ((c < 0) || (c == 0x1B)) {
                        return TUI_KEY_ESC;
                } else if ((c >= '1') && (c <= '9')) {
                        return TUI_KEY_F(c - '0');
                } else if (c == '0') {
                        return TUI_KEY_F(10);
                } else {
                        return TUI_MOD_ALT | decode_key(tui, c);
                }
        }

        if (c == '[') {
                return decode_csi(tui);
        }

        if (c == 'O') {
                c = read_byte(tui, tui->esc_delay);

                switch (c) {
                case 'A':
                        return TUI_KEY_UP;
                case 'B':
                        return TUI_KEY_DOWN;
                case 'C':
                        return TUI_KEY_RIGHT;
                case 'D':
                        return TUI_KEY_LEFT;
                case 'H':
                        return TUI_KEY_HOME;
                case 'F':
                        return TUI_KEY_END;
                case 'M':
                        return TUI_KEY_ENTER;
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                        return TUI_KEY_F(1 + c - 'P');
                default:
                        return TUI_KEY_UNKNOWN;
                }
        }

        /* Esc Esc: Esc key (as in mc) */
        if (c == 0x1B) {
                return TUI_KEY_ESC;
        }

        /* Esc + digit: function key (as in mc, for terminals that catch F keys) */
        if ((c >= '1') && (c <= '9')) {
                return TUI_KEY_F(c - '0');
        }

        if (c == '0') {
                return TUI_KEY_F(10);
        }

        return TUI_MOD_ALT | decode_key(tui, c);
}

//==============================================================================
/**
 * @brief  Decode key from the first byte.
 *
 * @param  tui          TUI object
 * @param  c            first byte
 *
 * @return Key.
 */
//==============================================================================
static int decode_key(tui_t *tui, int c)
{
        if (c == 0x1B) {
                return decode_esc(tui);

        } else if ((c == '\r') || (c == '\n')) {
                return TUI_KEY_ENTER;

        } else if ((c == 0x7F) || (c == 0x08)) {
                return TUI_KEY_BACKSPACE;

        } else if (c < 0x80) {
                return c;

        } else {
                return decode_utf8(tui, c);
        }
}

//==============================================================================
/**
 * @brief  Read key.
 *
 * @param  tui          TUI object
 * @param  timeout_ms   timeout (< 0 forever)
 *
 * @return Key or TUI_KEY_NONE on timeout.
 */
//==============================================================================
int tui_getkey(tui_t *tui, int timeout_ms)
{
        uint32_t tref = tui_port_time_ms();

        for (;;) {
                int remaining = timeout_ms;

                if (timeout_ms >= 0) {
                        uint32_t elapsed = tui_port_time_ms() - tref;
                        remaining = (elapsed >= (uint32_t)timeout_ms) ? 0 : (int)(timeout_ms - elapsed);
                }

                int c = read_byte(tui, remaining);
                if (c < 0) {
                        return TUI_KEY_NONE;
                }

                int key = decode_key(tui, c);
                if (key != KEY_IGNORE) {
                        return key;
                }
        }
}

//==============================================================================
/**
 * @brief  Get key name, e.g. "Ctrl+Left", "F5", "Alt+x", "a".
 *
 * @param  key          key
 * @param  buf          destination buffer
 * @param  size         buffer size
 */
//==============================================================================
void tui_key_name(int key, char *buf, size_t size)
{
        if (!buf || (size == 0)) {
                return;
        }

        buf[0] = '\0';

        if (key == TUI_KEY_NONE) {
                snprintf(buf, size, "None");
                return;
        }

        int code = TUI_KEY_CODE(key);
        int mods = TUI_KEY_MODS(key);

        size_t n = snprintf(buf, size, "%s%s%s", (mods & TUI_MOD_CTRL) ? "Ctrl+" : "",
                (mods & TUI_MOD_ALT) ? "Alt+" : "", (mods & TUI_MOD_SHIFT) ? "Shift+" : "");

        if (n >= size) {
                return;
        }

        for (size_t i = 0; i < sizeof(KEY_NAMES) / sizeof(KEY_NAMES[0]); i++) {
                if (KEY_NAMES[i].key == code) {
                        snprintf(&buf[n], size - n, "%s", KEY_NAMES[i].name);
                        return;
                }
        }

        if ((code >= TUI_KEY_F(1)) && (code <= TUI_KEY_F(12))) {
                snprintf(&buf[n], size - n, "F%d", code - TUI_KEY_F1 + 1);

        } else if (code < 0x20) {
                snprintf(&buf[n], size - n, "Ctrl+%c", code + '@');

        } else if (code <= 0x10FFFF) {
                char utf8[5];
                utf8[tui_utf8_encode(code, utf8)] = '\0';
                snprintf(&buf[n], size - n, "%s", utf8);

        } else {
                snprintf(&buf[n], size - n, "0x%X", code);
        }
}

//==============================================================================
/**
 * @brief  Return number of columns of UTF-8 string (one column per character).
 */
//==============================================================================
int tui_strwidth(const char *utf8)
{
        int n = 0;

        if (utf8) {
                while (tui_utf8_decode(&utf8) != 0) {
                        n++;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief  Encode character to UTF-8 (invalid character is encoded as '?').
 *
 * @param  ch           Unicode character
 * @param  dst          destination (min. 4 bytes)
 *
 * @return Number of bytes.
 */
//==============================================================================
int tui_utf8_encode(uint32_t ch, char *dst)
{
        if (ch < 0x80) {
                dst[0] = ch;
                return 1;

        } else if (ch < 0x800) {
                dst[0] = 0xC0 | (ch >> 6);
                dst[1] = 0x80 | (ch & 0x3F);
                return 2;

        } else if ((ch < 0x10000) && ((ch < 0xD800) || (ch > 0xDFFF))) {
                dst[0] = 0xE0 | (ch >> 12);
                dst[1] = 0x80 | ((ch >> 6) & 0x3F);
                dst[2] = 0x80 | (ch & 0x3F);
                return 3;

        } else if ((ch >= 0x10000) && (ch <= 0x10FFFF)) {
                dst[0] = 0xF0 | (ch >> 18);
                dst[1] = 0x80 | ((ch >> 12) & 0x3F);
                dst[2] = 0x80 | ((ch >> 6) & 0x3F);
                dst[3] = 0x80 | (ch & 0x3F);
                return 4;

        } else {
                dst[0] = '?';
                return 1;
        }
}

//==============================================================================
/**
 * @brief  Decode UTF-8 character and move pointer to the next one.
 *
 * @param  utf8         pointer to string
 *
 * @return Unicode character, 0 at end of string (pointer is not moved),
 *         U+FFFD for invalid byte (pointer is moved by one byte).
 */
//==============================================================================
uint32_t tui_utf8_decode(const char **utf8)
{
        const uint8_t *s = (const uint8_t *)*utf8;

        if (s[0] == 0) {
                return 0;
        }

        int len;
        uint32_t ch;

        if (s[0] < 0x80) {
                *utf8 += 1;
                return s[0];
        } else if ((s[0] & 0xE0) == 0xC0) {
                len = 2;
                ch = s[0] & 0x1F;
        } else if ((s[0] & 0xF0) == 0xE0) {
                len = 3;
                ch = s[0] & 0x0F;
        } else if ((s[0] & 0xF8) == 0xF0) {
                len = 4;
                ch = s[0] & 0x07;
        } else {
                *utf8 += 1;
                return 0xFFFD;
        }

        for (int i = 1; i < len; i++) {
                if ((s[i] & 0xC0) != 0x80) {
                        *utf8 += 1;
                        return 0xFFFD;
                }

                ch = (ch << 6) | (s[i] & 0x3F);
        }

        *utf8 += len;

        return ch;
}

/*==============================================================================
  End of file
==============================================================================*/
