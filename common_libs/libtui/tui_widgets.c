/*==============================================================================
@file    tui_widgets.c

@author  Daniel Zorychta

@brief   Text user interface library: modal boxes (message, question, input).

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define MAX_TEXT_WIDTH 70
#define MIN_INPUT_W    30
#define MAX_LINES      64

#define ATTR_BOX       TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_TITLE     (TUI_ATTR(TUI_BLUE, TUI_WHITE) | TUI_BOLD)
#define ATTR_ERR       (TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_RED) | TUI_BOLD)
#define ATTR_ERR_TITLE (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_RED) | TUI_BOLD)
#define ATTR_SHADOW    TUI_ATTR(TUI_BLACK, TUI_BLACK)
#define ATTR_BTN       TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_BTN_ERR   TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_RED)
#define ATTR_BTN_SEL   (TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_BLUE) | TUI_BOLD)
#define ATTR_INPUT     TUI_ATTR(TUI_BLACK, TUI_CYAN)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        char *buf;
        const char *line[MAX_LINES];
        int lines;
        int x;
        int y;
        int w;
        int h;
        bool error;
} box_t;

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Wrap text to lines (words longer than line are split).
 *
 * @param  box          box (lines are set)
 * @param  text         text
 * @param  width        line width
 *
 * @return true on success.
 */
//==============================================================================
static bool wrap(box_t *box, const char *text, int width)
{
        size_t len = strlen(text);

        /* every character can start new line in the worst case */
        box->buf = malloc(len * 2 + 1);
        if (!box->buf) {
                return false;
        }

        char *out = box->buf;
        int col = 0;
        const char *p = text;

        box->lines = 1;
        box->line[0] = out;

        while (*p) {
                if ((*p == '\n') || (*p == ' ' && col >= width)) {
                        *out++ = '\0';
                        p++;
                        col = 0;

                        if (box->lines < MAX_LINES) {
                                box->line[box->lines++] = out;
                        }
                        continue;
                }

                if (*p == ' ') {
                        *out++ = *p++;
                        col++;
                        continue;
                }

                /* word width */
                const char *w = p;
                int ww = 0;
                while (*w && (*w != ' ') && (*w != '\n')) {
                        tui_utf8_decode(&w);
                        ww++;
                }

                if ((col > 0) && (col + ww > width)) {
                        /* break before word, remove space at end of line */
                        if ((out > box->buf) && (out[-1] == ' ')) {
                                out--;
                        }

                        *out++ = '\0';
                        col = 0;

                        if (box->lines < MAX_LINES) {
                                box->line[box->lines++] = out;
                        }
                }

                while (p < w) {
                        if (col >= width) {
                                *out++ = '\0';
                                col = 0;

                                if (box->lines < MAX_LINES) {
                                        box->line[box->lines++] = out;
                                }
                        }

                        const char *c = p;
                        tui_utf8_decode(&p);
                        memcpy(out, c, p - c);
                        out += p - c;
                        col++;
                }
        }

        *out = '\0';

        return true;
}

//==============================================================================
/**
 * @brief  Print string clipped to selected width.
 */
//==============================================================================
static void print_clip(tui_t *tui, const char *str, int max)
{
        uint32_t ch;
        for (int n = 0; (n < max) && (ch = tui_utf8_decode(&str)); n++) {
                tui_putc(tui, ch);
        }
}

//==============================================================================
/**
 * @brief  Calculate box layout.
 *
 * @param  tui          TUI object
 * @param  box          box
 * @param  title        title (can be NULL)
 * @param  text         text
 * @param  min_w        minimal inner width
 * @param  extra_h      extra rows (input field, buttons)
 *
 * @return true on success.
 */
//==============================================================================
static bool layout(tui_t *tui, box_t *box, const char *title, const char *text, int min_w, int extra_h)
{
        int max_w = tui_cols(tui) - 6;
        int text_w = (max_w < MAX_TEXT_WIDTH) ? max_w : MAX_TEXT_WIDTH;
        text_w = (text_w < 10) ? 10 : text_w;

        if (!wrap(box, text ? text : "", text_w)) {
                return false;
        }

        int w = min_w;
        for (int i = 0; i < box->lines; i++) {
                int lw = tui_strwidth(box->line[i]);
                w = (lw > w) ? lw : w;
        }

        int tw = title ? tui_strwidth(title) + 2 : 0;
        w = (tw > w) ? tw : w;
        w = (w > max_w) ? max_w : w;

        int max_lines = tui_rows(tui) - 4 - extra_h;
        box->lines = (box->lines > max_lines) ? ((max_lines > 0) ? max_lines : 0) : box->lines;

        box->w = w + 4;
        box->h = box->lines + 2 + extra_h;
        box->x = (tui_cols(tui) - box->w) / 2;
        box->y = (tui_rows(tui) - box->h) / 2;

        return true;
}

//==============================================================================
/**
 * @brief  Draw box frame, title and text.
 */
