#/*=============================================================================
# @file    arch_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains list of existing modules
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

#ifndef _ARCH_FLAGS_H_
#define _ARCH_FLAGS_H_

#/*
# Modules in this list are disabled on import event. Function can be found in
# the config.h file.
#*/

#/*--
# this:SetTitle("Home > Microcontroller", "The microcontroller configuration menu.")
#
#
# this.PutWidgets = function(this, name, file)
#     if type(name) == "string" then
#         this:AddWidget("Combobox", name..": enable")
#         this:AddItem("Yes", "_YES_")
#         this:AddItem("No", "_NO_")
#
#         if type(file) == "string" then
#             this:AddExtraWidget("Submenu", "HL_"..name, name..": configure", file)
#         end
#     end
# end
#
# this.PeripheralExist = function(this, name)
#     for _,v in ipairs(uC.PERIPH[uC.NAME]) do
#         if v == name then
#               return true
#         end
#     end
#     return false
# end
#
# -- variable is cleared when architecture is selecting (may not be selected)
# -- variable is visible in all modules configuration
# uC = {}
#
# uC.ARCH    = this:GetFlagValue("__CPU_ARCH__", "project/project_flags.h")
# uC.OSCFREQ = this:GetFlagValue("__CPU_OSC_FREQ__", "project/project_flags.h")
# uC.NAME    = this:GetFlagValue("__CPU_NAME__", "arch/"..uC.ARCH.."/cpu_flags.h")
# uC.FAMILY  = this:GetFlagValue("__CPU_FAMILY__", "arch/"..uC.ARCH.."/cpu_flags.h"):match("_(.*)_")
# uC.PERIPH  = {}
#
# if uC.ARCH == "stm32f1" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         for i = 1, 14 do this:AddItem("Priority "..i, tostring(i)) end
#         this:AddItem("Priority 15 (the lowest, safe for kernel API)", "15")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["STM32F100C8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F100RBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F100RExx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F100VBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F100ZCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101C8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101CBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101R8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101RBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101V8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F101VBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC"}
#     uC.PERIPH["STM32F103C6xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103C8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103CBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103R6xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103R8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103RBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103RCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103RDxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103RExx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103RGxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103T8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103TBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103V8xx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103VBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103VCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103VDxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103VExx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103VGxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103ZCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103ZExx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F103ZGxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "USBD"}
#     uC.PERIPH["STM32F105RBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN"}
#     uC.PERIPH["STM32F105RCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN"}
#     uC.PERIPH["STM32F105VBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN"}
#     uC.PERIPH["STM32F105VCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN"}
#     uC.PERIPH["STM32F107RCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "ETH"}
#     uC.PERIPH["STM32F107VBxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "ETH"}
#     uC.PERIPH["STM32F107VCxx"]={"GPIO", "CLK", "CRC", "DMA", "PWM", "WDG", "ADC", "UART", "SPI", "AFM", "IRQ", "I2C", "NVM", "RTC", "CAN", "ETH"}
# end
#
# if uC.ARCH == "stm32f3" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         for i = 1, 14 do this:AddItem("Priority "..i, tostring(i)) end
#         this:AddItem("Priority 15 (the lowest, safe for kernel API)", "15")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["STM32F301C6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F301C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F301K6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F301K8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F301R6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F301R8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F302C6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302CBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302CCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302K6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302K8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302R6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302R8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302RBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302RCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302RDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302RExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302VBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302VCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302VDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302VExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302ZDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F302ZExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303C6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303CBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303CCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303K6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303K8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303R6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303R8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303RBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303RCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303RDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303RExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303VBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303VCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303VDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303VExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303ZDxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F303ZExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F318C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F318K8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART"}
#     uC.PERIPH["STM32F328C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334C4xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334C6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334K4xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334K6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334K8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334R6xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F334R8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F358CCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F358RCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F358VCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373C8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373CBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373CCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373R8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373RBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373RCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373V8xx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373VBxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F373VCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F378CCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F378RCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F378VCxx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
#     uC.PERIPH["STM32F398VExx"]={"GPIO", "CLK", "I2C", "IRQ", "DMA", "PWM", "UART", "CAN"}
# end
#
# if uC.ARCH == "stm32f4" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         for i = 1, 14 do this:AddItem("Priority "..i, tostring(i)) end
#         this:AddItem("Priority 15 (the lowest, safe for kernel API)", "15")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["STM32F401CBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401RBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401VBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401CCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401RCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401VCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401CDxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401RDxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401VDxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401CExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401RExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F401VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F405RGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F405OExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F405OGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F405VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F405ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F407VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F407VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F407ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F407ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F407IExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F407IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F410T8xx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F410TBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F410C8xx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F410CBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F410R8xx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F410RBxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411CCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411RCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411VCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411CExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411RExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F411VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F412CExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412CGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412RExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412RGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F412ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413CGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F413RGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413MGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413CHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F413RHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413MHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413VHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F413ZHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "QSPI", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F415RGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F415OGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F415VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F415ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F417VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F417VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F417ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F417ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F417IExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F417IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F423CHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART"}
#     uC.PERIPH["STM32F423RHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F423MHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F423VHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F423ZHxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI"}
#     uC.PERIPH["STM32F427VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427AGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F427IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F429VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429AGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429IExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429BExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429BGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429BIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429NExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429NGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F429NIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI", "USBH"}
#     uC.PERIPH["STM32F437VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F437AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439BGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439BIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439NGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439NIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F439AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F446MCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446RCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "DCI"}
#     uC.PERIPH["STM32F446VCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446ZCxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446MExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446RExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F446ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "CAN", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469VExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469ZExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469AExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469AGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F469IExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469BExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469BGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469BIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469NExx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469NGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F469NIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "CAN", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479VGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F479VIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F479ZGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479ZIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479AGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F479AIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "DCI"}
#     uC.PERIPH["STM32F479IGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479IIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479BGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479BIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479NGxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
#     uC.PERIPH["STM32F479NIxx"]={"GPIO", "CLK", "CRC", "RTC", "I2C", "DMA", "AFM", "SND", "IRQ", "NVM", "PWM", "QSPI", "SDMMC", "WDG", "SPI", "UART", "EMI", "ETH", "DCI"}
# end
#
# if uC.ARCH == "stm32f7" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         for i = 1, 14 do this:AddItem("Priority "..i, tostring(i)) end
#         this:AddItem("Priority 15 (the lowest, safe for kernel API)", "15")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["STM32F722ICxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722RCxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722RExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "LTDC"}
#     uC.PERIPH["STM32F722VCxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722ZCxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F722ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723ICxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723VCxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723ZCxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F723ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F730I8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F730R8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F730V8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F730Z8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F732IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F732RExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F732VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F732ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F733IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F733VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F733ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC"}
#     uC.PERIPH["STM32F745IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F745IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F745VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F745VGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F745ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F745ZGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F746BExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746BGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746IExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746NExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746NGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746VExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746VGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746ZExx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F746ZGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F750N8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F750V8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F750Z8xx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F756BGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F756IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F756NGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F756VGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F756ZGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F765BGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765BIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765IIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765NGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765NIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765VGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765VIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765ZGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F765ZIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F767BGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767BIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767IIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767NGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767NIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767VGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767VIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767ZGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F767ZIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F768AIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F769AGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH"}
#     uC.PERIPH["STM32F769AIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769BGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769BIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769IGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769IIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769NGxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F769NIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F777BIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F777IIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F777NIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F777VIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F777ZIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F778AIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F779AIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F779BIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F779IIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
#     uC.PERIPH["STM32F779NIxx"]={"GPIO", "WDG", "UART", "AFM", "DMA", "PWM", "IRQ", "SPI", "CAN", "EMI", "CLK", "I2C", "NVM", "SDMMC", "RTC", "TIMENC", "ETH", "LTDC"}
# end
#
# if uC.ARCH == "stm32h7" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         for i = 1, 14 do this:AddItem("Priority "..i, tostring(i)) end
#         this:AddItem("Priority 15 (the lowest, safe for kernel API)", "15")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["STM32H723VExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H723VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H723ZExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H723ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725AExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725AGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725IExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725RExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725RGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725VExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725ZExx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H725ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H730ABxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H730IBxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H730VBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H730ZBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H733VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H733ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H735AGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H735IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H735RGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H735VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H735ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H742AGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742BGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742XGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742AIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742VIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H742ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "CAN"}
#     uC.PERIPH["STM32H743AGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743BGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743XGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743AIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743VIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H743ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745BGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745XGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H745ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747AGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747BGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747XGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747AIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H747ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H750IBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H750VBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H750XBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H750ZBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753AIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753VIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H753ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H755BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H755IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H755XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H755ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H757AIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H757BIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H757IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H757XIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H757ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "SPI", "I2C", "SDMMC", "NVM", "EMI", "ETH", "LTDC", "CAN"}
#     uC.PERIPH["STM32H7A3AGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3IGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3IGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3LGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3NGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3QGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3RGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3VGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3VGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3ZGxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3ZGxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3AIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3IIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3LIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3NIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3QIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3RIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3VIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3VIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7A3ZIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0ABxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0IBKxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0IBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0RBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0VBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B0ZBxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3AIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3IIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3IIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3LIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3NIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3QIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3RIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3VIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3VIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3ZIxx" ]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
#     uC.PERIPH["STM32H7B3ZIxxQ"]={"GPIO", "CLK", "AFM", "RTC", "DMA", "UART", "QSPI", "I2C", "SDMMC", "NVM", "EMI", "LTDC"}
# end
#
# if uC.ARCH == "rpcm0" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0 (the highest)", "0")
#         this:AddItem("Priority 1", "1")
#         this:AddItem("Priority 2", "2")
#         this:AddItem("Priority 3 (the lowest, safe for kernel API)", "3")
#         if no_default ~= true then
#             this:AddItem("Default priority", "__CPU_DEFAULT_IRQ_PRIORITY__")
#         end
#     end
#
#     uC.PERIPH["RP2040"]={"GPIO", "CLK", "UART"}
# end
#
# if uC.ARCH == "posix" then
#     uC.AddPriorityItems = function(this, no_default)
#         this:AddItem("Priority 0", "0")
#     end
#
#     uC.PERIPH["POSIX"]={"GPIO", "UART", "HOSTDISK", "RTC"}
# end
#++*/

