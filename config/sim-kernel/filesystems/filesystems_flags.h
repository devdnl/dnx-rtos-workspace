#/*=============================================================================
# @file    filesystems_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file enable selected file systems.
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

#ifndef _FILE_SYSTEMS_FLAGS_H_
#define _FILE_SYSTEMS_FLAGS_H_

#/*--
# this:SetTitle("Home > File Systems", "File systems selection menu.")
#++*/

#/*--
# this:AddWidget("Combobox", "ramfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The ramfs is a general purpose RAM file system, that can be\n"..
#                 "used to store files, device-files, and pipes. The file system\n"..
#                 "is the best choice if you want to use only one file system for\n"..
#                 "each file operations.")
# this:AddExtraWidget("Submenu", "RAMFS_CONFIGURE", "ramfs: configure", "filesystems/ramfs_flags.h")
#--*/
#include "../filesystems/ramfs_flags.h"
#define __ENABLE_RAMFS__ _YES_
#/*
__ENABLE_RAMFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "procfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The procfs is a special file system, that provides special\n"..
#                 "functionality; by using this file system you can see all\n"..
#                 "tasks and their names and so on. In this file system are\n"..
#                 "stored special system files, that can be read to obtain system\n"..
#                 "specified settings, or microcontroller information. If you do\n"..
#                 "not need to read special information, then probably you do not\n"..
#                 "need this file system.")
#--*/
#define __ENABLE_PROCFS__ _YES_
#/*
__ENABLE_PROCFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "fatfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("If you want to use FAT12, FAT16, and FAT32 in your system\n"..
#                 "enable this file system. The fatfs read/write data from/to\n"..
#                 "device-file e.g. SD cards and other bigger volumes. This file\n"..
#                 "system does not support special files.")
# this:AddExtraWidget("Submenu", "FATFS_CONFIGURE", "fatfs: configure", "filesystems/fatfs_flags.h")
#--*/
#include "../filesystems/fatfs_flags.h"
#define __ENABLE_FATFS__ _YES_
#/*
__ENABLE_FATFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "eefs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The eefs is small file system that support very small EEPROM memories.\n"..
#                 "Typical size of EEPROM memory connected to I2C bus is 1-64 KiB thus\n"..
#                 "requires very small file system.")
# this:AddExtraWidget("Submenu", "EEFS_CONFIGURE", "eefs: configure", "filesystems/eefs_flags.h")
#--*/
#include "../filesystems/eefs_flags.h"
#define __ENABLE_EEFS__ _YES_
#/*
__ENABLE_EEFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "ext4fs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The EXT4FS is a file system that support Linux file systems\n "..
#                 "like EXT2, EXT3, and EXT4.")
# this:SetToolTip("EXT2,3,4 file systems.")
# this:AddExtraWidget("Submenu", "EXT4FS_CONFIGURE", "ext4fs: configure", "filesystems/ext4fs_flags.h")
#--*/
#include "../filesystems/ext4fs_flags.h"
#define __ENABLE_EXT4FS__ _YES_
#/*
__ENABLE_EXT4FS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "appsetfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The appsetfs presents programs of an application set (appset)\n"..
#                 "as executable files: mount(\"appsetfs\", \"/dev/appset\", \"/opt/progs\", \"\")\n"..
#                 "(memblock device with appset image executed in place) or\n"..
#                 "mount(\"appsetfs\", \"\", \"/bin\", \"\") (main appset, shell: mount -\n"..
#                 "appsetfs /bin). Programs are started by path or found in the\n"..
#                 "program search path.")
#--*/
#define __ENABLE_APPSETFS__ _YES_
#/*
__ENABLE_APPSETFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "romfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("ROM File System is a file system that is read-only.")
# this:AddExtraWidget("Submenu", "ROMFS_CONFIGURE", "romfs: configure", "filesystems/romfs_flags.h")
#--*/
#include "../filesystems/romfs_flags.h"
#define __ENABLE_ROMFS__ _YES_
#/*
__ENABLE_ROMFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "flatfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("FLAT File System")
# this:AddExtraWidget("Submenu", "FLATFS_CONFIGURE", "flatfs: configure", "filesystems/flatfs_flags.h")
#--*/
#include "../filesystems/flatfs_flags.h"
#define __ENABLE_FLATFS__ _YES_
#/*
__ENABLE_FLATFS__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "hostfs: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The hostfs gives access to the host directory selected by\n"..
#                 "the simulator --hostfs option (POSIX simulator only).")
#--*/
#define __ENABLE_HOSTFS__ _YES_
#/*
__ENABLE_HOSTFS__=_YES_
#*/

#endif /* _FILE_SYSTEMS_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
