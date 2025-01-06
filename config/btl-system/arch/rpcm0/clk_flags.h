/*==============================================================================
@file    clk_flags.h

@author  Daniel Zorychta

@brief   CLK module configuration flags.

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

#ifndef _CLK_FLAGS_H_
#define _CLK_FLAGS_H_

#/*--
this:SetLayout("TitledGridBack", 3, "Home > Microcontroller > CLK",
               function() this:LoadFile("arch/arch_flags.h") end)

this:SetEvent("PostDiscard", function() this:CalculateFreq() end)
++*/

//==============================================================================
// Oscillators Configuration
//==============================================================================
/*--
this:AddExtraWidget("Label", "LABEL_OSCILLATORS", "Oscillators", -1, "bold")
this:AddExtraWidget("Label", "LabelOsc1", "\t\t\t")
this:AddExtraWidget("Void", "VoidOsc2")
++*/
/*--
this:AddWidget("Editline", false, "GPCLK0 oscillator frequency")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPCLK0", "Hz")
--*/
#define __CLK_GPCLK0_FREQ__ 1000000

/*--
this:AddWidget("Editline", false, "GPCLK1 oscillator frequency")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPCLK1", "Hz")
--*/
#define __CLK_GPCLK1_FREQ__ 1000000

/*--
this:AddWidget("Combobox", "XOSC oscillator")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_XOSC_EN", "LABEL_XOSC_EN")
--*/
#define __CLK_XOSC_ENABLE__ _YES_

//==============================================================================
// System PLL Configuration
//==============================================================================
/*--
this:AddExtraWidget("Label", "LABEL_SYS_PLL_CONF", "\nSystem PLL configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidSysPLLCFG1")
this:AddExtraWidget("Void", "VoidSysPLLCFG2")
++*/
/*--
this:AddWidget("Combobox", "PLL enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_PLL_EN", "")
--*/
#define __CLK_SYS_PLL_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 63, "Input clock divider (REFDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_PLL_REFDIV", "")
--*/
#define __CLK_SYS_PLL_REFDIV__ 1

/*--
this:AddWidget("Spinbox", 1, 4095, "Feedback divider (FBDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_PLL_FBDIV", "LABEL_SYS_PLL_FBDIV")
--*/
#define __CLK_SYS_PLL_FBDIV__ 133

/*--
this:AddWidget("Spinbox", 1, 7, "Post divider 1 (POSTDIV1)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_PLL_POSTDIV1", "")
--*/
#define __CLK_SYS_PLL_POSTDIV1__ 6

/*--
this:AddWidget("Spinbox", 1, 7, "Post divider 2 (POSTDIV2)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_PLL_POSTDIV2", "LABEL_SYS_PLL_POSTDIV2")
--*/
#define __CLK_SYS_PLL_POSTDIV2__ 2

//==============================================================================
// USB PLL Configuration
//==============================================================================
/*--
this:AddExtraWidget("Label", "LABEL_USB_PLL_CONF", "\nUSB PLL configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidUSBPLLCFG1")
this:AddExtraWidget("Void", "VoidUSBPLLCFG2")
++*/
/*--
this:AddWidget("Combobox", "PLL enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_PLL_EN", "")
--*/
#define __CLK_USB_PLL_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 63, "Input clock divider (REFDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_PLL_REFDIV", "")
--*/
#define __CLK_USB_PLL_REFDIV__ 1

/*--
this:AddWidget("Spinbox", 1, 4095, "Feedback divider (FBDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_PLL_FBDIV", "LABEL_USB_PLL_FBDIV")
--*/
#define __CLK_USB_PLL_FBDIV__ 64

/*--
this:AddWidget("Spinbox", 1, 7, "Post divider 1 (POSTDIV1)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_PLL_POSTDIV1", "")
--*/
#define __CLK_USB_PLL_POSTDIV1__ 4

/*--
this:AddWidget("Spinbox", 1, 7, "Post divider 2 (POSTDIV2)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_PLL_POSTDIV2", "LABEL_USB_PLL_POSTDIV2")
--*/
#define __CLK_USB_PLL_POSTDIV2__ 4

