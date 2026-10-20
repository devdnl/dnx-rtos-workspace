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
#include <sys/statfs.h>
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
static void print_usage(const char *name);
static void print_summary(void);
static void print_details(void);
static void print_size(u64_t bytes);
static void print_percent(u64_t part, u64_t total);

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
 * @brief df main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        bool details = false;

        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "-d")) {
                        details = true;

                } else if (isstreq(argv[i], "-h") || isstreq(argv[i], "--help")) {
                        print_usage(argv[0]);
                        return EXIT_SUCCESS;

                } else {
                        printf("%s: unknown option '%s'\n", argv[0], argv[i]);
                        print_usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (details) {
                print_details();
        } else {
                print_summary();
        }

        return EXIT_SUCCESS;
}

//==============================================================================
/**
 * @brief Print program usage.
 *
 * @param name          program name
 */
//==============================================================================
static void print_usage(const char *name)
{
        printf("Usage: %s [-d]\n", name);
        printf("  -d    show detailed information about each file system\n");
        printf("  -h    show this help\n");
}

//==============================================================================
/**
 * @brief Print table of mounted file systems (default view).
 */
//==============================================================================
static void print_summary(void)
{
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

                u32_t percent = (dtotal > 0) ? (((dtotal - dfree) * 1000) / dtotal) : 0;

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
}

//==============================================================================
/**
 * @brief Print detailed information about each mounted file system (-d).
 */
//==============================================================================
static void print_details(void)
{
        struct mntent mnt;
        size_t i = 0;
        bool first = true;

        while (getmntentry(i++, &mnt) == 0) {

                if (!first) {
                        printf("\n");
                }
                first = false;

                printf(VT100_FONT_BOLD "%s" VT100_RESET_ATTRIBUTES "\n", mnt.mnt_dir);
                printf("  File system  : %s\n", mnt.mnt_fsname);
                printf("  Source       : %s\n", isstrempty(mnt.mnt_src) ? "-" : mnt.mnt_src);

                struct statfs fs;
                if (statfs(mnt.mnt_dir, &fs) != 0) {
                        printf("  statfs()     : error\n");
                        continue;
                }

                u64_t bsize   = fs.f_bsize;
                u64_t blocks  = fs.f_blocks;
                u64_t bfree   = fs.f_bfree;
                u64_t bused   = (blocks >= bfree) ? (blocks - bfree) : 0;

                printf("  Type         : 0x%X\n", (u32_t)fs.f_type);
                printf("  Block size   : %u B\n", (u32_t)bsize);

                printf("  Blocks       : %u total, %u free, %u used\n",
                       (u32_t)blocks, (u32_t)bfree, (u32_t)bused);

                printf("  Size         : ");
                print_size(blocks * bsize);
                printf("\n");

                printf("  Free         : ");
                print_size(bfree * bsize);
                printf("\n");

                printf("  Used         : ");
                print_size(bused * bsize);
                printf(" (");
                print_percent(bused, blocks);
                printf(")\n");

                if (fs.f_files > 0) {
                        u32_t files = fs.f_files;
                        u32_t ffree = fs.f_ffree;
                        u32_t fused = (files >= ffree) ? (files - ffree) : 0;

                        printf("  Inodes       : %u total, %u free, %u used (", files, ffree, fused);
                        print_percent(fused, files);
                        printf(")\n");
                } else {
                        printf("  Inodes       : -\n");
                }
        }
}

//==============================================================================
/**
 * @brief Print size in human readable form (one decimal place).
 *
 * @param bytes         size in bytes
 */
//==============================================================================
static void print_size(u64_t bytes)
{
        static const char *const unit[] = {"B", "KiB", "MiB", "GiB", "TiB"};

        u64_t  val  = bytes;
        u32_t  frac = 0;
        size_t u    = 0;

        while ((val >= 1024) && (u < ARRAY_SIZE(unit) - 1)) {
                frac = (u32_t)(((val % 1024) * 10) / 1024);
                val /= 1024;
                u++;
        }

        if (u == 0) {
                printf("%u %s", (u32_t)val, unit[u]);
        } else {
                printf("%u.%u %s", (u32_t)val, frac, unit[u]);
        }
}

//==============================================================================
/**
 * @brief Print percentage part/total with one decimal place.
 *
 * @param part          part value
 * @param total         total value
 */
//==============================================================================
static void print_percent(u64_t part, u64_t total)
{
        u32_t permille = (total > 0) ? (u32_t)((part * 1000) / total) : 0;
        printf("%u.%u%%", permille / 10, permille % 10);
}

/*==============================================================================
  End of file
==============================================================================*/
