/*==============================================================================
@file   tuidemo.c

@author Daniel Zorychta

@brief  libtui demonstration: colors, lines, keys, partial screen updates.

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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <tui.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define LOG_LINES     64
#define LOG_LINE_LEN  32
#define LEFT_WIDTH    54
#define COLORS_HEIGHT 14
#define MIN_COLS      80
#define MIN_ROWS      20
#define BAUDRATE      115200

#define ATTR_BODY     TUI_ATTR(TUI_WHITE, TUI_BLUE)
#define ATTR_FRAME    TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_BLUE)
#define ATTR_TITLE    TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_KEY_NUM  TUI_ATTR(TUI_WHITE, TUI_BLACK)
#define ATTR_KEY_TEXT TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_MARKER   (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_BLUE) | TUI_BOLD)
#define ATTR_HINT     TUI_ATTR(TUI_BRIGHT(TUI_CYAN), TUI_BLUE)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void draw_title(tui_t *tui);
static void draw_fkeys(tui_t *tui);
static void draw_panel(tui_t *tui, int x, int y, int w, int h, const char *title);
static void draw_colors(tui_t *tui, int x, int y, int w, int h);
static void draw_cursor_box(tui_t *tui, int x, int y, int w, int h);
static void draw_keys(tui_t *tui, int x, int y, int w, int h);
static void draw(tui_t *tui);
static void log_key(int key);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char log[LOG_LINES][LOG_LINE_LEN];
        int log_head;
        int log_count;
        int marker_x;
        int marker_y;
        int box_style;
        bool ascii_lines;
        size_t last_bytes;
};

static const char *const FKEY_LABEL[10] = {
        "",
        "Lines",
        "Box",
        "",
        "Redraw",
        "",
        "",
        "",
        "",
        "Quit",
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(tuidemo, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Draw title bar.
 */
//==============================================================================
static void draw_title(tui_t *tui)
{
        tui_set_attr(tui, ATTR_TITLE);
        tui_fill(tui, 0, 0, tui_cols(tui), 1, ' ');

        size_t ms = (global->last_bytes * 10 * 1000) / BAUDRATE;

        tui_move(tui, 1, 0);
        tui_printf(tui, "libtui demo   %dx%d   last refresh: %u bytes (~%u ms at %u baud)", tui_cols(tui),
                tui_rows(tui), (unsigned)global->last_bytes, (unsigned)ms, BAUDRATE);
}

//==============================================================================
/**
 * @brief Draw function key bar (mc style).
 */
//==============================================================================
static void draw_fkeys(tui_t *tui)
{
        int y = tui_rows(tui) - 1;
        int w = tui_cols(tui) / 10;

        tui_set_attr(tui, ATTR_KEY_TEXT);
        tui_fill(tui, 0, y, tui_cols(tui), 1, ' ');

        for (int i = 0; i < 10; i++) {
                tui_set_attr(tui, ATTR_KEY_NUM);
                tui_move(tui, i * w, y);
                tui_printf(tui, "%2d", i + 1);

                tui_set_attr(tui, ATTR_KEY_TEXT);
                tui_printf(tui, "%-*.*s", w - 2, w - 2, FKEY_LABEL[i]);
        }
}

//==============================================================================
/**
 * @brief Draw panel frame with title.
 */
//==============================================================================
static void draw_panel(tui_t *tui, int x, int y, int w, int h, const char *title)
{
        tui_set_attr(tui, ATTR_BODY);
        tui_fill(tui, x, y, w, h, ' ');

        tui_set_attr(tui, ATTR_FRAME);
        tui_box(tui, x, y, w, h, global->box_style);

        tui_move(tui, x + 2, y);
        tui_printf(tui, " %s ", title);
}

//==============================================================================
/**
 * @brief Draw colors and attributes panel.
 */
//==============================================================================
static void draw_colors(tui_t *tui, int x, int y, int w, int h)
{
        draw_panel(tui, x, y, w, h, "Colors");

        /* foreground colors (columns) on background colors (rows) */
        for (int bg = 0; bg < 8; bg++) {
                tui_move(tui, x + 2, y + 1 + bg);

                for (int fg = 0; fg < 16; fg++) {
                        tui_set_attr(tui, TUI_ATTR(fg, bg));
                        tui_puts(tui, (fg < 8) ? "Ab " : "Br ");
                }
        }

        int row = y + 10;

        tui_move(tui, x + 2, row++);
        tui_set_attr(tui, ATTR_BODY | TUI_BOLD);
        tui_puts(tui, "Bold");
        tui_set_attr(tui, ATTR_BODY);
        tui_puts(tui, " ");
        tui_set_attr(tui, ATTR_BODY | TUI_UNDERLINE);
        tui_puts(tui, "Underline");
        tui_set_attr(tui, ATTR_BODY);
        tui_puts(tui, " ");
        tui_set_attr(tui, ATTR_BODY | TUI_REVERSE);
        tui_puts(tui, "Reverse");
        tui_set_attr(tui, ATTR_BODY);
        tui_puts(tui, " ");
        tui_set_attr(tui, TUI_ATTR_DEFAULT);
        tui_puts(tui, "Terminal default");

        tui_print_at(tui, x + 2, row++, ATTR_BODY, "UTF-8: Zażółć gęślą jaźń ─│┌┐└┘ ═║╔╗╚╝ °±µ€");
        tui_print_at(tui, x + 2, row++, ATTR_HINT, "F2: ASCII lines  F3: single/double box");
}

