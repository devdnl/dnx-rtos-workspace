/*==============================================================================
File    lfs.c

Author  Daniel Zorychta

Brief   LuaFileSystem compatible module for dnx RTOS.

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
#include <dnx/misc.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define DIR_METATABLE "lfs.directory"
#define CWD_BUF_SIZE  256
#define CWD_BUF_MAX   4096
#define DEFAULT_MODE  0777

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        DIR *dir;
        bool closed;
} dir_data_t;

typedef struct {
        const char *name;
        void (*push)(lua_State *L, const struct stat *st);
} stat_member_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int fail(lua_State *L, int err);
static const char *mode_to_str(mode_t mode);
static void push_dev(lua_State *L, const struct stat *st);
static void push_ino(lua_State *L, const struct stat *st);
static void push_mode(lua_State *L, const struct stat *st);
static void push_nlink(lua_State *L, const struct stat *st);
static void push_uid(lua_State *L, const struct stat *st);
static void push_gid(lua_State *L, const struct stat *st);
static void push_rdev(lua_State *L, const struct stat *st);
static void push_access(lua_State *L, const struct stat *st);
static void push_modification(lua_State *L, const struct stat *st);
static void push_change(lua_State *L, const struct stat *st);
static void push_size(lua_State *L, const struct stat *st);
static void push_permissions(lua_State *L, const struct stat *st);
static void push_blocks(lua_State *L, const struct stat *st);
static void push_blksize(lua_State *L, const struct stat *st);

/*==============================================================================
  Local objects
==============================================================================*/
static const stat_member_t stat_members[] = {
        {"dev", push_dev},
        {"ino", push_ino},
        {"mode", push_mode},
        {"nlink", push_nlink},
        {"uid", push_uid},
        {"gid", push_gid},
        {"rdev", push_rdev},
        {"access", push_access},
        {"modification", push_modification},
        {"change", push_change},
        {"size", push_size},
        {"permissions", push_permissions},
        {"blocks", push_blocks},
        {"blksize", push_blksize},
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
        luaL_pushfail(L);
        lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
}

//==============================================================================
/**
 * @brief  Convert file mode to LuaFileSystem mode string.
 *
 * @param  mode         file mode
 *
 * @return Mode string.
 */
//==============================================================================
static const char *mode_to_str(mode_t mode)
{
        if (S_ISREG(mode)) {
                return "file";
        } else if (S_ISDIR(mode)) {
                return "directory";
        } else if (S_ISLNK(mode)) {
                return "link";
        } else if (S_ISDEV(mode)) {
                return "char device";
        } else if (S_ISFIFO(mode)) {
                return "named pipe";
        } else if (S_ISPROG(mode)) {
                return "program";
        } else {
                return "other";
        }
}

//==============================================================================
/**
 * @brief  Attribute push functions.
 *
 * @param  L            Lua state
 * @param  st           file information
 */
//==============================================================================
static void push_dev(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, st->st_dev);
}

static void push_ino(lua_State *L, const struct stat *st)
{
        UNUSED_ARG1(st);
        lua_pushinteger(L, 0);
}

static void push_mode(lua_State *L, const struct stat *st)
{
        lua_pushstring(L, mode_to_str(st->st_mode));
}

static void push_nlink(lua_State *L, const struct stat *st)
{
        UNUSED_ARG1(st);
        lua_pushinteger(L, 1);
}

static void push_uid(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, st->st_uid);
}

static void push_gid(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, st->st_gid);
}

static void push_rdev(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, S_ISDEV(st->st_mode) ? st->st_dev : 0);
}

static void push_access(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, cast(lua_Integer, st->st_mtime));
}

static void push_modification(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, cast(lua_Integer, st->st_mtime));
}

static void push_change(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, cast(lua_Integer, st->st_ctime));
}

static void push_size(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, cast(lua_Integer, st->st_size));
}

static void push_permissions(lua_State *L, const struct stat *st)
{
        static const char RWX[] = "rwxrwxrwx";
        char perm[sizeof(RWX)];

        for (size_t i = 0; i < sizeof(RWX) - 1; i++) {
                perm[i] = (st->st_mode & (0400 >> i)) ? RWX[i] : '-';
        }
        perm[sizeof(RWX) - 1] = '\0';

        lua_pushstring(L, perm);
}

static void push_blocks(lua_State *L, const struct stat *st)
{
        lua_pushinteger(L, cast(lua_Integer, (st->st_size + 511) / 512));
}

static void push_blksize(lua_State *L, const struct stat *st)
{
        UNUSED_ARG1(st);
        lua_pushinteger(L, 512);
}

