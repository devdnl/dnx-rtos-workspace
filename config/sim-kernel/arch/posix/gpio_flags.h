/*==============================================================================
@file    gpio_flags.h

@author  Daniel Zorychta

@brief   GPIO module configuration flags (POSIX simulator).

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
local port = {"A", "B", "C", "D", "E", "F", "G", "H"}

local function OpenPortMenu(port, pin) this.state.gpio_menu = {port, pin} end
local function ResetMenu() this.state.gpio_menu = nil end

if this.state.gpio_menu == nil then
    this:SetTitle("Home > Microcontroller > GPIO", "The GPIO module configuration.")

    for i = 1, #port do
        local gpio = port[i]
        this:AddExtraWidget("Submenu", "HL_GPIO"..gpio, gpio .. " GPIO"..gpio, "arch/posix/gpio_flags.h")
        this:SetEvent("clicked", "HL_GPIO"..gpio, function() OpenPortMenu(gpio, nil) end)
        this:SetEvent("BackClicked", function() ResetMenu() end)
    end

    this:Finish()

elseif this.state.gpio_menu[1] and (this.state.gpio_menu[2] == nil) then
    local gpio = this.state.gpio_menu[1]

    this:SetTitle("Home > Microcontroller > GPIO"..gpio, "The GPIO"..gpio.." port configuration.")

    for pin = 0, 31 do
        this:AddExtraWidget("Submenu", "HL_PIN"..pin, pin.." P"..gpio..pin, "arch/posix/gpio_flags.h")
        this:SetEvent("clicked", "HL_PIN"..pin, function() OpenPortMenu(gpio, pin) end)
    end

    this:SetEvent("BackClicked", function() ResetMenu() end)

    this:Finish()

else
    local gpio = this.state.gpio_menu[1]
    local pin = this.state.gpio_menu[2]

    this:SetTitle("Home > Microcontroller > GPIO"..gpio.."."..pin, "GPIO"..gpio.."."..pin.." pin configuration.")
    this:SetEvent("BackClicked", function() OpenPortMenu(gpio, nil) end)

    this.AddPinNameWidget = function(this, portLetter, pinNumber)
        if (this.state.gpio_menu[1] == portLetter) and (this.state.gpio_menu[2] == pinNumber) then
            this:AddWidget("Editline", false, "Name")
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
#define IOCTL_GPIO_PORTNONE_IDX 8

_GPIO_PIN_NAME(IOCTL_GPIO_PORTNONE_IDX, 17, NONE);
/*--
this:AddPinNameWidget("A", 0)
--*/
#define __GPIO_PA_PIN_0_NAME__ PA0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 0, __GPIO_PA_PIN_0_NAME__);

/*--
this:AddPinNameWidget("A", 1)
--*/
#define __GPIO_PA_PIN_1_NAME__ PA1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 1, __GPIO_PA_PIN_1_NAME__);

/*--
this:AddPinNameWidget("A", 2)
--*/
#define __GPIO_PA_PIN_2_NAME__ PA2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 2, __GPIO_PA_PIN_2_NAME__);

/*--
this:AddPinNameWidget("A", 3)
--*/
#define __GPIO_PA_PIN_3_NAME__ PA3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 3, __GPIO_PA_PIN_3_NAME__);

/*--
this:AddPinNameWidget("A", 4)
--*/
#define __GPIO_PA_PIN_4_NAME__ PA4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 4, __GPIO_PA_PIN_4_NAME__);

/*--
this:AddPinNameWidget("A", 5)
--*/
#define __GPIO_PA_PIN_5_NAME__ PA5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 5, __GPIO_PA_PIN_5_NAME__);

/*--
this:AddPinNameWidget("A", 6)
--*/
#define __GPIO_PA_PIN_6_NAME__ PA6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 6, __GPIO_PA_PIN_6_NAME__);

/*--
this:AddPinNameWidget("A", 7)
--*/
#define __GPIO_PA_PIN_7_NAME__ PA7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 7, __GPIO_PA_PIN_7_NAME__);

/*--
this:AddPinNameWidget("A", 8)
--*/
#define __GPIO_PA_PIN_8_NAME__ PA8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 8, __GPIO_PA_PIN_8_NAME__);

/*--
this:AddPinNameWidget("A", 9)
--*/
#define __GPIO_PA_PIN_9_NAME__ PA9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 9, __GPIO_PA_PIN_9_NAME__);

/*--
this:AddPinNameWidget("A", 10)
--*/
#define __GPIO_PA_PIN_10_NAME__ PA10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 10, __GPIO_PA_PIN_10_NAME__);

/*--
this:AddPinNameWidget("A", 11)
--*/
#define __GPIO_PA_PIN_11_NAME__ PA11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 11, __GPIO_PA_PIN_11_NAME__);

/*--
this:AddPinNameWidget("A", 12)
--*/
#define __GPIO_PA_PIN_12_NAME__ PA12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 12, __GPIO_PA_PIN_12_NAME__);

/*--
this:AddPinNameWidget("A", 13)
--*/
#define __GPIO_PA_PIN_13_NAME__ PA13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 13, __GPIO_PA_PIN_13_NAME__);

/*--
this:AddPinNameWidget("A", 14)
--*/
#define __GPIO_PA_PIN_14_NAME__ PA14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 14, __GPIO_PA_PIN_14_NAME__);

/*--
this:AddPinNameWidget("A", 15)
--*/
#define __GPIO_PA_PIN_15_NAME__ PA15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 15, __GPIO_PA_PIN_15_NAME__);

/*--
this:AddPinNameWidget("A", 16)
--*/
#define __GPIO_PA_PIN_16_NAME__ PA16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 16, __GPIO_PA_PIN_16_NAME__);

/*--
this:AddPinNameWidget("A", 17)
--*/
#define __GPIO_PA_PIN_17_NAME__ PA17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 17, __GPIO_PA_PIN_17_NAME__);

/*--
this:AddPinNameWidget("A", 18)
--*/
#define __GPIO_PA_PIN_18_NAME__ PA18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 18, __GPIO_PA_PIN_18_NAME__);

/*--
this:AddPinNameWidget("A", 19)
--*/
#define __GPIO_PA_PIN_19_NAME__ PA19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 19, __GPIO_PA_PIN_19_NAME__);

/*--
this:AddPinNameWidget("A", 20)
--*/
#define __GPIO_PA_PIN_20_NAME__ PA20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 20, __GPIO_PA_PIN_20_NAME__);

