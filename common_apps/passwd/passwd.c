/*==============================================================================
@file   passwd.c

@author Daniel Zorychta

@brief  Management of accounts of password file (auth library).

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
#include <sys/ioctl.h>
#include <dnx/misc.h>
#include <auth.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define RANDOM_PASSWORD_LEN 20

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
        char password[AUTH_PASSWORD_MAX + 2];
        char repeat[AUTH_PASSWORD_MAX + 2];
        auth_user_t user;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
/* privileged program: users change own passwords */
PROGRAM_PARAMS_FLAGS(passwd, STACK_DEPTH_LOW, NULL, PROGRAM_FLAG_PRIVILEGED);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Print usage.
 *
 * @param  name         program name
 */
//==============================================================================
static void usage(const char *name)
{
        printf("Usage: %s [options] <user>\n", name);
        printf("       %s -d [-s <services>] <user>\n", name);
        printf("       %s -l\n", name);
        printf("       %s [-s <services>]   (user: change own password)\n", name);
        puts("Set password of user (account is created when it does not exist).");
        puts("  -s <services>  services of password separated by commas: shell (console),");
        puts("                 telnet, ftp, mqtt,");
        puts("                 sudo ('*': all, default for new account); other lines of");
        puts("                 user lose these services (separate password of service)");
        printf("  -i <n>         PBKDF2 iterations (default %d)\n", AUTH_ITERATIONS_DEFAULT);
        puts("  -g             generate random password (printed), default 1 iteration");
        puts("  -u <uid>       user ID (new user: 0 for root, otherwise the first free from 1000)");
        puts("  -G <gid>       group ID (new user: user ID)");
        puts("  -H <dir>       home directory (empty: /)");
        puts("  -S <shell>     shell command (empty: program of service)");
        puts("  -d             remove account (with -s: remove services)");
        puts("  -l             list accounts");
        puts("Options -u, -G, -H, -S change account of existing user without password.");
        puts("Users change only own existing password (current password is checked).");
        puts("Password is read twice from terminal or once from other input.");
        printf("Files: %s, %s (default: %s, %s)\n", AUTH_PASSWD_RW_FILE, AUTH_SHADOW_RW_FILE,
                AUTH_PASSWD_FILE, AUTH_SHADOW_FILE);
}

//==============================================================================
/**
 * @brief  Print account (auth_foreach_user() callback).
 */
//==============================================================================
static void print_user(const auth_user_t *user, const char *services, uint32_t iterations, void *arg)
{
        (void)arg;
        printf("%-12s %5u %5u  %-12s %-8s %-20s %u\n", user->name, user->uid, user->gid,
                user->home[0] ? user->home : "/", user->shell[0] ? user->shell : "-",
                services ? services : "-", (unsigned)iterations);
}

//==============================================================================
/**
 * @brief  Read line without new line character.
 *
 * @param  prompt       prompt (NULL: none)
 * @param  buf          buffer
 * @param  size         buffer size
 *
 * @return true on success.
 */
//==============================================================================
static bool read_line(const char *prompt, char *buf, size_t size)
{
        if (prompt) {
                printf("%s", prompt);
                fflush(stdout);
        }

        if (!fgets(buf, size, stdin)) {
                return false;
        }

        buf[strcspn(buf, "\r\n")] = '\0';

        if (prompt) {
                puts("");
        }

        return true;
}

//==============================================================================
/**
 * @brief  Read current password of user.
 *
 * @return true on success.
 */