//==============================================================================
/**
 * @brief  lfs.attributes(path [, request_name | result_table])
 *
 * @param  L            Lua state
 *
 * @return Attribute table, single attribute or nil, message, errno.
 */
//==============================================================================
static int lfs_attributes(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);

        struct stat st;
        if (stat(path, &st) != 0) {
                int err = errno;
                luaL_pushfail(L);
                lua_pushfstring(L, "cannot obtain information from file '%s': %s", path, strerror(err));
                lua_pushinteger(L, err);
                return 3;
        }

        if (lua_isstring(L, 2)) {
                const char *member = lua_tostring(L, 2);
                for (const stat_member_t *m = stat_members; m->name; m++) {
                        if (strcmp(m->name, member) == 0) {
                                m->push(L, &st);
                                return 1;
                        }
                }
                return luaL_argerror(L, 2, "invalid attribute name");
        }

        if (lua_istable(L, 2)) {
                lua_settop(L, 2);
        } else {
                lua_newtable(L);
        }

        for (const stat_member_t *m = stat_members; m->name; m++) {
                m->push(L, &st);
                lua_setfield(L, -2, m->name);
        }

        return 1;
}

//==============================================================================
/**
 * @brief  lfs.symlinkattributes(path [, request_name | result_table])
 *         There is no lstat() in dnx RTOS, so this is lfs.attributes().
 *
 * @param  L            Lua state
 *
 * @return Attribute table, single attribute or nil, message, errno.
 */
//==============================================================================
static int lfs_symlinkattributes(lua_State *L)
{
        return lfs_attributes(L);
}

//==============================================================================
/**
 * @brief  lfs.chdir(path)
 *
 * @param  L            Lua state
 *
 * @return true or nil, message, errno.
 */
//==============================================================================
static int lfs_chdir(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);

        if (chdir(path) != 0) {
                int err = errno;
                luaL_pushfail(L);
                lua_pushfstring(L, "unable to change working directory to '%s': %s", path, strerror(err));
                lua_pushinteger(L, err);
                return 3;
        }

        lua_pushboolean(L, true);
        return 1;
}

//==============================================================================
/**
 * @brief  lfs.currentdir()
 *
 * @param  L            Lua state
 *
 * @return Current working directory or nil, message, errno.
 */
//==============================================================================
static int lfs_currentdir(lua_State *L)
{
        for (size_t size = CWD_BUF_SIZE; size <= CWD_BUF_MAX; size *= 2) {
                luaL_Buffer b;
                char *buf = luaL_buffinitsize(L, &b, size);

                errno = 0;
                if (getcwd(buf, size)) {
                        luaL_pushresultsize(&b, strlen(buf));
                        return 1;
                }

                int err = errno;
                lua_pop(L, 1); // buffer box
                if (err != ERANGE) {
                        return fail(L, err ? err : EINVAL);
                }
        }

        return fail(L, ERANGE);
}

//==============================================================================
/**
 * @brief  lfs.mkdir(path [, mode])
 *
 * @param  L            Lua state
 *
 * @return true or nil, message, errno.
 */
//==============================================================================
static int lfs_mkdir(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);
        mode_t mode = cast(mode_t, luaL_optinteger(L, 2, DEFAULT_MODE));

        return luaL_fileresult(L, mkdir(path, mode) == 0, path);
}

//==============================================================================
/**
 * @brief  lfs.rmdir(path)
 *
 * @param  L            Lua state
 *
 * @return true or nil, message, errno.
 */
//==============================================================================
static int lfs_rmdir(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);

        struct stat st;
        if (stat(path, &st) != 0) {
                return luaL_fileresult(L, false, path);
        }

        if (not S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                return luaL_fileresult(L, false, path);
        }

        return luaL_fileresult(L, remove(path) == 0, path);
}

//==============================================================================
/**
 * @brief  lfs.touch(path [, atime [, mtime]])
 *         dnx RTOS has no utime(). File is created if it does not exist.
 *         Setting of explicit times is not supported.
 *
 * @param  L            Lua state
 *
 * @return true or nil, message, errno.
 */
//==============================================================================
static int lfs_touch(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);

        if (not lua_isnoneornil(L, 2) or not lua_isnoneornil(L, 3)) {
                return fail(L, ENOSYS);
        }

        FILE *file = fopen(path, "a+");
        if (not file) {
                return luaL_fileresult(L, false, path);
        }

        fclose(file);

        lua_pushboolean(L, true);
        return 1;
}

