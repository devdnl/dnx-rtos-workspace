/*==============================================================================
@file   httpsrv.h

@author Daniel Zorychta

@brief  The HTTP server with script support.

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

/**
@defgroup HTTPSRV_H_ HTTPSRV_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <ctype.h>
#include <dirent.h>
#include <dnx/misc.h>
#include <dnx/net.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <errno.h>
#include <http_parser.h>
#include <multipartparser.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <conf_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#undef global
#define global cast(struct httpsrv_globals *, _getglobal)

#define DEBUG(...)                               \
        if (global->debug) {                     \
                printf("httpsrv: " __VA_ARGS__); \
        }

#define VERBOSE(...)                             printf("[httpsrv, info] " __VA_ARGS__)

#define HTTPSRV_SCRIPT_BEGIN_TOKEN               "<?lua"
#define HTTPSRV_SCRIPT_EXTENSION                 ".lua"

#define conf_get_value(_key)                     conf_parser__get_value(global->conf, _key)
#define conf_get_debug()                         conf_parser__is_value_true(conf_get_value("debug"))
#define conf_get_threads_num()                   strtoul(conf_get_value("threads-num"), NULL, 0)
#define conf_get_port()                          strtoul(conf_get_value("port"), NULL, 0)
#define conf_get_root_dir()                      conf_get_value("root-dir")
#define conf_get_upload_dir()                    conf_get_value("upload-dir")
#define conf_get_page_default()                  conf_get_value("page-default")
#define conf_get_page_not_found()                conf_get_value("page-not-found")
#define conf_get_multipart_buffer_size()         strtoul(conf_get_value("buffer.multipart"), NULL, 0)
#define conf_get_interpreter_fetch_buffer_size() strtoul(conf_get_value("buffer.interpreter-fetch"), NULL, 0)
#define conf_get_interpreter_code_buffer_size()  strtoul(conf_get_value("buffer.interpreter-code"), NULL, 0)
#define conf_get_interpreter_tmp_buffer_size()   strtoul(conf_get_value("buffer.interpreter-tmp"), NULL, 0)
#define conf_get_output_buffer_size()            strtoul(conf_get_value("buffer.output"), NULL, 0)
#define conf_get_http_parser_buffer_size()       strtoul(conf_get_value("buffer.http-parser"), NULL, 0)

/*==============================================================================
  Exported object types
==============================================================================*/
typedef enum {
        content_type__text = 0,
        content_type__multipart_form_data,
} httpsrv_content_type_t;

typedef struct {
        char *value;
        size_t value_length;
        size_t alloc_length;
} httpsrv_chunk_t;

typedef struct {
        SOCKET *socket;
        tid_t tid;

        struct {
                char *buf;
                size_t buf_size;
                size_t len;
        } output;

        struct interpreter {
                struct {
                        char *buf;
                        size_t buf_size;
                        size_t pos;
                        size_t len;
                } fetch_buf;

                struct {
                        struct {
                                char buf[sizeof(HTTPSRV_SCRIPT_BEGIN_TOKEN) - 1];
                                size_t buf_size;
                                size_t ptr;
                                size_t len;
                        } begin_token;

                        struct {
                                char last;
                        } end_token;

                        bool script_section;
                        size_t code_len;
                        size_t code_buf_size;
                        char *code_buf;
                } script;

                char *tmp_buf;
                size_t tmp_buf_size;

                SOCKET *ipc_socket;
                void *object;
                void *context;
        } interpreter;

        struct http_request {
                char *buf;
                size_t buf_size;
                http_parser parser;
                bool message_complete;
                httpsrv_content_type_t content_type;

                struct {
                        size_t field_chunk_num;
                        size_t value_chunk_num;

                        httpsrv_chunk_t field;
                        httpsrv_chunk_t content_type;
                        httpsrv_chunk_t body_data;

                        char *url;
                        char *get_ref;
                        size_t get_len;
                        char *multipart_boundary;
                } header;
        } http_request;

        struct multipart {
                multipartparser parser;

                size_t field_chunk_num;
                size_t value_chunk_num;

                httpsrv_chunk_t field;
                httpsrv_chunk_t content_disposition;
                httpsrv_chunk_t content_type;

                struct {
                        char *buf;
                        char *path;
                        size_t buf_seek;
                        size_t buf_size;
                        size_t write_count;
                } data;

                clock_t tref;
        } multipart;
} httpsrv_thread_t;

struct httpsrv_globals {
        bool debug;
        conf_parser_t *conf;
        SOCKET *socket;
        httpsrv_thread_t **thread;
        int thread_count;
        bool thread_run;
};

/*==============================================================================
  Exported objects
==============================================================================*/
GLOBAL_VARIABLES_TYPE(struct httpsrv_globals);

/*==============================================================================
  Exported functions
==============================================================================*/
extern int httpsrv_interpreter_init(httpsrv_thread_t *self);
extern void httpsrv_interpreter_deinit(httpsrv_thread_t *self);
extern int httpsrv_interpreter_dostring(httpsrv_thread_t *self, const char *string);
extern int httpsrv_interpreter_dofile(httpsrv_thread_t *self, const char *file);
extern int httpsrv_send(httpsrv_thread_t *self, const void *buf, size_t len, bool flush);
extern int httpsrv_send_file(httpsrv_thread_t *self, const char *content_type, u32_t cache_time,
        const char *path);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
