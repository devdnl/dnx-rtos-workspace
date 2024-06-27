/*=========================================================================*//**
@file    httpd.c

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

// #pragma GCC optimize("Og")

/*==============================================================================
  Include files
==============================================================================*/
#include "httpd.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        const char            *url;
        const char            *multipart_content_dispo;
        httpd_file_id_t        file_id;
        httpd_multipart_id_t   multipart_id;
        http_cb                http_on_message_complete;
        multipart_data_cb      multipart_on_data;
        multipart_cb           multipart_on_part_end;
        httpd_post_method_cb_t http_on_post_method;
} httpd_page_map_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int handle_http(void *arg);
static int socket_receive_stream(SOCKET *socket, void *buf, size_t len);
static int http_on_url(http_parser *parser, const char *at, size_t length);
static int http_header_field(http_parser *parser, const char *at, size_t length);
static int http_header_value(http_parser *parser, const char *at, size_t length);
static int http_on_headers_complete(http_parser *parser);
static int http_on_body(http_parser *parser, const char *at, size_t length);
static int http_on_message_complete(http_parser *parser);
static int multipart_on_part_begin(multipartparser *);
static int multipart_on_headers_complete(multipartparser *);
static int multipart_on_part_end(multipartparser *);
static int multipart_on_data(multipartparser *, const char *data, size_t size);
static int multipart_on_header_value(multipartparser *, const char *data, size_t size);
static int multipart_on_header_field(multipartparser *, const char *data, size_t size);

/*==============================================================================
  Local object definitions
==============================================================================*/
static const http_parser_settings parser_settings = {
        .on_message_begin    = NULL,
        .on_url              = http_on_url,
        .on_status           = NULL,
        .on_header_field     = http_header_field,
        .on_header_value     = http_header_value,
        .on_headers_complete = http_on_headers_complete,
        .on_body             = http_on_body,
        .on_message_complete = http_on_message_complete,
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

static const httpd_page_map_t httpd_page_map[] = {HTTPD_PAGE_MAP};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(httpd, STACK_DEPTH_MEDIUM);

const char HTTPD_CONTENT_TYPE_OK_TEXT_HTML
        [] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

const char HTTPD_CONTENT_TYPE_NOT_FOUND_TEXT_HTML
        [] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";

// clang-format off
const char HTTPD_COMMON_HTML_BEGIN[] =
"<!DOCTYPE html>"
"<html>"
  "<head>"
    "<title>Termometr</title>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<link rel='stylesheet' href='style.css'>"
    "<link rel='icon' type='image/x-icon' href='/etc/http/favicon.png'>"
  "</head>"
  "<body>"
     "<header>"
       "<nav>"
         "<ul>"
           "<li><a href='update'>Update</a></li>"
         "</ul>"
       "</nav>"
     "</header>"
     "<div class='content'>";

const char HTTPD_COMMON_HTML_END[] =
     "</div>"
   "</body>"
"</html>";
// clang-format on

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
        UNUSED_ARG2(argc, argv);

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
        SOCKET *socket = socket_open("inet", NET_PROTOCOL__TCP);
        if (not socket) {
                VERBOSE("Unable to create socket: %s\n", strerror(errno));
                goto cleanup;
        }

        static const NET_INET_sockaddr_t ADDR_ANY = {
                .addr = NET_INET_IPv4_ANY,
                .port = 80,
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

                http_thread_t *http_thread = calloc(1, sizeof(*http_thread));
                if (not http_thread) {
                        socket_close(new_socket);
                        continue;
                }

                http_thread->http_request.socket = new_socket;

                handle_http(http_thread);
        }

cleanup:
        socket_close(socket);

        return exit_code;
}

//==============================================================================
/**
 * @brief  Send file to client.
 *
 * @param  self         thread instance
 * @param  content_type content type and status
 * @param  path         file path
 *
 * @return Number of send bytes, or -1 on error.
 */
