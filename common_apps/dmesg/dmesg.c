/*==============================================================================
File    dmesg.c

Author  Daniel Zorychta

Brief   System log reader

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
#include <unistd.h>
#include <stdbool.h>
#include <dnx/misc.h>
#include <dnx/os.h>
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
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(dmesg, STACK_DEPTH_LOW);

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
        bool clear = false;
        bool loop = false;

        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "-h") || isstreq(argv[i], "--help")) {
                        printf("Usage: %s [options]\n\n", argv[0]);
                        puts("Options:");
                        puts("  -C, --clear     log clear");
                        puts("  -h, --help      this help");
                        puts("  -l,             loop");
                        return EXIT_FAILURE;
                }

                if (isstreq(argv[i], "-C") || isstreq(argv[i], "--help")) {
                        clear = true;
                }

                if (isstreq(argv[i], "-l")) {
                        loop = true;
                }
        }

        if (clear) {
                syslog_clear();

        } else {
                struct timeval t = {0, 0};

                ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);

                do {
                        char str[128];

                        while (syslog_read(str, sizeof(str), &t, &t)) {
                                printf("[%llu.%06lu] %s\n", t.tv_sec, t.tv_usec, str);
                        }

                        if (loop) {
                                clearerr(stdin);
                                int c = getchar();
                                if (c == ETX) {
                                        loop = false;
                                } else {
                                        msleep(100);
                                }
                        }
                } while (loop);

                ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
