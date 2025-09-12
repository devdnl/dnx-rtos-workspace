/*==============================================================================
File    system.c

Author  Daniel Zorychta

Brief   system() function implementation.

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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dnx/thread.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Function executes specified command.
 *
 * The system() executes a command specified in <i>command</i> and returns after
 * the command has been completed.
 *
 * @param command       command to execute
 */
//==============================================================================
int system(const char *command)
{
        int status = -1;

        size_t len = strlen(__LIBC_SYSTEM_PROG__) + strlen(command) + 4;

        static const int CWDLEN = 256;
        char *cwd = malloc(CWDLEN);
        char *cmd = calloc(1, len);
        if (cmd && cwd) {
                strcpy(cmd, __LIBC_SYSTEM_PROG__);
                strcat(cmd, " '");
                strcat(cmd, command);
                strcat(cmd, "'");

                getcwd(cwd, CWDLEN);

                process_attr_t attr = {
                      .fd_stdin  = 0,
                      .fd_stdout = 1,
                      .fd_stderr = 2,
                      .p_stdin   = NULL,
                      .p_stdout  = NULL,
                      .p_stderr  = NULL,
                      .cwd       = cwd,
                      .priority  = PRIORITY_NORMAL,
                      .detached  = false
                };

                process_wait(process_create(cmd, &attr), &status, MAX_DELAY_MS);

                free(cmd);
                free(cwd);
        } else {
                if (cwd) {
                        free(cwd);
                }

                if (cmd) {
                        free(cmd);
                }
        }

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
