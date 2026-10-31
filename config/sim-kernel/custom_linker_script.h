/*==============================================================================
@file    custom_linker_script.h

@author  Daniel Zorychta

@brief   Custom linker script configuration.

@note    Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*
 * NOTE: Purpose of this file is to generate custom linker script instead of
 *       including that file into the project.
 */

#define __ARCH__ stm32f1
#define __MEMORY_1__ "rom (rx)  : org = 0x08000000, len = 128k"
#define __MEMORY_2__ "ram (rwx) : org = 0x20000000, len = 20k"
#define __MEMORY_3__ ""
#define __MEMORY_4__ ""
#define __MEMORY_5__ ""
#define __MEMORY_6__ ""
#define __MEMORY_7__ ""
#define __MEMORY_8__ ""
#define __MEMORY_9__ ""
#define __MEMORY_10__ ""
#define __MEMORY_11__ ""
#define __MEMORY_12__ ""
#define __MEMORY_13__ ""
#define __MEMORY_14__ ""
#define __MEMORY_15__ ""
#define __MEMORY_16__ ""

/*==============================================================================
  End of file
==============================================================================*/
