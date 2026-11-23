/*==============================================================================
@file    usbd_flags.h

@author  Daniel Zorychta

@brief   USB-Device module configuration flags.

@note    MIT License

         Copyright (C) 2014 Daniel Zorychta <daniel.zorychta@gmail.com>

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
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
 *       and prefix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _USBD_FLAGS_H_
#define _USBD_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > USBD", "The USBD module configuration.")
++*/

/*--
this:AddWidget("Combobox", "Pullup pin")
for i = 0, 6 do
    local c = string.char(65 + i)
    this:AddItem("P"..c, tostring(i))
end
--*/
#define __USBD_PULLUP_PORT_INDEX__ 0

/*--
this:AddWidget("Combobox")
for i = 0, 15 do
    this:AddItem(tostring(i), tostring(i))
end
--*/
#define __USBD_PULLUP_PIN_INDEX__ 0

/*--
this:AddWidget("Checkbox", "Active low")
this:SetToolTip("Pull-up pin activated by low level state.")
--*/
#define __USBD_PULLUP_NEGATIVE__ _YES_

/*--
this:AddWidget("Combobox", "Endpoint 0 size")
this:AddItem("8",  "8")
this:AddItem("16", "16")
this:AddItem("32", "32")
this:AddItem("64", "64")
this:AddExtraWidget("Void", "VoidEP0SZ1")
this:AddExtraWidget("Void", "VoidEP0SZ2")
--*/
#define __USBD_ENDPOINT0_SIZE__ 8

#endif /* _USBD_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