#/* include of CPU mandatory file in Makefile
include $(CONFIG_DIR)/arch/$(__CPU_ARCH__)/cpu_flags.h
#*/

#define ARCH_noarch
#include "noarch/tty_flags.h"

#if (__CPU_ARCH__ == stm32f1)
#include "stm32f1/adc_flags.h"
#include "stm32f1/can_flags.h"
#include "stm32f1/cpu_flags.h"
#include "stm32f1/rtc_flags.h"
#include "stm32f1/crc_flags.h"
#include "stm32f1/gpio_flags.h"
#include "stm32f1/afm_flags.h"
#include "stm32f1/eth_flags.h"
#include "stm32f1/clk_flags.h"
#include "stm32f1/spi_flags.h"
#include "stm32f1/wdg_flags.h"
#include "stm32f1/uart_flags.h"
#include "stm32f1/usbd_flags.h"
#include "stm32f1/irq_flags.h"
#include "stm32f1/i2c_flags.h"
#include "stm32f1/nvm_flags.h"
#include "stm32f1/pwm_flags.h"
#elif (__CPU_ARCH__ == stm32f3)
#include "stm32f3/cpu_flags.h"
#include "stm32f3/gpio_flags.h"
#include "stm32f3/uart_flags.h"
#include "stm32f3/clk_flags.h"
#include "stm32f3/can_flags.h"
#include "stm32f3/i2c_flags.h"
#include "stm32f3/irq_flags.h"
#include "stm32f3/pwm_flags.h"
#elif (__CPU_ARCH__ == stm32f4)
#include "stm32f4/can_flags.h"
#include "stm32f4/cpu_flags.h"
#include "stm32f4/gpio_flags.h"
#include "stm32f4/rtc_flags.h"
#include "stm32f4/crc_flags.h"
#include "stm32f4/irq_flags.h"
#include "stm32f4/uart_flags.h"
#include "stm32f4/clk_flags.h"
#include "stm32f4/emi_flags.h"
#include "stm32f4/i2c_flags.h"
#include "stm32f4/afm_flags.h"
#include "stm32f4/eth_flags.h"
#include "stm32f4/dci_flags.h"
#include "stm32f4/spi_flags.h"
#include "stm32f4/wdg_flags.h"
#include "stm32f4/sdmmc_flags.h"
#include "stm32f4/nvm_flags.h"
#include "stm32f4/pwm_flags.h"
#include "stm32f4/snd_flags.h"
#include "stm32f4/usbh_flags.h"
#include "stm32f4/qspi_flags.h"
#elif (__CPU_ARCH__ == stm32f7)
#include "stm32f7/cpu_flags.h"
#include "stm32f7/gpio_flags.h"
#include "stm32f7/uart_flags.h"
#include "stm32f7/wdg_flags.h"
#include "stm32f7/afm_flags.h"
#include "stm32f7/clk_flags.h"
#include "stm32f7/can_flags.h"
#include "stm32f7/i2c_flags.h"
#include "stm32f7/rtc_flags.h"
#include "stm32f7/spi_flags.h"
#include "stm32f7/ltdc_flags.h"
#include "stm32f7/irq_flags.h"
#include "stm32f7/pwm_flags.h"
#include "stm32f7/eth_flags.h"
#include "stm32f7/sdmmc_flags.h"
#include "stm32f7/nvm_flags.h"
#include "stm32f7/timenc_flags.h"
#include "stm32f7/emi_flags.h"
#elif (__CPU_ARCH__ == stm32h7)
#include "stm32h7/cpu_flags.h"
#include "stm32h7/gpio_flags.h"
#include "stm32h7/uart_flags.h"
#include "stm32h7/clk_flags.h"
#include "stm32h7/eth_flags.h"
#include "stm32h7/rtc_flags.h"
#include "stm32h7/afm_flags.h"
#include "stm32h7/emi_flags.h"
#include "stm32h7/i2c_flags.h"
#include "stm32h7/spi_flags.h"
#include "stm32h7/ltdc_flags.h"
#include "stm32h7/sdmmc_flags.h"
#include "stm32h7/can_flags.h"
#include "stm32h7/nvm_flags.h"
#include "stm32h7/qspi_flags.h"
#elif (__CPU_ARCH__ == rpcm0)
#include "rpcm0/cpu_flags.h"
#include "rpcm0/clk_flags.h"
#include "rpcm0/gpio_flags.h"
#include "rpcm0/uart_flags.h"
#elif (__CPU_ARCH__ == posix)
#include "posix/cpu_flags.h"
#include "posix/gpio_flags.h"
#include "posix/uart_flags.h"
#endif

