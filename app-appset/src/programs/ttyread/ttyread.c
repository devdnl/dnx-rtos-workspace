/*==============================================================================
File    ttyread.c

Author  Daniel Zorychta

Brief   TTY reader

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
        char *buf;
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(ttyread, STACK_DEPTH_VERY_LOW);

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
        if (argc < 2) {
                printf("Usage: %s <tty-file>\n", argv[0]);
                return EXIT_FAILURE;

        } else {
                FILE *tty = fopen(argv[1], "r");
                if (tty) {

                        int cols = 0;
                        int rows = 0;

                        ioctl(fileno(tty), IOCTL_TTY__GET_COL, &cols);
                        ioctl(fileno(tty), IOCTL_TTY__GET_ROW, &rows);

                        size_t buf_size = cols * rows;
                        global->buf = malloc(buf_size);

                        TTY_buffer_t buf;
                        buf.ptr  = global->buf;
                        buf.size = buf_size;

                        int err = ioctl(fileno(tty), IOCTL_TTY__READ_BUFFER, &buf);
                        if (!err) {
                                fwrite(global->buf, 1, buf_size - buf.size, stdout);

                        } else {
                                perror(argv[1]);
                        }

                        fclose(tty);
                } else {
                        perror(argv[1]);
                }
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/

