/*==============================================================================
File    strncasecmp.c

Author  Daniel Zorychta

Brief   String compare no case sensitive.

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
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

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
 * @brief Function compares the two strings and ignore case of the characters with limits.
 *
 * The strncasecmp() function is similar to strcasecmp(), except it
 * compares the only first <i>n</i> bytes of <i>s1</i>.
 *
 * @param s1        string 1
 * @param s2        string 2
 * @param n         max number of bytes to compare
 *
 * @return The strncasecmp() function return an integer less than, equal to, or
 * greater than zero if <i>s1</i> or the first <i>n</i> bytes thereof is found,
 * respectively, to be less than, to match, or be greater than <i>s2</i>.
 *
 * @b Example
 * @code
        // ...
        const char *foo = "FOO";
        const char *bar = "foo";

        if (strncasecmp(foo, bar, 3) == 0) {
                // result: functions are equal
        }

        // ...
   @endcode
 *
 * @see strcasecmp(), strcmp(), strncmp(), memcmp()
 */
//==============================================================================
int strncasecmp(const char *s1, const char *s2, size_t n)
{
        if (!s1 || !s2 || !n) {
                return 1;
        }

        do {
                int d = tolower(*s1++) - tolower(*s2++);
                if (d != 0) {
                        return d;
                }

                if (--n == 0) {
                        break;
                }

                if (*s1 == '\0' || *s2 == '\0') {
                        return *s1 - *s2;
                }

        } while (*s1 && *s2);

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