/*--
this:AddPinNameWidget("A", 21)
--*/
#define __GPIO_PA_PIN_21_NAME__ PA21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 21, __GPIO_PA_PIN_21_NAME__);

/*--
this:AddPinNameWidget("A", 22)
--*/
#define __GPIO_PA_PIN_22_NAME__ PA22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 22, __GPIO_PA_PIN_22_NAME__);

/*--
this:AddPinNameWidget("A", 23)
--*/
#define __GPIO_PA_PIN_23_NAME__ PA23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 23, __GPIO_PA_PIN_23_NAME__);

/*--
this:AddPinNameWidget("A", 24)
--*/
#define __GPIO_PA_PIN_24_NAME__ PA24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 24, __GPIO_PA_PIN_24_NAME__);

/*--
this:AddPinNameWidget("A", 25)
--*/
#define __GPIO_PA_PIN_25_NAME__ PA25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 25, __GPIO_PA_PIN_25_NAME__);

/*--
this:AddPinNameWidget("A", 26)
--*/
#define __GPIO_PA_PIN_26_NAME__ PA26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 26, __GPIO_PA_PIN_26_NAME__);

/*--
this:AddPinNameWidget("A", 27)
--*/
#define __GPIO_PA_PIN_27_NAME__ PA27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 27, __GPIO_PA_PIN_27_NAME__);

/*--
this:AddPinNameWidget("A", 28)
--*/
#define __GPIO_PA_PIN_28_NAME__ PA28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 28, __GPIO_PA_PIN_28_NAME__);

/*--
this:AddPinNameWidget("A", 29)
--*/
#define __GPIO_PA_PIN_29_NAME__ PA29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 29, __GPIO_PA_PIN_29_NAME__);

/*--
this:AddPinNameWidget("A", 30)
--*/
#define __GPIO_PA_PIN_30_NAME__ PA30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 30, __GPIO_PA_PIN_30_NAME__);

/*--
this:AddPinNameWidget("A", 31)
--*/
#define __GPIO_PA_PIN_31_NAME__ PA31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTA_IDX, 31, __GPIO_PA_PIN_31_NAME__);


/*--
this:AddPinNameWidget("B", 0)
--*/
#define __GPIO_PB_PIN_0_NAME__ PB0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 0, __GPIO_PB_PIN_0_NAME__);

/*--
this:AddPinNameWidget("B", 1)
--*/
#define __GPIO_PB_PIN_1_NAME__ PB1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 1, __GPIO_PB_PIN_1_NAME__);

/*--
this:AddPinNameWidget("B", 2)
--*/
#define __GPIO_PB_PIN_2_NAME__ PB2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 2, __GPIO_PB_PIN_2_NAME__);

/*--
this:AddPinNameWidget("B", 3)
--*/
#define __GPIO_PB_PIN_3_NAME__ PB3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 3, __GPIO_PB_PIN_3_NAME__);

/*--
this:AddPinNameWidget("B", 4)
--*/
#define __GPIO_PB_PIN_4_NAME__ PB4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 4, __GPIO_PB_PIN_4_NAME__);

/*--
this:AddPinNameWidget("B", 5)
--*/
#define __GPIO_PB_PIN_5_NAME__ PB5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 5, __GPIO_PB_PIN_5_NAME__);

/*--
this:AddPinNameWidget("B", 6)
--*/
#define __GPIO_PB_PIN_6_NAME__ PB6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 6, __GPIO_PB_PIN_6_NAME__);

/*--
this:AddPinNameWidget("B", 7)
--*/
#define __GPIO_PB_PIN_7_NAME__ PB7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 7, __GPIO_PB_PIN_7_NAME__);

/*--
this:AddPinNameWidget("B", 8)
--*/
#define __GPIO_PB_PIN_8_NAME__ PB8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 8, __GPIO_PB_PIN_8_NAME__);

/*--
this:AddPinNameWidget("B", 9)
--*/
#define __GPIO_PB_PIN_9_NAME__ PB9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 9, __GPIO_PB_PIN_9_NAME__);

/*--
this:AddPinNameWidget("B", 10)
--*/
#define __GPIO_PB_PIN_10_NAME__ PB10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 10, __GPIO_PB_PIN_10_NAME__);

/*--
this:AddPinNameWidget("B", 11)
--*/
#define __GPIO_PB_PIN_11_NAME__ PB11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 11, __GPIO_PB_PIN_11_NAME__);

/*--
this:AddPinNameWidget("B", 12)
--*/
#define __GPIO_PB_PIN_12_NAME__ PB12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 12, __GPIO_PB_PIN_12_NAME__);

/*--
this:AddPinNameWidget("B", 13)
--*/
#define __GPIO_PB_PIN_13_NAME__ PB13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 13, __GPIO_PB_PIN_13_NAME__);

/*--
this:AddPinNameWidget("B", 14)
--*/
#define __GPIO_PB_PIN_14_NAME__ PB14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 14, __GPIO_PB_PIN_14_NAME__);

/*--
this:AddPinNameWidget("B", 15)
--*/
#define __GPIO_PB_PIN_15_NAME__ PB15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 15, __GPIO_PB_PIN_15_NAME__);

/*--
this:AddPinNameWidget("B", 16)
--*/
#define __GPIO_PB_PIN_16_NAME__ PB16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 16, __GPIO_PB_PIN_16_NAME__);

/*--
this:AddPinNameWidget("B", 17)
--*/
#define __GPIO_PB_PIN_17_NAME__ PB17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 17, __GPIO_PB_PIN_17_NAME__);

/*--
this:AddPinNameWidget("B", 18)
--*/
#define __GPIO_PB_PIN_18_NAME__ PB18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 18, __GPIO_PB_PIN_18_NAME__);

/*--
this:AddPinNameWidget("B", 19)
--*/
#define __GPIO_PB_PIN_19_NAME__ PB19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 19, __GPIO_PB_PIN_19_NAME__);

/*--
this:AddPinNameWidget("B", 20)
--*/
#define __GPIO_PB_PIN_20_NAME__ PB20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 20, __GPIO_PB_PIN_20_NAME__);

/*--
this:AddPinNameWidget("B", 21)
--*/
#define __GPIO_PB_PIN_21_NAME__ PB21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 21, __GPIO_PB_PIN_21_NAME__);

