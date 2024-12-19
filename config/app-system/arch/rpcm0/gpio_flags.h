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
local function Configure(gpio) _GPIO_SELECTION_ = gpio this:Reload() end

local port = {"BANK0", "QSPI"}

if _GPIO_SELECTION_ == nil then
    this:SetLayout("TitledGridBack", 2, "Home > Microcontroller > GPIO",
                   function() this:LoadFile("arch/arch_flags.h") end)

    this:AddExtraWidget("Label", "LabelTitle", "Available ports", -1, "bold")
    this:AddExtraWidget("Void", "VoidTitle")

    for i = 1, #port do
            local gpio = port[i]
            print("GPIO", gpio)
            this:AddExtraWidget("Label", "Label_GPIO"..gpio, gpio)
            this:AddExtraWidget("Hyperlink", "HL_GPIO"..gpio, "Configure")
            this:SetEvent("clicked", "HL_GPIO"..gpio, function() Configure(gpio) end)
    end

    this:Finish()
else
    this:SetLayout("TitledGridBack", 7, "Home > Microcontroller > GPIO >".._GPIO_SELECTION_,
                   function() Configure(nil) end)

    this:AddExtraWidget("Label", "LabelPin", "Pin")
    this:AddExtraWidget("Label", "LabelName", "Name")
    this:AddExtraWidget("Label", "LabelSpeed", "Drive strength")
    this:AddExtraWidget("Label", "LabelMode", "Mode")
    this:AddExtraWidget("Label", "LabelAF", "Function")
    this:AddExtraWidget("Label", "LabelSchmitt", "Schmitt")
    this:AddExtraWidget("Label", "LabelState", "Output state")

    this.IsPinInBank = function(this, port)
        print(port, _GPIO_SELECTION_)
        return port == _GPIO_SELECTION_
    end

    this.AddPinStrength = function(this)
        this:AddItem("2 mA, slow", "_GPIO_STRENGTH_2mA_SLEW_RATE_SLOW")
        this:AddItem("4 mA, slow", "_GPIO_STRENGTH_4mA_SLEW_RATE_SLOW")
        this:AddItem("8 mA, slow", "_GPIO_STRENGTH_8mA_SLEW_RATE_SLOW")
        this:AddItem("12 mA, slow", "_GPIO_STRENGTH_12mA_SLEW_RATE_SLOW")
        this:AddItem("2 mA, fast", "_GPIO_STRENGTH_2mA_SLEW_RATE_FAST")
        this:AddItem("4 mA, fast", "_GPIO_STRENGTH_4mA_SLEW_RATE_FAST")
        this:AddItem("8 mA, fast", "_GPIO_STRENGTH_8mA_SLEW_RATE_FAST")
        this:AddItem("12 mA, fast", "_GPIO_STRENGTH_12mA_SLEW_RATE_FAST")
    end

    this.AddPinFunction = function(this, port, pin)
        local fntab = {
            ["BANK0"] = {
                offset = 0,
                ["0"]  = {"F1 (SPI0 RX)",  "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM0 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["1"]  = {"F1 (SPI0 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM0 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["2"]  = {"F1 (SPI0 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM1 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["3"]  = {"F1 (SPI0 TX)",  "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM1 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["4"]  = {"F1 (SPI0 RX)",  "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM2 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["5"]  = {"F1 (SPI0 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM2 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["6"]  = {"F1 (SPI0 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM3 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["7"]  = {"F1 (SPI0 TX)",  "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM3 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["8"]  = {"F1 (SPI1 RX)",  "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM4 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["9"]  = {"F1 (SPI1 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM4 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["10"] = {"F1 (SPI1 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM5 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["11"] = {"F1 (SPI1 TX)",  "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM5 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["12"] = {"F1 (SPI1 RX)",  "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM6 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["13"] = {"F1 (SPI1 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM6 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["14"] = {"F1 (SPI1 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM7 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["15"] = {"F1 (SPI1 TX)",  "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM7 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["16"] = {"F1 (SPI0 RX)",  "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM0 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["17"] = {"F1 (SPI0 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM0 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["18"] = {"F1 (SPI0 SCK)", "F2 (UART0 CTS)", "F3 (I2C1 SDA)", "F4 (PWM1 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["19"] = {"F1 (SPI0 TX)",  "F2 (UART0 RTS)", "F3 (I2C1 SCL)", "F4 (PWM1 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["20"] = {"F1 (SPI0 RX)",  "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM2 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPIN0)",  "F9 (USB VBUS EN)"   },
                ["21"] = {"F1 (SPI0 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM2 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT0)", "F9 (USB OVCUR DET)"},
                ["22"] = {"F1 (SPI0 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM3 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPIN1)",  "F9 (USB VBUS DET)" },
                ["23"] = {"F1 (SPI0 TX)",  "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM3 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT1)", "F9 (USB VBUS EN)"  },
                ["24"] = {"F1 (SPI1 RX)",  "F2 (UART1 TX)",  "F3 (I2C0 SDA)", "F4 (PWM4 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT2)", "F9 (USB OVCUR DET)"},
                ["25"] = {"F1 (SPI1 CSn)", "F2 (UART1 RX)",  "F3 (I2C0 SCL)", "F4 (PWM4 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (CLOCK GPOUT3)", "F9 (USB VBUS DET)" },
                ["26"] = {"F1 (SPI1 SCK)", "F2 (UART1 CTS)", "F3 (I2C1 SDA)", "F4 (PWM5 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
                ["27"] = {"F1 (SPI1 TX)",  "F2 (UART1 RTS)", "F3 (I2C1 SCL)", "F4 (PWM5 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB OVCUR DET)"},
                ["28"] = {"F1 (SPI1 RX)",  "F2 (UART0 TX)",  "F3 (I2C0 SDA)", "F4 (PWM6 A)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS DET)" },
                ["29"] = {"F1 (SPI1 CSn)", "F2 (UART0 RX)",  "F3 (I2C0 SCL)", "F4 (PWM6 B)", "F5 (SIO)", "F6 (PIO0)", "F7 (PIO1)", "F8 (none)",         "F9 (USB VBUS EN)"  },
            },
            ["QSPI"] = {
                offset = -1,
                ["0"]  = {"F0 (XIP SCK)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
                ["1"]  = {"F0 (XIP CSn)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
                ["2"]  = {"F0 (XIP SD0)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
                ["3"]  = {"F0 (XIP SD1)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
                ["4"]  = {"F0 (XIP SD2)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
                ["5"]  = {"F0 (XIP SD3)", "F1 (none)", "F2 (none)", "F3 (none)", "F4 (none)", "F5 (SIO)", "F6 (none)", "F7 (none)", "F8 (none)", "F9 (none)"},
            }
        }

        local port_fn_tab = fntab[port]
        for k,v in ipairs(port_fn_tab[pin]) do
            this:AddItem(v, tostring(k + port_fn_tab.offset))
        end

        this:AddItem("Disconnected", "31")
    end

    this.AddPinMode = function(this)
        this:AddItem("Output PP",        "_GPIO_MODE_PP")
        this:AddItem("Output OD",        "_GPIO_MODE_OD")
        this:AddItem("Output OD + PU",   "_GPIO_MODE_OD_PU")
        this:AddItem("Output OD + PD",   "_GPIO_MODE_OD_PD")
        this:AddItem("Input",            "_GPIO_MODE_IN")
        this:AddItem("Input PU",         "_GPIO_MODE_IN_PU")
        this:AddItem("Input PD",         "_GPIO_MODE_IN_PD")
        this:AddItem("Input Bus Keeper", "_GPIO_MODE_IN_BK")
        this:AddItem("Analog",           "_GPIO_MODE_ANALOG")
    end

    this.AddPinSchmitt = function(this)
        this:AddItem("Disable", "_NO_")
        this:AddItem("Enable", "_YES_")
    end

    this.AddPinState = function(this)
        this:AddItem("Low", "0")
        this:AddItem("High", "1")
    end

    this.AddProposals = function(this, port, pin)
        local fntab = {
            ["BANK0"] = {
                ["0"]  = {"SPI0_RX",  "UART0_TX",  "I2C0_SDA", "PWM0_A", "SIO_0", "PIO0_0", "PIO1_0", "USB_OVCUR DET"},
                ["1"]  = {"SPI0_CSn", "UART0_RX",  "I2C0_SCL", "PWM0_B", "SIO_1", "PIO0_1", "PIO1_1", "USB_VBUS DET" },
                ["2"]  = {"SPI0_SCK", "UART0_CTS", "I2C1_SDA", "PWM1_A", "SIO_2", "PIO0_2", "PIO1_2", "USB_VBUS EN"  },
                ["3"]  = {"SPI0_TX",  "UART0_RTS", "I2C1_SCL", "PWM1_B", "SIO_3", "PIO0_3", "PIO1_3", "USB_OVCUR DET"},
                ["4"]  = {"SPI0_RX",  "UART1_TX",  "I2C0_SDA", "PWM2_A", "SIO_4", "PIO0_4", "PIO1_4", "USB_VBUS DET" },
                ["5"]  = {"SPI0_CSn", "UART1_RX",  "I2C0_SCL", "PWM2_B", "SIO_5", "PIO0_5", "PIO1_5", "USB_VBUS EN"  },
                ["6"]  = {"SPI0_SCK", "UART1_CTS", "I2C1_SDA", "PWM3_A", "SIO_6", "PIO0_6", "PIO1_6", "USB_OVCUR DET"},
                ["7"]  = {"SPI0_TX",  "UART1_RTS", "I2C1_SCL", "PWM3_B", "SIO_7", "PIO0_7", "PIO1_7", "USB_VBUS DET" },
                ["8"]  = {"SPI1_RX",  "UART1_TX",  "I2C0_SDA", "PWM4_A", "SIO_8", "PIO0_8", "PIO1_8", "USB_VBUS EN"  },
                ["9"]  = {"SPI1_CSn", "UART1_RX",  "I2C0_SCL", "PWM4_B", "SIO_9", "PIO0_9", "PIO1_9", "USB_OVCUR DET"},
                ["10"] = {"SPI1_SCK", "UART1_CTS", "I2C1_SDA", "PWM5_A", "SIO_10", "PIO0_10", "PIO1_10", "USB_VBUS DET" },
                ["11"] = {"SPI1_TX",  "UART1_RTS", "I2C1_SCL", "PWM5_B", "SIO_11", "PIO0_11", "PIO1_11", "USB_VBUS EN"  },
                ["12"] = {"SPI1_RX",  "UART0_TX",  "I2C0_SDA", "PWM6_A", "SIO_12", "PIO0_12", "PIO1_12", "USB_OVCUR DET"},
                ["13"] = {"SPI1_CSn", "UART0_RX",  "I2C0_SCL", "PWM6_B", "SIO_13", "PIO0_13", "PIO1_13", "USB_VBUS DET" },
                ["14"] = {"SPI1_SCK", "UART0_CTS", "I2C1_SDA", "PWM7_A", "SIO_14", "PIO0_14", "PIO1_14", "USB_VBUS EN"  },
                ["15"] = {"SPI1_TX",  "UART0_RTS", "I2C1_SCL", "PWM7_B", "SIO_15", "PIO0_15", "PIO1_15", "USB_OVCUR DET"},
                ["16"] = {"SPI0_RX",  "UART0_TX",  "I2C0_SDA", "PWM0_A", "SIO_16", "PIO0_16", "PIO1_16", "USB_VBUS DET" },
                ["17"] = {"SPI0_CSn", "UART0_RX",  "I2C0_SCL", "PWM0_B", "SIO_17", "PIO0_17", "PIO1_17", "USB_VBUS EN"  },
                ["18"] = {"SPI0_SCK", "UART0_CTS", "I2C1_SDA", "PWM1_A", "SIO_18", "PIO0_18", "PIO1_18", "USB_OVCUR DET"},
                ["19"] = {"SPI0_TX",  "UART0_RTS", "I2C1_SCL", "PWM1_B", "SIO_19", "PIO0_19", "PIO1_19", "USB_VBUS DET" },
                ["20"] = {"SPI0_RX",  "UART1_TX",  "I2C0_SDA", "PWM2_A", "SIO_20", "PIO0_20", "PIO1_20", "USB_VBUS EN",   "CLOCK_GPIN0" },
                ["21"] = {"SPI0_CSn", "UART1_RX",  "I2C0_SCL", "PWM2_B", "SIO_21", "PIO0_21", "PIO1_21", "USB_OVCUR DET", "CLOCK_GPOUT0"},
                ["22"] = {"SPI0_SCK", "UART1_CTS", "I2C1_SDA", "PWM3_A", "SIO_22", "PIO0_22", "PIO1_22", "USB_VBUS DET",  "CLOCK_GPIN1" },
                ["23"] = {"SPI0_TX",  "UART1_RTS", "I2C1_SCL", "PWM3_B", "SIO_23", "PIO0_23", "PIO1_23", "USB_VBUS EN",   "CLOCK_GPOUT1"},
                ["24"] = {"SPI1_RX",  "UART1_TX",  "I2C0_SDA", "PWM4_A", "SIO_24", "PIO0_24", "PIO1_24", "USB_OVCUR DET", "CLOCK_GPOUT2"},
                ["25"] = {"SPI1_CSn", "UART1_RX",  "I2C0_SCL", "PWM4_B", "SIO_25", "PIO0_25", "PIO1_25", "USB_VBUS DET",  "CLOCK_GPOUT3"},
                ["26"] = {"SPI1_SCK", "UART1_CTS", "I2C1_SDA", "PWM5_A", "SIO_26", "PIO0_26", "PIO1_26", "USB_VBUS EN"  },
                ["27"] = {"SPI1_TX",  "UART1_RTS", "I2C1_SCL", "PWM5_B", "SIO_27", "PIO0_27", "PIO1_27", "USB_OVCUR DET"},
                ["28"] = {"SPI1_RX",  "UART0_TX",  "I2C0_SDA", "PWM6_A", "SIO_28", "PIO0_28", "PIO1_28", "USB_VBUS DET" },
                ["29"] = {"SPI1_CSn", "UART0_RX",  "I2C0_SCL", "PWM6_B", "SIO_29", "PIO0_29", "PIO1_29", "USB_VBUS EN"  },
            },
            ["QSPI"] = {
                ["0"]  = {"XIP_SCK", "SIO"},
                ["1"]  = {"XIP_CSn", "SIO"},
                ["2"]  = {"XIP_SD0", "SIO"},
                ["3"]  = {"XIP_SD1", "SIO"},
                ["4"]  = {"XIP_SD2", "SIO"},
                ["5"]  = {"XIP_SD3", "SIO"},
            }
        }

        local port_fn_tab = fntab[port]
        for k,v in ipairs(port_fn_tab[pin]) do
            this:AddItem(v, v)
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
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP0")
    this:AddProposals("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_NAME__ UART0_TX
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 0, __GPIO_BANK0_PIN_0_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_STRENGTH__ _GPIO_STRENGTH_8mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_MODE__ _GPIO_MODE_PP

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_AF__ 2

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "0")
end
--*/
#define __GPIO_BANK0_PIN_0_STATE__ 1

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP1")
    this:AddProposals("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_NAME__ UART0_RX
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 1, __GPIO_BANK0_PIN_1_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_MODE__ _GPIO_MODE_IN_PU

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_AF__ 2

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "1")
end
--*/
#define __GPIO_BANK0_PIN_1_STATE__ 1

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP2")
    this:AddProposals("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_NAME__ GP2_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 2, __GPIO_BANK0_PIN_2_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "2")
end
--*/
#define __GPIO_BANK0_PIN_2_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP3")
    this:AddProposals("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_NAME__ GP3_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 3, __GPIO_BANK0_PIN_3_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "3")
end
--*/
#define __GPIO_BANK0_PIN_3_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP4")
    this:AddProposals("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_NAME__ GP4_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 4, __GPIO_BANK0_PIN_4_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "4")
end
--*/
#define __GPIO_BANK0_PIN_4_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP5")
    this:AddProposals("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_NAME__ GP5_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 5, __GPIO_BANK0_PIN_5_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "5")
end
--*/
#define __GPIO_BANK0_PIN_5_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP6")
    this:AddProposals("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_NAME__ GP6_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 6, __GPIO_BANK0_PIN_6_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "6")
end
--*/
#define __GPIO_BANK0_PIN_6_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP7")
    this:AddProposals("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_NAME__ GP7_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 7, __GPIO_BANK0_PIN_7_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "7")
end
--*/
#define __GPIO_BANK0_PIN_7_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP8")
    this:AddProposals("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_NAME__ GP8_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 8, __GPIO_BANK0_PIN_8_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "8")
end
--*/
#define __GPIO_BANK0_PIN_8_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP9")
    this:AddProposals("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_NAME__ GP9_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 9, __GPIO_BANK0_PIN_9_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "9")
end
--*/
#define __GPIO_BANK0_PIN_9_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP10")
    this:AddProposals("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_NAME__ GP10_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 10, __GPIO_BANK0_PIN_10_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "10")
end
--*/
#define __GPIO_BANK0_PIN_10_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP11")
    this:AddProposals("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_NAME__ GP11_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 11, __GPIO_BANK0_PIN_11_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "11")
end
--*/
#define __GPIO_BANK0_PIN_11_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP12")
    this:AddProposals("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_NAME__ GP12_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 12, __GPIO_BANK0_PIN_12_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "12")
end
--*/
#define __GPIO_BANK0_PIN_12_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP13")
    this:AddProposals("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_NAME__ GP13_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 13, __GPIO_BANK0_PIN_13_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "13")
end
--*/
#define __GPIO_BANK0_PIN_13_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP14")
    this:AddProposals("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_NAME__ GP14_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 14, __GPIO_BANK0_PIN_14_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "14")
end
--*/
#define __GPIO_BANK0_PIN_14_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP15")
    this:AddProposals("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_NAME__ GP15_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 15, __GPIO_BANK0_PIN_15_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "15")
end
--*/
#define __GPIO_BANK0_PIN_15_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP16")
    this:AddProposals("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_NAME__ GP16_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 16, __GPIO_BANK0_PIN_16_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "16")
end
--*/
#define __GPIO_BANK0_PIN_16_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP17")
    this:AddProposals("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_NAME__ GP17_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 17, __GPIO_BANK0_PIN_17_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "17")
end
--*/
#define __GPIO_BANK0_PIN_17_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP18")
    this:AddProposals("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_NAME__ GP18_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 18, __GPIO_BANK0_PIN_18_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "18")
end
--*/
#define __GPIO_BANK0_PIN_18_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP19")
    this:AddProposals("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_NAME__ GP19_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 19, __GPIO_BANK0_PIN_19_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "19")
end
--*/
#define __GPIO_BANK0_PIN_19_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP20")
    this:AddProposals("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_NAME__ GP20_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 20, __GPIO_BANK0_PIN_20_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "20")
end
--*/
#define __GPIO_BANK0_PIN_20_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP21")
    this:AddProposals("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_NAME__ GP21_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 21, __GPIO_BANK0_PIN_21_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "21")
end
--*/
#define __GPIO_BANK0_PIN_21_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP22")
    this:AddProposals("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_NAME__ GP22_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 22, __GPIO_BANK0_PIN_22_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "22")
end
--*/
#define __GPIO_BANK0_PIN_22_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP23")
    this:AddProposals("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_NAME__ GP23_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 23, __GPIO_BANK0_PIN_23_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "23")
end
--*/
#define __GPIO_BANK0_PIN_23_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP24")
    this:AddProposals("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_NAME__ GP24_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 24, __GPIO_BANK0_PIN_24_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "24")
end
--*/
#define __GPIO_BANK0_PIN_24_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP25")
    this:AddProposals("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_NAME__ LED
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 25, __GPIO_BANK0_PIN_25_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_MODE__ _GPIO_MODE_PP

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "25")
end
--*/
#define __GPIO_BANK0_PIN_25_STATE__ 1

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP26")
    this:AddProposals("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_NAME__ GP26_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 26, __GPIO_BANK0_PIN_26_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "26")
end
--*/
#define __GPIO_BANK0_PIN_26_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP27")
    this:AddProposals("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_NAME__ GP27_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 27, __GPIO_BANK0_PIN_27_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "27")
end
--*/
#define __GPIO_BANK0_PIN_27_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP28")
    this:AddProposals("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_NAME__ GP28_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 28, __GPIO_BANK0_PIN_28_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "28")
end
--*/
#define __GPIO_BANK0_PIN_28_STATE__ 0

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Textbox", "GP29")
    this:AddProposals("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_NAME__ GP29_NC
_GPIO_PIN_NAME(IOCTL_GPIO_BANK0_IDX, 29, __GPIO_BANK0_PIN_29_NAME__);

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinStrength("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_STRENGTH__ _GPIO_STRENGTH_2mA_SLEW_RATE_SLOW

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinMode("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinFunction("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_AF__ 5

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_SCHMITT__ _YES_

/*--
if this:IsPinInBank("BANK0") then
    this:AddWidget("Combobox")
    this:AddPinState("BANK0", "29")
end
--*/
#define __GPIO_BANK0_PIN_29_STATE__ 0


/*
 * BANK QSPI
 */
/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP0")
    this:AddProposals("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_NAME__ XIP_SCK
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 0, __GPIO_QSPI_PIN_0_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "0")
end
--*/
#define __GPIO_QSPI_PIN_0_STATE__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP1")
    this:AddProposals("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_NAME__ XIP_CSn
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 1, __GPIO_QSPI_PIN_1_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "1")
end
--*/
#define __GPIO_QSPI_PIN_1_STATE__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP2")
    this:AddProposals("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_NAME__ XIP_SD0
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 2, __GPIO_QSPI_PIN_2_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "2")
end
--*/
#define __GPIO_QSPI_PIN_2_STATE__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP3")
    this:AddProposals("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_NAME__ XIP_SD1
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 3, __GPIO_QSPI_PIN_3_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "3")
end
--*/
#define __GPIO_QSPI_PIN_3_STATE__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP4")
    this:AddProposals("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_NAME__ XIP_SD2
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 4, __GPIO_QSPI_PIN_4_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "4")
end
--*/
#define __GPIO_QSPI_PIN_4_STATE__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Textbox", "XIP5")
    this:AddProposals("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_NAME__ XIP_SD3
_GPIO_PIN_NAME(IOCTL_GPIO_QSPI_IDX, 5, __GPIO_QSPI_PIN_5_NAME__);

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinStrength("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_STRENGTH__ _GPIO_STRENGTH_12mA_SLEW_RATE_FAST

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinMode("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_MODE__ _GPIO_MODE_IN

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinFunction("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_AF__ 0

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinSchmitt("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_SCHMITT__ _NO_

/*--
if this:IsPinInBank("QSPI") then
    this:AddWidget("Combobox")
    this:AddPinState("QSPI", "5")
end
--*/
#define __GPIO_QSPI_PIN_5_STATE__ 0

#endif /* _GPIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
