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

// #pragma GCC optimize("Og")

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
static int  open_lib_request(lua_State *L);
static int  open_lib_ipc(lua_State *L);
static int  luafn_echo(lua_State *L);
static int  luafn_request_method(lua_State *L);
static int  luafn_request_data(lua_State *L);
static int  luafn_ipc_request(lua_State *L);

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

/*==============================================================================
  Exported object definitions
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

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
        self->interpreter.object = luaL_newstate();
        lua_State *L             = self->interpreter.object;
        if (not L) {
                VERBOSE("Lua: cannot initialize\n");
                return ENOMEM;
        }

        lua_pushlightuserdata(L, self);
        lua_setglobal(L, "_self");

        lua_pushstring(L, HTTPSRV_ROOT_DIR);
        lua_setglobal(L, "root_dir");

        luaL_openlibs(L);

        lua_getglobal(L, "_G");
        luaL_setfuncs(L, basic_lib, 0);
        lua_pop(L, 1);

        for (const luaL_Reg *lib = http_libs; lib->func; lib++) {
                luaL_requiref(L, lib->name, lib->func, true);
                lua_pop(L, 1);
        }

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
        lua_close(self->interpreter.object);
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
                const char *key_end   = strchr(key_begin, '=');
                size_t      key_len   = cast(uintptr_t, key_end) - cast(uintptr_t, key_begin) + 1;

                const char *val_begin = key_end + 1;
                const char *val_end   = strchr(val_begin, '&');
                if (not val_end) {
                        val_end = val_begin + strlen(val_begin);
                }
                size_t val_len = cast(uintptr_t, val_end) - cast(uintptr_t, val_begin) + 1;

                strlcpy(self->interpreter.tmp_buf, val_begin,
                        min(val_len, sizeof(self->interpreter.tmp_buf)));
                lua_pushstring(L, self->interpreter.tmp_buf);

                strlcpy(self->interpreter.tmp_buf, key_begin,
                        min(key_len, sizeof(self->interpreter.tmp_buf)));
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
                size_t      l;
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
        bs_write_chunk(&bs_wr, cast(u8_t *, self->interpreter.tmp_buf), sizeof(self->interpreter.tmp_buf));

        lua_pushvalue(L, -1);

        lua_pushnil(L);

        while (lua_next(L, -2)) {
                lua_pushvalue(L, -2);

                if (not lua_isstring(L, -1)) {
                        luaL_error(L, "key string-type required!");
                        break;
                }

                const char *key = lua_tostring(L, -1);

                if (lua_isinteger(L, -2)) {
                        bs_set_int32_t(&bs_wr, key, lua_tointeger(L, -2));

                } else if (lua_isnumber(L, -2)) {
                        bs_set_float(&bs_wr, key, lua_tonumber(L, -2));

                } else if (lua_isstring(L, -2)) {
                        bs_set_str(&bs_wr, key, lua_tostring(L, -2));

                } else if (lua_isboolean(L, -2)) {
                        bs_set_bool(&bs_wr, key, lua_toboolean(L, -2));

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
        errno          = 0;
        SOCKET *socket = socket_open("unix", NET_PROTOCOL__DATAGRAM);
        if (not socket) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                socket_close(socket);
                return 2;
        }

        NET_UDS_sockaddr_t sockaddr;
        strlcpy(sockaddr.path, "httpsrv", sizeof(sockaddr.path));
        errno = 0;
        if (socket_bind(socket, &sockaddr, sizeof(sockaddr)) != 0) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                socket_close(socket);
                return 2;
        }

        NET_UDS_sockaddr_t server_addr;
        strlcpy(server_addr.path, address, sizeof(server_addr.path));
        errno = 0;
        if (socket_sendto(socket, self->interpreter.tmp_buf, chunk_len, 0, &server_addr,
                    sizeof(server_addr)) != chunk_len) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                socket_close(socket);
                return 2;
        }

        errno = 0;
        int n = socket_recvfrom(socket, self->interpreter.tmp_buf, sizeof(self->interpreter.tmp_buf), 0,
                &server_addr, sizeof(server_addr));
        if (n <= 0) {
                lua_pushnumber(L, errno);
                lua_pushnil(L);
                socket_close(socket);
                return 2;
        }

        socket_close(socket);

        /*
         * Conver answer to Lua table.
         */
        lua_pushnumber(L, 0);
        lua_newtable(L);

        bs_rd_t bs_rd;
        bs_read_chunk(&bs_rd, cast(uint8_t *, self->interpreter.tmp_buf), n);

        uint8_t    *next = NULL;
        const char *key;
        const void *value;
        bs_type_t   type;

        while (bs_get_next(&bs_rd, &next, &key, &value, &type) == 0) {

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
