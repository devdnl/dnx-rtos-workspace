/*==============================================================================
@file   mc.c

@author Daniel Zorychta

@brief  Simple two-panel file manager in Midnight Commander style.

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
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <tui.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define PATH_LEN       256
#define CMD_LEN        256
#define COPY_BUF_SIZE  4096
#define MAX_DEPTH      32
#define PROGRESS_MS    250
#define VIEW_LINE_BUF  1024
#define VIEW_MAX_LINES 65536
#define TAB_WIDTH      8
#define SIZE_COL_W     9
#define SHELL          "dsh -e"
#define EDITOR         "nano"

#define ATTR_PANEL       TUI_ATTR(TUI_WHITE, TUI_BLUE)
#define ATTR_FRAME       TUI_ATTR(TUI_WHITE, TUI_BLUE)
#define ATTR_HEADER      (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_BLUE) | TUI_BOLD)
#define ATTR_PATH_ACTIVE TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_DIR         (TUI_ATTR(TUI_BRIGHT(TUI_WHITE), TUI_BLUE) | TUI_BOLD)
#define ATTR_EXEC        (TUI_ATTR(TUI_BRIGHT(TUI_GREEN), TUI_BLUE) | TUI_BOLD)
#define ATTR_DEVICE      (TUI_ATTR(TUI_BRIGHT(TUI_MAGENTA), TUI_BLUE) | TUI_BOLD)
#define ATTR_MARKED      (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_BLUE) | TUI_BOLD)
#define ATTR_CURSOR      TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_CURSOR_MARK (TUI_ATTR(TUI_BRIGHT(TUI_YELLOW), TUI_CYAN) | TUI_BOLD)
#define ATTR_CMDLINE     TUI_ATTR_DEFAULT
#define ATTR_KEY_NUM     TUI_ATTR(TUI_WHITE, TUI_BLACK)
#define ATTR_KEY_LABEL   TUI_ATTR(TUI_BLACK, TUI_CYAN)
#define ATTR_VIEW        TUI_ATTR(TUI_WHITE, TUI_BLUE)
#define ATTR_VIEW_BAR    TUI_ATTR(TUI_BLACK, TUI_CYAN)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        char *name;
        uint64_t size;
        mode_t mode;
        bool marked;
} entry_t;

typedef struct {
        char path[PATH_LEN];
        entry_t *e;
        int n;
        int cur;
        int top;
} panel_t;

typedef enum {
        OVERWRITE_ASK,
        OVERWRITE_ALL,
        OVERWRITE_NONE,
} overwrite_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void draw(void);
static void set_msg_error(const char *what, const char *path);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        tui_t *tui;
        panel_t panel[2];
        int active;
        char cmd[CMD_LEN];
        int cmd_pos;
        char input[PATH_LEN];
        char text[PATH_LEN * 2 + 64];
        bool quit;

        /* file operation state */
        char *copybuf;
        bool abort;
        overwrite_t overwrite;
        uint32_t progress_time;
        uint64_t progress_bytes;
        int progress_files;
        const char *progress_title;
};

static const char *const FKEY_LABEL[10] = {
        "Help",
        "",
        "View",
        "Edit",
        "Copy",
        "RenMov",
        "Mkdir",
        "Delete",
        "",
        "Quit",
};

static const char *const HELP_TEXT = "Tab          switch panel\n"
                                     "Enter        enter directory / run program / view file\n"
                                     "Insert, ^T   mark file;  + mark all, - unmark all, * invert\n"
                                     "F3 View      F4 Edit      F5 Copy      F6 Rename/Move\n"
                                     "F7 Mkdir     F8 Delete    F10 Quit\n"
                                     "^R reread    ^U swap panels    ^L redraw\n"
                                     "Alt+Enter    put file name to command line\n"
                                     "Typed text is a command executed in the panel directory.\n"
                                     "Esc+digit works as F key.";

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(mc, STACK_DEPTH_VERY_LARGE);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Active panel.
 */
//==============================================================================
static panel_t *active_panel(void)
{
        return &global->panel[global->active];
}

//==============================================================================
/**
 * @brief Other (not active) panel.
 */
//==============================================================================
static panel_t *other_panel(void)
{
        return &global->panel[!global->active];
}

//==============================================================================
/**
 * @brief Join directory and name.
 *
 * @return true if path fits in buffer.
 */
//==============================================================================
static bool path_join(char *dst, size_t size, const char *dir, const char *name)
{
        int n;

        if (name[0] == '/') {
                n = snprintf(dst, size, "%s", name);
        } else if (strcmp(dir, "/") == 0) {
                n = snprintf(dst, size, "/%s", name);
        } else {
                n = snprintf(dst, size, "%s/%s", dir, name);
        }

        return (n >= 0) && ((size_t)n < size);
}

//==============================================================================
/**
 * @brief Allocate joined path.
 *
 * @return Allocated path or NULL.
 */
//==============================================================================
static char *path_new(const char *dir, const char *name)
{
        char *path = malloc(PATH_LEN);
        if (path && !path_join(path, PATH_LEN, dir, name)) {
                free(path);
                errno = ENAMETOOLONG;
                return NULL;
        }

        return path;
}

//==============================================================================
/**
 * @brief Remove last path component. Returns removed name (pointer into
 *        buffer copy) in name buffer.
 */
//==============================================================================
static void path_parent(char *path, char *name, size_t size)
{
        char *slash = strrchr(path, '/');

        if (name) {
                snprintf(name, size, "%s", slash ? slash + 1 : path);
        }

        if (!slash || (slash == path)) {
                strcpy(path, "/");
        } else {
                *slash = '\0';
        }
}

//==============================================================================
/**
 * @brief Read file mode and size.
 *
 * @return true on success.
 */
//==============================================================================
static bool file_info(const char *path, mode_t *mode, uint64_t *size)
{
        struct stat st;

        if (stat(path, &st) != 0) {
                return false;
        }

        if (mode) {
                *mode = st.st_mode;
        }

        if (size) {
                *size = st.st_size;
        }

        return true;
}

//==============================================================================
/**
 * @brief Check if path is a directory.
 */
//==============================================================================
static bool is_dir(const char *path)
{
        mode_t mode;
        return file_info(path, &mode, NULL) && S_ISDIR(mode);
}

//==============================================================================
/**
 * @brief Check if entry is executable (program or script).
 */
//==============================================================================
static bool is_exec(const entry_t *e)
{
        return S_ISPROG(e->mode) || (S_ISREG(e->mode) && (e->mode & S_IXUSR));
}

//==============================================================================
/**
 * @brief Compare entries: ".." first, then directories, then files by name.
 */
