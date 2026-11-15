/*==============================================================================
@file   file.c

@author Daniel Zorychta

@brief  File handling functions.

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

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include "common.h"

/*==============================================================================
  Local macros
==============================================================================*/
#ifndef O_RDONLY
#define O_RDONLY 00
#endif

#ifndef O_WRONLY
#define O_WRONLY 01
#endif

#ifndef O_RDWR
#define O_RDWR 02
#endif

#ifndef O_CREAT
#define O_CREAT 0100
#endif

#ifndef O_EXCL
#define O_EXCL 0200
#endif

#ifndef O_TRUNC
#define O_TRUNC 01000
#endif

#ifndef O_APPEND
#define O_APPEND 02000
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
int unlink(const char *pathname);

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/
extern void *_libc_malloc(size_t size);
extern void  _libc_free(void *ptr);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Function open file as file descriptor.
 *
 * @param  path         file path
 * @param  flags        file open flags
 * @param  mode         optional argument in case of O_CREAT flag
 *
 * @return On success file descriptor is returned (fd >= 0), otherwise -1.
 */
//==============================================================================
int open(const char *path, int flags, ...)
{
        mode_t mode = 0600;

        if (flags & O_CREAT) {
                va_list arg;
                va_start(arg, flags);
                mode = va_arg(arg, mode_t);
                va_end(arg);
        }

        int fd;
        int err = _libc_syscall(_LIBC_SYS_OPEN, &fd, path, &flags, &mode);
        return err ? -1 : fd;
}

//==============================================================================
/**
 * @brief  Function close file descriptor.
 *
 * @param  fd           file descriptor
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int close(int fd)
{
        int err = _libc_syscall(_LIBC_SYS_CLOSE, &fd);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief  Function read file.
 *
 * @param  fd           file descriptor
 * @param  buf          buffer
 * @param  count        bytes to read
 *
 * @return On success number of written bytes is returned, otherwise -1.
 */
//==============================================================================
ssize_t write(int fd, const void *buf, size_t count)
{
        size_t n   = 0;
        int    err = _libc_syscall(_LIBC_SYS_WRITE, &fd, buf, &count, &n);
        return err ? -1 : (ssize_t)n;
}

//==============================================================================
/**
 * @brief  Function read file.
 *
 * @param  fd           file descriptor
 * @param  buf          buffer
 * @param  count        bytes to read
 *
 * @return On success number of read bytes is returned, otherwise -1 value.
 */
//==============================================================================
ssize_t read(int fd, void *buf, size_t count)
{
        size_t n   = 0;
        int    err = _libc_syscall(_LIBC_SYS_READ, &fd, buf, &count, &n);
        return err ? -1 : (ssize_t)n;
}

//==============================================================================
/**
 * @brief  Function set file position.
 *
 * @param  fd           file descriptor
 * @param  offset       offset
 * @param  whence       seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * @return On success return file offset after operation, otherwise negative value
 *         on error.
 */
//==============================================================================
int64_t lseek64(int fd, int64_t offset, int whence)
{
        i64_t seek = offset;
        int   err  = _libc_syscall(_LIBC_SYS_SEEK64, &fd, &seek, &whence);
        return err ? -1 : seek;
}

//==============================================================================
/**
 * @brief  Function set file position.
 *
 * @param  fd           file descriptor
 * @param  offset       offset
 * @param  whence       seek mode (SEEK_SET, SEEK_CUR, SEEK_END)
 *
 * @return On success return file offset after operation, otherwise negative value
 *         on error.
 */
//==============================================================================
off_t lseek(int fd, off_t offset, int whence)
{
        return lseek64(fd, offset, whence);
}

//==============================================================================
/**
 * @brief  Function remove selected file.
 *
 * @param  pathname     file to remove
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int unlink(const char *pathname)
{
        int err = _libc_syscall(_LIBC_SYS_REMOVE, pathname);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function remove selected file.
 *
 * remove() deletes a name from the file system. If the removed name was
 * the last link to a file and no processes have the file open, the file is
 * deleted and the space it was using is made available for reuse.<p>
 *
 * If the name referred to a FIFO, or device, the name is removed, but
 * processes which have the object open may continue to use it.
 *
 * @param path      path to file
 *
 * @exception | @ref ENOENT
 * @exception | @ref EINVAL
 *
 * @return On success, zero is returned. On error, -1 is returned, and
 * @ref errno is set appropriately.
 */
