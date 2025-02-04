/*==============================================================================
File    config_parser.c

Author  Daniel Zorychta

Brief   Configuration parser.

        Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

==============================================================================*/

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
#define LINE_LEN 128

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct pair {
        struct pair *next;
        bool         string;
        size_t       value_offset;
        char         data[];
} pair_t;

struct conf_parser {
        mutex_t *mutex;
        pair_t  *pair;
        char    *file_path;
        char    *current_section;
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
static int         interpret_line(conf_parser_t *this, char *line, pair_t **pair);
static pair_t     *pair_new(const char *key, const char *val);
static size_t      pair_get_section_length(const pair_t *pair);
static const char *pair_get_key(const pair_t *pair);
static const char *pair_get_value(const pair_t *pair);
static char       *skip_leading_spaces(char *str);
static void        remove_trailing_spaces(char *str);
static void        clear_conf(conf_parser_t *this);

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

                int err = interpret_line(this, line, pair);

                if (err == interpret_section) {
                        continue;

                } else if (err == interpret_pair) {
                        pair = &(*pair)->next;
                        n++;

                } else if (err == interpret_comment) {
                        continue;

                } else if (err == interpret_empty_line) {
                        continue;

                } else {
                        n = -1;
                        break;
                }
        }

        fclose(fp);

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
        if (not this and not buf and not bufsz) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        clear_conf(this);

        int n = 0;

        pair_t **pair = &this->pair;

        while ((*buf != '\0') and (bufsz > 0)) {
                size_t i = 0;
                char   line[LINE_LEN];

                while ((*buf != '\0') and (bufsz > 0) and (i < LINE_LEN - 1)) {
                        int c     = *buf;
                        line[i++] = *buf++;
                        bufsz--;

                        if (c == '\n') {
                                break;
                        }
                }

                line[i] = '\0';

                if (i > 0) {
                        int err = interpret_line(this, line, pair);

                        if (err == interpret_section) {
                                continue;

                        } else if (err == interpret_pair) {
                                pair = &(*pair)->next;
                                n++;

                        } else if (err == interpret_comment) {
                                continue;

                        } else if (err == interpret_empty_line) {
                                continue;

                        } else {
                                n = -1;
                                break;
                        }
                }
        }

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
                err             = conf_parser_status__no_memory;

        } else {
                free(old_path);
                err = conf_parser_status__success;
        }

        mutex_unlock(this->mutex);

        return err;
}

//==============================================================================
/**
 * @brief  Save configuration file.
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

        FILE *fp = fopen(path, "w");
        if (not fp) {
                mutex_unlock(this->mutex);
                return conf_parser_status__file_error;
        }

        int err = conf_parser_status__success;

        bool  next_line    = false;
        char *last_section = NULL;

        for (pair_t *pair = this->pair; pair; pair = pair->next) {

                const char *key   = pair_get_key(pair);
                const char *value = pair_get_value(pair);

                size_t section_len = pair_get_section_length(pair);
                if (section_len > 0) {
                        if ((last_section == NULL) or (strlen(last_section) != section_len) or
                                not isstreqn(last_section, pair_get_key(pair), section_len)) {

                                fprintf(fp, "%s[%.*s]\n", next_line ? "\n" : "", section_len, key);

                                free(last_section);
                                last_section = strndup(key, section_len);
                        }

                        key += section_len + 1;
                }

                int n = fprintf(fp, "%s = %s%s%s\n", key, pair->string ? "\"" : "", value,
                        pair->string ? "\"" : "");
                if (n <= 0) {
                        err = conf_parser_status__file_error;
                        break;
                }

                next_line = true;
        }

        free(last_section);

        fclose(fp);

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
 * @return Number of loaded parameters. On error one of conf_parser_status_t value.
 */
