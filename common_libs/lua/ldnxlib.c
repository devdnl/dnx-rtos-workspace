/*
** ldnxlib.c
** dnx RTOS extensions of Lua:
**  - io.popen()/close() implementation (see l_popen in luaconf.h),
**  - "lfs" module (subset of LuaFileSystem).
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
#include <dirent.h>
#include <sys/stat.h>
#include <dnx/os.h>
#include <dnx/thread.h>

#include "lua.h"
#include "lauxlib.h"
#include "ldnxlib.h"


#define POPEN_DIR      "/run/"
#define POPEN_REGISTRY "dnx.popen"
#define POPEN_SEQ      "dnx.popen.seq"
#define CWD_LEN        256
#define LFS_DIR_META   "lfs.dir"


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


/*
** {======================================================
** lfs module
** =======================================================
*/

typedef struct {
  DIR *dir;
} lfs_dir_t;


static int lfs_result (lua_State *L, int ok, const char *path) {
  if (ok) {
    lua_pushboolean(L, 1);
    return 1;
  }
  return luaL_fileresult(L, 0, path);
}


static int dir_close (lua_State *L) {
  lfs_dir_t *d = (lfs_dir_t *)luaL_checkudata(L, 1, LFS_DIR_META);
  if (d->dir) {
    closedir(d->dir);
    d->dir = NULL;
  }
  return 0;
}


static int dir_iter (lua_State *L) {
  lfs_dir_t *d = (lfs_dir_t *)luaL_checkudata(L, 1, LFS_DIR_META);
  luaL_argcheck(L, d->dir != NULL, 1, "closed directory");

  struct dirent *entry = readdir(d->dir);
  if (entry) {
    lua_pushstring(L, entry->d_name);
    return 1;
  }

  closedir(d->dir);
  d->dir = NULL;
  return 0;
}


/* lfs.dir(path): iterator, directory object (closed at end of loop) */
static int lfs_dir (lua_State *L) {
  const char *path = luaL_checkstring(L, 1);

  lua_pushcfunction(L, dir_iter);

  lfs_dir_t *d = (lfs_dir_t *)lua_newuserdatauv(L, sizeof(lfs_dir_t), 0);
  d->dir = NULL;
  luaL_setmetatable(L, LFS_DIR_META);

  d->dir = opendir(path);
  if (d->dir == NULL) {
    return luaL_error(L, "cannot open %s: %s", path, strerror(errno));
  }

  lua_pushnil(L);
  lua_pushvalue(L, -2);  /* closing value */
  return 4;
}


static const char *mode_name (mode_t mode) {
  if (S_ISREG(mode)) return "file";
  if (S_ISDIR(mode)) return "directory";
  if (S_ISLNK(mode)) return "link";
  if (S_ISFIFO(mode)) return "named pipe";
  if (S_ISDEV(mode)) return "char device";
  if (S_ISPROG(mode)) return "program";
  return "other";
}


/* lfs.attributes(path [, name | table]) */
static int lfs_attributes (lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  struct stat st;

  if (stat(path, &st) != 0) {
    return luaL_fileresult(L, 0, path);
  }

  if (lua_isstring(L, 2)) {
    const char *name = lua_tostring(L, 2);

    if (strcmp(name, "mode") == 0) lua_pushstring(L, mode_name(st.st_mode));
    else if (strcmp(name, "size") == 0) lua_pushinteger(L, (lua_Integer)st.st_size);
    else if (strcmp(name, "modification") == 0) lua_pushinteger(L, (lua_Integer)st.st_mtime);
    else if (strcmp(name, "change") == 0) lua_pushinteger(L, (lua_Integer)st.st_ctime);
    else if (strcmp(name, "access") == 0) lua_pushinteger(L, (lua_Integer)st.st_mtime);
    else if (strcmp(name, "dev") == 0) lua_pushinteger(L, (lua_Integer)st.st_dev);
    else if (strcmp(name, "uid") == 0) lua_pushinteger(L, (lua_Integer)st.st_uid);
    else if (strcmp(name, "gid") == 0) lua_pushinteger(L, (lua_Integer)st.st_gid);
    else return luaL_argerror(L, 2, "invalid attribute name");

    return 1;
  }

  if (lua_istable(L, 2)) {
    lua_settop(L, 2);
  } else {
    lua_newtable(L);
  }

  lua_pushstring(L, mode_name(st.st_mode));
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, (lua_Integer)st.st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, (lua_Integer)st.st_mtime);
  lua_setfield(L, -2, "modification");
  lua_pushinteger(L, (lua_Integer)st.st_mtime);
  lua_setfield(L, -2, "access");
  lua_pushinteger(L, (lua_Integer)st.st_ctime);
  lua_setfield(L, -2, "change");
  lua_pushinteger(L, (lua_Integer)st.st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, (lua_Integer)st.st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, (lua_Integer)st.st_gid);
  lua_setfield(L, -2, "gid");

  return 1;
}


static int lfs_currentdir (lua_State *L) {
  char *cwd = malloc(CWD_LEN);
  if (cwd == NULL) {
    return luaL_error(L, "not enough memory");
  }

  if (getcwd(cwd, CWD_LEN)) {
    lua_pushstring(L, cwd);
    free(cwd);
    return 1;
  }

  free(cwd);
  return luaL_fileresult(L, 0, NULL);
}


static int lfs_chdir (lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  return lfs_result(L, chdir(path) == 0, path);
}


static int lfs_mkdir (lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  return lfs_result(L, mkdir(path, 0777) == 0, path);
}


static int lfs_rmdir (lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  return lfs_result(L, remove(path) == 0, path);
}


static const luaL_Reg lfs_funcs[] = {
  {"dir", lfs_dir},
  {"attributes", lfs_attributes},
  {"symlinkattributes", lfs_attributes},
  {"currentdir", lfs_currentdir},
  {"chdir", lfs_chdir},
  {"mkdir", lfs_mkdir},
  {"rmdir", lfs_rmdir},
  {NULL, NULL}
};


static const luaL_Reg dir_meta[] = {
  {"__gc", dir_close},
  {"__close", dir_close},
  {"close", dir_close},
  {"next", dir_iter},
  {NULL, NULL}
};


LUAMOD_API int luaopen_lfs (lua_State *L) {
  luaL_newmetatable(L, LFS_DIR_META);
  luaL_setfuncs(L, dir_meta, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newlib(L, lfs_funcs);
  lua_pushliteral(L, "LuaFileSystem (dnx subset) 1.0");
  lua_setfield(L, -2, "_VERSION");
  return 1;
}

/* }====================================================== */