//==============================================================================
static int entry_cmp(const void *a, const void *b)
{
        const entry_t *ea = a;
        const entry_t *eb = b;

        bool up_a = (strcmp(ea->name, "..") == 0);
        bool up_b = (strcmp(eb->name, "..") == 0);

        if (up_a || up_b) {
                return up_b - up_a;
        }

        bool dir_a = S_ISDIR(ea->mode);
        bool dir_b = S_ISDIR(eb->mode);

        if (dir_a != dir_b) {
                return dir_b - dir_a;
        }

        return strcmp(ea->name, eb->name);
}

//==============================================================================
/**
 * @brief Release panel entries.
 */
//==============================================================================
static void panel_free(panel_t *p)
{
        for (int i = 0; i < p->n; i++) {
                free(p->e[i].name);
        }

        free(p->e);
        p->e = NULL;
        p->n = 0;
}

//==============================================================================
/**
 * @brief Add entry to panel.
 *
 * @return true on success.
 */
//==============================================================================
static bool panel_add(panel_t *p, int *cap, const char *name, mode_t mode, uint64_t size)
{
        if (p->n == *cap) {
                int ncap = (*cap) ? (*cap) * 2 : 32;
                entry_t *e = realloc(p->e, ncap * sizeof(entry_t));
                if (!e) {
                        return false;
                }

                p->e = e;
                *cap = ncap;
        }

        char *copy = strdup(name);
        if (!copy) {
                return false;
        }

        p->e[p->n].name = copy;
        p->e[p->n].mode = mode;
        p->e[p->n].size = size;
        p->e[p->n].marked = false;
        p->n++;

        return true;
}

//==============================================================================
/**
 * @brief Read panel directory. Cursor is set on selected name (if exists).
 *
 * @param p             panel
 * @param select        name to select (can be NULL: keep position)
 *
 * @return true on success.
 */
//==============================================================================
static bool panel_load(panel_t *p, const char *select)
{
        char *keep = NULL;
        if (!select && (p->n > 0) && (p->cur < p->n)) {
                keep = strdup(p->e[p->cur].name);
                select = keep;
        }

        int old_cur = p->cur;
        panel_free(p);

        DIR *dir = opendir(p->path);
        if (!dir) {
                set_msg_error("Cannot open directory", p->path);
        }

        int cap = 0;
        bool ok = true;

        if (strcmp(p->path, "/") != 0) {
                ok = panel_add(p, &cap, "..", S_IFDIR, 0);
        }

        struct dirent *ent;
        while (ok && dir && (ent = readdir(dir))) {
                if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) {
                        continue;
                }

                mode_t mode = 0;
                uint64_t size = 0;

#if defined(TUI_PORT_POSIX)
                /* POSIX: directory entry has no mode and size */
                char *path = path_new(p->path, ent->d_name);
                if (path) {
                        file_info(path, &mode, &size);
                        free(path);
                }
#else
                mode = ent->mode;
                size = ent->size;
#endif

                ok = panel_add(p, &cap, ent->d_name, mode, size);
        }

        if (dir) {
                closedir(dir);
        }

        if (p->n > 1) {
                qsort(p->e, p->n, sizeof(entry_t), entry_cmp);
        }

        p->cur = (old_cur < p->n) ? old_cur : p->n - 1;
        p->cur = (p->cur < 0) ? 0 : p->cur;

        for (int i = 0; select && (i < p->n); i++) {
                if (strcmp(p->e[i].name, select) == 0) {
                        p->cur = i;
                        break;
                }
        }

        free(keep);

        if (!ok) {
                tui_msgbox(global->tui, "Error", "Not enough memory to read directory", true);
        }

        return ok;
}

//==============================================================================
/**
 * @brief Reload both panels.
 */
//==============================================================================
static void reload_panels(void)
{
        panel_load(&global->panel[0], NULL);
        panel_load(&global->panel[1], NULL);
}

//==============================================================================
/**
 * @brief Show error message with errno description.
 */
//==============================================================================
static void set_msg_error(const char *what, const char *path)
{
        snprintf(global->text, sizeof(global->text), "%s\n%s\n%s", what, path, strerror(errno));
        tui_msgbox(global->tui, "Error", global->text, true);
}

//==============================================================================
/**
 * @brief Format file size to column.
 */
//==============================================================================
static void format_size(const entry_t *e, char *buf, size_t size)
{
        if (strcmp(e->name, "..") == 0) {
                snprintf(buf, size, "UP--DIR");
        } else if (S_ISDIR(e->mode)) {
                snprintf(buf, size, "<DIR>");
        } else if (e->size < 10000000) {
                snprintf(buf, size, "%u", (unsigned)e->size);
        } else if (e->size < 10000ULL * 1024 * 1024) {
                snprintf(buf, size, "%uM", (unsigned)(e->size / (1024 * 1024)));
        } else {
                snprintf(buf, size, "%uG", (unsigned)(e->size / (1024ULL * 1024 * 1024)));
        }
}

//==============================================================================
/**
 * @brief Format bytes in human readable form.
 */
//==============================================================================
static void format_bytes(uint64_t bytes, char *buf, size_t size)
{
        if (bytes < 10 * 1024) {
                snprintf(buf, size, "%u B", (unsigned)bytes);
        } else if (bytes < 10 * 1024 * 1024) {
                snprintf(buf, size, "%u KiB", (unsigned)(bytes / 1024));
        } else if (bytes < 10ULL * 1024 * 1024 * 1024) {
                snprintf(buf, size, "%u MiB", (unsigned)(bytes / (1024 * 1024)));
        } else {
                snprintf(buf, size, "%u GiB", (unsigned)(bytes / (1024ULL * 1024 * 1024)));
        }
}

//==============================================================================
/**
 * @brief Print string clipped to width (UTF-8). Too long string ends with '~'.
 *
 * @return Number of printed columns.
 */
//==============================================================================
static int print_fit(const char *str, int width)
{
        tui_t *tui = global->tui;
        int len = tui_strwidth(str);
        int n = 0;

        if (width <= 0) {
                return 0;
        }

        uint32_t ch;
        while ((n < width) && (ch = tui_utf8_decode(&str))) {
                if ((len > width) && (n == width - 1)) {
                        tui_putc(tui, '~');
                } else {
                        tui_putc(tui, ch);
                }
                n++;
        }

        return n;
}

//==============================================================================
/**
 * @brief Print path in width; too long path is shortened from the left.
 */
//==============================================================================
static void print_path(const char *path, int width)
{
        int len = tui_strwidth(path);

        if (len > width) {
                for (int skip = len - width + 3; skip > 0; skip--) {
                        tui_utf8_decode(&path);
                }

                tui_puts(global->tui, "...");
        }

        tui_puts(global->tui, path);
}

//==============================================================================
/**
 * @brief Number of list rows of panel.
 */
//==============================================================================
static int panel_rows(void)
{
        int rows = tui_rows(global->tui) - 2 - 5;
        return (rows < 1) ? 1 : rows;
}