//==============================================================================
/**
 * @brief  Not supported functions: lfs.link(), lfs.lock(), lfs.unlock(),
 *         lfs.lock_dir().
 *
 * @param  L            Lua state
 *
 * @return nil, message, errno.
 */
//==============================================================================
static int lfs_not_supported(lua_State *L)
{
        return fail(L, ENOSYS);
}

//==============================================================================
/**
 * @brief  lfs.setmode(file, mode). There is no text mode in dnx RTOS.
 *
 * @param  L            Lua state
 *
 * @return true, "binary".
 */
//==============================================================================
static int lfs_setmode(lua_State *L)
{
        lua_pushboolean(L, true);
        lua_pushliteral(L, "binary");
        return 2;
}

//==============================================================================
/**
 * @brief  Close directory object.
 *
 * @param  d            directory data
 */
//==============================================================================
static void dir_close_data(dir_data_t *d)
{
        if (not d->closed) {
                d->closed = true;
                if (d->dir) {
                        closedir(d->dir);
                        d->dir = NULL;
                }
        }
}

//==============================================================================
/**
 * @brief  Directory iterator (also dir:next()).
 *
 * @param  L            Lua state
 *
 * @return Next entry name or nil.
 */
//==============================================================================
static int dir_iter(lua_State *L)
{
        dir_data_t *d = luaL_checkudata(L, 1, DIR_METATABLE);
        luaL_argcheck(L, not d->closed, 1, "closed directory");

        errno = 0;
        struct dirent *entry = readdir(d->dir);
        if (entry and entry->d_name) {
                lua_pushstring(L, entry->d_name);
                return 1;
        }

        dir_close_data(d);
        return 0;
}

//==============================================================================
/**
 * @brief  dir:close()
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int dir_close(lua_State *L)
{
        dir_data_t *d = luaL_checkudata(L, 1, DIR_METATABLE);
        dir_close_data(d);
        return 0;
}

//==============================================================================
/**
 * @brief  lfs.dir(path)
 *
 * @param  L            Lua state
 *
 * @return Iterator function, directory object, nil, directory object
 *         (to-be-closed value in generic for).
 */
//==============================================================================
static int lfs_dir(lua_State *L)
{
        const char *path = luaL_checkstring(L, 1);

        lua_pushcfunction(L, dir_iter);

        dir_data_t *d = lua_newuserdatauv(L, sizeof(dir_data_t), 0);
        d->closed = true;
        d->dir = NULL;
        luaL_setmetatable(L, DIR_METATABLE);

        d->dir = opendir(path);
        if (not d->dir) {
                int err = errno;
                return luaL_error(L, "cannot open %s: %s", path, strerror(err));
        }
        d->closed = false;

        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 4;
}

//==============================================================================
/**
 * @brief  Create directory object metatable.
 *
 * @param  L            Lua state
 */
//==============================================================================
static void dir_create_meta(lua_State *L)
{
        static const luaL_Reg methods[] = {
                {"next", dir_iter},
                {"close", dir_close},
                {NULL, NULL},
        };

        static const luaL_Reg metamethods[] = {
                {"__gc", dir_close},
                {"__close", dir_close},
                {NULL, NULL},
        };

        luaL_newmetatable(L, DIR_METATABLE);
        luaL_setfuncs(L, metamethods, 0);
        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
        lua_pop(L, 1);
}

//==============================================================================
/**
 * @brief  Open 'lfs' module (LuaFileSystem 1.8 compatible API).
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_lfs(lua_State *L)
{
        static const luaL_Reg lfs_lib[] = {
                {"attributes", lfs_attributes},
                {"chdir", lfs_chdir},
                {"currentdir", lfs_currentdir},
                {"dir", lfs_dir},
                {"link", lfs_not_supported},
                {"lock", lfs_not_supported},
                {"lock_dir", lfs_not_supported},
                {"mkdir", lfs_mkdir},
                {"rmdir", lfs_rmdir},
                {"setmode", lfs_setmode},
                {"symlinkattributes", lfs_symlinkattributes},
                {"touch", lfs_touch},
                {"unlock", lfs_not_supported},
                {NULL, NULL},
        };

        dir_create_meta(L);

        luaL_newlib(L, lfs_lib);

        lua_pushliteral(L, "LuaFileSystem 1.8.0 (dnx RTOS)");
        lua_setfield(L, -2, "_VERSION");

        lua_pushliteral(L, "LuaFileSystem API for dnx RTOS");
        lua_setfield(L, -2, "_DESCRIPTION");

        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
