/*==============================================================================
@file    gpio_flags.h

@author  Daniel Zorychta

@brief   GPIO module configuration flags.

@note    Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


==============================================================================*/

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
 *       and prefix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _GPIO_FLAGS_H_
#define _GPIO_FLAGS_H_

/*--
local function OpenPortMenu(port, pin) this.state.gpio_menu = {port, pin} end
local function ResetMenu() this.state.gpio_menu = nil end

local PACKAGE = {}
PACKAGE["C"]  = {A = 0xFFFF, B = 0xF7FF, C = 0xE000, H = 0x0003}
PACKAGE["R"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0x0004, H = 0x0003}
PACKAGE["V"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, H = 0x0003}
PACKAGE["Q"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF}
PACKAGE["Z"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0x0003}
PACKAGE["A"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0x07FF}
PACKAGE["I"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0x07FF}
PACKAGE["B"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0xFFFF, J = 0xFFFF, K = 0x00FF}
PACKAGE["N"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0xFFFF, J = 0xFFFF, K = 0x00FF}
PACKAGE["X"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0xFFFF, J = 0xFFFF, K = 0xFFFF}
PACKAGE["L"]  = {A = 0xFFFF, B = 0xFFFF, C = 0xFFFF, D = 0xFFFF, E = 0xFFFF, F = 0xFFFF, G = 0xFFFF, H = 0xFFFF, I = 0xFFFF, J = 0xFFFF, K = 0xFFFF}

local port = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K"}
local pack = uC.NAME:gsub("STM32H...(.).*", "%1")

if this.state.gpio_menu == nil then
    this:SetTitle("Home > Microcontroller > GPIO", "The GPIO module configuration.")

    for i = 1, #port do
        if PACKAGE[pack][port[i]] ~= nil then
            local gpio = port[i]
            this:AddExtraWidget("Submenu", "HL_GPIO"..gpio, gpio .. " GPIO"..gpio, "arch/stm32h7/gpio_flags.h")
            this:SetEvent("clicked", "HL_GPIO"..gpio, function() OpenPortMenu(gpio, nil) end)
            this:SetEvent("BackClicked", function() ResetMenu() end)
        end
    end

    this:Finish()

elseif this.state.gpio_menu[1] and (this.state.gpio_menu[2] == nil) then
    local gpio = this.state.gpio_menu[1]

    this:SetTitle("Home > Microcontroller > GPIO"..gpio, "The GPIO"..gpio.." port configuration.")

    for pin = 0, 15 do
        if PACKAGE[pack][gpio] ~= nil and ((PACKAGE[pack][gpio] & (1 << pin)) > 0) then
            this:AddExtraWidget("Submenu", "HL_PIN"..pin, pin.." P"..gpio..pin, "arch/stm32h7/gpio_flags.h")
            this:SetEvent("clicked", "HL_PIN"..pin, function() OpenPortMenu(gpio, pin) end)
        end
    end

    this:SetEvent("BackClicked", function() ResetMenu() end)

    this:Finish()

else
    local gpio = this.state.gpio_menu[1]
    local pin = this.state.gpio_menu[2]

    this:SetTitle("Home > Microcontroller > GPIO"..gpio.."."..pin, "GPIO"..gpio.."."..pin.." pin configuration.")
    this:SetEvent("BackClicked", function() OpenPortMenu(gpio, nil) end)

    local function PinExist(this, portLetter, pinNumber)
        if this.state.gpio_menu[1] ~= portLetter then
            return false
        end

        if this.state.gpio_menu[2] ~= pinNumber then
            return false
        end

        if PACKAGE[pack][portLetter] ~= nil then
            return (PACKAGE[pack][portLetter] & (1 << pinNumber)) > 0
        else
            return false
        end
    end

    this.AddPinNameWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Editline", false, "Name")
        end
    end

    this.AddPinSpeedWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "Speed")
            this:AddItem("Low", "_GPIO_SPEED_LOW")
            this:AddItem("Medium", "_GPIO_SPEED_MEDIUM")
            this:AddItem("High", "_GPIO_SPEED_HIGH")
            this:AddItem("Very high", "_GPIO_SPEED_VERY_HIGH")
            this:SetToolTip("Select pin output speed.")
        end
    end

    this.AddPinModeWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "Mode")
            this:AddItem("Output PP", "_GPIO_MODE_PP")
            this:AddItem("Output OD", "_GPIO_MODE_OD")
            this:AddItem("Output OD + PU", "_GPIO_MODE_OD_PU")
            this:AddItem("Output OD + PD", "_GPIO_MODE_OD_PD")
            this:AddItem("Alt. PP", "_GPIO_MODE_AF_PP")
            this:AddItem("Alt. PP + PU", "_GPIO_MODE_AF_PP_PU")
            this:AddItem("Alt. PP + PD", "_GPIO_MODE_AF_PP_PD")
            this:AddItem("Alt. OD", "_GPIO_MODE_AF_OD")
            this:AddItem("Alt. OD + PU", "_GPIO_MODE_AF_OD_PU")
            this:AddItem("Alt. OD + PD", "_GPIO_MODE_AF_OD_PD")
            this:AddItem("Input", "_GPIO_MODE_IN")
            this:AddItem("Input PU", "_GPIO_MODE_IN_PU")
            this:AddItem("Input PD", "_GPIO_MODE_IN_PD")
            this:AddItem("Analog", "_GPIO_MODE_ANALOG")
            this:SetToolTip("Select pin mode.")
        end
    end

    this.AddPinAltFuncWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "Alt. function")
            this:AddItem("AF0 (SYS)", "0")
            this:AddItem("AF1 (TIM1,2,16,17/LPTIM1/HRTIM1)", "1")
            this:AddItem("AF2 (SAI1/TIM3,4,5/HRTIM1)", "2")
            this:AddItem("AF3 (LPUART/TIM8/LPTIM2,3,4,5/HRTIM1/DFSDM1)", "3")
            this:AddItem("AF4 (I²C1,2,3,4/USART1/TIM15/LPTIM2/DFSDM1/CEC)", "4")
            this:AddItem("AF5 (SPI1,2,3,4,5,6/CEC)", "5")
            this:AddItem("AF6 (SPI2,3/SAI1,3/I²C4/UART4/DFSDM1)", "6")
            this:AddItem("AF7 (SPI2,3,6/USART1,2,3,6/UART7/SDMMC1)", "7")
            this:AddItem("AF8 (SPI6/SAI2,4/UART4,5,8/LPUART/SDMMC1/SODIFRX1)", "8")
            this:AddItem("AF9 (SAI4/FDCAN1,2/TIM13,14/QUADSPI/FMC/SDMMC2/LCD/SPDIFRX1)", "9")
            this:AddItem("AF10 (SAI2,4/TIM8/QUADSPI/SDMMC2/OTG1_HS/OTG2_FS/LCD)", "10")
            this:AddItem("AF11 (I²C4/UART7/SWPMI1/TIM1,8/DFSDM1/SDMMC2/MDIOS/ETH)", "11")
            this:AddItem("AF12 (TIM1,8/FMC/SDMMC1/MDIOS/OTG1_FS/LCD)", "12")
            this:AddItem("AF13 (TIM1/DCMI/LCD/DSI/COMP)", "13")
            this:AddItem("AF14 (UART5/LCD)", "14")
            this:AddItem("AF15 (SYS/EVENTOUT)", "15")
            this:SetToolTip("Select pin alternative function in case when alternative function mode was selected in \"Mode\" field.")
        end
    end

    this.AddPinStateWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "State")
            this:AddItem("Low", "_LOW")
            this:AddItem("High", "_HIGH")
            this:SetToolTip("Select pin output value (if applicable).")
        end
    end
end
++*/

