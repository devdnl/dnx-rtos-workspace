/*==============================================================================
File    conf_parser.c

Author  Daniel Zorychta

Brief   Configuration parser.

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
#include "conf_parser.h"

#include <ctype.h>
#include <dnx/misc.h>
#include <dnx/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define LINE_LEN        128
#define TMP_FILE_SUFFIX ".tmp"

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct pair {
        struct pair *next;
        bool string;
        size_t section_len;
        size_t value_offset;
        char data[];
} pair_t;

typedef struct {
        FILE *fp;
        char *buf;
        size_t bufsz;
        size_t len;
        bool error;
} writer_t;

struct conf_parser {
        mutex_t *mutex;
        pair_t *pair;
        char *file_path;
        char *current_section;
};

enum {
        interpret_pair,
        interpret_section,
        interpret_comment,
        interpret_empty_line,
        interpret_no_memory,
        interpret_syntax_error,
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int interpret_line(conf_parser_t *this, char *line, pair_t **pair);
static int load_line(conf_parser_t *this, char *line, pair_t ***pair, int *n);
static int set_pair(conf_parser_t *this, const char *key, const char *val, size_t val_len, bool string);
static pair_t *pair_new(const char *section, size_t section_len, const char *key, const char *val,
        size_t val_len, bool string);
static bool pair_is_same_section(const pair_t *a, const pair_t *b);
static const char *pair_get_key(const pair_t *pair);
static const char *pair_get_value(const pair_t *pair);
static char *skip_leading_spaces(char *str);
static void remove_trailing_spaces(char *str);
static void clear_conf(conf_parser_t *this);
static void writer_printf(writer_t *w, const char *fmt, ...);
static void write_pair(writer_t *w, const pair_t *pair);
static void write_conf(conf_parser_t *this, writer_t *w);
static int write_file(conf_parser_t *this, const char *path, bool *opened);

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Create new pareser instance.
 *
 * @param  path         file to parse (can be NULL)
 *
 * @return Parser object.
 */
//==============================================================================
conf_parser_t *conf_parser__new(const char *path)
{
        conf_parser_t *this = calloc(1, sizeof(*this));
        if (not this) {
                return NULL;
        }

        this->mutex = mutex_new(MUTEX_TYPE_RECURSIVE);
        if (not this->mutex) {
                conf_parser__delete(this);
                return NULL;
        }

        if (not path) {
                return this;
        }

        this->file_path = strdup(path);
        if (not this->file_path) {
                conf_parser__delete(this);
                return NULL;
        }

        int n = conf_parser__load_file(this, path);
        if (n < 0) {
                conf_parser__delete(this);
                return NULL;
        }

        return this;
}

//==============================================================================
/**
 * @brief  Delete existing parser instance.
 *
 * @param  this         parser instance
 *
 * @param  Return NULL.
 */
