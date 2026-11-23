#/*=============================================================================
# @file    config.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains configuration.
#          Hybrid file: included both by Make and CC.
#
# @note    MIT License
#
#          Copyright (C) 2015 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          Permission is hereby granted, free of charge, to any person obtaining
#          a copy of this software and associated documentation files (the
#          "Software"), to deal in the Software without restriction, including
#          without limitation the rights to use, copy, modify, merge, publish,
#          distribute, sublicense, and/or sell copies of the Software, and to
#          permit persons to whom the Software is furnished to do so, subject to
#          the following conditions:
#
#          The above copyright notice and this permission notice shall be included
#          in all copies or substantial portions of the Software.
#
#          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#          OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#          FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#          THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#          OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#          ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#          OTHER DEALINGS IN THE SOFTWARE.
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

#define _CONFIG_VERSION 20221030

#// static definitions
#define _NO_  0
#define _YES_ 1
#/*
_NO_  = 0
_YES_ = 1
#*/

#// include all configuration flags
#include "./project/project_flags.h"
#include "./arch/arch_flags.h"
#include "./os/os_flags.h"
#include "./filesystems/filesystems_flags.h"
#include "./network/network_flags.h"
#/* include all configuration flags in Makefile
include $(CONFIG_DIR)/project/project_flags.h
include $(CONFIG_DIR)/arch/arch_flags.h
include $(CONFIG_DIR)/filesystems/filesystems_flags.h
include $(CONFIG_DIR)/network/network_flags.h
#*/

#/*--
# this:SetKeyRegExp("__.*__")
# this:SetTitle("Home", "dnx RTOS configuration menu.")
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
# this:AddExtraWidget("Submenu", "HL_Project", "Project", "project/project_flags.h")
# this:AddExtraWidget("Submenu", "HL_OS", "Operating System", "os/os_flags.h")
# this:AddExtraWidget("Submenu", "HL_FS", "File Systems", "filesystems/filesystems_flags.h")
# this:AddExtraWidget("Submenu", "HL_Net", "Network", "network/network_flags.h")
# this:AddExtraWidget("Submenu", "HL_Arch", "Microcontroller", "arch/arch_flags.h")
#++*/

#endif /* _CONFIG_CONFIG_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
