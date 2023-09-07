/*=========================================================================*//**
@file    libc_flags.h

@author  Daniel Zorychta

@brief   LIBC configuration flags

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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


*//*==========================================================================*/

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
 *       and prefix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _LIBC_FLAGS_H_
#define _LIBC_FLAGS_H_

/*--
# this:SetLayout("TitledGridBack", 2, "Home > Operating System",
#                function() this:LoadFile("config.h") end)
++*/


/*--
this:AddExtraWidget("Label", "LabelFeatures", "\nFeatures", -1, "bold")
this:AddExtraWidget("Label", "LabelFeatures2", "\t\t\t\t\t\t", -1, "bold")
++*/

/*--
this:AddWidget("Checkbox", "Color terminal")
this:AddExtraWidget("Void", "VoidFeaturesDummy")
this:SetToolTip("If this function is selected then terminal output can be colorized by using VT100 commands.")
--*/
#define __LIBC_COLOR_TERMINAL_ENABLE__ _YES_


/*--
this:AddExtraWidget("Label", "LabelMisc", "\nMiscellaneous", -1, "bold")
this:AddExtraWidget("Void", "VoidMisc")
++*/

/*--
this:AddWidget("Editline", true, "The system() shell program")
this:SetToolTip("This is a program that execute command passed to system() function.")
--*/
#define __LIBC_SYSTEM_PROG__ "dsh -e"

/*--
this:AddWidget("Editline", false, "The ioctl() requests definition file")
this:SetToolTip("File with ioctl() function requests definitions.")
--*/
#define __LIBC_IOCTL_REQUEST_FILE__ <libc/include/machine/ioctl/ioctl_requests.h>

#endif /* _LIBC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
