/*==============================================================================
@file    ltdc_flags.h

@author  Daniel Zorychta

@brief   LTDC module configuration flags (POSIX simulator).

@note    MIT License

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

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
 *       and prefix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _LTDC_FLAGS_H_
#define _LTDC_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > LTDC", "The LTDC module configuration.")
++*/

/*--
this:AddExtraWidget("Label", "LTDC_POSIX_INFO", "The display is shown by the board panel in a web browser (simulator option: --panel <socket>, tools/simpanel.py).")
--*/

/*--
this:AddWidget("Spinbox", 1, 4096, "Active width")
--*/
#define __LTDC_WIDTH__ 480

/*--
this:AddWidget("Spinbox", 1, 2048, "Active height")
--*/
#define __LTDC_HEIGHT__ 272

#endif /* _LTDC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
