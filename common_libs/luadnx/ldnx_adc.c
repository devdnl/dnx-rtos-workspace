/*==============================================================================
File    ldnx_adc.c

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services: dnx.adc module (analog-digital
        converter, ADC driver).

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
#include <string.h>
#include <sys/ioctl.h>

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Push fail result: nil, message, errno.
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

#ifdef IOCTL_ADC__READ_CHANNEL
//==============================================================================
/**
 * @brief  Execute request on converter file (path: argument 1).
 */
//==============================================================================
static int request(lua_State *L, int rq, void *arg)
{
        FILE *f = fopen(luaL_checkstring(L, 1), "r");
        if (!f) {
                return errno;
        }

        int err = (ioctl(fileno(f), rq, arg) != 0) ? errno : 0;
        fclose(f);
        return err;
}
#endif

//==============================================================================
/**
 * @brief  dnx.adc.read(path, channel): conversion of channel (number, "temp"
 *         or "vref": internal channels).
 *
 * @return Result (raw), voltage [V], or nil, message, errno.
 */
//==============================================================================
static int adc_read(lua_State *L)
{
#ifdef IOCTL_ADC__READ_CHANNEL
        ADC_measurement_t m;
        memset(&m, 0, sizeof(m));

        if (lua_type(L, 2) == LUA_TSTRING) {
                static const char *const NAMES[] = {"temp", "vref", NULL};
                m.channel = (luaL_checkoption(L, 2, NULL, NAMES) == 0) ? ADC_CHANNEL__TEMP
                                                                       : ADC_CHANNEL__VREFINT;
        } else {
                lua_Integer ch = luaL_checkinteger(L, 2);
                luaL_argcheck(L, ch >= 0 && ch < 0xF0, 2, "invalid channel");
                m.channel = ch;
        }

        int err = request(L, IOCTL_ADC__READ_CHANNEL, &m);
        if (err) {
                return fail(L, err);
        }

        lua_pushinteger(L, m.raw);
        lua_pushnumber(L, m.uV / 1e6);
        return 2;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.adc.temperature(path): temperature of chip.
 *
 * @return Temperature [C], or nil, message, errno.
 */
//==============================================================================
static int adc_temperature(lua_State *L)
{
#ifdef IOCTL_ADC__READ_TEMPERATURE
        i32_t mdeg = 0;
        int err = request(L, IOCTL_ADC__READ_TEMPERATURE, &mdeg);
        if (err) {
                return fail(L, err);
        }

        lua_pushnumber(L, mdeg / 1000.0);
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.adc.info(path): converter information.
 *
 * @return Table {resolution_max, resolution, channels, temp, vrefint, vref_mv,
 *         oversampling, hw_oversampling, sample_time_ns}, or nil, message,
 *         errno.
 */
//==============================================================================
static int adc_info(lua_State *L)
{
#ifdef IOCTL_ADC__GET_INFO
        ADC_info_t info;
        memset(&info, 0, sizeof(info));

        int err = request(L, IOCTL_ADC__GET_INFO, &info);
        if (err) {
                return fail(L, err);
        }

        lua_createtable(L, 0, 9);
        lua_pushinteger(L, info.resolution_max);
        lua_setfield(L, -2, "resolution_max");
        lua_pushinteger(L, info.resolution);
        lua_setfield(L, -2, "resolution");
        lua_pushinteger(L, info.channels);
        lua_setfield(L, -2, "channels");
        lua_pushboolean(L, info.temp);
        lua_setfield(L, -2, "temp");
        lua_pushboolean(L, info.vrefint);
        lua_setfield(L, -2, "vrefint");
        lua_pushinteger(L, info.vref_mV);
        lua_setfield(L, -2, "vref_mv");
        lua_pushinteger(L, info.oversampling);
        lua_setfield(L, -2, "oversampling");
        lua_pushboolean(L, info.hw_oversampling);
        lua_setfield(L, -2, "hw_oversampling");
        lua_pushinteger(L, info.sample_time_ns);
        lua_setfield(L, -2, "sample_time_ns");
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  Open dnx.adc module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_adc(lua_State *L)
{
        static const luaL_Reg adc_lib[] = {
                {"read", adc_read},
                {"temperature", adc_temperature},
                {"info", adc_info},
                {NULL, NULL},
        };

        luaL_newlib(L, adc_lib);
        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
