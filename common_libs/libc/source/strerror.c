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
 * @brief Function returns error string
 *
 * @param errnum        error number
 *
 * @return error number string
 */
//==============================================================================
const char *strerror(int errnum)
{
        static const char *errstr[_ENUMBER] = {
                [ESUCC] = "Success",
                [EPERM] = "Operation not permitted",
                [ENOENT] = "No such file or directory",
                [ESRCH] = "No such process",
                [EIO] = "I/O error",
                [ENXIO] = "No such device or address",
                [E2BIG] = "Argument list too long",
                [ENOEXEC] = "Exec format error",
                [EAGAIN] = "Try again",
                [ENOMEM] = "Out of memory",
                [EACCES] = "Permission denied",
                [EFAULT] = "Bad address",
                [EBUSY] = "Device or resource busy",
                [EEXIST] = "File exists",
                [ENODEV] = "No such device",
                [ENOTDIR] = "Not a directory",
                [EISDIR] = "Is a directory",
                [EINVAL] = "Invalid argument",
                [EMFILE] = "Too many open files",
                [EFBIG] = "File too large",
                [ENOSPC] = "No space left on device",
                [ESPIPE] = "Illegal seek",
                [EROFS] = "Read-only file system",
                [EDOM] = "Math argument out of domain of function",
                [ERANGE] = "Math result not representable",
                [EILSEQ] = "Illegal byte sequence",
                [ENAMETOOLONG] = "File name too long",
                [ENOTEMPTY] = "Directory not empty",
                [EBADRQC] = "Invalid request code",
                [ETIME] = "Timer expired",
                [ENONET] = "Machine is not on the network",
                [EUSERS] = "Too many users",
                [EADDRINUSE] = "Address already in use",
                [ENOMEDIUM] = "No medium found",
                [EMEDIUMTYPE] = "Wrong medium type",
                [ECANCELED] = "Operation Canceled",
                [ENOTSUP] = "Not supported",
                [ENOSYS] = "Function not implemented",
                [ECONNABORTED] = "Connection aborted",
                [ECONNREFUSED] = "Connection refused",
                [ECONNRESET] = "Connection reset",
                [EISCONN] = "Socket is connected",
                [EALREADY] = "Connection already in progress",
                [EBADF] = "Bad file descriptor",
        };

        if ((errnum >= 0) && (errnum < _ENUMBER)) {
                return errstr[errnum];
        } else {
                return "Unknown error";
        }
}

/*==============================================================================
  End of file
==============================================================================*/