#/*--
# this:AddExtraWidget("Submenu", "HL_CPU", "Configure "..uC.NAME, "arch/"..uC.ARCH.."/cpu_flags.h")
#++*/

#// MODULE LIST BEGIN
#/*--
# if this:PeripheralExist("ADC") then
#     this:PutWidgets("ADC", "arch/"..uC.ARCH.."/adc_flags.h")
#     this:SetToolTip("ADC Driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_ADC__", "_NO_")
# end
#--*/
#define __ENABLE_ADC__ _NO_
#/*
__ENABLE_ADC__=_NO_
#*/

#/*--
# if this:PeripheralExist("AFM") then
#     this:PutWidgets("AFM", "arch/"..uC.ARCH.."/afm_flags.h")
#     this:SetToolTip("Alternative Function Module. Here one can control alternative "..
#                     "function of selected microcontroller.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_AFM__", "_NO_")
# end
#--*/
#define __ENABLE_AFM__ _YES_
#/*
__ENABLE_AFM__=_YES_
#*/

#/*--
# if this:PeripheralExist("CAN") then
#     this:PutWidgets("CAN", "arch/"..uC.ARCH.."/can_flags.h")
#     this:SetToolTip("CAN driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_CAN__", "_NO_")
# end
#--*/
#define __ENABLE_CAN__ _NO_
#/*
__ENABLE_CAN__=_NO_
#*/

