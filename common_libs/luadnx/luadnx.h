/*==============================================================================
File    luadnx.h

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services (lfs, dnx.sys, dnx.proc).

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

/**
 * @defgroup LUADNX luadnx
 *
 * Lua modules giving access to dnx RTOS services that are not covered by the
 * Lua standard library:
 * - @b lfs      -- LuaFileSystem compatible module (directories, attributes),
 * - @b dnx.sys  -- system information, memory, CPU load, syslog, reboot,
 * - @b dnx.proc -- processes (spawn, wait, kill, list, statistics).
 *
 * Modules are loaded on demand by require():
 * @code
 * local lfs = require("lfs")
 * local dnx = require("dnx")   -- dnx.sys and dnx.proc
 * local sys = require("dnx.sys")
 * @endcode
 *
 * Library does not use global or static variables (appset rule).
 *
 * @{
 */

#ifndef _LUADNX_H_
#define _LUADNX_H_

/*==============================================================================
  Include files
==============================================================================*/
#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#define LUADNX_VERSION "luadnx 1.0"

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern int luaopen_lfs(lua_State *L);
extern int luaopen_dnx(lua_State *L);
extern int luaopen_dnx_sys(lua_State *L);
extern int luaopen_dnx_proc(lua_State *L);
extern void luadnx_openlibs(lua_State *L);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* _LUADNX_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