/*--
this:AddPinNameWidget("B", 22)
--*/
#define __GPIO_PB_PIN_22_NAME__ PB22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 22, __GPIO_PB_PIN_22_NAME__);

/*--
this:AddPinNameWidget("B", 23)
--*/
#define __GPIO_PB_PIN_23_NAME__ PB23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 23, __GPIO_PB_PIN_23_NAME__);

/*--
this:AddPinNameWidget("B", 24)
--*/
#define __GPIO_PB_PIN_24_NAME__ PB24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 24, __GPIO_PB_PIN_24_NAME__);

/*--
this:AddPinNameWidget("B", 25)
--*/
#define __GPIO_PB_PIN_25_NAME__ PB25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 25, __GPIO_PB_PIN_25_NAME__);

/*--
this:AddPinNameWidget("B", 26)
--*/
#define __GPIO_PB_PIN_26_NAME__ PB26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 26, __GPIO_PB_PIN_26_NAME__);

/*--
this:AddPinNameWidget("B", 27)
--*/
#define __GPIO_PB_PIN_27_NAME__ PB27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 27, __GPIO_PB_PIN_27_NAME__);

/*--
this:AddPinNameWidget("B", 28)
--*/
#define __GPIO_PB_PIN_28_NAME__ PB28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 28, __GPIO_PB_PIN_28_NAME__);

/*--
this:AddPinNameWidget("B", 29)
--*/
#define __GPIO_PB_PIN_29_NAME__ PB29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 29, __GPIO_PB_PIN_29_NAME__);

/*--
this:AddPinNameWidget("B", 30)
--*/
#define __GPIO_PB_PIN_30_NAME__ PB30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 30, __GPIO_PB_PIN_30_NAME__);

/*--
this:AddPinNameWidget("B", 31)
--*/
#define __GPIO_PB_PIN_31_NAME__ PB31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTB_IDX, 31, __GPIO_PB_PIN_31_NAME__);


/*--
this:AddPinNameWidget("C", 0)
--*/
#define __GPIO_PC_PIN_0_NAME__ PC0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 0, __GPIO_PC_PIN_0_NAME__);

/*--
this:AddPinNameWidget("C", 1)
--*/
#define __GPIO_PC_PIN_1_NAME__ PC1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 1, __GPIO_PC_PIN_1_NAME__);

/*--
this:AddPinNameWidget("C", 2)
--*/
#define __GPIO_PC_PIN_2_NAME__ PC2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 2, __GPIO_PC_PIN_2_NAME__);

/*--
this:AddPinNameWidget("C", 3)
--*/
#define __GPIO_PC_PIN_3_NAME__ PC3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 3, __GPIO_PC_PIN_3_NAME__);

/*--
this:AddPinNameWidget("C", 4)
--*/
#define __GPIO_PC_PIN_4_NAME__ PC4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 4, __GPIO_PC_PIN_4_NAME__);

/*--
this:AddPinNameWidget("C", 5)
--*/
#define __GPIO_PC_PIN_5_NAME__ PC5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 5, __GPIO_PC_PIN_5_NAME__);

/*--
this:AddPinNameWidget("C", 6)
--*/
#define __GPIO_PC_PIN_6_NAME__ PC6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 6, __GPIO_PC_PIN_6_NAME__);

/*--
this:AddPinNameWidget("C", 7)
--*/
#define __GPIO_PC_PIN_7_NAME__ PC7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 7, __GPIO_PC_PIN_7_NAME__);

/*--
this:AddPinNameWidget("C", 8)
--*/
#define __GPIO_PC_PIN_8_NAME__ PC8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 8, __GPIO_PC_PIN_8_NAME__);

/*--
this:AddPinNameWidget("C", 9)
--*/
#define __GPIO_PC_PIN_9_NAME__ PC9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 9, __GPIO_PC_PIN_9_NAME__);

/*--
this:AddPinNameWidget("C", 10)
--*/
#define __GPIO_PC_PIN_10_NAME__ PC10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 10, __GPIO_PC_PIN_10_NAME__);

/*--
this:AddPinNameWidget("C", 11)
--*/
#define __GPIO_PC_PIN_11_NAME__ PC11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 11, __GPIO_PC_PIN_11_NAME__);

/*--
this:AddPinNameWidget("C", 12)
--*/
#define __GPIO_PC_PIN_12_NAME__ PC12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 12, __GPIO_PC_PIN_12_NAME__);

/*--
this:AddPinNameWidget("C", 13)
--*/
#define __GPIO_PC_PIN_13_NAME__ PC13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 13, __GPIO_PC_PIN_13_NAME__);

/*--
this:AddPinNameWidget("C", 14)
--*/
#define __GPIO_PC_PIN_14_NAME__ PC14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 14, __GPIO_PC_PIN_14_NAME__);

/*--
this:AddPinNameWidget("C", 15)
--*/
#define __GPIO_PC_PIN_15_NAME__ PC15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 15, __GPIO_PC_PIN_15_NAME__);

/*--
this:AddPinNameWidget("C", 16)
--*/
#define __GPIO_PC_PIN_16_NAME__ PC16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 16, __GPIO_PC_PIN_16_NAME__);

/*--
this:AddPinNameWidget("C", 17)
--*/
#define __GPIO_PC_PIN_17_NAME__ PC17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 17, __GPIO_PC_PIN_17_NAME__);

/*--
this:AddPinNameWidget("C", 18)
--*/
#define __GPIO_PC_PIN_18_NAME__ PC18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 18, __GPIO_PC_PIN_18_NAME__);

/*--
this:AddPinNameWidget("C", 19)
--*/
#define __GPIO_PC_PIN_19_NAME__ PC19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 19, __GPIO_PC_PIN_19_NAME__);

/*--
this:AddPinNameWidget("C", 20)
--*/
#define __GPIO_PC_PIN_20_NAME__ PC20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 20, __GPIO_PC_PIN_20_NAME__);

/*--
this:AddPinNameWidget("C", 21)
--*/
#define __GPIO_PC_PIN_21_NAME__ PC21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 21, __GPIO_PC_PIN_21_NAME__);

/*--
this:AddPinNameWidget("C", 22)
--*/
#define __GPIO_PC_PIN_22_NAME__ PC22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 22, __GPIO_PC_PIN_22_NAME__);

/*--
this:AddPinNameWidget("C", 23)
--*/
#define __GPIO_PC_PIN_23_NAME__ PC23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 23, __GPIO_PC_PIN_23_NAME__);

