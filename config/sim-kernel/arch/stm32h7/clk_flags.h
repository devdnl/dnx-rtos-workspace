/*==============================================================================
@file    clk_flags.h

@author  Daniel Zorychta

@brief   PLL module configuration flags.

@note    Copyright (C) 2020 Daniel Zorychta <daniel.zorychta@gmail.com>

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
this:AddWidget("Combobox", "Supply source")
this:AddItem("LDO supply", "PWR_LDO_SUPPLY")
this:AddItem("External supply source", "PWR_EXTERNAL_SOURCE_SUPPLY")
this:AddItem("Direct SMPS supply", "PWR_DIRECT_SMPS_SUPPLY")
this:AddItem("SMPS supplies LDO 1.8V", "PWR_SMPS_1V8_SUPPLIES_LDO")
this:AddItem("SMPS supplies LDO 2.5V", "PWR_SMPS_2V5_SUPPLIES_LDO")
this:AddItem("SMPS supplies EXT and LDO 1.8V", "PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO")
this:AddItem("SMPS supplies EXT and LDO 2.5V", "PWR_SMPS_2V5_SUPPLIES_EXT_AND_LDO")
this:AddItem("SMPS supplies EXT 1.8V", "PWR_SMPS_1V8_SUPPLIES_EXT")
this:AddItem("SMPS supplies EXT 2.5V", "PWR_SMPS_2V5_SUPPLIES_EXT")
--*/
#define __CLK_SUPPLY_SOURCE__ PWR_LDO_SUPPLY

/*--
this:AddWidget("Combobox", "LSI oscillator")
this:AddItem("Disable", "DISABLE")
this:AddItem("Enable", "ENABLE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LSI", this.ShowFrequency)
--*/
#define __CLK_LSI_ON__ ENABLE

/*--
this:AddWidget("Combobox", "LSE oscillator")
this:AddItem("Disable", "DISABLE")
this:AddItem("Enable", "ENABLE")
this:AddItem("Baypass", "_CLK_BAYPASS")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LSE", this.ShowFrequency)
--*/
#define __CLK_LSE_ON__ ENABLE

/*--
this:AddWidget("Combobox", "LSE drive capability")
this:AddItem("Low", "LL_RCC_LSEDRIVE_LOW")
this:AddItem("Medium Low", "LL_RCC_LSEDRIVE_MEDIUMLOW")
this:AddItem("Medium High", "LL_RCC_LSEDRIVE_MEDIUMHIGH")
this:AddItem("High", "LL_RCC_LSEDRIVE_HIGH")
--*/
#define __CLK_LSE_DRIVE_CAPABILITY__ LL_RCC_LSEDRIVE_HIGH

/*--
this:AddWidget("Combobox", "HSE oscillator")
this:AddItem("Disable", "DISABLE")
this:AddItem("Enable", "ENABLE")
this:AddItem("Baypass", "_CLK_BAYPASS")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("HSE", this.ShowFrequency)
--*/
#define __CLK_HSE_ON__ _CLK_BAYPASS


//==============================================================================
// PLL clock source
//==============================================================================
/*--
this:AddWidget("Combobox", "PLL Clock source (ref_ck)")
this:AddItem("HSI", "LL_RCC_PLLSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_PLLSOURCE_CSI")
this:AddItem("HSE", "LL_RCC_PLLSOURCE_HSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("ref_ck", this.ShowFrequency)
--*/
#define __CLK_PLL_SRC__ LL_RCC_PLLSOURCE_HSI


//==============================================================================
// PLL1 clock configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "PLL1 Enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_ON__ _YES_

/*--
this:AddWidget("Spinbox", 0, 63, "PLL1 ref_ck divider (M)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_M__ 8

/*--
this:AddWidget("Spinbox", 4, 512, "PLL1 multiplier (N)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_N__ 60

/*--
this:AddWidget("Combobox", "PLL1 P clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_P_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "PLL1 divider P (pll1_p_ck)")
this:AddItem("1", "1")
for i = 2, 128, 2 do this:AddItem(tostring(i), tostring(i)) end
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL1P", this.ShowFrequency)
--*/
#define __CLK_PLL1_P__ 1

/*--
this:AddWidget("Combobox", "PLL1 Q clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_Q_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL1 divider Q (pll1_q_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL1Q", this.ShowFrequency)
--*/
#define __CLK_PLL1_Q__ 1

/*--
this:AddWidget("Combobox", "PLL1 R clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL1_R_ENABLE__ _NO_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL1 divider R (pll1_r_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL1R", this.ShowFrequency)
--*/
#define __CLK_PLL1_R__ 1


//==============================================================================
// PLL2 clock configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "PLL2 Enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_ON__ _YES_

/*--
this:AddWidget("Spinbox", 0, 63, "PLL2 ref_ck divider (M)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_M__ 8

/*--
this:AddWidget("Spinbox", 4, 512, "PLL2 multiplier (N)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_N__ 60

/*--
this:AddWidget("Combobox", "PLL2 P clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_P_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL2 divider P (pll2_p_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL2P", this.ShowFrequency)
--*/
#define __CLK_PLL2_P__ 1

/*--
this:AddWidget("Combobox", "PLL2 Q clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_Q_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL2 divider Q (pll2_q_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL1Q", this.ShowFrequency)
--*/
#define __CLK_PLL2_Q__ 1

/*--
this:AddWidget("Combobox", "PLL2 R clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL2_R_ENABLE__ _NO_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL2 divider R (pll2_r_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL2R", this.ShowFrequency)
--*/
#define __CLK_PLL2_R__ 1


//==============================================================================
// PLL3 clock configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "PLL3 Enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_ON__ _YES_

