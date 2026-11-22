/*==============================================================================
@file    uds_flags.h

@author  Daniel Zorychta

@brief   This file contains configuration of Unix Domain Socket stack.
         Hybrid file: included both by Make and CC.

@note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _UDS_FLAGS_H_
#define _UDS_FLAGS_H_

/*--
this:SetTitle("Home > Network > UDS Stack", "The Unix Domanin Socket Stack configuration.")
++*/

/*--
this:AddWidget("Combobox", "Debug messages")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
--*/
#define __NETWORK_UDS_DEBUG_ON__ _NO_

/*--
this:AddWidget("Spinbox", 128, 65536, "Stream receive buffer size [B]")
--*/
#define __NETWORK_UDS_BUF_SIZE__ 2048

/*--
this:AddWidget("Spinbox", 16, 65536, "Datagram max receive buffer size [B]")
--*/
#define __NETWORK_UDS_DATAGRAM_MAX_SIZE__ 1500

/*--
this:AddWidget("Spinbox", 1, 128, "Max number of datagrams in buffer")
--*/
#define __NETWORK_UDS_DATAGRAM_BUFFERS_SIZE__ 16

#endif /* _UDS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