//==============================================================================
/**
 * @brief Keep panel cursor visible.
 */
//==============================================================================
static void panel_scroll(panel_t *p)
{
        int rows = panel_rows();

        if (p->cur < p->top) {
                p->top = p->cur;
        } else if (p->cur >= p->top + rows) {
                p->top = p->cur - rows + 1;
        }

        int max_top = p->n - rows;
        p->top = (p->top > max_top) ? max_top : p->top;
        p->top = (p->top < 0) ? 0 : p->top;
}

//==============================================================================
/**
 * @brief Count marked entries and their size.
 */
//==============================================================================
static int panel_marked(const panel_t *p, uint64_t *size)
{
        int n = 0;
        uint64_t total = 0;

        for (int i = 0; i < p->n; i++) {
                if (p->e[i].marked) {
                        n++;
                        total += S_ISDIR(p->e[i].mode) ? 0 : p->e[i].size;
                }
        }

        if (size) {
                *size = total;
        }

        return n;
}

//==============================================================================
/**
 * @brief Draw panel.
 */
//==============================================================================
static void draw_panel(panel_t *p, int x, int w, bool active)
{
        tui_t *tui = global->tui;
        int h = tui_rows(tui) - 2;
        int rows = panel_rows();
        int name_w = w - 2 - SIZE_COL_W - 1;

        panel_scroll(p);

        tui_set_attr(tui, ATTR_PANEL);
        tui_fill(tui, x, 0, w, h, ' ');

        tui_set_attr(tui, ATTR_FRAME);
        tui_box(tui, x, 0, w, h, TUI_BOX_DOUBLE);
        tui_hline(tui, x + 1, h - 3, w - 2, TUI_BOX_SINGLE);
        tui_vline(tui, x + 1 + name_w, 1, rows + 1, TUI_BOX_SINGLE);

        /* path in top border */
        int path_w = w - 6;
        int plen = tui_strwidth(p->path);
        plen = (plen > path_w) ? path_w : plen;
        tui_set_attr(tui, active ? ATTR_PATH_ACTIVE : ATTR_FRAME);
        tui_move(tui, x + (w - plen - 2) / 2, 0);
        tui_putc(tui, ' ');
        print_path(p->path, path_w);
        tui_putc(tui, ' ');

        /* header */
        tui_set_attr(tui, ATTR_HEADER);
        tui_move(tui, x + 1 + (name_w - 4) / 2, 1);
        tui_puts(tui, "Name");
        tui_move(tui, x + 2 + name_w + (SIZE_COL_W - 4) / 2, 1);
        tui_puts(tui, "Size");

        /* entries */
        for (int r = 0; r < rows; r++) {
                int idx = p->top + r;
                if (idx >= p->n) {
                        break;
                }

                const entry_t *e = &p->e[idx];
                bool cursor = active && (idx == p->cur);
                uint16_t attr = ATTR_PANEL;

                if (e->marked) {
                        attr = cursor ? ATTR_CURSOR_MARK : ATTR_MARKED;
                } else if (cursor) {
                        attr = ATTR_CURSOR;
                } else if (S_ISDIR(e->mode)) {
                        attr = ATTR_DIR;
                } else if (is_exec(e)) {
                        attr = ATTR_EXEC;
                } else if (S_ISDEV(e->mode)) {
                        attr = ATTR_DEVICE;
                }

                char prefix = ' ';
                if (S_ISDIR(e->mode)) {
                        prefix = '/';
                } else if (is_exec(e)) {
                        prefix = '*';
                } else if (S_ISDEV(e->mode)) {
                        prefix = '+';
                } else if (S_ISFIFO(e->mode)) {
                        prefix = '|';
                } else if (S_ISLNK(e->mode)) {
                        prefix = '~';
                }

                if (strcmp(e->name, "..") == 0) {
                        prefix = '/';
                }

                int y = 2 + r;

                tui_set_attr(tui, attr);
                tui_fill(tui, x + 1, y, name_w, 1, ' ');
                tui_move(tui, x + 1, y);
                tui_putc(tui, prefix);
                print_fit(e->name, name_w - 1);

                char size[16];
                format_size(e, size, sizeof(size));
                tui_fill(tui, x + 2 + name_w, y, SIZE_COL_W, 1, ' ');
                tui_move(tui, x + 2 + name_w + SIZE_COL_W - (int)strlen(size), y);
                tui_puts(tui, size);

                if (cursor) {
                        tui_move(tui, x + 1 + name_w, y);
                        tui_putc(tui, 0x2502);
                }
        }

        /* mini status (local buffer: global->text can be used by caller) */
        char buf[64];
        tui_set_attr(tui, ATTR_PANEL);
        tui_move(tui, x + 1, h - 2);

        uint64_t marked_size;
        int marked = panel_marked(p, &marked_size);

        if (marked) {
                char bytes[24];
                format_bytes(marked_size, bytes, sizeof(bytes));
                tui_set_attr(tui, ATTR_MARKED);
                snprintf(buf, sizeof(buf), "%s in %d file%s", bytes, marked, (marked > 1) ? "s" : "");
                print_fit(buf, w - 2);

        } else if (p->n > 0) {
                const entry_t *e = &p->e[p->cur];
                char size[16];
                format_size(e, size, sizeof(size));
                print_fit(e->name, w - 2 - (int)strlen(size) - 1);
                tui_move(tui, x + w - 1 - (int)strlen(size), h - 2);
                tui_puts(tui, size);
        }

        /* free space in bottom border */
        struct statfs sfs;
        if ((statfs(p->path, &sfs) == 0) && (sfs.f_blocks > 0)) {
                uint64_t bsize = sfs.f_bsize;
                char free_s[24];
                char total_s[24];
                format_bytes((uint64_t)sfs.f_bfree * bsize, free_s, sizeof(free_s));
                format_bytes((uint64_t)sfs.f_blocks * bsize, total_s, sizeof(total_s));
                snprintf(buf, sizeof(buf), " %s/%s (%u%%) ", free_s, total_s,
                        (unsigned)((uint64_t)sfs.f_bfree * 100 / sfs.f_blocks));

                int tw = tui_strwidth(buf);
                if (tw < w - 4) {
                        tui_set_attr(tui, ATTR_FRAME);
                        tui_move(tui, x + w - 2 - tw, h - 1);
                        tui_puts(tui, buf);
                }
        }
}

//==============================================================================
/**
 * @brief Draw command line and function keys bar.
 */