//==============================================================================
int conf_parser__save_buffer(conf_parser_t *this, char *buf, size_t bufsz)
{
        if (not this and not buf and not bufsz) {
                return conf_parser_status__arg_error;
        }

        size_t n = 0;

        if (mutex_lock(this->mutex, MAX_DELAY_MS)) {

                for (pair_t *pair = this->pair; pair; pair = pair->next) {
                        n += snprintf(&buf[n], bufsz - n, "%s = %s%s%s\n", pair_get_key(pair),
                                pair->string ? "\"" : "", pair_get_value(pair), pair->string ? "\"" : "");
                }

                mutex_unlock(this->mutex);
        }

        return n;
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
        if (not this or not key or not value) {
                return conf_parser_status__arg_error;
        }

        if (not mutex_lock(this->mutex, MAX_DELAY_MS)) {
                return conf_parser_status__mutex_error;
        }

        pair_t *pair_prev = NULL;

        for (pair_t *pair = this->pair; pair; pair = pair->next) {
                if (strcmp(pair_get_key(pair), key) == 0) {
                        pair_t *p = pair_new(key, value);

                        if (pair_prev) {
                                pair_prev->next = p;
                        } else {
                                this->pair = p;
                        }

                        p->next = pair->next;
                        free(pair);

                        mutex_unlock(this->mutex);
                        return conf_parser_status__success;
                }

                pair_prev = pair;
        }

        pair_t *new_pair = pair_new(key, value);

        if (pair_prev) {
                pair_prev->next = new_pair;
        } else {
                this->pair = new_pair;
        }

        mutex_unlock(this->mutex);

        return conf_parser_status__success;
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

        struct conf_parser parser = {0};
        int                err    = interpret_line(&parser, line, &parser.pair);
        if (err == interpret_pair) {
                err = conf_parser__set_value(this, parser.pair->data,
                        &parser.pair->data[parser.pair->value_offset]);
        } else {
                err = interpret_syntax_error;
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
                        conf_parser__save_file(p, path);
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

        size_t n   = 0;
        int    err = conf_parser_status__no_entry;

        for (pair_t *pair = this->pair; pair; pair = pair->next, n++) {
                if (n == i) {
                        *key   = pair_get_key(pair);
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
        static const char *truths[] = {"yes", "true", "1", "set", "on", "active", "high", "positive",
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

                free(this->current_section);
                this->current_section = strdup(key);
                if (not this->current_section) {
                        return interpret_no_memory;
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

                char first_char = FIRST_CHARACTER(val);
                char last_char  = LAST_CHARACTER(val);

                if (((first_char == '\"') and (last_char == '\"')) or
                        ((first_char == '\'') and (last_char == '\''))) {
                        val++;
                        LAST_CHARACTER(val) = '\0';
                        string              = true;
                }

                char section_key[LINE_LEN];
                if (this->current_section) {
                        snprintf(section_key, sizeof(section_key), "%s.%s", this->current_section, key);
                } else {
                        strlcpy(section_key, key, sizeof(section_key));
                }

                pair_t *p = pair_new(section_key, val);
                if (p) {
                        p->string = string;
                        *pair     = p;
                        return interpret_pair;
                } else {
                        return interpret_no_memory;
                }
        }
}

//==============================================================================
/**
 * @brief  Skip leading spaces.
 *
 * @param  str          string
 *
 * @return New string position or NULL if string finished.
 */
//==============================================================================
static char *skip_leading_spaces(char *str)
{
        while (isspace(*str)) {
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
        if (isstrempty(str)) {
                return;
        }

        str += strlen(str) - 1;

        while (isspace(*str)) {
                *str = '\0';
                str--;
        }
}

//==============================================================================
/**
 * @brief  Create new pair chain.
 *
 * @param  key          key
 * @param  val          value
 *
 * @return Pair object or NULL.
 */
//==============================================================================
static pair_t *pair_new(const char *key, const char *val)
{
        size_t key_sz = strsize(key);
        size_t val_sz = strsize(val);
        bool   string = false;

        for (int i = 0; i < 2; i++) {
                if (val[val_sz - 2] == '\n') {
                        val_sz--;
                }

                if (val[val_sz - 2] == '\r') {
                        val_sz--;
                }
        }

        if (((*val == '"') and (val[val_sz - 2] == '"')) or ((*val == '\'') or (val[val_sz - 2] == '\''))) {
                val++;
                val_sz -= 2;
                string = true;
        }

        pair_t *pair = calloc(1, sizeof(*pair) + key_sz + val_sz);
        if (pair) {
                pair->string       = string;
                pair->value_offset = key_sz;
                memcpy(&pair->data[0], key, key_sz);
                memcpy(&pair->data[pair->value_offset], val, val_sz);
                pair->data[pair->value_offset + val_sz - 1] = '\0';

                char *value = &pair->data[pair->value_offset];
                while (*value) {
                        if (not isprint(*value)) {
                                *value = '_';
                        }
                        value++;
                }
        }

        return pair;
}

//==============================================================================
/**
 * @brief  Get pair key section.
 *
 * @param  pair         pair
 *
 * @return Section name length.
 */
//==============================================================================
static size_t pair_get_section_length(const pair_t *pair)
{
        char *dot = strchr(pair->data, '.');
        if (dot and (dot != pair->data)) {
                return dot - pair->data;
        } else {
                return 0;
        }
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

/*==============================================================================
  End of file
==============================================================================*/
