/*==============================================================================
@file    eth_flags.h

@author  Daniel Zorychta

@brief   ETH module configuration flags (POSIX simulator).

@note    Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


==============================================================================*/

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
 *       and prefix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _ETH_FLAGS_H_
#define _ETH_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > ETH", "The ETH module configuration.")
++*/

/*--
this:AddExtraWidget("Label", "ETH_POSIX_INFO", "Ethernet frames are transferred by the host TAP interface (simulator option: --tap <name>).")
--*/

#endif /* _ETH_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