//==============================================================================
static void draw_box(tui_t *tui, box_t *box, const char *title)
{
        uint16_t attr = box->error ? ATTR_ERR : ATTR_BOX;

        tui_set_attr(tui, ATTR_SHADOW);
        tui_fill(tui, box->x + 2, box->y + 1, box->w, box->h, ' ');

        tui_set_attr(tui, attr);
        tui_fill(tui, box->x, box->y, box->w, box->h, ' ');
        tui_box(tui, box->x, box->y, box->w, box->h, TUI_BOX_DOUBLE);

        if (title) {
                int tw = tui_strwidth(title) + 2;
                tw = (tw > box->w - 2) ? box->w - 2 : tw;
                tui_set_attr(tui, box->error ? ATTR_ERR_TITLE : ATTR_TITLE);
                tui_move(tui, box->x + (box->w - tw) / 2, box->y);
                tui_putc(tui, ' ');
                print_clip(tui, title, tw - 2);
                tui_putc(tui, ' ');
        }

        tui_set_attr(tui, attr);
        for (int i = 0; i < box->lines; i++) {
                tui_move(tui, box->x + 2, box->y + 1 + i);
                print_clip(tui, box->line[i], box->w - 4);
        }
}

//==============================================================================
/**
 * @brief  Width of buttons row.
 */
//==============================================================================
static int buttons_width(const char *const *buttons, int count)
{
        int w = 0;

        for (int i = 0; i < count; i++) {
                w += tui_strwidth(buttons[i]) + 4 + ((i > 0) ? 2 : 0);
        }

        return w;
}

//==============================================================================
/**
 * @brief  Draw buttons centered in selected row.
 */
//==============================================================================
static void draw_buttons(tui_t *tui, box_t *box, int y, const char *const *buttons, int count, int sel)
{
        int x = box->x + (box->w - buttons_width(buttons, count)) / 2;

        for (int i = 0; i < count; i++) {
                tui_set_attr(tui, (i == sel) ? ATTR_BTN_SEL : (box->error ? ATTR_BTN_ERR : ATTR_BTN));
                tui_move(tui, x, y);
                tui_puts(tui, "[ ");
                tui_puts(tui, buttons[i]);
                tui_puts(tui, " ]");

                x += tui_strwidth(buttons[i]) + 4 + 2;
        }
}

//==============================================================================
/**
 * @brief  Show question with buttons.
 *
 * @param  tui          TUI object
 * @param  title        title
 * @param  text         text
 * @param  buttons      button labels (first letter is a hot key)
 * @param  count        number of buttons
 * @param  def          default button
 * @param  error        error colors
 *
 * @return Selected button index, -1 if Esc or Ctrl+C pressed.
 */
//==============================================================================
int tui_question(tui_t *tui, const char *title, const char *text, const char *const *buttons, int count,
        int def, bool error)
{
        int sel = ((def >= 0) && (def < count)) ? def : 0;

        for (;;) {
                box_t box = {.error = error};

                if (!layout(tui, &box, title, text, buttons_width(buttons, count), 2)) {
                        return -1;
                }

                draw_box(tui, &box, title);
                draw_buttons(tui, &box, box.y + box.h - 2, buttons, count, sel);
                free(box.buf);

                tui_set_cursor(tui, -1, -1);
                tui_refresh(tui);

                int key = tui_getkey(tui, -1);

                switch (key) {
                case TUI_KEY_ENTER:
                        return sel;

                case TUI_KEY_ESC:
                case TUI_KEY_CTRL('C'):
                case TUI_KEY_NONE:
                        return -1;

                case TUI_KEY_LEFT:
                case TUI_KEY_SHIFT_TAB:
                        sel = (sel + count - 1) % count;
                        break;

                case TUI_KEY_RIGHT:
                case TUI_KEY_TAB:
                        sel = (sel + 1) % count;
                        break;

                case TUI_KEY_CTRL('L'):
                        tui_redraw(tui);
                        break;

                default:
                        for (int i = 0; (key > 0x20) && (key < 0x7F) && (i < count); i++) {
                                if (tolower((unsigned char)buttons[i][0]) == tolower(key)) {
                                        return i;
                                }
                        }
                        break;
                }
        }
}

//==============================================================================
/**
 * @brief  Show message (OK button).
 */
//==============================================================================
void tui_msgbox(tui_t *tui, const char *title, const char *text, bool error)
{
        static const char *const OK[] = {"OK"};
        tui_question(tui, title, text, OK, 1, 0, error);
}

//==============================================================================
/**
 * @brief  Show Yes/No question.
 *
 * @return true if Yes selected.
 */
//==============================================================================
bool tui_yesno(tui_t *tui, const char *title, const char *text, bool def_yes)
{
        static const char *const YES_NO[] = {"Yes", "No"};
        return tui_question(tui, title, text, YES_NO, 2, def_yes ? 0 : 1, false) == 0;
}

//==============================================================================
/**
 * @brief  Show text without waiting for key (e.g. progress).
 */
