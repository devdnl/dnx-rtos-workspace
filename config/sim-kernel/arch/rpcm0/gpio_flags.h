/*==============================================================================
@file    gpio_flags.h

@author  Daniel Zorychta

@brief   GPIO module configuration flags.

@note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

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

local port = {"BANK0", "QSPI"}

local fntab = {
    ["BANK0"] = {
        offset = 0,
        pinCount = 30,
        ["0"]  = {"F1 (SPI0 RX)", "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM0 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["1"]  = {"F1 (SPI0 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM0 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["2"]  = {"F1 (SPI0 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM1 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["3"]  = {"F1 (SPI0 TX)", "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM1 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["4"]  = {"F1 (SPI0 RX)",  "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM2 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["5"]  = {"F1 (SPI0 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM2 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["6"]  = {"F1 (SPI0 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM3 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["7"]  = {"F1 (SPI0 TX)", "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM3 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["8"]  = {"F1 (SPI1 RX)", "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM4 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["9"]  = {"F1 (SPI1 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM4 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["10"] = {"F1 (SPI1 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM5 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["11"] = {"F1 (SPI1 TX)", "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM5 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["12"] = {"F1 (SPI1 RX)", "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM6 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["13"] = {"F1 (SPI1 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM6 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["14"] = {"F1 (SPI1 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM7 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["15"] = {"F1 (SPI1 TX)", "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM7 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["16"] = {"F1 (SPI0 RX)", "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM0 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["17"] = {"F1 (SPI0 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM0 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["18"] = {"F1 (SPI0 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM1 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["19"] = {"F1 (SPI0 TX)", "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM1 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["20"] = {"F1 (SPI0 RX)", "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM2 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPIN0)", "F9 (USB VBUS EN)"   },
        ["21"] = {"F1 (SPI0 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM2 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT0)", "F9 (USB OVCUR DET)"},
        ["22"] = {"F1 (SPI0 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM3 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPIN1)", "F9 (USB VBUS DET)" },
        ["23"] = {"F1 (SPI0 TX)", "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM3 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT1)", "F9 (USB VBUS EN)"  },
        ["24"] = {"F1 (SPI1 RX)", "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM4 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT2)", "F9 (USB OVCUR DET)"},
        ["25"] = {"F1 (SPI1 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM4 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT3)", "F9 (USB VBUS DET)" },
        ["26"] = {"F1 (SPI1 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM5 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
        ["27"] = {"F1 (SPI1 TX)", "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM5 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB OVCUR DET)"},
        ["28"] = {"F1 (SPI1 RX)", "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM6 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS DET)" },
        ["29"] = {"F1 (SPI1 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM6 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)", "F9 (USB VBUS EN)"  },
    },
    ["QSPI"] = {
        offset = -1,
        pinCount = 6,
        ["0"] = {"F0 (XIP SCK)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
        ["1"] = {"F0 (XIP CSn)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
        ["2"] = {"F0 (XIP SD0)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
        ["3"] = {"F0 (XIP SD1)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
        ["4"] = {"F0 (XIP SD2)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
        ["5"] = {"F0 (XIP SD3)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
    }
}

if this.state.gpio_menu == nil then
    this:SetTitle("Home > Microcontroller > GPIO", "The GPIO module configuration.")

    for i = 1, #port do
        local gpio = port[i]
        this:AddExtraWidget("Submenu", "HL_GPIO"..gpio, gpio, "arch/rpcm0/gpio_flags.h")
        this:SetEvent("clicked", "HL_GPIO"..gpio, function() OpenPortMenu(gpio, nil) end)
        this:SetEvent("BackClicked", function() ResetMenu() end)
    end

    this:Finish()

elseif this.state.gpio_menu[1] and (this.state.gpio_menu[2] == nil) then

    local gpio = this.state.gpio_menu[1]

    this:SetTitle("Home > Microcontroller > "..gpio, "The "..gpio.." port configuration.")

    for pin = 0, fntab[gpio].pinCount - 1 do
        this:AddExtraWidget("Submenu", "HL_PIN"..pin, pin.." "..gpio.."."..pin, "arch/rpcm0/gpio_flags.h")
        this:SetEvent("clicked", "HL_PIN"..pin, function() OpenPortMenu(gpio, pin) end)
    end

    this:SetEvent("BackClicked", function() ResetMenu() end)

    this:Finish()

else
    local gpio = this.state.gpio_menu[1]
    local pin = this.state.gpio_menu[2]

    this:SetTitle("Home > Microcontroller > GPIO"..gpio.."."..pin, "GPIO"..gpio.."."..pin.." pin configuration.")
    this:SetEvent("BackClicked", function() OpenPortMenu(gpio, nil) end)

    local function PinExist(this, port, pinNumber)
        if this.state.gpio_menu[1] ~= port then
            return false
        end

        if this.state.gpio_menu[2] ~= pinNumber then
            return false
        end

        return (pinNumber < fntab[gpio].pinCount)
    end

    this.AddPinNameWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Editline", false, "Name")
        end
    end

    this.AddPinStrengthWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Combobox", "Strength")
            this:AddItem("2 mA, slow", "_GPIO_STRENGTH_2mA_SLEW_RATE_SLOW")
            this:AddItem("4 mA, slow", "_GPIO_STRENGTH_4mA_SLEW_RATE_SLOW")
            this:AddItem("8 mA, slow", "_GPIO_STRENGTH_8mA_SLEW_RATE_SLOW")
            this:AddItem("12 mA, slow", "_GPIO_STRENGTH_12mA_SLEW_RATE_SLOW")
            this:AddItem("2 mA, fast", "_GPIO_STRENGTH_2mA_SLEW_RATE_FAST")
            this:AddItem("4 mA, fast", "_GPIO_STRENGTH_4mA_SLEW_RATE_FAST")
            this:AddItem("8 mA, fast", "_GPIO_STRENGTH_8mA_SLEW_RATE_FAST")
            this:AddItem("12 mA, fast", "_GPIO_STRENGTH_12mA_SLEW_RATE_FAST")
        end
    end

    this.AddPinModeWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Combobox", "Mode")
            this:AddItem("Output PP", "_GPIO_MODE_PP")
            this:AddItem("Output OD", "_GPIO_MODE_OD")
            this:AddItem("Output OD + PU", "_GPIO_MODE_OD_PU")
            this:AddItem("Output OD + PD", "_GPIO_MODE_OD_PD")
            this:AddItem("Input", "_GPIO_MODE_IN")
            this:AddItem("Input PU", "_GPIO_MODE_IN_PU")
            this:AddItem("Input PD", "_GPIO_MODE_IN_PD")
            this:AddItem("Input Bus Keeper", "_GPIO_MODE_IN_BK")
            this:AddItem("Analog", "_GPIO_MODE_ANALOG")
        end
    end

    this.AddPinFunctionWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Combobox", "Alt. function")

            for k,v in ipairs(fntab[port][tostring(pin)]) do
                this:AddItem(v, tostring(k + fntab[port].offset))
            end

            this:AddItem("Disconnected", "31")
        end
    end

    this.AddPinSchmittWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Combobox", "Schmitt")
            this:AddItem("Disable", "_NO_")
            this:AddItem("Enable", "_YES_")
        end
    end

    this.AddPinStateWidget = function(this, port, pin)
        if PinExist(this, port, pin) then
            this:AddWidget("Combobox", "State")
            this:AddItem("Low", "0")
            this:AddItem("High", "1")
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

#define IOCTL_GPIO_BANK0_IDX 0
#define IOCTL_GPIO_QSPI_IDX 1
#define IOCTL_GPIO_PORTNONE_IDX 255
_GPIO_PIN_NAME(IOCTL_GPIO_PORTNONE_IDX, 0, NONE);

#define _GPIO_BIT_STRENGTH_2mA             (0 << 0)
#define _GPIO_BIT_STRENGTH_4mA             (1 << 0)
#define _GPIO_BIT_STRENGTH_8mA             (2 << 0)
#define _GPIO_BIT_STRENGTH_12mA            (3 << 0)
#define _GPOI_BIT_STRENGTH_MASK            (3 << 0)
#define _GPIO_BIT_SLEW_RATE_FAST_POS       2
#define _GPIO_BIT_SLEW_RATE_FAST           (1 << _GPIO_BIT_SLEW_RATE_FAST_POS)
#define _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW  (_GPIO_BIT_STRENGTH_2mA)
#define _GPIO_STRENGTH_4mA_SLEW_RATE_SLOW  (_GPIO_BIT_STRENGTH_4mA)
#define _GPIO_STRENGTH_8mA_SLEW_RATE_SLOW  (_GPIO_BIT_STRENGTH_8mA)
#define _GPIO_STRENGTH_12mA_SLEW_RATE_SLOW (_GPIO_BIT_STRENGTH_12mA)
#define _GPIO_STRENGTH_2mA_SLEW_RATE_FAST  (_GPIO_BIT_STRENGTH_2mA | _GPIO_BIT_SLEW_RATE_FAST)
#define _GPIO_STRENGTH_4mA_SLEW_RATE_FAST  (_GPIO_BIT_STRENGTH_4mA | _GPIO_BIT_SLEW_RATE_FAST)
#define _GPIO_STRENGTH_8mA_SLEW_RATE_FAST  (_GPIO_BIT_STRENGTH_8mA | _GPIO_BIT_SLEW_RATE_FAST)
#define _GPIO_STRENGTH_12mA_SLEW_RATE_FAST (_GPIO_BIT_STRENGTH_12mA | _GPIO_BIT_SLEW_RATE_FAST)

#define _GPIO_MODE_PP     ((GPIO_pin_pull_mode__none << 8) | GPIO_pin_mode__output_PP)
#define _GPIO_MODE_OD     ((GPIO_pin_pull_mode__none << 8) | GPIO_pin_mode__output_OD)
#define _GPIO_MODE_OD_PU  ((GPIO_pin_pull_mode__pull_up << 8) | GPIO_pin_mode__output_OD)
#define _GPIO_MODE_OD_PD  ((GPIO_pin_pull_mode__pull_down << 8) | GPIO_pin_mode__output_OD)
#define _GPIO_MODE_IN     ((GPIO_pin_pull_mode__none << 8) | GPIO_pin_mode__input)
#define _GPIO_MODE_IN_PU  ((GPIO_pin_pull_mode__pull_up << 8) | GPIO_pin_mode__input)
#define _GPIO_MODE_IN_PD  ((GPIO_pin_pull_mode__pull_down << 8) | GPIO_pin_mode__input)
#define _GPIO_MODE_IN_BK  ((GPIO_pin_pull_mode__bus_keeper << 8) | GPIO_pin_mode__input)
#define _GPIO_MODE_ANALOG ((GPIO_pin_pull_mode__none << 8) | GPIO_pin_mode__analog)

/*----------------------------------------------------------------------------*/
/*--
this:AddPinNameWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_NAME__ UART0_TX
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 0, __GPIO_BANK0_PIN_0_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_STRENGTH__ _GPIO_STRENGTH_8mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_MODE__ _GPIO_MODE_PP

/*--
this:AddPinFunctionWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_AF__ 2

/*--
this:AddPinSchmittWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 0)
--*/
#define __GPIO_BANK0_PIN_0_STATE__ 1

