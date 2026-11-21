/*==============================================================================
@file   sudo.c

@author Daniel Zorychta

@brief  Run command as root or other user (password of calling user).

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
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/thread.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>
#include <auth.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define SERVICE  "sudo"
#define ATTEMPTS 3

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char user[AUTH_USER_MAX + 1];
        auth_user_t account;
        char password[AUTH_PASSWORD_MAX + 2];
        char cwd[128];
        char cmd[256];
        bool allowed;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
/* privileged program: effective IDs of root, real IDs of calling user */
PROGRAM_PARAMS_FLAGS(sudo, STACK_DEPTH_LOW, NULL, PROGRAM_FLAG_PRIVILEGED);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Check password line: calling user with service "sudo" given
 *         explicitly (line with "*" does not allow sudo).
 *
 * @param  user         account
 * @param  services     services of line (NULL: no password)
 * @param  iterations   PBKDF2 iterations
 * @param  arg          argument (not used)
 */
//==============================================================================
static void check_line(const auth_user_t *user, const char *services, uint32_t iterations, void *arg)
{
        (void)iterations;
        (void)arg;

        if (!services || (strcmp(user->name, global->user) != 0)) {
                return;
        }

        size_t len = strlen(SERVICE);

        for (const char *s = services; *s;) {
                size_t n = strcspn(s, ",");
                if ((n == len) && (strncmp(s, SERVICE, len) == 0)) {
                        global->allowed = true;
                }

                s += n;
                if (*s == ',') {
                        s++;
                }
        }
}

//==============================================================================
/**
 * @brief  Append argument to command line (quoted when needed).
 *
 * @param  len          length of command line
 * @param  arg          argument
 *
 * @return true on success, false: command line too long.
 */
//==============================================================================
static bool append_arg(size_t *len, const char *arg)
{
        char *cmd = global->cmd;
        size_t size = sizeof(global->cmd);
        bool quote = (arg[0] == '\0') || (strpbrk(arg, " \t'\"\\") != NULL);

#define PUT(c)                          \
        do {                            \
                if (*len + 1 >= size) { \
                        return false;   \
                }                       \
                cmd[(*len)++] = (c);    \
        } while (0)

        if (*len) {
                PUT(' ');
        }

        if (quote) {
                PUT('\'');
        }

        for (const char *c = arg; *c; c++) {
                if (quote && (*c == '\'')) {
                        // end quotation, escaped quote, new quotation: '\''
                        PUT('\'');
                        PUT('\\');
                        PUT('\'');
                }
                PUT(*c);
        }

        if (quote) {
                PUT('\'');
        }

#undef PUT

        cmd[*len] = '\0';
        return true;
}

//==============================================================================
/**
 * @brief  Ask for password of calling user.
 *
 * @return true: correct password.
 */
//==============================================================================
static bool authenticate(void)
{
        bool ok = false;

        for (int i = 0; !ok && (i < ATTEMPTS); i++) {
                bool tty = ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF) == 0;

                printf("[sudo] password for %s: ", global->user);
                fflush(stdout);

                bool got = fgets(global->password, sizeof(global->password), stdin) != NULL;

                if (tty) {
                        ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
                }
                puts("");

                if (!got) {
                        break;
                }

                global->password[strcspn(global->password, "\r\n")] = '\0';

                ok = auth_check(global->user, global->password, SERVICE, NULL) == 0;
                auth_wipe(global->password, sizeof(global->password));

                if (!ok) {
                        msleep(AUTH_FAIL_DELAY_MS);
                        puts("Sorry, try again.");
                }
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Program main function. Runs command as root (or user selected by -u)
 *         after password of calling user is checked. User must have account
 *         with service "sudo" (passwd -s shell,sudo <user>). Root runs command
 *         without password.
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return Exit status of started program.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *target = "root";
        int i = 1;

        if ((i + 1 < argc) && (strcmp(argv[i], "-u") == 0)) {
                target = argv[i + 1];
                i += 2;
        }

        if ((i >= argc) || (argv[i][0] == '-')) {
                printf("Usage: %s [-u <user>] <command> [arguments]\n", argv[0]);
                puts("Run command as root (or user) after password of calling user is checked.");
                puts("User needs password of service 'sudo' (passwd -s shell,sudo <user>).");
                return EXIT_FAILURE;
        }

        size_t len = 0;
        global->cmd[0] = '\0';
        for (; i < argc; i++) {
                if (!append_arg(&len, argv[i])) {
                        printf("%s: command too long\n", argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (geteuid() != 0) {
                printf("%s: program is not privileged\n", argv[0]);
                return EXIT_FAILURE;
        }

        uid_t uid = getuid();

        if (uid != 0) {
                if (auth_get_user(NULL, uid, &global->account) == 0) {
                        strcpy(global->user, global->account.name);
                } else {
                        printf("%s: no account of user ID %u\n", argv[0], uid);
                        return EXIT_FAILURE;
                }

                auth_foreach_user(check_line, NULL);

                if (!global->allowed) {
                        printf("%s: user '%s' is not allowed to use sudo\n", argv[0], global->user);
                        return EXIT_FAILURE;
                }

                if (!authenticate()) {
                        printf("%s: incorrect password\n", argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (auth_get_user(target, 0, &global->account) != 0) {
                if (strcmp(target, "root") != 0) {
                        printf("%s: unknown user '%s'\n", argv[0], target);
                        return EXIT_FAILURE;
                }
                memset(&global->account, 0, sizeof(global->account)); // root without account
        }

        if (!getcwd(global->cwd, sizeof(global->cwd))) {
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

        pid_t pid = process_create_as(global->cmd, &attr, global->account.uid, global->account.gid);
        if ((pid == 0) && (errno == ENOSYS)) {
                pid = process_create(global->cmd, &attr); // system without users
        }

        if (pid == 0) {
                perror(global->cmd);
                return EXIT_FAILURE;
        }

        int status = EXIT_FAILURE;
        process_wait(pid, &status, MAX_DELAY_MS);

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
