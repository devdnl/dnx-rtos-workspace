#/*=============================================================================
# @file    config.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains configuration.
#          Hybrid file: included both by Make and CC.
#
# @note    Copyright (C) 2015 Daniel Zorychta <daniel.zorychta@gmail.com>
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


#ifndef _CONFIG_CONFIG_H_
#define _CONFIG_CONFIG_H_

#// static definitions
#define _NO_  0
#define _YES_ 1
#/*
_NO_  = 0
_YES_ = 1
#*/

#// include all configuration flags
#include "./project/project_flags.h"
#include "./libc/libc_flags.h"
#include "./programs/programs_flags.h"
#/* include all configuration flags in Makefile
include $(CONFIG_DIR)/project/project_flags.h
include $(CONFIG_DIR)/programs/programs_flags.h
#*/

#/*--
# this:SetKeyRegExp("__.*__")
# this:SetLayout("TitledGridBack", 2, "Home")
#++*/

#/*--
# -- import event used to disables all modules
# -- this flags must exists in the arch/arch_flags.h file
# this:SetEvent("ConfigImport",
#     function()
#
#         modules = this:GetFlagsList("arch/arch_flags.h", "__ENABLE_.*__")
#
#         for f, v in pairs(modules) do
#             this:SetFlagValue(f, "_NO_", "arch/arch_flags.h")
#         end
#     end)
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelProject", "Project", -1, "bold")
# this:AddExtraWidget("Hyperlink", "HL_Project", "Configure")
# this:SetEvent("clicked", "HL_Project", function() this:LoadFile("project/project_flags.h") end)
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelOS", "Standard Library  ", -1, "bold")
# this:AddExtraWidget("Hyperlink", "HL_LIBC", "Configure")
# this:SetEvent("clicked", "HL_LIBC", function() this:LoadFile("libc/libc_flags.h") end)
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelProgs", "Programs  ", -1, "bold")
# this:AddExtraWidget("Hyperlink", "HL_PROGS", "Configure")
# this:SetEvent("clicked", "HL_PROGS", function() this:LoadFile("programs/programs_flags.h") end)
#++*/

#endif /* _CONFIG_CONFIG_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