#/*--
# if this:PeripheralExist("CLK") then
#     this:PutWidgets("CLK", "arch/"..uC.ARCH.."/clk_flags.h")
#     this:SetToolTip("Clock distribution driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_CLK__", "_NO_")
# end
#--*/
#define __ENABLE_CLK__ _YES_
#/*
__ENABLE_CLK__=_YES_
#*/

#/*--
# if this:PeripheralExist("CRC") then
#     this:PutWidgets("CRC")
#     this:SetToolTip("CRC driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_CRC__", "_NO_")
# end
#--*/
#define __ENABLE_CRC__ _NO_
#/*
__ENABLE_CRC__=_NO_
#*/

#/*--
# if this:PeripheralExist("DCI") then
#     this:PutWidgets("DCI", "arch/"..uC.ARCH.."/dci_flags.h")
#     this:SetToolTip("Digital Camera Interface Driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_DCI__", "_NO_")
# end
#--*/
#define __ENABLE_DCI__ _NO_
#/*
__ENABLE_DCI__=_NO_
#*/

#/*--
# this:PutWidgets("DHT11")
# this:SetToolTip("DHT11 temperature & humidity sensor. To use this module set devices pins to open drain.")
#--*/
#define __ENABLE_DHT11__ _NO_
#/*
__ENABLE_DHT11__=_NO_
#*/

