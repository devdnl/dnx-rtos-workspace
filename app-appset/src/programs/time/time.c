/*==============================================================================
@file   time.c

@author Daniel Zorychta

@brief  Check execution time of selected program

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
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <sys/types.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define CWD_LEN         80
#define CMD_LEN         100

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
        char cwd[CWD_LEN];
        char cmd[CMD_LEN];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(time, STACK_DEPTH_VERY_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc == 1) {
                printf("Usage: %s [program]\n", argv[0]);
                return EXIT_SUCCESS;
        }

        getcwd(global->cwd, CWD_LEN);

        for (int i = 1; i < argc; i++) {
                if (strlen(argv[i]) + strlen(global->cmd) < CMD_LEN) {
                        strcat(global->cmd, argv[i]);
                        strcat(global->cmd, " ");
                } else {
                        break;
                }
        }

        errno = 0;

        u64_t start_time = get_uptime_ms();

        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.cwd       = global->cwd;
        attr.fd_stdin  = STDIN_FILENO;
        attr.fd_stdout = STDOUT_FILENO;
        attr.fd_stderr = STDERR_FILENO;
        attr.detached  = false;

        pid_t pid = process_create(global->cmd, &attr);
        if (pid) {
                process_wait(pid, NULL, MAX_DELAY_MS);
        } else {
                perror(argv[1]);
        }

        u32_t total_time = get_uptime_ms() - start_time;
        printf("\nreal\t%um%u.%03us\n", total_time / 60000, (total_time / 1000) % 60, total_time % 1000);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
