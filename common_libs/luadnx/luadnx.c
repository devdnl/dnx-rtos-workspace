/*==============================================================================
File    luadnx.c

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services: module registration.

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

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
static const luaL_Reg preload_libs[] = {
        {"lfs", luaopen_lfs},
        {"dnx", luaopen_dnx},
        {"dnx.sys", luaopen_dnx_sys},
        {"dnx.proc", luaopen_dnx_proc},
        {NULL, NULL},
};

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
 * @brief  Open 'dnx' module. Module contains sub-modules 'sys' and 'proc'
 *         that are also available directly by require("dnx.sys") and
 *         require("dnx.proc").
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx(lua_State *L)
{
        lua_newtable(L);

        luaL_requiref(L, "dnx.sys", luaopen_dnx_sys, 0);
        lua_setfield(L, -2, "sys");

        luaL_requiref(L, "dnx.proc", luaopen_dnx_proc, 0);
        lua_setfield(L, -2, "proc");

        lua_pushliteral(L, LUADNX_VERSION);
        lua_setfield(L, -2, "_VERSION");

        return 1;
}

//==============================================================================
/**
 * @brief  Register all luadnx modules in package.preload. Modules are loaded
 *         when required (require("lfs"), require("dnx"), ...). Function
 *         should be called after luaL_openlibs().
 *
 * @param  L            Lua state
 */
//==============================================================================
void luadnx_openlibs(lua_State *L)
{
        luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);

        for (const luaL_Reg *lib = preload_libs; lib->func; lib++) {
                lua_pushcfunction(L, lib->func);
                lua_setfield(L, -2, lib->name);
        }

        lua_pop(L, 1);
}

/*==============================================================================
  End of file
==============================================================================*/
