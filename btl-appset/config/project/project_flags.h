#/*=============================================================================
# @file   project_flags.h
#
# @author Daniel Zorychta
#
# @brief  This file contains base project configuration.
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


#ifndef _PROJECT_FLAGS_H_
#define _PROJECT_FLAGS_H_

#/*--
# this:SetTitle("Home > Project", "Project general settings.")
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
#                 " - arm-none-eabi-\n"..
#                 " - GCC: $(none)")
#--*/
#define __PROJECT_TOOLCHAIN__ arm-none-eabi-
#/*
none=
__PROJECT_TOOLCHAIN__=arm-none-eabi-
#*/

#// Existing architectures
#define stm32f1 0x16F54349
#define stm32f2 0x2f31e2fc
#define stm32f3 0x483962d4
#define stm32f4 0x8dd787b6
#define stm32f7 0x3cb2ba7c
#define stm32h7 0x56A5cde6
#define rpcm0   0x60b36426
#define posix   0x7a3c51e9
#/*--
# this:AddWidget("Combobox", "CPU architecture")
# this:AddItem("STMicroelectronics STM32F1", "stm32f1")
# this:AddItem("STMicroelectronics STM32F3", "stm32f3")
# this:AddItem("STMicroelectronics STM32F4", "stm32f4")
# this:AddItem("STMicroelectronics STM32F7", "stm32f7")
# this:AddItem("STMicroelectronics STM32H7", "stm32h7")
# this:AddItem("Raspberry Pi Pico M0+", "rpcm0")
# this:AddItem("POSIX simulator (Linux)", "posix")
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
# this:AddWidget("Editline", false, "Appset executable address")
# this:SetToolTip("Start address of application executable section.")
#--*/
#define __PROJ_APPSET_TABLE_ADDRESS__ 0x08010000

#/*--
# this:AddWidget("Editline", false, "Appset executable size")
# this:SetToolTip("Size of application executable section.")
#--*/
#define __PROJ_APPSET_TABLE_SIZE__ 1024k

#/*--
# this:AddWidget("Editline", false, "Appset RAM address")
# this:SetToolTip("Start address of application RAM section (normally not used).")
#--*/
#define __PROJ_APPSET_RAM_ADDRESS__ 0

#/*--
# this:AddWidget("Editline", false, "Appset RAM size")
# this:SetToolTip("Size of application RAM section (normally not used).")
#--*/
#define __PROJ_APPSET_RAM_SIZE__ 1024

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
#elif (__CPU_ARCH__ == posix)
#define ARCH_posix
#/*
ifeq ($(__CPU_ARCH__), posix)
CPUCONFIG_AFLAGS=-m32 -fno-pie
CPUCONFIG_CFLAGS=-m32 -fno-pie -fno-strict-aliasing -funsigned-char
CPUCONFIG_CXXFLAGS=-m32 -fno-pie -fno-strict-aliasing -funsigned-char
CPUCONFIG_LDFLAGS=-m32
endif
#*/
#endif

#endif /* _PROJECT_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
