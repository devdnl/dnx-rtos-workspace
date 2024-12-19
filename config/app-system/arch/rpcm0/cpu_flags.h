#/*=============================================================================
# @file    cpu_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains CPU configuration flags.
#          Hybrid file: included both by Make and CC.
#
# @note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>
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
# this:SetLayout("TitledGridBack", 2, "Home > Microcontroller > Selection",
#                function() this:LoadFile("arch/arch_flags.h") end)
#++*/

#/*-- Flag is set in __CPU_NAME__ event
# this:AddWidget("Value")
#--*/
#define __CPU_FAMILY__ _RP2040_

#/*-- Flag is set in __CPU_NAME__ event
# this:AddWidget("Value")
#--*/
#define __LD_SCRIPT__ RPxx40
#/*
__LD_SCRIPT__=RPxx40
#*/

#/*--
# local CPULIST = {{"RP2040", "_RP2040_", "RPxx40"}}
#
# this:AddWidget("Combobox", "Mirocontroller name")
# for i = 1, #CPULIST do this:AddItem(CPULIST[i][1], CPULIST[i][1]) end
# this:SetEvent("clicked",
#    function()
#        local CPUNAME = this:GetFlagValue("__CPU_NAME__")
#
#        for i = 1, #CPULIST do
#            if CPULIST[i][1] == CPUNAME then
#                this:SetFlagValue("__CPU_FAMILY__", CPULIST[i][2])
#                this:SetFlagValue("__LD_SCRIPT__", CPULIST[i][3])
#                break
#            end
#        end
#    end
# )
#--*/
#define __CPU_NAME__ RP2040
#/*
__CPU_NAME__=RP2040
#*/

#/*-- Flag is set in __CPU_BOOT2_FILE__ event
# this:AddWidget("Value")
#--*/
#define __CPU_LD_BOOT2__ _boot2
#/*
__CPU_LD_BOOT2__=_boot2
#*/

#/*--
# this:AddWidget("Combobox", "BOOT2 memory type")
# this:AddItem("No BOOT2", "none")
# this:AddItem("Generic 03h", "boot2_generic_03h.c")
# this:AddItem("AT25SF128A", "boot2_at25sf128a.c")
# this:AddItem("IS25LP080", "boot2_is25lp080.c")
# this:AddItem("W25Q080, W25Q16JV, AT25SF081, S25FL132K0", "boot2_w25q080.c")
# this:AddItem("W25X10CL", "boot2_w25x10cl.c")
#
# this:SetEvent("clicked",
#    function()
#        local boot2_file = this:GetFlagValue("__CPU_BOOT2_FILE__")
#        if boot2_file == "none" then
#           this:SetFlagValue("__CPU_LD_BOOT2__", "")
#        else
#           this:SetFlagValue("__CPU_LD_BOOT2__", "_boot2")
#        end
#    end
# )
#--*/
#define __CPU_BOOT2_FILE__ boot2_w25q080.c
#/*
__CPU_BOOT2_FILE__=boot2_w25q080.c
#*/


#/*
# * NOTE!
# * Any interrupt that uses the FreeRTOS API must be set to the same priority as the
# * RTOS kernel (as configured by the _CPU_IRQ_RTOS_KERNEL_PRIORITY_ macro), or at
# * or below _CPU_IRQ_RTOS_SYSCALL_PRIORITY_ for ports that include this functionality.
# */

#/*--
# this:AddWidget("Combobox", "RTOS Kernel IRQ priority")
# uC.AddPriorityItems(this, true)
#--*/
#define __CPU_IRQ_RTOS_KERNEL_PRIORITY_VAL__ 3

#/*--
# this:AddWidget("Combobox", "Max RTOS API call IRQ priority")
# uC.AddPriorityItems(this, true)
#--*/
#define __CPU_IRQ_RTOS_APICALL_PRIORITY_VAL__ 2

#/*--
# this:AddWidget("Combobox", "Default IRQ priority")
# uC.AddPriorityItems(this, true)
#--*/
#define __CPU_DEFAULT_IRQ_PRIORITY__ 2

#/*--
# this:AddWidget("Combobox", "Show and clear reset source")
# this:AddItem("Disable", "_NO_")
# this:AddItem("Enable", "_YES_")
#--*/
#define __CPU_SHOW_AND_CLEAR_RESET_SOURCE__ _YES_

#/*--
# this:AddWidget("Editline", false, "Interrupt vector position")
#--*/
#define __CPU_VTOR_TAB_POSITION__ 0x10000100

#/*--
# this:AddWidget("Combobox", "Custom linker script")
# this:AddItem("No", "_NO_")
# this:AddItem("Yes", "_YES_")
#--*/
#define __CUSTOM_LD_SCRIPT__ _NO_
#/*
__CUSTOM_LD_SCRIPT__=_NO_
#*/

#/*--
# this:AddWidget("Editline", true, "Custom linker script file path")
# this:SetToolTip("File is relative to configuration directory.")
#--*/
#define __CUSTOM_LD_SCRIPT_FILE__ "custom_script.ld"
#/*
__CUSTOM_LD_SCRIPT_FILE__="custom_script.ld"
#*/

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

#//-----------------------------------------------------------------------------
#// mandatory flags, not configurable
#//-----------------------------------------------------------------------------
#define _CPU_START_FREQUENCY_           (6000000UL)
#define _CPU_HEAP_ALIGN_                (4)
#define _CPU_IRQ_RTOS_KERNEL_PRIORITY_  (__CPU_IRQ_RTOS_KERNEL_PRIORITY_VAL__ << 4)
#define _CPU_IRQ_RTOS_APICALL_PRIORITY_ (__CPU_IRQ_RTOS_APICALL_PRIORITY_VAL__ << 4)
#define _CPU_IRQ_SAFE_PRIORITY_         (__CPU_IRQ_RTOS_KERNEL_PRIORITY_VAL__)
#define ARCH_rpcm0
#/*
CPUCONFIG_AFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_CFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m0plus -mthumb -ffast-math -fno-math-errno -mthumb-interwork
CPUCONFIG_LDFLAGS=-mcpu=cortex-m0plus -mthumb -mthumb-interwork -nostartfiles -Wl,-u,boot2tab

ifeq ($(__CUSTOM_LD_SCRIPT__), _YES_)
CPUCONFIG_LDFLAGS += -T$(CONFIG_DIR)/$(__CUSTOM_LD_SCRIPT_FILE__)
else
CPUCONFIG_LDFLAGS += -T./src/cpu/rpcm0/ld/$(__LD_SCRIPT__)$(__CPU_LD_BOOT2__).ld
endif

#*/

#// current CPU family definitions
#define _RP2040_ 0x4f58afdf
#if __CPU_FAMILY__ == _RP2040_
#define PICO_RP2040 1
#else
#error Wrong CPU family
#endif

#endif /* _CPU_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
