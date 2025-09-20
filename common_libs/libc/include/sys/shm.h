/*==============================================================================
File    shm.h

Author  Daniel Zorychta

Brief   Shared memory functions.

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
@defgroup sys-shm-h <sys/shm.h>

The library is used to control shared memory regions used in IPC.
*/
/**@{*/

#ifndef _SHM_H_
#define _SHM_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <libc/source/syscall.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Function create shared memory region.
 *
 * The shmget() function creates shared memory region of name <i>key</i> and
 * size <i>size</i>. If region already exists then function return error.
 *
 * @param key           region name
 * @param size          region size
 *
 * @exception | EINVAL
 * @exception | ENOMEM
 * @exception | EEXIST
 *
 * @return On success, <b>0</b> is returned. On error, <b>-1</b>
 * is returned, and <b>errno</b> is set appropriately.
 *
 * @b Example
 * @code
        #include <sys/shm.h>

        // ...

        #define SHMNAME "my_region"

        char   *shm = NULL;
        size_t  sz  = 0;

        if (shmget(SHMNAME, 100) == 0) {
                if (shmat(SHMNAME, &shm, &sz) == 0) {
                        puts("Shared memory in use");
                } else {
                        perror(SHMNAME);
                }
        } else {
                perror(SHMNAME);
        }

        // ...

   @endcode
 *
 * @see shmat(), shmdt(), shmrm()
 */
//==============================================================================
static inline int shmget(const char *key, size_t size)
{
        int err = _libc_syscall(_LIBC_SYS_SHMCREATE, key, &size);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function destroy shared memory region.
 *
 * The shmrm() function destroy shared memory region of name <i>key</i>.
 * The region should be released by processes to be possible remove block.
 *
 * @param key           region name
 *
 * @exception | EINVAL
 * @exception | ENOMEM
 * @exception | ENOENT
 * @exception | EADDRINUSE
 *
 * @return On success, <b>0</b> is returned. On error, <b>-1</b>
 * is returned, and <b>errno</b> is set appropriately.
 *
 * @b Example
 * @code
        #include <sys/shm.h>

        // ...

        #define SHMNAME "my_region"

        char   *shm = NULL;
        size_t  sz  = 0;

        if (shmget(SHMNAME, 100) == 0) {
                if (shmat(SHMNAME, &shm, &sz) == 0) {
                        puts("Shared memory in use");
                } else {
                        perror(SHMNAME);
                }
        } else {
                perror(SHMNAME);
        }

        // ...

        if (shm) {
                shmdt(SHMNAME);

                if (shmrm(SHMNAME)) {
                        perror(SHNAME);
                }
        }

        // ...

   @endcode
 *
 * @see shmat(), shmdt(), shmget()
 */
//==============================================================================
static inline int shmrm(const char *key)
{
        int err = _libc_syscall(_LIBC_SYS_SHMDESTROY, key);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function attach shared memory region.
 *
 * The shmat() function attach shared memory region of name <i>key</i>.
 *
 * @param key           region name
 * @param mem           region address
 * @param size          region size
 *
 * @exception | EINVAL
 * @exception | ENOENT
 *
 * @return On success, <b>0</b> is returned. On error, <b>-1</b>
 * is returned, and <b>errno</b> is set appropriately.
 *
 * @b Example
 * @code
        #include <sys/shm.h>
        #include <dnx/misc.h>

        // ...

        #define SHMNAME "my_region"

        char   *shm = NULL;
        size_t  sz  = 0;

        if (shmget(SHMNAME, 100) == 0) {
                if (shmat(SHMNAME, &shm, &sz) == 0) {
                        puts("Shared memory in use");

                        strlcpy(shm, "My region", sz);

                        // ...

                } else {
                        perror(SHMNAME);
                }
        } else {
                perror(SHMNAME);
        }

        // ...

   @endcode
 *
 * @see shmrm(), shmdt(), shmget()
 */
//==============================================================================
static inline int shmat(const char *key, void **mem, size_t *size)
{
        int err = _libc_syscall(_LIBC_SYS_SHMATTACH, key, mem, &size);
        return err ? -1 : 0;
}

//==============================================================================
/**
 * @brief Function detach shared memory region.
 *
 * The shmdt() function detach shared memory region of name <i>key</i>.
 *
 * @param key           region name
 *
 * @exception | EINVAL
 * @exception | ENOENT
 *
 * @return On success, <b>0</b> is returned. On error, <b>-1</b>
 * is returned, and <b>errno</b> is set appropriately.
 *
 * @b Example
 * @code
        #include <sys/shm.h>
        #include <dnx/misc.h>

        // ...

        #define SHMNAME "my_region"

        char   *shm = NULL;
        size_t  sz  = 0;

        if (shmget(SHMNAME, 100) == 0) {
                if (shmat(SHMNAME, &shm, &sz) == 0) {
                        puts("Shared memory in use");

                        strlcpy(shm, "My region", sz);

                        // ...

                } else {
                        perror(SHMNAME);
                }
        } else {
                perror(SHMNAME);
        }

        // ...

        if (shm) {
                shmdt(SHMNAME);

                if (shmrm(SHMNAME)) {
                        perror(SHNAME);
                }
        }

        // ...

        // ...

   @endcode
 *
 * @see shmrm(), shmdt(), shmget()
 */
//==============================================================================
static inline int shmdt(const char *key)
{
        int err = _libc_syscall(_LIBC_SYS_SHMDETACH, key);
        return err ? -1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _SHM_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
