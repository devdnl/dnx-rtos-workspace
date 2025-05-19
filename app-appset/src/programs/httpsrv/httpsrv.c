/*==============================================================================
@file    httpsrv.c

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

// #pragma GCC optimize("Og")

/*==============================================================================
  Include files
==============================================================================*/
#include "httpsrv.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int  handle_http(void *arg);
static void strtolower(char *string);
static void chunk_free(httpsrv_chunk_t *chunk);
static int  chunk_add(httpsrv_chunk_t *chunk, const char *str, size_t length, bool to_lower);
static int  parser_on_url(http_parser *parser, const char *at, size_t length);
static int  parser_header_field(http_parser *parser, const char *at, size_t length);
static int  parser_header_value(http_parser *parser, const char *at, size_t length);
static int  parser_on_headers_complete(http_parser *parser);
static int  parser_on_body(http_parser *parser, const char *at, size_t length);
static int  parser_on_message_complete(http_parser *parser);
static int  multipart_on_part_begin(multipartparser *);
static int  multipart_on_headers_complete(multipartparser *);
static int  multipart_on_part_end(multipartparser *);
static int  multipart_on_data(multipartparser *, const char *data, size_t size);
static int  multipart_on_header_value(multipartparser *, const char *data, size_t size);
static int  multipart_on_header_field(multipartparser *, const char *data, size_t size);
static int  multipart_write_file(httpsrv_thread_t *self, const char *path, const char *data, size_t size);
static int  multipart_flush_file(httpsrv_thread_t *self, const char *path);
static int  execute_file(httpsrv_thread_t *self, const char *url);
static int  execute_extract_script(httpsrv_thread_t *self, char c);
static void execute_flush_buffers(httpsrv_thread_t *self);

/*==============================================================================
  Local object definitions
==============================================================================*/
static const http_parser_settings parser_settings = {
        .on_message_begin    = NULL,
        .on_url              = parser_on_url,
        .on_status           = NULL,
        .on_header_field     = parser_header_field,
        .on_header_value     = parser_header_value,
        .on_headers_complete = parser_on_headers_complete,
        .on_body             = parser_on_body,
        .on_message_complete = parser_on_message_complete,
        .on_chunk_header     = NULL,
        .on_chunk_complete   = NULL,
};

