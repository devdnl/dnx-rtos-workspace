/*==============================================================================
@file   os.h

@author Daniel Zorychta

@brief  dnx system main header

        MIT License

        Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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
\defgroup dnx-os-h <dnx/os.h>

This library contains macros and functions related directly with dnx RTOS system.

*/
/**@{*/

#ifndef _LIBC_DNX_LIBC_DNX_OS_H_
#define _LIBC_DNX_LIBC_DNX_OS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <stdbool.h>
#include <libc/source/syscall.h>
#include <libc/include/sys/types.h>

/*==============================================================================
  Exported symbolic constants/macros
==============================================================================*/
#ifndef _COMMIT_HASH
#define _COMMIT_HASH ""
#endif

#define MEM_FLAG__DMA                   (1 << 0)
#define MEM_FLAG__CACHE                 (1 << 1)
#define MEM_FLAG__NOHEAP                (1 << 2)

#define KERNEL_PANIC_FLAG__KERNEL_SPACE (1 << 0)
#define KERNEL_PANIC_FLAG__KERNEL_PANIC (1 << 1)
#define KERNEL_PANIC_FLAG__PANIC_LOOP   (1 << 2)

/*==============================================================================
  Exported types, enums definitions
==============================================================================*/
/**
 * @brief Memory usage details
 *
 * The type contains details of dynamic and static memory usage
 *
 * @see get_memory_usage_details()
 */
typedef struct {
        uint32_t memory_size;
        uint32_t free_memory;
        uint32_t used_memory;
        int32_t  static_memory_usage;      /*!< The amount of memory that is used statically at build time.*/
        int32_t  kernel_memory_usage;      /*!< The amount of memory used by kernel.*/
        int32_t  filesystems_memory_usage; /*!< The amount of memory used by file systems.*/
        int32_t  network_memory_usage;     /*!< The amount of memory used by network subsystem.*/
        int32_t  modules_memory_usage;     /*!< The amount of memory used by modules (drivers).*/
        int32_t  programs_memory_usage;    /*!< The amount of memory used by users' programs (applications).*/
        int32_t  shared_memory_usage;      /*!< The amount of memory used by shared buffers.*/
        int32_t  cached_memory_usage;      /*!< The amount of memory used by disc caches.*/
} memstat_t;

/**
 * @brief Memory region information
 *
 * The type contains memory region information.
 *
 * @see get_memory_region_info()
 */
typedef struct {
        const char *name_ref;
        u32_t       flags;
        u32_t       address;
        u32_t       size;
        u32_t       heap_size;
        u32_t       heap_usage;
} mem_region_info_t;

/** average CPU load */
typedef struct {
        u32_t avg1sec;  //!< average CPU load within 1 second (1% = 10)
        u32_t avg1min;  //!< average CPU load within 1 minute (1% = 10)
        u32_t avg5min;  //!< average CPU load within 5 minutes (1% = 10)
        u32_t avg15min; //!< average CPU load within 15 minutes (1% = 10)
} avg_CPU_load_t;

/**
 * @brief Kernel Panic info
 *
 * The type contains information about last occurred kernel panic.
 *
 * @see get_kernel_panic_info()
 */
typedef struct {
        const char *cause_str;
        char        name[64];
        u32_t       flags;
        int         cause;
        pid_t       pid;
        tid_t       tid;
        u8_t        syscall;
        u8_t        core;
} kernel_panic_info_t;

/*==============================================================================
  Exported object declarations
==============================================================================*/

/*==============================================================================
  External object declarations
==============================================================================*/

/*==============================================================================
  Exported function prototypes
==============================================================================*/

/*==============================================================================
  Exported inline functions
==============================================================================*/

//==============================================================================
/**
 * @brief Function returns memory usage details.
 *
 * The function get_memory_usage_details() return detailed memory usage
 * pointed by <i>stat</i>.
 *
 * @param stat      memory information
 *
 * @exception | @ref EINVAL
 *
 * @return Return @b 0 on success. On error, @b -1 value
 * is returned, and @b errno is set appropriately.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        errno = 0;
        memstat_t stat;
        if (get_memory_usage_details(&stat) == 0) {
                printf("Used memory by kernel      : %d\n"
                       "Used memory by file system : %d\n"
                       "Used memory by network     : %d\n"
                       "Used memory by modules     : %d\n"
                       "Used memory by applications: %d\n",
                       stat.kernel_memory_usage,
                       stat.filesystems_memory_usage,
                       stat.network_memory_usage,
                       stat.modules_memory_usage,
                       stat.applications_memory_usage);
        } else {
                perror(NULL);
        }

        // ...

   @endcode
 */