#/*--
# if this:PeripheralExist("DMA") then
#     this:PutWidgets("DMA")
#     this:SetToolTip("General usage DMA driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_DMA__", "_NO_")
# end
#--*/
#define __ENABLE_DMA__ _YES_
#/*
__ENABLE_DMA__=_YES_
#*/

#/*--
# this:PutWidgets("ENC28J60")
# this:SetToolTip("ENC28J60 SPI-Ethernet 10 Mb/s chip driver.")
#--*/
#define __ENABLE_ENC28J60__ _NO_
#/*
__ENABLE_ENC28J60__=_NO_
#*/

#/*--
# if this:PeripheralExist("EMI") then
#     this:PutWidgets("EMI", "arch/"..uC.ARCH.."/emi_flags.h")
#     this:SetToolTip("External Memory Interface.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_EMI__", "_NO_")
# end
#--*/
#define __ENABLE_EMI__ _YES_
#/*
__ENABLE_EMI__=_YES_
#*/

#/*--
# if this:PeripheralExist("ETH") then
#     this:PutWidgets("ETH", "arch/"..uC.ARCH.."/eth_flags.h")
#     this:SetToolTip("Ethernet driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_ETH__", "_NO_")
# end
#--*/
#define __ENABLE_ETH__ _YES_
#/*
__ENABLE_ETH__=_YES_
#*/

#/*--
# this:PutWidgets("FBRIDGE")
# this:SetToolTip("File Bridge driver.")
#--*/
#define __ENABLE_FBRIDGE__ _YES_
#/*
__ENABLE_FBRIDGE__=_YES_
#*/

#/*--
# if this:PeripheralExist("GPIO") then
#     this:PutWidgets("GPIO", "arch/"..uC.ARCH.."/gpio_flags.h")
#     this:SetToolTip("GPIO driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_GPIO__", "_NO_")
# end
#--*/
#define __ENABLE_GPIO__ _YES_
#/*
__ENABLE_GPIO__=_YES_
#*/

#/*--
# if this:PeripheralExist("HOSTDISK") then
#     this:PutWidgets("HOSTDISK")
#     this:SetToolTip("Block device stored in the host file (POSIX simulator only).")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_HOSTDISK__", "_NO_")
# end
#--*/
#define __ENABLE_HOSTDISK__ _NO_
#/*
__ENABLE_HOSTDISK__=_NO_
#*/

