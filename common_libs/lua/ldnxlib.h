/*
** ldnxlib.h
** dnx RTOS extensions of Lua (io.popen support, lfs module)
** See Copyright Notice in lua.h
*/

#ifndef ldnxlib_h
#define ldnxlib_h

#include <stdio.h>

#include "lua.h"


extern FILE *luadnx_popen (lua_State *L, const char *cmd, const char *mode);
extern int luadnx_pclose (lua_State *L, FILE *file);

#define LUA_LFSLIBNAME	"lfs"
LUAMOD_API int (luaopen_lfs) (lua_State *L);

#endif
