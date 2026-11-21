/*==============================================================================
@file   chown.c

@author Daniel Zorychta

@brief  Change owner and group of files.

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
#include <sys/stat.h>
#include <auth.h>

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char owner[AUTH_USER_MAX + 1];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(chown, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Convert ID or user name to number.
 *
 * @param  str          ID or user name (group: only ID)
 * @param  user         user name is allowed
 * @param  id           result ID
 * @param  gid          group ID of user (can be NULL)
 *
 * @return true on success.
 */
//==============================================================================
static bool get_id(const char *str, bool user, int *id, int *gid)
{
        char *end = NULL;
        long n = strtol(str, &end, 10);

        if ((end != str) && (*end == '\0') && (n >= 0) && (n <= 65535)) {
                *id = n;
                return true;
        }

        bool found = false;
        auth_user_t *account = user ? malloc(sizeof(auth_user_t)) : NULL;
        if (account && (auth_get_user(str, 0, account) == 0)) {
                *id = account->uid;
                if (gid) {
                        *gid = account->gid;
                }
                found = true;
        }

        free(account);

        return found;
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
        if (argc < 3) {
                printf("Usage: %s <user|uid>[:<gid>] <file>...\n", argv[0]);
                puts("Change owner and group of files (only root). Without group the group of user is set.");
                return EXIT_FAILURE;
        }

        strlcpy(global->owner, argv[1], sizeof(global->owner));

        char *group = strchr(global->owner, ':');
        if (group) {
                *group++ = '\0';
        }

        int uid = -1;
        int gid = -1;
        int user_gid = -1;

        if (!get_id(global->owner, true, &uid, &user_gid) || (group && !get_id(group, false, &gid, NULL))) {
                printf("%s: unknown user or group '%s'\n", argv[0], argv[1]);
                return EXIT_FAILURE;
        }

        if (gid < 0) {
                gid = (user_gid >= 0) ? user_gid : uid;
        }

        int status = EXIT_SUCCESS;

        for (int i = 2; i < argc; i++) {
                if (chown(argv[i], uid, gid) != 0) {
                        perror(argv[i]);
                        status = EXIT_FAILURE;
                }
        }

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