/*--
this:AddWidget("Spinbox", 0, 63, "PLL3 ref_ck divider (M)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_M__ 8

/*--
this:AddWidget("Spinbox", 4, 512, "PLL3 multiplier (N)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_N__ 60

/*--
this:AddWidget("Combobox", "PLL3 P clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_P_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL3 divider P (pll3_p_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL3P", this.ShowFrequency)
--*/
#define __CLK_PLL3_P__ 1

/*--
this:AddWidget("Combobox", "PLL3 Q clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_Q_ENABLE__ _YES_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL3 divider Q (pll3_q_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL3Q", this.ShowFrequency)
--*/
#define __CLK_PLL3_Q__ 1

/*--
this:AddWidget("Combobox", "PLL3 R clock enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL3_R_ENABLE__ _NO_

/*--
this:AddWidget("Spinbox", 1, 128, "PLL3 divider R (pll3_r_ck)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLL3R", this.ShowFrequency)
--*/
#define __CLK_PLL3_R__ 1


//==============================================================================
// Buses clock configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "SYSCLK source (sys_ck)")
this:AddItem("HSI (hsi_ck)", "LL_RCC_SYS_CLKSOURCE_HSI")
this:AddItem("CSI (csi_ck)", "LL_RCC_SYS_CLKSOURCE_CSI")
this:AddItem("HSE (hse_ck)", "LL_RCC_SYS_CLKSOURCE_HSE")
this:AddItem("PLL1 (pll1_p_ck)", "LL_RCC_SYS_CLKSOURCE_PLL1")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("sys_ck", this.ShowFrequency)
--*/
#define __CLK_SYSCLK_SRC__ LL_RCC_SYS_CLKSOURCE_PLL1

/*--
this:AddWidget("Combobox", "SYSCLK prescaler")
this:AddItem("sys_ck / 1", "LL_RCC_SYSCLK_DIV_1")
this:AddItem("sys_ck / 2", "LL_RCC_SYSCLK_DIV_2")
this:AddItem("sys_ck / 4", "LL_RCC_SYSCLK_DIV_4")
this:AddItem("sys_ck / 8", "LL_RCC_SYSCLK_DIV_8")
this:AddItem("sys_ck / 16", "LL_RCC_SYSCLK_DIV_16")
this:AddItem("sys_ck / 64", "LL_RCC_SYSCLK_DIV_64")
this:AddItem("sys_ck / 128", "LL_RCC_SYSCLK_DIV_128")
this:AddItem("sys_ck / 256", "LL_RCC_SYSCLK_DIV_256")
this:AddItem("sys_ck / 512", "LL_RCC_SYSCLK_DIV_512")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("sysclk", this.ShowFrequency)
--*/
#define __CLK_SYSCLK_DIV__ LL_RCC_SYSCLK_DIV_1

/*--
this:AddWidget("Combobox", "AHB prescaler")
this:AddItem("sys_ck / 1", "LL_RCC_AHB_DIV_1")
this:AddItem("sys_ck / 2", "LL_RCC_AHB_DIV_2")
this:AddItem("sys_ck / 4", "LL_RCC_AHB_DIV_4")
this:AddItem("sys_ck / 8", "LL_RCC_AHB_DIV_8")
this:AddItem("sys_ck / 16", "LL_RCC_AHB_DIV_16")
this:AddItem("sys_ck / 64", "LL_RCC_AHB_DIV_64")
this:AddItem("sys_ck / 128", "LL_RCC_AHB_DIV_128")
this:AddItem("sys_ck / 256", "LL_RCC_AHB_DIV_256")
this:AddItem("sys_ck / 512", "LL_RCC_AHB_DIV_512")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("ahbclk", this.ShowFrequency)
--*/
#define __CLK_AHB_DIV__ LL_RCC_AHB_DIV_2

/*--
this:AddWidget("Combobox", "APB1 prescaler")
this:AddItem("ahb_ck / 1", "LL_RCC_APB1_DIV_1")
this:AddItem("ahb_ck / 2", "LL_RCC_APB1_DIV_2")
this:AddItem("ahb_ck / 4", "LL_RCC_APB1_DIV_4")
this:AddItem("ahb_ck / 8", "LL_RCC_APB1_DIV_8")
this:AddItem("ahb_ck / 16", "LL_RCC_APB1_DIV_16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("apb1clk", this.ShowFrequency)
--*/
#define __CLK_APB1_DIV__ LL_RCC_APB1_DIV_2

/*--
this:AddWidget("Combobox", "APB2 prescaler")
this:AddItem("ahb_ck / 1", "LL_RCC_APB2_DIV_1")
this:AddItem("ahb_ck / 2", "LL_RCC_APB2_DIV_2")
this:AddItem("ahb_ck / 4", "LL_RCC_APB2_DIV_4")
this:AddItem("ahb_ck / 8", "LL_RCC_APB2_DIV_8")
this:AddItem("ahb_ck / 16", "LL_RCC_APB2_DIV_16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("apb2clk", this.ShowFrequency)
--*/
#define __CLK_APB2_DIV__ LL_RCC_APB2_DIV_2

/*--
this:AddWidget("Combobox", "APB3 prescaler")
this:AddItem("ahb_ck / 1", "LL_RCC_APB3_DIV_1")
this:AddItem("ahb_ck / 2", "LL_RCC_APB3_DIV_2")
this:AddItem("ahb_ck / 4", "LL_RCC_APB3_DIV_4")
this:AddItem("ahb_ck / 8", "LL_RCC_APB3_DIV_8")
this:AddItem("ahb_ck / 16", "LL_RCC_APB3_DIV_16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("apb3clk", this.ShowFrequency)
--*/
#define __CLK_APB3_DIV__ LL_RCC_APB3_DIV_2

