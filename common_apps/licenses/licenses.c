/*==============================================================================
@file   licenses.c

@author Daniel Zorychta

@brief  Print licenses of the build (open source components and license texts).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#include <zlib.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DEFAULT_FILE "/etc/licenses.txt.gz"
#define WINDOW_BITS  12 /* window of kernel/tools/licenses.py (4 KiB) */
#define CHUNK        256

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        z_stream strm;
        unsigned char in[CHUNK];
        unsigned char out[CHUNK];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(licenses, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *path = (argc > 1) ? argv[1] : DEFAULT_FILE;

        if ((argc > 2) || ((argc == 2) && (argv[1][0] == '-'))) {
                printf("Usage: %s [file]\n", argv[0]);
                printf("Prints licenses of the build: open source components and license texts\n");
                printf("(gzip file created by build, default: %s).\n", DEFAULT_FILE);
                return EXIT_FAILURE;
        }

        FILE *f = fopen(path, "r");
        if (!f) {
                perror(path);
                return EXIT_FAILURE;
        }

        int err = inflateInit2(&global->strm, 16 + WINDOW_BITS);

        while (err == Z_OK) {
                if (global->strm.avail_in == 0) {
                        global->strm.avail_in = fread(global->in, 1, CHUNK, f);
                        global->strm.next_in = global->in;
                        if (global->strm.avail_in == 0) {
                                err = Z_DATA_ERROR; /* unexpected end of file */
                                break;
                        }
                }

                global->strm.next_out = global->out;
                global->strm.avail_out = CHUNK;
                err = inflate(&global->strm, Z_NO_FLUSH);
                fwrite(global->out, 1, CHUNK - global->strm.avail_out, stdout);
        }

        inflateEnd(&global->strm);
        fclose(f);

        if (err != Z_STREAM_END) {
                printf("%s: invalid file (%d)\n", path, err);
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