/*--
this:AddPinNameWidget("C", 24)
--*/
#define __GPIO_PC_PIN_24_NAME__ PC24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 24, __GPIO_PC_PIN_24_NAME__);

/*--
this:AddPinNameWidget("C", 25)
--*/
#define __GPIO_PC_PIN_25_NAME__ PC25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 25, __GPIO_PC_PIN_25_NAME__);

/*--
this:AddPinNameWidget("C", 26)
--*/
#define __GPIO_PC_PIN_26_NAME__ PC26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 26, __GPIO_PC_PIN_26_NAME__);

/*--
this:AddPinNameWidget("C", 27)
--*/
#define __GPIO_PC_PIN_27_NAME__ PC27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 27, __GPIO_PC_PIN_27_NAME__);

/*--
this:AddPinNameWidget("C", 28)
--*/
#define __GPIO_PC_PIN_28_NAME__ PC28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 28, __GPIO_PC_PIN_28_NAME__);

/*--
this:AddPinNameWidget("C", 29)
--*/
#define __GPIO_PC_PIN_29_NAME__ PC29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 29, __GPIO_PC_PIN_29_NAME__);

/*--
this:AddPinNameWidget("C", 30)
--*/
#define __GPIO_PC_PIN_30_NAME__ PC30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 30, __GPIO_PC_PIN_30_NAME__);

/*--
this:AddPinNameWidget("C", 31)
--*/
#define __GPIO_PC_PIN_31_NAME__ PC31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTC_IDX, 31, __GPIO_PC_PIN_31_NAME__);


/*--
this:AddPinNameWidget("D", 0)
--*/
#define __GPIO_PD_PIN_0_NAME__ PD0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 0, __GPIO_PD_PIN_0_NAME__);

/*--
this:AddPinNameWidget("D", 1)
--*/
#define __GPIO_PD_PIN_1_NAME__ PD1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 1, __GPIO_PD_PIN_1_NAME__);

/*--
this:AddPinNameWidget("D", 2)
--*/
#define __GPIO_PD_PIN_2_NAME__ PD2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 2, __GPIO_PD_PIN_2_NAME__);

/*--
this:AddPinNameWidget("D", 3)
--*/
#define __GPIO_PD_PIN_3_NAME__ PD3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 3, __GPIO_PD_PIN_3_NAME__);

/*--
this:AddPinNameWidget("D", 4)
--*/
#define __GPIO_PD_PIN_4_NAME__ PD4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 4, __GPIO_PD_PIN_4_NAME__);

/*--
this:AddPinNameWidget("D", 5)
--*/
#define __GPIO_PD_PIN_5_NAME__ PD5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 5, __GPIO_PD_PIN_5_NAME__);

/*--
this:AddPinNameWidget("D", 6)
--*/
#define __GPIO_PD_PIN_6_NAME__ PD6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 6, __GPIO_PD_PIN_6_NAME__);

/*--
this:AddPinNameWidget("D", 7)
--*/
#define __GPIO_PD_PIN_7_NAME__ PD7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 7, __GPIO_PD_PIN_7_NAME__);

/*--
this:AddPinNameWidget("D", 8)
--*/
#define __GPIO_PD_PIN_8_NAME__ PD8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 8, __GPIO_PD_PIN_8_NAME__);

/*--
this:AddPinNameWidget("D", 9)
--*/
#define __GPIO_PD_PIN_9_NAME__ PD9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 9, __GPIO_PD_PIN_9_NAME__);

/*--
this:AddPinNameWidget("D", 10)
--*/
#define __GPIO_PD_PIN_10_NAME__ PD10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 10, __GPIO_PD_PIN_10_NAME__);

/*--
this:AddPinNameWidget("D", 11)
--*/
#define __GPIO_PD_PIN_11_NAME__ PD11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 11, __GPIO_PD_PIN_11_NAME__);

/*--
this:AddPinNameWidget("D", 12)
--*/
#define __GPIO_PD_PIN_12_NAME__ PD12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 12, __GPIO_PD_PIN_12_NAME__);

/*--
this:AddPinNameWidget("D", 13)
--*/
#define __GPIO_PD_PIN_13_NAME__ PD13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 13, __GPIO_PD_PIN_13_NAME__);

/*--
this:AddPinNameWidget("D", 14)
--*/
#define __GPIO_PD_PIN_14_NAME__ PD14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 14, __GPIO_PD_PIN_14_NAME__);

/*--
this:AddPinNameWidget("D", 15)
--*/
#define __GPIO_PD_PIN_15_NAME__ PD15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 15, __GPIO_PD_PIN_15_NAME__);

/*--
this:AddPinNameWidget("D", 16)
--*/
#define __GPIO_PD_PIN_16_NAME__ PD16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 16, __GPIO_PD_PIN_16_NAME__);

/*--
this:AddPinNameWidget("D", 17)
--*/
#define __GPIO_PD_PIN_17_NAME__ PD17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 17, __GPIO_PD_PIN_17_NAME__);

/*--
this:AddPinNameWidget("D", 18)
--*/
#define __GPIO_PD_PIN_18_NAME__ PD18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 18, __GPIO_PD_PIN_18_NAME__);

/*--
this:AddPinNameWidget("D", 19)
--*/
#define __GPIO_PD_PIN_19_NAME__ PD19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 19, __GPIO_PD_PIN_19_NAME__);

/*--
this:AddPinNameWidget("D", 20)
--*/
#define __GPIO_PD_PIN_20_NAME__ PD20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 20, __GPIO_PD_PIN_20_NAME__);

/*--
this:AddPinNameWidget("D", 21)
--*/
#define __GPIO_PD_PIN_21_NAME__ PD21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 21, __GPIO_PD_PIN_21_NAME__);

/*--
this:AddPinNameWidget("D", 22)
--*/
#define __GPIO_PD_PIN_22_NAME__ PD22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 22, __GPIO_PD_PIN_22_NAME__);

/*--
this:AddPinNameWidget("D", 23)
--*/
#define __GPIO_PD_PIN_23_NAME__ PD23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 23, __GPIO_PD_PIN_23_NAME__);

/*--
this:AddPinNameWidget("D", 24)
--*/
#define __GPIO_PD_PIN_24_NAME__ PD24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 24, __GPIO_PD_PIN_24_NAME__);

/*--
this:AddPinNameWidget("D", 25)
--*/
#define __GPIO_PD_PIN_25_NAME__ PD25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 25, __GPIO_PD_PIN_25_NAME__);

