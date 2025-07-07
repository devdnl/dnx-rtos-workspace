/*==============================================================================
@file    interpreter.c

@author  Daniel Zorychta

@brief   The HTTP server with script support - interpreter file.

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

#pragma GCC optimize("Ofast")

/*==============================================================================
  Include files
==============================================================================*/
#include "httpsrv.h"
#include <bs/bs.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void parse_params_to_lua_table(httpsrv_thread_t *self, const char *params);
static int open_lib_request(lua_State *L);
static int open_lib_ipc(lua_State *L);
static int luafn_echo(lua_State *L);
static int luafn_request_method(lua_State *L);
static int luafn_request_data(lua_State *L);
static int luafn_ipc_request(lua_State *L);
static int luafn_send_file(lua_State *L);

/*==============================================================================
  Local object definitions
==============================================================================*/
static const struct luaL_Reg basic_lib[] = {
        {"echo", luafn_echo},
        {NULL, NULL},
};

static const luaL_Reg request_lib[] = {
        {"method", luafn_request_method},
        {"data", luafn_request_data},
        {"send_file", luafn_send_file},
        {NULL, NULL},
};

static const luaL_Reg ipc_lib[] = {
        {"request", luafn_ipc_request},
        {NULL, NULL},
};

static const luaL_Reg http_libs[] = {
        {"request", open_lib_request},
        {"ipc", open_lib_ipc},
        {NULL, NULL},
};

static const luaL_Reg lua_libs[] = {
        {LUA_GNAME, luaopen_base},
        {LUA_TABLIBNAME, luaopen_table},
        {LUA_IOLIBNAME, luaopen_io},
        {LUA_OSLIBNAME, luaopen_os},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
        {NULL, NULL},
};

/*==============================================================================
  Exported object definitions
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Lua allocation function.
 *
 * @param  ud           user data
 * @param  ptr          pointer to free/resize
 * @param  osize        old size
 * @param  nsize        new size
 *
 * @return Pointer to object or NULL.
 */
//==============================================================================
static void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
        UNUSED_ARG1(ud);

        if (nsize == 0) {
                free(ptr);
                return NULL;

        } else {
                if (ptr) {
                        if (osize >= nsize) {
                                return ptr;

                        } else {
                                void *mem = malloc(nsize);
                                if (mem) {
                                        memcpy(mem, ptr, osize);
                                }

                                free(ptr);

                                return mem;
                        }
                } else {
                        return malloc(nsize);
                }
        }
}

//==============================================================================
/**
 * @brief  Initialize interpreter.
 *
 * @param  self         http thread
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int httpsrv_interpreter_init(httpsrv_thread_t *self)
{
        self->interpreter.ipc_socket = NULL;
        self->interpreter.object = NULL;

        self->interpreter.fetch_buf.buf_size = conf_get_interpreter_fetch_buffer_size();
        self->interpreter.fetch_buf.buf = malloc(self->interpreter.fetch_buf.buf_size);
        if (not self->interpreter.fetch_buf.buf) {
                httpsrv_interpreter_deinit(self);
                return ENOMEM;
        }

        self->interpreter.script.code_buf_size = conf_get_interpreter_code_buffer_size();
        self->interpreter.script.code_buf = malloc(self->interpreter.script.code_buf_size);
        if (not self->interpreter.script.code_buf) {
                httpsrv_interpreter_deinit(self);
                return ENOMEM;
        }

        self->interpreter.tmp_buf_size = conf_get_interpreter_tmp_buffer_size();
        self->interpreter.tmp_buf = malloc(self->interpreter.tmp_buf_size);
        if (not self->interpreter.tmp_buf) {
                httpsrv_interpreter_deinit(self);
                return ENOMEM;
        }

        lua_State *L = lua_newstate(lua_alloc, self);
        if (not L) {
                VERBOSE("Lua: cannot initialize\n");
                httpsrv_interpreter_deinit(self);
                return ENOMEM;
        }

        lua_pushlightuserdata(L, self);
        lua_setglobal(L, "_self");

        lua_pushstring(L, conf_get_root_dir());
        lua_setglobal(L, "root_dir");

        lua_pushstring(L, conf_get_upload_dir());
        lua_setglobal(L, "upload_dir");

        for (const luaL_Reg *lib = lua_libs; lib->func; lib++) {
                luaL_requiref(L, lib->name, lib->func, true);
                lua_pop(L, 1);
        }

        lua_getglobal(L, "_G");
        luaL_setfuncs(L, basic_lib, 0);
        lua_pop(L, 1);

        for (const luaL_Reg *lib = http_libs; lib->func; lib++) {
                luaL_requiref(L, lib->name, lib->func, true);
                lua_pop(L, 1);
        }

        self->interpreter.ipc_socket = socket_open("unix", NET_SOCKET_TYPE__DATAGRAM, 0);
        if (not self->interpreter.ipc_socket) {
                VERBOSE("unable to create IPC socket!\n");
                lua_close(L);
                return ECONNREFUSED;
        }

        NET_UDS_sockaddr_t sockaddr;
        snprintf(sockaddr.path, sizeof(sockaddr.path), "httpsrv.%u", gettid());
        if (socket_bind(self->interpreter.ipc_socket, &sockaddr, sizeof(sockaddr)) != 0) {
                VERBOSE("unable to bind IPC socket!\n");
                socket_close(self->interpreter.ipc_socket);
                lua_close(L);
                return EADDRINUSE;
        }

        self->interpreter.object = L;

        return 0;
}

//==============================================================================
/**
 * @brief  Deinitialize interpreter.
 *
 * @param  self         http thread
 */
