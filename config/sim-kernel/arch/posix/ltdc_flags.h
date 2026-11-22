/*==============================================================================
@file    ltdc_flags.h

@author  Daniel Zorychta

@brief   LTDC module configuration flags (POSIX simulator).

@note    Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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