//==============================================================================
conf_parser_t *conf_parser__delete(conf_parser_t *this)
{
        if (this) {
                if (mutex_lock(this->mutex, MAX_DELAY_MS)) {
                        clear_conf(this);
                        free(this->file_path);
                        mutex_unlock(this->mutex);
                }

                mutex_delete(this->mutex);

                free(this);
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Clear configuration.
 *
 * @param  this         parser instance
 *
 * @param  Return NULL.
 */
//==============================================================================
int conf_parser__clear(conf_parser_t *this)
{
        if (this) {
                if (mutex_lock(this->mutex, MAX_DELAY_MS)) {
                        clear_conf(this);
                        mutex_unlock(this->mutex);
                        return ESUCC;
                }
        }

        return EINVAL;
}

//==============================================================================
/**
 * @brief  Load selected configuration file.
 *
 * @param  this         parser instance
 * @param  path         file path
 *
 * @return Number of loaded parameters. On error one of conf_parser_status_t value.
 */
//==============================================================================
int conf_parser__load_file(conf_parser_t *this, const char *path)
{
        if (not this or not path) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        free(this->file_path);
        this->file_path = strdup(path);
        if (not this->file_path) {
                mutex_unlock(this->mutex);
                return conf_parser_status__no_memory;
        }

        FILE *fp = fopen(path, "r");
        if (not fp) {
                mutex_unlock(this->mutex);
                return conf_parser_status__file_error;
        }

        clear_conf(this);

        int n = 0;

        pair_t **pair = &this->pair;

        char line[LINE_LEN];

        while (fgets(line, sizeof(line), fp)) {

                // line longer than buffer cannot be interpreted correctly
                if (not strchr(line, '\n') and not feof(fp)) {
                        n = conf_parser_status__syntax_error;
                        break;
                }

                int err = load_line(this, line, &pair, &n);
                if (err) {
                        n = err;
                        break;
                }
        }

        fclose(fp);

        free(this->current_section);
        this->current_section = NULL;

        mutex_unlock(this->mutex);

        return n;
}

//==============================================================================
/**
 * @brief  Load configuration from buffer or string
 *
 * @param  this         parser instance
 * @param  buf          buffer
 * @param  bufsz        buffer size
 *
 * @return Number of loaded parameters. On error one of conf_parser_status_t value.
 */
//==============================================================================
int conf_parser__load_buffer(conf_parser_t *this, const char *buf, size_t bufsz)
{
        if (not this or not buf) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        clear_conf(this);

        int n = 0;

        pair_t **pair = &this->pair;

        while ((bufsz > 0) and (*buf != '\0')) {
                size_t i = 0;
                char line[LINE_LEN];
                bool eol = false;

                while ((bufsz > 0) and (*buf != '\0') and (i < LINE_LEN - 1)) {
                        int c = *buf;
                        line[i++] = *buf++;
                        bufsz--;

                        if (c == '\n') {
                                eol = true;
                                break;
                        }
                }

                line[i] = '\0';

                // line longer than buffer cannot be interpreted correctly
                if (not eol and (bufsz > 0) and (*buf != '\0')) {
                        n = conf_parser_status__syntax_error;
                        break;
                }

                int err = load_line(this, line, &pair, &n);
                if (err) {
                        n = err;
                        break;
                }
        }

        free(this->current_section);
        this->current_section = NULL;

        mutex_unlock(this->mutex);

        return n;
}

//==============================================================================
/**
 * @brief  Set/replace file path.
 *
 * @param  this         parser instance
 * @param  path         new path
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_file_path(conf_parser_t *this, const char *path)
{
        if (not this or not path) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        char *old_path = this->file_path;

        int err = conf_parser_status__success;

        this->file_path = strdup(path);
        if (not this->file_path) {
                this->file_path = old_path;
                err = conf_parser_status__no_memory;

        } else {
                free(old_path);
                err = conf_parser_status__success;
        }

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Save configuration file. Configuration is written to temporary file
 *         <path>.tmp first and then renamed to the target file, so the target
 *         file is not damaged if writing fails (e.g. no space, power loss).
 *         If file system does not support temporary file or rename, the file
 *         is written directly.
 *
 * @param  this         parser instance
 * @param  path         file to save
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__save_file(conf_parser_t *this, const char *path)
{
        if (not this or not path) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        int err = conf_parser_status__file_error;
        bool direct = true;

        char *tmp_path = malloc(strlen(path) + sizeof(TMP_FILE_SUFFIX));
        if (tmp_path) {
                strcpy(tmp_path, path);
                strcat(tmp_path, TMP_FILE_SUFFIX);

                bool opened = false;
                err = write_file(this, tmp_path, &opened);

                if (not opened) {
                        // temporary file cannot be created (e.g. name too long)
                        direct = true;

                } else if (err) {
                        // writing failed (e.g. no space), target file stays untouched
                        direct = false;
                        remove(tmp_path);

                } else if (rename(tmp_path, path) == 0) {
                        direct = false;

                } else if (errno == ENOSYS) {
                        // rename not supported by system
                        direct = true;
                        remove(tmp_path);

                } else {
                        // file system does not replace existing file by rename
                        remove(path);

                        if (rename(tmp_path, path) == 0) {
                                direct = false;
                        } else {
                                direct = true;
                                remove(tmp_path);
                        }
                }

                free(tmp_path);
        }

        if (direct) {
                bool opened = false;
                err = write_file(this, path, &opened);
        }

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Save configuration.
 *
 * @param  this         parser instance
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__save(conf_parser_t *this)
{
        if (not this) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        int err = conf_parser_status__success;

        if (this->file_path) {
                err = conf_parser__save_file(this, this->file_path);
        }

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Reload configuration.
 *
 * @param  this         parser instance
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__reload(conf_parser_t *this)
{
        if (not this) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        if (not this->file_path) {
                mutex_unlock(this->mutex);
                return conf_parser_status__success;
        }

        char *path = strdup(this->file_path);
        if (not path) {
                mutex_unlock(this->mutex);
                return conf_parser_status__no_memory;
        }

        int err = conf_parser__load_file(this, path);
        free(path);

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Save configuration to buffer.
 *
 * @param  this         parser instance
 * @param  buf          buffer
 * @param  bufsz        buffer size
 *
 * @return Number of written bytes.
 */
//==============================================================================
int conf_parser__save_buffer(conf_parser_t *this, char *buf, size_t bufsz)
{
        if (not this or (not buf and bufsz)) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        writer_t w = {.buf = buf, .bufsz = bufsz};
        write_conf(this, &w);

        mutex_unlock(this->mutex);

        if (w.error) {
                return conf_parser_status__arg_error;
        }

        // output truncated to buffer size
        if (w.len >= bufsz) {
                w.len = (bufsz > 0) ? bufsz - 1 : 0;
        }

        return w.len;
}

//==============================================================================
/**
 * @brief  Get value by key.
 *
 * @param  this         parser instance
 * @param  key          key
 *
 * @return Reference to value. NULL if key not found.
 */
//==============================================================================
const char *conf_parser__get_value(conf_parser_t *this, const char *key)
{
        if (not this or not key) {
                return NULL;
        }

        const char *value = NULL;

        if (mutex_lock(this->mutex, MAX_DELAY_MS)) {

                for (pair_t *pair = this->pair; pair; pair = pair->next) {
                        if (strcmp(pair_get_key(pair), key) == 0) {
                                value = pair_get_value(pair);
                                break;
                        }
                }

                mutex_unlock(this->mutex);
        }

        return value;
}

//==============================================================================
/**
 * @brief  Get value by key of selected file (directly).
 *
 * @param  path         file path
 * @param  key          key
 *
 * @return Reference to value. NULL if key not found. Returned pointer should
 *         be freed by using free() function.
 */
//==============================================================================
char *conf_parser__get_file_value(const char *path, const char *key)
{
        char *value = NULL;

        auto_type p = conf_parser__new(path);
        if (p) {
                const char *v = conf_parser__get_value(p, key);

                if (v) {
                        value = strdup(v);
                }

                conf_parser__delete(p);
        }

        return value;
}

//==============================================================================
/**
 * @brief  Set selected value. If value exists then is updated, otherwise new one
 *         is created.
 *
 * @param  this         parser instance
 * @param  key          key
 * @param  value        value
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_value(conf_parser_t *this, const char *key, const char *value)
{
        if (not this or isstrempty(key) or not value) {
                return conf_parser_status__arg_error;
        }

        size_t val_len = strlen(value);

        while ((val_len > 0) and ((value[val_len - 1] == '\n') or (value[val_len - 1] == '\r'))) {
                val_len--;
        }

        bool string = false;

        if ((val_len >= 2) and
                (((value[0] == '"') and (value[val_len - 1] == '"')) or
                        ((value[0] == '\'') and (value[val_len - 1] == '\'')))) {
                value++;
                val_len -= 2;
                string = true;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        int err = set_pair(this, key, value, val_len, string);

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Set selected value. If value exists then is updated, otherwise new one
 *         is created. Value is formatted according to value_fmt.
 *
 * @param  this         parser instance
 * @param  key          key
 * @param  value_fmt    value format
 * @param  args         value format arguments
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_value_vfv(conf_parser_t *this, const char *key, const char *value_fmt, va_list args)
{
        char value[LINE_LEN];
        vsnprintf(value, sizeof(value), value_fmt, args);
        return conf_parser__set_value(this, key, value);
}

//==============================================================================
/**
 * @brief  Set selected value. If value exists then is updated, otherwise new one
 *         is created. Value is formatted according to value_fmt.
 *
 * @param  this         parser instance
 * @param  key          key
 * @param  value_fmt    value format
 * @param  ...          value format arguments
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_value_vf(conf_parser_t *this, const char *key, const char *value_fmt, ...)
{
        va_list args;
        va_start(args, value_fmt);
        int err = conf_parser__set_value_vfv(this, key, value_fmt, args);
        va_end(args);
        return err;
}

//==============================================================================
/**
 * @brief  Set selected value. If value exists then is updated, otherwise new one
 *         is created. Key and value are formatted according to key_value_fmt and
 *         args. That pair is then parsed.
 *
 * @param  this                 parser instance
 * @param  key_value_fmt        key value format (key = value)
 * @param  args                 value format arguments
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_value_vparse(conf_parser_t *this, const char *key_value_fmt, va_list args)
{
        char line[LINE_LEN];
        vsnprintf(line, sizeof(line), key_value_fmt, args);

        if (not this) {
                return conf_parser_status__arg_error;
        }

        struct conf_parser parser = {0};
        int err = interpret_line(&parser, line, &parser.pair);
        if (err == interpret_pair) {
                if (mutex_lock(this->mutex, MAX_DELAY_MS)) {
                        const char *value = pair_get_value(parser.pair);
                        err = set_pair(this, pair_get_key(parser.pair), value, strlen(value),
                                parser.pair->string);
                        mutex_unlock(this->mutex);
                } else {
                        err = conf_parser_status__mutex_error;
                }

        } else if (err == interpret_no_memory) {
                err = conf_parser_status__no_memory;

        } else {
                err = conf_parser_status__syntax_error;
        }

        clear_conf(&parser);

        return err;
}

//==============================================================================
/**
 * @brief  Set selected value. If value exists then is updated, otherwise new one
 *         is created. Key and value are formatted according to key_value_fmt and
 *         args. That pair is then parsed.
 *
 * @param  this                 parser instance
 * @param  key_value_fmt        key value format (key = value)
 * @param  ...                  value format arguments
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_value_parse(conf_parser_t *this, const char *key_value_fmt, ...)
{
        va_list args;
        va_start(args, key_value_fmt);
        int err = conf_parser__set_value_vparse(this, key_value_fmt, args);
        va_end(args);
        return err;
}

//==============================================================================
/**
 * @brief  Set specified value in file.
 *
 * @param  path         file
 * @param  key          key
 * @param  value        value
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__set_file_value(const char *path, const char *key, const char *value)
{
        int err = conf_parser_status__file_error;

        auto_type p = conf_parser__new(path);
        if (not p) {
                errno = 0;
                stat(path, &(struct stat) {});

                if (errno == ENOENT) {
                        p = conf_parser__new(NULL);
                        if (not p) {
                                return conf_parser_status__no_memory;
                        }
                } else {
                        return err;
                }
        }

        if (p) {
                err = conf_parser__set_value(p, key, value);

                if (not err) {
                        err = conf_parser__save_file(p, path);
                }

                conf_parser__delete(p);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Get key-value pair by index.
 *
 * @param  this         parser instance
 * @param  i            iterator
 * @param  key          pointer to key
 * @param  value        pointer to value
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
int conf_parser__get_pair(conf_parser_t *this, size_t i, const char **key, const char **value)
{
        if (not this or not key or not value) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        size_t n = 0;
        int err = conf_parser_status__no_entry;

        for (pair_t *pair = this->pair; pair; pair = pair->next, n++) {
                if (n == i) {
                        *key = pair_get_key(pair);
                        *value = pair_get_value(pair);

                        err = conf_parser_status__success;
                        break;
                }
        }

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Function check boolean value.
 *
 * @param  value        value to check
 *
 * @return True if one of following values exists: "yes", "true", "1", "set",
 *         "on", "active", "high", "positive", "enable" otherwise false. Detection is case
 *         insensitive.
 */
//==============================================================================
bool conf_parser__is_value_true(const char *value)
{
        static const char *const truths[] = {"yes", "true", "1", "set", "on", "active", "high", "positive",
                "enable"};

        if (value) {
                for (size_t i = 0; i < ARRAY_SIZE(truths); i++) {
                        if (strcasecmp(value, truths[i]) == 0) {
                                return true;
                        }
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief  Parse configration line.
 *
 * @param  this         library instance
 * @param  line         line to parse
 * @param  pair         creted pair
 *
 * @return Interpretation status.
 */
//==============================================================================
static int interpret_line(conf_parser_t *this, char *line, pair_t **pair)
{
        line = skip_leading_spaces(line);
        if ((*line == '#') or (*line == ';')) {
                return interpret_comment;
        }

        char *key = line;

        if (*key == '[') {
                key++;

                remove_trailing_spaces(key);

                char *end = strrchr(key, ']');
                if (not end) {
                        return interpret_syntax_error;
                }

                *end = '\0';

                key = skip_leading_spaces(key);
                remove_trailing_spaces(key);

                free(this->current_section);
                this->current_section = NULL;

                // empty section [] means keys without section
                if (not isstrempty(key)) {
                        this->current_section = strdup(key);
                        if (not this->current_section) {
                                return interpret_no_memory;
                        }
                }

                return interpret_section;

        } else {
                bool string = false;

                if (isstrempty(key)) {
                        return interpret_empty_line;
                }

                line = strchr(line, '=');
                if (not line) {
                        return interpret_syntax_error;
                }
                if (*line != '=') {
                        return interpret_syntax_error;
                }
                *line++ = '\0';

                char *val = skip_leading_spaces(line);

                remove_trailing_spaces(key);
                remove_trailing_spaces(val);

                if (isstrempty(key)) {
                        return interpret_syntax_error;
                }

                size_t val_len = strlen(val);

                if ((val_len >= 2) and
                        (((val[0] == '\"') and (val[val_len - 1] == '\"')) or
                                ((val[0] == '\'') and (val[val_len - 1] == '\'')))) {
                        val++;
                        val_len -= 2;
                        string = true;
                }

                const char *section = this->current_section;
                size_t section_len = section ? strlen(section) : 0;

                pair_t *p = pair_new(section, section_len, key, val, val_len, string);
                if (p) {
                        *pair = p;
                        return interpret_pair;
                } else {
                        return interpret_no_memory;
                }
        }
}

//==============================================================================
/**
 * @brief  Interpret loaded line and append created pair to the list.
 *
 * @param  this         parser instance
 * @param  line         line to interpret
 * @param  pair         pointer to list link where new pair is stored
 * @param  n            number of loaded pairs (incremented)
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
static int load_line(conf_parser_t *this, char *line, pair_t ***pair, int *n)
{
        switch (interpret_line(this, line, *pair)) {
        case interpret_pair:
                *pair = &(**pair)->next;
                (*n)++;
                return conf_parser_status__success;

        case interpret_section:
        case interpret_comment:
        case interpret_empty_line:
                return conf_parser_status__success;

        case interpret_no_memory:
                return conf_parser_status__no_memory;

        default:
                return conf_parser_status__syntax_error;
        }
}

//==============================================================================
/**
 * @brief  Set or create pair. New pair is placed at the end of own section to
 *         keep sections continuous. Function must be called with locked mutex.
 *
 * @param  this         parser instance
 * @param  key          key (section.key)
 * @param  val          value (not terminated)
 * @param  val_len      value length
 * @param  string       value is a string (saved in quotes)
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
static int set_pair(conf_parser_t *this, const char *key, const char *val, size_t val_len, bool string)
{
        pair_t **link = &this->pair;

        while (*link and (strcmp(pair_get_key(*link), key) != 0)) {
                link = &(*link)->next;
        }

        pair_t *old = *link;
        size_t section_len = 0;

        if (old) {
                section_len = old->section_len;
                string = string or old->string;

        } else {
                // the longest existing section that matches key prefix
                for (pair_t *p = this->pair; p; p = p->next) {
                        if ((p->section_len > section_len) and isstreqn(key, p->data, p->section_len) and
                                (key[p->section_len] == '.')) {
                                section_len = p->section_len;
                        }
                }

                // new section
                if (section_len == 0) {
                        const char *dot = strchr(key, '.');
                        if (dot and (dot != key) and (dot[1] != '\0')) {
                                section_len = dot - key;
                        }
                }
        }

        const char *name = section_len ? &key[section_len + 1] : key;

        // line that does not fit to the line buffer cannot be loaded back
        if ((section_len + strlen("[]\n") > LINE_LEN - 1) or
                (strlen(name) + val_len + strlen(" = \"\"\n") > LINE_LEN - 1)) {
                return conf_parser_status__arg_error;
        }

        pair_t *pair = pair_new(key, section_len, name, val, val_len, string);
        if (not pair) {
                return conf_parser_status__no_memory;
        }

        if (old) {
                pair->next = old->next;
                *link = pair;
                free(old);
                return conf_parser_status__success;
        }

        // insert after last pair of the same section (pairs without section are
        // inserted before all sections, new section is added at the end)
        pair_t **insert = NULL;

        for (link = &this->pair; *link; link = &(*link)->next) {
                if (pair_is_same_section(*link, pair)) {
                        insert = &(*link)->next;
                }
        }

        if (not insert) {
                insert = (section_len == 0) ? &this->pair : link;
        }

        pair->next = *insert;
        *insert = pair;

        return conf_parser_status__success;
}

//==============================================================================
/**
 * @brief  Skip leading spaces.
 *
 * @param  str          string
 *
 * @return New string position.
 */
//==============================================================================
static char *skip_leading_spaces(char *str)
{
        while (isspace((unsigned char)*str)) {
                str++;
        }

        return str;
}

//==============================================================================
/**
 * @brief  Remove trailing spaces in string.
 *
 * @param  str          string to modify
 */
//==============================================================================
static void remove_trailing_spaces(char *str)
{
        size_t len = strlen(str);

        while ((len > 0) and isspace((unsigned char)str[len - 1])) {
                str[--len] = '\0';
        }
}

//==============================================================================
/**
 * @brief  Create new pair. Key is stored as "section.key".
 *
 * @param  section      section name (can be NULL if section_len is 0)
 * @param  section_len  section name length (0 if pair has no section)
 * @param  key          key
 * @param  val          value (not terminated)
 * @param  val_len      value length
 * @param  string       value is a string
 *
 * @return Pair object or NULL.
 */
//==============================================================================
static pair_t *pair_new(const char *section, size_t section_len, const char *key, const char *val,
        size_t val_len, bool string)
{
        size_t key_len = strlen(key);
        size_t full_key_len = section_len ? (section_len + 1 + key_len) : key_len;

        pair_t *pair = calloc(1, sizeof(*pair) + full_key_len + 1 + val_len + 1);
        if (pair) {
                pair->string = string;
                pair->section_len = section_len;
                pair->value_offset = full_key_len + 1;

                char *dst = &pair->data[0];

                if (section_len) {
                        memcpy(dst, section, section_len);
                        dst[section_len] = '.';
                        dst += section_len + 1;
                }

                memcpy(dst, key, key_len);

                char *value = &pair->data[pair->value_offset];
                memcpy(value, val, val_len);

                for (size_t i = 0; i < val_len; i++) {
                        if ((unsigned char)value[i] < ' ') {
                                value[i] = '_';
                        }
                }
        }

        return pair;
}

//==============================================================================
/**
 * @brief  Check if pairs belong to the same section (or both have no section).
 *
 * @param  a            pair a
 * @param  b            pair b
 *
 * @return True if the same section.
 */
//==============================================================================
static bool pair_is_same_section(const pair_t *a, const pair_t *b)
{
        return (a->section_len == b->section_len) and isstreqn(a->data, b->data, a->section_len);
}

//==============================================================================
/**
 * @brief  Get pair key.
 *
 * @param  pair         pair
 *
 * @return Key.
 */
//==============================================================================
static const char *pair_get_key(const pair_t *pair)
{
        return &pair->data[0];
}

//==============================================================================
/**
 * @brief  Get pair value.
 *
 * @param  pair         pair
 *
 * @return Value.
 */
//==============================================================================
static const char *pair_get_value(const pair_t *pair)
{
        return &pair->data[pair->value_offset];
}

//==============================================================================
/**
 * @brief  Clear configuration.
 *
 * @param  this         parser instance
 */
//==============================================================================
static void clear_conf(conf_parser_t *this)
{
        pair_t *pair = this->pair;

        while (pair) {
                pair_t *next = pair->next;
                free(pair);
                pair = next;
        }

        this->pair = NULL;

        free(this->current_section);
        this->current_section = NULL;
}

//==============================================================================
/**
 * @brief  Write formatted text to file or buffer.
 *
 * @param  w            writer
 * @param  fmt          format
 * @param  ...          format arguments
 */
//==============================================================================
static void writer_printf(writer_t *w, const char *fmt, ...)
{
        if (w->error) {
                return;
        }

        va_list args;
        va_start(args, fmt);

        if (w->fp) {
                if (vfprintf(w->fp, fmt, args) < 0) {
                        w->error = true;
                }

        } else {
                size_t free_sz = (w->len < w->bufsz) ? (w->bufsz - w->len) : 0;

                int n = vsnprintf(free_sz ? &w->buf[w->len] : NULL, free_sz, fmt, args);
                if (n < 0) {
                        w->error = true;
                } else {
                        // vsnprintf() returns the length of not truncated output
                        w->len += n;
                }
        }

        va_end(args);
}

//==============================================================================
/**
 * @brief  Write single pair (key without section).
 *
 * @param  w            writer
 * @param  pair         pair to write
 */
//==============================================================================
static void write_pair(writer_t *w, const pair_t *pair)
{
        const char *key = pair_get_key(pair);
        const char *value = pair_get_value(pair);

        if (pair->section_len) {
                key += pair->section_len + 1;
        }

        // value that would be changed by the parser must be written in quotes
        bool quote = pair->string;
        size_t len = strlen(value);

        if (not quote and (len > 0)) {
                char first = value[0];
                char last = value[len - 1];

                quote = isspace((unsigned char)first) or isspace((unsigned char)last) or
                        ((len >= 2) and
                                (((first == '"') and (last == '"')) or ((first == '\'') and (last == '\''))));
        }

        writer_printf(w, "%s = %s%s%s\n", key, quote ? "\"" : "", value, quote ? "\"" : "");
}

//==============================================================================
/**
 * @brief  Write whole configuration. Pairs without section are written first,
 *         then all pairs grouped by section (each section is written once).
 *
 * @param  this         parser instance
 * @param  w            writer
 */
//==============================================================================
static void write_conf(conf_parser_t *this, writer_t *w)
{
        bool empty = true;

        for (pair_t *pair = this->pair; pair; pair = pair->next) {
                if (pair->section_len == 0) {
                        write_pair(w, pair);
                        empty = false;
                }
        }

        for (pair_t *pair = this->pair; pair; pair = pair->next) {

                if (pair->section_len == 0) {
                        continue;
                }

                bool written = false;

                for (pair_t *p = this->pair; p != pair; p = p->next) {
                        if (pair_is_same_section(p, pair)) {
                                written = true;
                                break;
                        }
                }

                if (written) {
                        continue;
                }

                writer_printf(w, "%s[%.*s]\n", empty ? "" : "\n", (int)pair->section_len, pair->data);
                empty = false;

                for (pair_t *p = pair; p; p = p->next) {
                        if (pair_is_same_section(p, pair)) {
                                write_pair(w, p);
                        }
                }
        }
}

//==============================================================================
/**
 * @brief  Write whole configuration to selected file.
 *
 * @param  this         parser instance
 * @param  path         file path
 * @param  opened       true if file was opened
 *
 * @return One of conf_parser_status_t.
 */
//==============================================================================
static int write_file(conf_parser_t *this, const char *path, bool *opened)
{
        FILE *fp = fopen(path, "w");
        *opened = (fp != NULL);

        if (not fp) {
                return conf_parser_status__file_error;
        }

        writer_t w = {.fp = fp};
        write_conf(this, &w);

        int err = w.error ? conf_parser_status__file_error : conf_parser_status__success;

        if (fclose(fp) != 0) {
                err = conf_parser_status__file_error;
        }

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