#define _GPIO_CONCAT(x, y) x##y
#define _GPIO_PIN_NAME(port_idx, pin_number, pin_name) \
enum __##port_idx##_PIN_##pin_number##_CONF {\
        _GPIO_CONCAT(IOCTL_GPIO_PIN_IDX__, pin_name) = pin_number,\
        _GPIO_CONCAT(IOCTL_GPIO_PIN_MASK__, pin_name) = (1 << (pin_number)),\
        _GPIO_CONCAT(IOCTL_GPIO_PORT_IDX__, pin_name) = port_idx,\
}

#define IOCTL_GPIO_PORTA_IDX 0
#define IOCTL_GPIO_PORTB_IDX 1
#define IOCTL_GPIO_PORTC_IDX 2
#define IOCTL_GPIO_PORTD_IDX 3
#define IOCTL_GPIO_PORTE_IDX 4
#define IOCTL_GPIO_PORTF_IDX 5
#define IOCTL_GPIO_PORTG_IDX 6
#define IOCTL_GPIO_PORTH_IDX 7
#define IOCTL_GPIO_PORTI_IDX 8
#define IOCTL_GPIO_PORTJ_IDX 9
#define IOCTL_GPIO_PORTK_IDX 10
#define IOCTL_GPIO_PORTNONE_IDX 11

_GPIO_PIN_NAME(IOCTL_GPIO_PORTNONE_IDX, 17, NONE);

