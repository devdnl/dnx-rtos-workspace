/*==============================================================================
@file    echo.c

@author  Daniel Zorychta

@brief   Print messages on terminal

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
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(echo, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Convert escape sequences in place (option -e): \\ \n \r \t \xHH.
 *
 * @param str           string
 *
 * @return Length of converted string (can contain zero bytes).
 */
//==============================================================================
static size_t unescape(char *str)
{
        char *dst = str;

        for (char *src = str; *src; src++) {
                if (*src != '\\' || src[1] == '\0') {
                        *dst++ = *src;
                        continue;
                }

                switch (*++src) {
                case 'n': *dst++ = '\n'; break;
                case 'r': *dst++ = '\r'; break;
                case 't': *dst++ = '\t'; break;
                case 'x': {
                        char hex[3] = {0};
                        int n = 0;
                        while (n < 2 && strchr("0123456789abcdefABCDEF", src[1]) && src[1]) {
                                hex[n++] = *++src;
                        }
                        *dst++ = n ? (char)strtol(hex, NULL, 16) : 'x';
                        break;
                }
                default: *dst++ = *src; break;
                }
        }

        return dst - str;
}

//==============================================================================
/**
 * @brief Echo main function. Options: -n (no new line), -e (escape sequences).
 */
//==============================================================================
int main(int argc, char *argv[])
{
        bool newline = true;
        bool escape = false;
        int i = 1;

        for (; i < argc && argv[i][0] == '-' && argv[i][1] && strspn(&argv[i][1], "ne") == strlen(&argv[i][1]); i++) {
                newline = newline && !strchr(argv[i], 'n');
                escape  = escape  || strchr(argv[i], 'e');
        }

        for (int first = i; i < argc; i++) {
                if (i > first) {
                        putchar(' ');
                }

                size_t len = escape ? unescape(argv[i]) : strlen(argv[i]);
                fwrite(argv[i], 1, len, stdout);
        }

        if (newline) {
                putchar('\n');
        }

        fflush(stdout);

        if (ferror(stdout)) {
                fprintf(stderr, "echo: %s\n", strerror(errno));
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