//==============================================================================
// clk_ref Configuration
//==============================================================================
#define _CLK_REF_CLK_SRC_ROSC     (CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH     << CLOCKS_CLK_REF_CTRL_SRC_LSB)
#define _CLK_REF_CLK_SRC_REF_AUX_ (CLOCKS_CLK_REF_CTRL_SRC_VALUE_CLKSRC_CLK_REF_AUX << CLOCKS_CLK_REF_CTRL_SRC_LSB)
#define _CLK_REF_CLK_SRC_XOSC     (CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC        << CLOCKS_CLK_REF_CTRL_SRC_LSB)
#define _CLK_REF_CLK_SRC_PLL_USB  ((CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_REF_CTRL_AUXSRC_LSB) | _CLK_REF_CLK_SRC_REF_AUX_)
#define _CLK_REF_CLK_SRC_GPIN0    ((CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_REF_CTRL_AUXSRC_LSB) | _CLK_REF_CLK_SRC_REF_AUX_)
#define _CLK_REF_CLK_SRC_GPIN1    ((CLOCKS_CLK_REF_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_REF_CTRL_AUXSRC_LSB) | _CLK_REF_CLK_SRC_REF_AUX_)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_REF_CONF", "\nReference clock configuration (clk_ref)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKREFCFG1")
this:AddExtraWidget("Void", "VoidCLKREFCFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("ROSC", "_CLK_REF_CLK_SRC_ROSC")
this:AddItem("XOSC", "_CLK_REF_CLK_SRC_XOSC")
this:AddItem("PLL USB", "_CLK_REF_CLK_SRC_PLL_USB")
this:AddItem("GPIN0", "_CLK_REF_CLK_SRC_GPIN0")
this:AddItem("GPIN1", "_CLK_REF_CLK_SRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_REF_CLK_SRC", "LABEL_REF_CLK_SRC")
--*/
#define __CLK_REF_CLK_SRC__ _CLK_REF_CLK_SRC_XOSC

/*--
this:AddWidget("Spinbox", 1, 3, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_REF_CLK_DIV", "LABEL_REF_CLK_DIV")
--*/
#define __CLK_REF_CLK_DIV__ 1

//==============================================================================
// clk_usb Configuration
//==============================================================================
#define _CLK_USB_CLK_AUXSRC_PLL_USB (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)
#define _CLK_USB_CLK_AUXSRC_PLL_SYS (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)
#define _CLK_USB_CLK_AUXSRC_ROSC    (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)
#define _CLK_USB_CLK_AUXSRC_XOSC    (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)
#define _CLK_USB_CLK_AUXSRC_GPIN0   (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)
#define _CLK_USB_CLK_AUXSRC_GPIN1   (CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_USB_CTRL_AUXSRC_LSB)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_USB_CONF", "\nUSB clock configuration (clk_usb)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKUSBCFG1")
this:AddExtraWidget("Void", "VoidCLKUSBCFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL USB", "_CLK_USB_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_USB_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_USB_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_USB_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_USB_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_USB_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_CLK_SRC", "LABEL_USB_CLK_SRC")
--*/
#define __CLK_USB_CLK_SRC__ _CLK_USB_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 3, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_USB_CLK_DIV", "LABEL_USB_CLK_DIV")
--*/
#define __CLK_USB_CLK_DIV__ 1

//==============================================================================
// clk_adc Configuration
//==============================================================================
#define _CLK_ADC_CLK_AUXSRC_PLL_USB (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)
#define _CLK_ADC_CLK_AUXSRC_PLL_SYS (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)
#define _CLK_ADC_CLK_AUXSRC_ROSC    (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)
#define _CLK_ADC_CLK_AUXSRC_XOSC    (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)
#define _CLK_ADC_CLK_AUXSRC_GPIN0   (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)
#define _CLK_ADC_CLK_AUXSRC_GPIN1   (CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_ADC_CTRL_AUXSRC_LSB)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_ADC_CONF", "\nADC clock configuration (clk_adc)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKADCCFG1")
this:AddExtraWidget("Void", "VoidCLKADCCFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL USB", "_CLK_ADC_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_ADC_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_ADC_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_ADC_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_ADC_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_ADC_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_ADC_CLK_SRC", "LABEL_ADC_CLK_SRC")
--*/
#define __CLK_ADC_CLK_SRC__ _CLK_ADC_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 3, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_ADC_CLK_DIV", "LABEL_ADC_CLK_DIV")
--*/
#define __CLK_ADC_CLK_DIV__ 1

//==============================================================================
// clk_rtc Configuration
//==============================================================================
#define _CLK_RTC_CLK_AUXSRC_PLL_USB (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)
#define _CLK_RTC_CLK_AUXSRC_PLL_SYS (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)
#define _CLK_RTC_CLK_AUXSRC_ROSC    (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)
#define _CLK_RTC_CLK_AUXSRC_XOSC    (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)
#define _CLK_RTC_CLK_AUXSRC_GPIN0   (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)
#define _CLK_RTC_CLK_AUXSRC_GPIN1   (CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_RTC_CTRL_AUXSRC_LSB)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_RTC_CONF", "\nRTC clock configuration (clk_rtc)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKRTCCFG1")
this:AddExtraWidget("Void", "VoidCLKRTCCFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL USB", "_CLK_RTC_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_RTC_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_RTC_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_RTC_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_RTC_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_RTC_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_RTC_CLK_SRC", "LABEL_RTC_CLK_SRC")
--*/
#define __CLK_RTC_CLK_SRC__ _CLK_RTC_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_RTC_CLK_DIV", "LABEL_RTC_CLK_DIV")
--*/
#define __CLK_RTC_CLK_DIV__ 1

//==============================================================================
// clk_rtc Configuration
//==============================================================================
#define _CLK_SYS_CLK_AUX_        (CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX << CLOCKS_CLK_SYS_CTRL_SRC_LSB)
#define _CLK_SYS_CLK_SRC_CLK_REF (CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF            << CLOCKS_CLK_SYS_CTRL_SRC_LSB)
#define _CLK_SYS_CLK_SRC_PLL_SYS ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)
#define _CLK_SYS_CLK_SRC_PLL_USB ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)
#define _CLK_SYS_CLK_SRC_ROSC    ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_ROSC_CLKSRC    << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)
#define _CLK_SYS_CLK_SRC_XOSC    ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)
#define _CLK_SYS_CLK_SRC_GPIN0   ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)
#define _CLK_SYS_CLK_SRC_GPIN1   ((CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_SYS_CTRL_AUXSRC_LSB) | _CLK_SYS_CLK_AUX_)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_SYS_CONF", "\nSystem clock configuration (clk_sys)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKSYSCFG1")
this:AddExtraWidget("Void", "VoidCLKSYSCFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("clk_ref", "_CLK_SYS_CLK_SRC_CLK_REF")
this:AddItem("PLL SYS", "_CLK_SYS_CLK_SRC_PLL_SYS")
this:AddItem("PLL_USB", "_CLK_SYS_CLK_SRC_PLL_USB")
this:AddItem("ROSC", "_CLK_SYS_CLK_SRC_ROSC")
this:AddItem("XOSC", "_CLK_SYS_CLK_SRC_XOSC")
this:AddItem("GPIN0", "_CLK_SYS_CLK_SRC_GPIN0")
this:AddItem("GPIN1", "_CLK_SYS_CLK_SRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_CLK_SRC", "LABEL_SYS_CLK_SRC")
--*/
#define __CLK_SYS_CLK_SRC__ _CLK_SYS_CLK_SRC_PLL_SYS

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_SYS_CLK_DIV", "LABEL_SYS_CLK_DIV")
--*/
#define __CLK_SYS_CLK_DIV__ 1

//==============================================================================
// clk_peri Configuration
//==============================================================================
#define _CLK_PERI_CLK_AUXSRC_CLK_SYS (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS        << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_PLL_SYS (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_PLL_USB (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_ROSC    (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_XOSC    (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_GPIN0   (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)
#define _CLK_PERI_CLK_AUXSRC_GPIN1   (CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_PERI_CTRL_AUXSRC_LSB)

/*--
this:AddExtraWidget("Label", "LABEL_CLK_PERI_CONF", "\nPeripheral clock configuration (clk_peri)", -1, "bold")
this:AddExtraWidget("Void", "VoidCLKPERICFG1")
this:AddExtraWidget("Void", "VoidCLKPERICFG2")
++*/
/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("sys_clk", "_CLK_PERI_CLK_AUXSRC_CLK_SYS")
this:AddItem("PLL SYS", "_CLK_PERI_CLK_AUXSRC_PLL_SYS")
this:AddItem("PLL USB", "_CLK_PERI_CLK_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_PERI_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_PERI_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_PERI_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_PERI_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_PERI_CLK_SRC", "LABEL_PERI_CLK_SRC")
--*/
#define __CLK_PERI_CLK_SRC__ _CLK_PERI_CLK_AUXSRC_CLK_SYS

//==============================================================================
// GPOUT Configuration
//==============================================================================
#define _CLK_GPOUT_AUXSRC_PLL_SYS (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_GPIN0   (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_GPIN0   << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_GPIN1   (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_GPIN1   << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_PLL_USB (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_ROSC    (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_ROSC_CLKSRC    << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_XOSC    (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_XOSC_CLKSRC    << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_CLK_SYS (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS        << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_CLK_USB (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_USB        << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_CLK_ADC (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_ADC        << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_CLK_RTC (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_RTC        << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)
#define _CLK_GPOUT_AUXSRC_CLK_REF (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_REF        << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB)

/*--
this:AddExtraWidget("Label", "LABEL_GPOUT0_CONF", "\nGPOUT0 configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidGPOUT0CFG1")
this:AddExtraWidget("Void", "VoidGPOUT0CFG2")
++*/
/*--
this:AddWidget("Combobox", "GPOUT0 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT0_EN", "")
--*/
#define __CLK_GPOUT0_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL SYS", "_CLK_GPOUT_AUXSRC_PLL_SYS")
this:AddItem("GPIN0", "_CLK_GPOUT_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_GPOUT_AUXSRC_GPIN1")
this:AddItem("PLL USB", "_CLK_GPOUT_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_GPOUT_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_GPOUT_AUXSRC_XOSC")
this:AddItem("clk_sys", "_CLK_GPOUT_AUXSRC_CLK_SYS")
this:AddItem("clk_usb", "_CLK_GPOUT_AUXSRC_CLK_USB")
this:AddItem("clk_adc", "_CLK_GPOUT_AUXSRC_CLK_ADC")
this:AddItem("clk_rtc", "_CLK_GPOUT_AUXSRC_CLK_RTC")
this:AddItem("clk_ref", "_CLK_GPOUT_AUXSRC_CLK_REF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT0_SRC", "LABEL_GPOUT0_SRC")
--*/
#define __CLK_GPOUT0_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT0_DIV", "LABEL_GPOUT0_DIV")
--*/
#define __CLK_GPOUT0_CLK_DIV__ 1

/*--
this:AddExtraWidget("Label", "LABEL_GPOUT1_CONF", "\nGPOUT1 configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidGPOUT1CFG1")
this:AddExtraWidget("Void", "VoidGPOUT1CFG2")
++*/
/*--
this:AddWidget("Combobox", "GPOUT1 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT1_EN", "")
--*/
#define __CLK_GPOUT1_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL SYS", "_CLK_GPOUT_AUXSRC_PLL_SYS")
this:AddItem("GPIN0", "_CLK_GPOUT_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_GPOUT_AUXSRC_GPIN1")
this:AddItem("PLL USB", "_CLK_GPOUT_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_GPOUT_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_GPOUT_AUXSRC_XOSC")
this:AddItem("clk_sys", "_CLK_GPOUT_AUXSRC_CLK_SYS")
this:AddItem("clk_usb", "_CLK_GPOUT_AUXSRC_CLK_USB")
this:AddItem("clk_adc", "_CLK_GPOUT_AUXSRC_CLK_ADC")
this:AddItem("clk_rtc", "_CLK_GPOUT_AUXSRC_CLK_RTC")
this:AddItem("clk_ref", "_CLK_GPOUT_AUXSRC_CLK_REF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT1_SRC", "LABEL_GPOUT1_SRC")
--*/
#define __CLK_GPOUT1_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT1_DIV", "LABEL_GPOUT1_DIV")
--*/
#define __CLK_GPOUT1_CLK_DIV__ 1

/*--
this:AddExtraWidget("Label", "LABEL_GPOUT2_CONF", "\nGPOUT2 configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidGPOUT2CFG1")
this:AddExtraWidget("Void", "VoidGPOUT2CFG2")
++*/
/*--
this:AddWidget("Combobox", "GPOUT2 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT2_EN", "")
--*/
#define __CLK_GPOUT2_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL SYS", "_CLK_GPOUT_AUXSRC_PLL_SYS")
this:AddItem("GPIN0", "_CLK_GPOUT_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_GPOUT_AUXSRC_GPIN1")
this:AddItem("PLL USB", "_CLK_GPOUT_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_GPOUT_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_GPOUT_AUXSRC_XOSC")
this:AddItem("clk_sys", "_CLK_GPOUT_AUXSRC_CLK_SYS")
this:AddItem("clk_usb", "_CLK_GPOUT_AUXSRC_CLK_USB")
this:AddItem("clk_adc", "_CLK_GPOUT_AUXSRC_CLK_ADC")
this:AddItem("clk_rtc", "_CLK_GPOUT_AUXSRC_CLK_RTC")
this:AddItem("clk_ref", "_CLK_GPOUT_AUXSRC_CLK_REF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT2_SRC", "LABEL_GPOUT2_SRC")
--*/
#define __CLK_GPOUT2_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT2_DIV", "LABEL_GPOUT2_DIV")
--*/
#define __CLK_GPOUT2_CLK_DIV__ 1

/*--
this:AddExtraWidget("Label", "LABEL_GPOUT3_CONF", "\nGPOUT3 configuration", -1, "bold")
this:AddExtraWidget("Void", "VoidGPOUT3CFG1")
this:AddExtraWidget("Void", "VoidGPOUT3CFG2")
++*/
/*--
this:AddWidget("Combobox", "GPOUT3 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT3_EN", "")
--*/
#define __CLK_GPOUT3_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "Clock source")
this:AddItem("PLL SYS", "_CLK_GPOUT_AUXSRC_PLL_SYS")
this:AddItem("GPIN0", "_CLK_GPOUT_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_GPOUT_AUXSRC_GPIN1")
this:AddItem("PLL USB", "_CLK_GPOUT_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_GPOUT_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_GPOUT_AUXSRC_XOSC")
this:AddItem("clk_sys", "_CLK_GPOUT_AUXSRC_CLK_SYS")
this:AddItem("clk_usb", "_CLK_GPOUT_AUXSRC_CLK_USB")
this:AddItem("clk_adc", "_CLK_GPOUT_AUXSRC_CLK_ADC")
this:AddItem("clk_rtc", "_CLK_GPOUT_AUXSRC_CLK_RTC")
this:AddItem("clk_ref", "_CLK_GPOUT_AUXSRC_CLK_REF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT3_SRC", "LABEL_GPOUT3_SRC")
--*/
#define __CLK_GPOUT3_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "Clock divider")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:AddExtraWidget("Label", "LABEL_GPOUT3_DIV", "LABEL_GPOUT3_DIV")
--*/
#define __CLK_GPOUT3_CLK_DIV__ 1


//==============================================================================
// Calculation script
//==============================================================================
/*--
this.CalculateFreq = function(self)
    local ifs = function(expr, t, f) if expr then return t else return f end end

    local freq       = {}
    freq.GPIN0       = tonumber(this:GetFlagValue("__CLK_GPCLK0_FREQ__"))
    freq.GPIN1       = tonumber(this:GetFlagValue("__CLK_GPCLK1_FREQ__"))
    freq.XOSC        = uC.OSCFREQ
    freq.ROSC        = 6e6
    freq.CLK_REF     = 0
    freq.PLL_USB     = 0
    freq.PLL_SYS     = 0
    freq.CLK_ADC     = 0
    freq.CLK_USB     = 0
    freq.CLK_RTC     = 0
    freq.CLK_PERI    = 0
    freq.CLK_SYS     = 0
    freq.GPOUT0      = 0
    freq.GPOUT1      = 0
    freq.GPOUT2      = 0
    freq.GPOUT3      = 0

    -- calculate XOSC frequency
    if this:GetFlagValue("__CLK_XOSC_ENABLE__") == "_NO_" then freq.XOSC = 0 end
    this:SetFlagValue("LABEL_XOSC_EN", PrintFrequency(tonumber(freq.XOSC)))

    -- PLL SYS frequency
    local pll_sys_in = freq.XOSC
    if this:GetFlagValue("__CLK_SYS_PLL_ENABLE__") == "_NO_" then pll_sys_in = 0 end
    local refdiv    = this:GetFlagValue("__CLK_SYS_PLL_REFDIV__")
    local fbdiv     = this:GetFlagValue("__CLK_SYS_PLL_FBDIV__")
    local postdiv1  = this:GetFlagValue("__CLK_SYS_PLL_POSTDIV1__")
    local postdiv2  = this:GetFlagValue("__CLK_SYS_PLL_POSTDIV2__")
    local clk_fbdiv = (pll_sys_in / refdiv * fbdiv)
    freq.PLL_SYS    = clk_fbdiv / (postdiv1 * postdiv2)
    local msg       = " [Keep in range 750-1600 MHz]"
    if clk_fbdiv >= 750e6 and clk_fbdiv <= 1600e6 then msg = "" end
    this:SetFlagValue("LABEL_SYS_PLL_FBDIV", PrintFrequency(tonumber(clk_fbdiv))..msg)
    this:SetFlagValue("LABEL_SYS_PLL_POSTDIV2", PrintFrequency(tonumber(freq.PLL_SYS)))

    -- PLL USB frequency
    local pll_usb_in = freq.XOSC
    if this:GetFlagValue("__CLK_USB_PLL_ENABLE__") == "_NO_" then pll_usb_in = 0 end
    local refdiv    = this:GetFlagValue("__CLK_USB_PLL_REFDIV__")
    local fbdiv     = this:GetFlagValue("__CLK_USB_PLL_FBDIV__")
    local postdiv1  = this:GetFlagValue("__CLK_USB_PLL_POSTDIV1__")
    local postdiv2  = this:GetFlagValue("__CLK_USB_PLL_POSTDIV2__")
    local clk_fbdiv = (pll_usb_in / refdiv * fbdiv)
    freq.PLL_USB    = clk_fbdiv / (postdiv1 * postdiv2)
    local msg       = " [Keep in range 750-1600 MHz]"
    if clk_fbdiv >= 750e6 and clk_fbdiv <= 1600e6 then msg = "" end
    this:SetFlagValue("LABEL_USB_PLL_FBDIV", PrintFrequency(tonumber(clk_fbdiv))..msg)
    this:SetFlagValue("LABEL_USB_PLL_POSTDIV2", PrintFrequency(tonumber(freq.PLL_USB)))

    -- CLK_REF
    local SRC    = this:GetFlagValue("__CLK_REF_CLK_SRC__"):gsub("_CLK_REF_CLK_SRC_", "")
    local div    = this:GetFlagValue("__CLK_REF_CLK_DIV__")
    local clk_in = freq[SRC]
    freq.CLK_REF = clk_in / div
    this:SetFlagValue("LABEL_REF_CLK_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_REF_CLK_DIV", PrintFrequency(tonumber(freq.CLK_REF)))

    -- CLK_USB
    local SRC    = this:GetFlagValue("__CLK_USB_CLK_SRC__"):gsub("_CLK_USB_CLK_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_USB_CLK_DIV__")
    local clk_in = freq[SRC]
    freq.CLK_USB = clk_in / div
    this:SetFlagValue("LABEL_USB_CLK_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_USB_CLK_DIV", PrintFrequency(tonumber(freq.CLK_USB)))

    -- CLK_ADC
    local SRC    = this:GetFlagValue("__CLK_ADC_CLK_SRC__"):gsub("_CLK_ADC_CLK_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_ADC_CLK_DIV__")
    local clk_in = freq[SRC]
    freq.CLK_ADC = clk_in / div
    this:SetFlagValue("LABEL_ADC_CLK_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_ADC_CLK_DIV", PrintFrequency(tonumber(freq.CLK_ADC)))

    -- CLK_RTC
    local SRC    = this:GetFlagValue("__CLK_RTC_CLK_SRC__"):gsub("_CLK_RTC_CLK_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_RTC_CLK_DIV__")
    local clk_in = freq[SRC]
    freq.CLK_RTC = clk_in / div
    this:SetFlagValue("LABEL_RTC_CLK_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_RTC_CLK_DIV", PrintFrequency(tonumber(freq.CLK_RTC)))

    -- CLK_SYS
    local SRC    = this:GetFlagValue("__CLK_SYS_CLK_SRC__"):gsub("_CLK_SYS_CLK_SRC_", "")
    local div    = this:GetFlagValue("__CLK_SYS_CLK_DIV__")
    local clk_in = freq[SRC]
    freq.CLK_SYS = clk_in / div
    this:SetFlagValue("LABEL_SYS_CLK_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_SYS_CLK_DIV", PrintFrequency(tonumber(freq.CLK_SYS)))

    -- CLK_PERI
    local SRC     = this:GetFlagValue("__CLK_PERI_CLK_SRC__"):gsub("_CLK_PERI_CLK_AUXSRC_", "")
    freq.CLK_PERI = freq[SRC] / div
    this:SetFlagValue("LABEL_PERI_CLK_SRC", PrintFrequency(tonumber(freq.CLK_PERI)))

    -- GPOUT0
    local EN     = ifs(this:GetFlagValue("__CLK_GPOUT0_ENABLE__") == "_YES_", 1, 0)
    local SRC    = this:GetFlagValue("__CLK_GPOUT0_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_GPOUT0_CLK_DIV__")
    local clk_in = freq[SRC] * EN
    freq.GPOUT0  = clk_in / div
    this:SetFlagValue("LABEL_GPOUT0_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_GPOUT0_DIV", PrintFrequency(tonumber(freq.GPOUT0)))

    -- GPOUT1
    local EN     = ifs(this:GetFlagValue("__CLK_GPOUT1_ENABLE__") == "_YES_", 1, 0)
    local SRC    = this:GetFlagValue("__CLK_GPOUT1_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_GPOUT1_CLK_DIV__")
    local clk_in = freq[SRC] * EN
    freq.GPOUT1  = clk_in / div
    this:SetFlagValue("LABEL_GPOUT1_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_GPOUT1_DIV", PrintFrequency(tonumber(freq.GPOUT1)))

    -- GPOUT2
    local EN     = ifs(this:GetFlagValue("__CLK_GPOUT2_ENABLE__") == "_YES_", 1, 0)
    local SRC    = this:GetFlagValue("__CLK_GPOUT2_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_GPOUT2_CLK_DIV__")
    local clk_in = freq[SRC] * EN
    freq.GPOUT2  = clk_in / div
    this:SetFlagValue("LABEL_GPOUT2_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_GPOUT2_DIV", PrintFrequency(tonumber(freq.GPOUT2)))

    -- GPOUT3
    local EN     = ifs(this:GetFlagValue("__CLK_GPOUT3_ENABLE__") == "_YES_", 1, 0)
    local SRC    = this:GetFlagValue("__CLK_GPOUT3_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div    = this:GetFlagValue("__CLK_GPOUT3_CLK_DIV__")
    local clk_in = freq[SRC] * EN
    freq.GPOUT3  = clk_in / div
    this:SetFlagValue("LABEL_GPOUT3_SRC", PrintFrequency(tonumber(clk_in)))
    this:SetFlagValue("LABEL_GPOUT3_DIV", PrintFrequency(tonumber(freq.GPOUT3)))
end

-- trigger frequency calculation after loading all flags
this.CalculateFreq()
++*/

#endif /* _CLK_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
