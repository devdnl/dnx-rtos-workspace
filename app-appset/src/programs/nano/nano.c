/*==============================================================================
@file   nano.c

@author Daniel Zorychta

@brief  Simple text editor in nano style.

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
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <tui.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define VERSION       "1.1"
#define TAB_WIDTH     8
#define LINE_MIN_CAP  16
#define LINES_MIN_CAP 64
#define PROMPT_LEN    128
#define MSG_LEN       128
#define IO_BUF_SIZE   1024
#define FKEY_ITEMS    6
#define UNDO_MAX      1000

#define ATTR_TEXT      TUI_ATTR_DEFAULT
#define ATTR_BAR       (TUI_ATTR_DEFAULT | TUI_REVERSE)
#define ATTR_KEY       (TUI_ATTR_DEFAULT | TUI_REVERSE)
#define ATTR_KEY_LABEL TUI_ATTR_DEFAULT
#define ATTR_SELECT    (TUI_ATTR_DEFAULT | TUI_REVERSE)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        char *s; /* always NUL terminated */
        int len;
        int cap;
} line_t;

typedef struct {
        line_t *v;
        int n;
        int cap;
} lines_t;

typedef enum {
        BAR_EDIT,
        BAR_PROMPT,
        BAR_QUESTION,
        BAR_REPLACE,
} bar_t;

/* undo record: text inserted at or deleted from position */
typedef struct undo {
        struct undo *next;
        int group;   /* records of one user action */
        bool insert; /* true: text was inserted, false: text was deleted */
        bool typing; /* typed characters (next typed character is merged) */
        int y;
        int x;
        int len;
        char *text; /* can contain new lines */
} undo_t;

/* text region (mark or search result) */
typedef struct {
        bool on;
        int y1;
        int x1;
        int y2;
        int x2;
} region_t;

typedef struct {
        const char *key;
        const char *label;
} shortcut_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void set_msg(const char *fmt, ...) __attribute__((format(__printf__, 1, 2)));
static void draw(bar_t bar);
static int pos_cmp(int y1, int x1, int y2, int x2);
static region_t mark_region(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        tui_t *tui;
        lines_t buf;
        char *cut; /* cut buffer (text with new lines) */
        int cut_len;
        int cut_cap;
        undo_t *undo;
        undo_t *redo;
        int undo_group;
        int undo_count;
        bool mark; /* region from mark to cursor */
        int mark_y;
        int mark_x;
        bool hl; /* highlighted search result */
        int hl_y;
        int hl_x;
        int hl_len;
        int drawn_rowoff; /* text offsets of last drawn screen (-1: not known) */
        int drawn_coloff;
        char *filename;
        int cy;
        int cx;
        int want_col;
        int rowoff;
        int coloff;
        bool modified;
        bool crlf;
        bool cut_continue;
        bool vertical_move;
        bool quit;
        char msg[MSG_LEN];
        char search[PROMPT_LEN];
        char input[PROMPT_LEN];
        const char *prompt_label;
        int prompt_pos;
};

static const shortcut_t SHORTCUTS_EDIT[2][FKEY_ITEMS] = {
        {
                {"^G", "Help"},
                {"^O", "Write Out"},
                {"^W", "Where Is"},
                {"^K", "Cut"},
                {"^\\", "Replace"},
                {"M-U", "Undo"},
        },
        {
                {"^X", "Exit"},
                {"^S", "Save"},
                {"M-A", "Mark"},
                {"^U", "Paste"},
                {"^_", "Go To Line"},
                {"M-E", "Redo"},
        },
};

static const shortcut_t SHORTCUTS_PROMPT[2][FKEY_ITEMS] = {
        {{"^C", "Cancel"}, {"Enter", "Accept"}},
        {{"", ""}},
};

static const shortcut_t SHORTCUTS_QUESTION[2][FKEY_ITEMS] = {
        {{"Y", "Yes"}, {"N", "No"}},
        {{"^C", "Cancel"}},
};

static const shortcut_t SHORTCUTS_REPLACE[2][FKEY_ITEMS] = {
        {{"Y", "Yes"}, {"N", "No"}, {"A", "All"}},
        {{"^C", "Cancel"}},
};

