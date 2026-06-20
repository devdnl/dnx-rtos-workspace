/*==============================================================================
@file   lsmod.c

@author Daniel Zorychta

@brief  Module listing

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
#include <string.h>
#include <stdbool.h>
#include <sys/mount.h>
#include <dnx/os.h>
#include <dnx/vt100.h>
#include <dnx/misc.h>
#include <errno.h>
#include <dirent.h>

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
PROGRAM_PARAMS(lsmod, STACK_DEPTH_LOW);

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
        (void)argc;
        (void)argv;

        size_t max_name_len = 0;
        size_t drv_number = get_number_of_drivers();
        for (size_t i = 0; i < drv_number; i++) {
                max_name_len = max(max_name_len, strlen(get_driver_name(i)));
        }
        max_name_len++;

        printf(VT100_FONT_BOLD "ID" VT100_CURSOR_BACKWARD(99)
                        VT100_CURSOR_FORWARD(4) "Name" VT100_CURSOR_BACKWARD(99)
                                VT100_CURSOR_FORWARD(%u) "Instances" VT100_RESET_ATTRIBUTES "\n",
                max_name_len + 5);

        for (size_t i = 0; i < drv_number; i++) {

                printf("%d" VT100_CURSOR_BACKWARD(99) VT100_CURSOR_FORWARD(4) "%s" VT100_CURSOR_BACKWARD(99)
                                VT100_CURSOR_FORWARD(%u) "%d\n",
                        i, get_driver_name(i), max_name_len + 5, get_number_of_driver_instances(i));
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