//==============================================================================
int httpd_send_file(http_thread_t *self, const char *content_type, const char *path)
{
        int fd = open(path, O_RDONLY);
        if (fd == -1) {
                VERBOSE("No such file: '%s'\n", path);
                return -1;
        }

        int sent = 0;

        size_t content_type_len = strlen(content_type);
        if (content_type_len) {
                errno = 0;
                if (socket_write(self->http_request.socket, content_type,
                            content_type_len) == cast(int, content_type_len)) {
                        sent = content_type_len;
                } else {
                        VERBOSE("file '%s' content type write error: %s\n",
                                strerror(errno));
                        sent = -1;
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
                        int m = socket_write(self->http_request.socket,
                                self->http_request.buf, n);
                        if (m != n) {
                                VERBOSE("file '%s' socket write error: %s\n", path,
                                        strerror(errno));
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
 * @brief  Flush multipart buffer do selected file.
 *
 * @param  path         file path
 *
 * @return Number of written bytes is returned, otherwise -1.
 */
//==============================================================================
int httpd_flush_multipart_file_buffer(http_thread_t *self, const char *path)
{
        struct multipart *multipart = &self->multipart;

        DEBUG("%s: %lu bytes to '%s'\n", __func__, multipart->buf_seek, path);

        int n = multipart->buf_seek;

        if (multipart->buf_seek > 0) {
                int oflag = O_WRONLY |
                        ((multipart->write_count == 0) ? (O_CREAT | O_TRUNC) : O_APPEND);
                errno  = 0;
                int fd = open(path, oflag);
                if (fd != -1) {
                        write(fd, multipart->buf, multipart->buf_seek);
                        close(fd);
                } else {
                        VERBOSE("%s: %s\n", path, strerror(errno));
                }

                multipart->buf_seek = 0;
                multipart->write_count++;
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
int httpd_write_multipart_file(http_thread_t *self, const char *path, const char *data,
        size_t size)
{
        struct multipart *multipart = &self->multipart;

        if (not multipart->buf) {
                return -1;
        }

        while (size) {
                size_t buf_available = HTTPD_MULTIPART_BUFFER_SIZE - multipart->buf_seek;
                size_t len           = min(buf_available, size);

                memcpy(multipart->buf + multipart->buf_seek, data, len);

                size -= len;
                data += len;
                multipart->buf_seek += len;

                if (multipart->buf_seek >= HTTPD_MULTIPART_BUFFER_SIZE) {
                        httpd_flush_multipart_file_buffer(self, path);
                }
        }

        return size;
}

//==============================================================================
/**
 * @brief  Convert string to lower case.
 *
 * @param  string       string to convert
 */
//==============================================================================
void httpd_strtolower(char *string)
{
        while (string and *string) {
                *string = tolower(*string);
                string++;
        }
}

//==============================================================================
/**
 * @brief  Printf to socket.
 *
 * @param  self         httpd thread
 * @param  format       format
 * @param  ...          arguments
 *
 * @return Number of send bytes or -1 on error.
 */
//==============================================================================
int httpd_sockprintf(http_thread_t *self, const char *format, ...)
{
        va_list args;
        va_start(args, format);

        size_t len = vsnprintf(self->http_request.buf, sizeof(self->http_request.buf),
                format, args);

        int n = socket_write(self->http_request.socket, self->http_request.buf, len);

        va_end(args);

        return n;
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
        http_thread_t *self = arg;

        NET_INET_sockaddr_t addr;
        socket_get_address(self->http_request.socket, &addr, sizeof(addr));

        /*
        VERBOSE("New connection from: %d.%d.%d.%d\n", NET_INET_IPv4_a(addr.addr),
                NET_INET_IPv4_b(addr.addr), NET_INET_IPv4_c(addr.addr),
                NET_INET_IPv4_d(addr.addr));
        */

        http_parser_init(&self->http_request.parser, HTTP_REQUEST);
        self->http_request.parser.data        = self;
        self->http_request.message_complete   = false;
        self->http_request.header.field_chunk = 0;
        self->http_request.header.value_chunk = 0;

        int len = 0;
        while ((len = socket_receive_stream(self->http_request.socket,
                        self->http_request.buf, sizeof(self->http_request.buf))) > 0) {

                http_parser_execute(&self->http_request.parser, &parser_settings,
                        self->http_request.buf, len);

                if (self->http_request.message_complete) {
                        break;
                }
        }

        free(self->http_request.header.field);
        self->http_request.header.field = NULL;

        free(self->http_request.header.url);
        self->http_request.header.url = NULL;

        free(self->http_request.header.content_type);
        self->http_request.header.content_type = NULL;

        free(self->http_request.header.multipart_boundary);
        self->http_request.header.multipart_boundary = NULL;

        free(self->http_request.header.body_data);
        self->http_request.header.body_data = NULL;

        free(self->multipart.header_field);
        self->multipart.header_field = NULL;

        free(self->multipart.content_disposition);
        self->multipart.content_disposition = NULL;

        free(self->multipart.content_type);
        self->multipart.content_type = NULL;

        free(self->multipart.buf);
        self->multipart.buf = NULL;

        socket_close(self->http_request.socket);
        free(self);

        return 0;
}

//==============================================================================
/**
 * @brief  Try to receive and fill to full input buffer.
 *
 * @param  socket       socket
 * @param  buf          destination buffer
 * @param  len          buffer length
 *
 * @return Number of received bytes, -1 on error.
 */
//==============================================================================
static int socket_receive_stream(SOCKET *socket, void *buf, size_t len)
{
        socket_set_recv_timeout(socket, 500);

        int recved = 0;

        while (len) {
                errno = 0;
                int n = socket_recv(socket, buf, len, 0);
                if (n > 0) {
                        buf += n;
                        len -= n;
                        recved += n;

                        socket_set_recv_timeout(socket, 1);

                } else if (n < 0) {

                        if (not((errno == ETIME) and (recved > 0))) {
                                recved = n;
                        }

                        break;
                } else {
                        break;
                }
        }

        return recved;
}

//==============================================================================
/**
 * @brief  Get value of specified key from get-formatted string.
 *
 * @param  str          source string in get-style formatting
 * @param  key          key
 * @param  val          value buffer
 * @param  val_max      value buffer size
 *
 * @return Pointer to val buffer. If not found then empty string.
 */
//==============================================================================
const char *httpd_get_value(const char *str, const char *key, char *val, size_t val_max)
{
        if (not str or not key or not val or not val_max) {
                return "";
        }

        char key_str[128];
        snprintf(key_str, sizeof(key_str), "%s=", key);

        char *start = strstr(str, key_str);
        if (not start) {
                return "";
        }

        if (start != str) {
                if (not((*(start - 1) == '?') or (*(start - 1) == '&'))) {
                        return "";
                }
        }

        char *v = strchr(start, '=');
        if (not v) {
                return "";
        }

        v++;
        if (*v == '\0') {
                return "";
        }

        size_t val_len;

        char *end = strchr(v, '&');
        if (not end) {
                val_len = strlen(v);
        } else {
                val_len = end - v;
        }

        strlcpy(val, v, min(val_max, val_len + 1));

        return val;
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
static int http_on_url(http_parser *parser, const char *at, size_t length)
{
        http_thread_t       *self = parser->data;
        struct http_request *http = &self->http_request;

        free(self->http_request.header.url);

        if ((length == 1) and (*at == '/')) {
                self->http_request.header.url = strdup(HTTPD_DEFAULT_PAGE);
        } else {
                self->http_request.header.url = strndup(at + 1, length - 1);
        }

        httpd_strtolower(self->http_request.header.url);

        char *get_start = strchr(self->http_request.header.url, '?');
        if (get_start) {
                *get_start                        = '\0';
                self->http_request.header.get_ref = get_start + 1;
        }

        http->file_id = httpd_file_id__none;

        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                const httpd_page_map_t *e = &httpd_page_map[i];

                if (isstreq(http->header.url, e->url)) {
                        http->file_id = e->file_id;
                        break;
                }
        }

        DEBUG("%s: '%s'\n", __func__, self->http_request.header.url);

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
static int http_header_field(http_parser *parser, const char *at, size_t length)
{
        http_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, length, at);

        struct http_request *http = &self->http_request;

        http->header.value_chunk = 0;

        if (http->header.field_chunk == 0) {
                free(http->header.field);
                http->header.field = strndup(at, length);
                http->header.field_chunk++;
        } else {
                size_t new_size    = strsize(http->header.field) + length;
                http->header.field = realloc(http->header.field, new_size);
                strlcat(http->header.field, at, new_size);
        }

        httpd_strtolower(http->header.field);

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
static int http_header_value(http_parser *parser, const char *at, size_t length)
{
        http_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, length, at);

        struct http_request *http = &self->http_request;

        http->header.field_chunk = 0;

        /*
         * INFO: in general headers are case insensitive, except
         *       boundary=xxxx in multipart/form-data.
         *       In other cases please use httpd_strtolower().
         */

        if (isstreq(http->header.field, "content-type")) {
                if (http->header.value_chunk == 0) {
                        free(http->header.content_type);
                        http->header.content_type = strndup(at, length);
                        http->header.value_chunk++;

                } else {
                        size_t new_size = strsize(http->header.content_type) + length;
                        http->header.content_type = realloc(http->header.content_type,
                                new_size);
                        strlcat(http->header.content_type, at, new_size);
                }
        }

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
static int http_on_headers_complete(http_parser *parser)
{
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        free(http->header.multipart_boundary);
        const char  *boundary_hdr     = "multipart/form-data; boundary=";
        const size_t boundary_hdr_len = strlen(boundary_hdr);

        if (isstreqn(http->header.content_type, boundary_hdr, boundary_hdr_len)) {

                http->content_type = content_type__multipart_form_data;

                http->header.multipart_boundary = strdup(
                        http->header.content_type + boundary_hdr_len);

                DEBUG("multipart boundary=%s\n", http->header.multipart_boundary);

                multipart->buf = malloc(HTTPD_MULTIPART_BUFFER_SIZE);

                multipartparser_init(&multipart->parser, http->header.multipart_boundary);
                multipart->parser.data = self;

        } else {
                http->content_type              = content_type__none;
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
static int http_on_body(http_parser *parser, const char *at, size_t length)
{
        http_thread_t *self = parser->data;

        DEBUG("%s: %lu bytes long\n", __func__, length);

        int err = 0;

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        if (http->content_type == content_type__multipart_form_data) {
                multipartparser_execute(&multipart->parser, &multipart_cbs, at, length);

        } else if (parser->method == HTTP_POST) {

                self->http_request.header.body_data = realloc(
                        self->http_request.header.body_data,
                        self->http_request.header.body_data_length + length);

                if (self->http_request.header.body_data) {
                        memcpy(&self->http_request.header.body_data
                                        [self->http_request.header.body_data_length],
                                at, length);
                        self->http_request.header.body_data_length += length;
                }
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
static int http_on_message_complete(http_parser *parser)
{
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct http_request *http = &self->http_request;

        bool found = false;

        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                const httpd_page_map_t *e = &httpd_page_map[i];

                if (http->file_id == e->file_id) {
                        if (e->http_on_message_complete) {
                                e->http_on_message_complete(parser);
                        }

                        found = true;
                        break;
                }
        }

        if (not found) {
                int err = httpd_send_file(parser->data, HTTPD_CONTENT_TYPE_OK_TEXT_HTML,
                        http->header.url);

                if (err < 0) {

                        bool found = false;

                        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                                const httpd_page_map_t *e = &httpd_page_map[i];

                                if (isstreq(e->url, HTTPD_PAGE_NOT_FOUND)) {
                                        if (e->http_on_message_complete) {
                                                e->http_on_message_complete(parser);
                                        }

                                        found = true;
                                        break;
                                }
                        }

                        if (not found) {
                                httpd_send_file(parser->data,
                                        HTTPD_CONTENT_TYPE_NOT_FOUND_TEXT_HTML,
                                        HTTPD_PAGE_NOT_FOUND);
                        }
                }
        }

        http->message_complete = true;

        free(self->multipart.buf);
        self->multipart.buf = NULL;

        return 0;
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
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct multipart *multipart = &self->multipart;

        multipart->buf_seek    = 0;
        multipart->write_count = 0;
        multipart->tref        = get_uptime_ms();

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
        http_thread_t *self = parser->data;

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                const httpd_page_map_t *e = &httpd_page_map[i];

                if (http->file_id != e->file_id) {
                        continue;
                }

                if (multipart->part_id != e->multipart_id) {
                        continue;
                }

                if (e->multipart_on_part_end) {
                        e->multipart_on_part_end(parser);
                }
                break;
        }

        DEBUG("%s: %llu ms\n", __func__, get_uptime_ms() - multipart->tref);

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
        http_thread_t *self = parser->data;

        DEBUG("%s: %lu bytes long\n", __func__, size);

        struct http_request *http      = &self->http_request;
        struct multipart    *multipart = &self->multipart;

        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                const httpd_page_map_t *e = &httpd_page_map[i];

                if (http->file_id != e->file_id) {
                        continue;
                }

                if (multipart->part_id != e->multipart_id) {
                        continue;
                }

                if (e->multipart_on_data) {
                        e->multipart_on_data(parser, data, size);
                }

                break;
        }

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
static int multipart_on_header_field(multipartparser *parser, const char *data,
        size_t size)
{
        http_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, size, data);

        struct multipart *multipart = &self->multipart;

        multipart->header_value_chunk = 0;

        if (multipart->header_field_chunk == 0) {
                free(multipart->header_field);
                multipart->header_field = strndup(data, size);
                multipart->header_field_chunk++;
        } else {
                size_t new_size         = strsize(multipart->header_field) + size;
                multipart->header_field = realloc(multipart->header_field, new_size);
                strlcat(multipart->header_field, data, new_size);
        }

        httpd_strtolower(multipart->header_field);

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
static int multipart_on_header_value(multipartparser *parser, const char *data,
        size_t size)
{
        http_thread_t *self = parser->data;

        DEBUG("%s: '%.*s'\n", __func__, size, data);

        struct multipart *multipart = &self->multipart;

        multipart->header_field_chunk = 0;

        if (isstreq(multipart->header_field, "content-disposition")) {
                if (multipart->header_value_chunk == 0) {
                        free(multipart->content_disposition);
                        multipart->content_disposition = strndup(data, size);
                        multipart->header_value_chunk++;

                } else {
                        size_t new_size = strsize(multipart->content_disposition) + size;
                        multipart->content_disposition = realloc(
                                multipart->content_disposition, new_size);
                        strlcat(multipart->content_disposition, data, new_size);
                }

                httpd_strtolower(multipart->content_disposition);

        } else if (isstreq(multipart->header_field, "content-type")) {
                if (multipart->header_value_chunk == 0) {
                        free(multipart->content_type);
                        multipart->content_type = strndup(data, size);
                        multipart->header_value_chunk++;
                } else {
                        size_t new_size         = strsize(multipart->content_type) + size;
                        multipart->content_type = realloc(multipart->content_type,
                                new_size);
                        strlcat(multipart->content_type, data, new_size);
                }

                httpd_strtolower(multipart->content_type);
        }

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
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        struct multipart *multipart = &self->multipart;

        multipart->part_id = httpd_multipart__none;

        for (size_t i = 0; i < ARRAY_SIZE(httpd_page_map); i++) {

                const httpd_page_map_t *e = &httpd_page_map[i];

                if (isstrempty(e->multipart_content_dispo)) {
                        continue;
                }

                if (strstr(multipart->content_disposition, e->multipart_content_dispo)) {

                        multipart->part_id = e->multipart_id;
                        break;
                }
        }

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
