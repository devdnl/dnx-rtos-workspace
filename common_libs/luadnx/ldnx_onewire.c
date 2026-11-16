/*==============================================================================
File    ldnx_onewire.c

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services: dnx.onewire module (1-Wire bus,
        ONEWIRE driver).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*==============================================================================
  Include files
==============================================================================*/
#include "luadnx.h"
#include "lauxlib.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Push fail result: nil, message, errno.
 *
 * @param  L            Lua state
 * @param  err          error number
 *
 * @return Number of results.
 */
//==============================================================================
static int fail(lua_State *L, int err)
{
        err = err ? err : EINVAL;
        luaL_pushfail(L);
        lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
}

#ifdef IOCTL_ONEWIRE__TRANSFER
//==============================================================================
/**
 * @brief  Open device file of 1-Wire bus.
 *
 * @param  L            Lua state
 * @param  f            opened file
 *
 * @return 0 on success, otherwise errno.
 */
//==============================================================================
static int open_bus(lua_State *L, FILE **f)
{
        *f = fopen(luaL_checkstring(L, 1), "r+");
        return *f ? 0 : errno;
}

//==============================================================================
/**
 * @brief  Convert ROM code to hex string (bus order, 16 characters).
 */
//==============================================================================
static void push_rom(lua_State *L, const u8_t rom[8])
{
        char s[17];
        for (int i = 0; i < 8; i++) {
                snprintf(&s[i * 2], 3, "%02X", rom[i]);
        }
        lua_pushlstring(L, s, 16);
}

//==============================================================================
/**
 * @brief  Convert hex string to ROM code.
 */
//==============================================================================
static void check_rom(lua_State *L, int arg, u8_t rom[8])
{
        size_t len;
        const char *s = luaL_checklstring(L, arg, &len);
        luaL_argcheck(L, len == 16, arg, "ROM code of 16 hex digits expected");

        for (int i = 0; i < 8; i++) {
                char b[3] = {s[i * 2], s[i * 2 + 1], 0};
                char *end;
                rom[i] = strtoul(b, &end, 16);
                luaL_argcheck(L, *end == 0, arg, "ROM code of 16 hex digits expected");
        }
}
#endif

//==============================================================================
/**
 * @brief  dnx.onewire.reset(path): reset pulse.
 *
 * @param  L            Lua state
 *
 * @return Presence of devices (boolean), or nil, message, errno.
 */
//==============================================================================
static int ow_reset(lua_State *L)
{
#ifdef IOCTL_ONEWIRE__RESET
        FILE *f;
        int err = open_bus(L, &f);
        bool presence = false;
        if (!err) {
                if (ioctl(fileno(f), IOCTL_ONEWIRE__RESET, &presence) != 0) {
                        err = errno;
                }
                fclose(f);
        }

        if (err) {
                return fail(L, err);
        }

        lua_pushboolean(L, presence);
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.onewire.search(path [, alarm]): search of devices (SEARCH ROM,
 *         ALARM SEARCH if alarm is true).
 *
 * @param  L            Lua state
 *
 * @return Table of ROM codes (hex strings), or nil, message, errno.
 */
//==============================================================================
static int ow_search(lua_State *L)
{
#ifdef IOCTL_ONEWIRE__SEARCH
        FILE *f;
        int err = open_bus(L, &f);
        if (err) {
                return fail(L, err);
        }

        ONEWIRE_search_t s;
        memset(&s, 0, sizeof(s));
        s.cmd = lua_toboolean(L, 2) ? ONEWIRE_SEARCH__ALARM : ONEWIRE_SEARCH__ALL;

        lua_newtable(L);
        int n = 0;

        while (ioctl(fileno(f), IOCTL_ONEWIRE__SEARCH, &s) == 0) {
                push_rom(L, s.rom);
                lua_rawseti(L, -2, ++n);
        }

        err = (errno == ENOENT) ? 0 : errno;
        fclose(f);

        return err ? fail(L, err) : 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.onewire.transfer(path, rom, tx [, rxlen [, spu_ms]]):
 *         transaction: reset, MATCH ROM (rom: hex string) or SKIP ROM (rom:
 *         nil), data write, strong pull-up and data read.
 *
 * @param  L            Lua state
 *
 * @return Received data (string), or nil, message, errno.
 */
//==============================================================================
static int ow_transfer(lua_State *L)
{
#ifdef IOCTL_ONEWIRE__TRANSFER
        u8_t rom[8];
        bool match = !lua_isnoneornil(L, 2);
        if (match) {
                check_rom(L, 2, rom);
        }

        size_t txlen;
        const char *tx = luaL_optlstring(L, 3, "", &txlen);
        lua_Integer rxlen = luaL_optinteger(L, 4, 0);
        lua_Integer spu = luaL_optinteger(L, 5, 0);
        luaL_argcheck(L, rxlen >= 0 && rxlen <= 4096, 4, "invalid size");
        luaL_argcheck(L, spu >= 0 && spu <= 65535, 5, "invalid time");

        u8_t *rx = malloc(rxlen ? rxlen : 1);
        if (!rx) {
                return fail(L, ENOMEM);
        }

        ONEWIRE_transfer_t t = {
                .rom = match ? rom : NULL,
                .tx = tx,
                .tx_len = txlen,
                .rx = rx,
                .rx_len = rxlen,
                .spu_ms = spu,
        };

        FILE *f;
        int err = open_bus(L, &f);
        if (!err) {
                if (ioctl(fileno(f), IOCTL_ONEWIRE__TRANSFER, &t) != 0) {
                        err = errno;
                }
                fclose(f);
        }

        if (!err) {
                lua_pushlstring(L, (const char *)rx, rxlen);
        }

        free(rx);
        return err ? fail(L, err) : 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  Open dnx.onewire module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_onewire(lua_State *L)
{
        static const luaL_Reg onewire_lib[] = {
                {"reset", ow_reset},
                {"search", ow_search},
                {"transfer", ow_transfer},
                {NULL, NULL},
        };

        luaL_newlib(L, onewire_lib);
        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
