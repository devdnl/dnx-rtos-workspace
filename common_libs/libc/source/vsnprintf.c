/*==============================================================================
@file   vsnprintf.c

@author Daniel Zorychta

@brief  Print functions.

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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <libc/source/vformat.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        _libc_fmt_out_t out; //!< must be the first member
        char *buf;           //!< destination buffer
        size_t size;         //!< buffer size (0 = nothing is stored)
        size_t pos;          //!< number of stored characters
} str_out_t;

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
 * @brief  String sink write function. Characters that do not fit to the
 *         buffer are dropped (they are still counted by format core).
 *
 * @param  out          sink
 * @param  str          text
 * @param  len          text length
 *
 * @return Always true.
 */
//==============================================================================
static bool str_write(_libc_fmt_out_t *out, const char *str, size_t len)
{
        str_out_t *so = (str_out_t *)out;

        if (so->size > 0) {
                size_t room = so->size - 1 - so->pos;
                size_t n = (len < room) ? len : room;
                memcpy(so->buf + so->pos, str, n);
                so->pos += n;
        }

        return true;
}

//==============================================================================
/**
 * @brief Function convert arguments to stream (POSIX vsnprintf()).
 *
 * At most size-1 characters are written to the buffer and the result is
 * always null-terminated if size is greater than 0. If size is 0 nothing
 * is written and buf can be NULL.
 *
 * See vformat.c for supported conversion specifications.
 *
 * @param[in] *buf           buffer for stream
 * @param[in]  size          buffer size
 * @param[in] *format        message format
 * @param[in]  arg           argument list
 *
 * @return Number of characters that would be written if the buffer was large
 *         enough (not counting the terminating null character). On error
 *         negative value is returned and errno is set.
 */
//==============================================================================
int vsnprintf(char *buf, size_t size, const char *format, va_list arg)
{
        str_out_t so;
        so.out.write = str_write;
        so.buf = buf;
        so.size = buf ? size : 0;
        so.pos = 0;

        int n = _libc_vformat(&so.out, format, arg);

        if (so.size > 0) {
                so.buf[so.pos] = '\0';
        }

        return n;
}

//==============================================================================
/**
 * @brief Function convert arguments to stream (POSIX snprintf()).
 *
 * @param[in] *buf           buffer for stream
 * @param[in]  size          buffer size
 * @param[in] *format        message format
 * @param[in]  ...           argument list
 *
 * @return Number of characters that would be written if the buffer was large
 *         enough. On error negative value is returned.
 */
//==============================================================================
int _libc_snprintf(char *buf, size_t size, const char *format, ...)
{
        va_list arg;
        va_start(arg, format);
        int status = vsnprintf(buf, size, format, arg);
        va_end(arg);
        return status;
}

//==============================================================================
/**
 * @brief Function convert arguments to stream (POSIX sprintf()).
 *
 * @param[in] *buf           buffer for stream
 * @param[in] *format        message format
 * @param[in]  ...           argument list
 *
 * @return Number of written characters. On error negative value is returned.
 */
//==============================================================================
int _libc_sprintf(char *s, const char *format, ...)
{
        va_list arg;
        va_start(arg, format);
        int status = vsnprintf(s, PTRDIFF_MAX, format, arg); // maximum object size
        va_end(arg);
        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
