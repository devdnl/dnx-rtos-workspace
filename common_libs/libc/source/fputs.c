/*==============================================================================
@file   fputs.c

@author Daniel Zorychta

@brief  String put functions.

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
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
 * @brief Function puts string to selected file (fputs & puts)
 *
 * @param[in] *s        string
 * @param[in] *file     file
 * @param[in]  puts     puts functionality (true: add \n at the end of string)
 *
 * @return number of characters written to the stream
 */
//==============================================================================
static int f_puts(const char *s, FILE *file, bool puts)
{
        if (file) {
                int n = EOF;

                if (puts) {
                        char *buf = malloc(strlen(s) + 2);
                        if (buf) {
                                strcpy(buf, s);
                                strcat(buf, "\n");
                                n = fwrite(buf, sizeof(char), strlen(buf), file);
                                free(buf);
                        }
                } else {
                        n = fwrite(s, sizeof(char), strlen(s), file);
                }

                if (n != 0) {
                        return n;
                }
        }
        return EOF;
}

//==============================================================================
/**
 * @brief Function puts string to selected file
 *
 * @param[in] *s        string
 * @param[in] *file     file
 *
 * @return number of characters written to the stream
 */
//==============================================================================
int fputs(const char *s, FILE *file)
{
        return f_puts(s, file, false);
}

//==============================================================================
/**
 * @brief Function puts string to stdout
 *
 * @param[in] *s        string
 *
 * @return number of characters written to the stream
 */
//==============================================================================
int puts(const char *s)
{
        return f_puts(s, stdout, true);
}

/*==============================================================================
  End of file
==============================================================================*/