/*--
this:AddWidget("Combobox", "APB4 prescaler")
this:AddItem("ahb_ck / 1", "LL_RCC_APB4_DIV_1")
this:AddItem("ahb_ck / 2", "LL_RCC_APB4_DIV_2")
this:AddItem("ahb_ck / 4", "LL_RCC_APB4_DIV_4")
this:AddItem("ahb_ck / 8", "LL_RCC_APB4_DIV_8")
this:AddItem("ahb_ck / 16", "LL_RCC_APB4_DIV_16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("apb4clk", this.ShowFrequency)
--*/
#define __CLK_APB4_DIV__ LL_RCC_APB4_DIV_4

//==============================================================================
// Peripheral clock configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "RTC HSE Clock divider")
this:AddItem("No clock", "LL_RCC_RTC_NOCLOCK")
for i=2,63 do this:AddItem(tostring(i), "LL_RCC_RTC_HSE_DIV_"..tostring(i)) end
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_RTC_HSE_DIV__ LL_RCC_RTC_NOCLOCK

/*--
this:AddWidget("Combobox", "RTC Clock source")
this:AddItem("None", "LL_RCC_RTC_CLKSOURCE_NONE")
this:AddItem("LSE clock", "LL_RCC_RTC_CLKSOURCE_LSE")
this:AddItem("LSI clock", "LL_RCC_RTC_CLKSOURCE_LSI")
this:AddItem("HSE clock", "LL_RCC_RTC_CLKSOURCE_HSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("RTC", this.ShowFrequency)
--*/
#define __CLK_SRC_RTC__ LL_RCC_RTC_CLKSOURCE_LSE

/*--
this:AddWidget("Combobox", "USART1..6 clock source")
this:AddItem("PCLK2", "LL_RCC_USART16_CLKSOURCE_PCLK2")
this:AddItem("PLL2Q", "LL_RCC_USART16_CLKSOURCE_PLL2Q")
this:AddItem("PLL3Q", "LL_RCC_USART16_CLKSOURCE_PLL3Q")
this:AddItem("HSI", "LL_RCC_USART16_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_USART16_CLKSOURCE_CSI")
this:AddItem("LSE", "LL_RCC_USART16_CLKSOURCE_LSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("UART16", this.ShowFrequency)
--*/
#define __CLK_SRC_UART16__ LL_RCC_USART16_CLKSOURCE_PCLK2

/*--
this:AddWidget("Combobox", "USART2..5,7..8 clock source")
this:AddItem("PCLK1", "LL_RCC_USART234578_CLKSOURCE_PCLK1")
this:AddItem("PLL2Q", "LL_RCC_USART234578_CLKSOURCE_PLL2Q")
this:AddItem("PLL3Q", "LL_RCC_USART234578_CLKSOURCE_PLL3Q")
this:AddItem("HSI", "LL_RCC_USART234578_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_USART234578_CLKSOURCE_CSI")
this:AddItem("LSE", "LL_RCC_USART234578_CLKSOURCE_LSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("UART234578", this.ShowFrequency)
--*/
#define __CLK_SRC_UART234578__ LL_RCC_USART234578_CLKSOURCE_PCLK1

/*--
this:AddWidget("Combobox", "LPUART1 clock source")
this:AddItem("PCLK4", "LL_RCC_LPUART1_CLKSOURCE_PCLK4")
this:AddItem("PLL2Q", "LL_RCC_LPUART1_CLKSOURCE_PLL2Q")
this:AddItem("PLL3Q", "LL_RCC_LPUART1_CLKSOURCE_PLL3Q")
this:AddItem("HSI", "LL_RCC_LPUART1_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_LPUART1_CLKSOURCE_CSI")
this:AddItem("LSE", "LL_RCC_LPUART1_CLKSOURCE_LSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LPUART1", this.ShowFrequency)
--*/
#define __CLK_SRC_LPUART1__ LL_RCC_LPUART1_CLKSOURCE_PCLK4

/*--
this:AddWidget("Combobox", "I2C1..3 clock source")
this:AddItem("PCLK1", "LL_RCC_I2C123_CLKSOURCE_PCLK1")
this:AddItem("PLL3R", "LL_RCC_I2C123_CLKSOURCE_PLL3R")
this:AddItem("HSI", "LL_RCC_I2C123_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_I2C123_CLKSOURCE_CSI")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("I2C123", this.ShowFrequency)
--*/
#define __CLK_SRC_I2C123__ LL_RCC_I2C123_CLKSOURCE_PCLK1

/*--
this:AddWidget("Combobox", "I2C4 clock source")
this:AddItem("PCLK4", "LL_RCC_I2C4_CLKSOURCE_PCLK4")
this:AddItem("PLL3R", "LL_RCC_I2C4_CLKSOURCE_PLL3R")
this:AddItem("HSI", "LL_RCC_I2C4_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_I2C4_CLKSOURCE_CSI")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("I2C4", this.ShowFrequency)
--*/
#define __CLK_SRC_I2C4__ LL_RCC_I2C4_CLKSOURCE_PCLK4

/*--
this:AddWidget("Combobox", "LPTIM1 clock source")
this:AddItem("PCLK1", "LL_RCC_LPTIM1_CLKSOURCE_PCLK1")
this:AddItem("PLL2P", "LL_RCC_LPTIM1_CLKSOURCE_PLL2P")
this:AddItem("PLL3R", "LL_RCC_LPTIM1_CLKSOURCE_PLL3R")
this:AddItem("LSE", "LL_RCC_LPTIM1_CLKSOURCE_LSE")
this:AddItem("LSI", "LL_RCC_LPTIM1_CLKSOURCE_LSI")
this:AddItem("CLKP", "LL_RCC_LPTIM1_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LPTIM1", this.ShowFrequency)
--*/
#define __CLK_SRC_LPTIM1__ LL_RCC_LPTIM1_CLKSOURCE_LSE

