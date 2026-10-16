/*==============================================================================
File    sdmmc_flags.h

Author  Daniel Zorychta

Brief   SD/MMC driver

        Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _SDMMC_FLAGS_H_
#define _SDMMC_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > SDMMC", "The SDMMC module configuration.")
++*/

/*--
this:AddWidget("Combobox", "SDMMC1 Device type")
this:AddItem("SD", "_SDMMC_TYPE_SD")
this:AddItem("MMC", "_SDMMC_TYPE_MMC")
this:AddItem("MMC/SD", "_SDMMC_TYPE_BOTH")
--*/
#define __SDMMC_CFG_SDMMC1_TYPE__ _SDMMC_TYPE_SD

/*--
this:AddWidget("Combobox", "SDMMC1 Driver mode")
this:AddItem("Polling", "_SDMMC_MODE_POLLING")
this:AddItem("IRQ", "_SDMMC_MODE_IRQ")
this:AddItem("DMA", "_SDMMC_MODE_DMA")
--*/
#define __SDMMC_CFG_SDMMC1_MODE__ _SDMMC_MODE_DMA

/*--
this:AddWidget("Combobox", "SDMMC1 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __SDMMC_CFG_SDMMC1_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__



/*--
this:AddExtraWidget("Label", "SD2_GAP", "")
this:AddWidget("Combobox", "SDMMC2 Device type")
this:AddItem("SD", "_SDMMC_TYPE_SD")
this:AddItem("MMC", "_SDMMC_TYPE_MMC")
this:AddItem("MMC/SD", "_SDMMC_TYPE_BOTH")
--*/
#define __SDMMC_CFG_SDMMC2_TYPE__ _SDMMC_TYPE_SD

/*--
this:AddWidget("Combobox", "SDMMC2 Driver mode")
this:AddItem("Polling", "_SDMMC_MODE_POLLING")
this:AddItem("IRQ", "_SDMMC_MODE_IRQ")
this:AddItem("DMA", "_SDMMC_MODE_DMA")
--*/
#define __SDMMC_CFG_SDMMC2_MODE__ _SDMMC_MODE_DMA

/*--
this:AddWidget("Combobox", "SDMMC2 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __SDMMC_CFG_SDMMC2_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

#endif /* _SDMMC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