/*--
this:AddPinNameWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_NAME__ UART0_RX
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 1, __GPIO_BANK0_PIN_1_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_MODE__ _GPIO_MODE_IN_PU

/*--
this:AddPinFunctionWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_AF__ 2

/*--
this:AddPinSchmittWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 1)
--*/
#define __GPIO_BANK0_PIN_1_STATE__ 1

/*--
this:AddPinNameWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_NAME__ GP2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 2, __GPIO_BANK0_PIN_2_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 2)
--*/
#define __GPIO_BANK0_PIN_2_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_NAME__ GP3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 3, __GPIO_BANK0_PIN_3_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 3)
--*/
#define __GPIO_BANK0_PIN_3_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_NAME__ GP4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 4, __GPIO_BANK0_PIN_4_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 4)
--*/
#define __GPIO_BANK0_PIN_4_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_NAME__ GP5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 5, __GPIO_BANK0_PIN_5_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 5)
--*/
#define __GPIO_BANK0_PIN_5_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_NAME__ GP6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 6, __GPIO_BANK0_PIN_6_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 6)
--*/
#define __GPIO_BANK0_PIN_6_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_NAME__ GP7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 7, __GPIO_BANK0_PIN_7_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 7)
--*/
#define __GPIO_BANK0_PIN_7_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_NAME__ GP8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 8, __GPIO_BANK0_PIN_8_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 8)
--*/
#define __GPIO_BANK0_PIN_8_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_NAME__ GP9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 9, __GPIO_BANK0_PIN_9_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 9)
--*/
#define __GPIO_BANK0_PIN_9_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_NAME__ GP10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 10, __GPIO_BANK0_PIN_10_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 10)
--*/
#define __GPIO_BANK0_PIN_10_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_NAME__ GP11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 11, __GPIO_BANK0_PIN_11_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 11)
--*/
#define __GPIO_BANK0_PIN_11_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_NAME__ GP12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 12, __GPIO_BANK0_PIN_12_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 12)
--*/
#define __GPIO_BANK0_PIN_12_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_NAME__ GP13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 13, __GPIO_BANK0_PIN_13_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 13)
--*/
#define __GPIO_BANK0_PIN_13_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_NAME__ GP14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 14, __GPIO_BANK0_PIN_14_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_MODE__ _GPIO_MODE_IN_PU

