/*==============================================================================
@file    ftpd.c

@author  Daniel Zorychta

@brief   File system test application.

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#include <dnx/misc.h>
#include <dnx/vt100.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        const char *test_name;
        const char *(*test_fn)(void);
} test_entry_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void        print_test_result(const test_entry_t *test_entry);
static const char *test_file_seek(void);
static const char *test_file_create_and_remove(void);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t passed_test;
        char   msg_buf[128];
};

static const char *SUCCESS_MSG = "SUCCESS";

static const test_entry_t TEST_TAB[] = {
        {.test_name = "File create and remove",          .test_fn = test_file_create_and_remove},
        {.test_name = "Seek/Write/Read file operations", .test_fn = test_file_seek             },
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
        UNUSED_ARG2(argc, argv);

        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                print_test_result(&TEST_TAB[i]);
        }

        puts("");
        printf(VT100_FONT_COLOR_YELLOW "SUMMARY:" VT100_RESET_ATTRIBUTES " total tests: "
                                       "%lu, passed: "
                                       "%lu, failed: "
                                       "%lu\n",
                ARRAY_SIZE(TEST_TAB), global->passed_test,
                ARRAY_SIZE(TEST_TAB) - global->passed_test);

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
        printf(VT100_FONT_COLOR_CYAN "TESTING:" VT100_RESET_ATTRIBUTES " %s... ",
                test_entry->test_name);
        fflush(stdout);

        const char *error_msg = test_entry->test_fn();

        if (isstreq(error_msg, SUCCESS_MSG)) {
                printf(VT100_FONT_COLOR_GREEN "[SUCCESS]" VT100_RESET_ATTRIBUTES "\n",
                        SUCCESS_MSG);
                global->passed_test++;
        } else {
                printf(VT100_FONT_COLOR_RED "[FAILURE]" VT100_RESET_ATTRIBUTES "\n");
                printf(VT100_FONT_COLOR_RED "=> %s" VT100_RESET_ATTRIBUTES "\n",
                        error_msg);
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

        errno    = 0;
        FILE *fp = fopen("test.file", "w");
        if (not fp) {
                snprintf(global->msg_buf, sizeof(global->msg_buf),
                        "unable to create test file (errno:%s)", strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        errno = 0;
        if (fclose(fp) != 0) {
                snprintf(global->msg_buf, sizeof(global->msg_buf),
                        "unable to close file (errno:%s)", strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        errno = 0;
        if (remove("test.file") != 0) {
                snprintf(global->msg_buf, sizeof(global->msg_buf),
                        "unable to remove file (errno:%s)", strerror(errno));
                exit_msg = global->msg_buf;
                goto cleanup;
        }

        fp = fopen("test.file", "r");
        if (fp) {
                fclose(fp);
                snprintf(global->msg_buf, sizeof(global->msg_buf),
                        "test file not removed!");
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

        const u8_t   block[5] = {0x51, 0x52, 0x53, 0x54, 0x55};
        const size_t blocks   = 10;

        for (size_t i = 0; i < blocks; i++) {
                errno    = 0;
                size_t n = fwrite(block, 1, sizeof(block), fp);
                if (n != sizeof(block)) {
                        snprintf(global->msg_buf, sizeof(global->msg_buf),
                                "block write error: %lu/%lu (errno:%s)", n, sizeof(block),
                                strerror(errno));
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

/*==============================================================================
  End of file
==============================================================================*/
