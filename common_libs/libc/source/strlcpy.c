/*==============================================================================
File    strlcpy.c

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
 * @brief The strlcpy() function copy strings.
 *
 * The strlcpy() function copies up to size - 1 characters from the NUL-terminated
 * string src to dst, NUL-terminating the result.
 *
 * Note, however, that if strlcat() traverses size characters without finding
 * a NUL, the length of the string is considered to be size and the destination
 * string will not be NUL-terminated (since there was no space for the NUL).
 *
 * @param  dst  destination buffer
 * @param  src  source buffer
 * @param  size destination buffer size
 *
 * @return The strlcpy() function return the total length of the string they
 *         tried to create. For strlcpy() that means the length of src.
 */
//==============================================================================
size_t _libc_strlcpy(char *dst, const char *src, size_t size)
{
        char       *d = dst;
        const char *s = src;
        size_t      n = size;

        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0) break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (size != 0) *d = '\0';
                while (*s++);
        }

        /* count does not include NUL */
        return (s - src - 1);
}

/*==============================================================================
  End of file
==============================================================================*/
