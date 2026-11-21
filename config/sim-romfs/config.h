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
# this:SetTitle("Home", "The romfs configuration.")
#++*/

#/*--
# this:AddWidget("Editline", true, "ROMFS name")
# this:SetToolTip("ROMFS name. That variable is changing output file name.")
#--*/
#define __ROMFS_NAME__ "romfs"
#/*
__ROMFS_NAME__="romfs"
#*/

#/*--
# this:AddWidget("Editline", false, "ROMFS offset address")
# this:SetToolTip("Start address of ROMFS image in hex file.")
#--*/
#define __ROMFS_ADDRESS__ 0x08080000
#/*
__ROMFS_ADDRESS__=0x08080000
#*/

#/*--
# this:AddWidget("Editline", false, "ROMFS partition size")
# this:SetToolTip("Size of ROMFS partition in bytes (e.g. 0x20000). The build fails when the image is "..
#                 "bigger. 0: not checked. The partition is given to the MEMBLOCK driver in initd, "..
#                 "romfs finds the image by its header and df shows free space of the partition.")
#--*/
#define __ROMFS_SIZE__ 0x80000
#/*
__ROMFS_SIZE__=0x80000
#*/

#/*--
# this:AddWidget("Editline", false, "Owner of files (user ID)")
# this:SetToolTip("Default owner (user ID) of files of the image (0: root).")
#--*/
#define __ROMFS_UID__ 0
#/*
__ROMFS_UID__=0
#*/

#/*--
# this:AddWidget("Editline", false, "Group of files (group ID)")
# this:SetToolTip("Default group ID of files of the image (0: root).")
#--*/
#define __ROMFS_GID__ 0
#/*
__ROMFS_GID__=0
#*/

#/*--
# this:AddWidget("Editline", false, "Mode of files")
# this:SetToolTip("Default permissions of files (octal), e.g. 0444: all users read files.")
#--*/
#define __ROMFS_FILE_MODE__ 0444
#/*
__ROMFS_FILE_MODE__=0444
#*/

#/*--
# this:AddWidget("Editline", false, "Mode of directories")
# this:SetToolTip("Permissions of directories (octal), e.g. 0555. Directories are owned by root.")
#--*/
#define __ROMFS_DIR_MODE__ 0555
#/*
__ROMFS_DIR_MODE__=0555
#*/

#/*--
# this:AddWidget("Editline", true, "Permissions of selected files")
# this:SetToolTip("Permissions of selected files of the image, entries separated by spaces:\n"..
#                 "  path:mode[:uid[:gid]]\n"..
#                 "path: absolute path in the image (/shadow is /etc/shadow when the image is mounted in "..
#                 "/etc), characters encoded as in URL: %20 space, %3A colon, %25 percent;\n"..
#                 "mode: octal permissions, e.g. 0600;\n"..
#                 "uid, gid: owner and group (default: user ID and group ID of files).\n"..
#                 "Example: \"/shadow:0600 /app.key:0640:0:10 /my%20data/key.pem:0600:1000:1000\"")
#--*/
#define __ROMFS_PERMISSIONS__ "/shadow:0600 /README:0640:0:10"
#/*
__ROMFS_PERMISSIONS__="/shadow:0600 /README:0640:0:10"
#*/

#endif /* _ROMFS_CONFIG_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