/*--
this:AddPinFunctionWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 14)
--*/
#define __GPIO_BANK0_PIN_14_STATE__ 1

/*--
this:AddPinNameWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_NAME__ GP15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 15, __GPIO_BANK0_PIN_15_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 15)
--*/
#define __GPIO_BANK0_PIN_15_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_NAME__ GP16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 16, __GPIO_BANK0_PIN_16_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 16)
--*/
#define __GPIO_BANK0_PIN_16_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_NAME__ GP17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 17, __GPIO_BANK0_PIN_17_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 17)
--*/
#define __GPIO_BANK0_PIN_17_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_NAME__ GP18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 18, __GPIO_BANK0_PIN_18_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 18)
--*/
#define __GPIO_BANK0_PIN_18_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_NAME__ GP19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 19, __GPIO_BANK0_PIN_19_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 19)
--*/
#define __GPIO_BANK0_PIN_19_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_NAME__ GP20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 20, __GPIO_BANK0_PIN_20_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 20)
--*/
#define __GPIO_BANK0_PIN_20_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_NAME__ GP21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 21, __GPIO_BANK0_PIN_21_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 21)
--*/
#define __GPIO_BANK0_PIN_21_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_NAME__ GP22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 22, __GPIO_BANK0_PIN_22_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 22)
--*/
#define __GPIO_BANK0_PIN_22_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_NAME__ GP23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 23, __GPIO_BANK0_PIN_23_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 23)
--*/
#define __GPIO_BANK0_PIN_23_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_NAME__ GP24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 24, __GPIO_BANK0_PIN_24_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 24)
--*/
#define __GPIO_BANK0_PIN_24_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_NAME__ LED
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 25, __GPIO_BANK0_PIN_25_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_MODE__ _GPIO_MODE_PP