static const multipartparser_callbacks multipart_cbs = {
        .on_body_begin       = NULL,
        .on_part_begin       = multipart_on_part_begin,
        .on_header_field     = multipart_on_header_field,
        .on_header_value     = multipart_on_header_value,
        .on_headers_complete = multipart_on_headers_complete,
        .on_data             = multipart_on_data,
        .on_part_end         = multipart_on_part_end,
        .on_body_end         = NULL,
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(httpsrv, STACK_DEPTH_LARGE);

static const char *SCRIPT_BEGIN_TOKEN = HTTPSRV_SCRIPT_BEGIN_TOKEN;

static const char *HTTP_STATUS_200_OK = "HTTP/1.1 200 OK\r\n";

static const char *HTTP_STATUS_404_NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";

static const char *HTTP_CONTENT_TYPE_TEXT_HTML = "Content-Type: text/html\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_TEXT_CSS = "Content-Type: text/css\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_IMAGE_PNG = "Content-Type: image/png\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_IMAGE_JPEG = "Content-Type: image/jpeg\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_IMAGE_PPM = "Content-Type: image/ppm\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_IMAGE_ICO = "Content-Type: image/x-icon\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_TEXT_TEXT = "Content-Type: text/text\r\n\r\n";

static const char *HTTP_CONTENT_TYPE_TEXT_JS = "Content-Type: text/javascript\r\n\r\n";

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Program main function
 *
 * @param  argc         count of arguments
 * @param *argv[]       argument table
 *
 * @return program status
 */
//==============================================================================
int main(int argc, char *argv[])
{
        size_t mem_size;
        if (shmat(NETMGR_SHM, cast(void *, &global->shared.netmgr), &mem_size) != 0) {
                printf("HTTPSRV: not such %s shared memory!\n", NETMGR_SHM);
        }

        mkdir(HTTPSRV_UPLOAD_DIR, 0666);

        bool wait_for_network = false;
        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "--wait-for-network")) {
                        wait_for_network = true;
                }
        }

        while (wait_for_network) {
                sleep(1);

                NET_family_t      family;
                NET_INET_status_t status;
                if (ifstatus("inet", &family, &status, sizeof(status)) == 0) {

                        if ((status.state == NET_INET_STATE__DHCP_CONFIGURED) or
                                (status.state == NET_INET_STATE__STATIC_IP)) {

                                break;
                        }
                }
        }

        int exit_code = EXIT_FAILURE;

        errno          = 0;
        SOCKET *socket = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
        if (not socket) {
                VERBOSE("Unable to create socket: %s\n", strerror(errno));
                goto cleanup;
        }

        static const NET_INET_sockaddr_t ADDR_ANY = {
                .addr = NET_INET_IPv4_ANY,
                .port = HTTPSRV_PORT,
        };

        errno = 0;
        if (socket_bind(socket, &ADDR_ANY, sizeof(ADDR_ANY)) != 0) {
                VERBOSE("Unable to bind socket: %s\n", strerror(errno));
                goto cleanup;
        }

        errno = 0;
        if (socket_listen(socket) != 0) {
                VERBOSE("Unable to listen socket: %s\n", strerror(errno));
                goto cleanup;
        }

        while (true) {
                SOCKET *new_socket;
                errno = 0;
                if (socket_accept(socket, &new_socket) != 0) {
                        VERBOSE("Socket not accepted: %s\n", strerror(errno));
                        continue;
                }

                httpsrv_thread_t *http_thread = calloc(1, sizeof(*http_thread));
                if (not http_thread) {
                        socket_close(new_socket);
                        continue;
                }

                http_thread->socket = new_socket;

                if (HTTPSRV_SINGLE_THREAD) {
                        handle_http(http_thread);

                } else {
                        while (thread_create(handle_http,
                                       &(thread_attr_t) {
                                               .detached    = false,
                                               .stack_depth = STACK_DEPTH_LARGE,
                                               .priority    = PRIORITY_NORMAL,
                                       },
                                       http_thread) == 0) {

                                msleep(50);
                        }
                }
        }

cleanup:
        socket_close(socket);

        return exit_code;
}

//==============================================================================
/**
 * @brief  Send data to network with buffering.
 *
 * @param  self         thread context
 * @param  buf          buffer
 * @param  len          buffer length
 * @param  flush        flush buffer
 *
 * @return On success number of send/written bytes, otherwise negative value.
 */
//==============================================================================
int httpsrv_send(httpsrv_thread_t *self, const void *buf, size_t len, bool flush)
{
        int written = 0;

        while (buf and (len > 0)) {
                size_t left = sizeof(self->output.buf) - self->output.len;
                size_t n    = min(left, len);

                memcpy(&self->output.buf[self->output.len], buf, n);

                buf += n;
                len -= n;
                self->output.len += n;
                written += n;

                if (self->output.len >= ARRAY_SIZE(self->output.buf)) {
                        errno = 0;
                        if (socket_write(self->socket, self->output.buf, self->output.len) !=
                                cast(int, self->output.len)) {
                                self->output.len = 0;
                                return -errno;
                        }
                        self->output.len = 0;
                }
        }

        if (flush and (self->output.len > 0)) {
                errno = 0;
                if (socket_write(self->socket, self->output.buf, self->output.len) !=
                        cast(int, self->output.len)) {
                        written = -errno;
                }

                self->output.len = 0;
        }

        return written;
}

