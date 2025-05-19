#/*=============================================================================
# @file    config.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains configuration.
#          Hybrid file: included both by Make and CC.
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

#ifndef _ROMFS_CONFIG_H_
#define _ROMFS_CONFIG_H_

#// static definitions
#define _NO_  0
#define _YES_ 1
#/*
_NO_  = 0
_YES_ = 1
#*/

#/*--
# this:SetKeyRegExp("__.*__")
# this:SetLayout("TitledGridBack", 2, "Home")
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelROMFS", "\nROMFS configuration", -1, "bold")
# this:AddExtraWidget("Label", "LabelADDR", "\t\t\t\t\t\t\t\t\t\t\t")
#++*/

#/*--
# this:AddWidget("Editline", true, "ROMFS name")
# this:SetToolTip("ROMFS name.")
#--*/
#define __ROMFS_NAME__ "romfs"
#/*
__ROMFS_NAME__="romfs"
#*/

#/*--
# this:AddWidget("Editline", false, "ROMFS offset address")
# this:SetToolTip("Start address of ROMFS image in hex file.")
#--*/
#define __ROMFS_ADDRESS__ 0x080C0000
#/*
__ROMFS_ADDRESS__=0x080C0000
#*/

#endif /* _ROMFS_CONFIG_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
