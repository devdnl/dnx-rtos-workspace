/*==============================================================================
@file   su.c

@author Daniel Zorychta

@brief  Start shell or command as other user (only root).

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
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <dnx/thread.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <auth.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define SHELL   "dsh"
#define SERVICE "shell"

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char cwd[128];
        char password[AUTH_PASSWORD_MAX + 2];
        auth_user_t account;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
/* privileged program: users switch to other account with its password */
PROGRAM_PARAMS_FLAGS(su, STACK_DEPTH_LOW, NULL, PROGRAM_FLAG_PRIVILEGED);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Program main function. Starts shell of account (default dsh) or
 *         command with user and group ID of account (auth library). Root
 *         switches without password, other users give password of the account
 *         (service "shell").
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return Exit status of started program.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *user = "root";
        const char *command = NULL;
        bool login = false;

        for (int i = 1; i < argc; i++) {
                if ((strcmp(argv[i], "-c") == 0) && (i + 1 < argc)) {
                        command = argv[++i];
                } else if ((strcmp(argv[i], "-") == 0) || (strcmp(argv[i], "-l") == 0)) {
                        login = true;
                } else if (argv[i][0] != '-') {
                        user = argv[i];
                } else {
                        printf("Usage: %s [-] [-c <command>] [user]\n", argv[0]);
                        puts("Start shell or command with user and group ID of user (default root).");
                        puts("  -, -l          start in home directory of user (login)");
                        puts("Users give password of the user (service 'shell'), root does not.");
                        return EXIT_FAILURE;
                }
        }

        if (geteuid() != 0) {
                printf("%s: program is not privileged\n", argv[0]);
                return EXIT_FAILURE;
        }

        if (auth_get_user(user, 0, &global->account) != 0) {
                if (strcmp(user, "root") != 0) {
                        printf("%s: unknown user '%s'\n", argv[0], user);
                        return EXIT_FAILURE;
                }
                memset(&global->account, 0, sizeof(global->account)); // root without account
        }

        if (getuid() != 0) {
                bool tty = ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF) == 0;
                if (tty) {
                        printf("Password: ");
                        fflush(stdout);
                }

                bool got = fgets(global->password, sizeof(global->password), stdin) != NULL;

                if (tty) {
                        ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
                        puts("");
                }

                global->password[strcspn(global->password, "\r\n")] = '\0';
                bool ok = got && (auth_check(user, global->password, SERVICE, NULL) == 0);
                auth_wipe(global->password, sizeof(global->password));

                if (!ok) {
                        msleep(AUTH_FAIL_DELAY_MS);
                        printf("%s: authentication failure\n", argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (!command) {
                command = global->account.shell[0] ? global->account.shell : SHELL;
        }

        struct stat st;
        if (login && global->account.home[0] && (stat(global->account.home, &st) == 0) &&
                S_ISDIR(st.st_mode)) {
                strlcpy(global->cwd, global->account.home, sizeof(global->cwd));
        } else if (login) {
                strcpy(global->cwd, "/");
        } else if (!getcwd(global->cwd, sizeof(global->cwd))) {
                strcpy(global->cwd, "/");
        }

        process_attr_t attr = {
                .cwd = global->cwd,
                .fd_stdin = STDIN_FILENO,
                .fd_stdout = STDOUT_FILENO,
                .fd_stderr = STDERR_FILENO,
                .priority = process_get_priority(getpid()),
                .detached = false,
        };

        pid_t pid = process_create_as(command, &attr, global->account.uid, global->account.gid);
        if (pid == 0) {
                perror(command);
                return EXIT_FAILURE;
        }

        int status = EXIT_FAILURE;
        process_wait(pid, &status, MAX_DELAY_MS);

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