/*--
this:AddPinNameWidget("D", 26)
--*/
#define __GPIO_PD_PIN_26_NAME__ PD26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 26, __GPIO_PD_PIN_26_NAME__);

/*--
this:AddPinNameWidget("D", 27)
--*/
#define __GPIO_PD_PIN_27_NAME__ PD27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 27, __GPIO_PD_PIN_27_NAME__);

/*--
this:AddPinNameWidget("D", 28)
--*/
#define __GPIO_PD_PIN_28_NAME__ PD28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 28, __GPIO_PD_PIN_28_NAME__);

/*--
this:AddPinNameWidget("D", 29)
--*/
#define __GPIO_PD_PIN_29_NAME__ PD29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 29, __GPIO_PD_PIN_29_NAME__);

/*--
this:AddPinNameWidget("D", 30)
--*/
#define __GPIO_PD_PIN_30_NAME__ PD30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 30, __GPIO_PD_PIN_30_NAME__);

/*--
this:AddPinNameWidget("D", 31)
--*/
#define __GPIO_PD_PIN_31_NAME__ PD31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTD_IDX, 31, __GPIO_PD_PIN_31_NAME__);


/*--
this:AddPinNameWidget("E", 0)
--*/
#define __GPIO_PE_PIN_0_NAME__ PE0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 0, __GPIO_PE_PIN_0_NAME__);

/*--
this:AddPinNameWidget("E", 1)
--*/
#define __GPIO_PE_PIN_1_NAME__ PE1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 1, __GPIO_PE_PIN_1_NAME__);

/*--
this:AddPinNameWidget("E", 2)
--*/
#define __GPIO_PE_PIN_2_NAME__ PE2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 2, __GPIO_PE_PIN_2_NAME__);

/*--
this:AddPinNameWidget("E", 3)
--*/
#define __GPIO_PE_PIN_3_NAME__ PE3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 3, __GPIO_PE_PIN_3_NAME__);

/*--
this:AddPinNameWidget("E", 4)
--*/
#define __GPIO_PE_PIN_4_NAME__ PE4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 4, __GPIO_PE_PIN_4_NAME__);

/*--
this:AddPinNameWidget("E", 5)
--*/
#define __GPIO_PE_PIN_5_NAME__ PE5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 5, __GPIO_PE_PIN_5_NAME__);

/*--
this:AddPinNameWidget("E", 6)
--*/
#define __GPIO_PE_PIN_6_NAME__ PE6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 6, __GPIO_PE_PIN_6_NAME__);

/*--
this:AddPinNameWidget("E", 7)
--*/
#define __GPIO_PE_PIN_7_NAME__ PE7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 7, __GPIO_PE_PIN_7_NAME__);

/*--
this:AddPinNameWidget("E", 8)
--*/
#define __GPIO_PE_PIN_8_NAME__ PE8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 8, __GPIO_PE_PIN_8_NAME__);

/*--
this:AddPinNameWidget("E", 9)
--*/
#define __GPIO_PE_PIN_9_NAME__ PE9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 9, __GPIO_PE_PIN_9_NAME__);

/*--
this:AddPinNameWidget("E", 10)
--*/
#define __GPIO_PE_PIN_10_NAME__ PE10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 10, __GPIO_PE_PIN_10_NAME__);

/*--
this:AddPinNameWidget("E", 11)
--*/
#define __GPIO_PE_PIN_11_NAME__ PE11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 11, __GPIO_PE_PIN_11_NAME__);

/*--
this:AddPinNameWidget("E", 12)
--*/
#define __GPIO_PE_PIN_12_NAME__ PE12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 12, __GPIO_PE_PIN_12_NAME__);

/*--
this:AddPinNameWidget("E", 13)
--*/
#define __GPIO_PE_PIN_13_NAME__ PE13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 13, __GPIO_PE_PIN_13_NAME__);

/*--
this:AddPinNameWidget("E", 14)
--*/
#define __GPIO_PE_PIN_14_NAME__ PE14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 14, __GPIO_PE_PIN_14_NAME__);

/*--
this:AddPinNameWidget("E", 15)
--*/
#define __GPIO_PE_PIN_15_NAME__ PE15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 15, __GPIO_PE_PIN_15_NAME__);

/*--
this:AddPinNameWidget("E", 16)
--*/
#define __GPIO_PE_PIN_16_NAME__ PE16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 16, __GPIO_PE_PIN_16_NAME__);

/*--
this:AddPinNameWidget("E", 17)
--*/
#define __GPIO_PE_PIN_17_NAME__ PE17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 17, __GPIO_PE_PIN_17_NAME__);

/*--
this:AddPinNameWidget("E", 18)
--*/
#define __GPIO_PE_PIN_18_NAME__ PE18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 18, __GPIO_PE_PIN_18_NAME__);

/*--
this:AddPinNameWidget("E", 19)
--*/
#define __GPIO_PE_PIN_19_NAME__ PE19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 19, __GPIO_PE_PIN_19_NAME__);

/*--
this:AddPinNameWidget("E", 20)
--*/
#define __GPIO_PE_PIN_20_NAME__ PE20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 20, __GPIO_PE_PIN_20_NAME__);

/*--
this:AddPinNameWidget("E", 21)
--*/
#define __GPIO_PE_PIN_21_NAME__ PE21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 21, __GPIO_PE_PIN_21_NAME__);

/*--
this:AddPinNameWidget("E", 22)
--*/
#define __GPIO_PE_PIN_22_NAME__ PE22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 22, __GPIO_PE_PIN_22_NAME__);

/*--
this:AddPinNameWidget("E", 23)
--*/
#define __GPIO_PE_PIN_23_NAME__ PE23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 23, __GPIO_PE_PIN_23_NAME__);

/*--
this:AddPinNameWidget("E", 24)
--*/
#define __GPIO_PE_PIN_24_NAME__ PE24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 24, __GPIO_PE_PIN_24_NAME__);

/*--
this:AddPinNameWidget("E", 25)
--*/
#define __GPIO_PE_PIN_25_NAME__ PE25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 25, __GPIO_PE_PIN_25_NAME__);

/*--
this:AddPinNameWidget("E", 26)
--*/
#define __GPIO_PE_PIN_26_NAME__ PE26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 26, __GPIO_PE_PIN_26_NAME__);

/*--
this:AddPinNameWidget("E", 27)
--*/
#define __GPIO_PE_PIN_27_NAME__ PE27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 27, __GPIO_PE_PIN_27_NAME__);

