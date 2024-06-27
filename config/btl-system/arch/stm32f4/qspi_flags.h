/*==============================================================================
File    qspi_flags.h

Author  Daniel Zorychta

Brief   Quad SPI

        Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation and modified by the dnx RTOS exception.

        NOTE: The modification  to the GPL is  included to allow you to
              distribute a combined work that includes dnx RTOS without
              being obliged to provide the source  code for proprietary
              components outside of the dnx RTOS.

        The dnx RTOS  is  distributed  in the hope  that  it will be useful,
        but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
        MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
        GNU General Public License for more details.

        Full license text is available on the following file: doc/license.txt.

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
this:SetLayout("TitledGridBack", 2, "Home > Microcontroller > QSPI",
               function() this:LoadFile("arch/arch_flags.h") end)
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
#define __QSPI_CFG_MODE__ _QSPI_MODE_DMA

#endif /* _QSPI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
