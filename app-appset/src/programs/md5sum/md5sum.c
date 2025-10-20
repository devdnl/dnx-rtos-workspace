/*==============================================================================
File    md5sum.c

Author  Daniel Zorychta

Brief   MD5 calculator.

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
#include <md5.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void calculate_hash(const char *filename);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        MD5_CTX ctx;
        uint8_t results[16];
        uint8_t buf[32768];
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(md5sum, STACK_DEPTH_LOW);

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
                printf("Usage: %s [FILE] [FILE]...\n", argv[0]);
                return EXIT_FAILURE;

        } else {

                for (int i = 1; i < argc; i++) {
                        calculate_hash(argv[i]);
                }
        }

        return EXIT_SUCCESS;
}

//==============================================================================
/**
 * @brief  Function calculate MD5 for selected file.
 *
 * @param  filename     file to calculate hash
 */
//==============================================================================
static void calculate_hash(const char *filename)
{
        FILE *f = fopen(filename, "r");
        if (f) {

                md5_init(&global->ctx);

                size_t n;

                while ((n = fread(global->buf, 1, sizeof(global->buf), f)) > 0) {

                        md5_update(&global->ctx, global->buf, n);
                }

                if (ferror(f)) {
                        perror(filename);

                } else {
                        md5_final(&global->ctx, global->results);

                        for (size_t i = 0; i < sizeof(global->results); i++) {
                                printf("%02x", global->results[i]);
                        }

                        printf("  %s\n", filename);
                }

                fclose(f);

        } else {
                perror(filename);
        }
}

/*==============================================================================
  End of file
==============================================================================*/
