/*==============================================================================
@file   dialog.c

@author Daniel Zorychta

@brief  Dialog boxes from scripts (subset of dialog(1) program).

        Supported boxes: --msgbox, --yesno, --infobox, --inputbox, --menu,
        --radiolist, --checklist. Exit status and output are compatible with
        dialog(1): 0 OK/Yes, 1 Cancel/No, 2 Help ("HELP <tag>" is printed),
        255 Esc or error.

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
#include <string.h>
#include <ctype.h>
#include <tui.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define EXIT_OK     0
#define EXIT_CANCEL 1
#define EXIT_HELP   2
#define EXIT_ESC    255
#define EXIT_ERROR  255

#define INPUT_LEN      256
#define MAX_BUTTONS    3
#define MIN_TEXT_WIDTH 20
#define SHADOW_W       2
#define SHADOW_H       1
#define ASPECT_RATIO   9  /* auto size: text width to height ratio (as dialog(1)) */
#define ASPECT_MIN_W   60 /* auto size: aspect ratio is used for wider text */

#define ATTR_SCREEN   TUI_ATTR(TUI_BRIGHT(TUI_CYAN), TUI_BLUE)
#define ATTR_DIALOG   TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_BORDER   TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_TITLE    (TUI_ATTR(TUI_BLUE, TUI_WHITE) | TUI_BOLD)
#define ATTR_SHADOW   TUI_ATTR(TUI_BLACK, TUI_BLACK)
#define ATTR_BTN      TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_BTN_KEY  TUI_ATTR(TUI_RED, TUI_WHITE)
#define ATTR_BTN_SEL  (TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_BLUE) | TUI_BOLD)
#define ATTR_BTN_SKEY (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_BLUE) | TUI_BOLD)
#define ATTR_ITEM     TUI_ATTR(TUI_BLACK, TUI_WHITE)
#define ATTR_TAG      (TUI_ATTR(TUI_BLUE, TUI_WHITE) | TUI_BOLD)
#define ATTR_ITEM_SEL (TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_BLUE) | TUI_BOLD)
#define ATTR_INPUT    TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_ARROW    (TUI_ATTR(TUI_GREEN, TUI_WHITE) | TUI_BOLD)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef enum {
        BOX_NONE,
        BOX_MSG,
        BOX_YESNO,
        BOX_INFO,
        BOX_INPUT,
        BOX_MENU,
        BOX_RADIO,
        BOX_CHECK,
} box_t;

typedef struct {
        const char *tag;
        const char *text;
        bool on;
} item_t;

typedef struct {
        const char *label;
        int code;
} button_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        tui_t *tui;

        /* options */
        const char *title;
        const char *backtitle;
        const char *ok_label;
        const char *cancel_label;
        const char *yes_label;
        const char *no_label;
        const char *help_label;
        const char *default_item;
        bool use_stdout;
        bool help_button;
        bool no_tags;
        bool cr_wrap;
        bool no_collapse;
        bool no_nl_expand;
        bool no_shadow;
        bool no_cancel;
        bool default_no;

        /* box definition */
        box_t box;
        const char *text;
        int height;
        int width;
        int list_height;
        item_t *items;
        int nitems;

        /* text prepared for display */
        char *norm;
        char *wrapped;
        const char **lines;
        int nlines;

        /* layout */
        int x;
        int y;
        int w;
        int h;
        int list_y;
        int list_rows;
        int input_y;
        int tag_w;
        int item_w;

        /* state */
        button_t buttons[MAX_BUTTONS];
        int nbuttons;
        int button;
        int sel;
        int top;
        char input[INPUT_LEN];
        int input_pos;
        int input_off;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(dialog, STACK_DEPTH_LARGE);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Print usage.
 */
//==============================================================================
static void usage(const char *name)
{
        fprintf(stderr,
                "Usage: %s [options] <box> <text> <height> <width> [box arguments]\n"
                "Boxes:\n"
                "  --msgbox    <text> <h> <w>\n"
                "  --yesno     <text> <h> <w>\n"
                "  --infobox   <text> <h> <w>\n"
                "  --inputbox  <text> <h> <w> [<init>]\n"
                "  --menu      <text> <h> <w> <menu h> <tag> <item>...\n"
                "  --radiolist <text> <h> <w> <list h> <tag> <item> <on|off>...\n"
                "  --checklist <text> <h> <w> <list h> <tag> <item> <on|off>...\n"
                "Options:\n"
                "  --title <t> --backtitle <t> --default-item <tag>\n"
                "  --ok-label <l> --cancel-label <l> --yes-label <l> --no-label <l>\n"
                "  --help-button --help-label <l> --no-cancel --defaultno --no-tags\n"
                "  --stdout --stderr --cr-wrap --no-collapse --no-nl-expand --no-shadow --clear\n"
                "Size 0 means automatic size, -1 maximum size.\n",
                name);
}

