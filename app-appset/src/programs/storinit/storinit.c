/*==============================================================================
@file   storinit.c

@author Daniel Zorychta

@brief  Storage initialization (SD cards, HDD, etc)

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
#include <sys/ioctl.h>
#include <errno.h>

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
PROGRAM_PARAMS(storinit, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int status = EXIT_FAILURE;

        if (argc == 1) {
                printf("Usage: %s [file]\n", argv[0]);
                return EXIT_FAILURE;
        }

        errno = 0;
        FILE *storage = fopen(argv[1], "r");
        if (storage) {
                if (ioctl(fileno(storage), IOCTL_STORAGE__INITIALIZE) == 0) {
                        status = EXIT_SUCCESS;

                        /* partition table is read only by drivers that support it (PART) */
                        if ((ioctl(fileno(storage), IOCTL_STORAGE__READ_MBR) != 0) && (errno != EBADRQC)) {
                                perror(argv[1]);
                                status = EXIT_FAILURE;
                        }

                } else {
                        perror(argv[1]);
                }

                fclose(storage);
        } else {
                perror(argv[1]);
        }

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