/*--
this:AddWidget("Combobox", "LPTIM2 clock source")
this:AddItem("PCLK4", "LL_RCC_LPTIM2_CLKSOURCE_PCLK4")
this:AddItem("PLL2P", "LL_RCC_LPTIM2_CLKSOURCE_PLL2P")
this:AddItem("PLL3R", "LL_RCC_LPTIM2_CLKSOURCE_PLL3R")
this:AddItem("LSE", "LL_RCC_LPTIM2_CLKSOURCE_LSE")
this:AddItem("LSI", "LL_RCC_LPTIM2_CLKSOURCE_LSI")
this:AddItem("CLKP", "LL_RCC_LPTIM2_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LPTIM2", this.ShowFrequency)
--*/
#define __CLK_SRC_LPTIM2__ LL_RCC_LPTIM2_CLKSOURCE_LSE

/*--
this:AddWidget("Combobox", "LPTIM345 clock source")
this:AddItem("PCLK4", "LL_RCC_LPTIM345_CLKSOURCE_PCLK4")
this:AddItem("PLL2P", "LL_RCC_LPTIM345_CLKSOURCE_PLL2P")
this:AddItem("PLL3R", "LL_RCC_LPTIM345_CLKSOURCE_PLL3R")
this:AddItem("LSE", "LL_RCC_LPTIM345_CLKSOURCE_LSE")
this:AddItem("LSI", "LL_RCC_LPTIM345_CLKSOURCE_LSI")
this:AddItem("CLKP", "LL_RCC_LPTIM345_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LPTIM345", this.ShowFrequency)
--*/
#define __CLK_SRC_LPTIM345__ LL_RCC_LPTIM345_CLKSOURCE_LSE

/*--
this:AddWidget("Combobox", "CLKP clock source")
this:AddItem("HSI", "LL_RCC_CLKP_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_CLKP_CLKSOURCE_CSI")
this:AddItem("HSE", "LL_RCC_CLKP_CLKSOURCE_HSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CLKP", this.ShowFrequency)
--*/
#define __CLK_SRC_CLKP__ LL_RCC_CLKP_CLKSOURCE_HSI

