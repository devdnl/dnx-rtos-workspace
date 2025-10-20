/*==============================================================================
File    conf_parser.h

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
        OTHER DEALINGS IN THE SOFTWARE.>

==============================================================================*/

/**
@defgroup CONF_PARSER_H_ CONF_PARSER_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct conf_parser conf_parser_t;

typedef enum {
        conf_parser_status__success = 0,
        conf_parser_status__syntax_error = -1,
        conf_parser_status__no_memory = -2,
        conf_parser_status__file_error = -3,
        conf_parser_status__arg_error = -4,
        conf_parser_status__no_entry = -5,
        conf_parser_status__mutex_error = -6,
} conf_parser_status_t;

/*==============================================================================
  Exported objects
==============================================================================*/
extern conf_parser_t *conf_parser__new(const char *path);

extern conf_parser_t *conf_parser__delete(conf_parser_t *parser);

extern int conf_parser__set_file_path(conf_parser_t *parser, const char *path);

extern int conf_parser__load_file(conf_parser_t *parser, const char *path);

extern int conf_parser__load_buffer(conf_parser_t *parser, const char *buf, size_t bufsz);

extern int conf_parser__save(conf_parser_t *parser);

extern int conf_parser__reload(conf_parser_t *parser);

extern int conf_parser__save_file(conf_parser_t *parser, const char *path);

extern int conf_parser__save_buffer(conf_parser_t *parser, char *buf, size_t bufsz);

extern const char *conf_parser__get_value(conf_parser_t *parser, const char *key);

extern char *conf_parser__get_file_value(const char *path, const char *key);

extern int conf_parser__set_value(conf_parser_t *parser, const char *key, const char *value);

extern int conf_parser__set_value_vfv(conf_parser_t *parser, const char *key, const char *value_fmt,
        va_list args);

extern int conf_parser__set_value_vf(conf_parser_t *parser, const char *key, const char *value_fmt, ...);

extern int conf_parser__set_value_vparse(conf_parser_t *parser, const char *key_value_fmt, va_list args);

extern int conf_parser__set_value_parse(conf_parser_t *parser, const char *key_value_fmt, ...);

extern int conf_parser__set_file_value(const char *path, const char *key, const char *value);

extern int conf_parser__get_pair(conf_parser_t *parser, size_t i, const char **key, const char **val);

extern bool conf_parser__is_value_true(const char *value);

/*==============================================================================
  Exported functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
