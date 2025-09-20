/*==============================================================================
@file   time.h

@author Daniel Zorychta

@brief  Get/Set system time.

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
\defgroup sys-time-h <sys/time.h>

The library provides system set/get time functions.

@{
*/

#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <libc/include/sys/types.h>
#include <stddef.h>
#include <libc/source/syscall.h>
#include <errno.h>
#include <dnx/misc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
/**
 * @brief Type representing subseconds.
 *
 * Alias of a fundamental arithmetic type capable of representing time in microseconds.
 *
 * @see gettimeofday(), settimeofday()
 */
#ifndef __SUSECONDS_TYPE_DEFINED__
typedef u32_t suseconds_t;
#endif

/**
 * @brief Type representing time value.
 *
 * @see gettimeofday(), settimeofday()
 */
#ifndef __STRUCT_TIMEVAL_DEFINED__
struct timeval {
        time_t tv_sec;          /*!< seconds */
        suseconds_t tv_usec;    /*!< microseconds */
};
#endif

/**
 * @brief Type representing timezone.
 *
 * @see gettimeofday(), settimeofday()
 */
#ifndef __STRUCT_TIMEZONE_DEFINED__
struct timezone {
        int tz_minuteswest;     /*!< minutes west of Greenwitch */
        int tz_dsttime;         /*!< type of DST correction */
};
#endif

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

/*==============================================================================
  Exported inline functions
==============================================================================*/
//==============================================================================
/**
 * @brief  Get system time
 *
 * The function can get the time as well as a timezone. If either tv or tz is
 * NULL, the corresponding structure is not get.
 *
 * @param  tv   time value      (can be NULL)
 * @param  tz   time zone       (can be NULL)
 *
 * @return The function return 0 for success, or -1 for failure and errno is
 *         set appropriately.
 *
 * @b Example
 * @code
        #include <sys/time.h>

        //...

        struct timeval timeval;

        gettimeofday(&timeval, NULL);

        struct tm tm;
        if (UTC) {
                gmtime_r(&timeval.tv_sec, &tm);
        } else {
                localtime_r(&timeval.tv_sec, &tm);
        }

        printf(asctime(&tm));

        //...
   @endcode
 *
 * @see settimeofday(), time()
 */
//==============================================================================
static inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
        int err = _libc_syscall(_LIBC_SYS_GETTIMEOFDAY, tv ,tz);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief  Set system time
 *
 * The function can set the time as well as a timezone. If either tv or tz is
 * NULL, the corresponding structure is not set.
 *
 * @param  tv   time value      (can be NULL)
 * @param  tz   time zone       (can be NULL)
 *
 * @return The function return 0 for success, or -1 for failure and errno is
 *         set appropriately.
 *
 * @see gettimeofday(), time(), stime()
 */
//==============================================================================
static inline int settimeofday(const struct timeval *tv, const struct timezone *tz)
{
        int err = _libc_syscall(_LIBC_SYS_SETTIMEOFDAY, tv, tz);
        return err ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIME_H_ */
/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