#/*--
# if this:PeripheralExist("I2C") then
#     this:PutWidgets("I2C", "arch/"..uC.ARCH.."/i2c_flags.h")
#     this:SetToolTip("I2C driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_I2C__", "_NO_")
# end
#--*/
#define __ENABLE_I2C__ _YES_
#/*
__ENABLE_I2C__=_YES_
#*/

#/*--
# this:PutWidgets("I2CEE")
# this:SetToolTip("I2C EEPROM driver for 24Cxx devices.")
#--*/
#define __ENABLE_I2CEE__ _NO_
#/*
__ENABLE_I2CEE__=_NO_
#*/

#/*--
# if this:PeripheralExist("IRQ") then
#     this:PutWidgets("IRQ", "arch/"..uC.ARCH.."/irq_flags.h")
#     this:SetToolTip("External IRQ driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_IRQ__", "_NO_")
# end
#--*/
#define __ENABLE_IRQ__ _NO_
#/*
__ENABLE_IRQ__=_NO_
#*/

#/*--
# this:PutWidgets("LOOP")
# this:SetToolTip("LOOP driver. Driver can be used to create device node controlled from application.")
#--*/
#define __ENABLE_LOOP__ _NO_
#/*
__ENABLE_LOOP__=_NO_
#*/

#/*--
# if this:PeripheralExist("LTDC") then
#     this:PutWidgets("LTDC", "arch/"..uC.ARCH.."/ltdc_flags.h")
#     this:SetToolTip("LCD-TFT Display Controller module.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_LTDC__", "_NO_")
# end
#--*/
#define __ENABLE_LTDC__ _NO_
#/*
__ENABLE_LTDC__=_NO_
#*/

#/*--
# this:PutWidgets("MEMBLOCK")
# this:SetToolTip("Memory Block driver represented as file.")
#--*/
#define __ENABLE_MEMBLOCK__ _YES_
#/*
__ENABLE_MEMBLOCK__=_YES_
#*/

#/*--
# if this:PeripheralExist("NVM") then
#     this:PutWidgets("NVM")
#     this:SetToolTip("NVM driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_NVM__", "_NO_")
# end
#--*/
#define __ENABLE_NVM__ _YES_
#/*
__ENABLE_NVM__=_YES_
#*/

#/*--
# if this:PeripheralExist("QSPI") then
#     this:PutWidgets("QSPI", "arch/"..uC.ARCH.."/qspi_flags.h")
#     this:SetToolTip("Quad SPI driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_QSPI__", "_NO_")
# end
#--*/
#define __ENABLE_QSPI__ _YES_
#/*
__ENABLE_QSPI__=_YES_
#*/

#/*--
# this:PutWidgets("PART")
# this:SetToolTip("Partition device driver.")
#--*/
#define __ENABLE_PART__ _YES_
#/*
__ENABLE_PART__=_YES_
#*/

#/*--
# if this:PeripheralExist("PWM") then
#     this:PutWidgets("PWM")
#     this:SetToolTip("PWM driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_PWM__", "_NO_")
# end
#--*/
#define __ENABLE_PWM__ _NO_
#/*
__ENABLE_PWM__=_NO_
#*/

#/*--
# if this:PeripheralExist("RTC") then
#     this:PutWidgets("RTC")
#     this:SetToolTip("RTC driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_RTC__", "_NO_")
# end
#--*/
#define __ENABLE_RTC__ _YES_
#/*
__ENABLE_RTC__=_YES_
#*/

#/*--
# if this:PeripheralExist("SDMMC") then
#     this:PutWidgets("SDMMC", "arch/"..uC.ARCH.."/sdmmc_flags.h")
#     this:SetToolTip("SD/MMC driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_SDMMC__", "_NO_")
# end
#--*/
#define __ENABLE_SDMMC__ _YES_
#/*
__ENABLE_SDMMC__=_YES_
#*/

#/*--
# this:PutWidgets("SDSPI")
# this:SetToolTip("SD Card driver over SPI interface. "..
#                 "Note: make sure that the MISO pin is set to input "..
#                 "pulled-up mode or is connected to external pull-up resistor.")
#--*/
#define __ENABLE_SDSPI__ _NO_
#/*
__ENABLE_SDSPI__=_NO_
#*/

