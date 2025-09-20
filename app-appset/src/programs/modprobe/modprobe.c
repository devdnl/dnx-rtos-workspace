/*==============================================================================
@file   modprobe.c

@author Daniel Zorychta

@brief  Module control.

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
#include <errno.h>
#include <sys/mount.h>
#include <dnx/os.h>

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
PROGRAM_PARAMS(modprobe, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Print help
 */
//==============================================================================
static void show_help(const char *name)
{
        printf("Usage: %s <flag> [OPTIONS]\n", name);
        puts("  -i            initialize module <module name> <major> <minor> [node path]");
        puts("  -r            release module <module name> <major> <minor>");
        puts("  -R            release module <module path>");
        puts("  -h, --help    this help");
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int err = EXIT_FAILURE;

        const char *action  = NULL;
        const char *modname = NULL;
        const char *path    = NULL;
        int         major   = 0;
        int         minor   = 0;


        if (argc < 2) {
                show_help(argv[0]);

        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
                show_help(argv[0]);

        } else if ((strcmp(argv[1], "-i") == 0) && (argc >= 5)) {

                action  = "initialize";
                modname = argv[2];
                major   = atoi(argv[3]);
                minor   = atoi(argv[4]);
                path    = argc >= 6 ? argv[5] : NULL;

                if (driver_init(modname, major, minor, path) >= 0) {
                        err = EXIT_SUCCESS;
                }

        } else if ((strcmp(argv[1], "-r") == 0) && (argc == 5)) {

                action  = "release";
                modname = argv[2];
                major   = atoi(argv[3]);
                minor   = atoi(argv[4]);

                if (driver_release(modname, major, minor) == 0) {
                        err = EXIT_SUCCESS;
                }

        } else if ((strcmp(argv[1], "-R") == 0) && (argc == 3)) {

                action = "release";
                path   = argv[2];

                if (driver_release2(path) == 0) {
                        err = EXIT_SUCCESS;
                }

        } else {
                show_help(argv[0]);
        }


        if (action) {
                if (!err) {
                        if (path) {
                                printf("Module '%s' %sd.\n", path, action);

                        } else {
                                printf("Module '%s%d-%d' %sd.\n",
                                       modname, major, minor, action);
                        }

                } else {
                        if (path) {
                                fprintf(stderr, "Module '%s' error: %s.\n",
                                        path, strerror(errno));

                        } else {
                                fprintf(stderr, "Module '%s%d-%d' error: %s.\n",
                                        modname, major, minor, strerror(errno));
                        }
                }
        }

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
