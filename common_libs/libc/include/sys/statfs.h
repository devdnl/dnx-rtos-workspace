/*==============================================================================
@file   statfs.h

@author Daniel Zorychta

@brief  File systems information.

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
\defgroup sys-statfs-h <sys/statfs.h>

The library is used to get file systems information.

*/
/**@{*/

#ifndef _STATFS_H_
#define _STATFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <sys/types.h>
#include <libc/source/syscall.h>

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/

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
 * @brief Function gets file system information.
 *
 * The function statfs() returns information about a mounted file system.
 * A <i>path</i> is directory of the mount point of file system.
 *
 * @param path          node name
 * @param statfs        file system information container
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | @ref ENOTDIR
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and \b errno
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        struct statfs info;
        if (statfs("/proc", &info) == 0) {
                // ...
        } else {
                perror("/proc");
                // ...
        }

        // ...
   @endcode
 *
 * @see mount(), umount()
 */
//==============================================================================
static inline int statfs(const char *path, struct statfs *statfs)
{
        int err = _libc_syscall(_LIBC_SYS_STATFS, path, statfs);
        return err ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _STATFS_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
