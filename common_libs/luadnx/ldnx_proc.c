/*==============================================================================
File    ldnx_proc.c

Author  Daniel Zorychta

Brief   Lua module dnx.proc: process management.

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
#include "lualib.h"
#include <dnx/misc.h>
#include <dnx/thread.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define CMDLINE_LEN 256
#define CWD_LEN     256
#define MAX_THREADS 32

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
 *
 * @return Number of results.
 */
//==============================================================================
static int fail(lua_State *L)
{
        int err = errno ? errno : EINVAL;
        luaL_pushfail(L);
        lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
}

//==============================================================================
/**
 * @brief  Set integer field of table at top of stack.
 *
 * @param  L            Lua state
 * @param  name         field name
 * @param  value        value
 */
//==============================================================================
static void set_int(lua_State *L, const char *name, lua_Integer value)
{
        lua_pushinteger(L, value);
        lua_setfield(L, -2, name);
}

//==============================================================================
/**
 * @brief  Set CPU load field (1% = 10) as percent value.
 *
 * @param  L            Lua state
 * @param  name         field name
 * @param  value        value in 0.1% units
 */
//==============================================================================
static void set_load(lua_State *L, const char *name, u32_t value)
{
        lua_pushnumber(L, cast(lua_Number, value) / 10);
        lua_setfield(L, -2, name);
}

//==============================================================================
/**
 * @brief  Push process statistics table.
 *
 * @param  L            Lua state
 * @param  stat         process statistics
 */
//==============================================================================
static void push_process_stat(lua_State *L, const process_stat_t *stat)
{
        lua_newtable(L);
        set_int(L, "pid", stat->pid);
        lua_pushstring(L, stat->cmdline_buf);
        lua_setfield(L, -2, "cmdline");
        set_int(L, "memory", cast(lua_Integer, stat->memory_usage));
        set_int(L, "memory_blocks", stat->memory_block_count);
        set_int(L, "files", stat->files_count);
        set_int(L, "dirs", stat->dir_count);
        set_int(L, "mutexes", stat->mutexes_count);
        set_int(L, "semaphores", stat->semaphores_count);
        set_int(L, "queues", stat->queue_count);
        set_int(L, "sockets", stat->socket_count);
        set_int(L, "threads", stat->threads_count);
        set_load(L, "cpu", stat->CPU_load);
        set_int(L, "stack_size", stat->stack_size);
        set_int(L, "stack_max_usage", stat->stack_max_usage);
        set_int(L, "priority", stat->priority);
        set_int(L, "syscalls_per_sec", stat->syscalls_per_sec);
        lua_pushboolean(L, stat->threads_count == 0);
        lua_setfield(L, -2, "zombie");
}

//==============================================================================
/**
 * @brief  Prepare process statistics container with command line buffer
 *         allocated as Lua userdata (pushed on the stack).
 *
 * @param  L            Lua state
 * @param  stat         process statistics
 */
//==============================================================================
static void prepare_process_stat(lua_State *L, process_stat_t *stat)
{
        memset(stat, 0, sizeof(*stat));
        stat->cmdline_buf = lua_newuserdatauv(L, CMDLINE_LEN, 0);
        stat->cmdline_len = CMDLINE_LEN;
        stat->cmdline_buf[0] = '\0';
}

//==============================================================================
/**
 * @brief  Get stdio attribute from options table field. Field can be:
 *         nil (default descriptor), false (no stream), path string or Lua
 *         file handle.
 *
 * @param  L            Lua state
 * @param  opts         options table index
 * @param  name         field name
 * @param  def_fd       default descriptor
 * @param  create       create file if path is given
 * @param  fd           descriptor (output)
 * @param  path         path (output)
 *
 * @return One of errno value.
 */
//==============================================================================
static int get_stdio_attr(lua_State *L, int opts, const char *name, int def_fd, bool create, int *fd,
        const char **path)
{
        int err = 0;

        *fd = def_fd;
        *path = NULL;

        if (opts == 0) {
                return err;
        }

        int type = lua_getfield(L, opts, name);

        if (type == LUA_TNIL) {
                // default descriptor

        } else if (type == LUA_TBOOLEAN and not lua_toboolean(L, -1)) {
                *fd = -1;

        } else if (type == LUA_TSTRING) {
                *fd = -1;
                *path = lua_tostring(L, -1);

                if (create) {
                        FILE *f = fopen(*path, "a");
                        if (f) {
                                fclose(f);
                        } else {
                                err = errno ? errno : EINVAL;
                        }
                }

        } else {
                luaL_Stream *stream = luaL_testudata(L, -1, LUA_FILEHANDLE);
                if (stream and stream->f and stream->closef) {
                        fflush(stream->f);
                        *fd = fileno(stream->f);
                } else {
                        luaL_error(L, "option '%s': path, file handle or false expected", name);
                }
        }

        // NOTE: string stays referenced by options table, it is safe to pop it
        lua_pop(L, 1);
        return err;
}