//==============================================================================
void httpsrv_interpreter_deinit(httpsrv_thread_t *self)
{
        if (self->interpreter.ipc_socket) {
                socket_close(self->interpreter.ipc_socket);
                self->interpreter.ipc_socket = NULL;
        }

        if (self->interpreter.object) {
                lua_close(self->interpreter.object);
                self->interpreter.object = NULL;
        }

        free(self->interpreter.fetch_buf.buf);
        self->interpreter.fetch_buf.buf = NULL;
        self->interpreter.fetch_buf.buf_size = 0;

        free(self->interpreter.script.code_buf);
        self->interpreter.script.code_buf = NULL;
        self->interpreter.script.code_buf_size = 0;

        free(self->interpreter.tmp_buf);
        self->interpreter.tmp_buf = NULL;
        self->interpreter.tmp_buf_size = 0;
}

//==============================================================================
/**
 * @brief  Execute string.
 *
 * @param  self         http thread
 * @param  string       string to execute
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int httpsrv_interpreter_dostring(httpsrv_thread_t *self, const char *string)
{
        if (luaL_dostring(self->interpreter.object, string)) {
                const char *error_msg = lua_tostring(self->interpreter.object, -1);
                VERBOSE("%s\n", error_msg);
                // httpsrv_send(self, error_msg, strlen(error_msg), false);
                lua_pop(self->interpreter.object, 1);
                return EFAULT;
        } else {
                return ESUCC;
        }
}

//==============================================================================
/**
 * @brief  Execute file.
 *
 * @param  self         http thread
 * @param  file         file path to execute
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int httpsrv_interpreter_dofile(httpsrv_thread_t *self, const char *file)
{
        if (luaL_dofile(self->interpreter.object, file)) {
                const char *error_msg = lua_tostring(self->interpreter.object, -1);
                VERBOSE("%s\n", error_msg);
                // httpsrv_send(self, error_msg, strlen(error_msg), false);
                lua_pop(self->interpreter.object, 1);
                return EFAULT;
        } else {
                return ESUCC;
        }
}

//==============================================================================
/**
 * @brief  Get http_thread_t object.
 *
 * @param  L            Lua state
 *
 * @return http_thread_t pointer.
 */
//==============================================================================
static httpsrv_thread_t *get_http_thread_self(lua_State *L)
{
        lua_getglobal(L, "_self");
        httpsrv_thread_t *self = lua_touserdata(L, -1);
        lua_pop(L, 1);

        return self;
}

//==============================================================================
/**
 * @brief  Convert %xx character encoding.
 *
 * @param  buf          text buffer.
 */
//==============================================================================
static void convert_html_encoding(char *buf)
{
        char *in = buf;
        char *out = buf;

        while (*in != '\0') {
                if (*in == '%') {
                        in++;

                        char low = '0';
                        char high = '0';
                        u8_t byte = 0x00;

                        if (*in == '\0') {
                                break;
                        } else {
                                high = *in++;
                        }

                        if (*in == '\0') {
                                break;
                        } else {
                                low = *in++;
                        }

                        low = tolower(low);
                        high = tolower(high);

                        if ((high >= '0') and (high <= '9')) {
                                byte = (high - '0');
                        } else if ((high >= 'a') and (high <= 'f')) {
                                byte = (high - 'a') + 10;
                        }

                        byte <<= 4;

                        if ((low >= '0') and (low <= '9')) {
                                byte |= (low - '0');
                        } else if ((low >= 'a') and (low <= 'f')) {
                                byte |= (low - 'a') + 10;
                        }

                        *out++ = byte;

                } else if (*in == '+') {
                        *out++ = ' ';
                        in++;

                } else {
                        *out++ = *in++;
                }
        }

        *out = '\0';
}

