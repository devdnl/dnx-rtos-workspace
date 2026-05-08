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

/*--
this:SetTitle("Home > Microcontroller > CLK", "The CLK module configuration.")

this:SetEvent("PostDiscard", function() this:CalculateFreq() end)

this.ShowFrequency = function(id, input)
    if type(this.freq[id]) == "number" then
        return input .. " [" .. PrintFrequency(this.freq[id]) .. "]"
    elseif type(this.freq[id]) == "string" then
        return input .. " [" .. this.freq[id] .. "]"
    else
        return input .. " ["..type(this.freq[id]).."]"
    end
end
++*/

//==============================================================================
// Oscillators Configuration
//==============================================================================
/*--
this:AddWidget("Editline", false, "GPCLK0 oscillator frequency")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPCLK0_FREQ__ 1000000

/*--
this:AddWidget("Editline", false, "GPCLK1 oscillator frequency")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPCLK1_FREQ__ 1000000

/*--
this:AddWidget("Combobox", "XOSC oscillator")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("XOSC", this.ShowFrequency)
--*/
#define __CLK_XOSC_ENABLE__ _YES_

//==============================================================================
// System PLL Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "System PLL enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_SYS_PLL_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 63, "System PLL Input clock divider (REFDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_SYS_PLL_REFDIV__ 1

/*--
this:AddWidget("Spinbox", 1, 4095, "System PLL Feedback divider (FBDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("pll_sys_clk_fbdiv", this.ShowFrequency)
--*/
#define __CLK_SYS_PLL_FBDIV__ 133

/*--
this:AddWidget("Spinbox", 1, 7, "System PLL Post divider 1 (POSTDIV1)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_SYS_PLL_POSTDIV1__ 6

/*--
this:AddWidget("Spinbox", 1, 7, "System PLL Post divider 2 (POSTDIV2)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL_SYS", this.ShowFrequency)
--*/
#define __CLK_SYS_PLL_POSTDIV2__ 2

//==============================================================================
// USB PLL Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "USB PLL enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_USB_PLL_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 63, "USB PLL Input clock divider (REFDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_USB_PLL_REFDIV__ 1

/*--
this:AddWidget("Spinbox", 1, 4095, "USB PLL Feedback divider (FBDIV)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("pll_usb_clk_fbdiv", this.ShowFrequency)
--*/
#define __CLK_USB_PLL_FBDIV__ 64

/*--
this:AddWidget("Spinbox", 1, 7, "USB PLL Post divider 1 (POSTDIV1)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_USB_PLL_POSTDIV1__ 4

/*--
this:AddWidget("Spinbox", 1, 7, "USB PLL Post divider 2 (POSTDIV2)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL_USB", this.ShowFrequency)
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
this:AddWidget("Combobox", "REF Clock source")
this:AddItem("ROSC", "_CLK_REF_CLK_SRC_ROSC")
this:AddItem("XOSC", "_CLK_REF_CLK_SRC_XOSC")
this:AddItem("PLL USB", "_CLK_REF_CLK_SRC_PLL_USB")
this:AddItem("GPIN0", "_CLK_REF_CLK_SRC_GPIN0")
this:AddItem("GPIN1", "_CLK_REF_CLK_SRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("clk_ref_in", this.ShowFrequency)
--*/
#define __CLK_REF_CLK_SRC__ _CLK_REF_CLK_SRC_XOSC