//==============================================================================
/**
 * @brief  Create process. Options table at index 2 (optional):
 *         stdin, stdout, stderr -- path, file handle or false,
 *         cwd -- working directory, priority -- integer,
 *         detached -- boolean.
 *
 * @param  L            Lua state
 *
 * @return PID or 0 on error (errno is set).
 */
//==============================================================================
static pid_t spawn(lua_State *L)
{
        const char *cmd = luaL_checkstring(L, 1);
        int opts = lua_istable(L, 2) ? 2 : 0;

        if (not opts) {
                luaL_argcheck(L, lua_isnoneornil(L, 2), 2, "options table expected");
        }

        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.priority = PRIORITY_NORMAL;
        attr.detached = false;

        if (opts) {
                lua_getfield(L, opts, "detached");
                attr.detached = lua_toboolean(L, -1);
                lua_pop(L, 1);

                lua_getfield(L, opts, "priority");
                attr.priority = cast(i16_t, luaL_optinteger(L, -1, PRIORITY_NORMAL));
                lua_pop(L, 1);
                luaL_argcheck(L, (attr.priority >= PRIORITY_LOWEST) and (attr.priority <= PRIORITY_HIGHEST),
                        2, "priority out of range");
        }

        int err = 0;
        err = err ? err
                  : get_stdio_attr(L, opts, "stdin", attr.detached ? -1 : fileno(stdin), false,
                            &attr.fd_stdin, &attr.p_stdin);
        err = err ? err
                  : get_stdio_attr(L, opts, "stdout", fileno(stdout), true, &attr.fd_stdout, &attr.p_stdout);
        err = err ? err
                  : get_stdio_attr(L, opts, "stderr", fileno(stderr), true, &attr.fd_stderr, &attr.p_stderr);

        if (err) {
                errno = err;
                return 0;
        }

        fflush(stdout);
        fflush(stderr);

        const char *cwd = NULL;
        if (opts and (lua_getfield(L, opts, "cwd") != LUA_TNIL)) {
                cwd = luaL_checkstring(L, -1); // stays referenced by options table
        }
        lua_pop(L, opts ? 1 : 0);

        if (not cwd) {
                char *buf = lua_newuserdatauv(L, CWD_LEN, 0);
                cwd = getcwd(buf, CWD_LEN) ? buf : "/";
        }

        attr.cwd = cwd;

        errno = 0;
        return process_create(cmd, &attr);
}

//==============================================================================
/**
 * @brief  dnx.proc.getpid()
 *
 * @param  L            Lua state
 *
 * @return PID of current process.
 */
