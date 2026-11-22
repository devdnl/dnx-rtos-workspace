/*==============================================================================
File    usbh_flags.h

Author  Daniel Zorychta

Brief   USB Host driver

        Copyright (C) 2019  <daniel.zorychta@gmail.com>

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License version 2 as
        published by the Free Software Foundation.

        The dnx RTOS is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
        GNU General Public License for more details.

        Programs using the kernel by system calls are not covered by the license,
        drivers and file systems of drvset partition: exception, see the license
        file: kernel/doc/license.txt.

==============================================================================*/

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as prefix
 *       and suffix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _USBH_FLAGS_H_
#define _USBH_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > USBH", "The USBH module configuration.")
++*/

/*--
this:AddWidget("Editline", false, "PHY reset pin")
--*/
#define __USBH_PHY_RESET_PIN__ NONE

/*--
this:AddWidget("Combobox", "Debug messages")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __USBH_DEBUG_MSG__ _NO_

/*--
this:AddWidget("Combobox", "Enable DMA transfers")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __USBH_ENABLE_DMA_TRANSFER__ _YES_

/*--
this:AddWidget("Combobox", "IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __USBH_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

#endif /* _USBH_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