/*--
this:AddPinFunctionWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 25)
--*/
#define __GPIO_BANK0_PIN_25_STATE__ 1

/*--
this:AddPinNameWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_NAME__ GP26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 26, __GPIO_BANK0_PIN_26_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 26)
--*/
#define __GPIO_BANK0_PIN_26_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_NAME__ GP27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 27, __GPIO_BANK0_PIN_27_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 27)
--*/
#define __GPIO_BANK0_PIN_27_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_NAME__ GP28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 28, __GPIO_BANK0_PIN_28_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 28)
--*/
#define __GPIO_BANK0_PIN_28_STATE__ 0

/*--
this:AddPinNameWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_NAME__ GP29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 29, __GPIO_BANK0_PIN_29_NAME__);

/*--
this:AddPinStrengthWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
this:AddPinModeWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_AF__ 5

/*--
this:AddPinSchmittWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_SCHMITT__ _YES_

/*--
this:AddPinStateWidget("BANK0", 29)
--*/
#define __GPIO_BANK0_PIN_29_STATE__ 0

/*
 * BANK QSPI
 */
 /*--
this:AddPinNameWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_NAME__ XIP_SCK
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 0, __GPIO_QSPI_PIN_0_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 0)
--*/
#define __GPIO_QSPI_PIN_0_STATE__ 0

/*--
this:AddPinNameWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_NAME__ XIP_CSn
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 1, __GPIO_QSPI_PIN_1_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 1)
--*/
#define __GPIO_QSPI_PIN_1_STATE__ 0

/*--
this:AddPinNameWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_NAME__ XIP_SD0
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 2, __GPIO_QSPI_PIN_2_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 2)
--*/
#define __GPIO_QSPI_PIN_2_STATE__ 0

/*--
this:AddPinNameWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_NAME__ XIP_SD1
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 3, __GPIO_QSPI_PIN_3_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 3)
--*/
#define __GPIO_QSPI_PIN_3_STATE__ 0

/*--
this:AddPinNameWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_NAME__ XIP_SD2
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 4, __GPIO_QSPI_PIN_4_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 4)
--*/
#define __GPIO_QSPI_PIN_4_STATE__ 0

/*--
this:AddPinNameWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_NAME__ XIP_SD3
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 5, __GPIO_QSPI_PIN_5_NAME__);

/*--
this:AddPinStrengthWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
this:AddPinModeWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_MODE__ _GPIO_MODE_IN

/*--
this:AddPinFunctionWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_AF__ 0

/*--
this:AddPinSchmittWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_SCHMITT__ _NO_

/*--
this:AddPinStateWidget("QSPI", 5)
--*/
#define __GPIO_QSPI_PIN_5_STATE__ 0

#endif /* _GPIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
