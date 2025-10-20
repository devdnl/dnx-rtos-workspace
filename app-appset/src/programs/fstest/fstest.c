/*==============================================================================
@file   ftpd.c

@author Daniel Zorychta

@brief  File system test application.

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
#include <dnx/misc.h>
#include <dnx/thread.h>
#include <dnx/vt100.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        const char *name;
        const char *(*fn)(void);
} test_entry_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_test_result(const test_entry_t *test_entry);
static void print_byte_array(const char *title, const void *array, size_t array_size);
static const char *test_file_seek(void);
static const char *test_file_create_and_remove(void);
static const char *test_file_mode(void);
static const char *test_dir_mode(void);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t passed_test;
        char msg_buf[128];
        bool verbose;
};

static const char *SUCCESS_MSG = "SUCCESS";

static const test_entry_t TEST_TAB[] = {
        {.name = "File create and remove", .fn = test_file_create_and_remove},
        {.name = "Seek/Write/Read file operations", .fn = test_file_seek},
        {.name = "Test file mode change", .fn = test_file_mode},
        {.name = "Test dir mode change", .fn = test_dir_mode},
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(fstest, STACK_DEPTH_MEDIUM);

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
 * @param argc      argument count
 * @param argv      arguments
 *
 * @return On succes 0.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        UNUSED_ARG1(argc);

        global->verbose = isstreq(argv[1], "-v");

        process_stat_t proc_stat;
        process_stat(getpid(), &proc_stat);
        u16_t stack_max_usage = proc_stat.stack_max_usage;

        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                print_test_result(&TEST_TAB[i]);
        }

        process_stat(getpid(), &proc_stat);
        int fs_stack_usage = proc_stat.stack_max_usage - stack_max_usage;

        puts("");
        printf(VT100_FONT_COLOR_YELLOW "SUMMARY:" VT100_RESET_ATTRIBUTES " total tests: "
                                       "%lu, passed: "
                                       "%lu, failed: "
                                       "%lu\n",
                ARRAY_SIZE(TEST_TAB), global->passed_test, ARRAY_SIZE(TEST_TAB) - global->passed_test);

        printf("Stack used for file system: %i levels.\n", fs_stack_usage);

        return 0;
}

//==============================================================================
/**
 * @brief  Print test result.
 *
 * @param  test_entry   test entry
 */
//==============================================================================
static void print_test_result(const test_entry_t *test_entry)
{
        printf(VT100_FONT_COLOR_CYAN "TESTING:" VT100_RESET_ATTRIBUTES " %s... ", test_entry->name);
        fflush(stdout);

        const char *error_msg = test_entry->fn();

        if (isstreq(error_msg, SUCCESS_MSG)) {
                printf(VT100_FONT_COLOR_GREEN "[SUCCESS]" VT100_RESET_ATTRIBUTES "\n", SUCCESS_MSG);
                global->passed_test++;
        } else {
                printf(VT100_FONT_COLOR_RED "[FAILURE]" VT100_RESET_ATTRIBUTES "\n");
                printf(VT100_FONT_COLOR_RED "=> %s" VT100_RESET_ATTRIBUTES "\n", error_msg);
        }
}

//==============================================================================
/**
 * @brief  Print byte array.
 *
 * @param  title        title
 * @param  array        array to print
 * @param  array_size   array size
 */
//==============================================================================
static void print_byte_array(const char *title, const void *array, size_t array_size)
{
        const uint8_t *byte = array;
        size_t offset = 0;

        if (title) {
                printf("%s (%lu bytes long):\n", title, array_size);
        }

        while (array_size > 0) {
                size_t col_len = min(16, array_size);

                printf("%04X: ", offset);

                for (size_t col = 0; col < col_len; col++) {
                        printf("%02X ", byte[col]);
                }

                for (size_t col = 0; col < 16 - col_len; col++) {
                        printf("   ");
                }

                for (size_t col = 0; col < col_len; col++) {
                        char c = byte[col];

                        if (c == ' ') {
                                // as is
                        } else if (isspace(c)) {
                                c = ' ';
                        } else if (isprint(c)) {
                                // as is
                        } else {
                                c = '.';
                        }

                        printf("%c", c);
                }

                printf("\n");

                array_size -= col_len;
                offset += col_len;
                byte += col_len;
        }
}

