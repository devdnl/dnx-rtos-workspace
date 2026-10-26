/*==============================================================================
File    ldnx_sys.c

Author  Daniel Zorychta

Brief   Lua module dnx.sys: system information and control.

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
#include <dnx/os.h>
#include <dnx/thread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define HOSTNAME_LEN     64
#define SYSLOG_MSG_LEN   256
#define SYSLOG_MAX_LINES 1000

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
 * @brief  Set string field of table at top of stack.
 *
 * @param  L            Lua state
 * @param  name         field name
 * @param  value        value
 */
//==============================================================================
static void set_str(lua_State *L, const char *name, const char *value)
{
        lua_pushstring(L, value ? value : "");
        lua_setfield(L, -2, name);
}

//==============================================================================
/**
 * @brief  Set boolean field of table at top of stack.
 *
 * @param  L            Lua state
 * @param  name         field name
 * @param  value        value
 */
//==============================================================================
static void set_bool(lua_State *L, const char *name, bool value)
{
        lua_pushboolean(L, value);
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
 * @brief  dnx.sys.uptime()
 *
 * @param  L            Lua state
 *
 * @return System uptime in milliseconds.
 */
//==============================================================================
static int sys_uptime(lua_State *L)
{
        lua_pushinteger(L, cast(lua_Integer, get_uptime_ms()));
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.hostname()
 *
 * @param  L            Lua state
 *
 * @return Host name or nil, message, errno.
 */
//==============================================================================
static int sys_hostname(lua_State *L)
{
        char name[HOSTNAME_LEN];

        errno = 0;
        if (get_host_name(name, sizeof(name)) != 0) {
                return fail(L);
        }

        name[sizeof(name) - 1] = '\0';
        lua_pushstring(L, name);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.info()
 *
 * @param  L            Lua state
 *
 * @return Table: os_name, os_version, os_commit, kernel_name, kernel_version,
 *         platform, hostname, cpu_cores.
 */
//==============================================================================
static int sys_info(lua_State *L)
{
        lua_newtable(L);
        set_str(L, "os_name", get_OS_name());
        set_str(L, "os_version", get_OS_version());
        set_str(L, "os_commit", get_OS_commithash());
        set_str(L, "kernel_name", get_kernel_name());
        set_str(L, "kernel_version", get_kernel_version());
        set_str(L, "platform", get_platform_name());

        char name[HOSTNAME_LEN];
        if (get_host_name(name, sizeof(name)) == 0) {
                name[sizeof(name) - 1] = '\0';
                set_str(L, "hostname", name);
        }

        u8_t cores = 1;
        if (thread_get_cpu_core_count(&cores) == 0) {
                set_int(L, "cpu_cores", cores);
        }

        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.memory()
 *
 * @param  L            Lua state
 *
 * @return Table with memory usage in bytes or nil, message, errno.
 */
//==============================================================================
static int sys_memory(lua_State *L)
{
        memstat_t mem;

        errno = 0;
        if (get_memory_usage_details(&mem) != 0) {
                return fail(L);
        }

        lua_newtable(L);
        set_int(L, "size", mem.memory_size);
        set_int(L, "free", mem.free_memory);
        set_int(L, "used", mem.used_memory);
        set_int(L, "static", mem.static_memory_usage);
        set_int(L, "kernel", mem.kernel_memory_usage);
        set_int(L, "filesystems", mem.filesystems_memory_usage);
        set_int(L, "network", mem.network_memory_usage);
        set_int(L, "modules", mem.modules_memory_usage);
        set_int(L, "programs", mem.programs_memory_usage);
        set_int(L, "shared", mem.shared_memory_usage);
        set_int(L, "cached", mem.cached_memory_usage);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.regions()
 *
 * @param  L            Lua state
 *
 * @return Array of memory regions: name, address, size, heap_size,
 *         heap_usage, dma, cache, noheap.
 */
//==============================================================================
static int sys_regions(lua_State *L)
{
        lua_newtable(L);

        mem_region_info_t info;
        for (size_t idx = 0; get_memory_region_info(idx, &info) == 0; idx++) {
                lua_newtable(L);
                set_str(L, "name", info.name_ref);
                set_int(L, "address", info.address);
                set_int(L, "size", info.size);
                set_int(L, "heap_size", info.heap_size);
                set_int(L, "heap_usage", info.heap_usage);
                set_bool(L, "dma", info.flags & MEM_FLAG__DMA);
                set_bool(L, "cache", info.flags & MEM_FLAG__CACHE);
                set_bool(L, "noheap", info.flags & MEM_FLAG__NOHEAP);
                lua_rawseti(L, -2, cast(lua_Integer, idx + 1));
        }

        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.cpuload()
 *
 * @param  L            Lua state
 *
 * @return Table with average CPU load in percent: avg1sec, avg1min, avg5min,
 *         avg15min or nil, message, errno.
 */
//==============================================================================
static int sys_cpuload(lua_State *L)
{
        avg_CPU_load_t load;

        errno = 0;
        if (get_average_CPU_load(&load) != 0) {
                return fail(L);
        }

        lua_newtable(L);
        set_load(L, "avg1sec", load.avg1sec);
        set_load(L, "avg1min", load.avg1min);
        set_load(L, "avg5min", load.avg5min);
        set_load(L, "avg15min", load.avg15min);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.syslog([from_us])
 *         Read system log messages newer than selected timestamp.
 *
 * @param  L            Lua state
 *
 * @return Array of messages {time = us, msg = string} and timestamp of the
 *         last message (in microseconds) to be used in the next call.
 */
//==============================================================================
static int sys_syslog(lua_State *L)
{
        lua_Integer from = luaL_optinteger(L, 1, 0);
        luaL_argcheck(L, from >= 0, 1, "negative timestamp");

        struct timeval t;
        t.tv_sec = cast(time_t, from / 1000000);
        t.tv_usec = cast(suseconds_t, from % 1000000);

        char *msg = lua_newuserdatauv(L, SYSLOG_MSG_LEN, 0);
        lua_newtable(L);

        int n = 0;
        while ((n < SYSLOG_MAX_LINES) and syslog_read(msg, SYSLOG_MSG_LEN, &t, &t)) {
                msg[SYSLOG_MSG_LEN - 1] = '\0';

                lua_newtable(L);
                set_int(L, "time", cast(lua_Integer, t.tv_sec) * 1000000 + t.tv_usec);
                set_str(L, "msg", msg);
                lua_rawseti(L, -2, ++n);
        }

        lua_pushinteger(L, cast(lua_Integer, t.tv_sec) * 1000000 + t.tv_usec);
        return 2;
}

//==============================================================================
/**
 * @brief  dnx.sys.syslog_clear()
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int sys_syslog_clear(lua_State *L)
{
        UNUSED_ARG1(L);
        syslog_clear();
        return 0;
}

//==============================================================================
/**
 * @brief  dnx.sys.panicinfo()
 *         Note: kernel clears the report when it is read.
 *
 * @param  L            Lua state
 *
 * @return Table with last kernel panic/application crash information or nil.
 */
//==============================================================================
static int sys_panicinfo(lua_State *L)
{
        kernel_panic_info_t info;
        memset(&info, 0, sizeof(info));

        if (not get_kernel_panic_info(&info)) {
                lua_pushnil(L);
                return 1;
        }

        info.name[sizeof(info.name) - 1] = '\0';

        lua_newtable(L);
        set_str(L, "name", info.name);
        set_str(L, "cause", info.cause_str);
        set_int(L, "cause_code", info.cause);
        set_int(L, "pid", info.pid);
        set_int(L, "tid", info.tid);
        set_int(L, "syscall", info.syscall);
        set_int(L, "core", info.core);
        set_bool(L, "kernel_space", info.flags & KERNEL_PANIC_FLAG__KERNEL_SPACE);
        set_bool(L, "kernel_panic", info.flags & KERNEL_PANIC_FLAG__KERNEL_PANIC);
        set_bool(L, "panic_loop", info.flags & KERNEL_PANIC_FLAG__PANIC_LOOP);
        return 1;
}

//==============================================================================
/**
 * @brief  dnx.sys.msleep(ms)
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int sys_msleep(lua_State *L)
{
        lua_Integer ms = luaL_checkinteger(L, 1);
        if (ms > 0) {
                msleep(cast(u32_t, ms));
        }
        return 0;
}

//==============================================================================
/**
 * @brief  dnx.sys.sleep(seconds), seconds can be fractional.
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int sys_sleep(lua_State *L)
{
        lua_Number s = luaL_checknumber(L, 1);
        if (s > 0) {
                msleep(cast(u32_t, s * 1000));
        }
        return 0;
}

//==============================================================================
/**
 * @brief  dnx.sys.reboot()
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int sys_reboot(lua_State *L)
{
        UNUSED_ARG1(L);
        sync();
        system_reboot();
        return 0;
}

//==============================================================================
/**
 * @brief  dnx.sys.shutdown()
 *
 * @param  L            Lua state
 *
 * @return 0 results.
 */
//==============================================================================
static int sys_shutdown(lua_State *L)
{
        UNUSED_ARG1(L);
        sync();
        system_shutdown();
        return 0;
}

//==============================================================================
/**
 * @brief  Open 'dnx.sys' module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_sys(lua_State *L)
{
        static const luaL_Reg sys_lib[] = {
                {"cpuload", sys_cpuload},
                {"hostname", sys_hostname},
                {"info", sys_info},
                {"memory", sys_memory},
                {"msleep", sys_msleep},
                {"panicinfo", sys_panicinfo},
                {"reboot", sys_reboot},
                {"regions", sys_regions},
                {"shutdown", sys_shutdown},
                {"sleep", sys_sleep},
                {"syslog", sys_syslog},
                {"syslog_clear", sys_syslog_clear},
                {"uptime", sys_uptime},
                {NULL, NULL},
        };

        luaL_newlib(L, sys_lib);
        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
