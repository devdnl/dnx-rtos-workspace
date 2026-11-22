/*==============================================================================
@file    ramfs_flags.h

@author  Daniel Zorychta

@brief   RAM file system configuration.

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

#ifndef _RAMFS_FLAGS_H_
#define _RAMFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > ramfs", "The ramfs file system configuration.")
++*/

/*--
this:AddWidget("Spinbox", 8, 4096, "File chain size (bytes)")
--*/
#define __RAMFS_FILE_CHAIN_SIZE__ 64

#endif /* _RAMFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