//==============================================================================
/**
 * @brief  Send file to client.
 *
 * @param  self         thread instance
 * @param  content_type content type and status
 * @param  cach_time    browser cache time in seconds, 0 for disable
 * @param  path         file path
 *
 * @return Number of send bytes, or -1 on error.
 */
//==============================================================================
int httpsrv_send_file(httpsrv_thread_t *self, const char *content_type, u32_t cache_time, const char *path)
{
        int fd = open(path, O_RDONLY);
        if (fd == -1) {
                VERBOSE("No such file: '%s'\n", path);
                return -1;
        }

        int sent = 0;

        if (cache_time > 0) {
                char   cache_control[64];
                size_t cache_control_len = snprintf(cache_control, sizeof(cache_control),
                        "Cache-Control: max-age=%lu\r\n", cache_time);

                errno = 0;
                if (httpsrv_send(self, cache_control, cache_control_len, false) ==
                        cast(int, cache_control_len)) {
                        sent += cache_control_len;
                } else {
                        VERBOSE("cache-control write error: %s\n", strerror(errno));
                        sent = -1;
                }
        }

        if (content_type) {
                size_t content_type_len = strlen(content_type);
                if (content_type_len) {
                        errno = 0;
                        if (httpsrv_send(self, content_type, content_type_len, false) ==
                                cast(int, content_type_len)) {
                                sent += content_type_len;
                        } else {
                                VERBOSE("file '%s' content type write error: %s\n", strerror(errno));
                                sent = -1;
                        }
                }
        }

        while (sent >= 0) {

                errno = 0;
                int n = read(fd, self->http_request.buf, sizeof(self->http_request.buf));
                if (n < 0) {
                        VERBOSE("file '%s' read error: %s\n", strerror(errno));
                        break;

                } else if (n == 0) {
                        break;

                } else {
                        errno = 0;
                        int m = httpsrv_send(self, self->http_request.buf, n, false);
                        if (m != n) {
                                VERBOSE("file '%s' socket write error: %s\n", path, strerror(errno));
                                break;
                        } else {
                                sent += n;
                        }
                }
        }

        close(fd);

        return sent;
}

//==============================================================================
/**
 * @brief  Convert string to lower case.
 *
 * @param  string       string to convert
 */
//==============================================================================
static void strtolower(char *string)
{
        while (string and *string) {
                *string = tolower(*string);
                string++;
        }
}

//==============================================================================
/**
 * @brief  Handle HTTP request.
 *
 * @param  arg          thread arguments
 *
 * @return Thread status.
 */