//==============================================================================
/**
 * @brief  Seek/Write/Read file operations test.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_create_and_remove(void)
{
        const char *exit_msg = SUCCESS_MSG;

        errno = 0;
        FILE *fp = fopen("test.file", "w");
        if (not fp) {
                snprintf(global->msg_buf, sizeof(global->msg_buf), "unable to create test file (errno:%s)",
                        strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        errno = 0;
        if (fclose(fp) != 0) {
                snprintf(global->msg_buf, sizeof(global->msg_buf), "unable to close file (errno:%s)",
                        strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        errno = 0;
        if (remove("test.file") != 0) {
                snprintf(global->msg_buf, sizeof(global->msg_buf), "unable to remove file (errno:%s)",
                        strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        fp = fopen("test.file", "r");
        if (fp) {
                fclose(fp);
                snprintf(global->msg_buf, sizeof(global->msg_buf), "test file not removed!");
                exit_msg = global->msg_buf;
                goto cleanup;
        }

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Seek/Write/Read file operations test.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_seek(void)
{
        const char *exit_msg = SUCCESS_MSG;

        /*
         * Write test.
         */

        FILE *fp = fopen("test.file", "w");
        if (not fp) {
                exit_msg = "unable to open test file!";
                goto cleanup;
        }

        const size_t file_seek = 10;

        if (fseek(fp, file_seek, SEEK_SET) != 0) {
                exit_msg = "unable to seek file position!";
                goto cleanup;
        }

        const u8_t block[5] = {0x51, 0x52, 0x53, 0x54, 0x55};
        const size_t blocks = 10;

        for (size_t i = 0; i < blocks; i++) {
                errno = 0;
                size_t n = fwrite(block, 1, sizeof(block), fp);
                if (n != sizeof(block)) {
                        snprintf(global->msg_buf, sizeof(global->msg_buf),
                                "block write error: %lu/%lu (errno:%s)", n, sizeof(block), strerror(errno));
                        exit_msg = global->msg_buf;
                        goto cleanup;
                }
        }

        struct stat s;
        if (fstat(fileno(fp), &s) != 0) {
                exit_msg = "unable to get file statistics!";
                goto cleanup;
        }

        if (s.st_size != (file_seek + (blocks * sizeof(block)))) {
                exit_msg = "file size is not correct!";
                goto cleanup;
        }

        /*
         * Read test.
         */

        fp = freopen("test.file", "r", fp);
        if (not fp) {
                exit_msg = "unable to reopen file to read!";
                goto cleanup;
        }

        rewind(fp);

        u8_t buf[64];
        if (ARRAY_SIZE(buf) < file_seek) {
                exit_msg = "invalid test assumptions: buf < file_seek!";
                goto cleanup;
        }

        if (fread(buf, 1, file_seek, fp) != file_seek) {
                exit_msg = "unable to read test file!";
                goto cleanup;
        }

        for (size_t i = 0; i < file_seek; i++) {
                if (buf[i] != 0) {
                        exit_msg = "non-zero skip file position!";
                        goto cleanup;
                }
        }

        for (size_t i = 0; i < blocks; i++) {
                size_t n = fread(buf, 1, sizeof(block), fp);
                if (n != sizeof(block)) {
                        exit_msg = "read file block error!";
                        goto cleanup;
                } else {
                        if (memcmp(buf, block, n) != 0) {
                                exit_msg = "invalid block data!";
                                goto cleanup;
                        }
                }
        }

