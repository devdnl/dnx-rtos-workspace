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
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
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
#define MOUNT_POINT   "/opt/testappset" /* mounted by siminit (appsetfs) */

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
static const char *test_start_by_path(void);
static const char *test_program_path(void);
static const char *test_main_appset_files(void);
static const char *test_files(void);
static const char *test_umount_busy(void);
static const char *test_confstr_path(void);
static const char *test_dot_name(void);

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
        {.name = "Program started by path (appsetfs)", .fn = test_start_by_path},
        {.name = "Program found in program search path", .fn = test_program_path},
        {.name = "Main appset mounted in /bin", .fn = test_main_appset_files},
        {.name = "Files of appsetfs (stat, read, dir)", .fn = test_files},
        {.name = "Unmount of used appset refused", .fn = test_umount_busy},
        {.name = "Program search path by confstr(_CS_PATH)", .fn = test_confstr_path},
        {.name = "Program name with '.' and '-' (dot.test-prog)", .fn = test_dot_name},
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
 * @brief  Start program and read the first line of its output.
 *
 * @param  cmd          command line
 * @param  status       exit status
 *
 * @return NULL on success (output in global->msg), otherwise error message.
 */
//==============================================================================
static const char *run_output(const char *cmd, int *status)
{
        FILE *f = fopen(OUT_FILE, "w");
        if (!f) {
                return "cannot create " OUT_FILE;
        }
        fclose(f);

        bool ok = run(cmd, OUT_FILE, status);

        global->msg[0] = '\0';
        f = fopen(OUT_FILE, "r");
        if (f) {
                if (!fgets(global->msg, sizeof(global->msg), f)) {
                        global->msg[0] = '\0';
                }
                fclose(f);
        }
        remove(OUT_FILE);

        global->msg[strcspn(global->msg, "\r\n")] = '\0';

        return ok ? NULL : "program not started";
}

//==============================================================================
/**
 * @brief  Start program and check the first line of its output.
 *
 * @param  cmd          command line
 * @param  expected     expected output
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *expect_output(const char *cmd, const char *expected)
{
        int status = -1;
        const char *err = run_output(cmd, &status);
        if (err) {
                return err;
        }

        if (strcmp(global->msg, expected) != 0) {
                char out[32];
                snprintf(out, sizeof(out), "%s", global->msg);
                snprintf(global->msg, sizeof(global->msg), "'%s' printed '%s'", cmd, out);
                return global->msg;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Program search order: echo exists in the main appset and in this
 *         appset (prints "testappset"), program started by name is found in
 *         the main appset first (then in the program search path).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_search_order(void)
{
        return expect_output("echo main appset", "main appset");
}

//==============================================================================
/**
 * @brief  Program of this appset started by path of the appsetfs.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_start_by_path(void)
{
        return expect_output(MOUNT_POINT "/echo", "testappset");
}

//==============================================================================
/**
 * @brief  Program of this appset started by name: not found in the main appset,
 *         found in the program search path (simulator configuration).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_program_path(void)
{
        int status = -1;
        if (!run("appsettest --exit 21", NULL, &status)) {
                return "program not found in the program search path";
        }

        return (status == 21) ? NULL : "wrong exit status";
}

//==============================================================================
/**
 * @brief  Main appset mounted by siminit in /bin ("-" source).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_main_appset_files(void)
{
        return expect_output("/bin/echo main", "main");
}

//==============================================================================
/**
 * @brief  Files of appsetfs: executable empty regular files, read-only file
 *         system, directory contains programs of this appset.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_files(void)
{
        struct stat st;
        if (stat(MOUNT_POINT "/echo", &st) != 0) {
                return "stat of program file failed";
        }

        if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IXUSR) || (st.st_size != 0)) {
                return "wrong status of program file";
        }

        FILE *f = fopen(MOUNT_POINT "/echo", "r");
        if (!f) {
                return "program file not opened";
        }

        char c;
        size_t rd = fread(&c, 1, 1, f);
        fclose(f);

        if (rd != 0) {
                return "program file is not empty";
        }

        errno = 0;
        f = fopen(MOUNT_POINT "/echo", "w");
        if (f) {
                fclose(f);
                return "program file opened for write";
        }

        if (errno != EROFS) {
                return "write open: errno is not EROFS";
        }

        DIR *dir = opendir(MOUNT_POINT);
        if (!dir) {
                return "directory not opened";
        }

        int found = 0;
        struct dirent *ent;
        while ((ent = readdir(dir))) {
                found += (strcmp(ent->d_name, "echo") == 0) || (strcmp(ent->d_name, "appsettest") == 0) ||
                         (strcmp(ent->d_name, "dot.test-prog") == 0);
        }
        closedir(dir);

        return (found == 3) ? NULL : "programs not found in directory";
}

//==============================================================================
/**
 * @brief  Unmount of the appset is refused while its program (this one) runs.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_umount_busy(void)
{
        errno = 0;
        if (umount(MOUNT_POINT) == 0) {
                return "appset unmounted while its program runs";
        }

        return (errno == EBUSY) ? NULL : "errno is not EBUSY";
}

//==============================================================================
/**
 * @brief  Program search path of the kernel (simulator configuration contains
 *         the mount point of this appset), size and truncation of confstr().
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_confstr_path(void)
{
        size_t size = confstr(_CS_PATH, NULL, 0);
        if (size <= 1) {
                return "program search path is empty";
        }

        if (confstr(_CS_PATH, global->msg, sizeof(global->msg)) != size) {
                return "wrong size of program search path";
        }

        if (strlen(global->msg) + 1 != size) {
                return "wrong length of program search path";
        }

        if (!strstr(global->msg, MOUNT_POINT)) {
                return "mount point not in program search path";
        }

        char small[4] = {'x', 'x', 'x', 'x'};
        if ((confstr(_CS_PATH, small, sizeof(small)) != size) || (small[3] != '\0') ||
            (strncmp(small, global->msg, 3) != 0)) {
                return "wrong truncation";
        }

        errno = 0;
        if ((confstr(12345, global->msg, sizeof(global->msg)) != 0) || (errno != EINVAL)) {
                return "unknown variable not rejected";
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Program name with '.' and '-': PROGRAM_PARAMS() identifier is
 *         dot_test_prog, the command is dot.test-prog (by path and by name in
 *         the program search path).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_dot_name(void)
{
        const char *err = expect_output(MOUNT_POINT "/dot.test-prog", "dot.test-prog");
        if (!err) {
                err = expect_output("dot.test-prog", "dot.test-prog");
        }

        return err;
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

        printf("Test of appset linked with the simulator (SIM_APPSETS, appsetfs)\n");

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