//==============================================================================
static void draw_bottom(void)
{
        tui_t *tui = global->tui;
        int cols = tui_cols(tui);
        int rows = tui_rows(tui);

        /* command line: prompt and command (scrolled to cursor) */
        tui_set_attr(tui, ATTR_CMDLINE);
        tui_fill(tui, 0, rows - 2, cols, 1, ' ');
        tui_move(tui, 0, rows - 2);

        const char *path = active_panel()->path;
        int prompt_w = tui_strwidth(path) + 2;
        int max_prompt = cols / 3;

        if (prompt_w > max_prompt) {
                print_path(path, max_prompt - 2);
                prompt_w = max_prompt;
        } else {
                tui_puts(tui, path);
        }

        tui_puts(tui, "$ ");

        int avail = cols - prompt_w - 1;
        int col = 0;
        for (const char *p = global->cmd; (p < &global->cmd[global->cmd_pos]) && tui_utf8_decode(&p);) {
                col++;
        }

        const char *p = global->cmd;
        int skip = (col >= avail) ? col - avail + 1 : 0;
        for (int i = 0; (i < skip) && tui_utf8_decode(&p); i++) {
        }

        tui_move(tui, prompt_w, rows - 2);
        print_fit(p, avail);
        tui_set_cursor(tui, prompt_w + col - skip, rows - 2);

        /* function keys */
        int w = cols / 10;
        tui_set_attr(tui, ATTR_KEY_LABEL);
        tui_fill(tui, 0, rows - 1, cols, 1, ' ');

        for (int i = 0; i < 10; i++) {
                tui_set_attr(tui, ATTR_KEY_NUM);
                tui_move(tui, i * w, rows - 1);
                int n = tui_printf(tui, "%d", i + 1);

                tui_set_attr(tui, ATTR_KEY_LABEL);
                print_fit(FKEY_LABEL[i], w - n);
        }
}

//==============================================================================
/**
 * @brief Draw screen.
 */
//==============================================================================
static void draw(void)
{
        tui_t *tui = global->tui;
        int cols = tui_cols(tui);
        int left_w = cols / 2;

        tui_set_attr(tui, TUI_ATTR_DEFAULT);
        tui_clear(tui);

        draw_panel(&global->panel[0], 0, left_w, global->active == 0);
        draw_panel(&global->panel[1], left_w, cols - left_w, global->active == 1);
        draw_bottom();
}

//==============================================================================
/**
 * @brief Quote argument for shell (single quotes).
 *
 * @return Number of characters written (without NUL) or -1 if buffer is too small.
 */
//==============================================================================
static int shell_quote(char *dst, size_t size, const char *arg)
{
        size_t n = 0;

#define PUT(c)               \
        if (n + 1 >= size) { \
                return -1;   \
        }                    \
        dst[n++] = (c);

        PUT('\'');

        for (; *arg; arg++) {
                if (*arg == '\'') {
                        PUT('\'');
                        PUT('\\');
                        PUT('\'');
                        PUT('\'');
                } else {
                        PUT(*arg);
                }
        }

        PUT('\'');
#undef PUT

        dst[n] = '\0';
        return n;
}

//==============================================================================
/**
 * @brief Run command in selected directory (terminal is in normal mode).
 *
 * @param cmd           command line passed to program
 * @param cwd           working directory
 * @param wait_key      wait for Enter after command
 */