cleanup:
        fclose(fp);
        remove("test.file");
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Seek/Write/Read file operations test.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_mode(void)
{
        const char *exit_msg = SUCCESS_MSG;

        /*
         * File test.
         */
        const char *file_path = "test.file";

        struct stat stat_fp = {0};
        struct stat stat_path = {0};

        FILE *fp = fopen(file_path, "w");
        if (fp) {
                if (fstat(fileno(fp), &stat_fp) != 0) {
                        exit_msg = "unable to perform fstat!";
                        goto cleanup;
                }

                if (stat(file_path, &stat_path) != 0) {
                        exit_msg = "unable to perform stat!";
                        goto cleanup;
                }

                if (memcmp(&stat_path, &stat_fp, sizeof(stat_path)) != 0) {
                        if (global->verbose) {
                                puts("");
                                printf("st_size  : %llu vs %llu\n", stat_fp.st_size, stat_path.st_size);
                                printf("st_dev   : %li vs %li\n", stat_fp.st_dev, stat_path.st_dev);
                                printf("st_mode  : %lu vs %lu\n", stat_fp.st_mode, stat_path.st_mode);
                                printf("st_uid   : %u vs %u\n", stat_fp.st_uid, stat_path.st_uid);
                                printf("st_gid   : %u vs %u\n", stat_fp.st_gid, stat_path.st_gid);
                                printf("st_ctime : %lli vs %lli\n", stat_fp.st_ctime, stat_path.st_ctime);
                                printf("st_mtime : %lli vs %lli\n", stat_fp.st_mtime, stat_path.st_mtime);

                                print_byte_array("fstat()", &stat_fp, sizeof(stat_fp));
                                print_byte_array("stat()", &stat_path, sizeof(stat_path));
                        }

                        exit_msg = "fstat() and stat() give different results!";

                        goto cleanup;
                }
        }

        /*
         *
         */
        if (chmod(file_path, 0666 | S_IFDIR) != 0) {
                exit_msg = "unable to chmod!";
                goto cleanup;
        }

        if (stat(file_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        if (stat_path.st_mode != 0666) {
                if (global->verbose) {
                        printf("\nMode: %lo vs %lo\n", 0666, stat_path.st_mode);
                }
                exit_msg = "file mode changed incorrectly (1)!";
                goto cleanup;
        }

        /*
         *
         */
        if (chmod(file_path, 0421) != 0) {
                exit_msg = "unable to chmod!";
                goto cleanup;
        }

        if (stat(file_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        if (stat_path.st_mode != 0421) {
                if (global->verbose) {
                        printf("\nMode: %lo vs %lo\n", 0421, stat_path.st_mode);
                }
                exit_msg = "file mode changed incorrectly (2)!";
                goto cleanup;
        }

cleanup:
        fclose(fp);
        remove(file_path);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Seek/Write/Read file operations test.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_mode(void)
{
        const char *exit_msg = SUCCESS_MSG;

        /*
         * File test.
         */
        const char *dir_path = "test.dir";

        struct stat stat_path;

        if (mkdir(dir_path, 0666) != 0) {
                exit_msg = "unable to create test directory!";
                goto cleanup;
        }

        if (stat(dir_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        /*
         *
         */
        if (chmod(dir_path, 0666) != 0) {
                exit_msg = "unable to chmod!";
                goto cleanup;
        }

        if (stat(dir_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        if ((0666 | S_IFDIR) != stat_path.st_mode) {
                if (global->verbose) {
                        printf("\nMode: %lo vs %lo\n", 0666 | S_IFDIR, stat_path.st_mode);
                }
                exit_msg = "dir mode changed incorrectly (1)!";
                goto cleanup;
        }

        /*
         *
         */
        if (chmod(dir_path, 0421) != 0) {
                exit_msg = "unable to chmod!";
                goto cleanup;
        }

        if (stat(dir_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        if (not S_ISDIR(stat_path.st_mode)) {
                if (global->verbose) {
                        printf("\nMode: %lo vs %lo\n", 0421 | S_IFDIR, stat_path.st_mode);
                }
                exit_msg = "dir mode changed incorrectly (2)!";
                goto cleanup;
        }

        if (chmod(dir_path, 0) != 0) {
                exit_msg = "unable to chmod!";
                goto cleanup;
        }

        if (stat(dir_path, &stat_path) != 0) {
                exit_msg = "unable to perform stat!";
                goto cleanup;
        }

        if ((stat_path.st_mode & (S_IFMT | S_IPMT)) != S_IFDIR) {
                if (global->verbose) {
                        printf("\nMode: %lo vs %lo\n", S_IFDIR, stat_path.st_mode);
                }
                exit_msg = "dir mode changed incorrectly (3)!";
                goto cleanup;
        }

cleanup:
        remove(dir_path);
        return exit_msg;
}

/*==============================================================================
  End of file
==============================================================================*/