/*----------------------------------------------------------------------------*/
/*--
this:AddPinNameWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_NAME__ PA0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 0, __GPIO_PA_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_NAME__ PA1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 1, __GPIO_PA_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_NAME__ PA2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 2, __GPIO_PA_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_NAME__ PA3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 3, __GPIO_PA_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_NAME__ PA4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 4, __GPIO_PA_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_NAME__ PA5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 5, __GPIO_PA_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_NAME__ PA6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 6, __GPIO_PA_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_NAME__ PA7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 7, __GPIO_PA_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_NAME__ PA8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 8, __GPIO_PA_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_NAME__ PA9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 9, __GPIO_PA_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_NAME__ PA10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 10, __GPIO_PA_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_NAME__ PA11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 11, __GPIO_PA_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_NAME__ PA12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 12, __GPIO_PA_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_NAME__ JTMS_SWDIO
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 13, __GPIO_PA_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_SPEED__ _GPIO_SPEED_HIGH

/*--
this:AddPinModeWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_MODE__ _GPIO_MODE_AF_PP

/*--
this:AddPinAltFuncWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_NAME__ JTCK_SWCLK
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 14, __GPIO_PA_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_SPEED__ _GPIO_SPEED_HIGH

/*--
this:AddPinModeWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_MODE__ _GPIO_MODE_AF_PP

/*--
this:AddPinAltFuncWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_NAME__ JTDI
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 15, __GPIO_PA_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_SPEED__ _GPIO_SPEED_HIGH

/*--
this:AddPinModeWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_MODE__ _GPIO_MODE_AF_PP

/*--
this:AddPinAltFuncWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_NAME__ PB0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 0, __GPIO_PB_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_NAME__ PB1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 1, __GPIO_PB_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_NAME__ PB2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 2, __GPIO_PB_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_NAME__ PB3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 3, __GPIO_PB_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_NAME__ PB4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 4, __GPIO_PB_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_NAME__ PB5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 5, __GPIO_PB_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_NAME__ PB6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 6, __GPIO_PB_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_NAME__ PB7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 7, __GPIO_PB_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_NAME__ PB8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 8, __GPIO_PB_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_NAME__ PB9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 9, __GPIO_PB_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_NAME__ PB10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 10, __GPIO_PB_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_NAME__ PB11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 11, __GPIO_PB_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_NAME__ PB12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 12, __GPIO_PB_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_NAME__ PB13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 13, __GPIO_PB_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_NAME__ PB14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 14, __GPIO_PB_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_NAME__ PB15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 15, __GPIO_PB_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_NAME__ PC0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 0, __GPIO_PC_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_NAME__ PC1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 1, __GPIO_PC_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_NAME__ PC2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 2, __GPIO_PC_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_NAME__ PC3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 3, __GPIO_PC_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_NAME__ PC4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 4, __GPIO_PC_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_NAME__ PC5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 5, __GPIO_PC_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_NAME__ PC6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 6, __GPIO_PC_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_NAME__ PC7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 7, __GPIO_PC_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_NAME__ PC8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 8, __GPIO_PC_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_NAME__ PC9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 9, __GPIO_PC_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_NAME__ PC10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 10, __GPIO_PC_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_NAME__ PC11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 11, __GPIO_PC_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_NAME__ PC12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 12, __GPIO_PC_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_NAME__ PC13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 13, __GPIO_PC_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_NAME__ PC14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 14, __GPIO_PC_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_NAME__ PC15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 15, __GPIO_PC_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_NAME__ PD0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 0, __GPIO_PD_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_NAME__ PD1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 1, __GPIO_PD_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_NAME__ PD2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 2, __GPIO_PD_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_NAME__ PD3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 3, __GPIO_PD_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_NAME__ PD4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 4, __GPIO_PD_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_NAME__ PD5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 5, __GPIO_PD_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_NAME__ PD6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 6, __GPIO_PD_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_NAME__ PD7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 7, __GPIO_PD_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_NAME__ PD8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 8, __GPIO_PD_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_NAME__ PD9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 9, __GPIO_PD_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_NAME__ PD10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 10, __GPIO_PD_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_NAME__ PD11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 11, __GPIO_PD_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_NAME__ PD12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 12, __GPIO_PD_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_NAME__ PD13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 13, __GPIO_PD_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_NAME__ PD14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 14, __GPIO_PD_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_NAME__ PD15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 15, __GPIO_PD_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_NAME__ PE0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 0, __GPIO_PE_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_NAME__ PE1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 1, __GPIO_PE_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_NAME__ PE2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 2, __GPIO_PE_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_NAME__ PE3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 3, __GPIO_PE_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_NAME__ PE4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 4, __GPIO_PE_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_NAME__ PE5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 5, __GPIO_PE_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_NAME__ PE6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 6, __GPIO_PE_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_NAME__ PE7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 7, __GPIO_PE_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_NAME__ PE8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 8, __GPIO_PE_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_NAME__ PE9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 9, __GPIO_PE_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_NAME__ PE10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 10, __GPIO_PE_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_NAME__ PE11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 11, __GPIO_PE_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_NAME__ PE12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 12, __GPIO_PE_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_NAME__ PE13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 13, __GPIO_PE_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_NAME__ PE14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 14, __GPIO_PE_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_NAME__ PE15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 15, __GPIO_PE_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_NAME__ PF0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 0, __GPIO_PF_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_NAME__ PF1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 1, __GPIO_PF_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_NAME__ PF2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 2, __GPIO_PF_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_NAME__ PF3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 3, __GPIO_PF_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_NAME__ PF4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 4, __GPIO_PF_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_NAME__ PF5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 5, __GPIO_PF_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_NAME__ PF6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 6, __GPIO_PF_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_NAME__ PF7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 7, __GPIO_PF_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_NAME__ PF8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 8, __GPIO_PF_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_NAME__ PF9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 9, __GPIO_PF_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_NAME__ PF10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 10, __GPIO_PF_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_NAME__ PF11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 11, __GPIO_PF_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_NAME__ PF12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 12, __GPIO_PF_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_NAME__ PF13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 13, __GPIO_PF_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_NAME__ PF14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 14, __GPIO_PF_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_NAME__ PF15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 15, __GPIO_PF_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_NAME__ PG0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 0, __GPIO_PG_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_NAME__ PG1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 1, __GPIO_PG_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_NAME__ PG2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 2, __GPIO_PG_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_NAME__ PG3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 3, __GPIO_PG_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_NAME__ PG4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 4, __GPIO_PG_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_NAME__ PG5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 5, __GPIO_PG_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_NAME__ PG6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 6, __GPIO_PG_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_NAME__ PG7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 7, __GPIO_PG_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_NAME__ PG8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 8, __GPIO_PG_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_NAME__ PG9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 9, __GPIO_PG_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_NAME__ PG10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 10, __GPIO_PG_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_NAME__ PG11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 11, __GPIO_PG_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_NAME__ PG12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 12, __GPIO_PG_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_NAME__ PG13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 13, __GPIO_PG_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_NAME__ PG14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 14, __GPIO_PG_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_STATE__ _HIGH

/*--
this:AddPinNameWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_NAME__ PG15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 15, __GPIO_PG_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_NAME__ PH0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 0, __GPIO_PH_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_NAME__ PH1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 1, __GPIO_PH_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_NAME__ PH2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 2, __GPIO_PH_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_NAME__ PH3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 3, __GPIO_PH_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_NAME__ PH4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 4, __GPIO_PH_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_NAME__ PH5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 5, __GPIO_PH_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_NAME__ PH6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 6, __GPIO_PH_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_NAME__ PH7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 7, __GPIO_PH_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_NAME__ PH8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 8, __GPIO_PH_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_NAME__ PH9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 9, __GPIO_PH_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_NAME__ PH10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 10, __GPIO_PH_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_NAME__ PH11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 11, __GPIO_PH_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_NAME__ PH12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 12, __GPIO_PH_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_NAME__ PH13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 13, __GPIO_PH_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_NAME__ PH14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 14, __GPIO_PH_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_NAME__ PH15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 15, __GPIO_PH_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 0)
--*/
#define __GPIO_PI_PIN_0_NAME__ PI0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 0, __GPIO_PI_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("I", 0)
--*/
#define __GPIO_PI_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 0)
--*/
#define __GPIO_PI_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 0)
--*/
#define __GPIO_PI_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("I", 0)
--*/
#define __GPIO_PI_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 1)
--*/
#define __GPIO_PI_PIN_1_NAME__ PI1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 1, __GPIO_PI_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("I", 1)
--*/
#define __GPIO_PI_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 1)
--*/
#define __GPIO_PI_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 1)
--*/
#define __GPIO_PI_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("I", 1)
--*/
#define __GPIO_PI_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 2)
--*/
#define __GPIO_PI_PIN_2_NAME__ PI2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 2, __GPIO_PI_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("I", 2)
--*/
#define __GPIO_PI_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 2)
--*/
#define __GPIO_PI_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 2)
--*/
#define __GPIO_PI_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("I", 2)
--*/
#define __GPIO_PI_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 3)
--*/
#define __GPIO_PI_PIN_3_NAME__ PI3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 3, __GPIO_PI_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("I", 3)
--*/
#define __GPIO_PI_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 3)
--*/
#define __GPIO_PI_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 3)
--*/
#define __GPIO_PI_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("I", 3)
--*/
#define __GPIO_PI_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 4)
--*/
#define __GPIO_PI_PIN_4_NAME__ PI4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 4, __GPIO_PI_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("I", 4)
--*/
#define __GPIO_PI_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 4)
--*/
#define __GPIO_PI_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 4)
--*/
#define __GPIO_PI_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("I", 4)
--*/
#define __GPIO_PI_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 5)
--*/
#define __GPIO_PI_PIN_5_NAME__ PI5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 5, __GPIO_PI_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("I", 5)
--*/
#define __GPIO_PI_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 5)
--*/
#define __GPIO_PI_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 5)
--*/
#define __GPIO_PI_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("I", 5)
--*/
#define __GPIO_PI_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 6)
--*/
#define __GPIO_PI_PIN_6_NAME__ PI6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 6, __GPIO_PI_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("I", 6)
--*/
#define __GPIO_PI_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 6)
--*/
#define __GPIO_PI_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 6)
--*/
#define __GPIO_PI_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("I", 6)
--*/
#define __GPIO_PI_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 7)
--*/
#define __GPIO_PI_PIN_7_NAME__ PI7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 7, __GPIO_PI_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("I", 7)
--*/
#define __GPIO_PI_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 7)
--*/
#define __GPIO_PI_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 7)
--*/
#define __GPIO_PI_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("I", 7)
--*/
#define __GPIO_PI_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 8)
--*/
#define __GPIO_PI_PIN_8_NAME__ PI8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 8, __GPIO_PI_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("I", 8)
--*/
#define __GPIO_PI_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 8)
--*/
#define __GPIO_PI_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 8)
--*/
#define __GPIO_PI_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("I", 8)
--*/
#define __GPIO_PI_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 9)
--*/
#define __GPIO_PI_PIN_9_NAME__ PI9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 9, __GPIO_PI_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("I", 9)
--*/
#define __GPIO_PI_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 9)
--*/
#define __GPIO_PI_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 9)
--*/
#define __GPIO_PI_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("I", 9)
--*/
#define __GPIO_PI_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 10)
--*/
#define __GPIO_PI_PIN_10_NAME__ PI10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 10, __GPIO_PI_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("I", 10)
--*/
#define __GPIO_PI_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 10)
--*/
#define __GPIO_PI_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 10)
--*/
#define __GPIO_PI_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("I", 10)
--*/
#define __GPIO_PI_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 11)
--*/
#define __GPIO_PI_PIN_11_NAME__ PI11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 11, __GPIO_PI_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("I", 11)
--*/
#define __GPIO_PI_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 11)
--*/
#define __GPIO_PI_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 11)
--*/
#define __GPIO_PI_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("I", 11)
--*/
#define __GPIO_PI_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 12)
--*/
#define __GPIO_PI_PIN_12_NAME__ PI12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 12, __GPIO_PI_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("I", 12)
--*/
#define __GPIO_PI_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 12)
--*/
#define __GPIO_PI_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 12)
--*/
#define __GPIO_PI_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("I", 12)
--*/
#define __GPIO_PI_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 13)
--*/
#define __GPIO_PI_PIN_13_NAME__ PI13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 13, __GPIO_PI_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("I", 13)
--*/
#define __GPIO_PI_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 13)
--*/
#define __GPIO_PI_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 13)
--*/
#define __GPIO_PI_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("I", 13)
--*/
#define __GPIO_PI_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 14)
--*/
#define __GPIO_PI_PIN_14_NAME__ PI14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 14, __GPIO_PI_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("I", 14)
--*/
#define __GPIO_PI_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 14)
--*/
#define __GPIO_PI_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 14)
--*/
#define __GPIO_PI_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("I", 14)
--*/
#define __GPIO_PI_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("I", 15)
--*/
#define __GPIO_PI_PIN_15_NAME__ PI15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTI_IDX, 15, __GPIO_PI_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("I", 15)
--*/
#define __GPIO_PI_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("I", 15)
--*/
#define __GPIO_PI_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("I", 15)
--*/
#define __GPIO_PI_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("I", 15)
--*/
#define __GPIO_PI_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 0)
--*/
#define __GPIO_PJ_PIN_0_NAME__ PJ0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 0, __GPIO_PJ_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("J", 0)
--*/
#define __GPIO_PJ_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 0)
--*/
#define __GPIO_PJ_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 0)
--*/
#define __GPIO_PJ_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("J", 0)
--*/
#define __GPIO_PJ_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 1)
--*/
#define __GPIO_PJ_PIN_1_NAME__ PJ1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 1, __GPIO_PJ_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("J", 1)
--*/
#define __GPIO_PJ_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 1)
--*/
#define __GPIO_PJ_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 1)
--*/
#define __GPIO_PJ_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("J", 1)
--*/
#define __GPIO_PJ_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 2)
--*/
#define __GPIO_PJ_PIN_2_NAME__ PJ2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 2, __GPIO_PJ_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("J", 2)
--*/
#define __GPIO_PJ_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 2)
--*/
#define __GPIO_PJ_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 2)
--*/
#define __GPIO_PJ_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("J", 2)
--*/
#define __GPIO_PJ_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 3)
--*/
#define __GPIO_PJ_PIN_3_NAME__ PJ3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 3, __GPIO_PJ_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("J", 3)
--*/
#define __GPIO_PJ_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 3)
--*/
#define __GPIO_PJ_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 3)
--*/
#define __GPIO_PJ_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("J", 3)
--*/
#define __GPIO_PJ_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 4)
--*/
#define __GPIO_PJ_PIN_4_NAME__ PJ4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 4, __GPIO_PJ_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("J", 4)
--*/
#define __GPIO_PJ_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 4)
--*/
#define __GPIO_PJ_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 4)
--*/
#define __GPIO_PJ_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("J", 4)
--*/
#define __GPIO_PJ_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 5)
--*/
#define __GPIO_PJ_PIN_5_NAME__ PJ5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 5, __GPIO_PJ_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("J", 5)
--*/
#define __GPIO_PJ_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 5)
--*/
#define __GPIO_PJ_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 5)
--*/
#define __GPIO_PJ_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("J", 5)
--*/
#define __GPIO_PJ_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 6)
--*/
#define __GPIO_PJ_PIN_6_NAME__ PJ6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 6, __GPIO_PJ_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("J", 6)
--*/
#define __GPIO_PJ_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 6)
--*/
#define __GPIO_PJ_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 6)
--*/
#define __GPIO_PJ_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("J", 6)
--*/
#define __GPIO_PJ_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 7)
--*/
#define __GPIO_PJ_PIN_7_NAME__ PJ7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 7, __GPIO_PJ_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("J", 7)
--*/
#define __GPIO_PJ_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 7)
--*/
#define __GPIO_PJ_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 7)
--*/
#define __GPIO_PJ_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("J", 7)
--*/
#define __GPIO_PJ_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 8)
--*/
#define __GPIO_PJ_PIN_8_NAME__ PJ8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 8, __GPIO_PJ_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("J", 8)
--*/
#define __GPIO_PJ_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 8)
--*/
#define __GPIO_PJ_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 8)
--*/
#define __GPIO_PJ_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("J", 8)
--*/
#define __GPIO_PJ_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 9)
--*/
#define __GPIO_PJ_PIN_9_NAME__ PJ9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 9, __GPIO_PJ_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("J", 9)
--*/
#define __GPIO_PJ_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 9)
--*/
#define __GPIO_PJ_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 9)
--*/
#define __GPIO_PJ_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("J", 9)
--*/
#define __GPIO_PJ_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 10)
--*/
#define __GPIO_PJ_PIN_10_NAME__ PJ10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 10, __GPIO_PJ_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("J", 10)
--*/
#define __GPIO_PJ_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 10)
--*/
#define __GPIO_PJ_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 10)
--*/
#define __GPIO_PJ_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("J", 10)
--*/
#define __GPIO_PJ_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 11)
--*/
#define __GPIO_PJ_PIN_11_NAME__ PJ11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 11, __GPIO_PJ_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("J", 11)
--*/
#define __GPIO_PJ_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 11)
--*/
#define __GPIO_PJ_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 11)
--*/
#define __GPIO_PJ_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("J", 11)
--*/
#define __GPIO_PJ_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 12)
--*/
#define __GPIO_PJ_PIN_12_NAME__ PJ12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 12, __GPIO_PJ_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("J", 12)
--*/
#define __GPIO_PJ_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 12)
--*/
#define __GPIO_PJ_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 12)
--*/
#define __GPIO_PJ_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("J", 12)
--*/
#define __GPIO_PJ_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 13)
--*/
#define __GPIO_PJ_PIN_13_NAME__ PJ13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 13, __GPIO_PJ_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("J", 13)
--*/
#define __GPIO_PJ_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 13)
--*/
#define __GPIO_PJ_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 13)
--*/
#define __GPIO_PJ_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("J", 13)
--*/
#define __GPIO_PJ_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 14)
--*/
#define __GPIO_PJ_PIN_14_NAME__ PJ14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 14, __GPIO_PJ_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("J", 14)
--*/
#define __GPIO_PJ_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 14)
--*/
#define __GPIO_PJ_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 14)
--*/
#define __GPIO_PJ_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("J", 14)
--*/
#define __GPIO_PJ_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("J", 15)
--*/
#define __GPIO_PJ_PIN_15_NAME__ PJ15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTJ_IDX, 15, __GPIO_PJ_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("J", 15)
--*/
#define __GPIO_PJ_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("J", 15)
--*/
#define __GPIO_PJ_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("J", 15)
--*/
#define __GPIO_PJ_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("J", 15)
--*/
#define __GPIO_PJ_PIN_15_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 0)
--*/
#define __GPIO_PK_PIN_0_NAME__ PK0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 0, __GPIO_PK_PIN_0_NAME__);