//==============================================================================
void tui_infobox(tui_t *tui, const char *title, const char *text)
{
        box_t box = {.error = false};

        if (layout(tui, &box, title, text, MIN_INPUT_W, 0)) {
                draw_box(tui, &box, title);
                free(box.buf);
                tui_set_cursor(tui, -1, -1);
                tui_refresh(tui);
        }
}

//==============================================================================
/**
 * @brief  Edit UTF-8 text in buffer.
 *
 * @return true if key was handled.
 */
//==============================================================================
static bool edit_key(int key, char *buf, size_t size, int *cursor)
{
        int len = strlen(buf);
        int pos = *cursor;

        switch (key) {
        case TUI_KEY_LEFT:
                while ((pos > 0) && ((buf[--pos] & 0xC0) == 0x80)) {
                }
                break;

        case TUI_KEY_RIGHT:
                while ((pos < len) && ((buf[++pos] & 0xC0) == 0x80)) {
                }
                break;

        case TUI_KEY_HOME:
        case TUI_KEY_CTRL('A'):
                pos = 0;
                break;

        case TUI_KEY_END:
        case TUI_KEY_CTRL('E'):
                pos = len;
                break;

        case TUI_KEY_BACKSPACE:
                if (pos > 0) {
                        int start = pos;
                        while ((start > 0) && ((buf[--start] & 0xC0) == 0x80)) {
                        }
                        memmove(&buf[start], &buf[pos], len - pos + 1);
                        pos = start;
                }
                break;

        case TUI_KEY_DELETE:
                if (pos < len) {
                        int end = pos;
                        while ((end < len) && ((buf[++end] & 0xC0) == 0x80)) {
                        }
                        memmove(&buf[pos], &buf[end], len - end + 1);
                }
                break;

        case TUI_KEY_CTRL('U'):
                buf[0] = '\0';
                pos = 0;
                break;

        default:
                if ((key >= 0x20) && (key <= 0x10FFFF) && (key != TUI_KEY_BACKSPACE)) {
                        char utf8[4];
                        int n = tui_utf8_encode(key, utf8);
                        if (len + n < (int)size) {
                                memmove(&buf[pos + n], &buf[pos], len - pos + 1);
                                memcpy(&buf[pos], utf8, n);
                                pos += n;
                        }
                        break;
                }
                return false;
        }

        *cursor = pos;
        return true;
}

//==============================================================================
/**
 * @brief  Read text. Buffer contains initial text.
 *
 * @param  tui          TUI object
 * @param  title        title
 * @param  prompt       text above input field
 * @param  buf          buffer (initial and result text)
 * @param  size         buffer size
 *
 * @return true if accepted by Enter, false if canceled (buffer is not changed).
 */
//==============================================================================
bool tui_inputbox(tui_t *tui, const char *title, const char *prompt, char *buf, size_t size)
{
        char *edit = malloc(size);
        if (!edit || (size == 0)) {
                free(edit);
                return false;
        }

        strncpy(edit, buf, size - 1);
        edit[size - 1] = '\0';

        int cursor = strlen(edit);
        int offset = 0;
        bool accepted = false;

        for (;;) {
                box_t box = {.error = false};
                int min_w = tui_cols(tui) / 2;
                min_w = (min_w < MIN_INPUT_W) ? MIN_INPUT_W : min_w;

                if (!layout(tui, &box, title, prompt, min_w, 2)) {
                        break;
                }

                draw_box(tui, &box, title);
                free(box.buf);

                /* input field with horizontal scroll */
                int fx = box.x + 2;
                int fy = box.y + box.h - 2;
                int fw = box.w - 4;

                int col = 0;
                for (const char *p = edit; (p < &edit[cursor]) && tui_utf8_decode(&p);) {
                        col++;
                }

                if (col < offset) {
                        offset = col;
                } else if (col >= offset + fw) {
                        offset = col - fw + 1;
                }

                const char *p = edit;
                for (int i = 0; (i < offset) && tui_utf8_decode(&p); i++) {
                }

                tui_set_attr(tui, ATTR_INPUT);
                tui_fill(tui, fx, fy, fw, 1, ' ');
                tui_move(tui, fx, fy);
                print_clip(tui, p, fw);

                tui_set_cursor(tui, fx + col - offset, fy);
                tui_refresh(tui);

                int key = tui_getkey(tui, -1);

                if (key == TUI_KEY_ENTER) {
                        strcpy(buf, edit);
                        accepted = true;
                        break;
                } else if ((key == TUI_KEY_ESC) || (key == TUI_KEY_CTRL('C')) || (key == TUI_KEY_NONE)) {
                        break;
                } else if (key == TUI_KEY_CTRL('L')) {
                        tui_redraw(tui);
                } else {
                        edit_key(key, edit, size, &cursor);
                }
        }

        free(edit);
        tui_set_cursor(tui, -1, -1);

        return accepted;
}

/*==============================================================================
  End of file
==============================================================================*/
