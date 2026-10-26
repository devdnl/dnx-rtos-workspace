/*
** ldnxlib.c
** dnx RTOS extensions of Lua:
**  - io.popen()/close() implementation (see l_popen in luaconf.h).
** The "lfs" module is provided by the luadnx library (common_libs/luadnx).
**
** io.popen() runs program directly (without shell), arguments are split by
** kernel (quoted arguments are supported). Program and Lua do not run
** concurrently: in read mode the program is finished before popen() returns
** (output is stored in temporary file), in write mode the program is started
** when file is closed (input is taken from temporary file).
**
** Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
** See Copyright Notice in lua.h
*/

#define ldnxlib_c
#define LUA_LIB

#include "lprefix.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/os.h>
#include <dnx/thread.h>

#include "lua.h"
#include "lauxlib.h"
#include "ldnxlib.h"


#define POPEN_DIR      "/run/"
#define POPEN_REGISTRY "dnx.popen"
#define POPEN_SEQ      "dnx.popen.seq"
#define CWD_LEN        256


/*
** {======================================================
** io.popen
** =======================================================
*/

/* push popen registry table */
static void popen_registry (lua_State *L) {
  luaL_getsubtable(L, LUA_REGISTRYINDEX, POPEN_REGISTRY);
}


/* create temporary file name (unique in this Lua state) */
static void popen_tmpname (lua_State *L, char *buf, size_t size) {
  lua_getfield(L, LUA_REGISTRYINDEX, POPEN_SEQ);
  lua_Integer seq = lua_tointeger(L, -1) + 1;
  lua_pop(L, 1);
  lua_pushinteger(L, seq);
  lua_setfield(L, LUA_REGISTRYINDEX, POPEN_SEQ);

  snprintf(buf, size, POPEN_DIR "lua-popen-%lx-%x-%x", (unsigned long)(uintptr_t)L,
           (unsigned)get_uptime_ms(), (unsigned)seq);
}


/* run program with selected standard input and output; returns exit status or -1 */
static int popen_run (const char *cmd, FILE *in, FILE *out) {
  char *cwd = malloc(CWD_LEN);
  if (cwd == NULL) {
    errno = ENOMEM;
    return -1;
  }

  if (getcwd(cwd, CWD_LEN) == NULL) {
    strcpy(cwd, "/");
  }

  process_attr_t attr = {
    .fd_stdin  = fileno(in ? in : stdin),
    .fd_stdout = fileno(out ? out : stdout),
    .fd_stderr = fileno(stderr),
    .p_stdin   = NULL,
    .p_stdout  = NULL,
    .p_stderr  = NULL,
    .cwd       = cwd,
    .priority  = PRIORITY_NORMAL,
    .detached  = false
  };

  int status = -1;
  pid_t pid = process_create(cmd, &attr);
  if (pid) {
    if (process_wait(pid, &status, MAX_DELAY_MS) != 0) {
      status = -1;
    }
  }

  free(cwd);

  return status;
}


/*
** Start program. Read mode: program output is stored in temporary file which
** is returned. Write mode: temporary file is returned, program is started by
** luadnx_pclose(). Information needed by close is stored in registry.
*/
FILE *luadnx_popen (lua_State *L, const char *cmd, const char *mode) {
  char path[64];
  popen_tmpname(L, path, sizeof(path));

  FILE *f = NULL;
  int status = 0;

  if (mode[0] == 'r') {
    FILE *out = fopen(path, "w");
    if (out == NULL) {
      return NULL;
    }

    fflush(stdout);
    status = popen_run(cmd, NULL, out);
    int err = errno;
    fclose(out);

    if (status < 0) {
      remove(path);
      errno = err ? err : ENOENT;
      return NULL;
    }

    f = fopen(path, "r");

  } else {
    f = fopen(path, "w");
  }

  if (f == NULL) {
    int err = errno;
    remove(path);
    errno = err;
    return NULL;
  }

  /* registry[f] = {path, mode, cmd, status} */
  popen_registry(L);
  lua_createtable(L, 0, 4);
  lua_pushstring(L, path);
  lua_setfield(L, -2, "path");
  lua_pushstring(L, mode);
  lua_setfield(L, -2, "mode");
  lua_pushstring(L, cmd);
  lua_setfield(L, -2, "cmd");
  lua_pushinteger(L, status);
  lua_setfield(L, -2, "status");
  lua_rawsetp(L, -2, f);
  lua_pop(L, 1);

  return f;
}


/*
** Close file opened by luadnx_popen(); returns exit status of program
** (or -1 on error).
*/
int luadnx_pclose (lua_State *L, FILE *file) {
  popen_registry(L);

  if (lua_rawgetp(L, -1, file) != LUA_TTABLE) {
    lua_pop(L, 2);
    fclose(file);
    return -1;
  }

  lua_getfield(L, -1, "path");
  lua_getfield(L, -2, "mode");
  lua_getfield(L, -3, "cmd");
  lua_getfield(L, -4, "status");

  int status = (int)lua_tointeger(L, -1);
  const char *cmd = lua_tostring(L, -2);
  const char *mode = lua_tostring(L, -3);
  const char *path = lua_tostring(L, -4);

  fclose(file);

  if (mode[0] == 'w') {
    FILE *in = fopen(path, "r");
    if (in) {
      status = popen_run(cmd, in, NULL);
      fclose(in);
    } else {
      status = -1;
    }
  }

  remove(path);

  lua_pop(L, 5);  /* path, mode, cmd, status, entry */
  lua_pushnil(L);
  lua_rawsetp(L, -2, file);
  lua_pop(L, 1);  /* registry */

  errno = 0;
  return status;
}

/* }====================================================== */