//==============================================================================
/**
 * @brief  Parse GET or POST parameters to Lua table.
 *
 * @param  params       GET or POST parameters string
 */
//==============================================================================
static void parse_params_to_lua_table(httpsrv_thread_t *self, const char *params)
{
        lua_State *L = self->interpreter.object;

        lua_newtable(L);

        while (params and (*params != '\0')) {
                const char *key_begin = params;
                const char *key_end = strchr(key_begin, '=');
                size_t key_len = cast(uintptr_t, key_end) - cast(uintptr_t, key_begin) + 1;

                const char *val_begin = key_end + 1;
                const char *val_end = strchr(val_begin, '&');
                if (not val_end) {
                        val_end = val_begin + strlen(val_begin);
                }
                size_t val_len = cast(uintptr_t, val_end) - cast(uintptr_t, val_begin) + 1;

                strlcpy(self->interpreter.tmp_buf, val_begin, min(val_len, self->interpreter.tmp_buf_size));
                convert_html_encoding(self->interpreter.tmp_buf);
                lua_pushstring(L, self->interpreter.tmp_buf);

                strlcpy(self->interpreter.tmp_buf, key_begin, min(key_len, self->interpreter.tmp_buf_size));
                convert_html_encoding(self->interpreter.tmp_buf);
                lua_setfield(L, -2, self->interpreter.tmp_buf);

                if (*val_end == '&') {
                        params = val_end + 1;
                } else {
                        break;
                }
        }
}

//==============================================================================
/**
 * @brief  Open request library.
 *
 * @param  L            lua state
 *
 * @return 1.
 */
//==============================================================================
static int open_lib_request(lua_State *L)
{
        luaL_newlib(L, request_lib);
        return 1;
}

//==============================================================================
/**
 * @brief  Open ipc library.
 *
 * @param  L            lua state
 *
 * @return 1.
 */
//==============================================================================
static int open_lib_ipc(lua_State *L)
{
        luaL_newlib(L, ipc_lib);
        return 1;
}

//==============================================================================
/**
 * @brief  Put string to HTML page.
 *
 * @param  L            Lua state
 *
 * @return Number of results.
 */
