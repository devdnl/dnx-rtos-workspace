/*==============================================================================
File    example_ioctl.h

Author  Daniel Zorychta

Brief   EXAMPLE driver (drvset example).

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

/**
@defgroup drv-example EXAMPLE Driver

\section drv-example-desc Description
Example driver of drvset partition (driver built separately from the kernel).
Read returns number of interrupts handled by the driver. On STM32H7 the
interrupt is triggered by software (IOCTL_EXAMPLE__TRIGGER_IRQ, CRS interrupt
that is not used by the kernel), so the drvset interrupt dispatcher can be
tested without additional hardware.

\section drv-example-sup-arch Supported architectures
\li noarch (interrupt test: stm32h7)

\subsection drv-example-ddesc-init Driver initialization
@code
driver_init("EXAMPLE", 0, 0, "/dev/example");
@endcode

\subsection drv-example-ddesc-read Data read
Text: "irq: <number of handled interrupts>\n".

@{
*/

#ifndef _EXAMPLE_IOCTL_H_
#define _EXAMPLE_IOCTL_H_

/*==============================================================================
  Include files
==============================================================================*/
#include "drivers/ioctl_macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
/**
 * @brief  Trigger interrupt of driver by software (STM32H7 only).
 * @param  [WR] NULL
 * @return On success 0 is returned, otherwise -1 (ENOTSUP: not supported by CPU).
 */
#define IOCTL_EXAMPLE__TRIGGER_IRQ _IO(EXAMPLE, 0x00)

#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_IOCTL_H_ */
/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
