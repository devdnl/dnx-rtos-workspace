/*==============================================================================
@file    gpio_flags.h

@author  Daniel Zorychta

@brief   GPIO module configuration flags.

@note    Copyright (C) 2014 Daniel Zorychta <daniel.zorychta@gmail.com>

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
local pack = uC.NAME:gsub("STM32F%d+(.).*", "%1")

if this.state.gpio_menu == nil then
    this:SetTitle("Home > Microcontroller > GPIO", "The GPIO module configuration.")

    for i = 1, #port do
        if PACKAGE[pack][port[i]] ~= nil then
            local gpio = port[i]
            this:AddExtraWidget("Submenu", "HL_GPIO"..gpio, gpio .. " GPIO"..gpio, "arch/stm32f1/gpio_flags.h")
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
            this:AddExtraWidget("Submenu", "HL_PIN"..pin, pin.." P"..gpio..pin, "arch/stm32f1/gpio_flags.h")
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

    this.AddPinModeWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "Mode")
            this:AddItem("Output Push-Pull 2 MHz", "_GPIO_OUT_PUSH_PULL_2MHZ")
            this:AddItem("Output Push-Pull 10 MHz", "_GPIO_OUT_PUSH_PULL_10MHZ")
            this:AddItem("Output Push-Pull 50 MHz", "_GPIO_OUT_PUSH_PULL_50MHZ")
            this:AddItem("Output Open drain 2 MHz", "_GPIO_OUT_OPEN_DRAIN_2MHZ")
            this:AddItem("Output Open drain 10 MHz", "_GPIO_OUT_OPEN_DRAIN_10MHZ")
            this:AddItem("Output Open drain 50 MHz", "_GPIO_OUT_OPEN_DRAIN_50MHZ")
            this:AddItem("Alternative output Push-Pull 2 MHz", "_GPIO_ALT_OUT_PUSH_PULL_2MHZ")
            this:AddItem("Alternative output Push-Pull 10 MHz", "_GPIO_ALT_OUT_PUSH_PULL_10MHZ")
            this:AddItem("Alternative output Push-Pull 50 MHz", "_GPIO_ALT_OUT_PUSH_PULL_50MHZ")
            this:AddItem("Alternative output Open drain 2 MHz", "_GPIO_ALT_OUT_OPEN_DRAIN_2MHZ")
            this:AddItem("Alternative output Open drain 10 MHz", "_GPIO_ALT_OUT_OPEN_DRAIN_10MHZ")
            this:AddItem("Alternative output Open drain 50 MHz", "_GPIO_ALT_OUT_OPEN_DRAIN_50MHZ")
            this:AddItem("Analog", "_GPIO_ANALOG")
            this:AddItem("Float input", "_GPIO_IN_FLOAT")
            this:AddItem("Input pulled", "_GPIO_IN_PULLED")
        end
    end

    this.AddPinStateWidget = function(this, portLetter, pinNumber)
        if PinExist(this, portLetter, pinNumber) then
            this:AddWidget("Combobox", "State")
            this:AddItem("Low", "_LOW")
            this:AddItem("High", "_HIGH")
            this:AddItem("Floating", "_FLOAT")
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
this:AddPinModeWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_STATE__ _FLOAT

/*--
this:AddPinNameWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_NAME__ JTCK_SWCLK
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 14, __GPIO_PA_PIN_14_NAME__);

/*--
this:AddPinModeWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_STATE__ _FLOAT

/*--
this:AddPinNameWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_NAME__ JTDI
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 15, __GPIO_PA_PIN_15_NAME__);

/*--
this:AddPinModeWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_STATE__ _FLOAT

 /*--
this:AddPinNameWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_NAME__ PB0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 0, __GPIO_PB_PIN_0_NAME__);

/*--
this:AddPinModeWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_STATE__ _LOW

/*--
this:AddPinNameWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_NAME__ JTDO
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 3, __GPIO_PB_PIN_3_NAME__);

/*--
this:AddPinModeWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_STATE__ _FLOAT

/*--
this:AddPinNameWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_NAME__ JTRST
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 4, __GPIO_PB_PIN_4_NAME__);

/*--
this:AddPinModeWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_STATE__ _FLOAT

/*--
this:AddPinNameWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_NAME__ PB5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 5, __GPIO_PB_PIN_5_NAME__);

/*--
this:AddPinModeWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_STATE__ _LOW

/*--
this:AddPinNameWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_NAME__ OSC32_IN
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 14, __GPIO_PC_PIN_14_NAME__);

/*--
this:AddPinModeWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_STATE__ _FLOAT

/*--
this:AddPinNameWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_NAME__ OSC32_OUT
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 15, __GPIO_PC_PIN_15_NAME__);

/*--
this:AddPinModeWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_STATE__ _FLOAT


/*--
this:AddPinNameWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_NAME__ PD0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 0, __GPIO_PD_PIN_0_NAME__);

/*--
this:AddPinModeWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_STATE__ _FLOAT

/*--
this:AddPinNameWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_NAME__ PD12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 12, __GPIO_PD_PIN_12_NAME__);

/*--
this:AddPinModeWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_STATE__ _FLOAT

/*--
this:AddPinNameWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_NAME__ PD13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 13, __GPIO_PD_PIN_13_NAME__);

/*--
this:AddPinModeWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_STATE__ _FLOAT

/*--
this:AddPinNameWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_NAME__ PD14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 14, __GPIO_PD_PIN_14_NAME__);

/*--
this:AddPinModeWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_MODE__ _GPIO_ANALOG

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
this:AddPinModeWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_MODE__ _GPIO_ANALOG

/*--
this:AddPinStateWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_STATE__ _LOW

 /*--
this:AddPinNameWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_NAME__ NC_GPIOF_0
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 0, __GPIO_PF_PIN_0_NAME__);

/*--
this:AddPinModeWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_NAME__ NC_GPIOF_1
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 1, __GPIO_PF_PIN_1_NAME__);

/*--
this:AddPinModeWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_NAME__ NC_GPIOF_2
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 2, __GPIO_PF_PIN_2_NAME__);

/*--
this:AddPinModeWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_NAME__ NC_GPIOF_3
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 3, __GPIO_PF_PIN_3_NAME__);

/*--
this:AddPinModeWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_NAME__ NC_GPIOF_4
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 4, __GPIO_PF_PIN_4_NAME__);

/*--
this:AddPinModeWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_NAME__ NC_GPIOF_5
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 5, __GPIO_PF_PIN_5_NAME__);

/*--
this:AddPinModeWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_NAME__ NC_GPIOF_6
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 6, __GPIO_PF_PIN_6_NAME__);

/*--
this:AddPinModeWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_NAME__ NC_GPIOF_7
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 7, __GPIO_PF_PIN_7_NAME__);

/*--
this:AddPinModeWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_NAME__ NC_GPIOF_8
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 8, __GPIO_PF_PIN_8_NAME__);

/*--
this:AddPinModeWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_NAME__ NC_GPIOF_9
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 9, __GPIO_PF_PIN_9_NAME__);

/*--
this:AddPinModeWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_NAME__ NC_GPIOF_10
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 10, __GPIO_PF_PIN_10_NAME__);

/*--
this:AddPinModeWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_NAME__ NC_GPIOF_11
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 11, __GPIO_PF_PIN_11_NAME__);

/*--
this:AddPinModeWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_NAME__ NC_GPIOF_12
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 12, __GPIO_PF_PIN_12_NAME__);

/*--
this:AddPinModeWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_NAME__ NC_GPIOF_13
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 13, __GPIO_PF_PIN_13_NAME__);

/*--
this:AddPinModeWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_NAME__ NC_GPIOF_14
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 14, __GPIO_PF_PIN_14_NAME__);

/*--
this:AddPinModeWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_STATE__ _FLOAT

/*--
this:AddPinNameWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_NAME__ NC_GPIOF_15
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 15, __GPIO_PF_PIN_15_NAME__);

/*--
this:AddPinModeWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_STATE__ _FLOAT


/*--
this:AddPinNameWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_NAME__ NC_GPIOG_0
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 0, __GPIO_PG_PIN_0_NAME__);

/*--
this:AddPinModeWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_NAME__ NC_GPIOG_1
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 1, __GPIO_PG_PIN_1_NAME__);

/*--
this:AddPinModeWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_NAME__ NC_GPIOG_2
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 2, __GPIO_PG_PIN_2_NAME__);

/*--
this:AddPinModeWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_NAME__ NC_GPIOG_3
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 3, __GPIO_PG_PIN_3_NAME__);

/*--
this:AddPinModeWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_NAME__ NC_GPIOG_4
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 4, __GPIO_PG_PIN_4_NAME__);

/*--
this:AddPinModeWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_NAME__ NC_GPIOG_5
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 5, __GPIO_PG_PIN_5_NAME__);

/*--
this:AddPinModeWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_NAME__ NC_GPIOG_6
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 6, __GPIO_PG_PIN_6_NAME__);

/*--
this:AddPinModeWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_NAME__ NC_GPIOG_7
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 7, __GPIO_PG_PIN_7_NAME__);

/*--
this:AddPinModeWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_NAME__ NC_GPIOG_8
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 8, __GPIO_PG_PIN_8_NAME__);

/*--
this:AddPinModeWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_NAME__ NC_GPIOG_9
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 9, __GPIO_PG_PIN_9_NAME__);

/*--
this:AddPinModeWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_NAME__ NC_GPIOG_10
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 10, __GPIO_PG_PIN_10_NAME__);

/*--
this:AddPinModeWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_NAME__ NC_GPIOG_11
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 11, __GPIO_PG_PIN_11_NAME__);

/*--
this:AddPinModeWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_NAME__ NC_GPIOG_12
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 12, __GPIO_PG_PIN_12_NAME__);

/*--
this:AddPinModeWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_NAME__ NC_GPIOG_13
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 13, __GPIO_PG_PIN_13_NAME__);

/*--
this:AddPinModeWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_NAME__ NC_GPIOG_14
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 14, __GPIO_PG_PIN_14_NAME__);

/*--
this:AddPinModeWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_STATE__ _FLOAT

/*--
this:AddPinNameWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_NAME__ NC_GPIOG_15
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 15, __GPIO_PG_PIN_15_NAME__);

/*--
this:AddPinModeWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_STATE__ _FLOAT


/*--
this:AddPinNameWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_NAME__ PH0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 0, __GPIO_PH_PIN_0_NAME__);

/*--
this:AddPinModeWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_MODE__ _GPIO_IN_FLOAT

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
this:AddPinModeWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_MODE__ _GPIO_IN_FLOAT

/*--
this:AddPinStateWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_STATE__ _LOW


#endif /* _GPIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