//==============================================================================
static int luafn_echo(lua_State *L)
{
        httpsrv_thread_t *self = get_http_thread_self(L);

        size_t len = 0;

        int n = lua_gettop(L);
        for (int i = 1; i <= n; i++) {
                size_t l;
                const char *s = luaL_tolstring(L, i, &l);

                httpsrv_send(self, s, l, false);

                len += l;

                lua_pop(L, 1);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Return request method.
 *
 * @param  L            Lua state
 *
 * @return Number of results.
 */
//==============================================================================
static int luafn_request_method(lua_State *L)
{
        httpsrv_thread_t *self = get_http_thread_self(L);

        const char *method = "UNKNOWN";

        if (self->http_request.parser.method == HTTP_GET) {
                method = "GET";

        } else if (self->http_request.parser.method == HTTP_POST) {
                method = "POST";
        }

        lua_pushstring(L, method);

        return 1;
}

//==============================================================================
/**
 * @brief  Return request data (from POST or GET).
 *
 * @param  L            Lua state
 *
 * @return Number of results.
 */
//==============================================================================
static int luafn_request_data(lua_State *L)
{
        httpsrv_thread_t *self = get_http_thread_self(L);

        if (self->http_request.parser.method == HTTP_GET) {
                parse_params_to_lua_table(self, self->http_request.header.get_ref);

        } else if (self->http_request.parser.method == HTTP_POST) {
                parse_params_to_lua_table(self, self->http_request.header.body_data.value);
        }

        return 1;
}

//==============================================================================
/**
 * @brief  Send file.
 *         integer:number_of_sent_bytes send_file(string:path)
 *
 * @param  L            Lua state
 *
 * @return Number of results.
 */
//==============================================================================
static int luafn_send_file(lua_State *L)
{
        httpsrv_thread_t *self = get_http_thread_self(L);

        const char *path = luaL_checkstring(L, 1);

        int n = httpsrv_send_file(self, NULL, 0, path);

        lua_pushinteger(L, n);

        return 1;
}

//==============================================================================
/**
 * @brief  IPC request.
 *
 * @param  L            Lua state
 *
 * @return Number of results.
 */
//==============================================================================
static int luafn_ipc_request(lua_State *L)
{
        const char *address = luaL_checkstring(L, 1);
        luaL_checktype(L, 2, LUA_TTABLE);

        httpsrv_thread_t *self = get_http_thread_self(L);

        /*
         * Compose request. Lua table -> bs
         */
        bs_wr_t bs_wr;
        bs_write_chunk(&bs_wr, cast(u8_t *, self->interpreter.tmp_buf), self->interpreter.tmp_buf_size);

        lua_pushvalue(L, -1);

        lua_pushnil(L);

        while (lua_next(L, -2)) {
                lua_pushvalue(L, -2);

                if (not lua_isstring(L, -1)) {
                        luaL_error(L, "key string-type required!");
                        break;
                }

                const char *key = lua_tostring(L, -1);

                if (lua_type(L, -2) == LUA_TSTRING) {
                        bs_set_str(&bs_wr, key, lua_tostring(L, -2));

                } else if (lua_isboolean(L, -2)) {
                        bs_set_bool(&bs_wr, key, lua_toboolean(L, -2));

                } else if (lua_isnumber(L, -2)) {

                        lua_Number num = lua_tonumber(L, -2);

                        if (num == cast(int, num)) {
                                if ((num >= INT8_MIN) and (num <= INT8_MAX)) {
                                        bs_set_int8_t(&bs_wr, key, cast(int, num));

                                } else if ((num >= INT16_MIN) and (num <= INT16_MAX)) {
                                        bs_set_int16_t(&bs_wr, key, cast(int, num));

                                } else if ((num >= INT32_MIN) and (num <= INT32_MAX)) {
                                        bs_set_int32_t(&bs_wr, key, cast(int, num));

                                } else {
                                        bs_set_int64_t(&bs_wr, key, cast(int, num));
                                }
                        } else {
                                if (sizeof(lua_Number) == sizeof(float)) {
                                        bs_set_float(&bs_wr, key, lua_tonumber(L, -2));
                                } else {
                                        bs_set_double(&bs_wr, key, lua_tonumber(L, -2));
                                }
                        }

                } else {
                        luaL_error(L, "Type not supported!");
                }

                lua_pop(L, 2);
        }

        lua_pop(L, 1);

        int chunk_len = bs_get_chunk_length(&bs_wr);

        /*
         * Connect to selected address.
         */
        NET_UDS_sockaddr_t server_addr;
        strlcpy(server_addr.path, address, sizeof(server_addr.path));
        errno = 0;
        if (socket_sendto(self->interpreter.ipc_socket, self->interpreter.tmp_buf, chunk_len, 0, &server_addr,
                    sizeof(server_addr)) != chunk_len) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                return 2;
        }

        errno = 0;
        int n = socket_recvfrom(self->interpreter.ipc_socket, self->interpreter.tmp_buf,
                self->interpreter.tmp_buf_size, 0, &server_addr, sizeof(server_addr));
        if (n <= 0) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                return 2;
        }

        /*
         * Conver answer to Lua table.
         */
        lua_pushnumber(L, 0);
        lua_newtable(L);

        bs_rd_t bs_rd;
        bs_read_chunk(&bs_rd, cast(uint8_t *, self->interpreter.tmp_buf), n);

        uint8_t *next = NULL;
        const char *key;
        const void *value;
        uint16_t value_len;
        bs_type_t type;

        while (bs_get_next(&bs_rd, &next, &key, &value, &value_len, &type) == 0) {

                // value
                switch (type) {
                case bs_type__bool: {
                        bool v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushboolean(L, v);
                        break;
                }

                case bs_type__int8_t: {
                        int8_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__uint8_t: {
                        uint8_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__int16_t: {
                        int16_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__uint16_t: {
                        uint16_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__int32_t: {
                        int32_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__uint32_t: {
                        uint32_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__int64_t: {
                        int64_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__uint64_t: {
                        uint64_t v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushinteger(L, v);
                        break;
                }

                case bs_type__float: {
                        float v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushnumber(L, v);
                        break;
                }

                case bs_type__double: {
                        double v;
                        memcpy(&v, value, sizeof(v));
                        lua_pushnumber(L, v);
                        break;
                }

                case bs_type__string:
                        lua_pushstring(L, value);
                        break;

                case bs_type__blob:
                        lua_pushlstring(L, value, value_len);
                        break;

                default:
                        // not handled
                        lua_pushinteger(L, 0);
                        break;
                }

                // key
                lua_setfield(L, -2, key);
        }

        return 2;
}

/*==============================================================================
  End of file
==============================================================================*/
