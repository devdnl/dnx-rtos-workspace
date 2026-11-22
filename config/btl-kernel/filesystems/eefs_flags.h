/*==============================================================================
@file    eefs_flags.h

@author  Daniel Zorychta

@brief   EEFS file system configuration.

@note    Copyright (C) 2017 Daniel Zorychta <daniel.zorychta@gmail.com>

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