//==============================================================================
int _libc_remove(const char *path)
{
        int err = _libc_syscall(_LIBC_SYS_REMOVE, path);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function renames selected file.
 *
 * rename() renames a file. In contrast to standard C library this function
 * don't move files between directories if <i>new_name</i> is localized on other
 * filesystem than <i>old_name</i>, otherwise it's depending on filesystem.
 *
 * @param old_name      old file name
 * @param new_name      new file name
 *
 * @exception | @ref ENOENT
 * @exception | @ref EPERM
 * @exception | @ref EINVAL
 *
 * @return On success, zero is returned. On error, -1 is returned, and @ref errno
 * is set appropriately.
 */
//==============================================================================
int _libc_rename(const char *old_name, const char *new_name)
{
        int err = _libc_syscall(_LIBC_SYS_RENAME, old_name, new_name);
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
 */
//==============================================================================
int _libc_fstat(int fd, struct stat *buf)
{
        if (fd >= 0) {
                int err = _libc_syscall(_LIBC_SYS_FSTAT, &fd, buf);
                return err ? -1 : 0;
        } else {
                errno = EINVAL;
                return -1;
        }
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
 */
//==============================================================================
int _libc_mkdir(const char *pathname, mode_t mode)
{
        int fd = open(pathname, O_CREAT | O_DIRECTORY, mode);
        if (fd >= 0) {
                return close(fd);
        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Function opens selected directory.
 *
 * Function opens a directory stream corresponding to the directory <i>name</i>, and
 * returns a pointer to the directory stream. The stream is positioned at the first
 * entry in the directory.
 *
 * @param name          directory path
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref ENOENT
 * @exception | ...
 *
 * @return A pointer to the directory stream. On error, <b>NULL</b> is returned,
 * and <b>errno</b> is set appropriately.
 */
//==============================================================================
DIR *_libc_opendir(const char *name)
{
        DIR *dir = _libc_malloc(sizeof(*dir));
        if (dir) {

                dir->fd = open(name, O_DIRECTORY);
                if (dir->fd >= 0) {
                        return dir;
                }

                _libc_free(dir);
        }

        return NULL;
}

//==============================================================================
/**
 * @brief Function closes selected directory stream.
 *
 * Function closes the directory stream associated with <i>dir</i>. The directory
 * stream descriptor <i>dir</i> is not available after this call.
 *
 * @param dir           pinter to directory object
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref ENOENT
 * @exception | ...
 *
 * @return Return \b 0 on success. On error, \b -1 is returned,
 * and <b>errno</b> is set appropriately.
 */
//==============================================================================
int _libc_closedir(DIR *dir)
{
        int err = -1;

        if (dir) {
                err = close(dir->fd);
                if (!err) {
                        _libc_free(dir);
                }
        } else {
                errno = EINVAL;
        }

        return err;
}

//==============================================================================
/**
 * @brief Function reads entry from opened directory stream.
 *
 * Function returns a pointer to object <b>dirent_t</b> type representing the
 * next directory entry in the directory stream pointed to by <i>dir</i>.<p>
 *
 * @param dir           directory object
 *
 * @exception | @ref EINVAL
 * @exception | @ref ENOMEM
 * @exception | @ref EACCES
 * @exception | @ref ENOENT
 * @exception | ...
 *
 * @return On success, function returns a pointer to a <b>dirent_t</b> type. If
 * the end of the directory stream is reached, field <b>name</b> of <b>dirent_t</b>
 * type is <b>NULL</b>. If an error occurs, NULL-object and <b>errno</b> is set
 * appropriately.
 */
//==============================================================================
struct dirent *_libc_readdir(DIR *dir)
{
        struct dirent *dirent = NULL;

        if (dir) {
                int err = _libc_syscall(_LIBC_SYS_DIRREAD, &dir->fd, &dirent);
                if (err) {
                        dirent = NULL;
                }
        }

        return dirent;
}

//==============================================================================
/**
 * Set the position of a directory stream.
 *
 * @param dir   directory object
 * @param seek  pointer position
 *
 * @exception | @ref EINVAL
 *
 * @return The function shall not return a value.
 */
//==============================================================================
void _libc_seekdir(DIR *dir, u32_t seek)
{
        if (dir) {
                _libc_syscall(_LIBC_SYS_DIRSEEK, &dir->fd, &seek);
        }
}

//==============================================================================
/**
 * Return current location of a named directory stream.
 *
 * @param dir   directory object
 *
 * @exception | @ref EINVAL
 *
 * @return Upon successful completion, the function shall return the current
 *         location of the specified directory stream. On error, -1 is rturned,
 *         and errno is set appropriately.
 */
//==============================================================================
i32_t _libc_telldir(DIR *dir)
{
        i32_t pos = -1;

        if (dir) {
                u32_t seek = 0;
                int   err  = _libc_syscall(_LIBC_SYS_DIRTELL, &dir->fd, &seek);
                if (!err) {
                        pos = (i32_t)seek;
                }
        }

        return pos;
}

/*==============================================================================
  End of file
==============================================================================*/