/*--
this:AddPinSpeedWidget("K", 0)
--*/
#define __GPIO_PK_PIN_0_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 0)
--*/
#define __GPIO_PK_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 0)
--*/
#define __GPIO_PK_PIN_0_AF__ 0

/*--
this:AddPinStateWidget("K", 0)
--*/
#define __GPIO_PK_PIN_0_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 1)
--*/
#define __GPIO_PK_PIN_1_NAME__ PK1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 1, __GPIO_PK_PIN_1_NAME__);

/*--
this:AddPinSpeedWidget("K", 1)
--*/
#define __GPIO_PK_PIN_1_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 1)
--*/
#define __GPIO_PK_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 1)
--*/
#define __GPIO_PK_PIN_1_AF__ 0

/*--
this:AddPinStateWidget("K", 1)
--*/
#define __GPIO_PK_PIN_1_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 2)
--*/
#define __GPIO_PK_PIN_2_NAME__ PK2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 2, __GPIO_PK_PIN_2_NAME__);

/*--
this:AddPinSpeedWidget("K", 2)
--*/
#define __GPIO_PK_PIN_2_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 2)
--*/
#define __GPIO_PK_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 2)
--*/
#define __GPIO_PK_PIN_2_AF__ 0

/*--
this:AddPinStateWidget("K", 2)
--*/
#define __GPIO_PK_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 3)
--*/
#define __GPIO_PK_PIN_3_NAME__ PK3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 3, __GPIO_PK_PIN_3_NAME__);

