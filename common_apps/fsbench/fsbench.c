/*==============================================================================
@file   fsbench.c

@author Daniel Zorychta

@brief  File system performance measurement of selected directory.

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

/*
 * Measurements (files are created in selected directory and removed at end):
 *      seq     sequential write and read of file (-s) by chunks (-c),
 *      append  small writes (-a) at end of open file (e.g. log),
 *      random  overwrite and read of small parts (-a) at random positions,
 *      meta    create, stat, open, readdir and remove of files (-n).
 *
 * Write time includes closing of file and sync(). If file system limits file
 * size (e.g. flatfs), size of file is reduced to the limit. With option -D statistics
 * of block device are reset before each measurement and number of requests
 * and transferred KiB are printed (R: read, W: write), and also estimated
 * read hits of block cache (LRU model of device statistics, see iostat).
 * Statistics are disabled at end of program.
 */

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <dnx/misc.h>
#include <dnx/os.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DATA_FILE      "fsbench.dat"
#define META_DIR       "fsbench.d"
#define MAX_CHUNKS     4
#define RANDOM_OPS     256
#define MAX_APPEND_OPS 1024
#define PATH_LEN       128

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
        const char *dir;
        const char *dev;
        const char *filter;
        u32_t size;
        u32_t chunk[MAX_CHUNKS];
        u32_t chunks;
        u32_t small;
        u32_t files;
        u8_t *buf;
        u32_t buflen;
        u32_t seed;
        clock_t tref;
        int dev_fd;
        STORAGE_stats_t stats;
        char path[PATH_LEN];
        char path2[PATH_LEN];
        char label[32];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(fsbench, STACK_DEPTH_LOW);

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
        printf("Usage: %s [options] <dir>\n", name);
        puts("  -s <KiB>     file size of seq and random tests (default 256)");
        puts("  -c <B>       chunk size of seq test, up to 4 times (default 512, 4096, 32768)");
        puts("  -a <B>       write size of append and random tests (default 64)");
        puts("  -n <files>   number of files of meta test (default 32)");
        puts("  -t <name>    run only tests that contain name: seq, append, random, meta");
        puts("  -D <device>  print statistics of block device (e.g. /dev/sda1)");
}

//==============================================================================
/**
 * @brief  Function returns random number (LCG, repeatable sequence).
 *
 * @return Random number.
 */
//==============================================================================
static u32_t rnd(void)
{
        global->seed = (global->seed * 1103515245) + 12345;
        return global->seed >> 8;
}

//==============================================================================
/**
 * @brief  Function prints error message.
 *
 * @param  what         operation
 * @param  path         path (can be NULL)
 *
 * @return Always false.
 */
//==============================================================================
static bool fail(const char *what, const char *path)
{
        printf("\n%s%s%s: %s\n", what, path ? " " : "", path ? path : "", strerror(errno));
        return false;
}

//==============================================================================
/**
 * @brief  Function starts measurement: resets device statistics and time.
 *
 * @param  name         measurement name
 * @param  param        parameter of measurement (0: none)
 * @param  unit         unit of parameter
 */
//==============================================================================
static void begin(const char *name, u32_t param, const char *unit)
{
        if (param) {
                snprintf(global->label, sizeof(global->label), "%s %lu %s", name, (unsigned long)param, unit);
        } else {
                snprintf(global->label, sizeof(global->label), "%s", name);
        }

        if (global->dev_fd >= 0) {
                ioctl(global->dev_fd, IOCTL_STORAGE__STATS_ENABLE);
        }

        global->tref = get_uptime_ms();
}

//==============================================================================
/**
 * @brief  Function finishes measurement and prints results.
 *
 * @param  bytes        transferred bytes (0: operations are reported)
 * @param  ops          number of operations
 */
