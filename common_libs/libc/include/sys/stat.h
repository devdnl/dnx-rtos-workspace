/*==============================================================================
@file   stat.h

@author Daniel Zorychta

@brief  File contains statistics functions and directory, FIFO, node create functions.

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
\defgroup sys-stat-h <sys/stat.h>

The library is used to create device nodes, directories, and FIFO (pipe) devices.
Library provides also function used to manipulate file modes. The set of library
contains functions that read file statistics.

*/
/**@{*/

#ifndef _LIBC_STAT_H_
#define _LIBC_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <sys/types.h>
#include <libc/source/syscall.h>
#include <dnx/misc.h>
#include <errno.h>

/*==============================================================================
  Exported macros
==============================================================================*/
#ifdef DOXYGEN
/** @brief Read permission, owner. @see mode_t */
#define S_IRUSR
/** @brief Write permission, owner. @see mode_t */
#define S_IWUSR
/** @brief Execute permission, owner. @see mode_t */
#define S_IXUSR
/** @brief Read permission, group. @see mode_t */
#define S_IRGRO
/** @brief Write permission, group. @see mode_t */
#define S_IWGRO
/** @brief Execute permission, group. @see mode_t */
#define S_IXGRO
/** @brief Read permission, others. @see mode_t */
#define S_IROTH
/** @brief Write permission, others. @see mode_t */
#define S_IWOTH
/** @brief Execute permission, others. @see mode_t */
#define S_IXOTH
/** @brief Mask permission flags. @see mode_t */
#define S_IPMT(mode_t_m)
/** @brief Mask file type flags. @see mode_t */
#define S_IFMT(mode_t_m)
/** @brief Check if file is a regular file. */
#define S_ISREG(mode_t_m)
/** @brief Check if file is a dir. */
#define S_ISDIR(mode_t_m)
/** @brief Check if file is a character device. */
#define S_ISCHR(mode_t_m)
/** @brief Check if file is a block device. */
#define S_ISBLK(mode_t_m)
/** @brief Check if file is a link. */
#define S_ISLNK(mode_t_m)
/** @brief Check if file is a built-in program. */
#define S_ISPROG(mode_t_m)
/** @brief Check if file is a FIFO. */
#define S_ISFIFO(mode_t_m)
/** @brief Check if file is a socket (UNIX domain socket bound to path). */
#define S_ISSOCK(mode_t_m)
#endif

/*==============================================================================
  Exported object types
==============================================================================*/
#ifdef DOXYGEN
/** @brief File information. */
struct stat {
        u64_t   st_size;        /*!< Total size, in bytes.*/
        dev_t   st_dev;         /*!< ID of device containing file.*/
        mode_t  st_mode;        /*!< Protection, file type.*/
        uid_t   st_uid;         /*!< User ID of owner.*/
        gid_t   st_gid;         /*!< Group ID of owner.*/
        time_t  st_ctime;       /*!< Time of creation.*/
        time_t  st_mtime;       /*!< Time of last modification.*/
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
 * @brief Function creates device node (device file).
 *
 * The mknod() function creates a file system node (device special file) named
 * <i>pathname</i>. Node is connected to the driver from module <i>mod_name</i>
 * of <i>major</i> and <i>minor</i> address.
 *
 * @param pathname      node name
 * @param mod_name      module name
 * @param major         driver major number
 * @param minor         driver minor number
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        mknod("/dev/uart0", "UART", 0, 0);