/*--
this:AddWidget("Spinbox", 1, 3, "REF Clock divider (CLK_REF)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_REF", this.ShowFrequency)
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
this:AddWidget("Combobox", "USB Clock source")
this:AddItem("PLL USB", "_CLK_USB_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_USB_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_USB_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_USB_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_USB_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_USB_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("clk_usb_in", this.ShowFrequency)
--*/
#define __CLK_USB_CLK_SRC__ _CLK_USB_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 3, "USB Clock divider (CLK_USB)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_USB", this.ShowFrequency)
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
this:AddWidget("Combobox", "ADC Clock source")
this:AddItem("PLL USB", "_CLK_ADC_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_ADC_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_ADC_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_ADC_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_ADC_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_ADC_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("adc_clk_in", this.ShowFrequency)
--*/
#define __CLK_ADC_CLK_SRC__ _CLK_ADC_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 3, "ADC Clock divider (CLK_ADC)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_ADC", this.ShowFrequency)
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
this:AddWidget("Combobox", "RTC Clock source")
this:AddItem("PLL USB", "_CLK_RTC_CLK_AUXSRC_PLL_USB")
this:AddItem("PLL SYS", "_CLK_RTC_CLK_AUXSRC_PLL_SYS")
this:AddItem("ROSC", "_CLK_RTC_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_RTC_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_RTC_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_RTC_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("rtc_clk_in", this.ShowFrequency)
--*/
#define __CLK_RTC_CLK_SRC__ _CLK_RTC_CLK_AUXSRC_PLL_USB

/*--
this:AddWidget("Spinbox", 1, 16777215, "RTC Clock divider (CLK_RTC)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_RTC", this.ShowFrequency)
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
this:AddWidget("Combobox", "System Clock source")
this:AddItem("clk_ref", "_CLK_SYS_CLK_SRC_CLK_REF")
this:AddItem("PLL SYS", "_CLK_SYS_CLK_SRC_PLL_SYS")
this:AddItem("PLL_USB", "_CLK_SYS_CLK_SRC_PLL_USB")
this:AddItem("ROSC", "_CLK_SYS_CLK_SRC_ROSC")
this:AddItem("XOSC", "_CLK_SYS_CLK_SRC_XOSC")
this:AddItem("GPIN0", "_CLK_SYS_CLK_SRC_GPIN0")
this:AddItem("GPIN1", "_CLK_SYS_CLK_SRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("sys_clk_in", this.ShowFrequency)
--*/
#define __CLK_SYS_CLK_SRC__ _CLK_SYS_CLK_SRC_PLL_SYS

/*--
this:AddWidget("Spinbox", 1, 16777215, "System Clock divider (CLK_SYS)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_SYS", this.ShowFrequency)
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
this:AddWidget("Combobox", "Peripherals Clock source (CLK_PERI)")
this:AddItem("sys_clk", "_CLK_PERI_CLK_AUXSRC_CLK_SYS")
this:AddItem("PLL SYS", "_CLK_PERI_CLK_AUXSRC_PLL_SYS")
this:AddItem("PLL USB", "_CLK_PERI_CLK_AUXSRC_PLL_USB")
this:AddItem("ROSC", "_CLK_PERI_CLK_AUXSRC_ROSC")
this:AddItem("XOSC", "_CLK_PERI_CLK_AUXSRC_XOSC")
this:AddItem("GPIN0", "_CLK_PERI_CLK_AUXSRC_GPIN0")
this:AddItem("GPIN1", "_CLK_PERI_CLK_AUXSRC_GPIN1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_PERI", this.ShowFrequency)
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
this:AddWidget("Combobox", "GPOUT0 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPOUT0_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "GPOUT0 Clock source")
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
this:SetValueFormatter("gpout0_clk_in", this.ShowFrequency)
--*/
#define __CLK_GPOUT0_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "GPOUT0 Clock divider (CLK_GPOUT0)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_GPOUT0", this.ShowFrequency)
--*/
#define __CLK_GPOUT0_CLK_DIV__ 1

/*--
this:AddWidget("Combobox", "GPOUT1 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPOUT1_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "GPOUT1 Clock source")
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
this:SetValueFormatter("gpout1_clk_in", this.ShowFrequency)
--*/
#define __CLK_GPOUT1_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "GPOUT1 Clock divider (CLK_GPOUT1)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_GPOUT1", this.ShowFrequency)
--*/
#define __CLK_GPOUT1_CLK_DIV__ 1

/*--
this:AddWidget("Combobox", "GPOUT2 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPOUT2_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "GPOUT2 Clock source")
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
this:SetValueFormatter("gpout2_clk_in", this.ShowFrequency)
--*/
#define __CLK_GPOUT2_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "GPOUT2 Clock divider (CLK_GPOUT2)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_GPOUT2", this.ShowFrequency)
--*/
#define __CLK_GPOUT2_CLK_DIV__ 1

/*--
this:AddWidget("Combobox", "GPOUT3 enable")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_GPOUT3_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "GPOUT3 Clock source")
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
this:SetValueFormatter("gpout3_clk_in", this.ShowFrequency)
--*/
#define __CLK_GPOUT3_CLK_SRC__ _CLK_GPOUT_AUXSRC_ROSC

/*--
this:AddWidget("Spinbox", 1, 16777215, "GPOUT3 Clock divider (CLK_GPOUT3)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLK_GPOUT3", this.ShowFrequency)
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

    -- PLL SYS frequency
    local pll_sys_in = freq.XOSC
    if this:GetFlagValue("__CLK_SYS_PLL_ENABLE__") == "_NO_" then pll_sys_in = 0 end
    local refdiv = this:GetFlagValue("__CLK_SYS_PLL_REFDIV__")
    local fbdiv = this:GetFlagValue("__CLK_SYS_PLL_FBDIV__")
    local postdiv1 = this:GetFlagValue("__CLK_SYS_PLL_POSTDIV1__")
    local postdiv2 = this:GetFlagValue("__CLK_SYS_PLL_POSTDIV2__")
    freq.pll_sys_clk_fbdiv = (pll_sys_in / refdiv * fbdiv)
    freq.PLL_SYS = freq.pll_sys_clk_fbdiv / (postdiv1 * postdiv2)
    local msg = ", out of 750-1600 MHz"
    if freq.pll_sys_clk_fbdiv >= 750e6 and freq.pll_sys_clk_fbdiv <= 1600e6 then msg = "" end
    freq.pll_sys_clk_fbdiv = PrintFrequency(tonumber(freq.pll_sys_clk_fbdiv))..msg

    -- PLL USB frequency
    local pll_usb_in = freq.XOSC
    if this:GetFlagValue("__CLK_USB_PLL_ENABLE__") == "_NO_" then pll_usb_in = 0 end
    local refdiv = this:GetFlagValue("__CLK_USB_PLL_REFDIV__")
    local fbdiv = this:GetFlagValue("__CLK_USB_PLL_FBDIV__")
    local postdiv1 = this:GetFlagValue("__CLK_USB_PLL_POSTDIV1__")
    local postdiv2 = this:GetFlagValue("__CLK_USB_PLL_POSTDIV2__")
    freq.pll_usb_clk_fbdiv = (pll_usb_in / refdiv * fbdiv)
    freq.PLL_USB = freq.pll_usb_clk_fbdiv / (postdiv1 * postdiv2)
    local msg = ", out of 750-1600 MHz"
    if freq.pll_usb_clk_fbdiv >= 750e6 and freq.pll_usb_clk_fbdiv <= 1600e6 then msg = "" end
    freq.pll_usb_clk_fbdiv = PrintFrequency(tonumber(freq.pll_usb_clk_fbdiv))..msg

    -- CLK_REF
    local SRC = this:GetFlagValue("__CLK_REF_CLK_SRC__"):gsub("_CLK_REF_CLK_SRC_", "")
    local div = this:GetFlagValue("__CLK_REF_CLK_DIV__")
    freq.clk_ref_in = freq[SRC]
    freq.CLK_REF = freq.clk_ref_in / div

    -- CLK_USB
    local SRC = this:GetFlagValue("__CLK_USB_CLK_SRC__"):gsub("_CLK_USB_CLK_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_USB_CLK_DIV__")
    freq.clk_usb_in = freq[SRC]
    freq.CLK_USB = freq.clk_usb_in / div

    -- CLK_ADC
    local SRC = this:GetFlagValue("__CLK_ADC_CLK_SRC__"):gsub("_CLK_ADC_CLK_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_ADC_CLK_DIV__")
    freq.adc_clk_in = freq[SRC]
    freq.CLK_ADC = freq.adc_clk_in / div

    -- CLK_RTC
    local SRC = this:GetFlagValue("__CLK_RTC_CLK_SRC__"):gsub("_CLK_RTC_CLK_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_RTC_CLK_DIV__")
    freq.rtc_clk_in = freq[SRC]
    freq.CLK_RTC = freq.rtc_clk_in / div

    -- CLK_SYS
    local SRC = this:GetFlagValue("__CLK_SYS_CLK_SRC__"):gsub("_CLK_SYS_CLK_SRC_", "")
    local div = this:GetFlagValue("__CLK_SYS_CLK_DIV__")
    freq.sys_clk_in = freq[SRC]
    freq.CLK_SYS = freq.sys_clk_in / div

    -- CLK_PERI
    local SRC = this:GetFlagValue("__CLK_PERI_CLK_SRC__"):gsub("_CLK_PERI_CLK_AUXSRC_", "")
    freq.CLK_PERI = freq[SRC] / div

    -- GPOUT0
    local EN = ifs(this:GetFlagValue("__CLK_GPOUT0_ENABLE__") == "_YES_", 1, 0)
    local SRC = this:GetFlagValue("__CLK_GPOUT0_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_GPOUT0_CLK_DIV__")
    freq.gpout0_clk_in = freq[SRC] * EN
    freq.CLK_GPOUT0 = freq.gpout0_clk_in / div

    -- GPOUT1
    local EN = ifs(this:GetFlagValue("__CLK_GPOUT1_ENABLE__") == "_YES_", 1, 0)
    local SRC = this:GetFlagValue("__CLK_GPOUT1_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_GPOUT1_CLK_DIV__")
    freq.gpout1_clk_in = freq[SRC] * EN
    freq.CLK_GPOUT1 = freq.gpout1_clk_in / div

    -- GPOUT2
    local EN = ifs(this:GetFlagValue("__CLK_GPOUT2_ENABLE__") == "_YES_", 1, 0)
    local SRC = this:GetFlagValue("__CLK_GPOUT2_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_GPOUT2_CLK_DIV__")
    freq.gpout2_clk_in = freq[SRC] * EN
    freq.CLK_GPOUT2 = freq.gpout2_clk_in / div

    -- GPOUT3
    local EN = ifs(this:GetFlagValue("__CLK_GPOUT3_ENABLE__") == "_YES_", 1, 0)
    local SRC = this:GetFlagValue("__CLK_GPOUT3_CLK_SRC__"):gsub("_CLK_GPOUT_AUXSRC_", "")
    local div = this:GetFlagValue("__CLK_GPOUT3_CLK_DIV__")
    freq.gpout3_clk_in = freq[SRC] * EN
    freq.CLK_GPOUT3 = freq.gpout3_clk_in / div

    this.freq = freq
end

-- trigger frequency calculation after loading all flags
this.CalculateFreq()
++*/

#endif /* _CLK_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