/*--
this:AddPinSpeedWidget("K", 3)
--*/
#define __GPIO_PK_PIN_3_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 3)
--*/
#define __GPIO_PK_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 3)
--*/
#define __GPIO_PK_PIN_3_AF__ 0

/*--
this:AddPinStateWidget("K", 3)
--*/
#define __GPIO_PK_PIN_3_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 4)
--*/
#define __GPIO_PK_PIN_4_NAME__ PK4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 4, __GPIO_PK_PIN_4_NAME__);

/*--
this:AddPinSpeedWidget("K", 4)
--*/
#define __GPIO_PK_PIN_4_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 4)
--*/
#define __GPIO_PK_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 4)
--*/
#define __GPIO_PK_PIN_4_AF__ 0

/*--
this:AddPinStateWidget("K", 4)
--*/
#define __GPIO_PK_PIN_4_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 5)
--*/
#define __GPIO_PK_PIN_5_NAME__ PK5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 5, __GPIO_PK_PIN_5_NAME__);

/*--
this:AddPinSpeedWidget("K", 5)
--*/
#define __GPIO_PK_PIN_5_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 5)
--*/
#define __GPIO_PK_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 5)
--*/
#define __GPIO_PK_PIN_5_AF__ 0

/*--
this:AddPinStateWidget("K", 5)
--*/
#define __GPIO_PK_PIN_5_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 6)
--*/
#define __GPIO_PK_PIN_6_NAME__ PK6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 6, __GPIO_PK_PIN_6_NAME__);