#/*--
# if this:PeripheralExist("SND") then
#     this:PutWidgets("SND", "arch/"..uC.ARCH.."/snd_flags.h")
#     this:SetToolTip("Sound interface driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_SND__", "_NO_")
# end
#--*/
#define __ENABLE_SND__ _NO_
#/*
__ENABLE_SND__=_NO_
#*/

#/*--
# if this:PeripheralExist("SPI") then
#     this:PutWidgets("SPI", "arch/"..uC.ARCH.."/spi_flags.h")
#     this:SetToolTip("SPI driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_SPI__", "_NO_")
# end
#--*/
#define __ENABLE_SPI__ _YES_
#/*
__ENABLE_SPI__=_YES_
#*/

#/*--
# this:PutWidgets("SPIEE")
# this:SetToolTip("SPI EEPROM driver.")
#--*/
#define __ENABLE_SPIEE__ _NO_
#/*
__ENABLE_SPIEE__=_NO_
#*/

#/*--
# if this:PeripheralExist("TIMENC") then
#     this:PutWidgets("TIMENC", "arch/"..uC.ARCH.."/timenc_flags.h")
#     this:SetToolTip("Timer driven AB encoder.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_TIMENC__", "_NO_")
# end
#--*/
#define __ENABLE_TIMENC__ _NO_
#/*
__ENABLE_TIMENC__=_NO_
#*/

#/*--
# this:PutWidgets("TTY", "arch/noarch/tty_flags.h")
# this:SetToolTip("TTY driver (terminal).")
#--*/
#define __ENABLE_TTY__ _YES_
#/*
__ENABLE_TTY__=_YES_
#*/

#/*--
# if this:PeripheralExist("UART") then
#     this:PutWidgets("UART", "arch/"..uC.ARCH.."/uart_flags.h")
#     this:SetToolTip("UART driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_UART__", "_NO_")
# end
#--*/
#define __ENABLE_UART__ _YES_
#/*
__ENABLE_UART__=_YES_
#*/

#/*--
# this:PutWidgets("URANDOM")
# this:SetToolTip("Pseudo random generator device.")
#--*/
#define __ENABLE_URANDOM__ _YES_
#/*
__ENABLE_URANDOM__=_YES_
#*/

#/*--
# if this:PeripheralExist("USBD") then
#     this:PutWidgets("USBD", "arch/"..uC.ARCH.."/usbd_flags.h")
#     this:SetToolTip("USB Device driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_USBD__", "_NO_")
# end
#--*/
#define __ENABLE_USBD__ _NO_
#/*
__ENABLE_USBD__=_NO_
#*/

#/*--
# if this:PeripheralExist("USBH") then
#     this:PutWidgets("USBH", "arch/"..uC.ARCH.."/usbh_flags.h")
#     this:SetToolTip("USB Host driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_USBH__", "_NO_")
# end
#--*/
#define __ENABLE_USBH__ _NO_
#/*
__ENABLE_USBH__=_NO_
#*/

#/*--
# if this:PeripheralExist("WDG") then
#     this:PutWidgets("WDG", "arch/"..uC.ARCH.."/wdg_flags.h")
#     this:SetToolTip("Watchdog driver.")
# else
#     this:AddWidget("Value")
#     this:SetFlagValue("__ENABLE_WDG__", "_NO_")
# end
#--*/
#define __ENABLE_WDG__ _NO_
#/*
__ENABLE_WDG__=_NO_
#*/

#/*--
# this:PutWidgets("ZERO")
# this:SetToolTip("Zero data device. Device node is working as /dev/zero or /dev/null.")
#--*/
#define __ENABLE_ZERO__ _YES_
#/*
__ENABLE_ZERO__=_YES_
#*/

#// MODULE LIST END
#//-----------------------------------------------------------------------------
#/*-- save current configuration if CPU was changed
# this:Save()
#++*/

#endif /* _ARCH_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
