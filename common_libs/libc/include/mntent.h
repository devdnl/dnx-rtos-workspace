/*==============================================================================
@file   mntent.h

@author Daniel Zorychta

@brief  Mount entry information.

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
\defgroup mntent-h <mntent.h>

The library provides information about file system mount entry.

*/
/**@{*/

#ifndef _MNTENT_H_
#define _MNTENT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <sys/types.h>
#include <libc/source/syscall.h>
#include <errno.h>
#include <dnx/misc.h>

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
#ifdef DOXYGEN
/** @brief Structure that describes a mount table entry. */
struct mntent {
        const char *mnt_fsname; /*!< Device or server for file system.*/
        const char *mnt_dir;    /*!< Directory mounted on.*/
        const char *mnt_src;    //!< Source path
        u64_t       mnt_total;  /*!< Device total size in bytes.*/
        u64_t       mnt_free;   /*!< Device free space in bytes.*/
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
 * @brief Function returns next mount entries.
 *
 * Function returns next file system entry according to file system order
 * <i>seek</i> (determined on mount time) and write information to mount entry
 * container pointed by <i>mntent</i>.
 *
 * @param seek          n-item to read
 * @param mntent        pointer to container
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOENT
 *
 * @return Returns \b 0 on success.
 * On error \b -1 is returned and <b>errno</b> is set appropriately.
 *
 * @b Example
 * @code
        #include <mntent.h>

        // ...

        int i = 0;

        struct mntent entry;
        while (getmntentry(i++, &entry) == 0) {
               // ...
        }

        // ...
   @endcode
 *
 * @see mount(), umount()
 */
//==============================================================================
static inline int getmntentry(size_t seek, struct mntent *mntent)
{
        int err = _libc_syscall(_LIBC_SYS_GETMNTENTRY, &seek, mntent);
        return err ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _MNTENT_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
