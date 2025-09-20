/*==============================================================================
File    common.h

Author  Daniel Zorychta

Brief   Common header file.

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

/**
@defgroup _LIBC_SOURCE_COMMON_H_ _LIBC_SOURCE_COMMON_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stdbool.h>
#include <libc/include/sys/types.h>
#include <libc/source/heap.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#ifndef errno
#define errno           (*_libc_errno)
#endif

/*==============================================================================
  Exported object types
==============================================================================*/
struct _libc_file {
        int fd;
        char *tmppath;

        struct {
                bool eof:1;
                bool error:1;
        } flag;
};

struct _libc_socket {
        int fd;
};

struct _libc_dir {
        int fd;
};

typedef struct {
        /** buffer used to store temporary time structure */
        struct tm _tmbuf;

        /** buffer used to store converted time to string */
        char _timestr[32];

        /** rand() seed */
        unsigned int _rand_seed;

        struct _libc_file *_stdin;
        struct _libc_file *_stdout;
        struct _libc_file *_stderr;

        _libc_heap_t *_heap;
} _libc_app_ctx_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern void *_libc_global_get(void);
extern int  *_libc_errno_get(void);
extern void *_libc_appctx_get(void);
extern void *_libc_malloc(size_t size);
extern void  _libc_free(void *mem);
extern int close(int fd);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