/*--
this:AddPinSpeedWidget("K", 6)
--*/
#define __GPIO_PK_PIN_6_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 6)
--*/
#define __GPIO_PK_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 6)
--*/
#define __GPIO_PK_PIN_6_AF__ 0

/*--
this:AddPinStateWidget("K", 6)
--*/
#define __GPIO_PK_PIN_6_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 7)
--*/
#define __GPIO_PK_PIN_7_NAME__ PK7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 7, __GPIO_PK_PIN_7_NAME__);

/*--
this:AddPinSpeedWidget("K", 7)
--*/
#define __GPIO_PK_PIN_7_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 7)
--*/
#define __GPIO_PK_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 7)
--*/
#define __GPIO_PK_PIN_7_AF__ 0

/*--
this:AddPinStateWidget("K", 7)
--*/
#define __GPIO_PK_PIN_7_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 8)
--*/
#define __GPIO_PK_PIN_8_NAME__ PK8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 8, __GPIO_PK_PIN_8_NAME__);

/*--
this:AddPinSpeedWidget("K", 8)
--*/
#define __GPIO_PK_PIN_8_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 8)
--*/
#define __GPIO_PK_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 8)
--*/
#define __GPIO_PK_PIN_8_AF__ 0

/*--
this:AddPinStateWidget("K", 8)
--*/
#define __GPIO_PK_PIN_8_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 9)
--*/
#define __GPIO_PK_PIN_9_NAME__ PK9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 9, __GPIO_PK_PIN_9_NAME__);

