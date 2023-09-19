/*==============================================================================
File    md5sum.c

Author  Daniel Zorychta

Brief   MD5 calculator.

        Copyright (C) 2018 Daniel Zorychta <daniel.zorychta@gmail.com>

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation and modified by the dnx RTOS exception.

        NOTE: The modification  to the GPL is  included to allow you to
              distribute a combined work that includes dnx RTOS without
              being obliged to provide the source  code for proprietary
              components outside of the dnx RTOS.

        The dnx RTOS  is  distributed  in the hope  that  it will be useful,
        but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
        MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
        GNU General Public License for more details.

        Full license text is available on the following file: doc/license.txt.

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
        MD5_CTX  ctx;
        uint8_t  results[16];
        uint8_t  buf[32768];
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
