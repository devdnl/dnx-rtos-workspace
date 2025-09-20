/*==============================================================================
@file   libc_flags.h

@author Daniel Zorychta

@brief  LIBC configuration flags

        MIT License

        Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _LIBC_FLAGS_H_
#define _LIBC_FLAGS_H_

/*--
# this:SetLayout("TitledGridBack", 2, "Home > Standard library",
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
#define __LIBC_IOCTL_REQUEST_FILE__ "../../build/ioctl/ioctl_requests.h"

#endif /* _LIBC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