/*--
this:AddPinSpeedWidget("K", 9)
--*/
#define __GPIO_PK_PIN_9_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 9)
--*/
#define __GPIO_PK_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 9)
--*/
#define __GPIO_PK_PIN_9_AF__ 0

/*--
this:AddPinStateWidget("K", 9)
--*/
#define __GPIO_PK_PIN_9_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 10)
--*/
#define __GPIO_PK_PIN_10_NAME__ PK10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 10, __GPIO_PK_PIN_10_NAME__);

/*--
this:AddPinSpeedWidget("K", 10)
--*/
#define __GPIO_PK_PIN_10_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 10)
--*/
#define __GPIO_PK_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 10)
--*/
#define __GPIO_PK_PIN_10_AF__ 0

/*--
this:AddPinStateWidget("K", 10)
--*/
#define __GPIO_PK_PIN_10_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 11)
--*/
#define __GPIO_PK_PIN_11_NAME__ PK11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 11, __GPIO_PK_PIN_11_NAME__);

/*--
this:AddPinSpeedWidget("K", 11)
--*/
#define __GPIO_PK_PIN_11_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 11)
--*/
#define __GPIO_PK_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 11)
--*/
#define __GPIO_PK_PIN_11_AF__ 0

/*--
this:AddPinStateWidget("K", 11)
--*/
#define __GPIO_PK_PIN_11_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 12)
--*/
#define __GPIO_PK_PIN_12_NAME__ PK12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 12, __GPIO_PK_PIN_12_NAME__);