//==============================================================================
static void end(u64_t bytes, u32_t ops)
{
        u32_t dt = (u32_t)(get_uptime_ms() - global->tref);

        char speed[24];
        if (dt == 0) {
                snprintf(speed, sizeof(speed), "-");
        } else if (bytes) {
                snprintf(speed, sizeof(speed), "%lu KiB/s", (unsigned long)((bytes * 1000) / dt / 1024));
        } else {
                snprintf(speed, sizeof(speed), "%lu op/s", (unsigned long)(((u64_t)ops * 1000) / dt));
        }

        char amount[16];
        if (bytes) {
                snprintf(amount, sizeof(amount), "%lu KiB", (unsigned long)(bytes / 1024));
        } else {
                snprintf(amount, sizeof(amount), "%lu op", (unsigned long)ops);
        }

        printf("%-24s%10s %8lu ms %12s", global->label, amount, (unsigned long)dt, speed);

        STORAGE_stats_t *s = &global->stats;
        if ((global->dev_fd >= 0) && (ioctl(global->dev_fd, IOCTL_STORAGE__GET_STATS, s) == 0)) {
                printf("  R %5lu/%-6lu W %5lu/%-6lu", (unsigned long)s->rd_req, (unsigned long)(s->rd_bytes / 1024),
                        (unsigned long)s->wr_req, (unsigned long)(s->wr_bytes / 1024));

                if (s->rd_lru_blocks) {
                        printf(" LRU %lu%%", (unsigned long)(((u64_t)s->rd_lru_hits * 100) / s->rd_lru_blocks));
                }
        }

        putchar('\n');
}

//==============================================================================
/**
 * @brief  Function creates path of file in tested directory.
 *
 * @param  buf          path buffer (PATH_LEN)
 * @param  name         file name
 *
 * @return Path.
 */
//==============================================================================
static char *make_path(char *buf, const char *name)
{
        snprintf(buf, PATH_LEN, "%s/%s", global->dir, name);
        return buf;
}

//==============================================================================
/**
 * @brief  Function writes whole buffer to file.
 *
 * @param  fd           file descriptor
 * @param  buf          buffer
 * @param  len          buffer length
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool write_all(int fd, const u8_t *buf, size_t len)
{
        ssize_t n = write(fd, buf, len);
        if ((n >= 0) && ((size_t)n != len)) {
                errno = ENOSPC;
        }

        return (n >= 0) && ((size_t)n == len);
}

//==============================================================================
/**
 * @brief  Function reads whole buffer from file.
 *
 * @param  fd           file descriptor
 * @param  buf          buffer
 * @param  len          buffer length
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool read_all(int fd, u8_t *buf, size_t len)
{
        ssize_t n = read(fd, buf, len);
        if ((n >= 0) && ((size_t)n != len)) {
                errno = EIO;
        }

        return (n >= 0) && ((size_t)n == len);
}

//==============================================================================
/**
 * @brief  Function writes and reads file sequentially by selected chunk.
 *
 * @param  chunk        chunk size
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool test_seq(u32_t chunk)
{
        char *path = make_path(global->path, DATA_FILE);
        u32_t size = global->size;

        begin("seq write", chunk, "B");

        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
                return fail("open", path);
        }

        for (u32_t pos = 0; pos < size; pos += chunk) {
                u32_t len = min(chunk, size - pos);
                memset(global->buf, pos / chunk, len);

                ssize_t n = write(fd, global->buf, len);
                if ((n < 0) && (errno != EFBIG)) {
                        close(fd);
                        return fail("write", path);
                }

                if ((u32_t)max(n, 0) < len) {
                        /* file size limit of file system */
                        size = pos + max(n, 0);
                        if (size == 0) {
                                close(fd);
                                return fail("write", path);
                        }
                        break;
                }
        }

        close(fd);
        sync();
        end(size, 0);

        if (size != global->size) {
                printf("File size limited to %lu B\n", (unsigned long)size);
                global->size = size;
        }

        begin("seq read", chunk, "B");

        fd = open(path, O_RDONLY);
        if (fd < 0) {
                return fail("open", path);
        }

        for (u32_t pos = 0; pos < size; pos += chunk) {
                if (!read_all(fd, global->buf, min(chunk, size - pos))) {
                        close(fd);
                        return fail("read", path);
                }
        }

        close(fd);
        end(size, 0);

        return true;
}

