/*==============================================================================
@file   vfprintf.c

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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dnx/misc.h>
#include <errno.h>
#include <libc/source/vformat.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        _libc_fmt_out_t out; //!< must be the first member
        FILE *file;          //!< destination file
        size_t len;          //!< number of characters in buffer
        char buf[BUFSIZ];    //!< output buffer
} file_out_t;

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
 * @brief  Write buffered characters to file.
 *
 * @param  fo           file sink
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool file_flush(file_out_t *fo)
{
        bool ok = true;

        if (fo->len > 0) {
                ok = (fwrite(fo->buf, sizeof(char), fo->len, fo->file) == fo->len);
                fo->len = 0;
        }

        return ok;
}

//==============================================================================
/**
 * @brief  File sink write function. Text is collected in the buffer, so
 *         messages shorter than BUFSIZ are written by single write.
 *
 * @param  out          sink
 * @param  str          text
 * @param  len          text length
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool file_write(_libc_fmt_out_t *out, const char *str, size_t len)
{
        file_out_t *fo = (file_out_t *)out;

        while (len > 0) {
                if (fo->len == sizeof(fo->buf)) {
                        if (!file_flush(fo)) {
                                return false;
                        }
                }

                size_t room = sizeof(fo->buf) - fo->len;
                size_t n = (len < room) ? len : room;
                memcpy(fo->buf + fo->len, str, n);
                fo->len += n;
                str += n;
                len -= n;
        }

        return true;
}

//==============================================================================
/**
 * @brief Function write to file formatted string (POSIX vfprintf()).
 *
 * See vformat.c for supported conversion specifications.
 *
 * @param file                file
 * @param format              formated text
 * @param arg                 arguments
 *
 * @return Number of written characters. On error negative value is returned.
 */
//==============================================================================
int vfprintf(FILE *file, const char *format, va_list arg)
{
        file_out_t fo;
        fo.out.write = file_write;
        fo.file = file;
        fo.len = 0;

        int n = _libc_vformat(&fo.out, format, arg);

        // flush also on format error (e.g. EILSEQ) to output preceding text
        if (!file_flush(&fo)) {
                n = -1;
        }

        return n;
}

//==============================================================================
/**
 * @brief  Function write to stdout formatted string.
 *
 * @param format              formated text
 * @param ...                 format arguments
 *
 * @return Number of written characters. On error negative value is returned.
 */
//==============================================================================
int _libc_printf(const char *format, ...)
{
        va_list arg;
        va_start(arg, format);
        int status = vfprintf(stdout, format, arg);
        va_end(arg);
        return status;
}

//==============================================================================
/**
 * @brief  Function write to file formatted string.
 *
 * @param streamm             stream
 * @param format              formated text
 * @param ...                 format arguments
 *
 * @return Number of written characters. On error negative value is returned.
 */
//==============================================================================
int _libc_fprintf(FILE *stream, const char *format, ...)
{
        va_list arg;
        va_start(arg, format);
        int status = vfprintf(stream, format, arg);
        va_end(arg);
        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
