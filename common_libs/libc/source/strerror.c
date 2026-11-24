/*==============================================================================
@file   strerror.c

@author Daniel Zorychta

@brief  strerror() function implementation. User space code.

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
#include <string.h>
#include <errno.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define TO_STR(str)        #str
#define NUMBER_TO_STR(val) TO_STR(val)

/* default: full strings (flag not defined in configuration) */
#ifndef __LIBC_ERRNO_STRING_LEN__
#define __LIBC_ERRNO_STRING_LEN__ 3
#endif

/*
 * Error strings in order of errno values (checked at compile time). Length of
 * strings is selected by __LIBC_ERRNO_STRING_LEN__: 0: empty string, 1: number
 * (e.g. "2"), 2: name (e.g. "ENOENT"), 3: description.
 */
#define ERRNO_LIST(X) \
        X(ESUCC, "Success") \
        X(EPERM, "Operation not permitted") \
        X(ENOENT, "No such file or directory") \
        X(ESRCH, "No such process") \
        X(EIO, "I/O error") \
        X(ENXIO, "No such device or address") \
        X(E2BIG, "Argument list too long") \
        X(ENOEXEC, "Exec format error") \
        X(EAGAIN, "Try again") \
        X(ENOMEM, "Out of memory") \
        X(EACCES, "Permission denied") \
        X(EFAULT, "Bad address") \
        X(EBUSY, "Device or resource busy") \
        X(EEXIST, "File exists") \
        X(ENODEV, "No such device") \
        X(ENOTDIR, "Not a directory") \
        X(EISDIR, "Is a directory") \
        X(EINVAL, "Invalid argument") \
        X(EMFILE, "Too many open files") \
        X(EFBIG, "File too large") \
        X(ENOSPC, "No space left on device") \
        X(ESPIPE, "Illegal seek") \
        X(EROFS, "Read-only file system") \
        X(EDOM, "Math argument out of domain of function") \
        X(ERANGE, "Math result not representable") \
        X(EILSEQ, "Illegal byte sequence") \
        X(ENAMETOOLONG, "File name too long") \
        X(ENOTEMPTY, "Directory not empty") \
        X(EBADRQC, "Invalid request code") \
        X(ETIME, "Timer expired") \
        X(ENONET, "Machine is not on the network") \
        X(EUSERS, "Too many users") \
        X(EADDRINUSE, "Address already in use") \
        X(ENOMEDIUM, "No medium found") \
        X(EMEDIUMTYPE, "Wrong medium type") \
        X(ECANCELED, "Operation Canceled") \
        X(ENOTSUP, "Not supported") \
        X(ENOSYS, "Function not implemented") \
        X(ECONNABORTED, "Connection aborted") \
        X(ECONNREFUSED, "Connection refused") \
        X(ECONNRESET, "Connection reset") \
        X(EISCONN, "Socket is connected") \
        X(EALREADY, "Connection already in progress") \
        X(EBADF, "Bad file descriptor") \
        X(EPIPE, "Broken pipe") \
        X(ENOTCONN, "Socket is not connected")

#define ERRIDX_ENUM(name, text)  ERRIDX_##name,
#define ERRIDX_CHECK(name, text) typedef char errno_order_##name[(ERRIDX_##name == name) ? 1 : -1];

#if __LIBC_ERRNO_STRING_LEN__ == 1
#define ERRSTR(name, text) NUMBER_TO_STR(name) "\0"
#elif __LIBC_ERRNO_STRING_LEN__ == 2
#define ERRSTR(name, text) #name "\0"
#else
#define ERRSTR(name, text) text "\0"
#endif

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
#if __LIBC_ERRNO_STRING_LEN__ > 0
/* order of list is the same as errno values (index of list) */
enum { ERRNO_LIST(ERRIDX_ENUM) ERRNO_LIST_LEN };
ERRNO_LIST(ERRIDX_CHECK)
typedef char errno_count[(ERRNO_LIST_LEN == _ENUMBER) ? 1 : -1];

/* strings are stored one after another (no table of pointers) */
static const char errstr[] = ERRNO_LIST(ERRSTR);
#endif

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
 * @brief Function returns error string (length selected by
 *        __LIBC_ERRNO_STRING_LEN__: empty, number, name or description)
 *
 * @param errnum        error number
 *
 * @return error number string
 */
//==============================================================================
const char *strerror(int errnum)
{
#if __LIBC_ERRNO_STRING_LEN__ > 0
        if ((errnum >= 0) && (errnum < ERRNO_LIST_LEN)) {
                const char *str = errstr;

                while (errnum--) {
                        str += strlen(str) + 1;
                }

                return str;
        }

#if __LIBC_ERRNO_STRING_LEN__ == 3
        return "Unknown error";
#else
        return "?";
#endif
#else
        (void)errnum;
        return "";
#endif
}

/*==============================================================================
  End of file
==============================================================================*/