//==============================================================================
/**
 * @brief  Function appends small parts of data to open file.
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool test_append(void)
{
        char *path = make_path(global->path, DATA_FILE);
        u32_t ops = min(global->size / global->small, MAX_APPEND_OPS);

        memset(global->buf, 'a', global->small);

        begin("append", global->small, "B");

        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
        if (fd < 0) {
                return fail("open", path);
        }

        for (u32_t i = 0; i < ops; i++) {
                if (!write_all(fd, global->buf, global->small)) {
                        close(fd);
                        return fail("write", path);
                }
        }

        close(fd);
        sync();
        end(0, ops);

        return true;
}

//==============================================================================
/**
 * @brief  Function overwrites and reads small parts of file at random positions.
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool test_random(void)
{
        char *path = make_path(global->path, DATA_FILE);
        u32_t size = global->size;
        u32_t small = global->small;

        /* prepare file */
        int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
                return fail("open", path);
        }

        memset(global->buf, 'r', global->buflen);
        for (u32_t pos = 0; pos < size; pos += global->buflen) {
                if (!write_all(fd, global->buf, min(global->buflen, size - pos))) {
                        close(fd);
                        return fail("write", path);
                }
        }

        close(fd);
        sync();

        if (size < small) {
                return true;
        }

        for (int pass = 0; pass < 2; pass++) {
                bool wr = (pass == 0);

                begin(wr ? "random write" : "random read", small, "B");

                fd = open(path, wr ? O_RDWR : O_RDONLY);
                if (fd < 0) {
                        return fail("open", path);
                }

                global->seed = 1;

                for (u32_t i = 0; i < RANDOM_OPS; i++) {
                        off_t pos = rnd() % (size - small + 1);

                        bool ok = (lseek(fd, pos, SEEK_SET) == pos);
                        if (ok) {
                                ok = wr ? write_all(fd, global->buf, small) : read_all(fd, global->buf, small);
                        }

                        if (!ok) {
                                close(fd);
                                return fail(wr ? "write" : "read", path);
                        }
                }

                close(fd);
                if (wr) {
                        sync();
                }

                end(0, RANDOM_OPS);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Function creates path of file of meta test.
 *
 * @param  n            file number
 *
 * @return Path.
 */
//==============================================================================
static char *meta_path(u32_t n)
{
        snprintf(global->path2, PATH_LEN, "%s/" META_DIR "/f%04lu", global->dir, (unsigned long)n);
        return global->path2;
}

//==============================================================================
/**
 * @brief  Function measures metadata operations.
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool test_meta(void)
{
        char *dir = make_path(global->path, META_DIR);
        u32_t n = global->files;

        if ((mkdir(dir, 0777) != 0) && (errno != EEXIST)) {
                return fail("mkdir", dir);
        }

        memset(global->buf, 'm', global->small);

        begin("meta create", n, "files");
        for (u32_t i = 0; i < n; i++) {
                int fd = open(meta_path(i), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd < 0) {
                        return fail("open", global->path2);
                }

                bool ok = write_all(fd, global->buf, global->small);
                close(fd);

                if (!ok) {
                        return fail("write", global->path2);
                }
        }
        sync();
        end(0, n);

        begin("meta stat", n, "files");
        for (u32_t i = 0; i < n; i++) {
                struct stat st;
                if (stat(meta_path(i), &st) != 0) {
                        return fail("stat", global->path2);
                }
        }
        end(0, n);

        begin("meta open/read", n, "files");
        for (u32_t i = 0; i < n; i++) {
                int fd = open(meta_path(i), O_RDONLY);
                if (fd < 0) {
                        return fail("open", global->path2);
                }

                bool ok = read_all(fd, global->buf, global->small);
                close(fd);

                if (!ok) {
                        return fail("read", global->path2);
                }
        }
        end(0, n);

        begin("meta readdir", n, "files");
        DIR *d = opendir(dir);
        if (!d) {
                return fail("opendir", dir);
        }

        u32_t items = 0;
        struct dirent *ent;
        while ((ent = readdir(d))) {
                if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0)) {
                        items++;
                }
        }
        closedir(d);
        end(0, items);

        if (items != n) {
                printf("readdir: %lu items, expected %lu\n", (unsigned long)items, (unsigned long)n);
                return false;
        }

        begin("meta remove", n, "files");
        for (u32_t i = 0; i < n; i++) {
                if (remove(meta_path(i)) != 0) {
                        return fail("remove", global->path2);
                }
        }
        sync();
        end(0, n);

        return true;
}

//==============================================================================
/**
 * @brief  Function removes files created by tests.
 */
//==============================================================================
static void cleanup(void)
{
        for (u32_t i = 0; i < global->files; i++) {
                remove(meta_path(i));
        }

        remove(make_path(global->path, META_DIR));
        remove(make_path(global->path, DATA_FILE));
        sync();
}

//==============================================================================
/**
 * @brief  Function checks if test is selected by filter.
 *
 * @param  name         test name
 *
 * @return True if test is selected.
 */
//==============================================================================
static bool selected(const char *name)
{
        return !global->filter || strstr(name, global->filter);
}

//==============================================================================
/**
 * @brief  Function parses numeric argument.
 *
 * @param  arg          argument (can be NULL)
 * @param  min          minimal value
 * @param  val          parsed value
 *
 * @return On success true is returned.
 */
//==============================================================================
static bool parse_num(const char *arg, u32_t min, u32_t *val)
{
        char *end = NULL;
        unsigned long v = arg ? strtoul(arg, &end, 0) : 0;

        if (!arg || (*end != '\0') || (v < min) || (v > 0x7FFFFFFF)) {
                printf("Invalid value: %s\n", arg ? arg : "(none)");
                return false;
        }

        *val = v;
        return true;
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->size = 256;
        global->small = 64;
        global->files = 32;
        global->dev_fd = -1;

        int i = 1;
        for (; (i < argc) && (argv[i][0] == '-'); i++) {
                const char *opt = argv[i];
                const char *arg = (i + 1 < argc) ? argv[i + 1] : NULL;
                bool ok = true;

                if (strcmp(opt, "-s") == 0) {
                        ok = parse_num(arg, 1, &global->size);
                } else if (strcmp(opt, "-c") == 0) {
                        if (global->chunks < MAX_CHUNKS) {
                                ok = parse_num(arg, 1, &global->chunk[global->chunks++]);
                        }
                } else if (strcmp(opt, "-a") == 0) {
                        ok = parse_num(arg, 1, &global->small);
                } else if (strcmp(opt, "-n") == 0) {
                        ok = parse_num(arg, 1, &global->files);
                } else if (strcmp(opt, "-t") == 0) {
                        global->filter = arg;
                        ok = (arg != NULL);
                } else if (strcmp(opt, "-D") == 0) {
                        global->dev = arg;
                        ok = (arg != NULL);
                } else {
                        print_usage(argv[0]);
                        return strcmp(opt, "-h") == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
                }

                if (!ok) {
                        return EXIT_FAILURE;
                }

                i++;
        }

        if (i != argc - 1) {
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }

        global->dir = argv[i];
        global->size *= 1024;

        if (global->chunks == 0) {
                global->chunk[0] = 512;
                global->chunk[1] = 4096;
                global->chunk[2] = 32768;
                global->chunks = 3;
        }

        global->buflen = global->small;
        for (u32_t c = 0; c < global->chunks; c++) {
                global->buflen = max(global->buflen, global->chunk[c]);
        }

        global->buf = malloc(global->buflen);
        if (!global->buf) {
                fail("malloc", NULL);
                return EXIT_FAILURE;
        }

        if (global->dev) {
                global->dev_fd = open(global->dev, O_RDONLY);
                if ((global->dev_fd < 0) || (ioctl(global->dev_fd, IOCTL_STORAGE__STATS_ENABLE) != 0)) {
                        fail("statistics", global->dev);
                        free(global->buf);
                        return EXIT_FAILURE;
                }
        }

        printf("Directory %s, file %lu KiB\n", global->dir, (unsigned long)(global->size / 1024));
        printf("%-24s%10s %11s %12s%s\n", "test", "amount", "time", "speed",
                global->dev ? "  device R/W: requests/KiB, LRU read hits" : "");

        bool ok = true;

        for (u32_t c = 0; ok && (c < global->chunks) && selected("seq"); c++) {
                ok = test_seq(global->chunk[c]);
        }

        if (ok && selected("append")) {
                ok = test_append();
        }

        if (ok && selected("random")) {
                ok = test_random();
        }

        if (ok && selected("meta")) {
                ok = test_meta();
        }

        cleanup();

        if (global->dev_fd >= 0) {
                ioctl(global->dev_fd, IOCTL_STORAGE__STATS_DISABLE);
                close(global->dev_fd);
        }

        free(global->buf);

        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*==============================================================================
  End of file
==============================================================================*/
