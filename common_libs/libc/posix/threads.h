/*==============================================================================
@file   threads.h

@author Daniel Zorychta

@brief  C11 threads declarations for the POSIX simulator (the host
        threads.h cannot be used, types are defined by dnx libc).

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

#ifndef _LIBC_POSIX_THREADS_H_
#define _LIBC_POSIX_THREADS_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <machine/_threads.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#if !defined(__cplusplus) || __cplusplus < 201103L
#define thread_local _Thread_local
#endif

/*==============================================================================
  Exported object types
==============================================================================*/
typedef void (*tss_dtor_t)(void *);
typedef int (*thrd_start_t)(void *);

enum {
        mtx_plain = 0x1,
        mtx_recursive = 0x2,
        mtx_timed = 0x4,
};

enum {
        thrd_busy = 1,
        thrd_error = 2,
        thrd_nomem = 3,
        thrd_success = 4,
        thrd_timedout = 5,
};

/*==============================================================================
  Exported functions
==============================================================================*/
extern void call_once(once_flag *, void (*)(void));
extern int cnd_broadcast(cnd_t *);
extern void cnd_destroy(cnd_t *);
extern int cnd_init(cnd_t *);
extern int cnd_signal(cnd_t *);
extern int cnd_timedwait(cnd_t *, mtx_t *, const struct timespec *);
extern int cnd_wait(cnd_t *, mtx_t *);
extern void mtx_destroy(mtx_t *);
extern int mtx_init(mtx_t *, int);
extern int mtx_lock(mtx_t *);
extern int mtx_timedlock(mtx_t *, const struct timespec *);
extern int mtx_trylock(mtx_t *);
extern int mtx_unlock(mtx_t *);
extern int thrd_create(thrd_t *, thrd_start_t, void *);
extern thrd_t thrd_current(void);
extern int thrd_detach(thrd_t);
extern int thrd_equal(thrd_t, thrd_t);
extern _Noreturn void thrd_exit(int);
extern int thrd_join(thrd_t, int *);
extern int thrd_sleep(const struct timespec *, struct timespec *);
extern void thrd_yield(void);
extern int tss_create(tss_t *, tss_dtor_t);
extern void tss_delete(tss_t);
extern void *tss_get(tss_t);
extern int tss_set(tss_t, void *);

#ifdef __cplusplus
}
#endif

#endif /* _LIBC_POSIX_THREADS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