//==============================================================================
/**
 * @brief Parse command line.
 *
 * @return true on success.
 */
//==============================================================================
static bool parse_args(int argc, char *argv[])
{
        for (int i = 1; i < argc; i++) {
                const char *a = argv[i];
                bool has_next = (i + 1 < argc);

#define OPT_STR(name, field)                                                         \
        if (strcmp(a, name) == 0) {                                                  \
                if (!has_next) {                                                     \
                        fprintf(stderr, "%s: missing argument of %s\n", argv[0], a); \
                        return false;                                                \
                }                                                                    \
                global->field = argv[++i];                                           \
                continue;                                                            \
        }

#define OPT_BOOL(name, field, value)   \
        if (strcmp(a, name) == 0) {    \
                global->field = value; \
                continue;              \
        }

                OPT_STR("--title", title);
                OPT_STR("--backtitle", backtitle);
                OPT_STR("--ok-label", ok_label);
                OPT_STR("--cancel-label", cancel_label);
                OPT_STR("--yes-label", yes_label);
                OPT_STR("--no-label", no_label);
                OPT_STR("--help-label", help_label);
                OPT_STR("--default-item", default_item);
                OPT_BOOL("--stdout", use_stdout, true);
                OPT_BOOL("--stderr", use_stdout, false);
                OPT_BOOL("--help-button", help_button, true);
                OPT_BOOL("--no-tags", no_tags, true);
                OPT_BOOL("--cr-wrap", cr_wrap, true);
                OPT_BOOL("--no-collapse", no_collapse, true);
                OPT_BOOL("--no-nl-expand", no_nl_expand, true);
                OPT_BOOL("--no-shadow", no_shadow, true);
                OPT_BOOL("--no-cancel", no_cancel, true);
                OPT_BOOL("--nocancel", no_cancel, true);
                OPT_BOOL("--defaultno", default_no, true);

                /* screen is always cleared */
                if (strcmp(a, "--clear") == 0) {
                        continue;
                }

#undef OPT_STR
#undef OPT_BOOL

                static const struct {
                        const char *name;
                        box_t box;
                } BOXES[] = {
                        {"--msgbox", BOX_MSG},
                        {"--yesno", BOX_YESNO},
                        {"--infobox", BOX_INFO},
                        {"--inputbox", BOX_INPUT},
                        {"--menu", BOX_MENU},
                        {"--radiolist", BOX_RADIO},
                        {"--checklist", BOX_CHECK},
                };

                box_t box = BOX_NONE;
                for (size_t b = 0; b < sizeof(BOXES) / sizeof(BOXES[0]); b++) {
                        if (strcmp(a, BOXES[b].name) == 0) {
                                box = BOXES[b].box;
                        }
                }

                if (box == BOX_NONE) {
                        fprintf(stderr, "%s: unknown option %s\n", argv[0], a);
                        return false;
                }

                if (global->box != BOX_NONE) {
                        fprintf(stderr, "%s: only one box is supported\n", argv[0]);
                        return false;
                }

                bool list = (box == BOX_MENU) || (box == BOX_RADIO) || (box == BOX_CHECK);

                int need = list ? 4 : 3; /* text, height, width [, list height] */
                if (i + need >= argc) {
                        fprintf(stderr, "%s: %s: missing arguments\n", argv[0], a);
                        return false;
                }

                global->box = box;
                global->text = argv[++i];
                global->height = atoi(argv[++i]);
                global->width = atoi(argv[++i]);

                if ((box == BOX_INPUT) && (i + 1 < argc) && (strncmp(argv[i + 1], "--", 2) != 0)) {
                        snprintf(global->input, sizeof(global->input), "%s", argv[++i]);
                }

                if (list) {
                        global->list_height = atoi(argv[++i]);

                        int per_item = (box == BOX_MENU) ? 2 : 3;
                        int first = i + 1;
                        int n = 0;

                        while ((first + (n + 1) * per_item <= argc) &&
                                (strncmp(argv[first + n * per_item], "--", 2) != 0)) {
                                n++;
                        }

                        global->items = calloc(n ? n : 1, sizeof(item_t));
                        if (!global->items) {
                                perror(argv[0]);
                                return false;
                        }

                        for (int k = 0; k < n; k++) {
                                char **it = &argv[first + k * per_item];
                                global->items[k].tag = it[0];
                                global->items[k].text = it[1];
                                global->items[k].on = (per_item == 3) && (strcasecmp(it[2], "on") == 0);
                        }

                        global->nitems = n;
                        i = first + n * per_item - 1;
                }
        }

        if (global->box == BOX_NONE) {
                usage(argv[0]);
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief Normalize text as dialog(1): "\n" sequences are new lines (then real
 *        new lines are spaces unless --cr-wrap), tabs are spaces, multiple
 *        spaces are collapsed (unless --no-collapse).
 *
 * @return true on success.
 */
//==============================================================================
static bool normalize_text(void)
{
        const char *src = global->text;
        size_t len = strlen(src);
        char *dst = malloc(len + 1);
        if (!dst) {
                return false;
        }

        bool escapes = !global->no_nl_expand && strstr(src, "\\n");
        size_t n = 0;

        for (size_t i = 0; i < len; i++) {
                char c = src[i];

                if (escapes && (c == '\\') && (src[i + 1] == 'n')) {
                        c = '\n';
                        i++;
                } else if ((c == '\n') && escapes && !global->cr_wrap) {
                        c = ' ';
                } else if (c == '\t') {
                        c = ' ';
                } else if (c == '\r') {
                        continue;
                }

                if ((c == ' ') && !global->no_collapse && (n > 0) && (dst[n - 1] == ' ')) {
                        continue;
                }

                /* trailing spaces of line are removed */
                if ((c == '\n') && !global->no_collapse) {
                        while ((n > 0) && (dst[n - 1] == ' ')) {
                                n--;
                        }
                }

                dst[n++] = c;
        }

        while ((n > 0) && (dst[n - 1] == ' ')) {
                n--;
        }

        dst[n] = '\0';

        free(global->norm);
        global->norm = dst;

        return true;
}

//==============================================================================
/**
 * @brief Width of the longest paragraph of normalized text.
 */
//==============================================================================
static int text_natural_width(void)
{
        int max = 0;
        int cur = 0;

        for (const char *p = global->norm; *p;) {
                if (*p == '\n') {
                        p++;
                        cur = 0;
                        continue;
                }

                if (tui_utf8_decode(&p)) {
                        cur++;
                }

                if (cur > max) {
                        max = cur;
                }
        }

        return max;
}

//==============================================================================
/**
 * @brief Wrap normalized text to lines of selected width.
 *
 * @return true on success.
 */
//==============================================================================
static bool wrap_text(int width)
{
        size_t len = strlen(global->norm);

        /* each character can start a new line in the worst case */
        char *out = malloc(len * 2 + 2);
        if (!out) {
                return false;
        }

        size_t n = 0;
        int col = 0;
        const char *p = global->norm;

        while (*p) {
                if (*p == '\n') {
                        out[n++] = '\n';
                        col = 0;
                        p++;
                        continue;
                }

                /* measure next word */
                const char *w = p;
                int ww = 0;
                while (*w && (*w != ' ') && (*w != '\n')) {
                        tui_utf8_decode(&w);
                        ww++;
                }

                if (*p == ' ') {
                        if (col > 0 && col < width) {
                                out[n++] = ' ';
                                col++;
                        }
                        p++;
                        continue;
                }

                if ((col > 0) && (col + ww > width)) {
                        /* remove space before line break */
                        if ((n > 0) && (out[n - 1] == ' ')) {
                                n--;
                        }

                        out[n++] = '\n';
                        col = 0;
                }

                /* copy word, split words longer than line */
                while (p < w) {
                        if (col >= width) {
                                out[n++] = '\n';
                                col = 0;
                        }

                        const char *c = p;
                        tui_utf8_decode(&p);
                        memcpy(&out[n], c, p - c);
                        n += p - c;
                        col++;
                }
        }

        out[n] = '\0';

        /* split to lines */
        int nlines = 1;
        for (size_t i = 0; i < n; i++) {
                nlines += (out[i] == '\n');
        }

        const char **lines = malloc(nlines * sizeof(char *));
        if (!lines) {
                free(out);
                return false;
        }

        lines[0] = out;
        for (size_t i = 0, k = 1; i < n; i++) {
                if (out[i] == '\n') {
                        out[i] = '\0';
                        lines[k++] = &out[i + 1];
                }
        }

        /* empty text has no lines */
        if ((nlines == 1) && (out[0] == '\0')) {
                nlines = 0;
        }

        free(global->wrapped);
        free(global->lines);
        global->wrapped = out;
        global->lines = lines;
        global->nlines = nlines;

        return true;
}

//==============================================================================
/**
 * @brief Check if box has list.
 */
//==============================================================================
static bool has_list(void)
{
        return (global->box == BOX_MENU) || (global->box == BOX_RADIO) || (global->box == BOX_CHECK);
}

//==============================================================================
/**
 * @brief Width of list prefix ("(*) " or "[X] ").
 */
//==============================================================================
static int list_prefix_w(void)
{
        return (global->box == BOX_MENU) ? 0 : 4;
}

//==============================================================================
/**
 * @brief Width of buttons row.
 */
//==============================================================================
static int buttons_width(void)
{
        int w = 0;

        for (int i = 0; i < global->nbuttons; i++) {
                w += tui_strwidth(global->buttons[i].label) + 4 + ((i > 0) ? 3 : 0);
        }

        return w;
}

//==============================================================================
/**
 * @brief Select text width for automatic size. Long text is wrapped to keep
 *        width to height ratio (as dialog(1) does).
 *
 * @param max           maximum width
 *
 * @return Text width or -1 on error.
 */
//==============================================================================
static int auto_text_width(int max)
{
        int w = text_natural_width();
        w = (w > max) ? max : w;

        for (int tw = w; tw > ASPECT_MIN_W; tw--) {
                if (!wrap_text(tw)) {
                        return -1;
                }

                if (tw <= ASPECT_RATIO * global->nlines) {
                        return tw;
                }
        }

        return (w > ASPECT_MIN_W) ? ASPECT_MIN_W : w;
}

//==============================================================================
/**
 * @brief Calculate box size and position, wrap text.
 *
 * @return true on success.
 */
//==============================================================================
static bool layout(void)
{
        int sw = tui_cols(global->tui);
        int sh = tui_rows(global->tui);
        int top = global->backtitle ? 1 : 0;
        int max_w = sw - (global->no_shadow ? 0 : SHADOW_W);
        int max_h = sh - top - (global->no_shadow ? 0 : SHADOW_H);

        /* list columns */
        global->tag_w = 0;
        global->item_w = 0;

        for (int i = 0; i < global->nitems; i++) {
                int tw = tui_strwidth(global->items[i].tag);
                int iw = tui_strwidth(global->items[i].text);
                global->tag_w = (tw > global->tag_w) ? tw : global->tag_w;
                global->item_w = (iw > global->item_w) ? iw : global->item_w;
        }

        if (global->no_tags) {
                global->tag_w = 0;
        }

        int list_w = 0;
        if (has_list()) {
                list_w = list_prefix_w() + global->tag_w + (global->tag_w ? 2 : 0) + global->item_w + 2;
        }

        /* width */
        int w;
        if (global->width > 0) {
                w = global->width;
        } else if (global->width < 0) {
                w = max_w;
        } else {
                int content = auto_text_width(max_w - 4);
                if (content < 0) {
                        return false;
                }

                content = (list_w + 2 > content) ? list_w + 2 : content;
                content = (buttons_width() > content) ? buttons_width() : content;

                int tw = global->title ? tui_strwidth(global->title) + 4 : 0;
                content = (tw > content) ? tw : content;
                content = (content < MIN_TEXT_WIDTH) ? MIN_TEXT_WIDTH : content;

                w = content + 4;
        }

        w = (w > max_w) ? max_w : w;
        w = (w < 8) ? 8 : w;

        if (!wrap_text(w - 4)) {
                return false;
        }

        /* height */
        int extra = 0;
        int list_rows = 0;

        if (has_list()) {
                list_rows = (global->list_height > 0) ? global->list_height : global->nitems;
                list_rows = (list_rows > global->nitems) ? global->nitems : list_rows;
                list_rows = (list_rows < 1) ? 1 : list_rows;
                extra = list_rows + 2;
        } else if (global->box == BOX_INPUT) {
                extra = 3;
        }

        int buttons_rows = (global->nbuttons > 0) ? 2 : 0;
        int needed = 2 + global->nlines + extra + buttons_rows;
        int h;

        if (global->height > 0) {
                h = global->height;
        } else if (global->height < 0) {
                h = max_h;
        } else {
                h = needed;
        }

        h = (h > max_h) ? max_h : h;

        /* not enough space: reduce list, then text */
        int over = needed - h;
        if ((over > 0) && has_list()) {
                int reduce = (list_rows - 1 < over) ? list_rows - 1 : over;
                list_rows -= reduce;
                over -= reduce;
        }

        if ((over > 0) && (global->nlines > 0)) {
                global->nlines = (global->nlines > over) ? global->nlines - over : 0;
        }

        /* list takes rest of space if height is selected */
        if (has_list() && (over < 0) && (global->list_height <= 0)) {
                int more = -over;
                list_rows = (list_rows + more > global->nitems) ? global->nitems : list_rows + more;
        }

        h = (h < 3) ? 3 : h;

        global->w = w;
        global->h = h;
        global->x = (max_w - w) / 2;
        global->y = top + (max_h - h) / 2;
        global->list_rows = list_rows;
        global->list_y = global->y + 1 + global->nlines;
        global->input_y = global->y + 1 + global->nlines;

        return true;
}

//==============================================================================
/**
 * @brief Print string clipped to selected number of columns.
 *
 * @return Number of printed columns.
 */
//==============================================================================
static int print_clip(const char *str, int max)
{
        int n = 0;
        uint32_t ch;

        while ((n < max) && (ch = tui_utf8_decode(&str))) {
                tui_putc(global->tui, ch);
                n++;
        }

        return n;
}

//==============================================================================
/**
 * @brief Draw buttons row.
 */
//==============================================================================
static void draw_buttons(void)
{
        tui_t *tui = global->tui;
        int bw = buttons_width();
        int bx = global->x + (global->w - bw) / 2;
        int by = global->y + global->h - 2;

        tui_set_attr(tui, ATTR_BORDER);
        tui_hline(tui, global->x + 1, by - 1, global->w - 2, TUI_BOX_SINGLE);

        for (int i = 0; i < global->nbuttons; i++) {
                bool sel = (i == global->button);
                const char *label = global->buttons[i].label;

                tui_move(tui, bx, by);
                tui_set_attr(tui, sel ? ATTR_BTN_SEL : ATTR_BTN);
                tui_puts(tui, "< ");

                const char *p = label;
                uint32_t first = tui_utf8_decode(&p);

                tui_set_attr(tui, sel ? ATTR_BTN_SKEY : ATTR_BTN_KEY);
                if (first) {
                        tui_putc(tui, first);
                }

                tui_set_attr(tui, sel ? ATTR_BTN_SEL : ATTR_BTN);
                tui_puts(tui, p);
                tui_puts(tui, " >");

                bx += tui_strwidth(label) + 4 + 3;
        }
}

//==============================================================================
/**
 * @brief Draw list.
 */
//==============================================================================
static void draw_list(void)
{
        tui_t *tui = global->tui;
        int lx = global->x + 2;
        int lw = global->w - 4;
        int ly = global->list_y;

        tui_set_attr(tui, ATTR_BORDER);
        tui_box(tui, lx, ly, lw, global->list_rows + 2, TUI_BOX_SINGLE);

        /* keep columns inside list */
        int inner = lw - 2;
        int tag_w = global->tag_w;
        if (list_prefix_w() + tag_w + 2 > inner) {
                tag_w = inner - list_prefix_w() - 2;
                tag_w = (tag_w < 0) ? 0 : tag_w;
        }

        for (int r = 0; r < global->list_rows; r++) {
                int idx = global->top + r;
                int y = ly + 1 + r;

                tui_set_attr(tui, ATTR_ITEM);
                tui_fill(tui, lx + 1, y, inner, 1, ' ');

                if (idx >= global->nitems) {
                        continue;
                }

                const item_t *it = &global->items[idx];
                bool sel = (idx == global->sel);

                /* tags and items are centered in list (as dialog(1) does) */
                int block = list_prefix_w() + (tag_w ? tag_w + 2 : 0) + global->item_w;
                int offset = (inner > block) ? (inner - block) / 2 : 0;

                tui_move(tui, lx + 1 + offset, y);
                int col = offset;

                if (global->box == BOX_RADIO) {
                        tui_set_attr(tui, ATTR_ITEM);
                        col += tui_puts(tui, it->on ? "(*) " : "( ) ");
                } else if (global->box == BOX_CHECK) {
                        tui_set_attr(tui, ATTR_ITEM);
                        col += tui_puts(tui, it->on ? "[X] " : "[ ] ");
                }

                if (tag_w > 0) {
                        tui_set_attr(tui, sel ? ATTR_ITEM_SEL : ATTR_TAG);
                        int n = print_clip(it->tag, tag_w);
                        for (; n < tag_w + 2; n++) {
                                tui_putc(tui, ' ');
                        }
                        col += tag_w + 2;
                }

                tui_set_attr(tui, sel ? ATTR_ITEM_SEL : ATTR_ITEM);
                print_clip(it->text, inner - col);
        }

        /* scroll indicators */
        tui_set_attr(tui, ATTR_ARROW);

        if (global->top > 0) {
                tui_print_at(tui, lx + lw - 6, ly, ATTR_ARROW, "(-)");
        }

        if (global->top + global->list_rows < global->nitems) {
                tui_print_at(tui, lx + lw - 6, ly + global->list_rows + 1, ATTR_ARROW, "(+)");
        }
}

//==============================================================================
/**
 * @brief Draw input field.
 */
//==============================================================================
static void draw_input(void)
{
        tui_t *tui = global->tui;
        int fx = global->x + 2;
        int fw = global->w - 4;
        int fy = global->input_y;
        int vis = fw - 2;

        tui_set_attr(tui, ATTR_BORDER);
        tui_box(tui, fx, fy, fw, 3, TUI_BOX_SINGLE);

        /* cursor column (in characters) */
        int cur = 0;
        for (const char *p = global->input; p < &global->input[global->input_pos] && tui_utf8_decode(&p);) {
                cur++;
        }

        if (cur < global->input_off) {
                global->input_off = cur;
        } else if (cur >= global->input_off + vis) {
                global->input_off = cur - vis + 1;
        }

        const char *p = global->input;
        for (int i = 0; i < global->input_off && tui_utf8_decode(&p); i++) {
        }

        tui_set_attr(tui, ATTR_INPUT);
        tui_fill(tui, fx + 1, fy + 1, vis, 1, ' ');
        tui_move(tui, fx + 1, fy + 1);
        print_clip(p, vis);

        tui_set_cursor(tui, fx + 1 + cur - global->input_off, fy + 1);
}

//==============================================================================
/**
 * @brief Draw screen.
 */
//==============================================================================
static void draw(void)
{
        tui_t *tui = global->tui;

        tui_set_cursor(tui, -1, -1);

        tui_set_attr(tui, ATTR_SCREEN);
        tui_clear(tui);

        if (global->backtitle) {
                tui_move(tui, 1, 0);
                print_clip(global->backtitle, tui_cols(tui) - 2);
        }

        if (!global->no_shadow) {
                tui_set_attr(tui, ATTR_SHADOW);
                tui_fill(tui, global->x + SHADOW_W, global->y + SHADOW_H, global->w, global->h, ' ');
        }

        tui_set_attr(tui, ATTR_DIALOG);
        tui_fill(tui, global->x, global->y, global->w, global->h, ' ');

        tui_set_attr(tui, ATTR_BORDER);
        tui_box(tui, global->x, global->y, global->w, global->h, TUI_BOX_SINGLE);

        if (global->title) {
                int tw = tui_strwidth(global->title) + 2;
                tw = (tw > global->w - 2) ? global->w - 2 : tw;
                tui_set_attr(tui, ATTR_TITLE);
                tui_move(tui, global->x + (global->w - tw) / 2, global->y);
                tui_putc(tui, ' ');
                print_clip(global->title, tw - 2);
                tui_putc(tui, ' ');
        }

        tui_set_attr(tui, ATTR_DIALOG);
        for (int i = 0; i < global->nlines; i++) {
                tui_move(tui, global->x + 2, global->y + 1 + i);
                print_clip(global->lines[i], global->w - 4);
        }

        if (has_list()) {
                draw_list();
        } else if (global->box == BOX_INPUT) {
                draw_input();
        }

        if (global->nbuttons) {
                draw_buttons();
        }
}

//==============================================================================
/**
 * @brief Add button.
 */
//==============================================================================
static void add_button(const char *label, int code)
{
        if (global->nbuttons < MAX_BUTTONS) {
                global->buttons[global->nbuttons].label = label;
                global->buttons[global->nbuttons].code = code;
                global->nbuttons++;
        }
}

//==============================================================================
/**
 * @brief Prepare buttons and initial state.
 */
//==============================================================================
static void setup(void)
{
        const char *ok = global->ok_label ? global->ok_label : "OK";
        const char *cancel = global->cancel_label ? global->cancel_label : "Cancel";

        switch (global->box) {
        case BOX_MSG:
                add_button(ok, EXIT_OK);
                break;

        case BOX_YESNO:
                add_button(global->yes_label ? global->yes_label : "Yes", EXIT_OK);
                add_button(global->no_label ? global->no_label : "No", EXIT_CANCEL);
                global->button = global->default_no ? 1 : 0;
                break;

        case BOX_INFO:
                break;

        default:
                add_button(ok, EXIT_OK);

                if (!global->no_cancel) {
                        add_button(cancel, EXIT_CANCEL);
                }

                if (global->help_button && has_list()) {
                        add_button(global->help_label ? global->help_label : "Help", EXIT_HELP);
                }
                break;
        }

        global->input_pos = strlen(global->input);

        if (global->default_item) {
                for (int i = 0; i < global->nitems; i++) {
                        if (strcmp(global->items[i].tag, global->default_item) == 0) {
                                global->sel = i;
                                break;
                        }
                }
        }

        /* radiolist: only one item can be selected */
        if (global->box == BOX_RADIO) {
                bool found = false;
                for (int i = 0; i < global->nitems; i++) {
                        global->items[i].on = global->items[i].on && !found;
                        found = found || global->items[i].on;
                }
        }
}

//==============================================================================
/**
 * @brief Move list selection and keep it visible.
 */
//==============================================================================
static void select_item(int sel)
{
        if (global->nitems == 0) {
                return;
        }

        sel = (sel < 0) ? 0 : (sel >= global->nitems) ? global->nitems - 1 : sel;
        global->sel = sel;
}

//==============================================================================
/**
 * @brief Keep selected item visible.
 */
//==============================================================================
static void scroll_list(void)
{
        if (global->sel < global->top) {
                global->top = global->sel;
        } else if (global->sel >= global->top + global->list_rows) {
                global->top = global->sel - global->list_rows + 1;
        }

        int max_top = global->nitems - global->list_rows;
        global->top = (global->top > max_top) ? max_top : global->top;
        global->top = (global->top < 0) ? 0 : global->top;
}

//==============================================================================
/**
 * @brief Find item by first character (hot key), starting after selection.
 */
//==============================================================================
static void hotkey_item(int key)
{
        for (int k = 1; k <= global->nitems; k++) {
                int idx = (global->sel + k) % global->nitems;
                const char *s = global->no_tags ? global->items[idx].text : global->items[idx].tag;
                uint32_t first = tui_utf8_decode(&s);

                if ((first == (uint32_t)key) ||
                        ((first < 0x80) && (key < 0x80) && (tolower((int)first) == tolower(key)))) {
                        global->sel = idx;
                        return;
                }
        }
}

//==============================================================================
/**
 * @brief Handle keys of list.
 *
 * @return true if key was handled.
 */
//==============================================================================
static bool list_key(int key)
{
        int page = (global->list_rows > 1) ? global->list_rows - 1 : 1;

        switch (key) {
        case TUI_KEY_UP:
                select_item(global->sel - 1);
                return true;
        case TUI_KEY_DOWN:
                select_item(global->sel + 1);
                return true;
        case TUI_KEY_PGUP:
                select_item(global->sel - page);
                return true;
        case TUI_KEY_PGDN:
                select_item(global->sel + page);
                return true;
        case TUI_KEY_HOME:
                select_item(0);
                return true;
        case TUI_KEY_END:
                select_item(global->nitems - 1);
                return true;

        case ' ':
                if ((global->box == BOX_RADIO) && global->nitems) {
                        for (int i = 0; i < global->nitems; i++) {
                                global->items[i].on = (i == global->sel);
                        }
                        return true;
                } else if ((global->box == BOX_CHECK) && global->nitems) {
                        global->items[global->sel].on = !global->items[global->sel].on;
                        return true;
                }
                return false;

        default:
                if ((key > ' ') && (key <= 0x10FFFF) && global->nitems) {
                        hotkey_item(key);
                        return true;
                }
                return false;
        }
}

//==============================================================================
/**
 * @brief Handle keys of input field.
 *
 * @return true if key was handled.
 */
//==============================================================================
static bool input_key(int key)
{
        char *in = global->input;
        int len = strlen(in);
        int pos = global->input_pos;

        switch (key) {
        case TUI_KEY_LEFT:
                while ((pos > 0) && ((in[--pos] & 0xC0) == 0x80)) {
                }
                break;

        case TUI_KEY_RIGHT:
                while ((pos < len) && ((in[++pos] & 0xC0) == 0x80)) {
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
                        while ((start > 0) && ((in[--start] & 0xC0) == 0x80)) {
                        }
                        memmove(&in[start], &in[pos], len - pos + 1);
                        pos = start;
                }
                break;

        case TUI_KEY_DELETE:
                if (pos < len) {
                        int end = pos;
                        while ((end < len) && ((in[++end] & 0xC0) == 0x80)) {
                        }
                        memmove(&in[pos], &in[end], len - end + 1);
                }
                break;

        case TUI_KEY_CTRL('U'):
                in[0] = '\0';
                pos = 0;
                break;

        default:
                if ((key >= 0x20) && (key <= 0x10FFFF) && (key != TUI_KEY_BACKSPACE)) {
                        char utf8[4];
                        int n = tui_utf8_encode(key, utf8);
                        if (len + n < (int)sizeof(global->input)) {
                                memmove(&in[pos + n], &in[pos], len - pos + 1);
                                memcpy(&in[pos], utf8, n);
                                pos += n;
                        }
                        break;
                }
                return false;
        }

        global->input_pos = pos;
        return true;
}

//==============================================================================
/**
 * @brief Find button by hot key (first letter of label).
 *
 * @return Button index or -1.
 */
//==============================================================================
static int hotkey_button(int key)
{
        if ((key < 0x20) || (key > 0x7E)) {
                return -1;
        }

        for (int i = 0; i < global->nbuttons; i++) {
                if (tolower((uint8_t)global->buttons[i].label[0]) == tolower(key)) {
                        return i;
                }
        }

        return -1;
}

//==============================================================================
/**
 * @brief Run box.
 *
 * @return Exit code.
 */
//==============================================================================
static int run(void)
{
        if (!normalize_text()) {
                return EXIT_ERROR;
        }

        for (;;) {
                if (!layout()) {
                        return EXIT_ERROR;
                }

                if (has_list()) {
                        scroll_list();
                }

                draw();
                tui_refresh(global->tui);

                if (global->box == BOX_INFO) {
                        return EXIT_OK;
                }

                int key = tui_getkey(global->tui, -1);

                if (key == TUI_KEY_NONE) {
                        return EXIT_ERROR;
                } else if (key == TUI_KEY_ESC) {
                        return EXIT_ESC;
                } else if (key == TUI_KEY_CTRL('L')) {
                        tui_redraw(global->tui);
                        continue;
                } else if (key == TUI_KEY_RESIZE) {
                        continue;
                } else if (key == TUI_KEY_ENTER) {
                        return global->buttons[global->button].code;
                } else if ((key == TUI_KEY_TAB) || ((key == TUI_KEY_RIGHT) && (global->box != BOX_INPUT))) {
                        global->button = (global->button + 1) % global->nbuttons;
                        continue;
                } else if ((key == TUI_KEY_SHIFT_TAB) ||
                        ((key == TUI_KEY_LEFT) && (global->box != BOX_INPUT))) {
                        global->button = (global->button + global->nbuttons - 1) % global->nbuttons;
                        continue;
                }

                if (has_list() && list_key(key)) {
                        continue;
                }

                if ((global->box == BOX_INPUT) && input_key(key)) {
                        continue;
                }

                /* buttons hot keys in boxes without list and input */
                if ((global->box == BOX_MSG) || (global->box == BOX_YESNO)) {
                        int b = hotkey_button(key);
                        if (b >= 0) {
                                return global->buttons[b].code;
                        }
                }
        }
}

//==============================================================================
/**
 * @brief Print tag (quoted if contains spaces) for checklist output.
 */
//==============================================================================
static void print_tag(FILE *out, const char *tag, bool first)
{
        if (!first) {
                fputc(' ', out);
        }

        if (strchr(tag, ' ')) {
                fprintf(out, "\"%s\"", tag);
        } else {
                fputs(tag, out);
        }
}

//==============================================================================
/**
 * @brief Print box result.
 */
//==============================================================================
static void print_result(int code)
{
        FILE *out = global->use_stdout ? stdout : stderr;
        const char *cur = (global->nitems > 0) ? global->items[global->sel].tag : "";

        if (code == EXIT_HELP) {
                fprintf(out, "HELP %s", cur);

        } else if (code == EXIT_OK) {
                switch (global->box) {
                case BOX_INPUT:
                        fputs(global->input, out);
                        break;

                case BOX_MENU:
                        fputs(cur, out);
                        break;

                case BOX_RADIO:
                        for (int i = 0; i < global->nitems; i++) {
                                if (global->items[i].on) {
                                        fputs(global->items[i].tag, out);
                                        break;
                                }
                        }
                        break;

                case BOX_CHECK: {
                        bool first = true;
                        for (int i = 0; i < global->nitems; i++) {
                                if (global->items[i].on) {
                                        print_tag(out, global->items[i].tag, first);
                                        first = false;
                                }
                        }
                        break;
                }

                default:
                        break;
                }
        }

        fflush(out);
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int code = EXIT_ERROR;

        if (parse_args(argc, argv)) {
                setup();

                /* with --stdout the result goes to stdout, so screen is drawn on stderr */
                global->tui = tui_init_stream(stdin, global->use_stdout ? stderr : stdout);
                if (global->tui) {
                        /* Esc + digit is not used, Esc works immediately */
                        tui_set_esc_prefix_timeout(global->tui, 0);

                        code = run();
                        tui_deinit(global->tui);
                        print_result(code);
                } else {
                        perror(argv[0]);
                }
        }

        free(global->items);
        free(global->norm);
        free(global->wrapped);
        free(global->lines);

        return code;
}

/*==============================================================================
  End of file
==============================================================================*/