//==============================================================================
static bool read_current_password(void)
{
        bool tty = ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF) == 0;
        bool ok = read_line(tty ? "Current password: " : NULL, global->password, sizeof(global->password));

        if (tty) {
                ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Read password (twice from terminal).
 *
 * @return true on success.
 */
//==============================================================================
static bool read_password(void)
{
        bool tty = ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF) == 0;
        bool ok = false;

        if (!tty) {
                ok = read_line(NULL, global->password, sizeof(global->password));

        } else if (read_line("New password: ", global->password, sizeof(global->password)) &&
                read_line("Repeat password: ", global->repeat, sizeof(global->repeat))) {

                ok = strcmp(global->password, global->repeat) == 0;
                if (!ok) {
                        puts("Passwords differ.");
                }
        }

        if (tty) {
                ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);
        }

        auth_wipe(global->repeat, sizeof(global->repeat));

        if (ok && (global->password[0] == '\0')) {
                puts("Empty password.");
                ok = false;
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Generate random password (letters and digits).
 *
 * @return true on success.
 */
//==============================================================================
static bool generate_password(void)
{
        static const char CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        uint8_t rnd[32];
        size_t n = 0;

        while (n < RANDOM_PASSWORD_LEN) {
                if (getentropy(rnd, sizeof(rnd)) != 0) {
                        return false;
                }

                /* rejection sampling: uniform distribution of 62 characters */
                for (size_t i = 0; (i < sizeof(rnd)) && (n < RANDOM_PASSWORD_LEN); i++) {
                        if (rnd[i] < (256 / 62) * 62) {
                                global->password[n++] = CHARS[rnd[i] % 62];
                        }
                }
        }

        global->password[n] = '\0';
        auth_wipe(rnd, sizeof(rnd));

        return true;
}

//==============================================================================
/**
 * @brief Program main function.
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return 0 on success.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *user = NULL;
        const char *services = NULL;
        const char *home = NULL;
        const char *shell = NULL;
        uint32_t iterations = 0;
        bool generate = false;
        bool remove_user = false;
        bool list = false;
        int uid = -1;
        int gid = -1;

        for (int i = 1; i < argc; i++) {
                if ((strcmp(argv[i], "-s") == 0) && (i + 1 < argc)) {
                        services = argv[++i];
                } else if ((strcmp(argv[i], "-i") == 0) && (i + 1 < argc)) {
                        iterations = strtoul(argv[++i], NULL, 10);
                        if ((iterations == 0) || (iterations > AUTH_ITERATIONS_MAX)) {
                                printf("Invalid number of iterations (1..%d).\n", AUTH_ITERATIONS_MAX);
                                return EXIT_FAILURE;
                        }
                } else if (((strcmp(argv[i], "-u") == 0) || (strcmp(argv[i], "-G") == 0)) && (i + 1 < argc)) {
                        char *end = NULL;
                        long id = strtol(argv[i + 1], &end, 10);
                        if ((end == argv[i + 1]) || (*end != '\0') || (id < 0) || (id > 65535)) {
                                puts("Invalid ID (0..65535).");
                                return EXIT_FAILURE;
                        }
                        *((argv[i][1] == 'u') ? &uid : &gid) = id;
                        i++;
                } else if ((strcmp(argv[i], "-H") == 0) && (i + 1 < argc)) {
                        home = argv[++i];
                } else if ((strcmp(argv[i], "-S") == 0) && (i + 1 < argc)) {
                        shell = argv[++i];
                } else if (strcmp(argv[i], "-g") == 0) {
                        generate = true;
                } else if (strcmp(argv[i], "-d") == 0) {
                        remove_user = true;
                } else if (strcmp(argv[i], "-l") == 0) {
                        list = true;
                } else if ((argv[i][0] != '-') && !user) {
                        user = argv[i];
                } else {
                        usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }

        bool account = (uid >= 0) || (gid >= 0) || home || shell;
        bool root = (getuid() == 0);
        int err;

        if (!root) {
                /* user changes own existing password */
                if (list || remove_user || account || generate || iterations) {
                        puts("Only root can use these options.");
                        return EXIT_FAILURE;
                }

                if (auth_get_user(NULL, getuid(), &global->user) != 0) {
                        printf("No account of user ID %u.\n", getuid());
                        return EXIT_FAILURE;
                }

                if (user && (strcmp(user, global->user.name) != 0)) {
                        puts("Only root can change password of other user.");
                        return EXIT_FAILURE;
                }

                user = global->user.name;

                if (!read_current_password()) {
                        return EXIT_FAILURE;
                }

                err = auth_check_line(user, global->password, services);
                auth_wipe(global->password, sizeof(global->password));

                if (err != 0) {
                        if (errno == EINVAL) {
                                printf("%s: user has passwords of several services, select them by -s\n",
                                        user);
                        } else {
                                msleep(AUTH_FAIL_DELAY_MS);
                                printf("%s: wrong password or no password of these services\n", user);
                        }
                        return EXIT_FAILURE;
                }
        }

        if (list) {
                printf("%-12s %5s %5s  %-12s %-8s %-20s %s\n", "USER", "UID", "GID", "HOME", "SHELL",
                        "SERVICES", "ITERATIONS");
                if (auth_foreach_user(print_user, NULL) != 0) {
                        if (errno == ENOENT) {
                                printf("No account file (%s, %s).\n", AUTH_PASSWD_RW_FILE, AUTH_PASSWD_FILE);
                        } else {
                                perror(NULL);
                        }
                        return EXIT_FAILURE;
                }
                return EXIT_SUCCESS;
        }

        if (!user) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }

        if (geteuid() != 0) {
                puts("Only root can change the password file.");
                return EXIT_FAILURE;
        }

        if (remove_user) {
                err = auth_remove_user(user, services);

        } else if (account && (auth_get_user(user, 0, &global->user) == 0) && !generate) {
                err = auth_set_user(user, uid, gid, home, shell);

        } else {
                if (generate) {
                        if (!generate_password()) {
                                perror("Random password");
                                return EXIT_FAILURE;
                        }

                        if (iterations == 0) {
                                iterations = 1;
                        }

                } else if (!read_password()) {
                        auth_wipe(global->password, sizeof(global->password));
                        return EXIT_FAILURE;
                }

                err = auth_set_password(user, global->password, services, iterations);

                if ((err == 0) && account) {
                        err = auth_set_user(user, uid, gid, home, shell);
                }

                if ((err == 0) && generate) {
                        printf("Password: %s\n", global->password);
                }

                auth_wipe(global->password, sizeof(global->password));
        }

        if (err != 0) {
                if ((errno == EINVAL) && !remove_user && !services && !account) {
                        printf("%s: user has passwords of several services, select them by -s\n", user);
                } else if (errno == EBUSY) {
                        printf("%s: password file is changed by other program (remove %s"
                               ".tmp if it stays)\n",
                                user, AUTH_SHADOW_RW_FILE);
                } else {
                        perror(user);
                }
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