static const char *const HELP_TEXT[] = {
        "nano (dnx) " VERSION " - simple text editor",
        "",
        "  ^G  F1          Show this help",
        "  ^X  F2          Exit (asks to save modified buffer)",
        "  ^O  F3          Write out (asks for file name)",
        "  ^S              Save to current file",
        "  ^W  F6          Search (case insensitive, empty: repeat last)",
        "  ^\\  M-R         Replace (Y: this, N: skip, A: all)",
        "  ^K  F9          Cut marked text or current line (lines are collected)",
        "  M-^ M-C         Copy marked text or current line",
        "  ^U  F10         Paste",
        "  M-A ^^          Set/unset mark (select text with cursor keys)",
        "  M-U  M-E        Undo, redo",
        "  ^C              Show cursor position",
        "  ^_  M-G         Go to line (\"line\" or \"line,column\")",
        "  ^L              Refresh screen",
        "",
        "  Arrows, Home/^A, End/^E, PgUp/^Y, PgDn/^V   Move cursor",
        "  Ctrl+Left, Ctrl+Right                       Previous/next word",
        "  Ctrl+Home, Ctrl+End, M-\\, M-/               First/last line",
        "  Backspace, Delete/^D, Tab, Enter            Edit",
        "",
        "  Esc+digit works as F-key (when terminal catches F keys).",
        "  DOS line endings (CR LF) are detected and kept.",
        "",
        "Press any key to return.",
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(nano, STACK_DEPTH_LARGE);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Set status message (shown until next key).
 */
//==============================================================================
static void set_msg(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vsnprintf(global->msg, sizeof(global->msg), fmt, args);
        va_end(args);
}

//==============================================================================
/**
 * @brief Reserve line capacity (including NUL).
 *
 * @return true on success, false when there is no memory.
 */
//==============================================================================
static bool line_reserve(line_t *line, int need)
{
        need += 1;

        if (line->cap >= need) {
                return true;
        }

        int cap = (line->cap * 2 > need) ? line->cap * 2 : need;
        if (cap < LINE_MIN_CAP) {
                cap = LINE_MIN_CAP;
        }

        char *s = realloc(line->s, cap);
        if (!s) {
                return false;
        }

        line->s = s;
        line->cap = cap;

        return true;
}

//==============================================================================
/**
 * @brief Insert bytes into line.
 */
//==============================================================================
static bool line_insert(line_t *line, int pos, const char *s, int n)
{
        if (!line_reserve(line, line->len + n)) {
                return false;
        }

        memmove(&line->s[pos + n], &line->s[pos], line->len - pos + 1);
        memcpy(&line->s[pos], s, n);
        line->len += n;

        return true;
}

//==============================================================================
/**
 * @brief Delete bytes from line.
 */
//==============================================================================
static void line_delete(line_t *line, int pos, int n)
{
        memmove(&line->s[pos], &line->s[pos + n], line->len - pos - n + 1);
        line->len -= n;
}

//==============================================================================
/**
 * @brief Insert new line (copy of bytes) into lines at selected index.
 */
//==============================================================================
static bool lines_insert(lines_t *lines, int idx, const char *s, int n)
{
        if (lines->n == lines->cap) {
                int cap = (lines->cap < LINES_MIN_CAP) ? LINES_MIN_CAP : lines->cap * 2;
                line_t *v = realloc(lines->v, cap * sizeof(line_t));
                if (!v) {
                        return false;
                }

                lines->v = v;
                lines->cap = cap;
        }

        line_t line = {NULL, 0, 0};
        if (!line_reserve(&line, n)) {
                return false;
        }

        memcpy(line.s, s, n);
        line.s[n] = '\0';
        line.len = n;

        memmove(&lines->v[idx + 1], &lines->v[idx], (lines->n - idx) * sizeof(line_t));
        lines->v[idx] = line;
        lines->n++;

        return true;
}

//==============================================================================
/**
 * @brief Delete line.
 */
//==============================================================================
static void lines_delete(lines_t *lines, int idx)
{
        free(lines->v[idx].s);
        memmove(&lines->v[idx], &lines->v[idx + 1], (lines->n - idx - 1) * sizeof(line_t));
        lines->n--;
}

//==============================================================================
/**
 * @brief Delete all lines.
 */
//==============================================================================
static void lines_clear(lines_t *lines)
{
        while (lines->n) {
                lines_delete(lines, lines->n - 1);
        }
}

//==============================================================================
/**
 * @brief Return length of UTF-8 character at position (1 for invalid byte).
 */
//==============================================================================
static int char_len(const line_t *line, int pos)
{
        uint8_t c = line->s[pos];
        int n = (c < 0x80)             ? 1
                : ((c & 0xE0) == 0xC0) ? 2
                : ((c & 0xF0) == 0xE0) ? 3
                : ((c & 0xF8) == 0xF0) ? 4
                                       : 1;

        if (pos + n > line->len) {
                return 1;
        }

        for (int i = 1; i < n; i++) {
                if ((line->s[pos + i] & 0xC0) != 0x80) {
                        return 1;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief Decode character at position (U+FFFD for invalid byte).
 */
//==============================================================================
static uint32_t char_at(const line_t *line, int pos, int n)
{
        const uint8_t *s = (const uint8_t *)&line->s[pos];

        if (n == 1) {
                return (s[0] < 0x80) ? s[0] : 0xFFFD;
        }

        uint32_t ch = s[0] & (0x7F >> n);
        for (int i = 1; i < n; i++) {
                ch = (ch << 6) | (s[i] & 0x3F);
        }

        return ch;
}

//==============================================================================
/**
 * @brief Return position of previous character.
 */
//==============================================================================
static int prev_pos(const line_t *line, int pos)
{
        if (pos <= 0) {
                return 0;
        }

        int start = pos - 1;
        while ((start > 0) && ((line->s[start] & 0xC0) == 0x80) && (pos - start < 4)) {
                start--;
        }

        return (start + char_len(line, start) == pos) ? start : pos - 1;
}

//==============================================================================
/**
 * @brief Return display column of position.
 */
//==============================================================================
static int col_of(const line_t *line, int pos)
{
        int col = 0;

        for (int i = 0; (i < pos) && (i < line->len); i += char_len(line, i)) {
                col = (line->s[i] == '\t') ? (col / TAB_WIDTH + 1) * TAB_WIDTH : col + 1;
        }

        return col;
}

//==============================================================================
/**
 * @brief Return position of character shown at (or before) display column.
 */
//==============================================================================
static int pos_of_col(const line_t *line, int target)
{
        int col = 0;
        int pos = 0;

        while (pos < line->len) {
                int ncol = (line->s[pos] == '\t') ? (col / TAB_WIDTH + 1) * TAB_WIDTH : col + 1;
                if (ncol > target) {
                        break;
                }

                col = ncol;
                pos += char_len(line, pos);
        }

        return pos;
}

//==============================================================================
/**
 * @brief Check if byte is part of word.
 */
//==============================================================================
static bool is_word(char c)
{
        return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) ||
                (c == '_') || ((uint8_t)c >= 0x80);
}

//==============================================================================
/**
 * @brief Current line.
 */
//==============================================================================
static line_t *cur_line(void)
{
        return &global->buf.v[global->cy];
}

//==============================================================================
/**
 * @brief Number of text lines (empty line after last line ending is not counted).
 */
//==============================================================================
static int count_lines(void)
{
        int n = global->buf.n;
        return (n > 0 && global->buf.v[n - 1].len == 0) ? n - 1 : n;
}

//==============================================================================
/**
 * @brief Load file. Not existing file creates new buffer.
 *
 * @return 0 on success, otherwise errno value.
 */
//==============================================================================
static int load_file(const char *name)
{
        FILE *f = fopen(name, "r");
        if (!f) {
                if (errno == ENOENT) {
                        set_msg("New File");
                        return lines_insert(&global->buf, 0, "", 0) ? 0 : ENOMEM;
                }

                return errno ? errno : EIO;
        }

        int err = 0;
        char *io = malloc(IO_BUF_SIZE);
        line_t cur = {NULL, 0, 0};
        int newlines = 0;
        int cr_lines = 0;

        if (!io || !line_reserve(&cur, 0)) {
                err = ENOMEM;
        }

        while (!err) {
                size_t n = fread(io, 1, IO_BUF_SIZE, f);
                if (n == 0) {
                        break;
                }

                for (size_t i = 0; (i < n) && !err; i++) {
                        if (io[i] == '\n') {
                                newlines++;

                                if (cur.len && (cur.s[cur.len - 1] == '\r')) {
                                        cr_lines++;
                                }

                                if (!lines_insert(&global->buf, global->buf.n, cur.s, cur.len)) {
                                        err = ENOMEM;
                                }

                                cur.len = 0;

                        } else if (line_reserve(&cur, cur.len + 1)) {
                                cur.s[cur.len++] = io[i];

                        } else {
                                err = ENOMEM;
                        }
                }
        }

        if (!err && ferror(f)) {
                err = EIO;
        }

        /* last line (after last LF, can be empty) */
        if (!err && !lines_insert(&global->buf, global->buf.n, cur.s ? cur.s : "", cur.len)) {
                err = ENOMEM;
        }

        free(cur.s);
        free(io);
        fclose(f);

        if (!err && (newlines > 0) && (cr_lines == newlines)) {
                global->crlf = true;

                for (int i = 0; i < global->buf.n - 1; i++) {
                        line_delete(&global->buf.v[i], global->buf.v[i].len - 1, 1);
                }
        }

        if (!err) {
                set_msg("Read %d lines%s", count_lines(), global->crlf ? " (DOS format)" : "");
        }

        return err;
}

//==============================================================================
/**
 * @brief Write buffered data to file.
 */
//==============================================================================
static bool flush_io(FILE *f, char *io, int *len)
{
        bool ok = (*len == 0) || (fwrite(io, 1, *len, f) == (size_t)*len);
        *len = 0;
        return ok;
}

//==============================================================================
/**
 * @brief Save buffer to file. Lines are joined by line endings, so file read
 *        and written without changes is not modified.
 *
 * @return true on success.
 */
//==============================================================================
static bool save_file(const char *name)
{
        char *io = malloc(IO_BUF_SIZE);
        if (!io) {
                set_msg("Error writing %s: not enough memory", name);
                return false;
        }

        FILE *f = fopen(name, "w");
        if (!f) {
                set_msg("Error writing %s: %s", name, strerror(errno));
                free(io);
                return false;
        }

        const char *eol = global->crlf ? "\r\n" : "\n";
        int eol_len = strlen(eol);
        int len = 0;
        bool ok = true;

        for (int i = 0; (i < global->buf.n) && ok; i++) {
                const line_t *line = &global->buf.v[i];

                for (int pos = 0; (pos < line->len) && ok; pos++) {
                        if (len == IO_BUF_SIZE) {
                                ok = flush_io(f, io, &len);
                        }

                        io[len++] = line->s[pos];
                }

                if (ok && (i < global->buf.n - 1)) {
                        if (len + eol_len > IO_BUF_SIZE) {
                                ok = flush_io(f, io, &len);
                        }

                        memcpy(&io[len], eol, eol_len);
                        len += eol_len;
                }
        }

        if (ok) {
                ok = flush_io(f, io, &len);
        }

        if (fclose(f) != 0) {
                ok = false;
        }

        free(io);

        if (ok) {
                global->modified = false;
                set_msg("Wrote %d lines", count_lines());
        } else {
                set_msg("Error writing %s: %s", name, strerror(errno ? errno : EIO));
        }

        return ok;
}

//==============================================================================
/**
 * @brief Adjust screen offsets to show cursor.
 */
//==============================================================================
static void scroll(int text_rows, int cols)
{
        if (global->cy < global->rowoff) {
                global->rowoff = global->cy;
        } else if (global->cy >= global->rowoff + text_rows) {
                global->rowoff = global->cy - text_rows + 1;
        }

        /* horizontal scroll by larger steps (whole text is sent again) */
        int col = col_of(cur_line(), global->cx);

        if (col < global->coloff) {
                global->coloff = (col > cols / 4) ? col - cols / 4 : 0;
        } else if (col >= global->coloff + cols) {
                global->coloff = col - (cols * 3) / 4;
        }
}

//==============================================================================
/**
 * @brief Draw text line (characters in region are highlighted).
 *
 * @param line          line
 * @param idx           line index
 * @param y             screen row
 * @param cols          screen columns
 * @param r             region
 */
//==============================================================================
static void draw_line(const line_t *line, int idx, int y, int cols, const region_t *r)
{
        tui_t *tui = global->tui;
        int col = 0;
        int end = global->coloff + cols;

        tui_move(tui, 0, y);

        for (int pos = 0; (pos < line->len) && (col < end);) {
                int n = char_len(line, pos);

                bool selected = r->on && (pos_cmp(idx, pos, r->y1, r->x1) >= 0) &&
                        (pos_cmp(idx, pos, r->y2, r->x2) < 0);
                tui_set_attr(tui, selected ? ATTR_SELECT : ATTR_TEXT);

                if (line->s[pos] == '\t') {
                        int next = (col / TAB_WIDTH + 1) * TAB_WIDTH;
                        for (; col < next; col++) {
                                if ((col >= global->coloff) && (col < end)) {
                                        tui_putc(tui, ' ');
                                }
                        }
                } else {
                        if (col >= global->coloff) {
                                tui_putc(tui, char_at(line, pos, n));
                        }

                        col++;
                }

                pos += n;
        }
}

//==============================================================================
/**
 * @brief Draw shortcut bars.
 */
//==============================================================================
static void draw_shortcuts(bar_t bar, int rows, int cols)
{
        tui_t *tui = global->tui;
        const shortcut_t(*items)[FKEY_ITEMS] = (bar == BAR_PROMPT) ? SHORTCUTS_PROMPT
                : (bar == BAR_QUESTION)                            ? SHORTCUTS_QUESTION
                : (bar == BAR_REPLACE)                             ? SHORTCUTS_REPLACE
                                                                   : SHORTCUTS_EDIT;
        int w = cols / FKEY_ITEMS;

        for (int r = 0; r < 2; r++) {
                int y = rows - 2 + r;

                tui_set_attr(tui, ATTR_KEY_LABEL);
                tui_fill(tui, 0, y, cols, 1, ' ');

                for (int i = 0; i < FKEY_ITEMS; i++) {
                        const shortcut_t *item = &items[r][i];
                        if (!item->key || !item->key[0]) {
                                continue;
                        }

                        int key_w = tui_print_at(tui, i * w, y, ATTR_KEY, item->key);
                        int label_w = w - key_w - 1;

                        if (label_w > 0) {
                                tui_set_attr(tui, ATTR_KEY_LABEL);
                                tui_printf(tui, " %.*s", label_w, item->label);
                        }
                }
        }
}

//==============================================================================
/**
 * @brief Draw editor screen.
 *
 * @param bar           shortcut bar and status line mode
 */
//==============================================================================
static void draw(bar_t bar)
{
        tui_t *tui = global->tui;
        int cols = tui_cols(tui);
        int rows = tui_rows(tui);
        int text_rows = (rows > 5) ? rows - 4 : 1;

        scroll(text_rows, cols);

        /* text moved vertically: terminal scrolls, only new lines are sent */
        if ((global->drawn_rowoff >= 0) && (global->coloff == global->drawn_coloff) &&
                (global->rowoff != global->drawn_rowoff)) {
                tui_scroll(tui, 1, text_rows, global->rowoff - global->drawn_rowoff);
        }

        global->drawn_rowoff = global->rowoff;
        global->drawn_coloff = global->coloff;

        /* highlighted region: mark or search result */
        region_t region = mark_region();
        if (!region.on && global->hl) {
                region = (region_t) {true, global->hl_y, global->hl_x, global->hl_y,
                        global->hl_x + global->hl_len};
        }

        /* title bar */
        tui_set_attr(tui, ATTR_BAR);
        tui_fill(tui, 0, 0, cols, 1, ' ');
        tui_move(tui, 2, 0);
        tui_puts(tui, "nano (dnx) " VERSION);

        const char *name = global->filename ? global->filename : "New Buffer";
        int name_w = tui_strwidth(name);
        tui_move(tui, (cols - name_w) / 2, 0);
        tui_puts(tui, name);

        if (global->modified) {
                tui_move(tui, cols - 10, 0);
                tui_puts(tui, "Modified");
        }

        if (global->mark) {
                tui_move(tui, cols - 17, 0);
                tui_puts(tui, "Mark");
        }

        /* text */
        tui_set_attr(tui, ATTR_TEXT);
        tui_fill(tui, 0, 1, cols, text_rows, ' ');

        for (int r = 0; r < text_rows; r++) {
                int idx = global->rowoff + r;
                if (idx < global->buf.n) {
                        draw_line(&global->buf.v[idx], idx, 1 + r, cols, &region);
                }
        }

        /* status line */
        int status_y = rows - 3;
        tui_set_attr(tui, ATTR_TEXT);
        tui_fill(tui, 0, status_y, cols, 1, ' ');

        if ((bar == BAR_PROMPT) || (bar == BAR_QUESTION) || (bar == BAR_REPLACE)) {
                tui_set_attr(tui, ATTR_BAR);
                tui_fill(tui, 0, status_y, cols, 1, ' ');
                tui_move(tui, 0, status_y);
                int label_w = tui_puts(tui, global->prompt_label);

                if (bar == BAR_PROMPT) {
                        tui_puts(tui, global->input);

                        int cur = label_w;
                        const char *p = global->input;
                        while ((p < &global->input[global->prompt_pos]) && tui_utf8_decode(&p)) {
                                cur++;
                        }

                        tui_set_cursor(tui, cur, status_y);
                } else {
                        tui_set_cursor(tui, label_w, status_y);
                }

        } else {
                if (global->msg[0]) {
                        int w = tui_strwidth(global->msg) + 4;
                        tui_set_attr(tui, ATTR_BAR);
                        tui_move(tui, (cols - w) / 2, status_y);
                        tui_printf(tui, "[ %s ]", global->msg);
                }

                int col = col_of(cur_line(), global->cx);
                tui_set_cursor(tui, col - global->coloff, 1 + global->cy - global->rowoff);
        }

        draw_shortcuts(bar, rows, cols);
}

//==============================================================================
/**
 * @brief Read text in status line.
 *
 * @param label         prompt
 * @param initial       initial text (can be NULL)
 *
 * @return true when accepted (text in global->input), false when canceled.
 */
//==============================================================================
static bool prompt(const char *label, const char *initial)
{
        char *in = global->input;
        size_t size = sizeof(global->input);

        snprintf(in, size, "%s", initial ? initial : "");
        global->prompt_label = label;
        global->prompt_pos = strlen(in);

        for (;;) {
                draw(BAR_PROMPT);
                tui_refresh(global->tui);

                int key = tui_getkey(global->tui, -1);
                int len = strlen(in);
                int pos = global->prompt_pos;

                if (key == TUI_KEY_ENTER) {
                        return true;

                } else if ((key == TUI_KEY_CTRL('C')) || (key == TUI_KEY_ESC) || (key == TUI_KEY_NONE)) {
                        set_msg("Cancelled");
                        return false;

                } else if (key == TUI_KEY_CTRL('L')) {
                        tui_redraw(global->tui);

                } else if (key == TUI_KEY_LEFT) {
                        while ((pos > 0) && ((in[--pos] & 0xC0) == 0x80)) {
                        }

                } else if (key == TUI_KEY_RIGHT) {
                        while ((pos < len) && ((in[++pos] & 0xC0) == 0x80)) {
                        }

                } else if ((key == TUI_KEY_HOME) || (key == TUI_KEY_CTRL('A'))) {
                        pos = 0;

                } else if ((key == TUI_KEY_END) || (key == TUI_KEY_CTRL('E'))) {
                        pos = len;

                } else if ((key == TUI_KEY_BACKSPACE) && (pos > 0)) {
                        int start = pos;
                        while ((start > 0) && ((in[--start] & 0xC0) == 0x80)) {
                        }
                        memmove(&in[start], &in[pos], len - pos + 1);
                        pos = start;

                } else if ((key == TUI_KEY_DELETE) && (pos < len)) {
                        int end = pos;
                        while ((end < len) && ((in[++end] & 0xC0) == 0x80)) {
                        }
                        memmove(&in[pos], &in[end], len - end + 1);

                } else if ((key >= 0x20) && (key <= 0x10FFFF) && (key != TUI_KEY_BACKSPACE)) {
                        char utf8[4];
                        int n = tui_utf8_encode(key, utf8);
                        if (len + n < (int)size) {
                                memmove(&in[pos + n], &in[pos], len - pos + 1);
                                memcpy(&in[pos], utf8, n);
                                pos += n;
                        }
                }

                global->prompt_pos = pos;
        }
}

//==============================================================================
/**
 * @brief Ask Yes/No (All) question.
 *
 * @param text          question
 * @param all           "All" answer is possible
 *
 * @return 1: yes, 0: no, 2: all, -1: canceled.
 */
//==============================================================================
static int question_ex(const char *text, bool all)
{
        global->prompt_label = text;

        for (;;) {
                draw(all ? BAR_REPLACE : BAR_QUESTION);
                tui_refresh(global->tui);

                int key = tui_getkey(global->tui, -1);

                if ((key == 'y') || (key == 'Y')) {
                        return 1;
                } else if ((key == 'n') || (key == 'N')) {
                        return 0;
                } else if (all && ((key == 'a') || (key == 'A'))) {
                        return 2;
                } else if ((key == TUI_KEY_CTRL('C')) || (key == TUI_KEY_ESC) || (key == TUI_KEY_NONE)) {
                        set_msg("Cancelled");
                        return -1;
                } else if (key == TUI_KEY_CTRL('L')) {
                        tui_redraw(global->tui);
                }
        }
}

//==============================================================================
/**
 * @brief Ask Yes/No question.
 *
 * @return 1: yes, 0: no, -1: canceled.
 */
//==============================================================================
static int question(const char *text)
{
        return question_ex(text, false);
}

//==============================================================================
/**
 * @brief Set file name.
 */
//==============================================================================
static bool set_filename(const char *name)
{
        char *copy = strdup(name);
        if (!copy) {
                set_msg("Not enough memory");
                return false;
        }

        free(global->filename);
        global->filename = copy;

        return true;
}

//==============================================================================
/**
 * @brief Write out: ask for file name and save.
 */
//==============================================================================
static bool cmd_write_out(void)
{
        if (!prompt("File Name to Write: ", global->filename)) {
                return false;
        }

        if (global->input[0] == '\0') {
                set_msg("Cancelled");
                return false;
        }

        if (!global->filename || (strcmp(global->filename, global->input) != 0)) {
                if (!set_filename(global->input)) {
                        return false;
                }
        }

        return save_file(global->filename);
}

//==============================================================================
/**
 * @brief Save to current file (ask for name if not set).
 */
//==============================================================================
static bool cmd_save(void)
{
        return global->filename ? save_file(global->filename) : cmd_write_out();
}

//==============================================================================
/**
 * @brief Exit (ask to save modified buffer).
 */
//==============================================================================
static void cmd_exit(void)
{
        if (!global->modified) {
                global->quit = true;
                return;
        }

        int answer = question("Save modified buffer?  ");

        if (answer == 0) {
                global->quit = true;
        } else if ((answer == 1) && cmd_save()) {
                global->quit = true;
        }
}

//==============================================================================
/**
 * @brief Compare positions.
 *
 * @return <0, 0, >0 like strcmp().
 */
//==============================================================================
static int pos_cmp(int y1, int x1, int y2, int x2)
{
        if (y1 != y2) {
                return (y1 < y2) ? -1 : 1;
        }

        return (x1 < x2) ? -1 : (x1 > x2) ? 1 : 0;
}

//==============================================================================
/**
 * @brief Calculate end position of text inserted at selected position.
 */
//==============================================================================
static void text_end(int y, int x, const char *text, int len, int *ey, int *ex)
{
        *ey = y;
        *ex = x;

        for (int i = 0; i < len; i++) {
                if (text[i] == '\n') {
                        (*ey)++;
                        *ex = 0;
                } else {
                        (*ex)++;
                }
        }
}

//==============================================================================
/**
 * @brief Insert text (can contain new lines) into document.
 *
 * @return true on success, false when there is no memory (document is not
 *         changed).
 */
//==============================================================================
static bool doc_insert(int y, int x, const char *text, int len)
{
        lines_t *b = &global->buf;
        const char *first_nl = memchr(text, '\n', len);

        if (!first_nl) {
                return line_insert(&b->v[y], x, text, len);
        }

        int extra = 0;
        for (int i = 0; i < len; i++) {
                extra += (text[i] == '\n');
        }

        /* line array is reserved first, so lines are moved only once */
        if (b->n + extra > b->cap) {
                int cap = (b->cap < LINES_MIN_CAP) ? LINES_MIN_CAP : b->cap;
                while (cap < b->n + extra) {
                        cap *= 2;
                }

                line_t *v = realloc(b->v, cap * sizeof(line_t));
                if (!v) {
                        return false;
                }

                b->v = v;
                b->cap = cap;
        }

        line_t *nl = calloc(extra, sizeof(line_t));
        if (!nl) {
                return false;
        }

        line_t *line = &b->v[y];
        int tail_len = line->len - x;
        const char *end = text + len;
        const char *seg = first_nl + 1;
        bool ok = true;

        /* new lines: segments after new lines, the last one gets tail of line */
        for (int k = 0; (k < extra) && ok; k++) {
                const char *e = memchr(seg, '\n', end - seg);
                e = e ? e : end;

                int slen = e - seg;
                int need = slen + ((k == extra - 1) ? tail_len : 0);

                if (line_reserve(&nl[k], need)) {
                        memcpy(nl[k].s, seg, slen);
                        if (k == extra - 1) {
                                memcpy(&nl[k].s[slen], &line->s[x], tail_len);
                        }
                        nl[k].len = need;
                        nl[k].s[need] = '\0';
                } else {
                        ok = false;
                }

                seg = e + 1;
        }

        int first_len = first_nl - text;
        if (ok && !line_reserve(line, x + first_len)) {
                ok = false;
        }

        if (!ok) {
                for (int k = 0; k < extra; k++) {
                        free(nl[k].s);
                }
                free(nl);
                return false;
        }

        memcpy(&line->s[x], text, first_len);
        line->len = x + first_len;
        line->s[line->len] = '\0';

        memmove(&b->v[y + 1 + extra], &b->v[y + 1], (b->n - y - 1) * sizeof(line_t));
        memcpy(&b->v[y + 1], nl, extra * sizeof(line_t));
        b->n += extra;

        free(nl);

        return true;
}

//==============================================================================
/**
 * @brief Copy text of region (lines are separated by new lines).
 *
 * @return Allocated text or NULL.
 */
//==============================================================================
static char *doc_extract(int y1, int x1, int y2, int x2, int *len)
{
        size_t n = 0;

        for (int y = y1; y <= y2; y++) {
                int a = (y == y1) ? x1 : 0;
                int b = (y == y2) ? x2 : global->buf.v[y].len;
                n += (b - a) + ((y < y2) ? 1 : 0);
        }

        char *text = malloc(n + 1);
        if (!text) {
                return NULL;
        }

        size_t k = 0;
        for (int y = y1; y <= y2; y++) {
                int a = (y == y1) ? x1 : 0;
                int b = (y == y2) ? x2 : global->buf.v[y].len;
                memcpy(&text[k], &global->buf.v[y].s[a], b - a);
                k += b - a;

                if (y < y2) {
                        text[k++] = '\n';
                }
        }

        text[k] = '\0';
        *len = n;

        return text;
}

//==============================================================================
/**
 * @brief Remove region from document.
 *
 * @return true on success.
 */
//==============================================================================
static bool doc_remove(int y1, int x1, int y2, int x2)
{
        lines_t *b = &global->buf;

        if (y1 == y2) {
                line_delete(&b->v[y1], x1, x2 - x1);
                return true;
        }

        int tail = b->v[y2].len - x2;

        if (!line_reserve(&b->v[y1], x1 + tail)) {
                return false;
        }

        line_t *first = &b->v[y1];
        memcpy(&first->s[x1], &b->v[y2].s[x2], tail);
        first->len = x1 + tail;
        first->s[first->len] = '\0';

        for (int y = y1 + 1; y <= y2; y++) {
                free(b->v[y].s);
        }

        memmove(&b->v[y1 + 1], &b->v[y2 + 1], (b->n - y2 - 1) * sizeof(line_t));
        b->n -= (y2 - y1);

        return true;
}

//==============================================================================
/**
 * @brief Release undo list.
 */
//==============================================================================
static void undo_free(undo_t **list)
{
        while (*list) {
                undo_t *u = *list;
                *list = u->next;
                free(u->text);
                free(u);
        }
}

//==============================================================================
/**
 * @brief Store undo record (redo list is cleared). Typed characters are
 *        merged with previous typed text.
 */
//==============================================================================
static void undo_record(bool insert, int y, int x, const char *text, int len, bool typing)
{
        undo_free(&global->redo);

        undo_t *top = global->undo;

        if (typing && insert && top && top->typing && top->insert && (top->y == y) &&
                (top->x + top->len == x) && !memchr(text, '\n', len)) {

                char *t = realloc(top->text, top->len + len + 1);
                if (t) {
                        memcpy(&t[top->len], text, len);
                        top->len += len;
                        t[top->len] = '\0';
                        top->text = t;
                        top->group = global->undo_group;
                        return;
                }
        }

        undo_t *u = calloc(1, sizeof(undo_t));
        char *t = malloc(len + 1);

        if (!u || !t) {
                free(u);
                free(t);
                set_msg("Not enough memory for undo");
                return;
        }

        memcpy(t, text, len);
        t[len] = '\0';

        u->group = global->undo_group;
        u->insert = insert;
        u->typing = typing;
        u->y = y;
        u->x = x;
        u->len = len;
        u->text = t;
        u->next = global->undo;
        global->undo = u;
        global->undo_count++;

        /* the oldest record is removed */
        if (global->undo_count > UNDO_MAX) {
                undo_t *prev = global->undo;
                while (prev->next && prev->next->next) {
                        prev = prev->next;
                }

                undo_free(&prev->next);
                global->undo_count--;
        }
}

//==============================================================================
/**
 * @brief Insert text at cursor (cursor is moved to the end of text).
 *
 * @return true on success.
 */
//==============================================================================
static bool edit_insert(const char *text, int len, bool typing)
{
        int y = global->cy;
        int x = global->cx;

        if (!doc_insert(y, x, text, len)) {
                set_msg("Not enough memory");
                return false;
        }

        undo_record(true, y, x, text, len, typing);
        text_end(y, x, text, len, &global->cy, &global->cx);
        global->modified = true;

        return true;
}

//==============================================================================
/**
 * @brief Delete region (cursor is moved to the beginning of region).
 *
 * @return true on success.
 */
//==============================================================================
static bool edit_delete(int y1, int x1, int y2, int x2)
{
        int len;
        char *text = doc_extract(y1, x1, y2, x2, &len);

        if (!text || !doc_remove(y1, x1, y2, x2)) {
                free(text);
                set_msg("Not enough memory");
                return false;
        }

        undo_record(false, y1, x1, text, len, false);
        free(text);

        global->cy = y1;
        global->cx = x1;
        global->modified = true;

        return true;
}

//==============================================================================
/**
 * @brief Undo (redo) last action.
 *
 * @param redo          true: redo, false: undo
 */
//==============================================================================
static void cmd_undo(bool redo)
{
        undo_t **from = redo ? &global->redo : &global->undo;
        undo_t **to = redo ? &global->undo : &global->redo;
        undo_t *u = *from;

        if (!u) {
                set_msg(redo ? "Nothing to redo" : "Nothing to undo");
                return;
        }

        int group = u->group;

        /* undo goes from the newest record, redo from the oldest one */
        while (u && (u->group == group)) {
                int ey, ex;
                text_end(u->y, u->x, u->text, u->len, &ey, &ex);

                /* undo of insert and redo of delete remove text */
                bool remove = (u->insert != redo);
                bool ok = remove ? doc_remove(u->y, u->x, ey, ex) : doc_insert(u->y, u->x, u->text, u->len);

                if (!ok) {
                        set_msg("Not enough memory");
                        break;
                }

                global->cy = remove ? u->y : ey;
                global->cx = remove ? u->x : ex;

                *from = u->next;
                u->next = *to;
                u->typing = false;
                *to = u;
                global->undo_count += redo ? 1 : -1;

                u = *from;
        }

        global->modified = true;
        global->mark = false;
        set_msg(redo ? "Redid action" : "Undid action");
}

//==============================================================================
/**
 * @brief Selected region (mark to cursor) in order.
 */
//==============================================================================
static region_t mark_region(void)
{
        region_t r = {.on = false};

        if (!global->mark) {
                return r;
        }

        /* mark could be moved out of text by editing */
        if (global->mark_y >= global->buf.n) {
                global->mark_y = global->buf.n - 1;
        }

        int len = global->buf.v[global->mark_y].len;
        if (global->mark_x > len) {
                global->mark_x = len;
        }

        r.on = true;

        if (pos_cmp(global->mark_y, global->mark_x, global->cy, global->cx) <= 0) {
                r.y1 = global->mark_y;
                r.x1 = global->mark_x;
                r.y2 = global->cy;
                r.x2 = global->cx;
        } else {
                r.y1 = global->cy;
                r.x1 = global->cx;
                r.y2 = global->mark_y;
                r.x2 = global->mark_x;
        }

        return r;
}

//==============================================================================
/**
 * @brief Set (append) cut buffer.
 *
 * @return true on success.
 */
//==============================================================================
static bool cut_store(const char *text, int len, bool append)
{
        if (!append) {
                global->cut_len = 0;
        }

        if (global->cut_len + len + 1 > global->cut_cap) {
                int cap = (global->cut_len + len + 1) * 2;
                char *c = realloc(global->cut, cap);
                if (!c) {
                        set_msg("Not enough memory");
                        return false;
                }

                global->cut = c;
                global->cut_cap = cap;
        }

        memcpy(&global->cut[global->cut_len], text, len);
        global->cut_len += len;
        global->cut[global->cut_len] = '\0';

        return true;
}

//==============================================================================
/**
 * @brief Region of current line with line ending (last line: line content).
 */
//==============================================================================
static region_t line_region(void)
{
        region_t r = {.on = true, .y1 = global->cy, .x1 = 0};

        if (global->cy < global->buf.n - 1) {
                r.y2 = global->cy + 1;
                r.x2 = 0;
        } else {
                r.y2 = global->cy;
                r.x2 = cur_line()->len;
        }

        return r;
}

//==============================================================================
/**
 * @brief Cut marked region or current line (consecutive line cuts are
 *        collected).
 *
 * @return true if line was cut (for collecting).
 */
//==============================================================================
static bool cmd_cut(bool collect)
{
        region_t r = mark_region();
        bool line = !r.on;

        if (line) {
                r = line_region();
        }

        global->mark = false;

        if (pos_cmp(r.y1, r.x1, r.y2, r.x2) == 0) {
                return line;
        }

        int len;
        char *text = doc_extract(r.y1, r.x1, r.y2, r.x2, &len);

        if (text && cut_store(text, len, line && collect)) {
                edit_delete(r.y1, r.x1, r.y2, r.x2);
        }

        free(text);

        return line;
}

//==============================================================================
/**
 * @brief Copy marked region or current line to cut buffer.
 */
//==============================================================================
static void cmd_copy(void)
{
        region_t r = mark_region();

        if (!r.on) {
                r = line_region();
        }

        global->mark = false;

        int len;
        char *text = doc_extract(r.y1, r.x1, r.y2, r.x2, &len);

        if (text && cut_store(text, len, false)) {
                set_msg("Copied %d bytes", len);
        }

        free(text);
}

//==============================================================================
/**
 * @brief Paste cut buffer at cursor.
 */
//==============================================================================
static void cmd_paste(void)
{
        if (global->cut_len == 0) {
                set_msg("Cutbuffer is empty");
                return;
        }

        edit_insert(global->cut, global->cut_len, false);
}

//==============================================================================
/**
 * @brief Backspace.
 */
//==============================================================================
static void cmd_backspace(void)
{
        if (global->cx > 0) {
                edit_delete(global->cy, prev_pos(cur_line(), global->cx), global->cy, global->cx);

        } else if (global->cy > 0) {
                int y = global->cy - 1;
                edit_delete(y, global->buf.v[y].len, global->cy, 0);
        }
}

//==============================================================================
/**
 * @brief Delete character at cursor.
 */
//==============================================================================
static void cmd_delete(void)
{
        line_t *line = cur_line();

        if (global->cx < line->len) {
                edit_delete(global->cy, global->cx, global->cy, global->cx + char_len(line, global->cx));

        } else if (global->cy < global->buf.n - 1) {
                edit_delete(global->cy, global->cx, global->cy + 1, 0);
        }
}

//==============================================================================
/**
 * @brief Find text in line (case insensitive).
 *
 * @return position or -1.
 */
//==============================================================================
static int find_in_line(const line_t *line, int from, const char *pattern, int plen)
{
        for (int i = from; i + plen <= line->len; i++) {
                if (strncasecmp(&line->s[i], pattern, plen) == 0) {
                        return i;
                }
        }

        return -1;
}

//==============================================================================
/**
 * @brief Search text.
 */
//==============================================================================
static void cmd_search(void)
{
        char label[PROMPT_LEN + 16];

        if (global->search[0]) {
                snprintf(label, sizeof(label), "Search [%s]: ", global->search);
        } else {
                snprintf(label, sizeof(label), "Search: ");
        }

        if (!prompt(label, NULL)) {
                return;
        }

        if (global->input[0]) {
                snprintf(global->search, sizeof(global->search), "%s", global->input);
        }

        const char *pattern = global->search;
        int plen = strlen(pattern);

        if (plen == 0) {
                set_msg("Cancelled");
                return;
        }

        /* search from the next character, then from the next lines, wrap at the end */
        int n = global->buf.n;

        for (int i = 0; i <= n; i++) {
                int idx = (global->cy + i) % n;
                int from = (i == 0) ? global->cx + 1 : 0;
                int pos = find_in_line(&global->buf.v[idx], from, pattern, plen);

                if ((pos >= 0) && ((i < n) || (pos <= global->cx))) {
                        if ((i == n) && (pos == global->cx)) {
                                set_msg("This is the only occurrence");
                        } else if (global->cy + i >= n) {
                                set_msg("Search Wrapped");
                        }

                        global->cy = idx;
                        global->cx = pos;
                        return;
                }
        }

        set_msg("\"%s\" not found", pattern);
}

//==============================================================================
/**
 * @brief Replace text (case insensitive search, asks for each occurrence).
 */
//==============================================================================
static void cmd_replace(void)
{
        char label[PROMPT_LEN + 32];

        if (global->search[0]) {
                snprintf(label, sizeof(label), "Search (to replace) [%s]: ", global->search);
        } else {
                snprintf(label, sizeof(label), "Search (to replace): ");
        }

        if (!prompt(label, NULL)) {
                return;
        }

        if (global->input[0]) {
                snprintf(global->search, sizeof(global->search), "%s", global->input);
        }

        int plen = strlen(global->search);
        if (plen == 0) {
                set_msg("Cancelled");
                return;
        }

        if (!prompt("Replace with: ", NULL)) {
                return;
        }

        char *with = strdup(global->input);
        if (!with) {
                set_msg("Not enough memory");
                return;
        }

        int wlen = strlen(with);
        int count = 0;
        bool all = false;
        bool wrapped = false;
        int sy = global->cy;
        int sx = global->cx;
        int y = sy;
        int x = sx;

        global->mark = false;

        for (;;) {
                int pos = find_in_line(&global->buf.v[y], x, global->search, plen);

                /* after wrap the search ends at the start position */
                if (wrapped && (y == sy) && ((pos < 0) || (pos >= sx))) {
                        break;
                }

                if (pos < 0) {
                        if (y + 1 < global->buf.n) {
                                y++;
                        } else if (!wrapped) {
                                wrapped = true;
                                y = 0;
                        } else {
                                break;
                        }

                        x = 0;
                        continue;
                }

                global->cy = y;
                global->cx = pos;

                if (!all) {
                        global->hl = true;
                        global->hl_y = y;
                        global->hl_x = pos;
                        global->hl_len = plen;

                        int answer = question_ex("Replace this instance?", true);

                        global->hl = false;

                        if (answer < 0) {
                                break;
                        } else if (answer == 0) {
                                x = pos + char_len(&global->buf.v[y], pos);
                                continue;
                        } else if (answer == 2) {
                                all = true;
                        }
                }

                if (!edit_delete(y, pos, y, pos + plen) || !edit_insert(with, wlen, false)) {
                        break;
                }

                count++;

                /* replaced text before start position on the same line moves start */
                if (wrapped && (y == sy) && (pos < sx)) {
                        sx += wlen - plen;
                }

                x = pos + wlen;
        }

        free(with);

        set_msg("Replaced %d occurrence%s", count, (count == 1) ? "" : "s");
}

//==============================================================================
/**
 * @brief Go to line (and column).
 */
//==============================================================================
static void cmd_goto(void)
{
        if (!prompt("Enter line number, column number: ", NULL)) {
                return;
        }

        char *end;
        long line = strtol(global->input, &end, 10);
        long col = 1;

        if (*end == ',') {
                col = strtol(end + 1, NULL, 10);
        }

        if (line < 1) {
                line = 1;
        }

        if (line > global->buf.n) {
                line = global->buf.n;
        }

        global->cy = line - 1;
        global->cx = pos_of_col(cur_line(), (col > 1) ? col - 1 : 0);
}

//==============================================================================
/**
 * @brief Show cursor position.
 */
//==============================================================================
static void cmd_location(void)
{
        long chars = 0;
        long total = 0;

        for (int i = 0; i < global->buf.n; i++) {
                const line_t *line = &global->buf.v[i];
                long n = 0;

                for (int pos = 0; pos < line->len; pos += char_len(line, pos)) {
                        if ((i == global->cy) && (pos == global->cx)) {
                                chars = total + n;
                        }

                        n++;
                }

                if ((i == global->cy) && (global->cx >= line->len)) {
                        chars = total + n;
                }

                total += n + ((i < global->buf.n - 1) ? 1 : 0);
        }

        const line_t *line = cur_line();
        int col = col_of(line, global->cx) + 1;
        int cols = col_of(line, line->len) + 1;

        set_msg("line %d/%d (%d%%), col %d/%d (%d%%), char %ld/%ld (%d%%)", global->cy + 1, global->buf.n,
                (global->cy + 1) * 100 / global->buf.n, col, cols, col * 100 / cols, chars + 1, total + 1,
                (int)((chars + 1) * 100 / (total + 1)));
}

//==============================================================================
/**
 * @brief Show help screen.
 */
//==============================================================================
static void cmd_help(void)
{
        tui_t *tui = global->tui;
        int rows = tui_rows(tui);

        tui_set_attr(tui, ATTR_TEXT);
        tui_clear(tui);

        tui_set_attr(tui, ATTR_BAR);
        tui_fill(tui, 0, 0, tui_cols(tui), 1, ' ');
        tui_print_at(tui, 2, 0, ATTR_BAR, "Main nano help text");

        for (size_t i = 0; (i < sizeof(HELP_TEXT) / sizeof(HELP_TEXT[0])) && ((int)i + 2 < rows); i++) {
                tui_print_at(tui, 0, 2 + i, ATTR_TEXT, HELP_TEXT[i]);
        }

        tui_set_cursor(tui, -1, -1);
        tui_refresh(tui);

        while (tui_getkey(tui, -1) == TUI_KEY_RESIZE) {
        }

        global->drawn_rowoff = -1;
}

//==============================================================================
/**
 * @brief Move cursor to previous/next word.
 */
//==============================================================================
static void move_word(bool forward)
{
        line_t *line = cur_line();

        if (forward) {
                if (global->cx >= line->len) {
                        if (global->cy < global->buf.n - 1) {
                                global->cy++;
                                global->cx = 0;
                        }
                        return;
                }

                while ((global->cx < line->len) && is_word(line->s[global->cx])) {
                        global->cx++;
                }

                while ((global->cx < line->len) && !is_word(line->s[global->cx])) {
                        global->cx++;
                }

        } else {
                if (global->cx == 0) {
                        if (global->cy > 0) {
                                global->cy--;
                                global->cx = cur_line()->len;
                        }
                        return;
                }

                while ((global->cx > 0) && !is_word(line->s[global->cx - 1])) {
                        global->cx--;
                }

                while ((global->cx > 0) && is_word(line->s[global->cx - 1])) {
                        global->cx--;
                }
        }
}

//==============================================================================
/**
 * @brief Move cursor vertically keeping column.
 */
//==============================================================================
static void move_vertical(int lines)
{
        if (!global->vertical_move) {
                global->want_col = col_of(cur_line(), global->cx);
        }

        int cy = global->cy + lines;
        cy = (cy < 0) ? 0 : (cy >= global->buf.n) ? global->buf.n - 1 : cy;

        global->cy = cy;
        global->cx = pos_of_col(cur_line(), global->want_col);
        global->vertical_move = true;
}

//==============================================================================
/**
 * @brief Handle key.
 */
//==============================================================================
static void handle_key(int key)
{
        int page = tui_rows(global->tui) - 5;
        bool cut = false;
        bool vertical = false;

        if (page < 1) {
                page = 1;
        }

        /* each key is a separate undo action (typed text is merged) */
        global->undo_group++;

        switch (key) {
        case TUI_MOD_ALT | 'u':
        case TUI_MOD_ALT | 'U':
                cmd_undo(false);
                break;

        case TUI_MOD_ALT | 'e':
        case TUI_MOD_ALT | 'E':
                cmd_undo(true);
                break;

        case TUI_MOD_ALT | 'a':
        case TUI_MOD_ALT | 'A':
        case TUI_KEY_CTRL('^'):
                global->mark = !global->mark;
                global->mark_y = global->cy;
                global->mark_x = global->cx;
                set_msg(global->mark ? "Mark Set" : "Mark Unset");
                break;

        case TUI_MOD_ALT | '^':
        case TUI_MOD_ALT | 'c':
        case TUI_MOD_ALT | 'C':
                cmd_copy();
                break;

        case TUI_KEY_CTRL('\\'):
        case TUI_MOD_ALT | 'r':
        case TUI_MOD_ALT | 'R':
                cmd_replace();
                break;

        case TUI_KEY_CTRL('X'):
        case TUI_KEY_F(2):
                cmd_exit();
                break;

        case TUI_KEY_CTRL('O'):
        case TUI_KEY_F(3):
                cmd_write_out();
                break;

        case TUI_KEY_CTRL('S'):
                cmd_save();
                break;

        case TUI_KEY_CTRL('G'):
        case TUI_KEY_F(1):
                cmd_help();
                break;

        case TUI_KEY_CTRL('W'):
        case TUI_KEY_F(6):
                cmd_search();
                break;

        case TUI_KEY_CTRL('K'):
        case TUI_KEY_F(9):
                cut = cmd_cut(global->cut_continue);
                break;

        case TUI_KEY_CTRL('U'):
        case TUI_KEY_F(10):
                cmd_paste();
                break;

        case TUI_KEY_CTRL('C'):
                cmd_location();
                break;

        case TUI_KEY_CTRL('_'):
        case TUI_MOD_ALT | 'g':
        case TUI_MOD_ALT | 'G':
                cmd_goto();
                break;

        case TUI_KEY_CTRL('L'):
                tui_redraw(global->tui);
                global->drawn_rowoff = -1;
                break;

        case TUI_KEY_RESIZE:
                global->drawn_rowoff = -1;
                break;

        case TUI_KEY_UP:
        case TUI_KEY_CTRL('P'):
                move_vertical(-1);
                vertical = true;
                break;

        case TUI_KEY_DOWN:
        case TUI_KEY_CTRL('N'):
                move_vertical(1);
                vertical = true;
                break;

        case TUI_KEY_PGUP:
        case TUI_KEY_CTRL('Y'):
                move_vertical(-page);
                global->rowoff -= page;
                if (global->rowoff < 0) {
                        global->rowoff = 0;
                }
                vertical = true;
                break;

        case TUI_KEY_PGDN:
        case TUI_KEY_CTRL('V'):
                move_vertical(page);
                global->rowoff += page;
                if (global->rowoff > global->cy) {
                        global->rowoff = global->cy;
                }
                vertical = true;
                break;

        case TUI_KEY_LEFT:
        case TUI_KEY_CTRL('B'):
                if (global->cx > 0) {
                        global->cx = prev_pos(cur_line(), global->cx);
                } else if (global->cy > 0) {
                        global->cy--;
                        global->cx = cur_line()->len;
                }
                break;

        case TUI_KEY_RIGHT:
        case TUI_KEY_CTRL('F'):
                if (global->cx < cur_line()->len) {
                        global->cx += char_len(cur_line(), global->cx);
                } else if (global->cy < global->buf.n - 1) {
                        global->cy++;
                        global->cx = 0;
                }
                break;

        case TUI_KEY_CTRL('A'):
        case TUI_KEY_HOME:
                global->cx = 0;
                break;

        case TUI_KEY_CTRL('E'):
        case TUI_KEY_END:
                global->cx = cur_line()->len;
                break;

        case TUI_MOD_CTRL | TUI_KEY_HOME:
        case TUI_MOD_ALT | '\\':
                global->cy = 0;
                global->cx = 0;
                break;

        case TUI_MOD_CTRL | TUI_KEY_END:
        case TUI_MOD_ALT | '/':
                global->cy = global->buf.n - 1;
                global->cx = cur_line()->len;
                break;

        case TUI_MOD_CTRL | TUI_KEY_LEFT:
                move_word(false);
                break;

        case TUI_MOD_CTRL | TUI_KEY_RIGHT:
                move_word(true);
                break;

        case TUI_KEY_ENTER:
                edit_insert("\n", 1, false);
                break;

        case TUI_KEY_BACKSPACE:
                cmd_backspace();
                break;

        case TUI_KEY_DELETE:
        case TUI_KEY_CTRL('D'):
                cmd_delete();
                break;

        case TUI_KEY_TAB:
                edit_insert("\t", 1, true);
                break;

        default:
                if ((key >= 0x20) && (key <= 0x10FFFF)) {
                        char utf8[4];
                        edit_insert(utf8, tui_utf8_encode(key, utf8), true);
                }
                break;
        }

        global->cut_continue = cut;
        global->vertical_move = vertical;
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *file = NULL;
        long start_line = 0;

        for (int i = 1; i < argc; i++) {
                if (argv[i][0] == '+') {
                        start_line = strtol(&argv[i][1], NULL, 10);
                } else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
                        printf("Usage: %s [+LINE] [FILE]\n", argv[0]);
                        return EXIT_SUCCESS;
                } else {
                        file = argv[i];
                }
        }

        int err = 0;

        if (file) {
                err = set_filename(file) ? load_file(file) : ENOMEM;
        } else {
                err = lines_insert(&global->buf, 0, "", 0) ? 0 : ENOMEM;
        }

        if (err) {
                fprintf(stderr, "%s: %s: %s\n", argv[0], file ? file : "", strerror(err));
                lines_clear(&global->buf);
                free(global->buf.v);
                free(global->filename);
                return EXIT_FAILURE;
        }

        if (start_line > 0) {
                global->cy = (start_line > global->buf.n) ? global->buf.n - 1 : start_line - 1;
        }

        global->drawn_rowoff = -1;

        global->tui = tui_init();
        if (!global->tui) {
                perror(argv[0]);
                err = 1;
        }

        while (!err && !global->quit) {
                draw(BAR_EDIT);
                tui_refresh(global->tui);

                int key = tui_getkey(global->tui, -1);
                if (key == TUI_KEY_NONE) {
                        break; /* input error (stream closed) */
                }

                global->msg[0] = '\0';

                handle_key(key);
        }

        tui_deinit(global->tui);

        lines_clear(&global->buf);
        free(global->buf.v);
        free(global->cut);
        undo_free(&global->undo);
        undo_free(&global->redo);
        free(global->filename);

        return err ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
