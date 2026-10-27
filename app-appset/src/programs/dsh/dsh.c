/*==============================================================================
@file    dsh.c

@author  Daniel Zorychta

@brief   dnx RTOS Shell interpreter

        MIT License

        Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

// #pragma GCC optimize("Og")

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/misc.h>
#include <dnx/vt100.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define PROMPT_LINE_LEN  256
#define CWD_PATH_LEN     128
#define HISTORY_NEXT_KEY "\033^[A"
#define HISTORY_PREV_KEY "\033^[B"
#define COMMAND_HINT_KEY "\033^[T"
#define HISTORY_ENTRIES  8
#define PIPE_NAME_LEN    48
#define TEST_MAX_ARGS    8
#define NOT_FOUND        SIZE_MAX
#define CAPTURE_MAX      4096   /* max. captured output of $(cmd) [bytes]  */
#define CAPTURE_DEPTH    4      /* max. nesting of $(cmd)                  */
#define SLEEP_SLICE_MS   20     /* CTRL+C check period during sleep        */
#define PIPELINE_POLL_MS 20     /* pipeline programs exit check period     */

#ifndef ETX
#define ETX              0x03   /* CTRL+C */
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)    (sizeof(a) / sizeof((a)[0]))
#endif

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
/** Shell variable (single linked list). */
typedef struct var {
        struct var *next;
        char       *name;
        char       *value;
} var_t;

/** Single pipeline stage. Pipe connects stage i (writer) with stage i+1 (reader). */
typedef struct {
        process_attr_t attr;
        pid_t          pid;
        FILE          *pipe;
        char           pipe_name[PIPE_NAME_LEN];
        bool           done;
} stage_t;

/** Control flow result of executed line(s). */
typedef enum {
        FLOW_NEXT,              /*!< continue with next line                    */
        FLOW_BREAK,             /*!< 'break' executed                           */
        FLOW_CONTINUE,          /*!< 'continue' executed                        */
        FLOW_EXIT,              /*!< 'exit' executed                            */
        FLOW_ERROR,             /*!< syntax error, stop block execution         */
        FLOW_INTERRUPT,         /*!< CTRL+C pressed, stop block execution       */
} flow_t;

/** Lines of compound command (if/while/for) collected before execution. */
typedef struct {
        char  **line;
        size_t  count;
        size_t  size;
} block_t;

/** Dynamic string. */
typedef struct {
        char   *s;
        size_t  len;
        size_t  size;
        bool    ok;
} strbuf_t;

/** Arithmetic expression parser state. */
typedef struct {
        const char *p;
        bool        ok;
} arith_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void clear_prompt(void);
static void print_prompt(void);
static bool read_input(void);
static bool history_request(void);
static bool command_hint(void);
static bool is_cd_cmd(const char *cmd);
static bool is_exit_cmd(const char *cmd);
static bool is_clear_cmd(const char *cmd);
static bool is_cmd(const char *cmd, const char *name);
static bool is_detached_cmd(char *cmd);
static char *find_arg(char *cmd);
static void change_directory(char *str);
static char *trim_string(char *str);
static void print_fail_message(char *cmd);
static bool run_pipeline(char **cmds, size_t count, char *std_in, char *file,
                         const char *fmode, bool detached);
