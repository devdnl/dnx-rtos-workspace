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
#include <string.h>
#include <errno.h>
#include <dnx/thread.h>
#include <dnx/os.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <auth.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DEFAULT_SHELL "dsh"
#define SERVICE       "shell"

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
        char login[AUTH_USER_MAX + 2];
        char password[AUTH_PASSWORD_MAX + 2];
        auth_user_t user;
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
 * @brief Ask for login and password until correct account is given.
 *
 * @return true on success, false: end of input.
 */
//==============================================================================
static bool authenticate(void)
{
        for (;;) {
                ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
                printf("\nLogin: ");
                fflush(stdout);

                if (!fgets(global->login, sizeof(global->login), stdin)) {
                        return false;
                }

                global->login[strcspn(global->login, "\r\n")] = '\0';
                if (isstrempty(global->login)) {
                        continue;
                }

                ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF);
                printf("Password: ");
                fflush(stdout);

                bool ok = false;
                if (fgets(global->password, sizeof(global->password), stdin)) {
                        global->password[strcspn(global->password, "\r\n")] = '\0';
                        ok = (auth_check(global->login, global->password, SERVICE, NULL) == 0) &&
                                (auth_get_user(global->login, 0, &global->user) == 0);
                }

                auth_wipe(global->password, sizeof(global->password));
                ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
                puts("");

                if (ok) {
                        return true;
                }

                msleep(AUTH_FAIL_DELAY_MS);
                puts("Login incorrect");
        }
}

//==============================================================================
/**
 * @brief Login main function. Accounts of service "shell" (auth library) are
 *        accepted, the shell of account (default: program of argument, dsh)
 *        is started with user and group ID of account in home directory and
 *        login waits for its end (option -l: login prompt is shown again).
 *        Without account file the program is started without login
 *        (passwords are set by passwd program).
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return Exit status of session.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *prog = DEFAULT_SHELL;
        bool loop = false;

        for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-l") == 0) {
                        loop = true;
                } else if ((argv[i][0] != '-') && (i == argc - 1)) {
                        prog = argv[i];
                } else {
                        printf("Usage: %s [-l] [prog]\n", argv[0]);
                        puts("Log in on terminal and start shell of account (default: prog, dsh).");
                        puts("  -l             show login prompt again after end of session (terminal");
                        puts("                 of initd), otherwise login ends with the session");
                        return EXIT_FAILURE;
                }
        }

        if (auth_is_configured() && (geteuid() != 0)) {
                printf("%s: only root can start session of user, use su\n", argv[0]);
                return EXIT_FAILURE;
        }

        get_host_name(global->hostname, sizeof(global->hostname));

        int major = -1;
        int minor = -1;

        struct stat tty_stat;
        if (fstat(fileno(stdin), &tty_stat) == 0) {
                major = get_driver_major(tty_stat.st_dev);
                minor = get_driver_minor(tty_stat.st_dev);
        }

        int status = EXIT_FAILURE;

        do {
                printf("Login to the TTY:%i-%i console on %s.\n", major, minor, global->hostname);

                bool configured = auth_is_configured();
                if (!configured) {
                        puts("No account file: login is not required (set passwords by passwd program).");
                        memset(&global->user, 0, sizeof(global->user));

                } else if (!authenticate()) {
                        return EXIT_FAILURE; // end of input
                }

                const char *cmd = global->user.shell[0] ? global->user.shell : prog;
                const char *home = global->user.home[0] ? global->user.home : "/";

                struct stat st;
                if ((stat(home, &st) != 0) || !S_ISDIR(st.st_mode)) {
                        printf("No directory %s, logging in with home directory /.\n", home);
                        home = "/";
                }
                strlcpy(global->cwd, home, sizeof(global->cwd));

                process_attr_t attr = {
                        .cwd = global->cwd,
                        .fd_stderr = STDERR_FILENO,
                        .fd_stdin = STDIN_FILENO,
                        .fd_stdout = STDOUT_FILENO,
                        .detached = false,
                        .priority = process_get_priority(getpid()),
                };

                pid_t pid = process_create_as(cmd, &attr, global->user.uid, global->user.gid);
                if ((pid == 0) && (errno == ENOSYS)) {
                        pid = process_create(cmd, &attr); // system without users
                }

                if (pid == 0) {
                        perror(cmd);
                        status = EXIT_FAILURE;
                } else {
                        process_wait(pid, &status, MAX_DELAY_MS); // session
                }

                memset(&global->user, 0, sizeof(global->user));

                if (loop) {
                        sleep(1); // program that ends at once is not restarted too often
                }

        } while (loop);

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
