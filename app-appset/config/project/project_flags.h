#/*=============================================================================
# @file    project_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains base project configuration.
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


#ifndef _PROJECT_FLAGS_H_
#define _PROJECT_FLAGS_H_

#/*--
# this:SetLayout("TitledGridBack", 2, "Home > Project",
#                function() this:LoadFile("config.h") end)
#++*/

#/*--
# this:AddExtraWidget("Label", "LabelBasic", "Basic", -1, "bold")
# this:AddExtraWidget("Void", "VoidBasic")
#++*/

#/*--
# this:AddWidget("Editline", false, "Project name")
# this:SetToolTip("This is a name of your project. The string is used to generate names of output files.")
#--*/
#define __PROJECT_NAME__ appset
#/*
__PROJECT_NAME__=appset
#*/

#/*--
# this:AddWidget("Editline", false, "Toolchain prefix")
# this:SetToolTip("Enter a name of first part of your toolchain's name, example:\n"..
#                 " - Linaro, CodeSourcery: arm-none-eabi-\n"..
#                 " - GCC: $(none)")
#--*/
#define __PROJECT_TOOLCHAIN__ arm-none-eabi-
#/*
none=
__PROJECT_TOOLCHAIN__=arm-none-eabi-
#*/


#/*--
# this:AddExtraWidget("Label", "LabelBoard", "\nBoard", -1, "bold")
# this:AddExtraWidget("Void", "VoidBoard")
#++*/
#// Existing architectures
#define stm32f1 0x16F54349
#define stm32f2 0x2f31e2fc
#define stm32f3 0x483962d4
#define stm32f4 0x8dd787b6
#define stm32f7 0x3cb2ba7c
#define stm32h7 0x56A5cde6
#define rpcm0   0x60b36426
#/*--
# this:AddWidget("Combobox", "CPU architecture")
# this:AddItem("STMicroelectronics STM32F1", "stm32f1")
# this:AddItem("STMicroelectronics STM32F3", "stm32f3")
# this:AddItem("STMicroelectronics STM32F4", "stm32f4")
# this:AddItem("STMicroelectronics STM32F7", "stm32f7")
# this:AddItem("STMicroelectronics STM32H7", "stm32h7")
# this:AddItem("Raspberry Pi Pico M0+", "rpcm0")
#--*/
#define __CPU_ARCH__ stm32f1
#/*
__CPU_ARCH__=stm32f1
#*/

#/*--
# this:AddWidget("Spinbox", 0, 16, "Terminal UART")
# this:SetToolTip("This value set UART terminal. This is UART major number.")
#--*/
#define __CPU_UART_TERM__ 0


#/*--
# this:AddExtraWidget("Label", "LabelAppset", "\nAppset configuration", -1, "bold")
# this:AddExtraWidget("Void", "VoidAppset")
#++*/
#/*--
# this:AddWidget("Editline", false, "Appset address")
# this:SetToolTip("Start address of application section.")
#--*/
#define __PROJ_APPSET_TABLE_ADDRESS__ 0x08010000

#/*--
# this:AddWidget("Editline", false, "Appset size")
# this:SetToolTip("Size of application section.")
#--*/
#define __PROJ_APPSET_TABLE_SIZE__ 1024k

#/*--
#this:AddWidget("Editline", false, "dnx RTOS section address")
#this:SetToolTip("dnx RTOS section start address.")
#--*/
#define __LIBC_DNX_RTOS_ADDRESS__ 0x08000000

#if (__CPU_ARCH__ == stm32f1)
#define ARCH_stm32f1
#/*
ifeq ($(__CPU_ARCH__), stm32f1)
CPUCONFIG_AFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_CFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_LDFLAGS=-mcpu=cortex-m3 -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#elif (__CPU_ARCH__ == stm32f3)
#define ARCH_stm32f3
#/*
ifeq ($(__CPU_ARCH__), stm32f3)
CPUCONFIG_AFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_LDFLAGS=-mcpu=cortex-m4 -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#elif (__CPU_ARCH__ == stm32f4)
#define ARCH_stm32f4
#/*
ifeq ($(__CPU_ARCH__), stm32f4)
CPUCONFIG_AFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_LDFLAGS=-mcpu=cortex-m4 -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#elif (__CPU_ARCH__ == stm32f7)
#define ARCH_stm32f7
#/*
ifeq ($(__CPU_ARCH__), stm32f7)
CPUCONFIG_AFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_LDFLAGS=-mcpu=cortex-m7 -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#elif (__CPU_ARCH__ == stm32h7)
#define ARCH_stm32h7
#/*
ifeq ($(__CPU_ARCH__), stm32h7)
CPUCONFIG_AFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_LDFLAGS=-mcpu=cortex-m7 -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#elif (__CPU_ARCH__ == rpcm0)
#define ARCH_rpcm0
#/*
ifeq ($(__CPU_ARCH__), rpcm0)
CPUCONFIG_AFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_CFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_LDFLAGS=-mcpu=cortex-m0plus -mthumb -mthumb-interwork -nostartfiles
CPUCONFIG_LDFLAGS += -T./build/program/layout.ld
endif
#*/
#endif

#endif /* _PROJECT_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
