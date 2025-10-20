/*==============================================================================
File    chmod.c

Author  Daniel Zorychta

Brief   Modify file mode.

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
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
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
GLOBAL_VARIABLES_SECTION {
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(chmod, STACK_DEPTH_VERY_LOW);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Print help message.
 *
 * @param  name         this program name
 */
//==============================================================================
static void print_help(const char *name)
{
        printf("Usage: %s MODE FILE\n\n", name);
        printf("Mode:\n");
        printf("   [+|-][x|r|w]  flag selection (only USR)\n");
        printf("   value         mode value\n");
}

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
        int err = EXIT_FAILURE;

        if (argc < 3) {
                print_help(argv[0]);
        } else {
                struct stat st;
                if (stat(argv[2], &st) == 0) {

                        bool set = argv[1][0] == '+';
                        bool unset = argv[1][0] == '-';

                        if (set || unset) {
                                mode_t flag = 0;

                                switch (argv[1][1]) {
                                case 'r':
                                        flag = S_IRUSR;
                                        break;
                                case 'w':
                                        flag = S_IWUSR;
                                        break;
                                case 'x':
                                        flag = S_IXUSR;
                                        break;
                                }

                                st.st_mode |= (set ? flag : 0);
                                st.st_mode &= ~(unset ? flag : 0);
                        } else {
                                if (isdigit(argv[1][0])) {
                                        char *end;
                                        st.st_mode = strtol(argv[1], &end, 0);
                                } else {
                                        print_help(argv[0]);
                                        return EXIT_FAILURE;
                                }
                        }

                        if (chmod(argv[2], st.st_mode) == 0) {
                                err = EXIT_SUCCESS;
                        } else {
                                perror(argv[2]);
                        }

                } else {
                        perror(argv[2]);
                }
        }

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
