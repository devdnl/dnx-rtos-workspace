/*==============================================================================
@file    tui.h

@author  Daniel Zorychta

@brief   Text user interface library (VT100/xterm terminals).

         The library keeps two screen buffers (what the program draws and
         what the terminal shows) and sends only differences on refresh,
         so it works well on slow serial links.

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

#ifndef _TUI_H_
#define _TUI_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
/* colors (0-7 normal, 8-15 bright), TUI_DEFAULT is terminal default color */
#define TUI_BLACK     0
#define TUI_RED       1
#define TUI_GREEN     2
#define TUI_YELLOW    3
#define TUI_BLUE      4
#define TUI_MAGENTA   5
#define TUI_CYAN      6
#define TUI_WHITE     7
#define TUI_BRIGHT(c) ((c) + 8)
#define TUI_DEFAULT   16

/* attribute: foreground, background and flags */
#define TUI_ATTR(fg, bg)     ((uint16_t)(((fg) & 0x1F) | (((bg) & 0x1F) << 5)))
#define TUI_BOLD             0x0400
#define TUI_UNDERLINE        0x0800
#define TUI_REVERSE          0x1000
#define TUI_ATTR_DEFAULT     TUI_ATTR(TUI_DEFAULT, TUI_DEFAULT)
#define TUI_ATTR_FG(attr)    ((attr) & 0x1F)
#define TUI_ATTR_BG(attr)    (((attr) >> 5) & 0x1F)
#define TUI_ATTR_FLAGS(attr) ((attr) & 0x1C00)

/*
 * Keys returned by tui_getkey():
 *  - printable characters as Unicode code points,
 *  - control characters as they are (TUI_KEY_CTRL('x')),
 *  - special keys from TUI_KEY_UP,
 *  - modifiers (TUI_MOD_*) are added to special keys and Alt+character.
 */
#define TUI_KEY_NONE      (-1) /* timeout */
#define TUI_KEY_CTRL(c)   ((c) & 0x1F)
#define TUI_KEY_TAB       0x09
#define TUI_KEY_ENTER     0x0D
#define TUI_KEY_ESC       0x1B
#define TUI_KEY_BACKSPACE 0x7F
#define TUI_KEY_UP        0x110000
#define TUI_KEY_DOWN      0x110001
#define TUI_KEY_LEFT      0x110002
#define TUI_KEY_RIGHT     0x110003
#define TUI_KEY_HOME      0x110004
#define TUI_KEY_END       0x110005
#define TUI_KEY_INSERT    0x110006
#define TUI_KEY_DELETE    0x110007
#define TUI_KEY_PGUP      0x110008
#define TUI_KEY_PGDN      0x110009
#define TUI_KEY_SHIFT_TAB 0x11000A
#define TUI_KEY_F1        0x110010
#define TUI_KEY_F(n)      (TUI_KEY_F1 + (n) - 1) /* n = 1..12 */
#define TUI_KEY_RESIZE    0x110030               /* terminal size changed (see tui_redraw()) */
#define TUI_KEY_UNKNOWN   0x110031               /* not recognized escape sequence */
#define TUI_MOD_SHIFT     0x01000000
#define TUI_MOD_ALT       0x02000000
#define TUI_MOD_CTRL      0x04000000
#define TUI_KEY_CODE(key) ((key) & 0x00FFFFFF)
#define TUI_KEY_MODS(key) ((key) & 0x07000000)

/* box styles */
#define TUI_BOX_SINGLE 0
#define TUI_BOX_DOUBLE 1

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct tui tui_t;

/*==============================================================================
  Exported functions
==============================================================================*/
/* initialization: raw mode, terminal size query, buffers; NULL on error (errno) */
extern tui_t *tui_init(void);
extern tui_t *tui_init_stream(FILE *in, FILE *out);
extern void tui_deinit(tui_t *tui);
extern void tui_suspend(tui_t *tui);
extern void tui_resume(tui_t *tui);

/* screen size */
extern int tui_cols(tui_t *tui);
extern int tui_rows(tui_t *tui);

/* drawing (screen is updated by tui_refresh()) */
extern void tui_set_attr(tui_t *tui, uint16_t attr);
extern uint16_t tui_get_attr(tui_t *tui);
extern void tui_move(tui_t *tui, int x, int y);
extern void tui_putc(tui_t *tui, uint32_t ch);
extern int tui_puts(tui_t *tui, const char *utf8);
extern int tui_printf(tui_t *tui, const char *fmt, ...) __attribute__((format(__printf__, 2, 3)));
extern int tui_vprintf(tui_t *tui, const char *fmt, va_list args);
extern int tui_print_at(tui_t *tui, int x, int y, uint16_t attr, const char *utf8);
extern void tui_fill(tui_t *tui, int x, int y, int w, int h, uint32_t ch);
extern void tui_clear(tui_t *tui);
extern void tui_hline(tui_t *tui, int x, int y, int w, int style);
extern void tui_vline(tui_t *tui, int x, int y, int h, int style);
extern void tui_box(tui_t *tui, int x, int y, int w, int h, int style);
extern void tui_set_ascii_lines(tui_t *tui, bool ascii);

/* cursor (x < 0 hides cursor) */
extern void tui_set_cursor(tui_t *tui, int x, int y);

/* screen update */
extern void tui_refresh(tui_t *tui);
extern void tui_redraw(tui_t *tui);
extern size_t tui_last_refresh_bytes(tui_t *tui);

/* input: timeout_ms < 0 waits forever, returns key or TUI_KEY_NONE */
extern int tui_getkey(tui_t *tui, int timeout_ms);
extern void tui_set_esc_delay(tui_t *tui, int delay_ms);
extern void tui_set_esc_prefix_timeout(tui_t *tui, int timeout_ms);
extern void tui_key_name(int key, char *buf, size_t size);

/*
 * Modal boxes drawn over current screen (dialog style). Text can contain new
 * lines and is wrapped. Screen under box is not restored: program should draw
 * its screen again after box is closed.
 */
extern void tui_msgbox(tui_t *tui, const char *title, const char *text, bool error);
extern int tui_question(tui_t *tui, const char *title, const char *text, const char *const *buttons,
        int count, int def, bool error);
extern bool tui_yesno(tui_t *tui, const char *title, const char *text, bool def_yes);
extern bool tui_inputbox(tui_t *tui, const char *title, const char *prompt, char *buf, size_t size);
extern void tui_infobox(tui_t *tui, const char *title, const char *text);

/* UTF-8 helpers */
extern int tui_strwidth(const char *utf8);
extern int tui_utf8_encode(uint32_t ch, char *dst);
extern uint32_t tui_utf8_decode(const char **utf8);

#ifdef __cplusplus
}
#endif

#endif /* _TUI_H_ */
/*==============================================================================
  End of file
==============================================================================*/
