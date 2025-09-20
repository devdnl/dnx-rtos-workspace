/*==============================================================================
File    syscall.c

Author  Daniel Zorychta

Brief   Syscall handling.

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
#include <stddef.h>
#include <stdarg.h>
#include "syscall.h"
#include "common.h"

/*==============================================================================
  Local macros
==============================================================================*/
/*
 * Syscall section address is 0x08000400 in normal mode.
 * The syscall section compiled is in the Thumb mode. To indicate this mode jump
 * address is incremented by 1.
 */
#define dnx_syscall ((syscall_func)(__LIBC_DNX_RTOS_ADDRESS__ + 0x401))

/*==============================================================================
  Local object types
==============================================================================*/
typedef int (*syscall_func)(_libc_syscall_t, va_list);

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
 * @brief  Syscall function.
 *
 * @param  syscall      syscall number
 * @param  ...          additional arguments
 *
 * @return Operation status (errno).
 */
//==============================================================================
int _libc_syscall(_libc_syscall_t syscall, ...)
{
        va_list args;
        va_start(args, syscall);
        int err = dnx_syscall(syscall, args);
        va_end(args);
        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
