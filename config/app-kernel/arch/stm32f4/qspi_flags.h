/*==============================================================================
File    qspi_flags.h

Author  Daniel Zorychta

Brief   Quad SPI

        MIT License

        Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as prefix
 *       and suffix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _QSPI_FLAGS_H_
#define _QSPI_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > QSPI", "The QSPI module configuration.")
++*/

/*--
this:AddWidget("Combobox", "IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __QSPI_CFG_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

/*--
this:AddWidget("Spinbox", 100, 60000, "QSPI operation timeout [ms]")
--*/
#define __QSPI_CFG_TIMEOUT__ 500

/*--
this:AddWidget("Combobox", "Memory mapped mode timeout counter")
this:AddItem("Disable", "QSPI_TIMEOUT_COUNTER_DISABLE")
this:AddItem("Enable", "QSPI_TIMEOUT_COUNTER_ENABLE")
--*/
#define __QSPI_CFG_MMM_TIMOUT_COUNTER__ QSPI_TIMEOUT_COUNTER_DISABLE

/*--
this:AddWidget("Spinbox", 0, 65535, "Memory mapped mode timeout cycles")
--*/
#define __QSPI_CFG_MMM_TIMEOUT_CYCLES__ 128

/*--
this:AddWidget("Spinbox", 0, 65535, "Polling interval clock cycles")
--*/
#define __QSPI_CFG_POLLING_INTERVAL_CLKS__ 128

/*--
this:AddWidget("Combobox", "Driver mode")
this:AddItem("Polling", "_QSPI_MODE_POLLING")
this:AddItem("IRQ", "_QSPI_MODE_IRQ")
this:AddItem("DMA", "_QSPI_MODE_DMA")
--*/
#define __QSPI_CFG_MODE__ _QSPI_MODE_POLLING

#endif /* _QSPI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
