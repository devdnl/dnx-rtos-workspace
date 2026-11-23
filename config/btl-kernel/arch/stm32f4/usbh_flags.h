/*==============================================================================
File    usbh_flags.h

Author  Daniel Zorychta

Brief   USB Host driver

        MIT License

        Copyright (C) 2019  <daniel.zorychta@gmail.com>

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