/*--
this:AddPinNameWidget("E", 28)
--*/
#define __GPIO_PE_PIN_28_NAME__ PE28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 28, __GPIO_PE_PIN_28_NAME__);

/*--
this:AddPinNameWidget("E", 29)
--*/
#define __GPIO_PE_PIN_29_NAME__ PE29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 29, __GPIO_PE_PIN_29_NAME__);

/*--
this:AddPinNameWidget("E", 30)
--*/
#define __GPIO_PE_PIN_30_NAME__ PE30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 30, __GPIO_PE_PIN_30_NAME__);

/*--
this:AddPinNameWidget("E", 31)
--*/
#define __GPIO_PE_PIN_31_NAME__ PE31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTE_IDX, 31, __GPIO_PE_PIN_31_NAME__);


/*--
this:AddPinNameWidget("F", 0)
--*/
#define __GPIO_PF_PIN_0_NAME__ PF0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 0, __GPIO_PF_PIN_0_NAME__);

/*--
this:AddPinNameWidget("F", 1)
--*/
#define __GPIO_PF_PIN_1_NAME__ PF1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 1, __GPIO_PF_PIN_1_NAME__);

/*--
this:AddPinNameWidget("F", 2)
--*/
#define __GPIO_PF_PIN_2_NAME__ PF2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 2, __GPIO_PF_PIN_2_NAME__);

/*--
this:AddPinNameWidget("F", 3)
--*/
#define __GPIO_PF_PIN_3_NAME__ PF3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 3, __GPIO_PF_PIN_3_NAME__);

/*--
this:AddPinNameWidget("F", 4)
--*/
#define __GPIO_PF_PIN_4_NAME__ PF4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 4, __GPIO_PF_PIN_4_NAME__);

/*--
this:AddPinNameWidget("F", 5)
--*/
#define __GPIO_PF_PIN_5_NAME__ PF5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 5, __GPIO_PF_PIN_5_NAME__);

/*--
this:AddPinNameWidget("F", 6)
--*/
#define __GPIO_PF_PIN_6_NAME__ PF6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 6, __GPIO_PF_PIN_6_NAME__);

/*--
this:AddPinNameWidget("F", 7)
--*/
#define __GPIO_PF_PIN_7_NAME__ PF7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 7, __GPIO_PF_PIN_7_NAME__);

/*--
this:AddPinNameWidget("F", 8)
--*/
#define __GPIO_PF_PIN_8_NAME__ PF8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 8, __GPIO_PF_PIN_8_NAME__);

/*--
this:AddPinNameWidget("F", 9)
--*/
#define __GPIO_PF_PIN_9_NAME__ PF9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 9, __GPIO_PF_PIN_9_NAME__);

/*--
this:AddPinNameWidget("F", 10)
--*/
#define __GPIO_PF_PIN_10_NAME__ PF10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 10, __GPIO_PF_PIN_10_NAME__);

/*--
this:AddPinNameWidget("F", 11)
--*/
#define __GPIO_PF_PIN_11_NAME__ PF11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 11, __GPIO_PF_PIN_11_NAME__);

/*--
this:AddPinNameWidget("F", 12)
--*/
#define __GPIO_PF_PIN_12_NAME__ PF12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 12, __GPIO_PF_PIN_12_NAME__);

/*--
this:AddPinNameWidget("F", 13)
--*/
#define __GPIO_PF_PIN_13_NAME__ PF13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 13, __GPIO_PF_PIN_13_NAME__);

/*--
this:AddPinNameWidget("F", 14)
--*/
#define __GPIO_PF_PIN_14_NAME__ PF14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 14, __GPIO_PF_PIN_14_NAME__);

/*--
this:AddPinNameWidget("F", 15)
--*/
#define __GPIO_PF_PIN_15_NAME__ PF15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 15, __GPIO_PF_PIN_15_NAME__);

/*--
this:AddPinNameWidget("F", 16)
--*/
#define __GPIO_PF_PIN_16_NAME__ PF16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 16, __GPIO_PF_PIN_16_NAME__);

/*--
this:AddPinNameWidget("F", 17)
--*/
#define __GPIO_PF_PIN_17_NAME__ PF17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 17, __GPIO_PF_PIN_17_NAME__);

/*--
this:AddPinNameWidget("F", 18)
--*/
#define __GPIO_PF_PIN_18_NAME__ PF18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 18, __GPIO_PF_PIN_18_NAME__);

/*--
this:AddPinNameWidget("F", 19)
--*/
#define __GPIO_PF_PIN_19_NAME__ PF19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 19, __GPIO_PF_PIN_19_NAME__);

/*--
this:AddPinNameWidget("F", 20)
--*/
#define __GPIO_PF_PIN_20_NAME__ PF20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 20, __GPIO_PF_PIN_20_NAME__);

/*--
this:AddPinNameWidget("F", 21)
--*/
#define __GPIO_PF_PIN_21_NAME__ PF21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 21, __GPIO_PF_PIN_21_NAME__);

/*--
this:AddPinNameWidget("F", 22)
--*/
#define __GPIO_PF_PIN_22_NAME__ PF22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 22, __GPIO_PF_PIN_22_NAME__);

/*--
this:AddPinNameWidget("F", 23)
--*/
#define __GPIO_PF_PIN_23_NAME__ PF23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 23, __GPIO_PF_PIN_23_NAME__);

/*--
this:AddPinNameWidget("F", 24)
--*/
#define __GPIO_PF_PIN_24_NAME__ PF24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 24, __GPIO_PF_PIN_24_NAME__);

/*--
this:AddPinNameWidget("F", 25)
--*/
#define __GPIO_PF_PIN_25_NAME__ PF25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 25, __GPIO_PF_PIN_25_NAME__);

/*--
this:AddPinNameWidget("F", 26)
--*/
#define __GPIO_PF_PIN_26_NAME__ PF26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 26, __GPIO_PF_PIN_26_NAME__);

/*--
this:AddPinNameWidget("F", 27)
--*/
#define __GPIO_PF_PIN_27_NAME__ PF27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 27, __GPIO_PF_PIN_27_NAME__);

/*--
this:AddPinNameWidget("F", 28)
--*/
#define __GPIO_PF_PIN_28_NAME__ PF28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 28, __GPIO_PF_PIN_28_NAME__);

