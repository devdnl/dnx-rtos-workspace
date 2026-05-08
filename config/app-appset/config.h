#/*=============================================================================
# @file   config.h
#
# @author Daniel Zorychta
#
# @brief  This file contains configuration.
#         Hybrid file: included both by Make and CC.
#
#         MIT License
#
#         Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#         Permission is hereby granted, free of charge, to any person obtaining
#         a copy of this software and associated documentation files (the
#         "Software"), to deal in the Software without restriction, including
#         without limitation the rights to use, copy, modify, merge, publish,
#         distribute, sublicense, and/or sell copies of the Software, and to
#         permit persons to whom the Software is furnished to do so, subject to
#         the following conditions:
#
#         The above copyright notice and this permission notice shall be included
#         in all copies or substantial portions of the Software.
#
#         THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#         OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#         FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#         THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#         OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#         ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#         OTHER DEALINGS IN THE SOFTWARE.
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
# this:SetTitle("Home", "Appset configuration menu.")
#
# this:AddExtraWidget("Submenu", "HL_Project", "Project", "project/project_flags.h")
# this:AddExtraWidget("Submenu", "HL_LIBC", "Standard Library", "libc/libc_flags.h")
# this:AddExtraWidget("Submenu", "HL_PROGS", "Programs", "programs/programs_flags.h")
#++*/

#endif /* _CONFIG_CONFIG_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
