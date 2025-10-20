/*==============================================================================
File    more.c

Author  Daniel Zorychta

Brief   Show more data.

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
#include <sys/ioctl.h>
#include <unistd.h>

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
        char line[256];
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(more, STACK_DEPTH_LOW);

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
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        FILE *file;

        if ((argc == 2) && argv[1]) {
                file = fopen(argv[1], "r+");

                if (!file) {
                        perror(argv[1]);
                        return EXIT_FAILURE;
                }

        } else {
                file = stdin;
        }

        ioctl(fileno(stdout), IOCTL_VFS__NON_BLOCKING_RD_MODE);
        ioctl(fileno(stdout), IOCTL_TTY__ECHO_OFF);

        int ROWS = 23;
        ioctl(fileno(stdout), IOCTL_TTY__GET_ROW, &ROWS);
        ROWS -= 1;

        int show = ROWS;

        while (true) {
                for (int i = 0; i < show; i++) {
                        if (fgets(global->line, sizeof(global->line), file)) {
                                printf("%s", global->line);
                        } else {
                                goto exit;
                        }
                }

                while (true) {
                        int c = getc(stdout);

                        if (c == '\n') {
                                show = 1;
                                break;

                        } else if (c == ' ') {
                                show = ROWS;
                                break;
                        }

                        msleep(20);
                }
        }

exit:
        ioctl(fileno(stdout), IOCTL_TTY__ECHO_ON);
        ioctl(fileno(stdout), IOCTL_VFS__DEFAULT_RD_MODE);

        if (file != stdin) {
                fclose(file);
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