/*--
this:AddPinNameWidget("F", 29)
--*/
#define __GPIO_PF_PIN_29_NAME__ PF29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 29, __GPIO_PF_PIN_29_NAME__);

/*--
this:AddPinNameWidget("F", 30)
--*/
#define __GPIO_PF_PIN_30_NAME__ PF30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 30, __GPIO_PF_PIN_30_NAME__);

/*--
this:AddPinNameWidget("F", 31)
--*/
#define __GPIO_PF_PIN_31_NAME__ PF31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTF_IDX, 31, __GPIO_PF_PIN_31_NAME__);


/*--
this:AddPinNameWidget("G", 0)
--*/
#define __GPIO_PG_PIN_0_NAME__ PG0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 0, __GPIO_PG_PIN_0_NAME__);

/*--
this:AddPinNameWidget("G", 1)
--*/
#define __GPIO_PG_PIN_1_NAME__ PG1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 1, __GPIO_PG_PIN_1_NAME__);

/*--
this:AddPinNameWidget("G", 2)
--*/
#define __GPIO_PG_PIN_2_NAME__ PG2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 2, __GPIO_PG_PIN_2_NAME__);

/*--
this:AddPinNameWidget("G", 3)
--*/
#define __GPIO_PG_PIN_3_NAME__ PG3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 3, __GPIO_PG_PIN_3_NAME__);

/*--
this:AddPinNameWidget("G", 4)
--*/
#define __GPIO_PG_PIN_4_NAME__ PG4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 4, __GPIO_PG_PIN_4_NAME__);

/*--
this:AddPinNameWidget("G", 5)
--*/
#define __GPIO_PG_PIN_5_NAME__ PG5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 5, __GPIO_PG_PIN_5_NAME__);

/*--
this:AddPinNameWidget("G", 6)
--*/
#define __GPIO_PG_PIN_6_NAME__ PG6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 6, __GPIO_PG_PIN_6_NAME__);

/*--
this:AddPinNameWidget("G", 7)
--*/
#define __GPIO_PG_PIN_7_NAME__ PG7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 7, __GPIO_PG_PIN_7_NAME__);

/*--
this:AddPinNameWidget("G", 8)
--*/
#define __GPIO_PG_PIN_8_NAME__ PG8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 8, __GPIO_PG_PIN_8_NAME__);

/*--
this:AddPinNameWidget("G", 9)
--*/
#define __GPIO_PG_PIN_9_NAME__ PG9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 9, __GPIO_PG_PIN_9_NAME__);

/*--
this:AddPinNameWidget("G", 10)
--*/
#define __GPIO_PG_PIN_10_NAME__ PG10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 10, __GPIO_PG_PIN_10_NAME__);

/*--
this:AddPinNameWidget("G", 11)
--*/
#define __GPIO_PG_PIN_11_NAME__ PG11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 11, __GPIO_PG_PIN_11_NAME__);

/*--
this:AddPinNameWidget("G", 12)
--*/
#define __GPIO_PG_PIN_12_NAME__ PG12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 12, __GPIO_PG_PIN_12_NAME__);

/*--
this:AddPinNameWidget("G", 13)
--*/
#define __GPIO_PG_PIN_13_NAME__ PG13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 13, __GPIO_PG_PIN_13_NAME__);

/*--
this:AddPinNameWidget("G", 14)
--*/
#define __GPIO_PG_PIN_14_NAME__ PG14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 14, __GPIO_PG_PIN_14_NAME__);

/*--
this:AddPinNameWidget("G", 15)
--*/
#define __GPIO_PG_PIN_15_NAME__ PG15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 15, __GPIO_PG_PIN_15_NAME__);

/*--
this:AddPinNameWidget("G", 16)
--*/
#define __GPIO_PG_PIN_16_NAME__ PG16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 16, __GPIO_PG_PIN_16_NAME__);

/*--
this:AddPinNameWidget("G", 17)
--*/
#define __GPIO_PG_PIN_17_NAME__ PG17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 17, __GPIO_PG_PIN_17_NAME__);

/*--
this:AddPinNameWidget("G", 18)
--*/
#define __GPIO_PG_PIN_18_NAME__ PG18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 18, __GPIO_PG_PIN_18_NAME__);

/*--
this:AddPinNameWidget("G", 19)
--*/
#define __GPIO_PG_PIN_19_NAME__ PG19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 19, __GPIO_PG_PIN_19_NAME__);

/*--
this:AddPinNameWidget("G", 20)
--*/
#define __GPIO_PG_PIN_20_NAME__ PG20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 20, __GPIO_PG_PIN_20_NAME__);

/*--
this:AddPinNameWidget("G", 21)
--*/
#define __GPIO_PG_PIN_21_NAME__ PG21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 21, __GPIO_PG_PIN_21_NAME__);

/*--
this:AddPinNameWidget("G", 22)
--*/
#define __GPIO_PG_PIN_22_NAME__ PG22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 22, __GPIO_PG_PIN_22_NAME__);

/*--
this:AddPinNameWidget("G", 23)
--*/
#define __GPIO_PG_PIN_23_NAME__ PG23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 23, __GPIO_PG_PIN_23_NAME__);

/*--
this:AddPinNameWidget("G", 24)
--*/
#define __GPIO_PG_PIN_24_NAME__ PG24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 24, __GPIO_PG_PIN_24_NAME__);

/*--
this:AddPinNameWidget("G", 25)
--*/
#define __GPIO_PG_PIN_25_NAME__ PG25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 25, __GPIO_PG_PIN_25_NAME__);

/*--
this:AddPinNameWidget("G", 26)
--*/
#define __GPIO_PG_PIN_26_NAME__ PG26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 26, __GPIO_PG_PIN_26_NAME__);

/*--
this:AddPinNameWidget("G", 27)
--*/
#define __GPIO_PG_PIN_27_NAME__ PG27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 27, __GPIO_PG_PIN_27_NAME__);

/*--
this:AddPinNameWidget("G", 28)
--*/
#define __GPIO_PG_PIN_28_NAME__ PG28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 28, __GPIO_PG_PIN_28_NAME__);

/*--
this:AddPinNameWidget("G", 29)
--*/
#define __GPIO_PG_PIN_29_NAME__ PG29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 29, __GPIO_PG_PIN_29_NAME__);

/*--
this:AddPinNameWidget("G", 30)
--*/
#define __GPIO_PG_PIN_30_NAME__ PG30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 30, __GPIO_PG_PIN_30_NAME__);

