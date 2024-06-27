/*=========================================================================*//**
@file    httpd.h

@author  Daniel Zorychta

@brief   The HTTP server.

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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


*//*==========================================================================*/

/**
@defgroup HTTPD_H_ HTTPD_H_

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

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#undef global
#define global cast(struct httpd_globals *, _getglobal)

#if 0
#define DEBUG(...) printf("httpd: " __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define VERBOSE(...)                printf("httpd: " __VA_ARGS__)

#define HTTPD_MULTIPART_BUFFER_SIZE 4096
#define HTTPD_DEFAULT_PAGE          "update"
#define HTTPD_PAGE_NOT_FOUND        "page-not-found"

// clang-format off
#define HTTPD_PAGE_MAP                                                                 \
        {                                                                              \
                .url                      = "updating",                                \
                .multipart_content_dispo  = "name=\"firmware\"",                       \
                .file_id                  = httpd_file_id__updating,                   \
                .multipart_id             = httpd_multipart__firmware,                 \
                .http_on_message_complete = httpd_on_message_complete__updating,       \
                .multipart_on_data        = httpd_multipart_on_data__updating,         \
                .multipart_on_part_end    = httpd_multipart_on_part_end__updating,     \
        },                                                                             \
        {                                                                              \
                .url                      = "update",                                  \
                .multipart_content_dispo  = NULL,                                      \
                .file_id                  = httpd_file_id__update,                     \
                .multipart_id             = httpd_multipart__none,                     \
                .http_on_message_complete = httpd_on_message_complete__update,         \
                .multipart_on_data        = NULL,                                      \
                .multipart_on_part_end    = NULL,                                      \
        },                                                                             \
        {                                                                              \
                .url                      = "page-not-found",                          \
                .multipart_content_dispo  = NULL,                                      \
                .file_id                  = httpd_file_id__page_not_found,             \
                .multipart_id             = httpd_multipart__none,                     \
                .http_on_message_complete = httpd_on_message_complete__page_not_found, \
                .multipart_on_data        = NULL,                                      \
                .multipart_on_part_end    = NULL,                                      \
        },

/*==============================================================================
  Exported object types
==============================================================================*/
typedef enum {
        httpd_file_id__none = 0,
        httpd_file_id__update,
        httpd_file_id__updating,
        httpd_file_id__page_not_found,
} httpd_file_id_t;

typedef enum {
        httpd_multipart__none = 0,
        httpd_multipart__firmware,
} httpd_multipart_id_t;

typedef enum {
        content_type__none = 0,
        content_type__multipart_form_data,
        content_type__post_method,
} httpd_content_type_t;

typedef void (*httpd_post_method_cb_t)(http_parser *, const char *data, size_t size);

typedef struct {
        struct http_request {
                SOCKET              *socket;
                char                 buf[8192];
                http_parser          parser;
                bool                 message_complete;
                httpd_file_id_t      file_id;
                httpd_content_type_t content_type;

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
                multipartparser      parser;
                httpd_multipart_id_t part_id;
                char                *buf;
                size_t               buf_seek;
                size_t               write_count;
                size_t               header_field_chunk;
                size_t               header_value_chunk;
                char                *header_field;
                char                *content_disposition;
                char                *content_type;
                clock_t              tref;
        } multipart;
} http_thread_t;

struct httpd_globals {};

/*==============================================================================
  Exported objects
==============================================================================*/
GLOBAL_VARIABLES_TYPE(struct httpd_globals);

extern const char HTTPD_CONTENT_TYPE_OK_TEXT_HTML[];
extern const char HTTPD_CONTENT_TYPE_NOT_FOUND_TEXT_HTML[];
extern const char HTTPD_COMMON_HTML_BEGIN[];
extern const char HTTPD_COMMON_HTML_END[];

/*==============================================================================
  Exported functions
==============================================================================*/
extern int httpd_send_file(http_thread_t *self, const char *content_type,
        const char *path);

extern int httpd_sockprintf(http_thread_t *self, const char *format, ...);

extern int httpd_flush_multipart_file_buffer(http_thread_t *self, const char *path);

extern int httpd_write_multipart_file(http_thread_t *self, const char *path,
        const char *buf, size_t size);

extern void httpd_strtolower(char *string);

extern int httpd_on_message_complete__update(http_parser *parser);

extern int httpd_on_message_complete__updating(http_parser *parser);

extern int httpd_multipart_on_data__updating(multipartparser *parser, const char *data,
        size_t size);

extern int httpd_multipart_on_part_end__updating(multipartparser *parser);

extern int httpd_on_message_complete__page_not_found(http_parser *parser);

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
