/*==============================================================================
File    atomic.h

Author  Daniel Zorychta

Brief   Atomic operations of libc (futex, heap).

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

#ifndef _LIBC_SOURCE_ATOMIC_H_
#define _LIBC_SOURCE_ATOMIC_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>
#include "syscall.h"

/*==============================================================================
  Exported macros
==============================================================================*/
#if defined(__arm__) && (!defined(__ARM_FEATURE_LDREX) || defined(__DNX_ATOMIC_KERNEL__))
#define _LIBC_ATOMIC_KERNEL 1
#endif

/*==============================================================================
  Exported inline functions
==============================================================================*/
//==============================================================================
/**
 * @brief  Atomic test and set of a flag (atomic_flag, futex_t), acquire order.
 *
 * On CPUs without exclusive access instructions (Cortex-M0+) compiler generates
 * not atomic code for atomic_flag (not protected against preemption and other
 * cores of RP2040), so the flag is set by the kernel. The same is done on
 * multicore CPUs whose exclusive access does not work between cores
 * (__DNX_ATOMIC_KERNEL__ in CPUCONFIG_CFLAGS of the CPU, see kernel
 * src/kernel/syscall.c). The flag is cleared by atomic_flag_clear_explicit()
 * (memory_order_release) on all CPUs.
 *
 * @param  flag         flag (atomic_flag or first byte of futex_t)
 *
 * @return Previous state of the flag: false when the flag is set by caller.
 */
//==============================================================================
static inline bool _libc_atomic_flag_test_and_set(volatile void *flag)
{
#if defined(_LIBC_ATOMIC_KERNEL)
        return _libc_syscall(_LIBC_SYS_ATOMICTAS, flag) != 0;
#else
        return atomic_flag_test_and_set_explicit((volatile atomic_flag *)flag, memory_order_acquire);
#endif
}

//==============================================================================
/**
 * @brief  Check that atomic operations of libc are the same as of the kernel.
 *
 * The kernel of multicore CPU without working exclusive access between cores
 * handles SYSCALL_ATOMICTAS (__DNX_ATOMIC_KERNEL__ in CPUCONFIG_CFLAGS of the
 * kernel), then libc shall be built with the same flag (CPUCONFIG_CFLAGS of the
 * appset project), otherwise futex and heap are not protected between cores.
 *
 * @return true when libc and kernel use the same atomic operations.
 */
//==============================================================================
static inline bool _libc_atomic_check(void)
{
#if defined(__arm__) && !defined(_LIBC_ATOMIC_KERNEL)
        volatile atomic_flag probe = ATOMIC_FLAG_INIT;
        return _libc_syscall(_LIBC_SYS_ATOMICTAS, &probe) == ENOSYS;
#else
        return true;
#endif
}

#endif /* _LIBC_SOURCE_ATOMIC_H_ */
/*==============================================================================
  End of file
==============================================================================*/
