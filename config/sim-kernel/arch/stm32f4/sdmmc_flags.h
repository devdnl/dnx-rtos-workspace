/*==============================================================================
File    sdmmc_flags.h

Author  Daniel Zorychta

Brief   SD/MMC driver

        MIT License

        Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#define __SDMMC_CFG_SDMMC1_TYPE__ _SDMMC_TYPE_BOTH

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

#endif /* _SDMMC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
