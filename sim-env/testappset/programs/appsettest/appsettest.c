/*==============================================================================
@file    appsettest.c

@author  Daniel Zorychta

@brief   Test of other appset linked with the POSIX simulator (SIM_APPSETS)

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
#include <unistd.h>
#include <dnx/os.h>
#include <dnx/thread.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define TIMEOUT       5000                  /* process and thread timeout [ms] */
#define OUT_FILE      "/tmp/appsettest.out" /* output of started programs */
#define CMD_LEN       64
#define HEAP_SIZE     4096
#define THREAD_STATUS 7
#define CHILD_STATUS  33

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef const char *(*test_fn_t)(void);

typedef struct {
        const char *name;
        test_fn_t fn;
} test_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static const char *test_globals(void);
static const char *test_heap(void);
static const char *test_libc(void);
static const char *test_thread(void);
static const char *test_child(void);
static const char *test_search_order(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        const char *progname;
        int counter;
        char msg[96];
};

static const test_t TESTS[] = {
        {.name = "Global variables of program", .fn = test_globals},
        {.name = "Heap", .fn = test_heap},
        {.name = "libc of appset (formatting)", .fn = test_libc},
        {.name = "Thread create and join", .fn = test_thread},
        {.name = "Child process of the same appset", .fn = test_child},
        {.name = "Program search order (main appset first)", .fn = test_search_order},
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(appsettest, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Global variables: values are kept between functions.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_globals(void)
{
        global->counter = 0;
        for (int i = 0; i < 10; i++) {
                global->counter += i;
        }

        return (global->counter == 45) ? NULL : "wrong value of global variable";
}

//==============================================================================
/**
 * @brief  Heap: allocation, write and read of block.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_heap(void)
{
        unsigned char *buf = malloc(HEAP_SIZE);
        if (!buf) {
                return "malloc failed";
        }

        for (int i = 0; i < HEAP_SIZE; i++) {
                buf[i] = (unsigned char)(i * 7);
        }

        bool ok = true;
        for (int i = 0; i < HEAP_SIZE; i++) {
                ok = ok && (buf[i] == (unsigned char)(i * 7));
        }

        free(buf);

        return ok ? NULL : "wrong content of allocated block";
}

//==============================================================================
/**
 * @brief  libc of the appset: formatting functions.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_libc(void)
{
        char buf[32];
        snprintf(buf, sizeof(buf), "%d|%5s|%08X|%c", -42, "dnx", 0xBEEFu, 'x');

        if (strcmp(buf, "-42|  dnx|0000BEEF|x") != 0) {
                snprintf(global->msg, sizeof(global->msg), "snprintf: '%s'", buf);
                return global->msg;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Thread function: modifies global variable of program.
 *
 * @param  arg          argument (value added to counter)
 *
 * @return Thread status.
 */
//==============================================================================
static int thread_func(void *arg)
{
        global->counter += *(int *)arg;
        return THREAD_STATUS;
}

//==============================================================================
/**
 * @brief  Thread of the program: global variables are shared, status is
 *         returned by join.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_thread(void)
{
        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        int add = 100;
        global->counter = 1;

        tid_t tid = thread_create(thread_func, &attr, &add);
        if (tid == 0) {
                return "thread not created";
        }

        int status = -1;
        if (thread_join2(tid, &status, TIMEOUT) != 0) {
                return "thread join failed";
        }

        if (status != THREAD_STATUS) {
                return "wrong thread status";
        }

        return (global->counter == 101) ? NULL : "global variable not changed by thread";
}

//==============================================================================
/**
 * @brief  Start program and wait for its exit.
 *
 * @param  cmd          command line
 * @param  out          stdout file path (NULL: stdout of this program)
 * @param  status       exit status
 *
 * @return true if program was started and finished.
 */
//==============================================================================
static bool run(const char *cmd, const char *out, int *status)
{
        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.fd_stdin = STDIN_FILENO;
        attr.fd_stdout = out ? -1 : STDOUT_FILENO;
        attr.p_stdout = out;
        attr.fd_stderr = STDERR_FILENO;
        attr.priority = PRIORITY_NORMAL;
        attr.detached = false;

        pid_t pid = process_create(cmd, &attr);
        if (pid == 0) {
                return false;
        }

        return process_wait(pid, status, TIMEOUT) == 0;
}

//==============================================================================
/**
 * @brief  Child process: program of this appset is found by the kernel.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_child(void)
{
        char cmd[CMD_LEN];
        snprintf(cmd, sizeof(cmd), "%s --exit %d", global->progname, CHILD_STATUS);

        int status = -1;
        if (!run(cmd, NULL, &status)) {
                return "child process not started";
        }

        return (status == CHILD_STATUS) ? NULL : "wrong exit status of child";
}

//==============================================================================
/**
 * @brief  Program search order: echo exists in the main appset and in this
 *         appset (prints "testappset"), the program of the main appset must
 *         be started.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_search_order(void)
{
        FILE *f = fopen(OUT_FILE, "w");
        if (!f) {
                return "cannot create " OUT_FILE;
        }
        fclose(f);

        int status = -1;
        bool ok = run("echo main appset", OUT_FILE, &status);

        char buf[32] = {0};
        f = fopen(OUT_FILE, "r");
        if (f) {
                if (!fgets(buf, sizeof(buf), f)) {
                        buf[0] = '\0';
                }
                fclose(f);
        }
        remove(OUT_FILE);

        if (!ok) {
                return "echo not started";
        }

        buf[strcspn(buf, "\r\n")] = '\0';
        if (strcmp(buf, "main appset") != 0) {
                snprintf(global->msg, sizeof(global->msg), "echo printed '%s'", buf);
                return global->msg;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Main function. Without arguments runs the tests,
 *         "--exit <status>" finishes with selected status (child mode).
 *
 * @param  argc         argument count
 * @param  argv         arguments
 *
 * @return Exit status.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->progname = argv[0];

        if ((argc == 3) && (strcmp(argv[1], "--exit") == 0)) {
                return atoi(argv[2]);
        }

        printf("Test of appset linked with the simulator (SIM_APPSETS)\n");

        unsigned executed = 0, passed = 0, failed = 0;

        for (size_t i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); i++) {
                const char *err = TESTS[i].fn();
                executed++;

                if (err) {
                        failed++;
                        printf("  %-44s FAIL: %s\n", TESTS[i].name, err);
                } else {
                        passed++;
                        printf("  %-44s PASS\n", TESTS[i].name);
                }
        }

        printf("SUMMARY: executed: %u, passed: %u, failed: %u, skipped: 0\n", executed, passed, failed);

        return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