/*--
this:AddPinNameWidget("G", 31)
--*/
#define __GPIO_PG_PIN_31_NAME__ PG31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTG_IDX, 31, __GPIO_PG_PIN_31_NAME__);


/*--
this:AddPinNameWidget("H", 0)
--*/
#define __GPIO_PH_PIN_0_NAME__ PH0_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 0, __GPIO_PH_PIN_0_NAME__);

/*--
this:AddPinNameWidget("H", 1)
--*/
#define __GPIO_PH_PIN_1_NAME__ PH1_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 1, __GPIO_PH_PIN_1_NAME__);

/*--
this:AddPinNameWidget("H", 2)
--*/
#define __GPIO_PH_PIN_2_NAME__ PH2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 2, __GPIO_PH_PIN_2_NAME__);

/*--
this:AddPinNameWidget("H", 3)
--*/
#define __GPIO_PH_PIN_3_NAME__ PH3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 3, __GPIO_PH_PIN_3_NAME__);

/*--
this:AddPinNameWidget("H", 4)
--*/
#define __GPIO_PH_PIN_4_NAME__ PH4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 4, __GPIO_PH_PIN_4_NAME__);

/*--
this:AddPinNameWidget("H", 5)
--*/
#define __GPIO_PH_PIN_5_NAME__ PH5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 5, __GPIO_PH_PIN_5_NAME__);

/*--
this:AddPinNameWidget("H", 6)
--*/
#define __GPIO_PH_PIN_6_NAME__ PH6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 6, __GPIO_PH_PIN_6_NAME__);

/*--
this:AddPinNameWidget("H", 7)
--*/
#define __GPIO_PH_PIN_7_NAME__ PH7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 7, __GPIO_PH_PIN_7_NAME__);

/*--
this:AddPinNameWidget("H", 8)
--*/
#define __GPIO_PH_PIN_8_NAME__ PH8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 8, __GPIO_PH_PIN_8_NAME__);

/*--
this:AddPinNameWidget("H", 9)
--*/
#define __GPIO_PH_PIN_9_NAME__ PH9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 9, __GPIO_PH_PIN_9_NAME__);

/*--
this:AddPinNameWidget("H", 10)
--*/
#define __GPIO_PH_PIN_10_NAME__ PH10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 10, __GPIO_PH_PIN_10_NAME__);

/*--
this:AddPinNameWidget("H", 11)
--*/
#define __GPIO_PH_PIN_11_NAME__ PH11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 11, __GPIO_PH_PIN_11_NAME__);

/*--
this:AddPinNameWidget("H", 12)
--*/
#define __GPIO_PH_PIN_12_NAME__ PH12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 12, __GPIO_PH_PIN_12_NAME__);

/*--
this:AddPinNameWidget("H", 13)
--*/
#define __GPIO_PH_PIN_13_NAME__ PH13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 13, __GPIO_PH_PIN_13_NAME__);

/*--
this:AddPinNameWidget("H", 14)
--*/
#define __GPIO_PH_PIN_14_NAME__ PH14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 14, __GPIO_PH_PIN_14_NAME__);

/*--
this:AddPinNameWidget("H", 15)
--*/
#define __GPIO_PH_PIN_15_NAME__ PH15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 15, __GPIO_PH_PIN_15_NAME__);

/*--
this:AddPinNameWidget("H", 16)
--*/
#define __GPIO_PH_PIN_16_NAME__ PH16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 16, __GPIO_PH_PIN_16_NAME__);

/*--
this:AddPinNameWidget("H", 17)
--*/
#define __GPIO_PH_PIN_17_NAME__ PH17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 17, __GPIO_PH_PIN_17_NAME__);

/*--
this:AddPinNameWidget("H", 18)
--*/
#define __GPIO_PH_PIN_18_NAME__ PH18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 18, __GPIO_PH_PIN_18_NAME__);

/*--
this:AddPinNameWidget("H", 19)
--*/
#define __GPIO_PH_PIN_19_NAME__ PH19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 19, __GPIO_PH_PIN_19_NAME__);

/*--
this:AddPinNameWidget("H", 20)
--*/
#define __GPIO_PH_PIN_20_NAME__ PH20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 20, __GPIO_PH_PIN_20_NAME__);

/*--
this:AddPinNameWidget("H", 21)
--*/
#define __GPIO_PH_PIN_21_NAME__ PH21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 21, __GPIO_PH_PIN_21_NAME__);

/*--
this:AddPinNameWidget("H", 22)
--*/
#define __GPIO_PH_PIN_22_NAME__ PH22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 22, __GPIO_PH_PIN_22_NAME__);

/*--
this:AddPinNameWidget("H", 23)
--*/
#define __GPIO_PH_PIN_23_NAME__ PH23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 23, __GPIO_PH_PIN_23_NAME__);

/*--
this:AddPinNameWidget("H", 24)
--*/
#define __GPIO_PH_PIN_24_NAME__ PH24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 24, __GPIO_PH_PIN_24_NAME__);

/*--
this:AddPinNameWidget("H", 25)
--*/
#define __GPIO_PH_PIN_25_NAME__ PH25_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 25, __GPIO_PH_PIN_25_NAME__);

/*--
this:AddPinNameWidget("H", 26)
--*/
#define __GPIO_PH_PIN_26_NAME__ PH26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 26, __GPIO_PH_PIN_26_NAME__);

/*--
this:AddPinNameWidget("H", 27)
--*/
#define __GPIO_PH_PIN_27_NAME__ PH27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 27, __GPIO_PH_PIN_27_NAME__);

/*--
this:AddPinNameWidget("H", 28)
--*/
#define __GPIO_PH_PIN_28_NAME__ PH28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 28, __GPIO_PH_PIN_28_NAME__);

/*--
this:AddPinNameWidget("H", 29)
--*/
#define __GPIO_PH_PIN_29_NAME__ PH29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 29, __GPIO_PH_PIN_29_NAME__);

/*--
this:AddPinNameWidget("H", 30)
--*/
#define __GPIO_PH_PIN_30_NAME__ PH30_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 30, __GPIO_PH_PIN_30_NAME__);

/*--
this:AddPinNameWidget("H", 31)
--*/
#define __GPIO_PH_PIN_31_NAME__ PH31_NC
_GPIO_PIN_NAME(IOCTL_GPIO_PORTH_IDX, 31, __GPIO_PH_PIN_31_NAME__);



#endif /* _GPIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
