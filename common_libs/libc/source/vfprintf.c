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
#include <dnx/misc.h>
#include <errno.h>

/*==============================================================================
  Local macros
==============================================================================*/

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
 * @brief Function write to file formatted string.
 *
 * @param file                file
 * @param format              formated text
 * @param arg                 arguments
 *
 * @retval number of written characters.
 */
//==============================================================================
int vfprintf(FILE *file, const char *format, va_list arg)
{
        int n = 0;

        va_list carg;
        va_copy(carg, arg);
        u32_t size = vsnprintf(NULL, 0, format, carg) + 1;
        va_end(carg);

        if (size < BUFSIZ) {
                char str[BUFSIZ];
                n = vsnprintf(str, sizeof(str), format, arg);
                fwrite(str, sizeof(char), n, file);

        } else {
                char *str = malloc(size);
                if (str) {
                        n = vsnprintf(str, size, format, arg);
                        fwrite(str, sizeof(char), n, file);
                        free(str);
                }
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
 * @return number of written characters.
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
 * @return number of written characters.
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
