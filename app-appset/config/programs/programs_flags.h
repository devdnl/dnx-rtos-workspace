#/*=============================================================================
# @file    programs_flags.h
#
# @author  Daniel Zorychta
#
# @brief   Programs flags.
#
# @note    Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          This program is free software; you can redistribute it and/or modify
#          it under the terms of the GNU General Public License as published by
#          the Free Software Foundation and modified by the dnx RTOS exception.
#
#          NOTE: The modification  to the GPL is  included to allow you to
#                distribute a combined work that includes dnx RTOS without
#                being obliged to provide the source  code for proprietary
#                components outside of the dnx RTOS.
#
#          The dnx RTOS  is  distributed  in the hope  that  it will be useful,
#          but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
#          MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
#          GNU General Public License for more details.
#
#          Full license text is available on the following file: doc/license.txt.
#
#
#=============================================================================*/

#/*
#* NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
#*       and prefix) are exported to the single configuration file
#*       (by using Configtool) when entire project configuration is exported.
#*       All other flag definitions and statements are ignored.
#*/


#ifndef _PROGRAMS_FLAGS_H_
#define _PROGRAMS_FLAGS_H_

#/*--
# this:SetLayout("TitledGridBack", 2, "Home > Programs",
#                function() this:LoadFile("config.h") end)
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelBasic1", "Basic", -1, "bold")
# this:AddExtraWidget("Label", "LabelBasic2", "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", -1, "bold")
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelProgs1", "", -1, "bold")
# this:AddExtraWidget("Label", "LabelProgs2", "List of programs to be added to the compilation. Use * to add all programs.")
#++*/

#/*--
# this:AddWidget("Editline", true, "Programs")
# this:SetToolTip("Select programs that be added to the project.")
#--*/
#define __PROGRAMS_LIST__ "*"
#/*
__PROGRAMS_LIST__="*"
#*/

#endif /* _PROGRAMS_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