//==============================================================================
/**
 * @brief Draw panel with marker moved by arrow keys.
 */
//==============================================================================
static void draw_cursor_box(tui_t *tui, int x, int y, int w, int h)
{
        draw_panel(tui, x, y, w, h, "Arrows move @");

        int iw = w - 2;
        int ih = h - 2;

        global->marker_x = (global->marker_x < 0) ? 0 : (global->marker_x >= iw) ? iw - 1 : global->marker_x;
        global->marker_y = (global->marker_y < 0) ? 0 : (global->marker_y >= ih) ? ih - 1 : global->marker_y;

        tui_set_attr(tui, ATTR_MARKER);
        tui_move(tui, x + 1 + global->marker_x, y + 1 + global->marker_y);
        tui_putc(tui, '@');
}

//==============================================================================
/**
 * @brief Draw key log panel (the newest key first).
 */
//==============================================================================
static void draw_keys(tui_t *tui, int x, int y, int w, int h)
{
        draw_panel(tui, x, y, w, h, "Keys");

        tui_set_attr(tui, ATTR_BODY);

        for (int i = 0; (i < global->log_count) && (i < h - 2); i++) {
                int idx = (global->log_head - 1 - i + LOG_LINES) % LOG_LINES;
                tui_move(tui, x + 2, y + 1 + i);
                tui_printf(tui, "%.*s", w - 4, global->log[idx]);
        }
}

//==============================================================================
/**
 * @brief Draw whole screen.
 */
//==============================================================================
static void draw(tui_t *tui)
{
        int cols = tui_cols(tui);
        int rows = tui_rows(tui);

        tui_set_ascii_lines(tui, global->ascii_lines);

        if ((cols < MIN_COLS) || (rows < MIN_ROWS)) {
                tui_set_attr(tui, TUI_ATTR_DEFAULT);
                tui_clear(tui);
                tui_move(tui, 0, 0);
                tui_printf(tui, "Terminal %dx%d is too small (min. %dx%d)", cols, rows, MIN_COLS, MIN_ROWS);
                tui_move(tui, 0, 1);
                tui_puts(tui, "F10/Ctrl+X: quit, Ctrl+L: redraw");
                tui_set_cursor(tui, -1, -1);
                return;
        }

        draw_title(tui);
        draw_colors(tui, 0, 1, LEFT_WIDTH, COLORS_HEIGHT);
        draw_cursor_box(tui, 0, 1 + COLORS_HEIGHT, LEFT_WIDTH, rows - 2 - COLORS_HEIGHT);
        draw_keys(tui, LEFT_WIDTH, 1, cols - LEFT_WIDTH, rows - 2);
        draw_fkeys(tui);

        tui_set_cursor(tui, -1, -1);
}

//==============================================================================
/**
 * @brief Add key to log.
 */
//==============================================================================
static void log_key(int key)
{
        tui_key_name(key, global->log[global->log_head], LOG_LINE_LEN);

        global->log_head = (global->log_head + 1) % LOG_LINES;

        if (global->log_count < LOG_LINES) {
                global->log_count++;
        }
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;

        tui_t *tui = tui_init();
        if (!tui) {
                perror("Terminal initialization error");
                return EXIT_FAILURE;
        }

        global->box_style = TUI_BOX_SINGLE;

        bool run = true;

        while (run) {
                draw(tui);
                tui_refresh(tui);
                global->last_bytes = tui_last_refresh_bytes(tui);

                int key = tui_getkey(tui, -1);

                log_key(key);

                switch (key) {
                case TUI_KEY_F(10):
                case TUI_KEY_CTRL('X'):
                        run = false;
                        break;

                case TUI_KEY_F(5):
                case TUI_KEY_CTRL('L'):
                        tui_redraw(tui);
                        break;

                case TUI_KEY_F(2):
                        global->ascii_lines = !global->ascii_lines;
                        break;

                case TUI_KEY_F(3):
                        global->box_style = (global->box_style == TUI_BOX_SINGLE) ? TUI_BOX_DOUBLE
                                                                                  : TUI_BOX_SINGLE;
                        break;

                case TUI_KEY_UP:
                        global->marker_y--;
                        break;

                case TUI_KEY_DOWN:
                        global->marker_y++;
                        break;

                case TUI_KEY_LEFT:
                        global->marker_x--;
                        break;

                case TUI_KEY_RIGHT:
                        global->marker_x++;
                        break;

                default:
                        break;
                }
        }

        tui_deinit(tui);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
