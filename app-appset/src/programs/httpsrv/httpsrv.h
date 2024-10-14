/*==============================================================================
@file    httpsrv.h

@author  Daniel Zorychta

@brief   The HTTP server with script support.

@note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


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
#include <netmgr/shared.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#define HTTPSRV_DEBUG_ON                       0
#define HTTPSRV_MULTIPART_BUFFER_SIZE          (256 * 1024)
#define HTTPSRV_INTERPRETER_FETCH_BUFFER_SIZE  4096
#define HTTPSRV_INTERPRETER_CODE_BUFFER_SIZE   8192
#define HTTPSRV_INTERPRETER_OUTPUT_BUFFER_SIZE 1024
#define HTTPSRV_HTTP_PARSER_BUFFER_SIZE        8192
#define HTTPSRV_PORT                           8080
#define HTTPSRV_DEFAULT_PAGE                   "index.html"
#define HTTPSRV_PAGE_NOT_FOUND                 "404.html"
#define HTTPSRV_ROOT_DIR                       "/mmc/srv2"
#define HTTPSRV_UPLOAD_DIR                     "/mmc/srv2/upload"
#define HTTPSRV_SCRIPT_BEGIN_TOKEN             "<?lua"
#define HTTPSRV_SCRIPT_EXTENSION               ".lua"

#undef global
#define global cast(struct httpsrv_globals *, _getglobal)

#if DEBUG_ON
#define DEBUG(...) printf("httplua: " __VA_ARGS__)
#else
#define DEBUG(...)
#endif
#define VERBOSE(...) printf("httpsrv: " __VA_ARGS__)

/*==============================================================================
  Exported object types
==============================================================================*/
typedef enum {
        content_type__text = 0,
        content_type__multipart_form_data,
} httpsrv_content_type_t;

typedef struct {
        struct interpreter {
                struct {
                        char   buf[HTTPSRV_INTERPRETER_FETCH_BUFFER_SIZE];
                        size_t pos;
                        size_t len;
                } fetch_buf;

                struct {
                        struct {
                                char   buf[sizeof(HTTPSRV_SCRIPT_BEGIN_TOKEN) - 1];
                                size_t ptr;
                                size_t len;
                        } begin_token;

                        struct {
                                char last;
                        } end_token;

                        bool   script_section;
                        size_t code_len;
                        char   code_buf[HTTPSRV_INTERPRETER_FETCH_BUFFER_SIZE];
                } script;

                struct {
                        char buf[HTTPSRV_INTERPRETER_OUTPUT_BUFFER_SIZE];
                } output;

                void *object;
        } interpreter;

        struct http_request {
                SOCKET                *socket;
                char                   buf[HTTPSRV_HTTP_PARSER_BUFFER_SIZE];
                http_parser            parser;
                bool                   message_complete;
                httpsrv_content_type_t content_type;

                struct {
                        size_t field_chunk;
                        size_t value_chunk;
                        size_t content_length;
                        size_t body_data_length;
                        char  *field;
                        char  *url;
                        char  *get_ref;
                        char  *content_type;
                        char  *multipart_boundary;
                        char  *body_data;
                } header;
        } http_request;

        struct multipart {
                multipartparser parser;
                char           *buf;
                char           *path;
                size_t          buf_seek;
                size_t          write_count;
                size_t          header_field_chunk;
                size_t          header_value_chunk;
                char           *header_field;
                char           *content_disposition;
                char           *content_type;
                clock_t         tref;
        } multipart;
} httpsrv_thread_t;

struct httpsrv_globals {
        struct {
                netmgr_shared_t *netmgr;
        } shared;
};

/*==============================================================================
  Exported objects
==============================================================================*/
GLOBAL_VARIABLES_TYPE(struct httpsrv_globals);

/*==============================================================================
  Exported functions
==============================================================================*/
extern int  httpsrv_interpreter_init(httpsrv_thread_t *self);
extern void httpsrv_interpreter_deinit(httpsrv_thread_t *self);
extern int  httpsrv_interpreter_dostring(httpsrv_thread_t *self, const char *string);
extern int  httpsrv_interpreter_dofile(httpsrv_thread_t *self, const char *file);

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