//==============================================================================
static void run_command(const char *cmd, const char *cwd, bool wait_key)
{
        tui_suspend(global->tui);

        process_attr_t attr = {
                .fd_stdin = fileno(stdin),
                .fd_stdout = fileno(stdout),
                .fd_stderr = fileno(stderr),
                .p_stdin = NULL,
                .p_stdout = NULL,
                .p_stderr = NULL,
                .cwd = cwd,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        errno = 0;
        int status = 0;
        pid_t pid = process_create(cmd, &attr);

        if (pid) {
                process_wait(pid, &status, MAX_DELAY_MS);
        } else {
                perror(cmd);
                wait_key = true;
        }

        if (wait_key) {
                printf("\nPress Enter to continue...");
                fflush(stdout);

                char line[8];
                if (!fgets(line, sizeof(line), stdin)) {
                        clearerr(stdin);
                }
        }

        tui_resume(global->tui);
        reload_panels();
}

//==============================================================================
/**
 * @brief Run command line typed by user (by shell).
 */
//==============================================================================
static void exec_cmdline(void)
{
        char *line = malloc(CMD_LEN * 4 + 16);
        if (!line) {
                return;
        }

        int n = snprintf(line, CMD_LEN * 4 + 16, SHELL " ");
        if (shell_quote(&line[n], CMD_LEN * 4 + 16 - n, global->cmd) >= 0) {
                run_command(line, active_panel()->path, true);
        }

        free(line);

        global->cmd[0] = '\0';
        global->cmd_pos = 0;
}

//==============================================================================
/**
 * @brief Run program with file path as argument.
 */
//==============================================================================
static void run_with_file(const char *program, const char *path, bool wait_key)
{
        char *line = malloc(PATH_LEN * 4 + 32);
        if (!line) {
                return;
        }

        int n = snprintf(line, PATH_LEN * 4 + 32, "%s ", program);
        if (shell_quote(&line[n], PATH_LEN * 4 + 32 - n, path) >= 0) {
                run_command(line, active_panel()->path, wait_key);
        }

        free(line);
}

//==============================================================================
/**
 * @brief Read line of file to buffer (for viewer).
 *
 * @return Number of bytes.
 */
//==============================================================================
static int view_read_line(FILE *f, uint32_t offset, uint32_t next, char *buf, int size)
{
        uint32_t len = next - offset;
        len = (len > (uint32_t)size) ? (uint32_t)size : len;

        if (fseek(f, offset, SEEK_SET) != 0) {
                return 0;
        }

        size_t n = fread(buf, 1, len, f);
        clearerr(f);

        /* remove line end */
        while ((n > 0) && ((buf[n - 1] == '\n') || (buf[n - 1] == '\r'))) {
                n--;
        }

        return n;
}

//==============================================================================
/**
 * @brief Draw one viewer line from selected column. Binary file is shown
 *        as ASCII (other bytes as '.').
 */
//==============================================================================
static void view_draw_line(const char *buf, int len, int y, int hoff, int cols, bool binary)
{
        tui_t *tui = global->tui;
        int col = 0;
        const char *p = buf;
        const char *end = buf + len;

        tui_move(tui, 0, y);

        while ((p < end) && (col < hoff + cols)) {
                uint32_t ch;
                const char *start = p;

                if (((uint8_t)*p < 0x80) || binary) {
                        ch = (uint8_t)*p++;
                        ch = (ch < 0x80) ? ch : '.';
                } else {
                        ch = tui_utf8_decode(&p);
                        if ((ch == 0) || (p > end)) {
                                ch = '.';
                                p = start + 1;
                        }
                }

                if (ch == '\t') {
                        int next = (col / TAB_WIDTH + 1) * TAB_WIDTH;
                        for (; col < next; col++) {
                                if (col >= hoff && col < hoff + cols) {
                                        tui_putc(tui, ' ');
                                }
                        }
                        continue;
                }

                if ((ch < 0x20) || (ch == 0x7F) || (ch == 0xFFFD)) {
                        ch = '.';
                }

                if (col >= hoff) {
                        tui_putc(tui, ch);
                }

                col++;
        }
}

//==============================================================================
/**
 * @brief Show file in viewer.
 */
//==============================================================================
static void view_file(const char *path)
{
        tui_t *tui = global->tui;

        FILE *f = fopen(path, "r");
        if (!f) {
                set_msg_error("Cannot open file", path);
                return;
        }

        /* index of line offsets (last element is end of file) */
        int cap = 256;
        int lines = 0;
        bool truncated = false;
        uint32_t *offs = malloc(cap * sizeof(uint32_t));
        char *buf = malloc(VIEW_LINE_BUF);

        if (!offs || !buf) {
                tui_msgbox(tui, "Error", "Not enough memory", true);
                goto finish;
        }

        offs[0] = 0;
        uint32_t pos = 0;
        bool binary = false;
        bool first = true;

        tui_infobox(tui, "View", "Reading file...");

        for (;;) {
                size_t n = fread(buf, 1, VIEW_LINE_BUF, f);
                if (n == 0) {
                        break;
                }

                /* file with NUL bytes at the beginning is binary */
                if (first) {
                        binary = (memchr(buf, '\0', n) != NULL);
                        first = false;
                }

                for (size_t i = 0; i < n; i++) {
                        pos++;

                        if (buf[i] != '\n') {
                                continue;
                        }

                        if (lines + 2 >= cap) {
                                if (cap >= VIEW_MAX_LINES) {
                                        truncated = true;
                                        break;
                                }

                                uint32_t *o = realloc(offs, cap * 2 * sizeof(uint32_t));
                                if (!o) {
                                        truncated = true;
                                        break;
                                }

                                offs = o;
                                cap *= 2;
                        }

                        offs[++lines] = pos;
                }

                if (truncated) {
                        break;
                }
        }

        clearerr(f);

        /* last line without line ending */
        if (!truncated && (pos > offs[lines])) {
                offs[++lines] = pos;
        }

        int top = 0;
        int hoff = 0;

        for (;;) {
                int cols = tui_cols(tui);
                int rows = tui_rows(tui);
                int text_rows = rows - 1;
                int max_top = (lines > text_rows) ? lines - text_rows : 0;

                top = (top > max_top) ? max_top : top;
                top = (top < 0) ? 0 : top;
                hoff = (hoff < 0) ? 0 : hoff;

                tui_set_attr(tui, ATTR_VIEW_BAR);
                tui_fill(tui, 0, 0, cols, 1, ' ');
                tui_move(tui, 0, 0);
                print_path(path, cols / 2);
                snprintf(global->text, sizeof(global->text), "Line %d/%d  Col %d  %d%%%s", top + 1, lines,
                        hoff,
                        lines ? (int)(((top + text_rows < lines) ? top + text_rows : lines) * 100 / lines)
                              : 100,
                        truncated ? " (truncated)" : "");
                tui_move(tui, cols - tui_strwidth(global->text) - 1, 0);
                tui_puts(tui, global->text);

                tui_set_attr(tui, ATTR_VIEW);
                tui_fill(tui, 0, 1, cols, text_rows, ' ');

                for (int r = 0; (r < text_rows) && (top + r < lines); r++) {
                        int idx = top + r;
                        int len = view_read_line(f, offs[idx], offs[idx + 1], buf, VIEW_LINE_BUF);
                        view_draw_line(buf, len, 1 + r, hoff, cols, binary);
                }

                tui_set_cursor(tui, -1, -1);
                tui_refresh(tui);

                int key = tui_getkey(tui, -1);
                int page = (text_rows > 1) ? text_rows - 1 : 1;

                switch (key) {
                case TUI_KEY_UP:
                        top--;
                        break;
                case TUI_KEY_DOWN:
                case TUI_KEY_ENTER:
                        top++;
                        break;
                case TUI_KEY_PGUP:
                case 'b':
                        top -= page;
                        break;
                case TUI_KEY_PGDN:
                case ' ':
                        top += page;
                        break;
                case TUI_KEY_HOME:
                case 'g':
                        top = 0;
                        hoff = 0;
                        break;
                case TUI_KEY_END:
                case 'G':
                        top = max_top;
                        break;
                case TUI_KEY_LEFT:
                        hoff -= cols / 4;
                        break;
                case TUI_KEY_RIGHT:
                        hoff += cols / 4;
                        break;
                case TUI_KEY_CTRL('L'):
                        tui_redraw(tui);
                        break;
                case TUI_KEY_F(3):
                case TUI_KEY_F(10):
                case TUI_KEY_ESC:
                case TUI_KEY_NONE:
                case 'q':
                        goto finish;
                default:
                        break;
                }
        }

finish:
        free(offs);
        free(buf);
        fclose(f);
}

//==============================================================================
/**
 * @brief Show progress (rate limited) and check abort key.
 *
 * @return true if operation should be aborted.
 */
//==============================================================================
static bool progress(const char *path, bool force)
{
        uint32_t now = (uint32_t)get_uptime_ms();

        if (!force && ((uint32_t)(now - global->progress_time) < PROGRESS_MS)) {
                return global->abort;
        }

        global->progress_time = now;

        char bytes[24];
        format_bytes(global->progress_bytes, bytes, sizeof(bytes));
        snprintf(global->text, sizeof(global->text), "%s\n%d files, %s\n\nEsc: abort", path,
                global->progress_files, bytes);

        draw();
        tui_infobox(global->tui, global->progress_title, global->text);

        int key = tui_getkey(global->tui, 0);
        if ((key == TUI_KEY_ESC) || (key == TUI_KEY_CTRL('C'))) {
                draw();
                global->abort = tui_yesno(global->tui, global->progress_title, "Abort operation?", true);
        }

        return global->abort;
}

//==============================================================================
/**
 * @brief Report operation error.
 *
 * @return true if operation should be aborted.
 */
//==============================================================================
static bool op_error(const char *what, const char *path)
{
        static const char *const BUTTONS[] = {"Skip", "Abort"};

        snprintf(global->text, sizeof(global->text), "%s\n%s\n%s", what, path, strerror(errno));
        draw();

        if (tui_question(global->tui, "Error", global->text, BUTTONS, 2, 0, true) != 0) {
                global->abort = true;
        }

        return global->abort;
}

//==============================================================================
/**
 * @brief Ask if file should be overwritten.
 *
 * @return true if file should be overwritten.
 */
//==============================================================================
static bool ask_overwrite(const char *path)
{
        static const char *const BUTTONS[] = {"Yes", "No", "All", "Skip all", "Cancel"};

        if (global->overwrite == OVERWRITE_ALL) {
                return true;
        } else if (global->overwrite == OVERWRITE_NONE) {
                return false;
        }

        snprintf(global->text, sizeof(global->text), "File exists:\n%s\n\nOverwrite?", path);
        draw();

        switch (tui_question(global->tui, "Copy", global->text, BUTTONS, 5, 0, true)) {
        case 0:
                return true;
        case 2:
                global->overwrite = OVERWRITE_ALL;
                return true;
        case 3:
                global->overwrite = OVERWRITE_NONE;
                return false;
        case 1:
                return false;
        default:
                global->abort = true;
                return false;
        }
}

//==============================================================================
/**
 * @brief Copy regular file.
 *
 * @return true on success (or skipped).
 */
//==============================================================================
static bool copy_file(const char *src, const char *dst, mode_t mode)
{
        if (file_info(dst, NULL, NULL) && !ask_overwrite(dst)) {
                return !global->abort;
        }

        FILE *in = fopen(src, "r");
        if (!in) {
                return !op_error("Cannot open file", src);
        }

        FILE *out = fopen(dst, "w");
        if (!out) {
                fclose(in);
                return !op_error("Cannot create file", dst);
        }

        bool ok = true;

        for (;;) {
                size_t n = fread(global->copybuf, 1, COPY_BUF_SIZE, in);
                if (n == 0) {
                        if (ferror(in)) {
                                ok = !op_error("Cannot read file", src);
                        }
                        break;
                }

                if (fwrite(global->copybuf, 1, n, out) != n) {
                        ok = !op_error("Cannot write file", dst);
                        break;
                }

                global->progress_bytes += n;

                if (progress(src, false)) {
                        ok = false;
                        break;
                }
        }

        fclose(in);

        if ((fclose(out) != 0) && ok) {
                ok = !op_error("Cannot write file", dst);
        }

        /* not finished file is removed, permissions are copied */
        if (global->abort) {
                remove(dst);
        } else if (ok) {
                chmod(dst, mode & 0777);
        }

        global->progress_files++;

        return ok;
}

//==============================================================================
/**
 * @brief Read directory entry names (directory is closed before return).
 *
 * @return Array of names (NULL terminated) or NULL on error.
 */
//==============================================================================
static char **read_names(const char *path)
{
        DIR *dir = opendir(path);
        if (!dir) {
                return NULL;
        }

        int cap = 16;
        int n = 0;
        char **names = malloc(cap * sizeof(char *));

        struct dirent *ent;
        while (names && (ent = readdir(dir))) {
                if ((strcmp(ent->d_name, ".") == 0) || (strcmp(ent->d_name, "..") == 0)) {
                        continue;
                }

                if (n + 1 >= cap) {
                        char **nn = realloc(names, cap * 2 * sizeof(char *));
                        if (!nn) {
                                break;
                        }

                        names = nn;
                        cap *= 2;
                }

                names[n] = strdup(ent->d_name);
                if (!names[n]) {
                        break;
                }

                n++;
        }

        closedir(dir);

        if (names) {
                names[n] = NULL;
        } else {
                errno = ENOMEM;
        }

        return names;
}

//==============================================================================
/**
 * @brief Release names array.
 */
//==============================================================================
static void free_names(char **names)
{
        for (int i = 0; names && names[i]; i++) {
                free(names[i]);
        }

        free(names);
}

//==============================================================================
/**
 * @brief Copy file or directory (recursive).
 *
 * @return true on success.
 */
//==============================================================================
static bool copy_any(const char *src, const char *dst, int depth)
{
        mode_t mode;

        if (!file_info(src, &mode, NULL)) {
                return !op_error("Cannot read", src);
        }

        if (!S_ISDIR(mode)) {
                return copy_file(src, dst, mode);
        }

        if (depth > MAX_DEPTH) {
                errno = ENAMETOOLONG;
                return !op_error("Directory too deep", src);
        }

        /* directory into itself */
        size_t len = strlen(src);
        if ((strncmp(src, dst, len) == 0) && ((dst[len] == '/') || (dst[len] == '\0'))) {
                errno = EINVAL;
                return !op_error("Cannot copy directory into itself", src);
        }

        if (!is_dir(dst) && (mkdir(dst, 0777) != 0)) {
                return !op_error("Cannot create directory", dst);
        }

        char **names = read_names(src);
        if (!names) {
                return !op_error("Cannot read directory", src);
        }

        bool ok = true;

        for (int i = 0; ok && names[i] && !global->abort; i++) {
                char *s = path_new(src, names[i]);
                char *d = path_new(dst, names[i]);

                if (s && d) {
                        ok = copy_any(s, d, depth + 1);
                } else {
                        ok = !op_error("Path too long", names[i]);
                }

                free(s);
                free(d);
        }

        free_names(names);

        return ok && !global->abort;
}

//==============================================================================
/**
 * @brief Delete file or directory (recursive).
 *
 * @return true on success.
 */
//==============================================================================
static bool delete_any(const char *path, int depth)
{
        mode_t mode;

        if (!file_info(path, &mode, NULL)) {
                return !op_error("Cannot delete", path);
        }

        if (S_ISDIR(mode)) {
                if (depth > MAX_DEPTH) {
                        errno = ENAMETOOLONG;
                        return !op_error("Directory too deep", path);
                }

                char **names = read_names(path);
                if (!names) {
                        return !op_error("Cannot read directory", path);
                }

                bool ok = true;

                for (int i = 0; ok && names[i] && !global->abort; i++) {
                        char *p = path_new(path, names[i]);
                        ok = p ? delete_any(p, depth + 1) : !op_error("Path too long", names[i]);
                        free(p);
                }

                free_names(names);

                if (!ok || global->abort) {
                        return false;
                }
        }

        if (remove(path) != 0) {
                return !op_error("Cannot delete", path);
        }

        global->progress_files++;

        return !progress(path, false);
}

//==============================================================================
/**
 * @brief Collect selected entries (marked or current).
 *
 * @return Number of entries (indexes are stored in allocated array).
 */
//==============================================================================
static int selected_entries(panel_t *p, int **idx)
{
        int marked = panel_marked(p, NULL);
        int n = 0;

        *idx = malloc((marked ? marked : 1) * sizeof(int));
        if (!*idx) {
                return 0;
        }

        if (marked) {
                for (int i = 0; i < p->n; i++) {
                        if (p->e[i].marked) {
                                (*idx)[n++] = i;
                        }
                }
        } else if ((p->n > 0) && (strcmp(p->e[p->cur].name, "..") != 0)) {
                (*idx)[n++] = p->cur;
        }

        return n;
}

//==============================================================================
/**
 * @brief Begin file operation (allocate buffers, reset state).
 */
//==============================================================================
static bool op_begin(const char *title)
{
        global->copybuf = malloc(COPY_BUF_SIZE);
        if (!global->copybuf) {
                tui_msgbox(global->tui, "Error", "Not enough memory", true);
                return false;
        }

        global->abort = false;
        global->overwrite = OVERWRITE_ASK;
        global->progress_time = 0;
        global->progress_bytes = 0;
        global->progress_files = 0;
        global->progress_title = title;

        return true;
}

//==============================================================================
/**
 * @brief End file operation.
 */
//==============================================================================
static void op_end(void)
{
        free(global->copybuf);
        global->copybuf = NULL;

        for (int k = 0; k < 2; k++) {
                for (int i = 0; i < global->panel[k].n; i++) {
                        global->panel[k].e[i].marked = false;
                }
        }

        reload_panels();
}

//==============================================================================
/**
 * @brief Copy (F5) or move (F6) selected entries.
 */
//==============================================================================
static void cmd_copy_move(bool move)
{
        panel_t *p = active_panel();
        const char *title = move ? "Rename/Move" : "Copy";
        int *idx;
        int n = selected_entries(p, &idx);

        if (n == 0) {
                free(idx);
                return;
        }

        if (n == 1) {
                snprintf(global->text, sizeof(global->text), "%s \"%s\" to:", move ? "Move" : "Copy",
                        p->e[idx[0]].name);
        } else {
                snprintf(global->text, sizeof(global->text),
                        "%s %d files/directories to:", move ? "Move" : "Copy", n);
        }

        snprintf(global->input, sizeof(global->input), "%s", other_panel()->path);

        if (!tui_inputbox(global->tui, title, global->text, global->input, sizeof(global->input)) ||
                (global->input[0] == '\0')) {
                free(idx);
                return;
        }

        /* relative destination is related to active panel */
        char *dest = path_new(p->path, global->input);
        bool dest_dir = dest && is_dir(dest);

        if (!dest) {
                set_msg_error("Invalid destination", global->input);
        } else if ((n > 1) && !dest_dir) {
                errno = ENOTDIR;
                set_msg_error("Destination must be a directory", dest);
        } else if (op_begin(title)) {
                for (int i = 0; (i < n) && !global->abort; i++) {
                        const char *name = p->e[idx[i]].name;
                        char *src = path_new(p->path, name);
                        char *dst = dest_dir ? path_new(dest, name) : strdup(dest);

                        if (!src || !dst) {
                                op_error("Path too long", name);
                        } else if (strcmp(src, dst) == 0) {
                                errno = EEXIST;
                                op_error("Source and destination are the same", src);
                        } else if (move && (rename(src, dst) == 0)) {
                                global->progress_files++;
                        } else if (copy_any(src, dst, 0) && move && !global->abort) {
                                /* other file system: copy and delete */
                                delete_any(src, 0);
                        }

                        free(src);
                        free(dst);
                }

                op_end();
        }

        free(dest);
        free(idx);
}

//==============================================================================
/**
 * @brief Delete selected entries (F8).
 */
//==============================================================================
static void cmd_delete(void)
{
        panel_t *p = active_panel();
        int *idx;
        int n = selected_entries(p, &idx);

        if (n == 0) {
                free(idx);
                return;
        }

        if (n == 1) {
                snprintf(global->text, sizeof(global->text), "Delete \"%s\"?", p->e[idx[0]].name);
        } else {
                snprintf(global->text, sizeof(global->text), "Delete %d files/directories?", n);
        }

        if (tui_yesno(global->tui, "Delete", global->text, false) && op_begin("Delete")) {
                for (int i = 0; (i < n) && !global->abort; i++) {
                        char *path = path_new(p->path, p->e[idx[i]].name);
                        if (path) {
                                delete_any(path, 0);
                                free(path);
                        }
                }

                op_end();
        }

        free(idx);
}

//==============================================================================
/**
 * @brief Make directory (F7).
 */
//==============================================================================
static void cmd_mkdir(void)
{
        global->input[0] = '\0';

        if (!tui_inputbox(global->tui, "Mkdir", "Enter directory name:", global->input,
                    sizeof(global->input)) ||
                (global->input[0] == '\0')) {
                return;
        }

        char *path = path_new(active_panel()->path, global->input);

        if (!path || (mkdir(path, 0777) != 0)) {
                set_msg_error("Cannot create directory", path ? path : global->input);
        } else {
                char name[PATH_LEN];
                path_parent(path, name, sizeof(name));
                panel_load(active_panel(), name);
                panel_load(other_panel(), NULL);
        }

        free(path);
}

//==============================================================================
/**
 * @brief Enter directory or run/view file (Enter key).
 */
//==============================================================================
static void cmd_enter(void)
{
        panel_t *p = active_panel();
        if (p->n == 0) {
                return;
        }

        entry_t *e = &p->e[p->cur];

        if (strcmp(e->name, "..") == 0) {
                char name[PATH_LEN];
                path_parent(p->path, name, sizeof(name));
                panel_load(p, name);

        } else if (S_ISDIR(e->mode)) {
                char path[PATH_LEN];
                if (path_join(path, sizeof(path), p->path, e->name)) {
                        DIR *dir = opendir(path);
                        if (dir) {
                                closedir(dir);
                                strcpy(p->path, path);
                                p->cur = 0;
                                p->top = 0;
                                panel_load(p, NULL);
                                p->cur = 0;
                        } else {
                                set_msg_error("Cannot open directory", path);
                        }
                }

        } else if (S_ISPROG(e->mode)) {
                /* built-in program: started by name */
                run_with_file(SHELL, e->name, true);

        } else if (is_exec(e)) {
                char path[PATH_LEN];
                if (path_join(path, sizeof(path), p->path, e->name)) {
                        run_with_file(SHELL, path, true);
                }

        } else {
                char path[PATH_LEN];
                if (path_join(path, sizeof(path), p->path, e->name)) {
                        view_file(path);
                }
        }
}

//==============================================================================
/**
 * @brief View (F3) or edit (F4) current file.
 */
//==============================================================================
static void cmd_view_edit(bool edit)
{
        panel_t *p = active_panel();
        if (p->n == 0) {
                return;
        }

        entry_t *e = &p->e[p->cur];

        if (S_ISDIR(e->mode)) {
                if (!edit) {
                        cmd_enter();
                }
                return;
        }

        char path[PATH_LEN];
        if (!path_join(path, sizeof(path), p->path, e->name)) {
                return;
        }

        if (edit) {
                run_with_file(EDITOR, path, false);
        } else {
                view_file(path);
        }
}

//==============================================================================
/**
 * @brief Mark entries: 1 mark all files, 0 unmark all, -1 invert.
 */
//==============================================================================
static void mark_all(int how)
{
        panel_t *p = active_panel();

        for (int i = 0; i < p->n; i++) {
                if (S_ISDIR(p->e[i].mode) && (how != 0)) {
                        continue;
                }

                p->e[i].marked = (how < 0) ? !p->e[i].marked : (how > 0);
        }
}

//==============================================================================
/**
 * @brief Edit command line.
 *
 * @return true if key was handled.
 */
//==============================================================================
static bool cmdline_key(int key)
{
        char *cmd = global->cmd;
        int len = strlen(cmd);
        int pos = global->cmd_pos;

        switch (key) {
        case TUI_KEY_LEFT:
                if (len == 0) {
                        return false;
                }
                while ((pos > 0) && ((cmd[--pos] & 0xC0) == 0x80)) {
                }
                break;

        case TUI_KEY_RIGHT:
                if (len == 0) {
                        return false;
                }
                while ((pos < len) && ((cmd[++pos] & 0xC0) == 0x80)) {
                }
                break;

        case TUI_KEY_BACKSPACE:
                if (pos > 0) {
                        int start = pos;
                        while ((start > 0) && ((cmd[--start] & 0xC0) == 0x80)) {
                        }
                        memmove(&cmd[start], &cmd[pos], len - pos + 1);
                        pos = start;
                }
                break;

        case TUI_KEY_DELETE:
                if (pos < len) {
                        int end = pos;
                        while ((end < len) && ((cmd[++end] & 0xC0) == 0x80)) {
                        }
                        memmove(&cmd[pos], &cmd[end], len - end + 1);
                }
                break;

        default:
                if ((key >= 0x20) && (key <= 0x10FFFF) && (key != TUI_KEY_BACKSPACE)) {
                        char utf8[4];
                        int n = tui_utf8_encode(key, utf8);
                        if (len + n < (int)sizeof(global->cmd)) {
                                memmove(&cmd[pos + n], &cmd[pos], len - pos + 1);
                                memcpy(&cmd[pos], utf8, n);
                                pos += n;
                        }
                        break;
                }
                return false;
        }

        global->cmd_pos = pos;
        return true;
}

//==============================================================================
/**
 * @brief Insert text into command line.
 */
//==============================================================================
static void cmdline_insert(const char *text)
{
        for (const char *p = text; *p;) {
                uint32_t ch = tui_utf8_decode(&p);
                cmdline_key(ch);
        }
}

//==============================================================================
/**
 * @brief Handle key.
 */
//==============================================================================
static void handle_key(int key)
{
        panel_t *p = active_panel();
        int page = panel_rows() - 1;
        page = (page < 1) ? 1 : page;
        bool cmd_empty = (global->cmd[0] == '\0');

        switch (key) {
        case TUI_KEY_F(10):
                global->quit = tui_yesno(global->tui, "Midnight Commander", "Do you really want to quit?",
                        true);
                break;

        case TUI_KEY_TAB:
                global->active = !global->active;
                break;

        case TUI_KEY_UP:
                p->cur--;
                break;

        case TUI_KEY_DOWN:
                p->cur++;
                break;

        case TUI_KEY_PGUP:
                p->cur -= page;
                break;

        case TUI_KEY_PGDN:
                p->cur += page;
                break;

        case TUI_KEY_HOME:
                p->cur = 0;
                break;

        case TUI_KEY_END:
                p->cur = p->n - 1;
                break;

        case TUI_KEY_INSERT:
        case TUI_KEY_CTRL('T'):
                if ((p->n > 0) && (strcmp(p->e[p->cur].name, "..") != 0)) {
                        p->e[p->cur].marked = !p->e[p->cur].marked;
                }
                p->cur++;
                break;

        case TUI_KEY_ENTER:
                if (cmd_empty) {
                        cmd_enter();
                } else {
                        exec_cmdline();
                }
                break;

        case TUI_MOD_ALT | TUI_KEY_ENTER:
                if (p->n > 0) {
                        cmdline_insert(p->e[p->cur].name);
                        cmdline_insert(" ");
                }
                break;

        case TUI_KEY_F(1):
                tui_msgbox(global->tui, "Help", HELP_TEXT, false);
                break;

        case TUI_KEY_F(3):
                cmd_view_edit(false);
                break;

        case TUI_KEY_F(4):
                cmd_view_edit(true);
                break;

        case TUI_KEY_F(5):
                cmd_copy_move(false);
                break;

        case TUI_KEY_F(6):
                cmd_copy_move(true);
                break;

        case TUI_KEY_F(7):
                cmd_mkdir();
                break;

        case TUI_KEY_F(8):
                cmd_delete();
                break;

        case TUI_KEY_CTRL('R'):
                reload_panels();
                break;

        case TUI_KEY_CTRL('U'): {
                panel_t tmp = global->panel[0];
                global->panel[0] = global->panel[1];
                global->panel[1] = tmp;
                global->active = !global->active;
                break;
        }

        case TUI_KEY_CTRL('L'):
                tui_redraw(global->tui);
                break;

        case '+':
        case '-':
        case '*':
                if (cmd_empty) {
                        mark_all((key == '+') ? 1 : (key == '-') ? 0 : -1);
                } else {
                        cmdline_key(key);
                }
                break;

        default:
                cmdline_key(key);
                break;
        }

        p->cur = (p->cur >= p->n) ? p->n - 1 : p->cur;
        p->cur = (p->cur < 0) ? 0 : p->cur;
}

//==============================================================================
/**
 * @brief Set panel path from argument or current directory.
 */
//==============================================================================
static void panel_init(panel_t *p, const char *arg)
{
        char cwd[PATH_LEN];

        if (!getcwd(cwd, sizeof(cwd))) {
                strcpy(cwd, "/");
        }

        if (!arg || !path_join(p->path, sizeof(p->path), cwd, arg) || !is_dir(p->path)) {
                snprintf(p->path, sizeof(p->path), "%s", cwd);
        }

        /* remove trailing slash */
        size_t len = strlen(p->path);
        while ((len > 1) && (p->path[len - 1] == '/')) {
                p->path[--len] = '\0';
        }
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if ((argc > 1) && ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0))) {
                printf("Usage: %s [left dir] [right dir]\n", argv[0]);
                return EXIT_SUCCESS;
        }

        panel_init(&global->panel[0], (argc > 1) ? argv[1] : NULL);
        panel_init(&global->panel[1], (argc > 2) ? argv[2] : NULL);

        global->tui = tui_init();
        if (!global->tui) {
                perror(argv[0]);
                return EXIT_FAILURE;
        }

        reload_panels();

        while (!global->quit) {
                draw();
                tui_refresh(global->tui);

                int key = tui_getkey(global->tui, -1);
                if (key == TUI_KEY_NONE) {
                        break;
                }

                handle_key(key);
        }

        tui_deinit(global->tui);

        panel_free(&global->panel[0]);
        panel_free(&global->panel[1]);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