//==============================================================================
static int handle_http(void *arg)
{
        httpsrv_thread_t *self = arg;

        http_parser_init(&self->http_request.parser, HTTP_REQUEST);
        self->http_request.parser.data      = self;
        self->http_request.message_complete = false;

        int len = 0;
        while ((len = socket_read(self->socket, self->http_request.buf, sizeof(self->http_request.buf))) >
                0) {

                http_parser_execute(&self->http_request.parser, &parser_settings, self->http_request.buf,
                        len);

                if (self->http_request.message_complete) {
                        break;
                }
        }

        chunk_free(&self->http_request.header.field);

        chunk_free(&self->http_request.header.content_type);

        chunk_free(&self->http_request.header.body_data);

        chunk_free(&self->multipart.field);

        chunk_free(&self->multipart.content_disposition);

        chunk_free(&self->multipart.content_type);

        free(self->http_request.header.url);
        self->http_request.header.url = NULL;

        free(self->http_request.header.multipart_boundary);
        self->http_request.header.multipart_boundary = NULL;

        free(self->multipart.data.buf);
        self->multipart.data.buf = NULL;

        free(self->multipart.data.path);
        self->multipart.data.path = NULL;

        if (self->interpreter.object) {
                httpsrv_interpreter_deinit(self);
        }

        socket_disconnect(self->socket);

        socket_close(self->socket);
        free(self);

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when URL is parsed in HTTP request.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_on_url(http_parser *parser, const char *at, size_t length)
{
        httpsrv_thread_t *self = parser->data;

        free(self->http_request.header.url);

        if ((length == 1) and (*at == '/')) {
                self->http_request.header.url = strdup(HTTPSRV_DEFAULT_PAGE);
        } else {
                self->http_request.header.url = strndup(at + 1, length - 1);
        }

        strtolower(self->http_request.header.url);

        char *get_start = strchr(self->http_request.header.url, '?');
        if (get_start) {
                *get_start                        = '\0';
                self->http_request.header.get_ref = get_start + 1;
        }

        DEBUG("%s: '%s'\n", __func__, self->http_request.header.url);

        return 0;
}

//==============================================================================
/**
 * @brief  Free chunk.
 *
 * @param  value        value chunk
 */
//==============================================================================
static void chunk_free(httpsrv_chunk_t *chunk)
{
        free(chunk->value);
        chunk->value        = NULL;
        chunk->alloc_length = 0;
        chunk->value_length = 0;
}

//==============================================================================
/**
 * @brief  Add string to chunk
 *
 * @param  chunk        chunk
 * @param  str          string to add
 * @param  length       string length
 * @param  to_lower     to lower case
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int chunk_add(httpsrv_chunk_t *chunk, const char *str, size_t length, bool to_lower)
{
        if (chunk->alloc_length < (chunk->value_length + length + 1)) {
                size_t new_size = max(chunk->value_length + length + 1,
                        chunk->alloc_length + HTTPSRV_ALLOC_SIZE);

                chunk->value = realloc(chunk->value, new_size);
                if (not chunk->value) {
                        return ENOMEM;
                }

                chunk->alloc_length = new_size;
        }

        memcpy(&chunk->value[chunk->value_length], str, length);
        chunk->value[chunk->value_length + length] = '\0';

        if (to_lower) {
                strtolower(&chunk->value[chunk->value_length]);
        }

        chunk->value_length += length;

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when header filed is parsed in HTTP request.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_header_field(http_parser *parser, const char *at, size_t length)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, length, at);

        struct http_request *http = &self->http_request;

        if (http->header.field_chunk_num == 0) {
                chunk_free(&http->header.field);
        }

        chunk_add(&http->header.field, at, length, true);

        http->header.value_chunk_num = 0;
        http->header.field_chunk_num++;

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when header value is parsed in HTTP request.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_header_value(http_parser *parser, const char *at, size_t length)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, length, at);

        struct http_request *http = &self->http_request;

        /*
         * INFO: in general headers are case insensitive, except
         *       boundary=xxxx in multipart/form-data.
         *       In other cases please use httpsrv_strtolower().
         */

        if (isstreq(http->header.field.value, "content-type")) {
                if (http->header.value_chunk_num == 0) {
                        chunk_free(&http->header.content_type);
                }

                chunk_add(&http->header.content_type, at, length, false);
        }

        http->header.field_chunk_num = 0;
        http->header.value_chunk_num++;

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when entire headers of HTTP request were parsed.
 *
 * @param  parser       http parser
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_on_headers_complete(http_parser *parser)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        free(http->header.multipart_boundary);
        const char  *boundary_hdr     = "multipart/form-data; boundary=";
        const size_t boundary_hdr_len = strlen(boundary_hdr);

        if (isstreqn(http->header.content_type.value, boundary_hdr, boundary_hdr_len)) {

                http->content_type = content_type__multipart_form_data;

                http->header.multipart_boundary = strdup(http->header.content_type.value + boundary_hdr_len);

                DEBUG("multipart boundary=%s\n", http->header.multipart_boundary);

                multipart->data.buf = malloc(HTTPSRV_MULTIPART_BUFFER_SIZE);

                multipartparser_init(&multipart->parser, http->header.multipart_boundary);
                multipart->parser.data = self;

        } else {
                http->content_type              = content_type__text;
                http->header.multipart_boundary = strdup("");
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when body of HTTP request is parsed.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_on_body(http_parser *parser, const char *at, size_t length)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s: %lu bytes long\n", __func__, length);

        int err = 0;

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        if (http->content_type == content_type__multipart_form_data) {
                multipartparser_execute(&multipart->parser, &multipart_cbs, at, length);

        } else if (parser->method == HTTP_POST) {
                chunk_add(&self->http_request.header.body_data, at, length, false);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Callback called when entire message of HTTP request is parsed.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
static int parser_on_message_complete(http_parser *parser)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct http_request *http = &self->http_request;

        int err = execute_file(self, http->header.url);
        if (err) {
                char *ext = strrchr(http->header.url, '.');
                if (ext and ((isstreq(ext, ".html") or isstreq(ext, ".lua")))) {
                        execute_file(self, HTTPSRV_PAGE_NOT_FOUND);
                }
        }

        httpsrv_send(self, NULL, 0, true);

        http->message_complete = true;

        free(self->multipart.data.buf);
        self->multipart.data.buf = NULL;

        return 0;
}

//==============================================================================
/**
 * @brief  Flush multipart buffer do selected file.
 *
 * @param  path         file path
 *
 * @return Number of written bytes is returned, otherwise -1.
 */
//==============================================================================
static int multipart_flush_file(httpsrv_thread_t *self, const char *path)
{
        struct multipart *multipart = &self->multipart;

        DEBUG("%s: %lu bytes to '%s'\n", __func__, multipart->data.buf_seek, path);

        int n = multipart->data.buf_seek;

        if (multipart->data.buf_seek > 0) {
                int oflag = O_WRONLY | ((multipart->data.write_count == 0) ? (O_CREAT | O_TRUNC) : O_APPEND);
                errno     = 0;
                int fd    = open(path, oflag, 0666);
                if (fd != -1) {
                        write(fd, multipart->data.buf, multipart->data.buf_seek);
                        close(fd);
                } else {
                        VERBOSE("%s: %s\n", path, strerror(errno));
                }

                multipart->data.buf_seek = 0;
                multipart->data.write_count++;
        }

        return n;
}

//==============================================================================
/**
 * @brief  Write multipart data to file (or buffer).
 *
 * @param  path         file path
 * @param  data         data
 * @param  size         data size
 *
 * @return Number of written bytes is returned, otherwise -1.
 */
//==============================================================================
static int multipart_write_file(httpsrv_thread_t *self, const char *path, const char *data, size_t size)
{
        struct multipart *multipart = &self->multipart;

        if (not multipart->data.buf) {
                VERBOSE("no multipart buffer allocated!\n");
                return -1;
        }

        while (size) {
                size_t buf_available = HTTPSRV_MULTIPART_BUFFER_SIZE - multipart->data.buf_seek;
                size_t len           = min(buf_available, size);

                memcpy(multipart->data.buf + multipart->data.buf_seek, data, len);

                size -= len;
                data += len;
                multipart->data.buf_seek += len;

                if (multipart->data.buf_seek >= HTTPSRV_MULTIPART_BUFFER_SIZE) {
                        multipart_flush_file(self, path);
                }
        }

        return size;
}

//==============================================================================
/**
 * @brief  Callback called on part begin.
 *
 * @param  parser       http parser
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_part_begin(multipartparser *parser)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct multipart *multipart = &self->multipart;

        multipart->data.buf_seek    = 0;
        multipart->data.write_count = 0;
        multipart->tref             = get_uptime_ms();

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when data is parsed.
 *
 * @param  parser       http parser
 * @param  data         buffer positon
 * @param  size         data size
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_data(multipartparser *parser, const char *data, size_t size)
{
        httpsrv_thread_t *self = parser->data;

        struct multipart *multipart = &self->multipart;

        DEBUG("%s: %lu bytes long\n", __func__, size);

        multipart_write_file(self, multipart->data.path, data, size);

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called on part end.
 *
 * @param  parser       http parser
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_part_end(multipartparser *parser)
{
        httpsrv_thread_t *self = parser->data;

        struct multipart *multipart = &self->multipart;

        DEBUG("%s: %llu ms\n", __func__, get_uptime_ms() - multipart->tref);

        multipart_flush_file(self, multipart->data.path);

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when header field in muiltipart is parsed.
 *
 * @param  parser       http parser
 * @param  data         buffer positon
 * @param  size         data size
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_header_field(multipartparser *parser, const char *data, size_t size)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, size, data);

        struct multipart *multipart = &self->multipart;

        if (multipart->field_chunk_num == 0) {
                chunk_free(&multipart->field);
        }

        chunk_add(&multipart->field, data, size, true);

        multipart->value_chunk_num = 0;
        multipart->field_chunk_num++;

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when header valie in muiltipart is parsed.
 *
 * @param  parser       http parser
 * @param  data         buffer positon
 * @param  size         data size
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_header_value(multipartparser *parser, const char *data, size_t size)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, size, data);

        struct multipart *multipart = &self->multipart;

        httpsrv_chunk_t *chunk    = NULL;
        bool             to_lower = false;

        if (isstreq(multipart->field.value, "content-disposition")) {
                chunk    = &multipart->content_disposition;
                to_lower = true;

        } else if (isstreq(multipart->field.value, "content-type")) {
                chunk    = &multipart->content_type;
                to_lower = true;
        }

        if (chunk) {
                if (multipart->value_chunk_num == 0) {
                        chunk_free(chunk);
                }

                chunk_add(chunk, data, size, to_lower);
        }

        multipart->field_chunk_num = 0;
        multipart->value_chunk_num++;

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when all headers are complete.
 *
 * @param  parser       parser
 *
 * @return On success 0.
 */
//==============================================================================
static int multipart_on_headers_complete(multipartparser *parser)
{
        httpsrv_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct multipart *multipart = &self->multipart;

        const char *name_token = "name=";
        char       *name_start = strstr(multipart->content_disposition.value, name_token);
        if (name_start) {
                name_start += strlen(name_token);
                char  colon    = *name_start++;
                char *name_end = strchr(name_start, colon);
                if (name_end) {
                        size_t name_len = cast(uintptr_t, name_end) - cast(uintptr_t, name_start);

                        char path[128];
                        snprintf(path, sizeof(path), "%s/%.*s", HTTPSRV_UPLOAD_DIR, name_len, name_start);

                        free(multipart->data.path);

                        multipart->data.path = strdup(path);
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Parse page file.
 *
 * @param  self         HTTP thread.
 * @param  url          url
 *
 * @return On success 0.
 */
//==============================================================================
static int execute_file(httpsrv_thread_t *self, const char *url)
{
        DEBUG("%s\n", __func__);

        struct interpreter *inter = &self->interpreter;

        char file_path[128];
        snprintf(file_path, sizeof(file_path), "%s/%s", HTTPSRV_ROOT_DIR, url);
        const char *ext = strrchr(url, '.');

        errno = 0;
        if (stat(file_path, &(struct stat) {}) != 0) {

                if (ext == NULL) {
                        strlcat(file_path, ".html", sizeof(file_path));
                        ext = strrchr(file_path, '.');
                }

                if (stat(file_path, &(struct stat) {}) != 0) {
                        VERBOSE("no such '%s' file!\n", url);
                        return errno;
                }
        }

        if (isstreq(url, HTTPSRV_PAGE_NOT_FOUND)) {
                httpsrv_send(self, HTTP_STATUS_404_NOT_FOUND, strlen(HTTP_STATUS_404_NOT_FOUND), false);

        } else {
                httpsrv_send(self, HTTP_STATUS_200_OK, strlen(HTTP_STATUS_200_OK), false);
        }

        if (isstreq(ext, ".html")) {
                httpsrv_send(self, HTTP_CONTENT_TYPE_TEXT_HTML, strlen(HTTP_CONTENT_TYPE_TEXT_HTML), false);

        } else if (isstreq(ext, ".png")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_IMAGE_PNG, HTTPSRV_CACHE_TIME_FOR_PNG,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, ".jpg") or isstreq(ext, ".jpeg")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_IMAGE_JPEG, HTTPSRV_CACHE_TIME_FOR_JPEG,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, ".ppm")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_IMAGE_PPM, HTTPSRV_CACHE_TIME_FOR_JPEG,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, ".css")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_TEXT_CSS, HTTPSRV_CACHE_TIME_FOR_CSS,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, ".ico")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_IMAGE_ICO, HTTPSRV_CACHE_TIME_FOR_ICO,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, ".js")) {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_TEXT_JS, HTTPSRV_CACHE_TIME_FOR_JS,
                            file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }

        } else if (isstreq(ext, HTTPSRV_SCRIPT_EXTENSION)) {

                httpsrv_send(self, HTTP_CONTENT_TYPE_TEXT_HTML, strlen(HTTP_CONTENT_TYPE_TEXT_HTML), false);

                if (httpsrv_interpreter_init(self) == 0) {

                        int err = httpsrv_interpreter_dofile(self, file_path);
                        if (err) {
                                const char *msg = "<font color='red'><b>SCRIPT ERROR</b></font>";
                                socket_send(self->socket, msg, strlen(msg), NET_FLAGS__MORE);
                        }

                        httpsrv_interpreter_deinit(self);
                        return err;

                } else {
                        VERBOSE("cannot initialize interpreter!\n");
                        return 1;
                }

        } else {
                if (httpsrv_send_file(self, HTTP_CONTENT_TYPE_TEXT_TEXT, 0, file_path) >= 0) {
                        return 0;
                } else {
                        return 1;
                }
        }

        if (httpsrv_interpreter_init(self) != 0) {
                VERBOSE("cannot initialize interpreter!\n");
        }

        errno      = 0;
        FILE *page = fopen(file_path, "r");
        if (not page) {
                VERBOSE("no such '%s' file!\n", url);
                return errno;
        }

        inter->script.begin_token.len = 0;
        inter->script.begin_token.ptr = 0;
        inter->script.code_len        = 0;
        inter->script.script_section  = false;

        bool run = true;

        while (run) {
                if (inter->fetch_buf.pos == inter->fetch_buf.len) {
                        inter->fetch_buf.pos = 0;

                        inter->fetch_buf.len = fread(inter->fetch_buf.buf, 1, sizeof(inter->fetch_buf.buf),
                                page);

                        if (inter->fetch_buf.len == 0) {
                                execute_flush_buffers(self);
                                break;
                        }
                }

                char c = inter->fetch_buf.buf[inter->fetch_buf.pos++];
                run    = execute_extract_script(self, c) == 0;
        }

        fclose(page);

        return 0;
}

