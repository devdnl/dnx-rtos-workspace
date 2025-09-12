/*==============================================================================
File    strndup.c

Author  Daniel Zorychta

Brief   String duplication.

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
 * @brief The strndup() function is used to duplicate a string of size n.
 *
 * This function returns a pointer to a null-terminated byte string, which is
 * a duplicate of the string pointed to by s. The memory obtained is done
 * dynamically using malloc and hence it can be freed using free().
 *
 * @param s             string
 * @param n             string size
 *
 * @return It returns a pointer to the duplicated string s.
 */
//==============================================================================
char *strndup(const char *s, size_t n)
{
        extern size_t _libc_strlcpy(char *dst, const char *src, size_t size);
        extern void *_libc_malloc(size_t size);

        n += 1;
        char *dup = _libc_malloc(n);
        if (dup) {
                _libc_strlcpy(dup, s, n);
        }

        return dup;
}

/*==============================================================================
  End of file
==============================================================================*/
