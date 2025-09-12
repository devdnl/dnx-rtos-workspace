/*==============================================================================
File    strlcat.c

Author  Daniel Zorychta

Brief   String functions.

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
 * @brief The strlcat() function concatenate strings.
 *
 * The strlcat() function appends the NUL-terminated string src to the end of
 * dst. It will append at most size - strlen(dst) - 1 bytes, NUL-terminating
 * the result.
 *
 * @param  dst  destination buffer
 * @param  src  source buffer
 * @param  size destination buffer size
 *
 * @return The strlcat() function return the total length of the string it
 *         tried to create. For strlcat() that means the initial length of dst
 *         plus the length of src. While this may seem somewhat confusing, it
 *         was done to make truncation detection simple.
 */
//==============================================================================
size_t _libc_strlcat(char *dst, const char *src, size_t size)
{
        char       *d = dst;
        const char *s = src;
        size_t      n = size;
        size_t      dlen;

        /* Find the end of dst and adjust bytes left but don't go past end */
        while (n-- != 0 && *d != '\0') {
                d++;
        }

        dlen = d - dst;
        n    = size - dlen;

        if (n == 0) {
                return (dlen + strlen(s));
        }

        while (*s != '\0') {
                if (n != 1) {
                        *d++ = *s;
                         n--;
                }
                s++;
        }

        *d = '\0';

        /* count does not include NUL */
        return (dlen + (s - src));
}

/*==============================================================================
  End of file
==============================================================================*/
