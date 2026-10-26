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
 * the command has been completed. The command is passed to the shell as one
 * argument (in single quotes, quote characters are escaped).
 *
 * @param command       command to execute (NULL: check if shell is available)
 *
 * @return Command exit status, -1 on error. For NULL command non-zero value
 *         (shell is available).
 */
//==============================================================================
int system(const char *command)
{
        if (!command) {
                return 1;
        }

        int status = -1;

        /* each quote is written as '\'' (4 characters) */
        size_t len = strlen(__LIBC_SYSTEM_PROG__) + 4;
        for (const char *p = command; *p; p++) {
                len += (*p == '\'') ? 4 : 1;
        }

        static const int CWDLEN = 256;
        char *cwd = malloc(CWDLEN);
        char *cmd = calloc(1, len);
        if (cmd && cwd) {
                char *dst = cmd;

                strcpy(dst, __LIBC_SYSTEM_PROG__ " '");
                dst += strlen(dst);

                for (const char *p = command; *p; p++) {
                        if (*p == '\'') {
                                memcpy(dst, "'\\''", 4);
                                dst += 4;
                        } else {
                                *dst++ = *p;
                        }
                }

                *dst++ = '\'';
                *dst = '\0';

                if (!getcwd(cwd, CWDLEN)) {
                        strcpy(cwd, "/");
                }

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

                pid_t pid = process_create(cmd, &attr);
                if (pid) {
                        process_wait(pid, &status, MAX_DELAY_MS);
                }
        }

        free(cmd);
        free(cwd);

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
