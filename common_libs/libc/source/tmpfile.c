/*==============================================================================
File    tmpfile.c

Author  Daniel Zorychta

Brief   Temporary files: tmpfile(), mkstemp(), tmpnam().

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

        Names: template ending with "XXXXXX" (P_tmpdir "/tmpXXXXXX" for
        tmpfile() and tmpnam()), the suffix is made of process ID, thread ID,
        system time and number of attempt. File is created by open() with
        O_CREAT | O_EXCL, so the same file is never created by two callers
        (EEXIST: next name). tmpfile() removes the file just after creation
        (file systems that keep data of removed open file, e.g. ramfs: no
        file is left after crash or kill), on other file systems the file is
        removed by fclose().

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "common.h"
#include "syscall.h"

/*==============================================================================
  Local macros
==============================================================================*/
#define SUFFIX_LEN   6
#define MAX_ATTEMPTS 64

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
 * @brief  Function replaces "XXXXXX" at the end of template by unique suffix.
 *
 * @param  suffix       suffix in template (SUFFIX_LEN characters)
 * @param  attempt      number of attempt
 */
//==============================================================================
static void make_suffix(char *suffix, unsigned attempt)
{
        static const char CHARS[] = "0123456789abcdefghijklmnopqrstuvwxyz";

        uint64_t time_ms = 0;
        _libc_syscall(_LIBC_SYS_GETUPTIMEMS, &time_ms);

        uint32_t v = (uint32_t)time_ms * 2654435761u;
        v ^= ((uint32_t)getpid() << 20) ^ ((uint32_t)gettid() << 8) ^ (attempt * 0x9E3779B9u);

        for (int i = 0; i < SUFFIX_LEN; i++) {
                suffix[i] = CHARS[v % (sizeof(CHARS) - 1)];
                v = (v / (sizeof(CHARS) - 1)) ^ (v << 26);
        }
}

//==============================================================================
/**
 * @brief  Function checks template and returns its suffix.
 *
 * @param  template     template ending with "XXXXXX"
 *
 * @return Suffix or NULL (errno EINVAL).
 */
//==============================================================================
static char *template_suffix(char *template)
{
        size_t len = template ? strlen(template) : 0;

        if ((len < SUFFIX_LEN) || (strcmp(template + len - SUFFIX_LEN, "XXXXXX") != 0)) {
                errno = EINVAL;
                return NULL;
        }

        return template + len - SUFFIX_LEN;
}

//==============================================================================
/**
 * @brief  The function creates and opens unique file (O_RDWR, mode 0600).
 *
 * The last six characters of template must be "XXXXXX", they are replaced by
 * the suffix of created file.
 *
 * @param  template     template of file name (modified)
 *
 * @return File descriptor or -1 and errno is set (EINVAL: wrong template,
 *         EEXIST: unique name not found, errors of open()).
 */
//==============================================================================
int _libc_mkstemp(char *template)
{
        char *suffix = template_suffix(template);
        if (!suffix) {
                return -1;
        }

        for (unsigned attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
                make_suffix(suffix, attempt);

                int fd = open(template, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
                if ((fd >= 0) || (errno != EEXIST)) {
                        return fd;
                }
        }

        memcpy(suffix, "XXXXXX", SUFFIX_LEN);
        errno = EEXIST;
        return -1;
}

//==============================================================================
/**
 * @brief  Function creates temporary file in P_tmpdir directory (mode "w+").
 *
 * The file is removed just after creation when file system keeps data of
 * removed open file (ramfs), otherwise by fclose() (also at exit()).
 *
 * @return Stream or NULL and errno is set.
 */
//==============================================================================
FILE *_libc_tmpfile(void)
{
        char path[] = P_tmpdir "/tmpXXXXXX";

        int fd = _libc_mkstemp(path);
        if (fd < 0) {
                return NULL;
        }

        int err = errno;
        bool removed = (remove(path) == 0);
        errno = err;

        FILE *f = fdopen(fd, "w+");
        if (f && !removed) {
                f->tmppath = strdup(path);
                if (!f->tmppath) {
                        fclose(f);
                        f = NULL;
                }
        }

        if (!f) {
                if (!removed) {
                        remove(path);
                }

                errno = ENOMEM;
        }

        return f;
}

//==============================================================================
/**
 * @brief  Function creates name of not existing file in P_tmpdir directory.
 *
 * The file may be created by other program before it is used by the caller,
 * mkstemp() and tmpfile() create the file atomically.
 *
 * @param  str          buffer of L_tmpnam bytes (NULL: not supported, there
 *                      is no static buffer in libc)
 *
 * @return str or NULL and errno is set (EINVAL: str is NULL, EEXIST).
 */
//==============================================================================
char *_libc_tmpnam(char *str)
{
        if (!str) {
                errno = EINVAL;
                return NULL;
        }

        strcpy(str, P_tmpdir "/tmpXXXXXX");
        char *suffix = str + strlen(str) - SUFFIX_LEN;

        for (unsigned attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
                make_suffix(suffix, attempt);

                struct stat st;
                if ((stat(str, &st) != 0) && (errno == ENOENT)) {
                        return str;
                }
        }

        errno = EEXIST;
        return NULL;
}

/*==============================================================================
  End of file
==============================================================================*/
