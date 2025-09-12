/*==============================================================================
File    grep.c

Author  Daniel Zorychta

Brief   grep.

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
#include <string.h>
#include <dnx/vt100.h>
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
GLOBAL_VARIABLES_SECTION {
        char str[512];
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(grep, STACK_DEPTH_LOW);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * Main program function.
 *
 * Note: Please adjust stack size according to programs needs.
 *
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc == 1) {
                printf("Usage: %s PATTERN [FILE]\n", argv[0]);
                // no return, input stream need to be empty
        }

        FILE *input = (argc == 3) ? fopen(argv[2], "r") : stdin;

        while (fgets(global->str, sizeof(global->str), input)) {

                const char *point = strstr(global->str, argv[1]);
                if (point) {
                        size_t begin_len  = point - global->str;
                        size_t middle_len = strlen(argv[1]);

                        printf("%.*s"VT100_FONT_BOLD VT100_FONT_COLOR_RED
                               "%.*s"VT100_RESET_ATTRIBUTES"%s",
                               begin_len, global->str, middle_len,
                               point, point + middle_len);
                }
        }

        if (input != stdin) fclose(input);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/

