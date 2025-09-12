/*==============================================================================
@file   login.c

@author Daniel Zorychta

@brief  Terminal login application.

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
#include <dnx/thread.h>
#include <dnx/os.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define LOGIN            "root"
#define PASSWORD         "!4u"
#define ERROR_SLEEP_TIME 2

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
        char cwd[256];
        char hostname[64];
        char login[64];
        char password[64];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(login, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Cat main function
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return 0 on success.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <prog>\n", argv[0]);
                return EXIT_FAILURE;

        } else {
                get_host_name(global->hostname, sizeof(global->hostname));

                int major = -1;
                int minor = -1;

                struct stat tty_stat;
                if (fstat(fileno(stdin), &tty_stat) == 0) {
                        major = get_driver_major(tty_stat.st_dev);
                        minor = get_driver_minor(tty_stat.st_dev);
                }

                printf("Login to the TTY:%i-%i console on %s.\n", major, minor, global->hostname);

                while (true) {
                        ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
                        printf("\nLogin: ");
                        fflush(stdout);

                        if (fgets(global->login, sizeof(global->login), stdin)) {

                                LAST_CHARACTER(global->login) = '\0';

                                if (isstrempty(global->login)) {
                                        continue;
                                }

                                ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF);
                                printf("Password: ");
                                fflush(stdout);

                                if (fgets(global->password, sizeof(global->password), stdin)) {

                                        LAST_CHARACTER(global->password) = '\0';

                                        puts("");
                                        ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);

                                        if (isstreq(global->login, LOGIN) and
                                                isstreq(global->password, PASSWORD)) {
                                                break;
                                        } else {
                                                printf("Incorrect password!\n");
                                                sleep(ERROR_SLEEP_TIME);
                                        }
                                }
                        }
                }

                getcwd(global->cwd, sizeof(global->cwd));

                process_attr_t attr = {
                        .cwd       = global->cwd,
                        .fd_stderr = STDERR_FILENO,
                        .fd_stdin  = STDIN_FILENO,
                        .fd_stdout = STDOUT_FILENO,
                        .detached  = true,
                        .p_stderr  = NULL,
                        .p_stdin   = NULL,
                        .p_stdout  = NULL,
                        .priority  = process_get_priority(getpid()),
                };

                pid_t pid = process_create(argv[1], &attr);

                return pid != 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        }
}

/*==============================================================================
  End of file
==============================================================================*/
