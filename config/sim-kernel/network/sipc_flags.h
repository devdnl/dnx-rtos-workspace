/*==============================================================================
@file    sipc_flags.h

@author  Daniel Zorychta

@brief   This file contains configuration of SIPC stack.
         Hybrid file: included both by Make and CC.

@note    Copyright (C) 2019 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _SIPC_FLAGS_H_
#define _SIPC_FLAGS_H_

/*--
this:SetTitle("Home > Network > SIPC Stack", "The SIPC stack configuration.")
++*/

/*--
this:AddWidget("Combobox", "Debug messages")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
--*/
#define __NETWORK_SIPC_DEBUG_ON__ _NO_

/*--
this:AddWidget("Spinbox", 128, 65536, "Receive buffer size [B]")
--*/
#define __NETWORK_SIPC_RECV_BUF_SIZE__ 2048

#endif /* _SIPC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
