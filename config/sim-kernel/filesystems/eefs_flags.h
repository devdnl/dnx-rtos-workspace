/*==============================================================================
@file    eefs_flags.h

@author  Daniel Zorychta

@brief   EEFS file system configuration.

@note    MIT License

         Copyright (C) 2017 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _EEFS_FLAGS_H_
#define _EEFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > eefs", "The eefs file system configuartion.")
++*/

/*--
this:AddWidget("Combobox", "Enable file system debug")
this:AddItem("Enable", "_YES_")
this:AddItem("Disable", "_NO_")
--*/
#define __EEFS_LOG_ENABLE__ _NO_

/*--
this:AddWidget("Combobox", "Enable file system check")
this:AddItem("Enable", "_YES_")
this:AddItem("Disable", "_NO_")
--*/
#define __EEFS_CHECK_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 0, 512, "Cache size (128 bytes blocks)")
--*/
#define __EEFS_CACHE_SIZE__ 8

#endif /* _EEFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