//==============================================================================
static inline int get_memory_usage_details(memstat_t *stat)
{
        int err = _libc_syscall(_LIBC_SYS_GETMEMDETAILS, stat);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function returns memory region information.
 *
 * The function get_memory_region_info() return memory region information indexed
 * by <i>idx</i>. The result is stored in pointed by <i>info</i> variable.
 *
 * @param idx           region index
 * @param info          region information
 *
 * @exception | @ref ENOENT
 *
 * @return Return @b 0 on success. On error, @b -1 value
 * is returned, and @b errno is set appropriately.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        size_t idx = 0;
        mem_region_info_t info;
        while (get_memory_region_info(idx++, &info) == 0) {
                printf("Region name  : %s\n"
                       "Flags        : %08Xh\n"
                       "Address      : %08Xh\n"
                       "Size         : %lu bytes\n"
                       "Heap size    : %lu bytes\n"
                       "Heap usage   : %lu bytes\n",
                       info.name_ref,
                       info.flags,
                       info.address,
                       info.size,
                       info.heap_size,
                       info.heap_usage);
        }

        // ...

   @endcode
 */
//==============================================================================
static inline int get_memory_region_info(size_t idx, mem_region_info_t *info)
{
        int err = _libc_syscall(_LIBC_SYS_MEMREGIONINFO, idx, info);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function returns memory usage of selected module (driver).
 *
 * The function get_module_memory_usage() return memory usage by specified
 * module number <i>module_number</i>.
 *
 * @param module_number     module number
 * @param usage             usage destination buffer
 *
 * @return On success \b 0 is returned. On error, \b -1 is returned.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        uint number_of_modules = get_number_of_modules();
        for (uint i = 0; i < number_of_modules; i++) {
                int32_t usage;
                get_module_memory_usage(i);
                printf("%s : %ld\n", get_module_name(i), usage);
        }

        // ...

   @endcode
 */
//==============================================================================
static inline int get_driver_memory_usage(uint module_number, int32_t *usage)
{
        int err = _libc_syscall(_LIBC_SYS_GETMODMEMUSAGE, &module_number, usage);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function returns up time in milliseconds.
 *
 * The function get_uptime_ms() return number of milliseconds which
 * elapsed after kernel start. Function is similar to get_uptime(), except
 * that return milliseconds instead of seconds. In this function the tick
 * counter value is calculated to milliseconds, what means that resolution of
 * this counter depends on system tick counter increase value.
 *
 * @return System work time in milliseconds.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("System works by %llu ms\n", get_uptime_ms());

        // ...

   @endcode
 *
 * @see get_uptime(), clock()
 */
//==============================================================================
static inline clock_t get_uptime_ms(void)
{
        clock_t uptime = 0;
        int     err    = _libc_syscall(_LIBC_SYS_GETUPTIMEMS, &uptime);
        return err ? (clock_t)(-1LL) : uptime;
}

//==============================================================================
/**
 * @brief Function returns an average CPU load.
 *
 * The function get_average_CPU_load() fill average CPU load container
 * pointed by <i>avg_CPU_load</i>.
 *
 * @param avg_CPU_load          average CPU load container
 *
 * @return On success \b 0 is returned, otherwise \b -1.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        avg_CPU_load_t avg;
        get_average_CPU_load(&avg);

        printf("Average CPU load: %2d.%d%%, %2d.%d%%, %2d.%d%%\n",
                avg.min1  / 10, avg.min1  % 10,
                avg.min5  / 10, avg.min5  % 10,
                avg.min15 / 10, avg.min15 % 10);

        // ...

   @endcode
 */
//==============================================================================
static inline int get_average_CPU_load(avg_CPU_load_t *avg_CPU_load)
{
        int err = _libc_syscall(_LIBC_SYS_GETAVGCPULOAD, avg_CPU_load);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function returns the name of current platform.
 *
 * The function get_platform_name() return platform name on which dnx RTOS
 * is working.
 *
 * @return Platform name.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Platform name: %s\n", get_platform_name());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_platform_name(void)
{
        const char *name = NULL;
        int         err  = _libc_syscall(_LIBC_SYS_GETPLATFORMNAME, &name);
        return err ? "" : name;
}

//==============================================================================
/**
 * @brief Function returns the operating system name.
 *
 * The function get_OS_name() return operating system name (dnx RTOS).
 *
 * @return Operating system name.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Operating system name: %s\n", get_OS_name());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_OS_name(void)
{
        const char *buf = NULL;
        int         err = _libc_syscall(_LIBC_SYS_GETOSNAME, &buf);
        return err ? "" : buf;
}

//==============================================================================
/**
 * @brief Function returns an operating system version.
 *
 * The function get_OS_version() return operating system version string.
 *
 * @return Operating system version string.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Operating system version: %s\n", get_OS_version());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_OS_version(void)
{
        const char *buf = NULL;
        int         err = _libc_syscall(_LIBC_SYS_GETOSVER, &buf);
        return err ? "" : buf;
}

//==============================================================================
/**
 * @brief Function returns a commit hash of operating system version.
 *
 * The function get_OS_commithash() return a commit hash string.
 *
 * @return Commit hash string.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Commit hash: %s\n", get_OS_commithash());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_OS_commithash(void)
{
        const char *buf = NULL;
        int         err = _libc_syscall(_LIBC_SYS_GETOSCOMMITHASH, &buf);
        return err ? "" : buf;
}

//==============================================================================
/**
 * @brief Function returns name of used kernel.
 *
 * The function get_kernel_name() return kernel name string.
 *
 * @return Kernel name string.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Kernel name: %s\n", get_kernel_name());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_kernel_name(void)
{
        const char *buf = NULL;
        int         err = _libc_syscall(_LIBC_SYS_GETKERNELNAME, &buf);
        return err ? "" : buf;
}

//==============================================================================
/**
 * @brief Funciton returns version of used kernel.
 *
 * The function get_kernel_version() return kernel version string.
 *
 * @return Kernel version string.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        printf("Kernel version: %s\n", get_kernel_version());

        // ...

   @endcode
 */
//==============================================================================
static inline const char *get_kernel_version(void)
{
        const char *buf = NULL;
        int         err = _libc_syscall(_LIBC_SYS_GETKERNELVER, &buf);
        return err ? "" : buf;
}

//==============================================================================
/**
 * @brief Function returns host name string.
 *
 * The function get_host_name() return host name string configured in
 * configuration files.
 *
 * @return On success 0 is returned, otherwise -1.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        char hostname[64];
        get_host_name(hostname, sizeof(hostname));
        printf("Host name: '%s'\n", hostname);

        // ...

   @endcode
 */
//==============================================================================
static inline int get_host_name(char *hostname, size_t hostname_len)
{
        int err = _libc_syscall(_LIBC_SYS_GETHOSTNAME, hostname, &hostname_len);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function restarts the system.
 *
 * The function restart system. Function does not write unsaved data.
 *
 * @return This function does not return.
 *
 * @b Example
 * @code
        #include <dnx/os.h>
        #include <unistd.h>

        // ...

        sync();

        // e.g. user requests system restart
        system_reboot();

   @endcode
 */
//==============================================================================
static inline void system_reboot(void)
{
        _libc_syscall(_LIBC_SYS_SYSTEMRESTART);
}

//==============================================================================
/**
 * @brief Function sleep the system.
 *
 * The function sleep system. Mode number depends on used CPU.
 *
 * @return On success \b 0 is returned, otherwise \b -1.
 *
 * @b Example
 * @code
        #include <dnx/os.h>
        #include <unistd.h>

        // ...

        const int standby = 2;

        system_low_power_mode(standby);

        // ...

   @endcode
 */
//==============================================================================
static inline int system_low_power_mode(int mode)
{
        int err = _libc_syscall(_LIBC_SYS_LOWPOWERMODE, &mode);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function restarts the system and execute specified address.
 *
 * The function restart system. Function does not write unsaved data.
 * The function can be used to handle bootloaders.
 *
 * @return This function does not return.
 *
 * @b Example
 * @code
        #include <dnx/os.h>
        #include <unistd.h>

        // ...

        sync();

        system_reboot_to_address(0x8010000);

   @endcode
 */
//==============================================================================
static inline void system_reboot_to_address(u32_t addr)
{
        _libc_syscall(_LIBC_SYS_REBOOTTOADDR, &addr);
}

//==============================================================================
/**
 * @brief Function shutdown the system.
 *
 * The function shutdown system. Function does not write unsaved data.
 *
 * @return This function does not return.
 *
 * @b Example
 * @code
        #include <dnx/os.h>
        #include <unistd.h>

        // ...

        sync();

        // e.g. user requests system restart
        system_shutdown();

   @endcode
 */
//==============================================================================
static inline void system_shutdown(void)
{
        _libc_syscall(_LIBC_SYS_SYSTEMSHUTDOWN);
}

//==============================================================================
/**
 * @brief Function raed system log.
 *
 * The function syslog_read() read system log messages. Read message is
 * deleted from system ring buffer.
 *
 * @param  str          message string
 * @param  len          maximum string size
 * @param  from_time    time from log is get (time from system start)
 * @param  curr_time    log current time (time from system start)
 *
 * @return String size. 0 if system log is empty.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        char msg[128];
        struct timeval t = {0, 0};
        while (syslog_read(msg, sizeof(msg), &t, &t) {
                puts(msg);
        }

        // ...

   @endcode
 */
//==============================================================================
static inline size_t syslog_read(char *str, size_t len, const struct timeval *from_time,
        struct timeval *curr_time)
{
        size_t n   = 0;
        int    err = _libc_syscall(_LIBC_SYS_SYSLOGREAD, str, &len, from_time, curr_time, &n);
        return err ? 0 : n;
}

//==============================================================================
/**
 * @brief Function clear system log.
 *
 * The function syslog_clear() clears system log messages.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        syslog_clear();

        // ...

   @endcode
 */
//==============================================================================
static inline void syslog_clear(void)
{
        _libc_syscall(_LIBC_SYS_SYSLOGCLEAR);
}

//==============================================================================
/**
 * @brief Function is used to get occurred kernel panic info.
 *
 * The function get_kernel_panic_info() check if in the last session
 * the Kernel Panic occurred.
 *
 * @param  info         kernel panic information
 *
 * @return If kernel panic occurred then \b true is returned, otherwise \b false.
 *
 * @b Example
 * @code
        #include <dnx/os.h>

        // ...

        kernel_panic_info_t info;
        bool occurred = get_kernel_panic_info(&info);
        if (occurred) {
                // ...
        }

        // ...

   @endcode
 */
//==============================================================================
static inline bool get_kernel_panic_info(kernel_panic_info_t *info)
{
        int err = _libc_syscall(_LIBC_SYS_KERNELPANICINFO, info);
        return err ? false : true;
}

//==============================================================================
/**
 * @brief  Function check if selected object (address) is allocated in heap.
 *
 * @param  ptr          object's pointer
 *
 * @return If object is in heap then true is returned, otherwise false.
 */
//==============================================================================
static inline bool is_object_in_heap(const void *ptr)
{
        int err = _libc_syscall(_LIBC_SYS_ISHEAPADDR, ptr);
        return err ? false : true;
}

//==============================================================================
/**
 * @brief  Flush CPU cache.
 */
//==============================================================================
static inline void cache_flush(void)
{
        _libc_syscall(_LIBC_SYS_CACHEFLUSH);
}

//==============================================================================
/**
 * @brief  Register new appset(s).
 *
 * @param  appset_address_array appset address array
 * @param  appset_count         number of addresses
 *
 * @return One of errno value.
 */
//==============================================================================
extern int register_appset(const void **appset_address_array, size_t appset_count);

//==============================================================================
/**
 * @brief  Set heap allocation size. It is minimal memory block size that will
 *         be requested from system to be used as heap memory.
 *
 * @param  alloc_size           allocation size
 *
 * @return One of errno value.
 */
//==============================================================================
extern int set_heap_allocation_size(size_t alloc_size);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* _LIBC_DNX_OS_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
