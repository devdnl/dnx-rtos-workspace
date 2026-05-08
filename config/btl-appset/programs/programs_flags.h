#/*=============================================================================
# @file   programs_flags.h
#
# @author Daniel Zorychta
#
# @brief  Programs flags.
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


#ifndef _PROGRAMS_FLAGS_H_
#define _PROGRAMS_FLAGS_H_

#/*--
# this:SetTitle("Home > Programs", "List of programs to be included in the compilation. In the 'Add' field, use '*' to add all programs; otherwise, enter each program name separated by a space.\nThe 'Exclude' field removes selected programs from compilation; enter each program name separated by a space.")
#++*/

#/*--
# this:AddWidget("Editline", true, "Add")
# this:SetToolTip("List of programs to be added to the compilation. Use * to add all programs.")
#--*/
#define __PROGRAMS_LIST__ "*"
#/*
__PROGRAMS_LIST__="*"
#*/

#/*--
# this:AddWidget("Editline", true, "Exclude")
# this:SetToolTip("Select programs that be excluded from the project.")
#--*/
#define __PROGRAMS_EXCLUDE_LIST__ ""
#/*
__PROGRAMS_EXCLUDE_LIST__=""
#*/

#endif /* _PROGRAMS_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
