/*==============================================================================
@file   ioctl.h

@author Daniel Zorychta

@brief  Header contain all device control commands. Depend on existing drivers.

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
\defgroup sys-ioctl-h <sys/ioctl.h>

The library is used to control IO and files in non-standard way.

*/
/**@{*/

#ifndef _LIBC_IOCTL_H_
#define _LIBC_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <config.h>
#include <stdio.h>
#include <stdarg.h>
#include __LIBC_IOCTL_REQUEST_FILE__
#include <libc/source/syscall.h>
#include <stdbool.h>

/*==============================================================================
  Exported macros
==============================================================================*/
/**
 * @brief Request closes opened \b pipe device.
 *
 * Request closes opened \b pipe device.
 *
 * @see   ioctl()
 */
#define IOCTL_PIPE__CLOSE 0xF0000

/**
 * @brief Request clears \b pipe device.
 *
 * Request clears \b pipe device.
 *
 * @see   ioctl()
 */
#define IOCTL_PIPE__CLEAR 0xF0001

/**
 * @brief Request set stream to non-blocking read mode.
 *
 * Request set stream to non-blocking read mode.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__NON_BLOCKING_RD_MODE 0xF0003

/**
 * @brief Request set stream to default read mode.
 *
 * Request set stream to default read mode.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__DEFAULT_RD_MODE 0xF0004

/**
 * @brief Request return read mode status.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__IS_NON_BLOCKING_RD_MODE 0xF0005

/**
 * @brief Request set stream to non-blocking write mode.
 *
 * Request set stream to non-blocking write mode.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__NON_BLOCKING_WR_MODE 0xF0006

/**
 * @brief Request set stream to default write mode.
 *
 * Request set stream to default write mode.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__DEFAULT_WR_MODE 0xF0007

/**
 * @brief Request return write mode status.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__IS_NON_BLOCKING_WR_MODE 0xF0008

/**
 * @brief Request set stream to non-strict read mode.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__NON_STRICT_RD_MODE 0xF0009

/**
 * @brief Request return read strict mode status.
 *
 * @see   ioctl()
 */
#define IOCTL_VFS__IS_NON_STRICT_RD_MODE 0xF000A

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported inline functions
==============================================================================*/
//==============================================================================
/**
 * @brief Function sends request to selected file to do non-standard operation.
 *
 * The ioctl() function manipulates the file parameters. In particular, many
 * operating characteristics of character special files (e.g., drivers) may
 * be controlled with ioctl() requests.
 *
 * The second argument is a device-dependent request code. The third
 * argument is an untyped pointer to memory.
 *
 * @param fd            file descriptor
 * @param request       request number (each driver has own requests)
 * @param ...           untyped pointer to memory (optional in some requests)
 *
 * @exception | ...
 *
 * @return On success zero is returned. On error, -1 is returned, and
 * <b>errno</b> is set appropriately.
 *
 * @b Example @b 1
 * @code
        // ...

        FILE *file = fopen("/dev/tty0", "r");
        if (file) {
                ioctl(fileno(file), IOCTL_TTY__CLEAR_SCR);

                // ...
        } else {
                perror(NULL);
        }

        fclose(file);

        // ...

   @endcode

 * @b Example @b 2
 * @code
        // ...

        FILE *file = fopen("/dev/tty0", "r");
        if (file) {
                int row = -1;
                ioctl(fileno(file), IOCTL_TTY__GET_ROW, &row);

                // ...
        } else {
                perror(NULL);
        }

        fclose(file);

        // ...

   @endcode
 *
 * @note
 * The names of all requests are constructed in the same way: @b IOCTL_<MODULE_NAME>__<REQUEST_NAME>.
 */
//==============================================================================
static inline int ioctl(int fd, int request, ...)
{
        va_list arg;
        va_start(arg, request);
        int err = _libc_syscall(_LIBC_SYS_IOCTL, &fd, &request, &arg);
        va_end(arg);
        return err ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBC_IOCTL_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
