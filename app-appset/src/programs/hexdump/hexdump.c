/*==============================================================================
File    hexdump.c

Author  Daniel Zorychta

Brief   Hex presentation program.

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
#include <ctype.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define printline(buf, ...) snprintf(buf, sizeof(global->line) - (ptr - global->line), __VA_ARGS__)

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
        u8_t buffer[16];
        char line[100];
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(hexdump, STACK_DEPTH_LOW);

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
                printf("Usage: %s <file> [seek] [bytes]\n", argv[0]);
                return EXIT_FAILURE;
        }

        u32_t seek  = 0;
        u32_t bytes = UINT32_MAX;

        if (argc >= 3) {
                seek = strtol(argv[2], NULL, 0);
        }

        if (argc >= 4) {
                bytes = strtol(argv[3], NULL, 0);
        }

        FILE *f = fopen(argv[1], "r");
        if (f) {
                fseek(f, seek, SEEK_SET);

                ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);

                do {
                        if (getc(stdin) == ETX) {
                                break;
                        }

                        u32_t seek = ftell(f);
                        u32_t len  = min(sizeof(global->buffer), bytes);

                        size_t n = fread(global->buffer, 1, len, f);

                        char *ptr = global->line;

                        // file offset
                        ptr += printline(ptr, "%08x  ", seek);

                        // 0-7 byte
                        for (size_t i = 0; (i < 8) && (i < n); i++) {
                                ptr += printline(ptr, "%02x ", global->buffer[i]);
                        }

                        // delimiter
                        ptr += printline(ptr, " ");

                        // 8-16 byte
                        for (size_t i = 8; i < n; i++) {
                                ptr += printline(ptr, "%02x ", global->buffer[i]);
                        }

                        // print empty space if less than 16 bytes
                        int s = 16 - n;

                        while (s > 0) {
                                ptr += printline(ptr, "   ");
                                s--;
                        }

                        // characters
                        ptr += printline(ptr, " |");
                        for (size_t i = 0; i < n; i++) {
                                char c = global->buffer[i];
                                ptr += printline(ptr, "%c", isprint(c) ? c : ' ');
                        }

                        // end
                        ptr += printline(ptr, "|");

                        puts(global->line);

                        bytes -= n;

                        if (n == 0) break;

                } while ((bytes > 0) && !feof(f));

                ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);

                fclose(f);

                return EXIT_SUCCESS;

        } else {
                perror(argv[1]);
                return EXIT_FAILURE;
        }
}

/*==============================================================================
  End of file
==============================================================================*/