//==============================================================================
/**
 * @brief  Extract and execute script (if found).
 *
 * @param  self         HTTP thread.
 * @param  c            character (from stream).
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int execute_extract_script(httpsrv_thread_t *self, char c)
{
        struct interpreter *inter = &self->interpreter;

        if (not inter->script.script_section) {
                if (inter->script.begin_token.len == sizeof(inter->script.begin_token.buf)) {

                        size_t first_char_idx = inter->script.begin_token.ptr;

                        inter->script.code_buf[inter->script.code_len++] = inter->script.begin_token
                                                                                   .buf[first_char_idx];

                        inter->script.begin_token.len--;
                }

                if (inter->script.code_len >= sizeof(inter->script.code_buf)) {
                        errno = 0;
                        int n = httpsrv_send(self, inter->script.code_buf, inter->script.code_len, false);

                        if (n != cast(int, inter->script.code_len)) {
                                int err = errno;
                                VERBOSE("extract script: %s\n", strerror(errno));
                                return err;
                        }

                        inter->script.code_len = 0;
                }

                inter->script.begin_token.buf[inter->script.begin_token.ptr] = c;
                if (++inter->script.begin_token.ptr >= sizeof(inter->script.begin_token.buf)) {
                        inter->script.begin_token.ptr = 0;
                }
                inter->script.begin_token.len++;

                size_t token_start_idx = inter->script.begin_token.ptr;

                for (size_t i = 0; i < sizeof(inter->script.begin_token.buf); i++) {
                        if (inter->script.begin_token.buf[token_start_idx] != SCRIPT_BEGIN_TOKEN[i]) {
                                return 0;
                        }

                        if (++token_start_idx >= sizeof(inter->script.begin_token.buf)) {
                                token_start_idx = 0;
                        }
                }

                if (inter->script.code_len) {
                        errno = 0;
                        int n = httpsrv_send(self, inter->script.code_buf, inter->script.code_len, false);

                        if (n != cast(int, inter->script.code_len)) {
                                int err = errno;
                                VERBOSE("extract script: %s\n", strerror(errno));
                                return err;
                        }
                }

                inter->script.code_len = 0;

                memset(inter->script.begin_token.buf, 0, sizeof(inter->script.begin_token.buf));

                inter->script.begin_token.ptr = 0;
                inter->script.begin_token.len = 0;

                inter->script.script_section = true;
                inter->script.end_token.last = 0;

        } else {
                if ((inter->script.end_token.last == '?') and (c == '>')) {

                        if (httpsrv_interpreter_dostring(self, inter->script.code_buf)) {
                                const char *msg = "<font color='red'><b>ERROR</b></font>";
                                httpsrv_send(self, msg, strlen(msg), false);
                        }

                        inter->script.code_len = 0;

                        inter->script.script_section = false;
                        inter->script.end_token.last = 0;

                } else {
                        if (inter->script.end_token.last != 0) {
                                if (inter->script.code_len < sizeof(inter->script.code_buf) - 1) {

                                        inter->script.code_buf[inter->script.code_len++] = inter->script
                                                                                                   .end_token
                                                                                                   .last;

                                        inter->script.code_buf[inter->script.code_len] = '\0';
                                } else {
                                        VERBOSE("script too long!\n");
                                }

                                inter->script.end_token.last = c;
                        } else {
                                inter->script.end_token.last = c;
                        }
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Flush execute buffers.
 *
 * @param  self         HTTP thread.
 */