/*--
this:AddWidget("Combobox", "SPDIF clock source")
this:AddItem("PLL1Q", "LL_RCC_SPDIF_CLKSOURCE_PLL1Q")
this:AddItem("PLL2R", "LL_RCC_SPDIF_CLKSOURCE_PLL2R")
this:AddItem("PLL3R", "LL_RCC_SPDIF_CLKSOURCE_PLL3R")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SPDIF", this.ShowFrequency)
--*/
#define __CLK_SRC_SPDIF__ LL_RCC_SPDIF_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI1 clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI1_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI1_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI1_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI1_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI1_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI1", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI1__ LL_RCC_SAI1_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI23 clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI23_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI23_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI23_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI23_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI23_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI23", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI23__ LL_RCC_SAI23_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI2A clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI2A_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI2A_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI2A_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI2A_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI2A_CLKSOURCE_CLKP")
this:AddItem("SPDIF", "LL_RCC_SAI2A_CLKSOURCE_SPDIF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI2A", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI2A__ LL_RCC_SAI2A_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI2B clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI2B_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI2B_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI2B_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI2B_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI2B_CLKSOURCE_CLKP")
this:AddItem("SPDIF", "LL_RCC_SAI2B_CLKSOURCE_SPDIF")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI2B", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI2B__ LL_RCC_SAI2B_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI4A clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI4A_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI4A_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI4A_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI4A_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI4A_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI4A", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI4A__ LL_RCC_SAI4A_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SAI4B clock source")
this:AddItem("PLL1Q", "LL_RCC_SAI4B_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SAI4B_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SAI4B_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SAI4B_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SAI4B_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SAI4B", this.ShowFrequency)
--*/
#define __CLK_SRC_SAI4B__ LL_RCC_SAI4B_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SDMMC clock source")
this:AddItem("PLL1Q", "LL_RCC_SDMMC_CLKSOURCE_PLL1Q")
this:AddItem("PLL2R", "LL_RCC_SDMMC_CLKSOURCE_PLL2R")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SDMMC", this.ShowFrequency)
--*/
#define __CLK_SRC_SDMMC__ LL_RCC_SDMMC_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "RNG clock source")
this:AddItem("HSI48", "LL_RCC_RNG_CLKSOURCE_HSI48")
this:AddItem("PLL1Q", "LL_RCC_RNG_CLKSOURCE_PLL1Q")
this:AddItem("LSE", "LL_RCC_RNG_CLKSOURCE_LSE")
this:AddItem("LSI", "LL_RCC_RNG_CLKSOURCE_LSI")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("RNG", this.ShowFrequency)
--*/
#define __CLK_SRC_RNG__ LL_RCC_RNG_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "USB clock source")
this:AddItem("DISABLE", "LL_RCC_USB_CLKSOURCE_DISABLE")
this:AddItem("PLL1Q", "LL_RCC_USB_CLKSOURCE_PLL1Q")
this:AddItem("PLL3Q", "LL_RCC_USB_CLKSOURCE_PLL3Q")
this:AddItem("HSI48", "LL_RCC_USB_CLKSOURCE_HSI48")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("USB", this.ShowFrequency)
--*/
#define __CLK_SRC_USB__ LL_RCC_USB_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "CEC clock source")
this:AddItem("LSE", "LL_RCC_CEC_CLKSOURCE_LSE")
this:AddItem("LSI", "LL_RCC_CEC_CLKSOURCE_LSI")
this:AddItem("CSI_DIV122", "LL_RCC_CEC_CLKSOURCE_CSI_DIV122")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("CEC", this.ShowFrequency)
--*/
#define __CLK_SRC_CEC__ LL_RCC_CEC_CLKSOURCE_LSE

/*--
this:AddWidget("Combobox", "DFSDM1 clock source")
this:AddItem("PCLK2", "LL_RCC_DFSDM1_CLKSOURCE_PCLK2")
this:AddItem("SYSCLK", "LL_RCC_DFSDM1_CLKSOURCE_SYSCLK")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("DFSDM1", this.ShowFrequency)
--*/
#define __CLK_SRC_DFSDM1__ LL_RCC_DFSDM1_CLKSOURCE_PCLK2

/*--
this:AddWidget("Combobox", "DFSDM2 clock source")
this:AddItem("PCLK4", "LL_RCC_DFSDM2_CLKSOURCE_PCLK4")
this:AddItem("SYSCLK", "LL_RCC_DFSDM2_CLKSOURCE_SYSCLK")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("DFSDM2", this.ShowFrequency)
--*/
#define __CLK_SRC_DFSDM2__ LL_RCC_DFSDM2_CLKSOURCE_PCLK4

/*--
this:AddWidget("Combobox", "FMC clock source")
this:AddItem("HCLK", "LL_RCC_FMC_CLKSOURCE_HCLK")
this:AddItem("PLL1Q", "LL_RCC_FMC_CLKSOURCE_PLL1Q")
this:AddItem("PLL2R", "LL_RCC_FMC_CLKSOURCE_PLL2R")
this:AddItem("CLKP", "LL_RCC_FMC_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("FMC", this.ShowFrequency)
--*/
#define __CLK_SRC_FMC__ LL_RCC_FMC_CLKSOURCE_HCLK

/*--
this:AddWidget("Combobox", "QSPI clock source")
this:AddItem("HCLK", "LL_RCC_QSPI_CLKSOURCE_HCLK")
this:AddItem("PLL1Q", "LL_RCC_QSPI_CLKSOURCE_PLL1Q")
this:AddItem("PLL2R", "LL_RCC_QSPI_CLKSOURCE_PLL2R")
this:AddItem("CLKP", "LL_RCC_QSPI_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("QSPI", this.ShowFrequency)
--*/
#define __CLK_SRC_QSPI__ LL_RCC_QSPI_CLKSOURCE_HCLK

/*--
this:AddWidget("Combobox", "OSPI clock source")
this:AddItem("HCLK", "LL_RCC_OSPI_CLKSOURCE_HCLK")
this:AddItem("PLL1Q", "LL_RCC_OSPI_CLKSOURCE_PLL1Q")
this:AddItem("PLL2R", "LL_RCC_OSPI_CLKSOURCE_PLL2R")
this:AddItem("CLKP", "LL_RCC_OSPI_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("OSPI", this.ShowFrequency)
--*/
#define __CLK_SRC_OSPI__ LL_RCC_OSPI_CLKSOURCE_HCLK

/*--
this:AddWidget("Combobox", "DSI clock source")
this:AddItem("PHY", "LL_RCC_DSI_CLKSOURCE_PHY")
this:AddItem("PLL2Q", "LL_RCC_DSI_CLKSOURCE_PLL2Q")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("DSI", this.ShowFrequency)
--*/
#define __CLK_SRC_DSI__ LL_RCC_DSI_CLKSOURCE_PHY

/*--
this:AddWidget("Combobox", "SPI1..3 clock source")
this:AddItem("PLL1Q", "LL_RCC_SPI123_CLKSOURCE_PLL1Q")
this:AddItem("PLL2P", "LL_RCC_SPI123_CLKSOURCE_PLL2P")
this:AddItem("PLL3P", "LL_RCC_SPI123_CLKSOURCE_PLL3P")
this:AddItem("I2S_CKIN", "LL_RCC_SPI123_CLKSOURCE_I2S_CKIN")
this:AddItem("CLKP", "LL_RCC_SPI123_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SPI123", this.ShowFrequency)
--*/
#define __CLK_SRC_SPI123__ LL_RCC_SPI123_CLKSOURCE_PLL1Q

/*--
this:AddWidget("Combobox", "SPI4..5 clock source")
this:AddItem("PCLK2", "LL_RCC_SPI45_CLKSOURCE_PCLK2")
this:AddItem("PLL2Q", "LL_RCC_SPI45_CLKSOURCE_PLL2Q")
this:AddItem("PLL3Q", "LL_RCC_SPI45_CLKSOURCE_PLL3Q")
this:AddItem("HSI", "LL_RCC_SPI45_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_SPI45_CLKSOURCE_CSI")
this:AddItem("HSE", "LL_RCC_SPI45_CLKSOURCE_HSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SPI45", this.ShowFrequency)
--*/
#define __CLK_SRC_SPI45__ LL_RCC_SPI45_CLKSOURCE_PCLK2

/*--
this:AddWidget("Combobox", "SPI6 clock source")
this:AddItem("PCLK4", "LL_RCC_SPI6_CLKSOURCE_PCLK4")
this:AddItem("PLL2Q", "LL_RCC_SPI6_CLKSOURCE_PLL2Q")
this:AddItem("PLL3Q", "LL_RCC_SPI6_CLKSOURCE_PLL3Q")
this:AddItem("HSI", "LL_RCC_SPI6_CLKSOURCE_HSI")
this:AddItem("CSI", "LL_RCC_SPI6_CLKSOURCE_CSI")
this:AddItem("HSE", "LL_RCC_SPI6_CLKSOURCE_HSE")
this:AddItem("I2S_CKIN", "LL_RCC_SPI6_CLKSOURCE_I2S_CKIN")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SPI6", this.ShowFrequency)
--*/
#define __CLK_SRC_SPI6__ LL_RCC_SPI6_CLKSOURCE_PCLK4

/*--
this:AddWidget("Combobox", "FDCAN clock source")
this:AddItem("HSE", "LL_RCC_FDCAN_CLKSOURCE_HSE")
this:AddItem("PLL1Q", "LL_RCC_FDCAN_CLKSOURCE_PLL1Q")
this:AddItem("PLL2Q", "LL_RCC_FDCAN_CLKSOURCE_PLL2Q")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("FDCAN", this.ShowFrequency)
--*/
#define __CLK_SRC_FDCAN__ LL_RCC_FDCAN_CLKSOURCE_HSE

/*--
this:AddWidget("Combobox", "SWP clock source")
this:AddItem("PCLK1", "LL_RCC_SWP_CLKSOURCE_PCLK1")
this:AddItem("HSI", "LL_RCC_SWP_CLKSOURCE_HSI")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SWP", this.ShowFrequency)
--*/
#define __CLK_SRC_SWP__ LL_RCC_SWP_CLKSOURCE_PCLK1

/*--
this:AddWidget("Combobox", "ADC clock source")
this:AddItem("PLL2P", "LL_RCC_ADC_CLKSOURCE_PLL2P")
this:AddItem("PLL3R", "LL_RCC_ADC_CLKSOURCE_PLL3R")
this:AddItem("CLKP", "LL_RCC_ADC_CLKSOURCE_CLKP")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("ADC", this.ShowFrequency)
--*/
#define __CLK_SRC_ADC__ LL_RCC_ADC_CLKSOURCE_PLL2P

//==============================================================================
// Calculated values
//==============================================================================
// Value is calculated according to SYSCLK source selection
#define __CLK_SYSCLK_SRC_STATUS__ LL_RCC_SYS_CLKSOURCE_STATUS_PLL1

// Value is calculated according to SYSCLK source selection
#define __CLK_HCLK_FREQ__ 240000000
#define __CLK_SYSCLK_FREQ__ 400000000

//==============================================================================
// Calculation script
//==============================================================================
/*--
this.CalculateFreq = function(self)
    local freq = {}
    freq.NONE     = 0
    freq.DISABLE  = 0
    freq.PHY      = 100e6
    freq.I2S_CKIN = 192e3

    -- HSE clock frequency -----------------------------------------------------
    local HSEON = iff(this:GetFlagValue("__CLK_HSE_ON__") == "DISABLE", 0, 1)
    freq.HSE = uC.OSCFREQ * HSEON

    -- HSI & CSI clock frequency -----------------------------------------------
    freq.HSI = 64e6
    freq.CSI = 4e6
    freq.CSI_DIV122 = freq.CSI / 122
    freq.HSI48 = 48e6

    -- LSE clock frequency -----------------------------------------------------
    local LSEON = iff(this:GetFlagValue("__CLK_LSE_ON__") == "DISABLE", 0, 1)
    freq.LSE = 32768 * LSEON

    -- LSI clock frequency -----------------------------------------------------
    local LSION = iff(this:GetFlagValue("__CLK_LSI_ON__") == "DISABLE", 0, 1)
    freq.LSI = 32e3 * LSION

    -- PLLs clock source -------------------------------------------------------
    local PLLSRC = this:GetFlagValue("__CLK_PLL_SRC__")
    if PLLSRC == "LL_RCC_PLLSOURCE_HSI" then
        freq.ref_ck = freq.HSI
    elseif PLLSRC == "LL_RCC_PLLSOURCE_HSE" then
        freq.ref_ck = freq.HSE
    elseif PLLSRC == "LL_RCC_PLLSOURCE_CSI" then
        freq.ref_ck = freq.CSI
    else
        freq.ref_ck = 0
    end

    -- PLL1 clock frequency ----------------------------------------------------
    local PLL1ON = iff(this:GetFlagValue("__CLK_PLL1_ON__") == "_NO_", 0, 1)
    local DIVM1  = this:GetFlagValue("__CLK_PLL1_M__")
    local MULN1  = this:GetFlagValue("__CLK_PLL1_N__")
    local DIVP1  = this:GetFlagValue("__CLK_PLL1_P__")
    local DIVQ1  = this:GetFlagValue("__CLK_PLL1_Q__")
    local DIVR1  = this:GetFlagValue("__CLK_PLL1_R__")
    local PLLPON = iff(this:GetFlagValue("__CLK_PLL1_P_ENABLE__") == "_NO_", 0, 1)
    local PLLQON = iff(this:GetFlagValue("__CLK_PLL1_Q_ENABLE__") == "_NO_", 0, 1)
    local PLLRON = iff(this:GetFlagValue("__CLK_PLL1_R_ENABLE__") == "_NO_", 0, 1)

    freq.pll1_ck = freq.ref_ck / tonumber(DIVM1) * tonumber(MULN1) * PLL1ON

    freq.pll1_p_ck = freq.pll1_ck / tonumber(DIVP1) * PLLPON
    freq.PLL1P = freq.pll1_p_ck

    freq.pll1_q_ck = freq.pll1_ck / tonumber(DIVQ1) * PLLQON
    freq.PLL1Q = freq.pll1_q_ck

    freq.pll1_r_ck = freq.pll1_ck / tonumber(DIVR1) * PLLRON
    freq.PLL1R = freq.pll1_r_ck


    -- PLL2 clock frequency ----------------------------------------------------
    local PLL2ON = iff(this:GetFlagValue("__CLK_PLL2_ON__") == "_NO_", 0, 1)
    local DIVM1  = this:GetFlagValue("__CLK_PLL2_M__")
    local MULN1  = this:GetFlagValue("__CLK_PLL2_N__")
    local DIVP1  = this:GetFlagValue("__CLK_PLL2_P__")
    local DIVQ1  = this:GetFlagValue("__CLK_PLL2_Q__")
    local DIVR1  = this:GetFlagValue("__CLK_PLL2_R__")
    local PLLPON = iff(this:GetFlagValue("__CLK_PLL2_P_ENABLE__") == "_NO_", 0, 1)
    local PLLQON = iff(this:GetFlagValue("__CLK_PLL2_Q_ENABLE__") == "_NO_", 0, 1)
    local PLLRON = iff(this:GetFlagValue("__CLK_PLL2_R_ENABLE__") == "_NO_", 0, 1)

    freq.pll2_ck = freq.ref_ck / tonumber(DIVM1) * tonumber(MULN1) * PLL2ON

    freq.pll2_p_ck = freq.pll2_ck / tonumber(DIVP1) * PLLPON
    freq.PLL2P = freq.pll2_p_ck

    freq.pll2_q_ck = freq.pll2_ck / tonumber(DIVQ1) * PLLQON
    freq.PLL2Q = freq.pll2_q_ck

    freq.pll2_r_ck = freq.pll2_ck / tonumber(DIVR1) * PLLRON
    freq.PLL2R = freq.pll2_r_ck


    -- PLL3 clock frequency ----------------------------------------------------
    local PLL3ON = iff(this:GetFlagValue("__CLK_PLL3_ON__") == "_NO_", 0, 1)
    local DIVM1  = this:GetFlagValue("__CLK_PLL3_M__")
    local MULN1  = this:GetFlagValue("__CLK_PLL3_N__")
    local DIVP1  = this:GetFlagValue("__CLK_PLL3_P__")
    local DIVQ1  = this:GetFlagValue("__CLK_PLL3_Q__")
    local DIVR1  = this:GetFlagValue("__CLK_PLL3_R__")
    local PLLPON = iff(this:GetFlagValue("__CLK_PLL3_P_ENABLE__") == "_NO_", 0, 1)
    local PLLQON = iff(this:GetFlagValue("__CLK_PLL3_Q_ENABLE__") == "_NO_", 0, 1)
    local PLLRON = iff(this:GetFlagValue("__CLK_PLL3_R_ENABLE__") == "_NO_", 0, 1)

    freq.pll3_ck = freq.ref_ck / tonumber(DIVM1) * tonumber(MULN1) * PLL3ON

    freq.pll3_p_ck = freq.pll3_ck / tonumber(DIVP1) * PLLPON
    freq.PLL3P = freq.pll3_p_ck

    freq.pll3_q_ck = freq.pll3_ck / tonumber(DIVQ1) * PLLQON
    freq.PLL3Q = freq.pll3_q_ck

    freq.pll3_r_ck = freq.pll3_ck / tonumber(DIVR1) * PLLRON
    freq.PLL3R = freq.pll3_r_ck


    -- SYSCLK source
    local SYSCLKSRC = this:GetFlagValue("__CLK_SYSCLK_SRC__"):gsub("LL_RCC_SYS_CLKSOURCE_", "")

    if SYSCLKSRC == "HSI" then
        freq.sys_ck = freq.HSI
    elseif SYSCLKSRC == "HSE" then
        freq.sys_ck = freq.HSE
    elseif SYSCLKSRC == "CSI" then
        freq.sys_ck = freq.CSI
    elseif SYSCLKSRC == "PLL1" then
        freq.sys_ck = freq.pll1_p_ck
    else
        freq.sys_ck = 0
    end

    -- SYSCLK frequency
    local SYSCLK_DIV = this:GetFlagValue("__CLK_SYSCLK_DIV__"):gsub("LL_RCC_SYSCLK_DIV_", "")
    freq.sysclk = freq.sys_ck / tonumber(SYSCLK_DIV)
    freq.SYSCLK = freq.sysclk

    -- AHB clk frequency
    local AHBCLK_DIV = this:GetFlagValue("__CLK_AHB_DIV__"):gsub("LL_RCC_AHB_DIV_", "")
    freq.ahbclk = freq.sys_ck / tonumber(AHBCLK_DIV)
    freq.HCLK = freq.ahbclk

    -- APB1 clk frequency
    local APB1CLK_DIV = this:GetFlagValue("__CLK_APB1_DIV__"):gsub("LL_RCC_APB1_DIV_", "")
    freq.apb1clk = freq.ahbclk / tonumber(APB1CLK_DIV)
    freq.PCLK1 = freq.apb1clk

    -- APB2 clk frequency
    local APB2CLK_DIV = this:GetFlagValue("__CLK_APB2_DIV__"):gsub("LL_RCC_APB2_DIV_", "")
    freq.apb2clk = freq.ahbclk / tonumber(APB2CLK_DIV)
    freq.PCLK2 = freq.apb2clk

    -- APB3 clk frequency
    local APB3CLK_DIV = this:GetFlagValue("__CLK_APB3_DIV__"):gsub("LL_RCC_APB3_DIV_", "")
    freq.apb3clk = freq.ahbclk / tonumber(APB3CLK_DIV)
    freq.PCLK3 = freq.apb3clk

    -- APB4 clk frequency
    local APB4CLK_DIV = this:GetFlagValue("__CLK_APB4_DIV__"):gsub("LL_RCC_APB4_DIV_", "")
    freq.apb4clk = freq.ahbclk / tonumber(APB4CLK_DIV)
    freq.PCLK4 = freq.apb4clk

    -- PERIPHERAL CLOCK --------------------------------------------------------
    -- RTC frequency
    local SEL = this:GetFlagValue("__CLK_SRC_RTC__"):gsub("LL_RCC_RTC_CLKSOURCE_", "")
    if SEL == "HSE" then
        local div = this:GetFlagValue("__CLK_RTC_HSE_DIV__"):gsub("LL_RCC_RTC_", "")
        if div == "NOCLOCK" then
           freq.RTC = 0
        else
           div = div:gsub("HSE_DIV_", "")
           freq.RTC = freq[SEL] / tonumber(div)
        end
    else
        freq.RTC = freq[SEL]
    end

    -- UART1..6 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_UART16__"):gsub("LL_RCC_USART16_CLKSOURCE_", "")
    freq.UART16 = freq[SEL]

    -- UART234578 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_UART234578__"):gsub("LL_RCC_USART234578_CLKSOURCE_", "")
    freq.UART234578 = freq[SEL]

    -- LPUART1 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_LPUART1__"):gsub("LL_RCC_LPUART1_CLKSOURCE_", "")
    freq.LPUART1 = freq[SEL]

    -- I2C123 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_I2C123__"):gsub("LL_RCC_I2C123_CLKSOURCE_", "")
    freq.I2C123 = freq[SEL]

    -- I2C4 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_I2C4__"):gsub("LL_RCC_I2C4_CLKSOURCE_", "")
    freq.I2C4 = freq[SEL]

    -- LPTIM1 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_LPTIM1__"):gsub("LL_RCC_LPTIM1_CLKSOURCE_", "")
    freq.LPTIM1 = freq[SEL]

    -- LPTIM2 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_LPTIM2__"):gsub("LL_RCC_LPTIM2_CLKSOURCE_", "")
    freq.LPTIM2 = freq[SEL]

    -- LPTIM345 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_LPTIM345__"):gsub("LL_RCC_LPTIM345_CLKSOURCE_", "")
    freq.LPTIM345 = freq[SEL]

    -- CLKP frequency
    local SEL = this:GetFlagValue("__CLK_SRC_CLKP__"):gsub("LL_RCC_CLKP_CLKSOURCE_", "")
    freq.CLKP = freq[SEL]

    -- SPDIF frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SPDIF__"):gsub("LL_RCC_SPDIF_CLKSOURCE_", "")
    freq.SPDIF = freq[SEL]

    -- SAI1 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI1__"):gsub("LL_RCC_SAI1_CLKSOURCE_", "")
    freq.SAI1 = freq[SEL]

    -- SAI23 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI23__"):gsub("LL_RCC_SAI23_CLKSOURCE_", "")
    freq.SAI23 = freq[SEL]

    -- SAI2A frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI2A__"):gsub("LL_RCC_SAI2A_CLKSOURCE_", "")
    freq.SAI2A = freq[SEL]

    -- SAI2B frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI2B__"):gsub("LL_RCC_SAI2B_CLKSOURCE_", "")
    freq.SAI2B = freq[SEL]

    -- SAI4A frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI4A__"):gsub("LL_RCC_SAI4A_CLKSOURCE_", "")
    freq.SAI4A = freq[SEL]

    -- SAI4B frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SAI4B__"):gsub("LL_RCC_SAI4B_CLKSOURCE_", "")
    freq.SAI4B = freq[SEL]

    -- SDMMC frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SDMMC__"):gsub("LL_RCC_SDMMC_CLKSOURCE_", "")
    freq.SDMMC = freq[SEL]

    -- RNG frequency
    local SEL = this:GetFlagValue("__CLK_SRC_RNG__"):gsub("LL_RCC_RNG_CLKSOURCE_", "")
    freq.RNG = freq[SEL]

    -- USB frequency
    local SEL = this:GetFlagValue("__CLK_SRC_USB__"):gsub("LL_RCC_USB_CLKSOURCE_", "")
    freq.USB = freq[SEL]

    -- CEC frequency
    local SEL = this:GetFlagValue("__CLK_SRC_CEC__"):gsub("LL_RCC_CEC_CLKSOURCE_", "")
    freq.CEC = freq[SEL]

    -- DFSDM1 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_DFSDM1__"):gsub("LL_RCC_DFSDM1_CLKSOURCE_", "")
    freq.DFSDM1 = freq[SEL]

    -- DFSDM2 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_DFSDM2__"):gsub("LL_RCC_DFSDM2_CLKSOURCE_", "")
    freq.DFSDM2 = freq[SEL]

    -- FMC frequency
    local SEL = this:GetFlagValue("__CLK_SRC_FMC__"):gsub("LL_RCC_FMC_CLKSOURCE_", "")
    freq.FMC = freq[SEL]

    -- QSPI frequency
    local SEL = this:GetFlagValue("__CLK_SRC_QSPI__"):gsub("LL_RCC_QSPI_CLKSOURCE_", "")
    freq.QSPI = freq[SEL]

    -- OSPI frequency
    local SEL = this:GetFlagValue("__CLK_SRC_OSPI__"):gsub("LL_RCC_OSPI_CLKSOURCE_", "")
    freq.OSPI = freq[SEL]

    -- DSI frequency
    local SEL = this:GetFlagValue("__CLK_SRC_DSI__"):gsub("LL_RCC_DSI_CLKSOURCE_", "")
    freq.DSI = freq[SEL]

    -- SPI123 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SPI123__"):gsub("LL_RCC_SPI123_CLKSOURCE_", "")
    freq.SPI123 = freq[SEL]

    -- SPI45 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SPI45__"):gsub("LL_RCC_SPI45_CLKSOURCE_", "")
    freq.SPI45 = freq[SEL]

    -- SPI6 frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SPI6__"):gsub("LL_RCC_SPI6_CLKSOURCE_", "")
    freq.SPI6 = freq[SEL]

    -- FDCAN frequency
    local SEL = this:GetFlagValue("__CLK_SRC_FDCAN__"):gsub("LL_RCC_FDCAN_CLKSOURCE_", "")
    freq.FDCAN = freq[SEL]

    -- SWP frequency
    local SEL = this:GetFlagValue("__CLK_SRC_SWP__"):gsub("LL_RCC_SWP_CLKSOURCE_", "")
    freq.SWP = freq[SEL]

    -- ADC frequency
    local SEL = this:GetFlagValue("__CLK_SRC_ADC__"):gsub("LL_RCC_ADC_CLKSOURCE_", "")
    freq.ADC = freq[SEL]

    -- end calculations
    this:SetFlagValue("__CLK_SYSCLK_SRC_STATUS__", "LL_RCC_SYS_CLKSOURCE_STATUS_"..SYSCLKSRC)
    this:SetFlagValue("__CLK_HCLK_FREQ__", tostring(math.floor(freq.HCLK)))
    this:SetFlagValue("__CLK_SYSCLK_FREQ__", tostring(math.floor(freq.sys_ck)))

    this.freq = freq
end

-- trigger frequency calculation after loading all flags
this.CalculateFreq()
++*/

#endif /* _CLK_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
