/*==============================================================================
File    ldnx_spi.c

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services: dnx.spi module (SPI transfers).

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
  Local macros
==============================================================================*/
#define MAX_FRAMES 16

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

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
 * @brief  Push standard error result: nil, message, errno.
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

//==============================================================================
/**
 * @brief  dnx.spi.transceive(path, frame [, frame ...]): full duplex transfer
 *         of frames (strings) by SPI device file. All frames are transferred
 *         while the device is selected (CS is active from the first byte of
 *         the first frame to the last byte of the last frame).
 *
 * @param  L            Lua state
 *
 * @return Received data (one string for each frame, of the frame length), or
 *         nil, message, errno.
 */
//==============================================================================
static int spi_transceive(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);
        int frames = lua_gettop(L) - 1;

        luaL_argcheck(L, frames >= 1, 2, "frame expected");
        luaL_argcheck(L, frames <= MAX_FRAMES, MAX_FRAMES + 2, "too many frames");

        size_t total = 0;
        for (int i = 0; i < frames; i++) {
                size_t len;
                luaL_checklstring(L, i + 2, &len);
                luaL_argcheck(L, len > 0, i + 2, "empty frame");
                total += len;
        }

#ifdef IOCTL_SPI__TRANSCEIVE
        SPI_transceive_t tr[MAX_FRAMES];
        memset(tr, 0, sizeof(tr));

        u8_t *rx = malloc(total);
        if (!rx) {
                return fail(L, ENOMEM);
        }

        size_t pos = 0;
        for (int i = 0; i < frames; i++) {
                size_t len;
                tr[i].tx_buffer = (const u8_t *)lua_tolstring(L, i + 2, &len);
                tr[i].rx_buffer = &rx[pos];
                tr[i].count = len;
                tr[i].separated = false;
                tr[i].next = (i + 1 < frames) ? &tr[i + 1] : NULL;
                pos += len;
        }

        int err = 0;
        FILE *f = fopen(path, "r+");
        if (f) {
                if (ioctl(fileno(f), IOCTL_SPI__TRANSCEIVE, tr) != 0) {
                        err = errno;
                }
                fclose(f);
        } else {
                err = errno;
        }

        if (err) {
                free(rx);
                return fail(L, err);
        }

        pos = 0;
        for (int i = 0; i < frames; i++) {
                lua_pushlstring(L, (const char *)&rx[pos], tr[i].count);
                pos += tr[i].count;
        }

        free(rx);
        return frames;
#else
        (void)path;
        (void)total;
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  Open dnx.spi module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_spi(lua_State *L)
{
        static const luaL_Reg spi_lib[] = {
                {"transceive", spi_transceive},
                {NULL, NULL},
        };

        luaL_newlib(L, spi_lib);
        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