/*--
this:AddPinSpeedWidget("K", 12)
--*/
#define __GPIO_PK_PIN_12_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 12)
--*/
#define __GPIO_PK_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 12)
--*/
#define __GPIO_PK_PIN_12_AF__ 0

/*--
this:AddPinStateWidget("K", 12)
--*/
#define __GPIO_PK_PIN_12_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 13)
--*/
#define __GPIO_PK_PIN_13_NAME__ PK13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 13, __GPIO_PK_PIN_13_NAME__);

/*--
this:AddPinSpeedWidget("K", 13)
--*/
#define __GPIO_PK_PIN_13_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 13)
--*/
#define __GPIO_PK_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 13)
--*/
#define __GPIO_PK_PIN_13_AF__ 0

/*--
this:AddPinStateWidget("K", 13)
--*/
#define __GPIO_PK_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 14)
--*/
#define __GPIO_PK_PIN_14_NAME__ PK14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 14, __GPIO_PK_PIN_14_NAME__);

/*--
this:AddPinSpeedWidget("K", 14)
--*/
#define __GPIO_PK_PIN_14_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 14)
--*/
#define __GPIO_PK_PIN_14_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 14)
--*/
#define __GPIO_PK_PIN_14_AF__ 0

/*--
this:AddPinStateWidget("K", 14)
--*/
#define __GPIO_PK_PIN_14_STATE__ _LOW

/*--
this:AddPinNameWidget("K", 15)
--*/
#define __GPIO_PK_PIN_15_NAME__ PK15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTK_IDX, 15, __GPIO_PK_PIN_15_NAME__);

/*--
this:AddPinSpeedWidget("K", 15)
--*/
#define __GPIO_PK_PIN_15_SPEED__ _GPIO_SPEED_LOW

/*--
this:AddPinModeWidget("K", 15)
--*/
#define __GPIO_PK_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinAltFuncWidget("K", 15)
--*/
#define __GPIO_PK_PIN_15_AF__ 0

/*--
this:AddPinStateWidget("K", 15)
--*/
#define __GPIO_PK_PIN_15_STATE__ _LOW

#endif /* _GPIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
