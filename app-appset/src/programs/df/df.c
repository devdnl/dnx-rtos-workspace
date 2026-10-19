/*==============================================================================
@file   df.c

@author Daniel Zorychta

@brief  List mounted file systems

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
#include <mntent.h>
#include <dnx/misc.h>
#include <dnx/vt100.h>
#include <dnx/thread.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define KiB                  (u32_t)(1024)
#define MiB                  (u32_t)(1024 * 1024)
#define GiB                  (u64_t)(1024 * 1024 * 1024)
#define CONVERT_TO_KiB(_val) (_val >> 10)
#define CONVERT_TO_MiB(_val) (_val >> 20)
#define CONVERT_TO_GiB(_val) (_val >> 30)

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
PROGRAM_PARAMS(df, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Cat main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;

        printf(VT100_FONT_BOLD "File system" VT100_CURSOR_FORWARD(6) "Total" VT100_CURSOR_FORWARD(
                6) "Free" VT100_CURSOR_FORWARD(7) "%%Used  Mount point" VT100_RESET_ATTRIBUTES "\n");

        struct mntent mnt;
        size_t i = 0;
        while (getmntentry(i++, &mnt) == 0) {
                u32_t dtotal;
                u32_t dfree;
                const char *unit;

                if (mnt.mnt_total > 1 * GiB) {
                        dtotal = CONVERT_TO_MiB(mnt.mnt_total);
                        dfree = CONVERT_TO_MiB(mnt.mnt_free);
                        unit = "MiB";
                } else if (mnt.mnt_total > 1 * MiB) {
                        dtotal = CONVERT_TO_KiB(mnt.mnt_total);
                        dfree = CONVERT_TO_KiB(mnt.mnt_free);
                        unit = "KiB";
                } else {
                        dtotal = mnt.mnt_total;
                        dfree = mnt.mnt_free;
                        unit = "B";
                }

                u32_t percent = ((dtotal - dfree) * 1000) / dtotal;

                if (isstreq(mnt.mnt_src, "/")) {
                        mnt.mnt_src = "";
                }

                printf("%s" VT100_CURSOR_BACKWARD(90) VT100_CURSOR_FORWARD(17) "%u %s" VT100_CURSOR_BACKWARD(
                               90) VT100_CURSOR_FORWARD(28) "%u %s" VT100_CURSOR_BACKWARD(90)
                                VT100_CURSOR_FORWARD(39) "%u.%u%%" VT100_CURSOR_BACKWARD(90)
                                        VT100_CURSOR_FORWARD(46) "%s %s%s%s\n",
                        mnt.mnt_fsname, dtotal, unit, dfree, unit, percent / 10, percent % 10, mnt.mnt_dir,
                        isstrempty(mnt.mnt_src) ? "" : "(", mnt.mnt_src, isstrempty(mnt.mnt_src) ? "" : ")");
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
