/*==============================================================================
@file   iostat.c

@author Daniel Zorychta

@brief  Block device I/O statistics.

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
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DEV_DIR "/dev"

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef enum {
        CMD_SHOW,
        CMD_ENABLE,
        CMD_DISABLE,
} cmd_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        STORAGE_stats_t stats;
        char path[128];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(iostat, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Function prints program usage.
 *
 * @param  name         program name
 */
//==============================================================================
static void print_usage(const char *name)
{
        printf("Usage: %s [-e|-d] [device...]\n", name);
        puts("  -e    enable (and reset) statistics");
        puts("  -d    disable statistics");
        puts("Without options statistics are printed. Without devices all block");
        puts("devices from " DEV_DIR " are used.");
}

//==============================================================================
/**
 * @brief  Function returns percentage value.
 *
 * @param  part         part
 * @param  total        total
 *
 * @return Percent value.
 */
//==============================================================================
static unsigned percent(u32_t part, u32_t total)
{
        return total ? (unsigned)((u64_t)part * 100 / total) : 0;
}

//==============================================================================
/**
 * @brief  Function prints statistics of device.
 *
 * @param  path         device path
 * @param  s            statistics
 */
//==============================================================================
static void print_stats(const char *path, const STORAGE_stats_t *s)
{
        printf("%s: block size %lu B", path, (unsigned long)s->blksize);

        if (!s->enabled) {
                puts(", statistics disabled");
                return;
        }

        printf(", unit %lu B\n", (unsigned long)s->unit);
        puts("        requests          bytes  drv calls  unaligned  bounce  errors     1   2-7  8-63   64+");

        printf("  read  %8lu %14llu  %9lu  %9lu  %6lu  %6lu %5lu %5lu %5lu %5lu\n", (unsigned long)s->rd_req,
                (unsigned long long)s->rd_bytes, (unsigned long)s->rd_drv, (unsigned long)s->rd_unaligned,
                (unsigned long)s->rd_bounce, (unsigned long)s->rd_err, (unsigned long)s->rd_size[0],
                (unsigned long)s->rd_size[1], (unsigned long)s->rd_size[2], (unsigned long)s->rd_size[3]);

        printf("  write %8lu %14llu  %9lu  %9lu  %6lu  %6lu %5lu %5lu %5lu %5lu\n", (unsigned long)s->wr_req,
                (unsigned long long)s->wr_bytes, (unsigned long)s->wr_drv, (unsigned long)s->wr_unaligned,
                (unsigned long)s->wr_bounce, (unsigned long)s->wr_err, (unsigned long)s->wr_size[0],
                (unsigned long)s->wr_size[1], (unsigned long)s->wr_size[2], (unsigned long)s->wr_size[3]);

        printf("  LRU model (%u blocks, requests <= %u blocks): read hits %lu/%lu (%u%%), write hits %lu/%lu "
               "(%u%%)\n",
                STORAGE_STATS_LRU_BLOCKS, STORAGE_STATS_LRU_MAX_REQ, (unsigned long)s->rd_lru_hits,
                (unsigned long)s->rd_lru_blocks, percent(s->rd_lru_hits, s->rd_lru_blocks),
                (unsigned long)s->wr_lru_hits, (unsigned long)s->wr_lru_blocks,
                percent(s->wr_lru_hits, s->wr_lru_blocks));
}

//==============================================================================
/**
 * @brief  Function executes command on selected device.
 *
 * @param  path         device path
 * @param  cmd          command
 * @param  quiet        do not report devices that are not block devices
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int handle_device(const char *path, cmd_t cmd, bool quiet)
{
        struct stat st;
        if (stat(path, &st) != 0) {
                perror(path);
                return -1;
        }

        if (!S_ISBLK(st.st_mode)) {
                if (!quiet) {
                        printf("%s: not a block device\n", path);
                }
                return quiet ? 0 : -1;
        }

        FILE *f = fopen(path, "r");
        if (!f) {
                perror(path);
                return -1;
        }

        int status = 0;

        switch (cmd) {
        case CMD_ENABLE:
                status = ioctl(fileno(f), IOCTL_STORAGE__STATS_ENABLE);
                break;

        case CMD_DISABLE:
                status = ioctl(fileno(f), IOCTL_STORAGE__STATS_DISABLE);
                break;

        case CMD_SHOW:
                status = ioctl(fileno(f), IOCTL_STORAGE__GET_STATS, &global->stats);
                if (status == 0) {
                        print_stats(path, &global->stats);
                }
                break;
        }

        if (status != 0) {
                perror(path);
        }

        fclose(f);

        return status;
}

//==============================================================================
/**
 * @brief  Function executes command on all block devices from /dev directory.
 *
 * @param  cmd          command
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int handle_all_devices(cmd_t cmd)
{
        DIR *dir = opendir(DEV_DIR);
        if (!dir) {
                perror(DEV_DIR);
                return -1;
        }

        int status = 0;

        struct dirent *dirent;
        while ((dirent = readdir(dir))) {
                if (S_ISBLK(dirent->mode)) {
                        snprintf(global->path, sizeof(global->path), DEV_DIR "/%s", dirent->d_name);

                        if (handle_device(global->path, cmd, true) != 0) {
                                status = -1;
                        }
                }
        }

        closedir(dir);

        return status;
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        cmd_t cmd = CMD_SHOW;
        int first = 1;

        for (; (first < argc) && (argv[first][0] == '-'); first++) {
                if (strcmp(argv[first], "-e") == 0) {
                        cmd = CMD_ENABLE;
                } else if (strcmp(argv[first], "-d") == 0) {
                        cmd = CMD_DISABLE;
                } else {
                        print_usage(argv[0]);
                        return strcmp(argv[first], "-h") == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
                }
        }

        int status = 0;

        if (first == argc) {
                status = handle_all_devices(cmd);
        } else {
                for (int i = first; i < argc; i++) {
                        if (handle_device(argv[i], cmd, false) != 0) {
                                status = -1;
                        }
                }
        }

        return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*==============================================================================
  End of file
==============================================================================*/
