/*==============================================================================
File    ltdc_flags.h

Author  Daniel Zorychta

Brief   LCD-TFT Display Controller module

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

#ifndef _LTDC_FLAGS_H_
#define _LTDC_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > LTDC", "The LTDC module configuration.")
++*/
/*--
this:AddWidget("Spinbox", 1, 4096, "Active width")
--*/
#define __LTDC_WIDTH__ 480

/*--
this:AddWidget("Spinbox", 1, 2048, "Active height")
--*/
#define __LTDC_HEIGHT__ 272

/*--
this:AddWidget("Spinbox", 1, 4096, "HSYNC width")
--*/
#define __LTDC_HSYNC__ 41

/*--
this:AddWidget("Spinbox", 1, 1024, "Horizontal back porch")
--*/
#define __LTDC_HBP__ 13

/*--
this:AddWidget("Spinbox", 1, 1024, "Horizontal front porch")
--*/
#define __LTDC_HFP__ 32

/*--
this:AddWidget("Spinbox", 1, 2048, "VSYNC width")
--*/
#define __LTDC_VSYNC__ 10

/*--
this:AddWidget("Spinbox", 1, 1024, "Vertical back porch")
--*/
#define __LTDC_VBP__ 2

/*--
this:AddWidget("Spinbox", 1, 1024, "Vertical front porch")
--*/
#define __LTDC_VFP__ 2

/*--
this:AddWidget("Combobox", "HSYNC polarity")
this:AddItem("Active low", "LTDC_HSPOLARITY_AL")
this:AddItem("Active high", "LTDC_HSPOLARITY_AH")
--*/
#define __LTDC_HSPOLARITY__ LTDC_HSPOLARITY_AL

/*--
this:AddWidget("Combobox", "VSYNC polarity")
this:AddItem("Active low", "LTDC_VSPOLARITY_AL")
this:AddItem("Active high", "LTDC_VSPOLARITY_AH")
--*/
#define __LTDC_VSPOLARITY__ LTDC_VSPOLARITY_AL

/*--
this:AddWidget("Combobox", "DE polarity")
this:AddItem("Active low", "LTDC_DEPOLARITY_AL")
this:AddItem("Active high", "LTDC_DEPOLARITY_AH")
--*/
#define __LTDC_DEPOLARITY__ LTDC_DEPOLARITY_AL

/*--
this:AddWidget("Combobox", "PCLK polarity")
this:AddItem("Input pixel clock", "LTDC_PCPOLARITY_IPC")
this:AddItem("Inverted input pixel clock", "LTDC_PCPOLARITY_IIPC")
--*/
#define __LTDC_PCPOLARITY__ LTDC_PCPOLARITY_IPC

#endif /* _LTDC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
