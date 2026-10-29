#/*=============================================================================
# @file    cpu_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains CPU configuration flags.
#          Hybrid file: included both by Make and CC.
#
# @note    Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
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

#ifndef _CPU_FLAGS_H_
#define _CPU_FLAGS_H_

#/*--
# this:AddWidget("Value")
#--*/
#define __CPU_FAMILY__ _POSIX_SIM_

#/*--
# this:AddWidget("Value")
#--*/
#define __CPU_NAME__ POSIX
#/*
__CPU_NAME__=POSIX
#*/

#/*--
# this:AddWidget("Spinbox", 64, 1048576, "Default RAM size [KiB]")
# this:SetToolTip("Size of RAM region used as kernel heap. Can be changed by the --ram program option.")
#--*/
#define __CPU_RAM_SIZE_KIB__ 1024

#/*--
# this:AddWidget("Editline", false, "Preffered application RAM")
# this:SetToolTip('Default: "SRAM"')
#--*/
#define __CPU_PREFFERED_APP_RAM__ "SRAM"
#/*
__CPU_PREFFERED_APP_RAM__="SRAM"
#*/

#/*--
# this:AddWidget("Editline", false, "Preffered file systems RAM")
# this:SetToolTip('Default: "SRAM"')
#--*/
#define __CPU_PREFFERED_FS_RAM__ "SRAM"
#/*
__CPU_PREFFERED_FS_RAM__="SRAM"
#*/

#/*--
# this:AddWidget("Editline", false, "Preffered fast access RAM")
# this:SetToolTip('Default: "SRAM"')
#--*/
#define __CPU_PREFFERED_FAST_RAM__ "SRAM"
#/*
__CPU_PREFFERED_FAST_RAM__="SRAM"
#*/

#/*--
# this:AddWidget("Editline", true, "Extra linker flags")
# this:SetToolTip("Additional flags passed to the linker. The default i686-unknown-linux-musl\n"..
#                 "toolchain creates static executable. Remove -static flag if host gcc\n"..
#                 "(toolchain prefix $(none)) with 32-bit libraries is used.")
#--*/
#define __CPU_EXTRA_LDFLAGS__ -static
#/*
__CPU_EXTRA_LDFLAGS__=-static
#*/

#//-----------------------------------------------------------------------------
#// mandatory flags, not configurable
#//-----------------------------------------------------------------------------
#define _CPU_START_FREQUENCY_           (1000000000UL)
#define _CPU_HEAP_ALIGN_                (4)
#define _CPU_IRQ_RTOS_KERNEL_PRIORITY_  (0)
#define _CPU_IRQ_RTOS_APICALL_PRIORITY_ (0)
#define _CPU_IRQ_SAFE_PRIORITY_         (0)
#define __CPU_DEFAULT_IRQ_PRIORITY__    (0)
#define ARCH_posix
#/*
CPUCONFIG_AFLAGS=-m32 -fno-pie
CPUCONFIG_CFLAGS=-m32 -fno-pie -fno-strict-aliasing -funsigned-char
CPUCONFIG_CXXFLAGS=-m32 -fno-pie -fno-strict-aliasing -funsigned-char
CPUCONFIG_LDFLAGS=-m32 -no-pie $(__CPU_EXTRA_LDFLAGS__)
CPUCONFIG_HOST_LIBS=-lpthread -lm
#*/

#// current CPU family definitions
#define _POSIX_SIM_ 0x2b5e7f10
#if __CPU_FAMILY__ != _POSIX_SIM_
#error Wrong CPU family
#endif

#endif /* _CPU_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