//==============================================================================
static int proc_getpid(lua_State *L)
{
        lua_pushinteger(L, process_getpid());
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.spawn(cmd [, opts])
 *         Start program (without shell) and return immediately.
 *
 * @param  L            Lua state
 *
 * @return PID or nil, message, errno.
 */
//==============================================================================
static int proc_spawn(lua_State *L)
{
        pid_t pid = spawn(L);
        if (pid == 0) {
                return fail(L);
        }

        lua_pushinteger(L, pid);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.wait(pid [, timeout_ms])
 *
 * @param  L            Lua state
 *
 * @return Exit status or nil, message, errno (ETIME on timeout).
 */
//==============================================================================
static int proc_wait(lua_State *L)
{
        pid_t pid = cast(pid_t, luaL_checkinteger(L, 1));
        lua_Integer timeout = luaL_optinteger(L, 2, MAX_DELAY_MS);
        luaL_argcheck(L, (timeout >= 0) and (timeout <= cast(lua_Integer, MAX_DELAY_MS)), 2,
                "timeout out of range");

        int status = -1;
        errno = 0;
        if (process_wait(pid, &status, cast(u32_t, timeout)) != 0) {
                return fail(L);
        }

        lua_pushinteger(L, status);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.run(cmd [, opts])
 *         Start program (without shell) and wait for it.
 *
 * @param  L            Lua state
 *
 * @return Exit status or nil, message, errno.
 */
//==============================================================================
static int proc_run(lua_State *L)
{
        pid_t pid = spawn(L);
        if (pid == 0) {
                return fail(L);
        }

        int status = -1;
        errno = 0;
        if (process_wait(pid, &status, MAX_DELAY_MS) != 0) {
                return fail(L);
        }

        lua_pushinteger(L, status);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.kill(pid)
 *
 * @param  L            Lua state
 *
 * @return true or nil, message, errno.
 */
//==============================================================================
static int proc_kill(lua_State *L)
{
        pid_t pid = cast(pid_t, luaL_checkinteger(L, 1));

        errno = 0;
        if (process_kill(pid) != 0) {
                return fail(L);
        }

        lua_pushboolean(L, true);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.stat(pid)
 *
 * @param  L            Lua state
 *
 * @return Process statistics table or nil, message, errno.
 */
//==============================================================================
static int proc_stat(lua_State *L)
{
        pid_t pid = cast(pid_t, luaL_checkinteger(L, 1));

        process_stat_t stat;
        prepare_process_stat(L, &stat);

        errno = 0;
        if (process_stat(pid, &stat) != 0) {
                return fail(L);
        }

        push_process_stat(L, &stat);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.list()
 *
 * @param  L            Lua state
 *
 * @return Array of process statistics tables.
 */
//==============================================================================
static int proc_list(lua_State *L)
{
        process_stat_t stat;
        prepare_process_stat(L, &stat);

        lua_newtable(L);

        lua_Integer n = 0;
        for (size_t seek = 0; process_stat_seek(seek, &stat) == 0; seek++) {
                push_process_stat(L, &stat);
                lua_rawseti(L, -2, ++n);

                stat.cmdline_buf[0] = '\0';
        }

        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.threads(pid)
 *
 * @param  L            Lua state
 *
 * @return Array of thread statistics: tid, cpu, stack_size, stack_max_usage,
 *         priority, syscalls_per_sec.
 */
//==============================================================================
static int proc_threads(lua_State *L)
{
        pid_t pid = cast(pid_t, luaL_checkinteger(L, 1));

        lua_newtable(L);

        lua_Integer n = 0;
        for (tid_t tid = 0; tid < MAX_THREADS; tid++) {
                thread_stat_t stat;
                if (thread_stat(pid, tid, &stat) == 0) {
                        lua_newtable(L);
                        set_int(L, "tid", stat.tid);
                        set_load(L, "cpu", stat.CPU_load);
                        set_int(L, "stack_size", stat.stack_size);
                        set_int(L, "stack_max_usage", stat.stack_max_usage);
                        set_int(L, "priority", stat.priority);
                        set_int(L, "syscalls_per_sec", stat.syscalls_per_sec);
                        lua_rawseti(L, -2, ++n);
                }
        }

        return 1;
}

//==============================================================================
/**
 * @brief  dnx.proc.priority([pid])
 *
 * @param  L            Lua state
 *
 * @return Process priority.
 */
//==============================================================================
static int proc_priority(lua_State *L)
{
        pid_t pid = cast(pid_t, luaL_optinteger(L, 1, process_getpid()));
        lua_pushinteger(L, process_get_priority(pid));
        return 1;
}

//==============================================================================
/**
 * @brief  Open 'dnx.proc' module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_proc(lua_State *L)
{
        static const luaL_Reg proc_lib[] = {
                {"getpid", proc_getpid},
                {"kill", proc_kill},
                {"list", proc_list},
                {"priority", proc_priority},
                {"run", proc_run},
                {"spawn", proc_spawn},
                {"stat", proc_stat},
                {"threads", proc_threads},
                {"wait", proc_wait},
                {NULL, NULL},
        };

        luaL_newlib(L, proc_lib);

        lua_pushinteger(L, PRIORITY_LOWEST);
        lua_setfield(L, -2, "PRIORITY_LOWEST");
        lua_pushinteger(L, PRIORITY_NORMAL);
        lua_setfield(L, -2, "PRIORITY_NORMAL");
        lua_pushinteger(L, PRIORITY_HIGHEST);
        lua_setfield(L, -2, "PRIORITY_HIGHEST");

        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