        // ...
   @endcode
 */
//==============================================================================
static inline int mknod(const char *pathname, const char *mod_name, int major, int minor)
{
        int err = _libc_syscall(_LIBC_SYS_MKNOD, pathname, mod_name, &major, &minor);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function creates new directory.
 *
 * The mkdir() attempts to create a directory named <i>pathname</i>. The
 * argument <i>mode</i> specifies the permissions to use.
 *
 * @param pathname      directory name
 * @param mode          directory permissions
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        mkdir("/dev", 0666);    // wr+rd access for all users, groups and others

        // ...

   @endcode
 */
//==============================================================================
static inline int mkdir(const char *pathname, mode_t mode)
{
        extern int _libc_mkdir(const char *pathname, mode_t mode);
        return _libc_mkdir(pathname, mode);
}

//==============================================================================
/**
 * @brief Function creates FIFO file.
 *
 * The mkfifo() makes a FIFO special file with name <i>pathname</i>. <i>mode</i>
 * specifies the FIFO's permissions. A FIFO special file is similar to pipe, but
 * is created in filesystem and is not an anonymous. Access to FIFO is the same
 * as to regular file, except that data can be read only one time. Not all
 * filesystems support this file type.
 *
 * @param pathname      FIFO name
 * @param mode          FIFO permissions
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        mkfifo("/dev/my_fifo", 0666);    // wr+rd access for all users, groups and others

        // ...

   @endcode
 */
//==============================================================================
static inline int mkfifo(const char *pathname, mode_t mode)
{
        int err = _libc_syscall(_LIBC_SYS_MKFIFO, pathname, &mode);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function changes file mode.
 *
 * The chmod() system call change the permissions of a file.
 *
 * @param pathname      file to permission change
 * @param mode          new permissions
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        chmod("/foo/bar", 0666);   // wr+rd access for all users, groups and others

        // ...

   @endcode
 */
//==============================================================================
static inline int chmod(const char *pathname, mode_t mode)
{
        int err = _libc_syscall(_LIBC_SYS_CHMOD, pathname, &mode);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function sets file mode creation mask.
 *
 * The umask() function sets file mode creation mask of the process: bits of
 * the mask are removed from mode of new files, directories and FIFOs (e.g.
 * mask 022: files created with mode 0666 get 0644). New processes get mask of
 * the creating process (the first process: 022). Without users and file
 * permissions in the system the function has no effect and returns 0.
 *
 * @param mask          new mask
 *
 * @return Previous mask.
 *
 * @b Example
 * @code
        #include <sys/stat.h>

        // ...
        umask(077);   // new files only for owner
        // ...
   @endcode
 */
//==============================================================================
static inline mode_t umask(mode_t mask)
{
        mode_t old = 0;
#if __LIBC_PERMISSIONS_ENABLE__ == _YES_
        _libc_syscall(_LIBC_SYS_UMASK, &mask, &old);
#else
        (void)mask;
#endif
        return old;
}

//==============================================================================
/**
 * @brief Function gets file information.
 *
 * The stat() function return information about a file.
 *
 * @param pathname      file to inspect
 * @param buf           file's information
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        struct stat info;
        errno = 0;
        if (stat("/dev/foo", &info) == 0) {
                // ...
        } else {
                perror("stat()");
                // ...
        }

        // ...

   @endcode
 */
//==============================================================================
static inline int stat(const char *pathname, struct stat *buf)
{
        int err = _libc_syscall(_LIBC_SYS_STAT, pathname, buf);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function gets file information.
 *
 * The fstat() function return information about a descriptor <i>fd</i>.
 *
 * @param fd            file descriptor
 * @param buf           file's information
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref EEXIST
 * @exception | @ref ENOENT
 * @exception | @ref ENOSPC
 * @exception | ...
 *
 * @return On success, \b 0 is returned. On error, \b -1 is returned, and <b>errno</b>
 * is set appropriately.
 *
 * @b Example
 * @code
        // ...

        errno = 0;
        FILE *file = fopen("/foo/bar", "r");
        if (file) {
                struct stat info;
                if (fstat(fileno(file), &info) == 0) {
                        // ...
                } else {
                        perror("stat()");
                        // ...
                }

                fclose(file);
        } else {
                perror("fopen()");
        }

        // ...

   @endcode
 */
//==============================================================================
static inline int fstat(int fd, struct stat *buf)
{
        extern int _libc_fstat(int fd, struct stat *buf);
        return _libc_fstat(fd , buf);
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBC_STAT_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
