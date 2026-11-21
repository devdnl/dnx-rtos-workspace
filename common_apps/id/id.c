/*==============================================================================
@file   id.c

@author Daniel Zorychta

@brief  Print user and group ID.

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
#include <unistd.h>
#include <auth.h>

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        auth_user_t user;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(id, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Print user ID with name (name only when password file is readable).
 *
 * @param  label        label of ID
 * @param  uid          user ID
 */
//==============================================================================
static void print_user(const char *label, uid_t uid)
{
        printf("%s=%u", label, uid);

        if (auth_get_user(NULL, uid, &global->user) == 0) {
                printf("(%s)", global->user.name);
        } else if (uid == 0) {
                printf("(root)");
        }
}

//==============================================================================
/**
 * @brief  Program main function. Prints real user and group ID of the program,
 *         effective IDs when they differ (privileged program).
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return 0 on success.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;

        uid_t uid = getuid();
        gid_t gid = getgid();

        print_user("uid", uid);
        printf(" gid=%u", gid);

        if (geteuid() != uid) {
                printf(" ");
                print_user("euid", geteuid());
        }

        if (getegid() != gid) {
                printf(" egid=%u", getegid());
        }

        puts("");

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