//==============================================================================
static void execute_flush_buffers(httpsrv_thread_t *self)
{
        struct interpreter *inter = &self->interpreter;

        if (inter->script.script_section) {
                return;
        }

        if (inter->script.code_len) {
                errno = 0;
                int n = httpsrv_send(self, inter->script.code_buf, inter->script.code_len, false);

                if (n != cast(int, inter->script.code_len)) {
                        VERBOSE("%s(1): %s\n", __func__, strerror(errno));
                        return;
                }
        }

        if (inter->script.begin_token.len) {
                size_t token_start_idx = inter->script.begin_token.ptr;
                inter->script.code_len = inter->script.begin_token.len;

                for (size_t i = 0; i < inter->script.begin_token.len; i++) {

                        inter->script.code_buf[i] = inter->script.begin_token.buf[token_start_idx];

                        if (++token_start_idx >= sizeof(inter->script.begin_token.buf)) {
                                token_start_idx = 0;
                        }
                }

                errno = 0;
                int n = httpsrv_send(self, inter->script.code_buf, inter->script.code_len, false);

                if (n != cast(int, inter->script.code_len)) {
                        VERBOSE("%s(2): %s\n", __func__, strerror(errno));
                        return;
                }
        }
}

/*==============================================================================
  End of file
==============================================================================*/
