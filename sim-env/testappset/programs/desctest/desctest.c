/*==============================================================================
@file    desctest.c

@author  Daniel Zorychta

@brief   Tests of descriptors: object type check, descriptor of other process,
         shared descriptor of closed object.

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
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/thread.h>

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        unsigned executed;
        unsigned failed;
        char cmd[64];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(desctest, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Check result of test.
 *
 * @param  ok           test result
 * @param  name         test name
 */
//==============================================================================
static void check(bool ok, const char *name)
{
        global->executed++;
        if (!ok) {
                global->failed++;
        }
        printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
}

//==============================================================================
/**
 * @brief  Check that call failed with EBADF.
 *
 * @param  result       result of call
 *
 * @return true if call failed with EBADF.
 */
//==============================================================================
static bool ebadf(int result)
{
        return (result == -1) && (errno == EBADF);
}

//==============================================================================
/**
 * @brief  Child process: uses descriptors of parent.
 *
 * @param  mtx          mutex descriptor of parent (not shared)
 * @param  shared       shared mutex descriptor of parent
 *
 * @return 0 if all checks passed, otherwise number of failed checks.
 */
//==============================================================================
static int child(int mtx, int shared)
{
        /* own mutex in the same slot of descriptor table as mutex of parent */
        int own = mut_open(MUTEX_TYPE_NORMAL, 0);
        int fails = 0;

        fails += !ebadf(mut_lock(mtx, 0)); // descriptor of other process
        fails += !ebadf(close(mtx));
        fails += !((mut_lock(shared, 0) == 0) && (mut_unlock(shared) == 0)); // shared descriptor
        fails += !((mut_lock(own, 0) == 0) && (mut_unlock(own) == 0));

        close(own);

        return fails;
}

//==============================================================================
/**
 * @brief  Program main function.
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return 0 on success.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if ((argc == 4) && (strcmp(argv[1], "-c") == 0)) {
                return child(atoi(argv[2]), atoi(argv[3]));
        }

        int mtx = mut_open(MUTEX_TYPE_NORMAL, 0);
        int sem = sem_open(1, 0, 0);
        check((mtx >= 0) && (sem >= 0), "create mutex and semaphore");

        /* object type */
        check(ebadf(sem_wait(mtx, 0)), "semaphore function with mutex descriptor: EBADF");
        check(ebadf(mut_lock(sem, 0)), "mutex function with semaphore descriptor: EBADF");
        check((mut_lock(mtx, 0) == 0) && (mut_unlock(mtx) == 0), "mutex function with mutex descriptor");
        check(ebadf(mut_lock(-1, 0)) && ebadf(mut_lock(0x7FFF0000, 0)), "incorrect descriptor: EBADF");

        /* descriptor of other process, shared descriptor */
        int shared = mut_open(MUTEX_TYPE_NORMAL, O_SHARED);
        check(shared >= 0, "create shared mutex");

        snprintf(global->cmd, sizeof(global->cmd), "desctest -c %d %d", mtx, shared);
        process_attr_t attr = {.cwd = "/", .detached = false, .priority = PRIORITY_NORMAL};
        pid_t pid = process_create(global->cmd, &attr);
        int status = -1;
        if (pid) {
                process_wait(pid, &status, 10000);
        }
        check(status == 0, "child: descriptor of parent EBADF, shared descriptor works");

        /* shared descriptor of closed object (slot used by new object) */
        check(close(shared) == 0, "close shared mutex");
        int shared2 = mut_open(MUTEX_TYPE_NORMAL, O_SHARED);
        check((shared2 >= 0) && ((shared2 & 0xFFFF) == (shared & 0xFFFF)),
                "new shared mutex in the same slot");
        check(ebadf(mut_lock(shared, 0)), "descriptor of closed shared mutex: EBADF");
        check((mut_lock(shared2, 0) == 0) && (mut_unlock(shared2) == 0), "new shared mutex works");
        check(ebadf(close(shared)), "close of closed descriptor: EBADF");

        /* stdio descriptors without tag */
        check(write(STDOUT_FILENO, "\n", 1) == 1, "STDOUT_FILENO works");

        close(shared2);
        close(sem);
        close(mtx);

        printf("SUMMARY: executed: %u, passed: %u, failed: %u, skipped: 0\n", global->executed,
                global->executed - global->failed, global->failed);

        return global->failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