static bool analyze_line(char *cmd);
static var_t *var_find(const char *name, size_t len);
static bool var_set(const char *name, const char *value);
static void var_unset(const char *name);
static void var_list(void);
static void var_free_all(void);
static bool is_assignment(const char *cmd);
static void assign_variable(char *cmd);
static char *expand_variables(const char *src);
static bool builtin_variables(char *cmd);
static bool builtin_test(char *cmd);
static bool builtin_sleep(char *cmd);
static char *next_token(char **cur);
static flow_t exec_simple(const char *raw);
static char *next_segment(char **cur);
static flow_t exec_range(char **lines, size_t from, size_t to);
static flow_t run_block(char *first);
static char *read_command(void);
static void drain_stdin(void);
static bool interrupt_requested(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char line[PROMPT_LINE_LEN];
        char *history[HISTORY_ENTRIES];
        u8_t history_wr;
        u8_t history_rd;
        char cwd[CWD_PATH_LEN];
        bool prompt_enable;
        bool continuation;
        FILE *input;
        bool stream_closed;
        const char *pipe_file;
        var_t *vars;
        int status;
        char *next_seg;
        bool interrupted;
        const char *capture_file;
        u8_t capture_depth;
        bool captured;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(dsh, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Clear prompt line
 */
//==============================================================================
static void clear_prompt()
{
        memset(global->line, '\0', PROMPT_LINE_LEN);
}

//==============================================================================
/**
 * @brief Function print line prompt
 */
//==============================================================================
static void print_prompt(void)
{
        if (global->prompt_enable && global->input == stdin) {

                int err = errno;
                ioctl(fileno(stdin), IOCTL_TTY__RAW_MODE_OFF);
                errno = err;

                if (global->continuation) {
                        printf(VT100_FONT_COLOR_GREEN "> " VT100_RESET_ATTRIBUTES);
                        fflush(stdout);
                        return;
                }

                char hostname[64];
                get_host_name(hostname, sizeof(hostname));

                printf(VT100_FONT_COLOR_GREEN "%s:%s" VT100_RESET_ATTRIBUTES "\n", hostname, global->cwd);

                printf(VT100_FONT_COLOR_GREEN "$ " VT100_RESET_ATTRIBUTES);

                fflush(stdout);
        }
}

//==============================================================================
/**
 * @brief Read command from selected file
 *
 * @return true if string read, false if not
 */
//==============================================================================
static bool read_input(void)
{
        if (global->stream_closed) {
                strcpy(global->line, "exit");
                return true;
        }

        if (fgets(global->line, PROMPT_LINE_LEN, global->input)) {

                if (feof(global->input)) {
                        global->stream_closed = true;
                }

                /* remove LF/CR at the end of line (last line of file may have no LF) */
                size_t len = strlen(global->line);
                while (len and ((global->line[len - 1] == '\n') or (global->line[len - 1] == '\r'))) {
                        global->line[--len] = '\0';
                }

                return true;
        } else {
                return false;
        }
}

//==============================================================================
/**
 * @brief Function handle history. Check if command line contain AUP, ADN keys
 *        and set prompt to historical value
 *
 * @return true if history got, false is new command inserted
 */
//==============================================================================
static bool history_request(void)
{
        if (strcmp(global->line, HISTORY_NEXT_KEY) == 0) {

                bool found = false;

                for (size_t i = 0; i < HISTORY_ENTRIES and not found; i++) {
                        if (not isstrempty(global->history[global->history_rd])) {
                                ioctl(fileno(global->input), IOCTL_TTY__SET_EDITLINE,
                                        global->history[global->history_rd]);
                                found = true;
                        }

                        if (++global->history_rd >= HISTORY_ENTRIES) {
                                global->history_rd = 0;
                        }
                }

                global->prompt_enable = false;

                return true;

        } else if (strcmp(global->line, HISTORY_PREV_KEY) == 0) {

                bool found = false;

                for (size_t i = 0; i < HISTORY_ENTRIES and not found; i++) {
                        if (--global->history_rd >= HISTORY_ENTRIES) {
                                global->history_rd = HISTORY_ENTRIES - 1;
                        }

                        if (not isstrempty(global->history[global->history_rd])) {
                                ioctl(fileno(global->input), IOCTL_TTY__SET_EDITLINE,
                                        global->history[global->history_rd]);
                                found = true;
                        }
                }

                global->prompt_enable = false;

                return true;

        } else if (strrchr(global->line, '\e') == NULL) {
                global->prompt_enable = true;

                if (strlen(global->line) and (global->line[0] != '\033')) {

                        if ((global->history[global->history_rd] == NULL) or
                                not isstreq(global->history[global->history_wr], global->line)) {

                                if (++global->history_wr >= HISTORY_ENTRIES) {
                                        global->history_wr = 0;
                                }

                                if (global->history[global->history_wr]) {
                                        free(global->history[global->history_wr]);
                                }

                                global->history[global->history_wr] = strdup(global->line);
                        }

                        global->history_rd = global->history_wr;
                }

                return false;
        } else {
                global->prompt_enable = true;
                return false;
        }
}

//==============================================================================
/**
 * @brief Finds hint for typed command
 * @param None
 * @return true if hint key was recognized, otherwise false
 */
//==============================================================================
static bool command_hint(void)
{
        char *tabstart = strchr(global->line, '\033');
        if (tabstart and (strcmp(tabstart, COMMAND_HINT_KEY) == 0)) {

                *tabstart = '\0';

                char search[128];
                char *line = "";

                /*
                 * Setup file search path
                 */
                char *begin = NULL;
                if ((begin = strrchr(global->line, ' '))) {
                        line = begin + 1;
                } else if (strncmp(global->line, "./", 2) == 0) {
                        begin = global->line;
                        line = global->line + 2;
                } else if ((begin = strchr(global->line, '/'))) {
                        line = begin;
                }

                if (begin) {
                        if (*line == '/') {
                                char *slash = strrchr(line, '/');

                                if (slash == line) {
                                        strlcpy(search, "/", sizeof(search));
                                        line++;
                                } else {
                                        size_t len = (slash - line) + 1;
                                        strlcpy(search, line, min(sizeof(search), len));
                                        line = slash + 1;
                                }
                        } else {
                                if (strncmp(line, "./", 2) == 0) {
                                        line += 2;
                                }

                                if (*begin == '/') {
                                        strlcpy(search, "/", sizeof(search));
                                } else {
                                        strlcpy(search, global->cwd, sizeof(search));
                                        strlcat(search, "/", sizeof(search));
                                }

                                char *slash = strrchr(line, '/');
                                if (slash) {
                                        size_t len = (slash - line) + strlen(search) + 1;
                                        strlcat(search, line, min(sizeof(search), len));
                                        line = slash + 1;
                                }
                        }

                } else {
                        strlcpy(search, "/proc/bin", sizeof(search));
                        line = global->line;
                }

                /*
                 * Match entered string
                 */
                if (strlen(global->line) != 0) {
                        dirent_t *dirent;
                        int matching_entries = 0;

                        DIR *dir = opendir(search);
                        if (dir) {
                                while ((matching_entries < 2) and (dirent = readdir(dir))) {
                                        if (strncmp(dirent->d_name, line, strlen(line)) == 0) {
                                                matching_entries++;
                                        }
                                }

                                if (matching_entries > 1) {
                                        puts("");
                                }

                                closedir(dir);
                                dir = opendir(search);

                                char common[128];
                                memset(common, EOT, sizeof(common));
                                memcpy(common, line, min(strlen(line), sizeof(common) - 1));
                                size_t min_len = UINT16_MAX;

                                while ((dirent = readdir(dir))) {

                                        if (strncmp(dirent->d_name, line, strlen(line)) == 0) {
                                                if (matching_entries > 1) {
                                                        printf("'%s'  ", dirent->d_name);

                                                        min_len = min(min_len, strlen(dirent->d_name));

                                                        for (size_t i = 0; i < min_len; i++) {
                                                                if (common[i] == EOT) {
                                                                        common[i] = dirent->d_name[i];

                                                                } else if (common[i] != dirent->d_name[i]) {
                                                                        common[i] = '\0';
                                                                        break;
                                                                }
                                                        }

                                                        common[min_len] = '\0';

                                                } else {
                                                        strlcpy(line, dirent->d_name,
                                                                PROMPT_LINE_LEN - (global->line - line));

                                                        if (S_ISDIR(dirent->mode)) {
                                                                strlcat(global->line, "/", PROMPT_LINE_LEN);
                                                        }

                                                        ioctl(fileno(global->input), IOCTL_TTY__SET_EDITLINE,
                                                                global->line);
                                                        break;
                                                }
                                        }
                                }

                                if (matching_entries > 1) {
                                        puts(" ");
                                        print_prompt();
                                        ioctl(fileno(global->input), IOCTL_TTY__REFRESH_LAST_LINE);

                                        strlcpy(line, common, PROMPT_LINE_LEN - (global->line - line));
                                        ioctl(fileno(global->input), IOCTL_TTY__SET_EDITLINE, global->line);
                                }

                                closedir(dir);
                        }
                }

                global->prompt_enable = false;
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief Check if command is CD
 *
 * @param cmd
 *
 * @return true if CD command, otherwise false
 */
//==============================================================================
static bool is_cd_cmd(const char *cmd)
{
        return strncmp(cmd, "cd ", 3) == 0 || strcmp(cmd, "cd") == 0;
}

//==============================================================================
/**
 * @brief Check if command is EXIT
 *
 * @param cmd
 *
 * @return true if EXIT command, otherwise false
 */
//==============================================================================
static bool is_exit_cmd(const char *cmd)
{
        return strncmp(cmd, "exit ", 5) == 0 || strcmp(cmd, "exit") == 0;
}

//==============================================================================
/**
 * @brief Check if command is CLEAR
 *
 * @param  cmd
 *
 * @return true if EXIT command, otherwise false
 */
//==============================================================================
static bool is_clear_cmd(const char *cmd)
{
        return strncmp(cmd, "clear ", 6) == 0 || strcmp(cmd, "clear") == 0;
}

//==============================================================================
/**
 * @brief Check if command line starts with selected built-in command name or
 *        keyword. Name must be terminated by end of line, space or ';'.
 *
 * @param  cmd          command line
 * @param  name         command name
 *
 * @return true if command match, otherwise false
 */
//==============================================================================
static bool is_cmd(const char *cmd, const char *name)
{
        size_t len = strlen(name);
        return (strncmp(cmd, name, len) == 0)
           and ((cmd[len] == '\0') or (cmd[len] == ';') or isspace(cast(int, cmd[len])));
}

//==============================================================================
/**
 * @brief Check if line is commented.
 *
 * @param  cmd
 *
 * @return True when commented, otherwise false.
 */
//==============================================================================
static bool is_comment(const char *cmd)
{
        return cmd[0] == '#';
}

//==============================================================================
/**
 * @brief  Find ampersand at end of string and replace it by null.
 *
 * @param  cmd  Command string. String modified when ampersand is found.
 *
 * @return True when ampersand found, otherwise false.
 */
//==============================================================================
static bool is_detached_cmd(char *cmd)
{
        size_t len = strlen(cmd);
        char *back = &cmd[len - 1];
        while (len-- && isspace(cast(int, *back)) && --back)
                ;

        if (*back == '&') {
                *back = '\0';
                return true;
        } else {
                return false;
        }
}

//==============================================================================
/**
 * @brief Finds arguments after command
 *
 * @param cmd           command line begin
 *
 * @return pointer to first argument
 */
//==============================================================================
static char *find_arg(char *cmd)
{
        char *arg;
        if ((arg = strchr(cmd, ' ')) != NULL) {
                *(arg++) = '\0';
                arg += strspn(arg, " ");
        } else {
                arg = strchr(cmd, '\0');
        }

        return arg;
}

//==============================================================================
/**
 * @brief Function change current working path
 *
 * @param str          command name with arguments
 */
//==============================================================================
static void change_directory(char *str)
{
        char *arg = find_arg(str);

        memset(global->cwd, 0, sizeof(global->cwd));

        if (arg[0] == '/') {
                strlcpy(global->cwd, arg, sizeof(global->cwd));
        } else {
                getcwd(global->cwd, sizeof(global->cwd));
                strlcat(global->cwd, "/", sizeof(global->cwd));
                strlcat(global->cwd, arg, sizeof(global->cwd));
                strlcat(global->cwd, "/", sizeof(global->cwd));
        }

        DIR *dir = opendir(global->cwd);
        if (dir) {
                closedir(dir);
                chdir(global->cwd);
                global->status = 0;
        } else {
                perror(arg);
                global->status = 1;
        }

        getcwd(global->cwd, sizeof(global->cwd));
}

//==============================================================================
/**
 * @brief Remove leading and trailing spaces.
 *
 * @param str           string to trim
 *
 * @return New string pointer (basing on the same buffer).
 */
//==============================================================================
static char *trim_string(char *str)
{
        str = str + strspn(str, " \t");

        for (int i = strlen(str) - 1; i > 0; i--) {
                if (isspace(cast(int, str[i]))) {
                        str[i] = '\0';
                } else {
                        break;
                }
        }

        return str;
}

//==============================================================================
/**
 * @brief Function print command find failure
 *
 * @param cmd           cmd string
 */
//==============================================================================
static void print_fail_message(char *cmd)
{
        cmd = trim_string(cmd);

        if (strchr(cmd, ' ')) {
                *strchr(cmd, ' ') = '\0';
        }

        if (errno == ENOMEM || errno == EACCES) {
                perror(cmd);
        } else {
                printf("\'%s\' is unknown command.\n", cmd);
        }
}

//==============================================================================
/**
 * @brief Read all pending characters from stdin (non-blocking). If CTRL+C
 *        (ETX) is found then interrupt flag is set.
 */
//==============================================================================
static void drain_stdin(void)
{
        ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);

        int c;
        while ((c = getchar()) != EOF) {
                if (c == ETX) {
                        global->interrupted = true;
                }
        }

        clearerr(stdin);
        ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);
}

//==============================================================================
/**
 * @brief Check if CTRL+C was pressed. Flag is cleared after read.
 *
 * @return true if execution should be interrupted.
 */
//==============================================================================
static bool interrupt_requested(void)
{
        drain_stdin();

        if (global->interrupted) {
                global->interrupted = false;
                puts("^C");
                global->status = 130;
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief Print syntax error and set status.
 *
 * @param msg           error details
 *
 * @return FLOW_ERROR
 */
//==============================================================================
static flow_t syntax_error(const char *msg)
{
        printf("sh: syntax error: %s\n", msg);
        global->status = 2;
        return FLOW_ERROR;
}

//==============================================================================
/**
 * @brief Get next word from string. Quotes ("..." and '...') are removed and
 *        their content is treated as a single word. String is modified.
 *
 * @param cur           cursor (updated)
 *
 * @return Word or NULL if no more words.
 */
//==============================================================================
static char *next_token(char **cur)
{
        char *s = *cur + strspn(*cur, " \t");
        if (*s == '\0') {
                *cur = s;
                return NULL;
        }

        char *tok = s;
        char *w = s;
        char quote = '\0';

        while (*s and (quote or not isspace(cast(int, *s)))) {
                if (quote) {
                        if (*s == quote) {
                                quote = '\0';
                        } else {
                                *w++ = *s;
                        }
                } else if ((*s == '"') or (*s == '\'')) {
                        quote = *s;
                } else {
                        *w++ = *s;
                }

                s++;
        }

        if (*s) {
                s++;
        }

        *w = '\0';
        *cur = s;

        return tok;
}

//==============================================================================
/**
 * @brief Check if character can be a part of variable name.
 */
//==============================================================================
static bool is_var_name_char(char c, bool first)
{
        int ch = cast(unsigned char, c);
        return isalpha(ch) or (c == '_') or (not first and isdigit(ch));
}

//==============================================================================
/**
 * @brief Check if whole string is valid variable name.
 */
//==============================================================================
static bool is_var_name(const char *name)
{
        if (not is_var_name_char(name[0], true)) {
                return false;
        }

        for (size_t i = 1; name[i]; i++) {
                if (not is_var_name_char(name[i], false)) {
                        return false;
                }
        }

        return true;
}

//==============================================================================
/**
 * @brief Find shell variable.
 *
 * @param name          variable name (not necessarily null terminated)
 * @param len           name length
 *
 * @return Variable object or NULL if not found.
 */
//==============================================================================
static var_t *var_find(const char *name, size_t len)
{
        for (var_t *var = global->vars; var; var = var->next) {
                if ((strncmp(var->name, name, len) == 0) and (var->name[len] == '\0')) {
                        return var;
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief Create or modify shell variable.
 *
 * @param name          variable name
 * @param value         variable value
 *
 * @return true on success, false on error.
 */
//==============================================================================
static bool var_set(const char *name, const char *value)
{
        char *val = strdup(value);
        if (!val) {
                perror("sh");
                return false;
        }

        var_t *var = var_find(name, strlen(name));
        if (var) {
                free(var->value);
                var->value = val;
                return true;
        }

        var = malloc(sizeof(var_t));
        if (var) {
                var->name = strdup(name);
                if (var->name) {
                        var->value  = val;
                        var->next   = global->vars;
                        global->vars = var;
                        return true;
                }

                free(var);
        }

        free(val);
        perror("sh");
        return false;
}

//==============================================================================
/**
 * @brief Remove shell variable.
 *
 * @param name          variable name
 */
//==============================================================================
static void var_unset(const char *name)
{
        for (var_t **pp = &global->vars; *pp; pp = &(*pp)->next) {
                var_t *var = *pp;

                if (isstreq(var->name, name)) {
                        *pp = var->next;
                        free(var->name);
                        free(var->value);
                        free(var);
                        return;
                }
        }
}

//==============================================================================
/**
 * @brief Print all shell variables.
 */
//==============================================================================
static void var_list(void)
{
        for (var_t *var = global->vars; var; var = var->next) {
                printf("%s=%s\n", var->name, var->value);
        }
}

//==============================================================================
/**
 * @brief Free all shell variables.
 */
//==============================================================================
static void var_free_all(void)
{
        while (global->vars) {
                var_t *next = global->vars->next;
                free(global->vars->name);
                free(global->vars->value);
                free(global->vars);
                global->vars = next;
        }
}

//==============================================================================
/**
 * @brief Check if command line is variable assignment (NAME=value).
 *
 * @param cmd           command line
 *
 * @return true if assignment, otherwise false.
 */
//==============================================================================
static bool is_assignment(const char *cmd)
{
        if (not is_var_name_char(cmd[0], true)) {
                return false;
        }

        size_t i = 1;
        while (is_var_name_char(cmd[i], false)) {
                i++;
        }

        return cmd[i] == '=';
}

//==============================================================================
/**
 * @brief Execute variable assignment (NAME=value, NAME="value", NAME='value').
 *
 * @param cmd           command line (modified)
 */
//==============================================================================
static void assign_variable(char *cmd)
{
        char *eq = strchr(cmd, '=');
        *eq = '\0';

        char *value = trim_string(eq + 1);
        size_t len  = strlen(value);

        if ((len >= 2) and ((value[0] == '"') or (value[0] == '\''))
           and (value[len - 1] == value[0])) {

                value[len - 1] = '\0';
                value++;
        }

        bool ok = var_set(cmd, value);

        /* NAME=$(cmd) keeps exit code of cmd (as in sh) */
        if (not ok or not global->captured) {
                global->status = ok ? 0 : 1;
        }
}

//==============================================================================
/**
 * @brief Arithmetic: skip white spaces.
 */
//==============================================================================
static void arith_ws(arith_t *a)
{
        while (isspace(cast(unsigned char, *a->p))) {
                a->p++;
        }
}

static long arith_expr(arith_t *a);

//==============================================================================
/**
 * @brief Arithmetic: factor := number | name | (expr) | -factor | +factor
 */
//==============================================================================
static long arith_factor(arith_t *a)
{
        arith_ws(a);

        if (*a->p == '(') {
                a->p++;
                long v = arith_expr(a);
                arith_ws(a);
                if (*a->p == ')') {
                        a->p++;
                } else {
                        a->ok = false;
                }
                return v;

        } else if (*a->p == '-') {
                a->p++;
                return -arith_factor(a);

        } else if (*a->p == '+') {
                a->p++;
                return arith_factor(a);

        } else if (isdigit(cast(unsigned char, *a->p))) {
                char *end;
                long v = strtol(a->p, &end, 0);
                a->p = end;
                return v;

        } else if (is_var_name_char(*a->p, true)) {
                size_t len = 0;
                while (is_var_name_char(a->p[len], len == 0)) {
                        len++;
                }

                var_t *var = var_find(a->p, len);
                a->p += len;
                return var ? strtol(var->value, NULL, 0) : 0;
        }

        a->ok = false;
        return 0;
}

//==============================================================================
/**
 * @brief Arithmetic: term := factor { (*|/|%) factor }
 */
//==============================================================================
static long arith_term(arith_t *a)
{
        long v = arith_factor(a);

        while (a->ok) {
                arith_ws(a);
                char op = *a->p;

                if ((op != '*') and (op != '/') and (op != '%')) {
                        break;
                }

                a->p++;
                long r = arith_factor(a);

                if ((op != '*') and (r == 0)) {
                        a->ok = false;  // division by zero
                        return 0;
                }

                v = (op == '*') ? v * r : (op == '/') ? v / r : v % r;
        }

        return v;
}

//==============================================================================
/**
 * @brief Arithmetic: expr := term { (+|-) term }
 */
//==============================================================================
static long arith_expr(arith_t *a)
{
        long v = arith_term(a);

        while (a->ok) {
                arith_ws(a);
                char op = *a->p;

                if ((op != '+') and (op != '-')) {
                        break;
                }

                a->p++;
                long r = arith_term(a);
                v = (op == '+') ? v + r : v - r;
        }

        return v;
}

//==============================================================================
/**
 * @brief Evaluate arithmetic expansion $((expr)).
 *
 * @param src           pointer to '$' of "$(("
 * @param res           result
 *
 * @return Pointer to character after "))" or NULL on error.
 */
//==============================================================================
static const char *expand_arith(const char *src, long *res)
{
        const char *p = src + 1;
        int depth = 0;

        for (; *p; p++) {
                if (*p == '(') {
                        depth++;
                } else if ((*p == ')') and (--depth == 0)) {
                        break;
                }
        }

        if ((*p != ')') or (p[-1] != ')') or (p - 1 < src + 3)) {
                puts("sh: arithmetic: missing '))'");
                return NULL;
        }

        size_t len = (p - 1) - (src + 3);
        char *inner = malloc(len + 1);
        char *exp = NULL;
        const char *next = NULL;

        if (inner) {
                memcpy(inner, src + 3, len);
                inner[len] = '\0';

                exp = expand_variables(inner);
                if (exp) {
                        arith_t a = {.p = exp, .ok = true};
                        *res = arith_expr(&a);
                        arith_ws(&a);

                        if (a.ok and (*a.p == '\0')) {
                                next = p + 1;
                        } else {
                                printf("sh: arithmetic error: '%s'\n", exp);
                        }
                }
        } else {
                perror("sh");
        }

        free(inner);
        free(exp);

        return next;
}

//==============================================================================
/**
 * @brief Append data to dynamic string. On allocation error ok flag is cleared.
 */
//==============================================================================
static void sb_append(strbuf_t *sb, const char *str, size_t n)
{
        if (not sb->ok) {
                return;
        }

        if (sb->len + n + 1 > sb->size) {
                size_t size = sb->size ? sb->size * 2 : 64;
                while (size < sb->len + n + 1) {
                        size *= 2;
                }

                char *s = malloc(size);
                if (!s) {
                        sb->ok = false;
                        return;
                }

                if (sb->s) {
                        memcpy(s, sb->s, sb->len);
                        free(sb->s);
                }

                sb->s    = s;
                sb->size = size;
        }

        memcpy(&sb->s[sb->len], str, n);
        sb->len += n;
        sb->s[sb->len] = '\0';
}

//==============================================================================
/**
 * @brief Append expanded value. CR is removed. When value is not quoted then
 *        new lines and tabs are converted to spaces (words separators).
 */
//==============================================================================
static void sb_append_value(strbuf_t *sb, const char *val, bool keep_nl)
{
        for (; *val and sb->ok; val++) {
                char c = *val;

                if (c == '\r') {
                        continue;
                }

                if (not keep_nl and ((c == '\n') or (c == '\t'))) {
                        c = ' ';
                }

                sb_append(sb, &c, 1);
        }
}

//==============================================================================
/**
 * @brief Find closing parenthesis (quotes and nesting are respected).
 *
 * @param p             pointer to opening '('
 *
 * @return Pointer to matching ')' or NULL.
 */
//==============================================================================
static const char *find_closing(const char *p)
{
        int depth = 0;
        char quote = '\0';

        for (; *p; p++) {
                if (quote) {
                        if (*p == quote) {
                                quote = '\0';
                        }
                } else if ((*p == '\'') or (*p == '"')) {
                        quote = *p;
                } else if (*p == '(') {
                        depth++;
                } else if ((*p == ')') and (--depth == 0)) {
                        return p;
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief Execute command(s) and return its standard output (command
 *        substitution). Output is written to temporary file in /run.
 *        Trailing new lines are removed.
 *
 * @param cmd           command (may contain ';' separated commands)
 * @param len           command length
 *
 * @return Allocated string or NULL on error.
 */
//==============================================================================
static char *capture_output(const char *cmd, size_t len)
{
        if (global->capture_depth >= CAPTURE_DEPTH) {
                puts("sh: command substitution nested too deep");
                return NULL;
        }

        char path[PIPE_NAME_LEN];
        snprintf(path, sizeof(path), "%scap%llx-%x", global->pipe_file,
                 get_uptime_ms(), cast(unsigned, global->capture_depth));

        /* create empty file, programs append their output */
        FILE *f = fopen(path, "w");
        if (!f) {
                perror("sh");
                return NULL;
        }
        fclose(f);

        char *buf = malloc(len + 1);
        if (!buf) {
                perror("sh");
                remove(path);
                return NULL;
        }

        memcpy(buf, cmd, len);
        buf[len] = '\0';

        const char *prev = global->capture_file;
        global->capture_file = path;
        global->capture_depth++;

        char *cur = buf;
        char *seg;
        while ((seg = next_segment(&cur))) {
                seg = trim_string(seg);

                if ((strlen(seg) > 0) and (exec_simple(seg) != FLOW_NEXT)) {
                        break;
                }
        }

        global->capture_depth--;
        global->capture_file = prev;
        global->captured = true;
        free(buf);

        /* read output */
        strbuf_t sb = {.s = NULL, .len = 0, .size = 0, .ok = true};
        sb_append(&sb, "", 0);
        bool truncated = false;

        f = fopen(path, "r");
        if (f) {
                char chunk[64];
                size_t n;

                while (sb.ok and not truncated and ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)) {
                        if (sb.len + n > CAPTURE_MAX) {
                                n = CAPTURE_MAX - sb.len;
                                truncated = true;
                        }

                        sb_append(&sb, chunk, n);
                }

                fclose(f);
        }

        remove(path);

        if (not sb.ok) {
                puts("sh: not enough memory");
                free(sb.s);
                return NULL;
        }

        if (truncated) {
                printf("sh: command output truncated to %d bytes\n", CAPTURE_MAX);
        }

        while ((sb.len > 0) and ((sb.s[sb.len - 1] == '\n') or (sb.s[sb.len - 1] == '\r'))) {
                sb.s[--sb.len] = '\0';
        }

        return sb.s;
}

//==============================================================================
/**
 * @brief Expand command line: $NAME, ${NAME}, $?, $((expr)), $(cmd), `cmd`.
 *        Nothing is expanded inside single quotes, \$ and \` give literal
 *        characters. Not existing variables are expanded to empty string.
 *        New lines of expanded values are kept in assignments (NAME=...)
 *        and inside double quotes, otherwise are converted to spaces.
 *
 * @param src           source line
 *
 * @return Allocated expanded line or NULL on error.
 */
//==============================================================================
static char *expand_variables(const char *src)
{
        strbuf_t sb = {.s = NULL, .len = 0, .size = 0, .ok = true};
        bool squote = false;
        bool dquote = false;
        char num[24];

        bool assign = is_assignment(src);
        if (not assign and (strncmp(src, "export ", 7) == 0)) {
                const char *arg = src + 7;
                assign = is_assignment(arg + strspn(arg, " \t"));
        }

        sb_append(&sb, "", 0);

        while (*src and sb.ok) {

                if ((*src == '\'') and not dquote) {
                        squote = not squote;
                        sb_append(&sb, src++, 1);

                } else if ((*src == '"') and not squote) {
                        dquote = not dquote;
                        sb_append(&sb, src++, 1);

                } else if (squote) {
                        sb_append(&sb, src++, 1);

                } else if ((*src == '\\') and ((src[1] == '$') or (src[1] == '`'))) {
                        sb_append(&sb, src + 1, 1);
                        src += 2;

                } else if ((*src == '$') and (src[1] == '?')) {
                        snprintf(num, sizeof(num), "%d", global->status);
                        sb_append(&sb, num, strlen(num));
                        src += 2;

                } else if ((*src == '$') and (src[1] == '(') and (src[2] == '(')) {
                        long res = 0;
                        const char *next = expand_arith(src, &res);
                        if (!next) {
                                goto error;
                        }

                        snprintf(num, sizeof(num), "%ld", res);
                        sb_append(&sb, num, strlen(num));
                        src = next;

                } else if (((*src == '$') and (src[1] == '(')) or (*src == '`')) {
                        const char *begin;
                        const char *end;

                        if (*src == '`') {
                                begin = src + 1;
                                end   = strchr(begin, '`');
                        } else {
                                begin = src + 2;
                                end   = find_closing(src + 1);
                        }

                        if (!end) {
                                printf("sh: missing '%c'\n", (*src == '`') ? '`' : ')');
                                goto error;
                        }

                        char *out = capture_output(begin, end - begin);
                        if (!out) {
                                goto error;
                        }

                        sb_append_value(&sb, out, assign or dquote);
                        free(out);
                        src = end + 1;

                } else if (*src == '$') {
                        const char *name = src + 1;
                        bool brace = (*name == '{');
                        if (brace) {
                                name++;
                        }

                        size_t len = 0;
                        while (is_var_name_char(name[len], len == 0)) {
                                len++;
                        }

                        if ((len == 0) or (brace and (name[len] != '}'))) {
                                /* not a variable reference, copy '$' as is */
                                sb_append(&sb, src++, 1);
                        } else {
                                var_t *var = var_find(name, len);
                                if (var) {
                                        sb_append_value(&sb, var->value, assign or dquote);
                                }
                                src = name + len + (brace ? 1 : 0);
                        }

                } else {
                        sb_append(&sb, src++, 1);
                }
        }

        if (sb.ok) {
                return sb.s;
        }

        puts("sh: not enough memory");

error:
        global->status = 1;
        free(sb.s);
        return NULL;
}

//==============================================================================
/**
 * @brief Handle variable related built-in commands:
 *        NAME=value, export [NAME=value], unset NAME..., set
 *
 * @param cmd           command line (modified)
 *
 * @return true if command was handled, otherwise false.
 */
//==============================================================================
static bool builtin_variables(char *cmd)
{
        if (is_assignment(cmd)) {
                assign_variable(cmd);
                return true;

        } else if (is_cmd(cmd, "set")) {
                var_list();
                global->status = 0;
                return true;

        } else if (is_cmd(cmd, "export")) {
                char *arg = find_arg(cmd);

                if (*arg == '\0') {
                        var_list();
                        global->status = 0;
                } else if (is_assignment(arg)) {
                        assign_variable(arg);
                } else {
                        printf("export: '%s': not a valid assignment\n", arg);
                        global->status = 1;
                }

                return true;

        } else if (is_cmd(cmd, "unset")) {
                char *arg = find_arg(cmd);

                while (*arg) {
                        char *next = find_arg(arg);
                        var_unset(arg);
                        arg = next;
                }

                global->status = 0;
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief Check if path is a directory.
 */
//==============================================================================
static bool path_is_dir(const char *path)
{
        DIR *dir = opendir(path);
        if (dir) {
                closedir(dir);
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief Check if path is a file (anything that is not a directory and can
 *        be opened for reading).
 */
//==============================================================================
static bool path_is_file(const char *path)
{
        if (path_is_dir(path)) {
                return false;
        }

        FILE *f = fopen(path, "r");
        if (f) {
                fclose(f);
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief Convert string to number.
 *
 * @return true if whole string is a number, otherwise false.
 */
//==============================================================================
static bool parse_long(const char *str, long *value)
{
        char *end = NULL;
        *value = strtol(str, &end, 0);
        return (*str != '\0') and (*end == '\0');
}

//==============================================================================
/**
 * @brief Evaluate test expression.
 *
 * @param argc          number of arguments
 * @param argv          arguments
 *
 * @return 0 if true, 1 if false, 2 if syntax error.
 */
//==============================================================================
static int test_eval(int argc, char **argv)
{
        if ((argc > 0) and isstreq(argv[0], "!")) {
                int r = test_eval(argc - 1, argv + 1);
                return (r == 2) ? 2 : !r;
        }

        if (argc == 0) {
                return 1;

        } else if (argc == 1) {
                return argv[0][0] ? 0 : 1;

        } else if (argc == 2) {
                const char *op = argv[0];
                const char *a  = argv[1];

                if (isstreq(op, "-z")) return a[0] ? 1 : 0;
                if (isstreq(op, "-n")) return a[0] ? 0 : 1;
                if (isstreq(op, "-d")) return path_is_dir(a) ? 0 : 1;
                if (isstreq(op, "-f")) return path_is_file(a) ? 0 : 1;
                if (isstreq(op, "-e")) return (path_is_dir(a) or path_is_file(a)) ? 0 : 1;

                return 2;

        } else if (argc == 3) {
                const char *a  = argv[0];
                const char *op = argv[1];
                const char *b  = argv[2];

                if (isstreq(op, "=") or isstreq(op, "==")) return isstreq(a, b) ? 0 : 1;
                if (isstreq(op, "!=")) return isstreq(a, b) ? 1 : 0;

                static const char *const NUM_OP[] = {"-eq", "-ne", "-lt", "-le", "-gt", "-ge"};

                for (size_t i = 0; i < ARRAY_SIZE(NUM_OP); i++) {
                        if (isstreq(op, NUM_OP[i])) {
                                long x, y;
                                if (not parse_long(a, &x) or not parse_long(b, &y)) {
                                        return 2;
                                }

                                bool r = false;
                                switch (i) {
                                case 0: r = (x == y); break;
                                case 1: r = (x != y); break;
                                case 2: r = (x <  y); break;
                                case 3: r = (x <= y); break;
                                case 4: r = (x >  y); break;
                                case 5: r = (x >= y); break;
                                }

                                return r ? 0 : 1;
                        }
                }

                return 2;
        }

        return 2;
}

//==============================================================================
/**
 * @brief Handle built-in commands: true, false, :, test ..., [ ... ]
 *
 * @param cmd           command line (modified)
 *
 * @return true if command was handled, otherwise false.
 */
//==============================================================================
static bool builtin_test(char *cmd)
{
        if (is_cmd(cmd, "true") or is_cmd(cmd, ":")) {
                global->status = 0;
                return true;

        } else if (is_cmd(cmd, "false")) {
                global->status = 1;
                return true;
        }

        bool bracket = is_cmd(cmd, "[");

        if (not bracket and not is_cmd(cmd, "test")) {
                return false;
        }

        char *argv[TEST_MAX_ARGS];
        int argc = 0;

        char *cur = cmd;
        next_token(&cur);       // skip command name

        char *tok;
        while ((tok = next_token(&cur))) {
                if (argc >= TEST_MAX_ARGS) {
                        puts("test: too many arguments");
                        global->status = 2;
                        return true;
                }

                argv[argc++] = tok;
        }

        if (bracket) {
                if ((argc == 0) or not isstreq(argv[argc - 1], "]")) {
                        puts("[: missing ']'");
                        global->status = 2;
                        return true;
                }

                argc--;
        }

        global->status = test_eval(argc, argv);

        if (global->status == 2) {
                puts("test: syntax error");
        }

        return true;
}

//==============================================================================
/**
 * @brief Parse sleep time in seconds: N or N.fff (e.g. 2, 0.5, 1.25).
 *        Fixed point arithmetic is used (no float required).
 *
 * @param str           time string
 * @param ms            result in milliseconds
 *
 * @return true on success, false if format is invalid.
 */
//==============================================================================
static bool parse_sleep_time(const char *str, u32_t *ms)
{
        u32_t whole = 0;
        u32_t frac  = 0;        /* fraction in 1/1000 s */
        u32_t scale = 100;
        bool digits = false;

        for (; isdigit(cast(unsigned char, *str)); str++) {
                whole  = whole * 10 + (*str - '0');
                digits = true;

                if (whole > 1000000) {
                        return false;
                }
        }

        if (*str == '.') {
                for (str++; isdigit(cast(unsigned char, *str)); str++) {
                        frac  += (*str - '0') * scale;
                        scale /= 10;
                        digits = true;
                }
        }

        if (not digits or (*str != '\0')) {
                return false;
        }

        *ms = whole * 1000 + frac;
        return true;
}

//==============================================================================
/**
 * @brief Built-in command: sleep SECONDS (fraction allowed, e.g. 0.5).
 *        Sleep can be interrupted by CTRL+C (exit code 130).
 *
 * @param cmd           command line (modified)
 *
 * @return true if command was handled, otherwise false.
 */
//==============================================================================
static bool builtin_sleep(char *cmd)
{
        if (not is_cmd(cmd, "sleep")) {
                return false;
        }

        char *cur = cmd;
        next_token(&cur);       // skip command name

        char *arg = next_token(&cur);
        u32_t ms  = 0;

        if (!arg or next_token(&cur) or not parse_sleep_time(arg, &ms)) {
                puts("Usage: sleep SECONDS (e.g. 2, 0.5)");
                global->status = 2;
                return true;
        }

        global->status = 0;

        while (ms > 0) {
                u32_t slice = min(ms, SLEEP_SLICE_MS);
                msleep(slice);
                ms -= slice;

                drain_stdin();

                if (global->interrupted) {
                        /* caller stops loop/script */
                        global->status = 130;
                        break;
                }
        }

        return true;
}

//==============================================================================
/**
 * @brief Function start program pipeline: cmd1 | cmd2 | ... | cmdN
 *        Exit code of the last program is stored in $?.
 *
 * @param cmds          programs (with arguments)
 * @param count         number of programs (>= 1)
 * @param std_in        input file of first program (can be NULL)
 * @param file          output file of last program (can be NULL)
 * @param fmode         output file mode
 * @param detached      detached operation (only single program)
 *
 * @return true if command executed, false if error
 */
//==============================================================================
static bool run_pipeline(char **cmds, size_t count, char *std_in, char *file,
                         const char *fmode, bool detached)
{
        errno = 0;
        bool status = true;
        FILE *fout = NULL;

        global->status = 1;

        stage_t *stage = calloc(count, sizeof(stage_t));
        if (!stage) {
                perror("sh");
                return true;
        }

        /*
         * Open output file. Detached program uses file path because the shell
         * closes its own file object immediately after program start.
         */
        if (file and not detached) {
                fout = fopen(file, fmode);
                if (!fout) {
                        perror(file);
                        goto free_resources;
                }

        } else if (file and detached) {
                /* file is opened by kernel without creation: create/truncate it now */
                fout = fopen(file, fmode);
                if (!fout) {
                        perror(file);
                        goto free_resources;
                }

                fclose(fout);
                fout = NULL;

        } else if (not file and not detached and global->capture_file) {
                /* command substitution $(...): output to temporary file */
                fout = fopen(global->capture_file, "a");
                if (!fout) {
                        perror("sh");
                        goto free_resources;
                }
        }

        /*
         * Create pipes between programs.
         */
        u64_t uptime = get_uptime_ms();

        for (size_t i = 0; i + 1 < count; i++) {
                snprintf(stage[i].pipe_name, PIPE_NAME_LEN, "%s%llx-%x",
                         global->pipe_file, uptime, cast(unsigned, i));

                if (mkfifo(stage[i].pipe_name, 0666)) {
                        stage[i].pipe_name[0] = '\0';
                        perror("sh");
                        goto free_resources;
                }

                stage[i].pipe = fopen(stage[i].pipe_name, "r+");
                if (!stage[i].pipe) {
                        perror("sh");
                        goto free_resources;
                }
        }

        /*
         * Start programs.
         */
        for (size_t i = 0; i < count; i++) {
                process_attr_t *attr = &stage[i].attr;

                attr->fd_stdin  = -1;
                attr->fd_stdout = -1;
                attr->fd_stderr = fileno(stderr);
                attr->p_stdin   = NULL;
                attr->p_stdout  = NULL;
                attr->p_stderr  = NULL;
                attr->cwd       = global->cwd;
                attr->detached  = detached;
                attr->priority  = PRIORITY_NORMAL;

                if (i > 0) {
                        attr->fd_stdin = fileno(stage[i - 1].pipe);
                } else if (std_in) {
                        attr->p_stdin = std_in;
                } else if (not detached) {
                        attr->fd_stdin = fileno(stdin);
                }

                if (i + 1 < count) {
                        attr->fd_stdout = fileno(stage[i].pipe);
                } else if (fout) {
                        attr->fd_stdout = fileno(fout);
                } else if (file) {
                        attr->p_stdout = file;
                        attr->p_stderr = file;
                        attr->fd_stderr = -1;
                } else {
                        attr->fd_stdout = fileno(stdout);
                }

                stage[i].pid = process_create(cmds[i], attr);
                if (stage[i].pid == 0) {
                        int err = errno;

                        for (size_t j = 0; j < i; j++) {
                                process_kill(stage[j].pid);
                        }

                        errno = err;
                        print_fail_message(cmds[i]);
                        global->status = 127;
                        goto free_resources;
                }
        }

        /*
         * Wait for programs in any order. When program finishes then pipe to
         * the next program is closed, so the reader gets EOF. When reader
         * finishes (e.g. 'more' closed by CTRL+C) then nobody reads the pipe,
         * so the writer is killed (SIGPIPE behavior) and the pipe is cleared
         * to release writer blocked on full pipe.
         */
        if (not detached) {
                size_t running = count;

                while (running > 0) {
                        for (size_t i = 0; i < count; i++) {
                                if (stage[i].done) {
                                        continue;
                                }

                                int exit_code = 0;
                                errno = 0;
                                if (process_wait(stage[i].pid, &exit_code, PIPELINE_POLL_MS) != 0) {
                                        if (errno == ETIME) {
                                                continue;
                                        }

                                        exit_code = 1;
                                }

                                stage[i].done = true;
                                running--;

                                if (stage[i].pipe) {
                                        ioctl(fileno(stage[i].pipe), IOCTL_PIPE__CLOSE);
                                }

                                if ((i > 0) and not stage[i - 1].done) {
                                        FILE *pipe = stage[i - 1].pipe;
                                        ioctl(fileno(pipe), IOCTL_PIPE__CLOSE);
                                        process_kill(stage[i - 1].pid);
                                        ioctl(fileno(pipe), IOCTL_PIPE__CLEAR);
                                }

                                if (i + 1 == count) {
                                        global->status = exit_code;
                                }
                        }
                }
        } else {
                global->status = 0;
        }

        // clear stdin (CTRL+C pressed during program execution is remembered)
        if (global->input == stdin) {
                drain_stdin();
        }

free_resources:
        if (fout) {
                fclose(fout);
        }

        for (size_t i = 0; i < count; i++) {
                if (stage[i].pipe) {
                        fclose(stage[i].pipe);
                }

                if (stage[i].pipe_name[0] != '\0') {
                        remove(stage[i].pipe_name);
                }
        }

        free(stage);

        return status;
}

//==============================================================================
/**
 * @brief Find character outside quotes.
 *
 * @param s             string
 * @param c             character to find
 *
 * @return Pointer to character or NULL.
 */
//==============================================================================
static char *find_unquoted(char *s, char c)
{
        char quote = '\0';

        for (; *s; s++) {
                if (quote) {
                        if (*s == quote) {
                                quote = '\0';
                        }
                } else if ((*s == '\'') or (*s == '"')) {
                        quote = *s;
                } else if (*s == c) {
                        return s;
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief Function analyze line
 *
 *        Syntax: cmd1 [< in] | cmd2 | ... | cmdN [> out | >> out] [&]
 *        Operators inside quotes are not interpreted.
 *        Input redirection is allowed only in the first program, output
 *        redirection only in the last one.
 *
 * @param cmd           command line
 *
 * @return true if command executed, false if error
 */
//==============================================================================
static bool analyze_line(char *cmd)
{
        bool detached = is_detached_cmd(cmd);
        char *std_in = NULL;
        char *file = NULL;
        const char *fmode = "w";

        if (strlen(trim_string(cmd)) == 0) {
                return false;
        }

        size_t count = 1;
        for (char *p = cmd; (p = find_unquoted(p, '|')); p++) {
                count++;
        }

        if (detached and (count > 1)) {
                puts("sh: detached pipeline is not supported");
                global->status = 2;
                return true;
        }

        char **cmds = calloc(count, sizeof(char *));
        if (!cmds) {
                perror("sh");
                global->status = 1;
                return true;
        }

        bool syntax_ok = true;

        /*
         * Split line to programs.
         */
        char *seg = cmd;
        for (size_t i = 0; i < count; i++) {
                cmds[i] = seg;

                char *bar = find_unquoted(seg, '|');
                if (bar) {
                        *bar = '\0';
                        seg = bar + 1;
                }
        }

        /*
         * Find redirections.
         */
        for (size_t i = 0; (i < count) and syntax_ok; i++) {
                char *in  = find_unquoted(cmds[i], '<');
                char *out = find_unquoted(cmds[i], '>');

                if (in) {
                        if ((i != 0) or find_unquoted(in + 1, '<')) {
                                syntax_ok = false;
                                break;
                        }
                }

                if (out) {
                        char *after = out + 1;
                        if (*after == '>') {
                                fmode = "a";
                                after++;
                        }

                        if ((i != count - 1) or find_unquoted(after, '>')) {
                                syntax_ok = false;
                                break;
                        }

                        *out = '\0';
                        file = trim_string(after);

                        if (strlen(file) == 0) {
                                syntax_ok = false;
                                break;
                        }
                }

                if (in) {
                        *in = '\0';
                        std_in = trim_string(in + 1);

                        if (strlen(std_in) == 0) {
                                syntax_ok = false;
                                break;
                        }
                }

                cmds[i] = trim_string(cmds[i]);

                if (strlen(cmds[i]) == 0) {
                        syntax_ok = false;
                }
        }

        bool status = true;

        if (syntax_ok) {
                status = run_pipeline(cmds, count, std_in, file, fmode, detached);
        } else {
                puts("sh: syntax error");
                global->status = 2;
        }

        free(cmds);

        return status;
}

//==============================================================================
/**
 * @brief Check if line starts compound command (if, while, for).
 */
//==============================================================================
static bool is_block_start(const char *line)
{
        return is_cmd(line, "if") or is_cmd(line, "while") or is_cmd(line, "for");
}

//==============================================================================
/**
 * @brief Check if line ends compound command (fi, done).
 */
//==============================================================================
static bool is_block_end(const char *line)
{
        return is_cmd(line, "fi") or is_cmd(line, "done");
}

//==============================================================================
/**
 * @brief Check if line is a keyword that can not be used as a command.
 */
//==============================================================================
static bool is_stray_keyword(const char *line)
{
        static const char *const KW[] = {
                "if", "then", "elif", "else", "fi", "while", "for", "do", "done"
        };

        for (size_t i = 0; i < ARRAY_SIZE(KW); i++) {
                if (is_cmd(line, KW[i])) {
                        return true;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief Return text after first word of line (keyword argument).
 */
//==============================================================================
static const char *keyword_arg(const char *line)
{
        const char *p = line + strcspn(line, " \t;");
        return p + strspn(p, " \t");
}

//==============================================================================
/**
 * @brief Skip line that contains only selected keyword (then, do).
 */
//==============================================================================
static size_t skip_kw(char **lines, size_t idx, size_t end, const char *kw)
{
        return ((idx < end) and isstreq(lines[idx], kw)) ? idx + 1 : idx;
}

//==============================================================================
/**
 * @brief Find line with one of selected keywords on the same nesting level.
 *
 * @param lines         lines
 * @param from          first line to check
 * @param to            end of range (exclusive)
 * @param keys          NULL terminated keyword list
 *
 * @return Line index or NOT_FOUND.
 */
//==============================================================================
static size_t find_next(char **lines, size_t from, size_t to, const char *const keys[])
{
        int depth = 0;

        for (size_t i = from; i < to; i++) {
                if (depth == 0) {
                        for (const char *const *k = keys; *k; k++) {
                                if (is_cmd(lines[i], *k)) {
                                        return i;
                                }
                        }
                }

                if (is_block_start(lines[i])) {
                        depth++;
                } else if (is_block_end(lines[i])) {
                        if (--depth < 0) {
                                break;
                        }
                }
        }

        return NOT_FOUND;
}

//==============================================================================
/**
 * @brief Evaluate condition of if/elif/while. Condition is any command,
 *        it is true when command exit code is 0. Leading '!' negates result.
 *
 * @param text          condition text
 *
 * @return 1 if true, 0 if false, -1 if exit requested, -2 on error.
 */
//==============================================================================
static int eval_cond(const char *text)
{
        char *buf = strdup(text);
        if (!buf) {
                perror("sh");
                return -2;
        }

        char *cond = trim_string(buf);

        bool negate = false;
        if ((cond[0] == '!') and ((cond[1] == '\0') or isspace(cast(int, cond[1])))) {
                negate = true;
                cond = trim_string(cond + 1);
        }

        int result;

        if (strlen(cond) == 0) {
                syntax_error("missing condition");
                result = -2;
        } else {
                flow_t flow = exec_simple(cond);

                if (flow == FLOW_EXIT) {
                        result = -1;
                } else if (flow == FLOW_ERROR) {
                        result = -2;
                } else {
                        result = ((global->status == 0) != negate) ? 1 : 0;
                }
        }

        free(buf);

        return result;
}

//==============================================================================
/**
 * @brief Execute if/elif/else/fi.
 *
 * @param lines         lines
 * @param start         'if' line index
 * @param end           'fi' line index
 */
//==============================================================================
static flow_t exec_if(char **lines, size_t start, size_t end)
{
        static const char *const KW_IF[] = {"elif", "else", "fi", NULL};

        size_t clause = start;

        for (;;) {
                size_t next = find_next(lines, clause + 1, end + 1, KW_IF);
                if (next == NOT_FOUND) {
                        return syntax_error("'fi' expected");
                }

                int cond;

                if (is_cmd(lines[clause], "else")) {
                        if (next != end) {
                                return syntax_error("'fi' expected after 'else'");
                        }
                        cond = 1;
                } else {
                        cond = eval_cond(keyword_arg(lines[clause]));
                        if (cond == -1) return FLOW_EXIT;
                        if (cond == -2) return FLOW_ERROR;
                }

                if (cond) {
                        return exec_range(lines, skip_kw(lines, clause + 1, next, "then"), next);
                }

                if (next == end) {
                        global->status = 0;
                        return FLOW_NEXT;
                }

                clause = next;
        }
}

//==============================================================================
/**
 * @brief Execute while/do/done.
 *
 * @param lines         lines
 * @param start         'while' line index
 * @param end           'done' line index
 */
//==============================================================================
static flow_t exec_while(char **lines, size_t start, size_t end)
{
        size_t body = skip_kw(lines, start + 1, end, "do");

        for (;;) {
                if (interrupt_requested()) {
                        return FLOW_INTERRUPT;
                }

                int cond = eval_cond(keyword_arg(lines[start]));
                if (cond == -1) return FLOW_EXIT;
                if (cond == -2) return FLOW_ERROR;
                if (cond == 0) break;

                flow_t flow = exec_range(lines, body, end);

                if (flow == FLOW_BREAK) {
                        break;
                } else if ((flow == FLOW_EXIT) or (flow == FLOW_ERROR) or (flow == FLOW_INTERRUPT)) {
                        return flow;
                }
        }

        global->status = 0;
        return FLOW_NEXT;
}

//==============================================================================
/**
 * @brief Execute for NAME in WORDS.../do/done.
 *
 * @param lines         lines
 * @param start         'for' line index
 * @param end           'done' line index
 */
//==============================================================================
static flow_t exec_for(char **lines, size_t start, size_t end)
{
        flow_t result = FLOW_NEXT;
        char *buf = strdup(keyword_arg(lines[start]));
        char *exp = NULL;

        if (!buf) {
                perror("sh");
                result = FLOW_ERROR;
                goto finish;
        }

        char *cur  = buf;
        char *name = next_token(&cur);
        char *in   = next_token(&cur);

        if (!name or !in or not is_var_name(name) or not isstreq(in, "in")) {
                result = syntax_error("expected 'for NAME in WORDS...'");
                goto finish;
        }

        exp = expand_variables(cur);
        if (!exp) {
                result = FLOW_ERROR;
                goto finish;
        }

        size_t body = skip_kw(lines, start + 1, end, "do");

        global->status = 0;

        char *word;
        cur = exp;
        while ((word = next_token(&cur))) {
                if (interrupt_requested()) {
                        result = FLOW_INTERRUPT;
                        break;
                }

                var_set(name, word);

                flow_t flow = exec_range(lines, body, end);

                if (flow == FLOW_BREAK) {
                        break;
                } else if ((flow == FLOW_EXIT) or (flow == FLOW_ERROR) or (flow == FLOW_INTERRUPT)) {
                        result = flow;
                        break;
                }
        }

finish:
        free(buf);
        free(exp);

        return result;
}

//==============================================================================
/**
 * @brief Execute range of lines (body of compound command).
 *
 * @param lines         lines
 * @param from          first line
 * @param to            end of range (exclusive)
 */
//==============================================================================
static flow_t exec_range(char **lines, size_t from, size_t to)
{
        static const char *const KW_FI[]   = {"fi", NULL};
        static const char *const KW_DONE[] = {"done", NULL};

        for (size_t i = from; i < to; i++) {
                const char *line = lines[i];
                flow_t flow;

                if (is_cmd(line, "if")) {
                        size_t end = find_next(lines, i + 1, to, KW_FI);
                        if (end == NOT_FOUND) {
                                return syntax_error("'fi' expected");
                        }

                        flow = exec_if(lines, i, end);
                        i = end;

                } else if (is_cmd(line, "while") or is_cmd(line, "for")) {
                        size_t end = find_next(lines, i + 1, to, KW_DONE);
                        if (end == NOT_FOUND) {
                                return syntax_error("'done' expected");
                        }

                        flow = is_cmd(line, "while") ? exec_while(lines, i, end)
                                                     : exec_for(lines, i, end);
                        i = end;

                } else {
                        flow = exec_simple(line);
                }

                if (flow != FLOW_NEXT) {
                        return flow;
                }
        }

        return FLOW_NEXT;
}

//==============================================================================
/**
 * @brief Execute single, already expanded command line.
 *
 * @param cmd           command line (modified)
 */
//==============================================================================
static flow_t exec_expanded(char *cmd)
{
        if (strlen(cmd) == 0) {
                return FLOW_NEXT;
        }

        if (is_cmd(cmd, "break")) {
                return FLOW_BREAK;
        }

        if (is_cmd(cmd, "continue")) {
                return FLOW_CONTINUE;
        }

        if (is_exit_cmd(cmd)) {
                return FLOW_EXIT;
        }

        if (is_stray_keyword(cmd)) {
                printf("sh: syntax error near '%s'\n", cmd);
                global->status = 2;
                return FLOW_ERROR;
        }

        if (builtin_variables(cmd)) {
                return FLOW_NEXT;
        }

        if (builtin_test(cmd)) {
                return FLOW_NEXT;
        }

        if (builtin_sleep(cmd)) {
                if (global->interrupted) {
                        global->interrupted = false;
                        puts("^C");
                        return FLOW_INTERRUPT;
                }

                return FLOW_NEXT;
        }

        if (is_cd_cmd(cmd)) {
                change_directory(cmd);
                return FLOW_NEXT;
        }

        if (isstreq(cmd, "..")) {
                char cd[] = "cd ..";
                change_directory(cd);
                return FLOW_NEXT;
        }

        if (is_clear_cmd(cmd)) {
                const char *clear = VT100_CLEAR_SCREEN;
                fwrite(clear, 1, strlen(clear), stdout);
                ioctl(fileno(stdout), IOCTL_TTY__CLEAR_SCR);
                global->status = 0;
                return FLOW_NEXT;
        }

        if (not analyze_line(cmd)) {
                print_fail_message(cmd);
                global->status = 127;
        }

        return FLOW_NEXT;
}

//==============================================================================
/**
 * @brief Execute single command line (no compound commands).
 *
 * @param raw           command line before variable expansion
 */
//==============================================================================
static flow_t exec_simple(const char *raw)
{
        global->captured = false;

        char *exp = expand_variables(raw);
        if (!exp) {
                return FLOW_NEXT;
        }

        flow_t flow = exec_expanded(trim_string(exp));

        free(exp);

        return flow;
}

//==============================================================================
/**
 * @brief Add line to block.
 */
//==============================================================================
static bool block_add(block_t *block, const char *line)
{
        if (block->count == block->size) {
                size_t size = block->size ? block->size * 2 : 8;

                char **lines = malloc(size * sizeof(char *));
                if (!lines) {
                        return false;
                }

                if (block->line) {
                        memcpy(lines, block->line, block->count * sizeof(char *));
                        free(block->line);
                }

                block->line = lines;
                block->size = size;
        }

        block->line[block->count] = strdup(line);
        if (!block->line[block->count]) {
                return false;
        }

        block->count++;
        return true;
}

//==============================================================================
/**
 * @brief Free block lines.
 */
//==============================================================================
static void block_free(block_t *block)
{
        for (size_t i = 0; i < block->count; i++) {
                free(block->line[i]);
        }

        free(block->line);
}

//==============================================================================
/**
 * @brief Add command to block and track nesting level. Lines like
 *        "then cmd", "do cmd", "else cmd" are split into keyword and command.
 *
 * @param block         block
 * @param cmd           command line (modified)
 * @param depth         nesting level (updated)
 */
//==============================================================================
static bool block_push(block_t *block, char *cmd, int *depth)
{
        static const char *const KW[] = {"then", "do", "else"};

        for (size_t i = 0; i < ARRAY_SIZE(KW); i++) {
                size_t len = strlen(KW[i]);

                if (is_cmd(cmd, KW[i]) and (cmd[len] != '\0')) {
                        cmd[len] = '\0';

                        if (not block_add(block, cmd)) {
                                return false;
                        }

                        cmd = trim_string(cmd + len + 1);
                        if (strlen(cmd) == 0) {
                                return true;
                        }

                        break;
                }
        }

        if (is_block_start(cmd)) {
                (*depth)++;
        } else if (is_block_end(cmd)) {
                (*depth)--;
        }

        return block_add(block, cmd);
}

//==============================================================================
/**
 * @brief Read lines of compound command (until matching fi/done) and execute.
 *
 * @param first         first line (if/while/for)
 */
//==============================================================================
static flow_t run_block(char *first)
{
        block_t block = {.line = NULL, .count = 0, .size = 0};
        flow_t flow = FLOW_ERROR;
        int depth = 0;
        bool ok = block_push(&block, first, &depth);

        global->continuation = true;

        while (ok and (depth > 0)) {
                char *cmd = read_command();
                if (!cmd) {
                        syntax_error("unexpected end of input");
                        break;
                }

                ok = block_push(&block, cmd, &depth);
        }

        global->continuation = false;

        if (not ok) {
                perror("sh");
        } else if (depth == 0) {
                flow = exec_range(block.line, 0, block.count);
        }

        block_free(&block);

        return flow;
}

//==============================================================================
/**
 * @brief Get next command from line with commands separated by ';'.
 *        Separators inside quotes, $(...) and `...` are ignored.
 *
 * @param cur           cursor (updated, NULL when no more commands)
 *
 * @return Command or NULL if no more commands.
 */
//==============================================================================
static char *next_segment(char **cur)
{
        char *seg = *cur;
        if (!seg) {
                return NULL;
        }

        char quote = '\0';
        bool btick = false;
        int depth = 0;
        char *p = seg;

        for (; *p; p++) {
                if (quote) {
                        if (*p == quote) {
                                quote = '\0';
                        }
                } else if ((*p == '\'') or (*p == '"')) {
                        quote = *p;
                } else if (*p == '`') {
                        btick = not btick;
                } else if (btick) {
                        continue;
                } else if (*p == '(') {
                        depth++;
                } else if ((*p == ')') and (depth > 0)) {
                        depth--;
                } else if ((*p == ';') and (depth == 0)) {
                        break;
                }
        }

        if (*p == ';') {
                *p = '\0';
                *cur = p + 1;
        } else {
                *cur = NULL;
        }

        return seg;
}

//==============================================================================
/**
 * @brief Read next command. Lines are split by ';' (outside quotes).
 *        Empty lines and comments are skipped.
 *
 * @return Command line or NULL on end of input.
 */
//==============================================================================
static char *read_command(void)
{
        for (;;) {
                if (global->next_seg) {
                        char *seg = trim_string(next_segment(&global->next_seg));
                        if (strlen(seg) == 0) {
                                continue;
                        }

                        return seg;
                }

                if (global->continuation and global->stream_closed) {
                        return NULL;
                }

                clear_prompt();

                print_prompt();

                if (!read_input()) {
                        if ((stdin == global->input) and not isstrempty(global->line)) {
                                memset(global->line, '\0', sizeof(global->line));
                                global->prompt_enable = false;
                                continue;
                        }

                        return NULL;
                }

                if (history_request()) {
                        continue;
                }

                if (command_hint()) {
                        continue;
                }

                char *cmd = trim_string(global->line);

                if ((strlen(cmd) == 0) or is_comment(cmd)) {
                        continue;
                }

                /* line is split into commands separated by ';' */
                global->next_seg = cmd;
        }
}

//==============================================================================
/**
 * @brief Terminal main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->prompt_enable = true;
        global->input = stdin;
        global->pipe_file = "/run/dsh-";

        /* working directory and pipe directory are needed also by -e option */
        mkdir("/run", 0666);

        getcwd(global->cwd, CWD_PATH_LEN);
        chdir(global->cwd);

        if (argc >= 2) {
                for (int i = 1; i < argc; i++) {
                        if (isstreq(argv[i], "-e")) {

                                memset(global->line, 0, sizeof(global->line));

                                for (int n = i + 1; n < argc; n++) {
                                        strlcat(global->line, argv[n], sizeof(global->line));
                                        strlcat(global->line, " ", sizeof(global->line));
                                }

                                exec_simple(trim_string(global->line));

                                int status = global->status;
                                var_free_all();
                                exit(status);
                        }
                }

                global->input = fopen(argv[1], "r");
                if (!global->input) {
                        perror(argv[1]);
                        return EXIT_FAILURE;
                }
        }

        for (;;) {
                char *cmd = read_command();
                if (!cmd) {
                        puts("Exit");
                        break;
                }

                /* script file can be stopped by CTRL+C between commands */
                if ((global->input != stdin) and interrupt_requested()) {
                        break;
                }

                global->interrupted = false;

                flow_t flow = is_block_start(cmd) ? run_block(cmd) : exec_simple(cmd);

                if (flow == FLOW_EXIT) {
                        break;
                }

                /* CTRL+C stops whole script, interactive shell continues */
                if ((flow == FLOW_INTERRUPT) and (global->input != stdin)) {
                        break;
                }
        }

        if (global->input != stdin) {
                fclose(global->input);
        }

        var_free_all();

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
