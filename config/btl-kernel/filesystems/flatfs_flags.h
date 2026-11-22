/*==============================================================================
File    flatfs_flags.h

Author  daniel.zorychta@gmail.com

Brief   Daniel Zorychta

        Copyright (C) 2021 daniel.zorychta@gmail.com <>

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

#ifndef _FLATFS_FLAGS_H_
#define _FLATFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > flatfs", "The flatfs file system configuration.")
++*/

/*--
this:AddWidget("Combobox", "Enable file system debug")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_LOG_ENABLE__ _NO_

/*--
this:AddWidget("Spinbox", 0, 512, "Number of index entries")
--*/
#define __FLATFS_INDEX_SIZE__ 32

/*--
this:AddWidget("Combobox", "Enable option async (deferred update of file size)")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_ASYNC_ENABLE__ _NO_

/*--
this:AddWidget("Combobox", "Enable option cache=N (write-through block cache)")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_CACHE_ENABLE__ _NO_

#endif /* _FLATFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
