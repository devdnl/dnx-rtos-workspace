/*==============================================================================
@file    clk_flags.h

@author  Daniel Zorychta

@brief   CLK module configuration flags.

@note    Copyright (C) 2017 Daniel Zorychta <daniel.zorychta@gmail.com>

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
this:AddWidget("Combobox", "LSI oscillator")
this:AddItem("Disable", "DISABLE")
this:AddItem("Enable", "ENABLE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LSI", this.ShowFrequency)
--*/
#define __CLK_LSI_ON__ DISABLE

/*--
this:AddWidget("Combobox", "LSE oscillator")
this:AddItem("Disable", "RCC_LSE_OFF")
this:AddItem("Enable", "RCC_LSE_ON")
this:AddItem("Baypass", "RCC_LSE_Bypass")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("LSE", this.ShowFrequency)
--*/
#define __CLK_LSE_ON__ RCC_LSE_OFF

/*--
this:AddWidget("Combobox", "HSE oscillator")
this:AddItem("Disable", "RCC_HSE_OFF")
this:AddItem("Enable", "RCC_HSE_ON")
this:AddItem("Baypass", "RCC_HSE_Bypass")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("HSE", this.ShowFrequency)
--*/
#define __CLK_HSE_ON__ RCC_HSE_ON

//==============================================================================
// PLL Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "PLLs Clock source")
this:AddItem("HSI",  "RCC_PLLSource_HSI")
this:AddItem("HSE", "RCC_PLLSource_HSE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLSRC", this.ShowFrequency)
--*/
#define __CLK_PLL_SRC__ RCC_PLLSource_HSE

/*--
this:AddWidget("Spinbox", 2, 63, "PLLs Clock divider (M)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLM", this.ShowFrequency)
--*/
#define __CLK_PLL_SRC_DIV_M__ 4

/*--
this:AddWidget("Combobox", "PLL")
this:AddItem("Enable",  "ENABLE")
this:AddItem("Disable", "DISABLE")
this:SetEvent("clicked", function() this.CalculateFreq() end)
--*/
#define __CLK_PLL_ON__ ENABLE

/*--
this:AddWidget("Spinbox", 50, 432, "PLL Clock multiplier (N)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLVCO", this.ShowFrequency)
--*/
#define __CLK_PLL_N__ 168

/*--
this:AddWidget("Combobox", "PLL Main clock divider (P)")
this:AddItem("VCO divided by 2",  "2")
this:AddItem("VCO divided by 4",  "4")
this:AddItem("VCO divided by 6",  "6")
this:AddItem("VCO divided by 8",  "8")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLCLK", this.ShowFrequency)
--*/
#define __CLK_PLL_P__ 2

/*--
this:AddWidget("Spinbox", 2, 15, "PLL USB, SDIO clock divider (Q)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLQ", this.ShowFrequency)
--*/
#define __CLK_PLL_Q__ 7

/*--
this:AddWidget("Spinbox", 2, 7, "PLL I2S, SAI clock divider (R)")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PLLR", this.ShowFrequency)
--*/
#define __CLK_PLL_R__ 7

//==============================================================================
// PLLI2S Configuration
//==============================================================================
/*--
this.is_PLLI2S =  uC.NAME:match("STM32F40") or uC.NAME:match("STM32F41") or uC.NAME:match("STM32F401")
               or uC.NAME:match("STM32F411.E") or uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
               or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439") or uC.NAME:match("STM32F469")
               or uC.NAME:match("STM32F479") or uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F413")
               or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446")
++*/
/*--
if this.is_PLLI2S ~= nil then
    this:AddWidget("Combobox", "PLLI2S")
    this:AddItem("Enable",  "ENABLE")
    this:AddItem("Disable", "DISABLE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_PLLI2S_ON__ DISABLE

/*--
if this.is_PLLI2S ~= nil then
    this:AddWidget("Spinbox", 50, 432, "PLLI2S Clock multiplier (N)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLI2SVCO", this.ShowFrequency)
end
--*/
#define __CLK_PLLI2S_N__ 192

/*--
if this.is_PLLI2S ~= nil then
    this:AddWidget("Combobox", "PLLI2S 48MHz clock divider (P)")
    this:AddItem("VCO divided by 2",  "2")
    this:AddItem("VCO divided by 4",  "4")
    this:AddItem("VCO divided by 6",  "6")
    this:AddItem("VCO divided by 8",  "8")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLI2SP", this.ShowFrequency)
end
--*/
#define __CLK_PLLI2S_P__ 2

/*--
if this.is_PLLI2S ~= nil then
    this:AddWidget("Spinbox", 2, 15, "PLLI2S SAI1 clock divider (Q)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLI2SQ", this.ShowFrequency)
end
--*/
#define __CLK_PLLI2S_Q__ 2

/*--
if this.is_PLLI2S ~= nil then
    this:AddWidget("Spinbox", 2, 7, "PLLI2S I2S clock divider (R)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLI2SR", this.ShowFrequency)
end
--*/
#define __CLK_PLLI2S_R__ 2


//==============================================================================
// PLLSAI Configuration
//==============================================================================
/*--
this.is_PLLSAI =  uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479")
               or uC.NAME:match("STM32F446") or uC.NAME:match("STM32F427")
               or uC.NAME:match("STM32F437") or uC.NAME:match("STM32F429")
               or uC.NAME:match("STM32F439")
++*/
/*--
if this.is_PLLSAI ~= nil then
    this:AddWidget("Combobox", "PLLSAI")
    this:AddItem("Enable",  "ENABLE")
    this:AddItem("Disable", "DISABLE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_PLLSAI_ON__ DISABLE

/*--
if this.is_PLLSAI ~= nil then
    this:AddWidget("Spinbox", 50, 432, "PLLSAI Clock multiplier (N)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLSAIVCO", this.ShowFrequency)
end
--*/
#define __CLK_PLLSAI_N__ 203

/*--
if this.is_PLLSAI ~= nil then
    this:AddWidget("Combobox", "PLLSAI 48MHz clock divider (P)")
    this:AddItem("VCO divided by 2",  "2")
    this:AddItem("VCO divided by 4",  "4")
    this:AddItem("VCO divided by 6",  "6")
    this:AddItem("VCO divided by 8",  "8")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLSAIP", this.ShowFrequency)
end
--*/
#define __CLK_PLLSAI_P__ 8

/*--
if this.is_PLLSAI ~= nil then
    this:AddWidget("Spinbox", 2, 15, "PLLSAI SAI1 clock divider (Q)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLSAIQ", this.ShowFrequency)
end
--*/
#define __CLK_PLLSAI_Q__ 12

/*--
if this.is_PLLSAI ~= nil then
    this:AddWidget("Spinbox", 2, 7, "PLLSAI LTDC clock divider (R)")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("PLLSAIR", this.ShowFrequency)
end
--*/
#define __CLK_PLLSAI_R__ 7

//==============================================================================
// Clock Sources Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "System clock source")
this:AddItem("HSI clock", "RCC_SYSCLKSource_HSI")
this:AddItem("HSE clock", "RCC_SYSCLKSource_HSE")
this:AddItem("PLL clock", "RCC_SYSCLKSource_PLLCLK")
if uC.NAME:match("STM32F446") then
    this:AddItem("PLL P clock (F446)", "RCC_SYSCLKSource_PLLPCLK")
end

if  uC.NAME:match("STM32F412") or uC.NAME:match("STM32F413")
 or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446") then
    this:AddItem("PLL R clock (F412,F413,F423,F446)", "RCC_SYSCLKSource_PLLRCLK")
end

this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("SYSCLK", this.ShowFrequency)
--*/
#define __CLK_SYS_CLK_SRC__ RCC_SYSCLKSource_PLLCLK

/*--
this:AddWidget("Combobox", "RTC Clock source")
this:AddItem("LSE clock", "RCC_RTCCLKSource_LSE")
this:AddItem("LSI clock", "RCC_RTCCLKSource_LSI")
for i = 2, 31 do this:AddItem("HSE clock divided by "..i, "RCC_RTCCLKSource_HSE_Div"..i) end
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("RTCCLK", this.ShowFrequency)
--*/
#define __CLK_RTC_CLK_SRC__ RCC_RTCCLKSource_LSE

/*--
this:AddWidget("Combobox", "MCO1 Clock source")
if uC.NAME:match("STM32F410") then this:AddItem("Disable", "0") end
this:AddItem("HSI", "RCC_MCO1Source_HSI")
this:AddItem("HSE", "RCC_MCO1Source_HSE")
this:AddItem("LSE", "RCC_MCO1Source_LSE")
this:AddItem("PLLCLK", "RCC_MCO1Source_PLLCLK")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("MCO1CLK", this.ShowFrequency)
--*/
#define __CLK_MC01_CLK_SRC__ RCC_MCO1Source_HSE

/*--
this:AddWidget("Combobox", "MCO1 Clock divisor (output)")
for i = 1, 5 do this:AddItem("MCO divided by "..i, "RCC_MCO1Div_"..tostring(i)) end
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("MCO1CLKDIV", this.ShowFrequency)
--*/
#define __CLK_MC01_CLK_DIV__ RCC_MCO1Div_2

/*--
this:AddWidget("Combobox", "MCO2 Clock source")
if uC.NAME:match("STM32F410") then this:AddItem("Disable", "0") end
this:AddItem("SYSCLK", "RCC_MCO2Source_SYSCLK")
this:AddItem("PLLI2SCLK", "RCC_MCO2Source_PLLI2SCLK")
this:AddItem("HSE", "RCC_MCO2Source_HSE")
this:AddItem("PLLCLK", "RCC_MCO2Source_PLLCLK")
if uC.NAME:match("STM32F410") then this:AddItem("I2SCLK", "RCC_MCO2Source_I2SCLK") end
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("MCO2CLK", this.ShowFrequency)
--*/
#define __CLK_MC02_CLK_SRC__ RCC_MCO2Source_HSE

/*--
this:AddWidget("Combobox", "MCO2 Clock divisor (output)")
for i = 1, 5 do this:AddItem("MCO divided by "..i, "RCC_MCO2Div_"..tostring(i)) end
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("MCO2CLKDIV", this.ShowFrequency)
--*/
#define __CLK_MC02_CLK_DIV__ RCC_MCO2Div_1

/*--
if   uC.NAME:match("STM32F412") or uC.NAME:match("STM32F413")
  or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446") then

    this:AddWidget("Combobox", "I2S1 clock source")
    this:AddItem("I2S PLL R clock", "RCC_I2SCLKSource_PLLI2S")
    this:AddItem("External clock", "RCC_I2SCLKSource_Ext")
    this:AddItem("PLL R clock", "RCC_I2SCLKSource_PLL")
    this:AddItem("HSI or HSE clock", "RCC_I2SCLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("I2S1CLK", this.ShowFrequency)
end
--*/
#define __CLK_I2SAPB1_CLK_SRC__ RCC_I2SCLKSource_HSI_HSE

/*--
if   uC.NAME:match("STM32F412") or uC.NAME:match("STM32F413")
  or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446") then

    this:AddWidget("Combobox", "I2S2 clock source")
    this:AddItem("PLL I2S R clock", "RCC_I2SCLKSource_PLLI2S")
    this:AddItem("External clock", "RCC_I2SCLKSource_Ext")
    this:AddItem("PLL R clock", "RCC_I2SCLKSource_PLL")
    this:AddItem("HSI or HSE clock", "RCC_I2SCLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("I2S2CLK", this.ShowFrequency)
end
--*/
#define __CLK_I2SAPB2_CLK_SRC__ RCC_I2SCLKSource_HSI_HSE

/*--
if uC.NAME:match("STM32F410") then
    this:AddWidget("Combobox", "I2S clock source")
    this:AddItem("I2S PLLR", "RCC_I2SAPBCLKSOURCE_PLLR")
    this:AddItem("External clock", "RCC_I2SAPBCLKSOURCE_EXT")
    this:AddItem("PLL clock source", "RCC_I2SAPBCLKSOURCE_PLLSRC")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("I2SCLK", this.ShowFrequency)

elseif uC.NAME:match("STM32F405") or uC.NAME:match("STM32F407") or uC.NAME:match("STM32F415")
    or uC.NAME:match("STM32F417") or uC.NAME:match("STM32F427")
    or uC.NAME:match("STM32F437") or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
    or uC.NAME:match("STM32F401") or uC.NAME:match("STM32F411") or uC.NAME:match("STM32F469")
    or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "I2S clock source")
    this:AddItem("I2S PLL clock", "RCC_I2S2CLKSource_PLLI2S")
    this:AddItem("External clock", "RCC_I2SAPBCLKSOURCE_EXT")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("I2SCLK", this.ShowFrequency)
end
--*/
#define __CLK_I2S_CLK_SRC__ RCC_I2S2CLKSource_PLLI2S

/*--
if uC.NAME:match("STM32F446") then
    this:AddWidget("Combobox", "SAI1 clock source")
    this:AddItem("SAI PLL clock", "RCC_SAICLKSource_PLLSAI")
    this:AddItem("I2S PLL clock", "RCC_SAICLKSource_PLLI2S")
    this:AddItem("PLL R clock", "RCC_SAICLKSource_PLL")
    this:AddItem("PLL clock source", "RCC_SAICLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAI1CLK", this.ShowFrequency)
end
--*/
#define __CLK_SAI1_CLK_SRC__ RCC_SAICLKSource_HSI_HSE

/*--
if uC.NAME:match("STM32F446") then
    this:AddWidget("Combobox", "SAI2 clock source")
    this:AddItem("SAI PLL clock", "RCC_SAICLKSource_PLLSAI")
    this:AddItem("I2S PLL clock", "RCC_SAICLKSource_PLLI2S")
    this:AddItem("PLL R clock", "RCC_SAICLKSource_PLL")
    this:AddItem("PLL clock source", "RCC_SAICLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAI2CLK", this.ShowFrequency)
end
--*/
#define __CLK_SAI2_CLK_SRC__ RCC_SAICLKSource_HSI_HSE

/*--
if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
  or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
  or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479")
  or uC.NAME:match("STM32F446") then

    this:AddWidget("Spinbox", 1, 32, "SAI PLLSAI clock divider")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_SAI_PLLSAI_CLK_DIV__ 3

/*--
if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
  or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
  or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479")
  or uC.NAME:match("STM32F446") then

    this:AddWidget("Spinbox", 1, 32, "SAI PLLI2S clock divider")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_SAI_PLLI2S_CLK_DIV__ 1

/*--
if uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then
    this:AddWidget("Spinbox", 1, 32, "SAI PLLI2S clock divider")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_SAI_PLLI2S_CLK_DIVR__ 1

/*--
if uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then
    this:AddWidget("Spinbox", 1, 32, "SAI PLL clock divider")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
end
--*/
#define __CLK_SAI_PLL_CLK_DIVR__ 1

/*--
if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
  or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
  or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "SAI Block A clock source")
    this:AddItem("I2S PLL clock", "RCC_SAIACLKSource_PLLI2S")
    this:AddItem("SAI PLL clock", "RCC_SAIACLKSource_PLLSAI")
    this:AddItem("External clock", "RCC_SAIACLKSource_Ext")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAIBLKACLK", this.ShowFrequency)

elseif uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then

    this:AddWidget("Combobox", "SAI Block A clock source")
    this:AddItem("I2S PLL R clock", "RCC_SAIACLKSource_PLLI2SR")
    this:AddItem("I2S PLL Q clock", "RCC_SAIACLKSource_PLLI2S")
    this:AddItem("PLL clock", "RCC_SAIACLKSource_PLL")
    this:AddItem("HSI or HSE clock", "RCC_SAIACLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAIBLKACLK", this.ShowFrequency)
end
--*/
#define __CLK_SAI_BLOCK_A_CLK_SRC__ RCC_SAIACLKSource_PLLSAI

/*--
if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
  or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
  or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "SAI Block B clock source")
    this:AddItem("I2S PLL clock", "RCC_SAIBCLKSource_PLLI2S")
    this:AddItem("SAI PLL clock", "RCC_SAIBCLKSource_PLLSAI")
    this:AddItem("External clock", "RCC_SAIBCLKSource_Ext")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAIBLKBCLK", this.ShowFrequency)

elseif uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then

    this:AddWidget("Combobox", "SAI Block B clock source")
    this:AddItem("I2S PLL R clock", "RCC_SAIBCLKSource_PLLI2SR")
    this:AddItem("I2S PLL Q clock", "RCC_SAIBCLKSource_PLLI2S")
    this:AddItem("PLL clock", "RCC_SAIBCLKSource_PLL")
    this:AddItem("HSI or HSE clock", "RCC_SAIBACLKSource_HSI_HSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SAIBLKBCLK", this.ShowFrequency)
end
--*/
#define __CLK_SAI_BLOCK_B_CLK_SRC__ RCC_SAIBCLKSource_PLLSAI

/*--
if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
  or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
  or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "LTDC clock divider")
    this:AddItem("PLLSAI R / 2",  "RCC_PLLSAIDivR_Div2")
    this:AddItem("PLLSAI R / 4",  "RCC_PLLSAIDivR_Div4")
    this:AddItem("PLLSAI R / 8",  "RCC_PLLSAIDivR_Div8")
    this:AddItem("PLLSAI R / 16", "RCC_PLLSAIDivR_Div16")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("LTDCCLK", this.ShowFrequency)
end
--*/
#define __CLK_LTDC_DIV__ RCC_PLLSAIDivR_Div2

/*--
if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F413")
  or uC.NAME:match("STM32F423") then

    this:AddWidget("Combobox", "DFSDM1 clock source")
    this:AddItem("APB1 clock",  "RCC_DFSDMCLKSource_APB")
    this:AddItem("SYSCLK",  "RCC_DFSDMCLKSource_SYS")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("DFSDM1CLK", this.ShowFrequency)
end
--*/
#define __CLK_DFSDM1_CLK_SRC__ RCC_DFSDMCLKSource_APB

/*--
if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F413")
  or uC.NAME:match("STM32F423") then

    this:AddWidget("Combobox", "DFSDM1 audio clock source")
    this:AddItem("APB1 clock",  "RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB1")
    this:AddItem("APB2 clock",  "RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB2")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("DFSDM1ACLK", this.ShowFrequency)
end
--*/
#define __CLK_DFSDM1_ACLK_SRC__ RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB1

/*--
if uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then

    this:AddWidget("Combobox", "DFSDM2 audio clock source")
    this:AddItem("APB1 clock",  "RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB1")
    this:AddItem("APB2 clock",  "RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB2")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("DFSDM2ACLK", this.ShowFrequency)
end
--*/
#define __CLK_DFSDM2_ACLK_SRC__ RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB1

/*--
if uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "DSI clock source")
    this:AddItem("PHY clock",  "RCC_DSICLKSource_PHY")
    this:AddItem("PLLR clock", "RCC_DSICLKSource_PLLR")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("DSICLK", this.ShowFrequency)
end
--*/
#define __CLK_DSI_SRC__ RCC_DSICLKSource_PHY

/*--
if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
  or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
  or uC.NAME:match("STM32F469")   or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "48MHz clock source")
    this:AddItem("PLL", "RCC_48MHZCLKSource_PLL")
    this:AddItem("PLLSAI", "RCC_48MHZCLKSource_PLLSAI")
    this:AddItem("PLLI2SQ", "RCC_CK48CLKSOURCE_PLLI2SQ")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("CLK48MHZ", this.ShowFrequency)
end
--*/
#define __CLK_48MHZ_SRC__ RCC_48MHZCLKSource_PLL

/*--
if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
  or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
  or uC.NAME:match("STM32F469")   or uC.NAME:match("STM32F479") then

    this:AddWidget("Combobox", "SDIO clock source")
    this:AddItem("PLL48CLK", "RCC_SDIOCLKSource_48MHZ")
    this:AddItem("SYSCLK", "RCC_SDIOCLKSource_SYSCLK")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SDIOCLK", this.ShowFrequency)
end
--*/
#define __CLK_SDIO_SRC__ RCC_SDIOCLKSource_48MHZ

/*--
if uC.NAME:match("STM32F446") then

    this:AddWidget("Combobox", "SPDIFRX clock source")
    this:AddItem("PLL R", "RCC_SPDIFRXCLKSource_PLLR")
    this:AddItem("PLLI2S P", "RCC_SPDIFRXCLKSource_PLLI2SP")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("SPDIF", this.ShowFrequency)
end
--*/
#define __CLK_SPDIF_SRC__ RCC_SPDIFRXCLKSource_PLLR

/*--
if uC.NAME:match("STM32F446") then

    this:AddWidget("Combobox", "CEC clock source")
    this:AddItem("HSI / 488", "RCC_CECCLKSource_HSIDiv488")
    this:AddItem("LSE", "RCC_CECCLKSource_LSE")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("CEC", this.ShowFrequency)
end
--*/
#define __CLK_CEC_SRC__ RCC_CECCLKSource_HSIDiv488

/*--
if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
  or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
  or uC.NAME:match("STM32F410") then

    this:AddWidget("Combobox", "FMPI2C1 clock source")
    this:AddItem("APB1 clock", "RCC_FMPI2C1CLKSource_APB1")
    this:AddItem("SYSCLK", "RCC_FMPI2C1CLKSource_SYSCLK")
    this:AddItem("HSI", "RCC_FMPI2C1CLKSource_HSI")
    this:SetEvent("clicked", function() this.CalculateFreq() end)
    this:SetValueFormatter("FMPI2C1", this.ShowFrequency)
end
--*/
#define __CLK_FMPI2C1_SRC__ RCC_FMPI2C1CLKSource_APB1


//==============================================================================
// System Clocks Prescallers Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "AHB prescaler")
this:AddItem("SYSCLK / 1",  "RCC_SYSCLK_Div1")
this:AddItem("SYSCLK / 2",  "RCC_SYSCLK_Div2")
this:AddItem("SYSCLK / 4",  "RCC_SYSCLK_Div4")
this:AddItem("SYSCLK / 8",  "RCC_SYSCLK_Div8")
this:AddItem("SYSCLK / 16",  "RCC_SYSCLK_Div16")
this:AddItem("SYSCLK / 64",  "RCC_SYSCLK_Div64")
this:AddItem("SYSCLK / 128",  "RCC_SYSCLK_Div128")
this:AddItem("SYSCLK / 256",  "RCC_SYSCLK_Div256")
this:AddItem("SYSCLK / 512",  "RCC_SYSCLK_Div512")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("HCLK", this.ShowFrequency)
--*/
#define __CLK_AHB_PRE__ RCC_SYSCLK_Div1

/*--
this:AddWidget("Combobox", "APB1 prescaler")
this:AddItem("HCLK / 1",  "RCC_HCLK_Div1")
this:AddItem("HCLK / 2",  "RCC_HCLK_Div2")
this:AddItem("HCLK / 4",  "RCC_HCLK_Div4")
this:AddItem("HCLK / 8",  "RCC_HCLK_Div8")
this:AddItem("HCLK / 16",  "RCC_HCLK_Div16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PCLK1", this.ShowFrequency)
--*/
#define __CLK_APB1_PRE__ RCC_HCLK_Div2

/*--
this:AddWidget("Combobox", "APB2 prescaler")
this:AddItem("HCLK / 1",  "RCC_HCLK_Div1")
this:AddItem("HCLK / 2",  "RCC_HCLK_Div2")
this:AddItem("HCLK / 4",  "RCC_HCLK_Div4")
this:AddItem("HCLK / 8",  "RCC_HCLK_Div8")
this:AddItem("HCLK / 16",  "RCC_HCLK_Div16")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetValueFormatter("PCLK2", this.ShowFrequency)
--*/
#define __CLK_APB2_PRE__ RCC_HCLK_Div2


//==============================================================================
// Flash Latency Configuration
//==============================================================================
/*--
this:AddWidget("Combobox", "CPU voltage")
this:AddItem("2.7-3.6V",  "3p6V")
this:AddItem("2.4-2.7V",  "2p7V")
this:AddItem("2.1-2.4V",  "2p4V")
this:AddItem("1.8-2.1V",  "2p1V")
this:SetEvent("clicked", function() this.CalculateFreq() end)
this:SetToolTip("Select power voltage level to calculate FLASH latency.")
this:SetValueFormatter("FLASHLATENCY", this.ShowFrequency)
--*/
#define __CLK_CPU_VOLTAGE__ 3p6V

/*--
-- Value calculated automatically
this:AddWidget("Value")
--*/
#define __CLK_FLASH_LATENCY__ 5

/*--
-- Value calculated automatically
this:AddWidget("Value")
--*/
#define __CLK_HCLK_FREQ__ 168000000

//==============================================================================
// Calculation script
//==============================================================================
/*--
this.CalculateFreq = function(self)
    local PLLON         = iff(this:GetFlagValue("__CLK_PLL_ON__") == "ENABLE", 1, 0)
    local PLLSRC        = this:GetFlagValue("__CLK_PLL_SRC__")
    local PLLM          = this:GetFlagValue("__CLK_PLL_SRC_DIV_M__")
    local PLLN          = this:GetFlagValue("__CLK_PLL_N__")
    local PLLP          = this:GetFlagValue("__CLK_PLL_P__")
    local PLLQ          = this:GetFlagValue("__CLK_PLL_Q__")
    local PLLR          = this:GetFlagValue("__CLK_PLL_R__")

    local PLLI2SON      = iff(this:GetFlagValue("__CLK_PLLI2S_ON__") == "ENABLE", 1, 0)
    local PLLI2SM       = this:GetFlagValue("__CLK_PLL_SRC_DIV_M__")
    local PLLI2SN       = this:GetFlagValue("__CLK_PLLI2S_N__")
    local PLLI2SP       = this:GetFlagValue("__CLK_PLLI2S_P__")
    local PLLI2SQ       = this:GetFlagValue("__CLK_PLLI2S_Q__")
    local PLLI2SR       = this:GetFlagValue("__CLK_PLLI2S_R__")

    local PLLSAION      = iff(this:GetFlagValue("__CLK_PLLSAI_ON__") == "ENABLE", 1, 0)
    local PLLSAIM       = this:GetFlagValue("__CLK_PLL_SRC_DIV_M__")
    local PLLSAIN       = this:GetFlagValue("__CLK_PLLSAI_N__")
    local PLLSAIP       = this:GetFlagValue("__CLK_PLLSAI_P__")
    local PLLSAIQ       = this:GetFlagValue("__CLK_PLLSAI_Q__")
    local PLLSAIR       = this:GetFlagValue("__CLK_PLLSAI_R__")

    local HSEON         = iff(this:GetFlagValue("__CLK_HSE_ON__") == "RCC_HSE_OFF", 0, 1)
    local LSEON         = iff(this:GetFlagValue("__CLK_LSE_ON__") == "RCC_LSE_OFF", 0, 1)
    local LSION         = iff(this:GetFlagValue("__CLK_LSI_ON__") == "ENABLE", 1, 0)

    local SYSCLKSRC     = this:GetFlagValue("__CLK_SYS_CLK_SRC__")
    local RTCSRC        = this:GetFlagValue("__CLK_RTC_CLK_SRC__")

    local AHBPRE        = this:GetFlagValue("__CLK_AHB_PRE__")
    local APB1PRE       = this:GetFlagValue("__CLK_APB1_PRE__")
    local APB2PRE       = this:GetFlagValue("__CLK_APB2_PRE__")

    local MCO1SRC       = this:GetFlagValue("__CLK_MC01_CLK_SRC__")
    local MCO1DIV       = this:GetFlagValue("__CLK_MC01_CLK_DIV__"):gsub("RCC_MCO1Div_", "")
    local MCO2SRC       = this:GetFlagValue("__CLK_MC02_CLK_SRC__")
    local MCO2DIV       = this:GetFlagValue("__CLK_MC02_CLK_DIV__"):gsub("RCC_MCO2Div_", "")
    local CPUVOLTAGE    = this:GetFlagValue("__CLK_CPU_VOLTAGE__")

    local I2S1SRC       = this:GetFlagValue("__CLK_I2SAPB1_CLK_SRC__")
    local I2S2SRC       = this:GetFlagValue("__CLK_I2SAPB2_CLK_SRC__")
    local I2SSRC        = this:GetFlagValue("__CLK_I2S_CLK_SRC__")

    local SAI1SRC       = this:GetFlagValue("__CLK_SAI1_CLK_SRC__")
    local SAI2SRC       = this:GetFlagValue("__CLK_SAI2_CLK_SRC__")

    local SAIPLLSAIDIV  = this:GetFlagValue("__CLK_SAI_PLLSAI_CLK_DIV__")
    local SAIPLLI2SDIV  = this:GetFlagValue("__CLK_SAI_PLLI2S_CLK_DIV__")
    local SAIPLLI2SDIVR = this:GetFlagValue("__CLK_SAI_PLLI2S_CLK_DIVR__")
    local SAIPLLDIVR    = this:GetFlagValue("__CLK_SAI_PLL_CLK_DIVR__")
    local SAIBLKASRC    = this:GetFlagValue("__CLK_SAI_BLOCK_A_CLK_SRC__")
    local SAIBLKBSRC    = this:GetFlagValue("__CLK_SAI_BLOCK_B_CLK_SRC__")

    local LTDCDIV       = this:GetFlagValue("__CLK_LTDC_DIV__"):gsub("RCC_PLLSAIDivR_Div", "")
    local DSISRC        = this:GetFlagValue("__CLK_DSI_SRC__")
    local CLK48MHZSRC   = this:GetFlagValue("__CLK_48MHZ_SRC__")
    local SDIOCLKSRC    = this:GetFlagValue("__CLK_SDIO_SRC__")
    local SPIDFSRC      = this:GetFlagValue("__CLK_SPDIF_SRC__")
    local CECSRC        = this:GetFlagValue("__CLK_CEC_SRC__")
    local FMPI2C1SRC    = this:GetFlagValue("__CLK_FMPI2C1_SRC__")

    local DFSDM1CLKSRC  = this:GetFlagValue("__CLK_DFSDM1_CLK_SRC__")
    local DFSDM1ACLKSRC = this:GetFlagValue("__CLK_DFSDM1_ACLK_SRC__")
    local DFSDM2ACLKSRC = this:GetFlagValue("__CLK_DFSDM2_ACLK_SRC__")

    local freq          = {}
    freq.HSE            = uC.OSCFREQ * HSEON
    freq.HSI            = 16e6
    freq.LSI            = 32e3 * LSION
    freq.LSE            = 32768 * LSEON
    freq.I2SCLKIN       = 2.048e6
    freq.DSIPHYCLK      = 62.5e6
    freq.RTCCLK         = 0
    freq.SYSCLK         = 0
    freq.PLLCLK         = 0
    freq.PLLVCO         = 0
    freq.PLL48CLK       = 0
    freq.PLLQ           = 0
    freq.PLLR           = 0
    freq.PLLM           = 0
    freq.PLLI2SM        = 0
    freq.PLLI2SVCO      = 0
    freq.PLLI2SP        = 0
    freq.PLLI2SQ        = 0
    freq.PLLI2SR        = 0
    freq.PLLSAIM        = 0
    freq.PLLSAIVCO      = 0
    freq.PLLSAIP        = 0
    freq.PLLSAIQ        = 0
    freq.PLLSAIR        = 0
    freq.HCLK           = 0
    freq.PCLK1          = 0
    freq.PCLK2          = 0
    freq.TIMCLK1        = 0
    freq.TIMCLK2        = 0
    freq.MCO1CLK        = 0
    freq.MCO1CLKDIV     = 0
    freq.MCO2CLK        = 0
    freq.MCO2CLKDIV     = 0
    freq.I2S1CLK        = 0
    freq.I2S2CLK        = 0
    freq.I2SCLK         = 0
    freq.SAI1CLK        = 0
    freq.SAI2CLK        = 0
    freq.SAIBLKACLK     = 0
    freq.SAIBLKBCLK     = 0
    freq.LTDCCLK        = 0
    freq.DFSDM1CLK      = 0
    freq.DFSDM1ACLK     = 0
    freq.DFSDM2ACLK     = 0
    freq.DSICLK         = 0
    freq.CLK48MHZ       = 0
    freq.SDIOCLK        = 0
    freq.SPDIF          = 0
    freq.CEC            = 0
    freq.FMPI2C1        = 0
    freq.FLASHLATENCY   = 0

    -- calculate RTCCLK --------------------------------------------------------
    if RTCSRC == "RCC_RTCCLKSource_LSI" then
        freq.RTCCLK = freq.LSI
    elseif RTCSRC == "RCC_RTCCLKSource_LSE" then
        freq.RTCCLK = freq.LSE
    else
        freq.RTCCLK = freq.HSE / RTCSRC:gsub("RCC_RTCCLKSource_HSE_Div", "")
    end

    -- calculate PLL Clk frequency ---------------------------------------------
    if PLLSRC == "RCC_PLLSource_HSI" then
        freq.PLLSRC = freq.HSI
        freq.PLLM = freq.HSI / PLLM
    else
        freq.PLLSRC = freq.HSE
        freq.PLLM = freq.HSE / PLLM
    end

    if PLLON == 1 then
        freq.PLLVCO = freq.PLLM * PLLN
        freq.PLLCLK = freq.PLLVCO / PLLP
        freq.PLLQ = freq.PLLVCO / PLLQ
        freq.PLL48CLK = freq.PLLQ
        freq.PLLR = freq.PLLVCO / PLLR

        if freq.PLLVCO < 100e6 or freq.PLLVCO > 432e6 then
           freq.PLLVCO = PrintFrequency(freq.PLLVCO)..", out of 100..432 MHz"
        end

        if freq.PLLM < 1e6 or freq.PLLM > 2e6 then
           freq.PLLM = PrintFrequency(freq.PLLM)..", out of 1..2 MHz"
        end
    end

    -- calculate PLLI2S Clk frequency ------------------------------------------
    if this.is_PLLI2S ~= nil then
        if PLLI2SON == 1 then
            if PLLSRC == "RCC_PLLSource_HSI" then
               freq.PLLI2SM = freq.HSI / PLLI2SM
            else
               freq.PLLI2SM = freq.HSE / PLLI2SM
            end

            freq.PLLI2SVCO = freq.PLLI2SM * PLLI2SN
            freq.PLLI2SP = freq.PLLI2SVCO / PLLI2SP
            freq.PLLI2SQ = freq.PLLI2SVCO / PLLI2SQ
            freq.PLLI2SR = freq.PLLI2SVCO / PLLI2SR

            if freq.PLLI2SVCO < 100e6 or freq.PLLI2SVCO > 432e6 then
               freq.PLLI2SVCO = PrintFrequency(freq.PLLI2SVCO)..", out of 100..432MHz"
            end

            if freq.PLLI2SR > 192e6 then
                freq.PLLI2SR = PrintFrequency(freq.PLLI2SR)..", max is 192 MHz!"
            end
        end
    end

    -- calculate PLLSAI Clk frequency ------------------------------------------
    if this.is_PLLSAI ~= nil then
        if PLLSAION == 1 then
            if PLLSRC == "RCC_PLLSource_HSI" then
               freq.PLLSAIM = freq.HSI / PLLSAIM
            else
               freq.PLLSAIM = freq.HSE / PLLSAIM
            end

            freq.PLLSAIVCO = freq.PLLSAIM * PLLSAIN
            freq.PLLSAIP = freq.PLLSAIVCO / PLLSAIP
            freq.PLLSAIQ = freq.PLLSAIVCO / PLLSAIQ
            freq.PLLSAIR = freq.PLLSAIVCO / PLLSAIR

            if freq.PLLSAIVCO < 100e6 or freq.PLLSAIVCO > 432e6 then
               freq.PLLSAIVCO = PrintFrequency(freq.PLLSAIVCO)..", out of 100..432 MHz"
            end
        end
    end

    -- calculate SYSCLK --------------------------------------------------------
    if SYSCLKSRC == "RCC_SYSCLKSource_HSE" then
        freq.SYSCLK = freq.HSE
    elseif SYSCLKSRC == "RCC_SYSCLKSource_HSI" then
        freq.SYSCLK = freq.HSI
    else
        freq.SYSCLK = freq.PLLCLK
    end

    -- calculate MCO1 frequency ------------------------------------------------
    if MCO1SRC == "RCC_MCO1Source_HSI" then
        freq.MCO1CLK = freq.HSI
    elseif MCO1SRC == "RCC_MCO1Source_HSE" then
        freq.MCO1CLK = freq.HSE
    elseif MCO1SRC == "RCC_MCO1Source_LSE" then
        freq.MCO1CLK = freq.LSE
    else
        freq.MCO1CLK = freq.PLLCLK
    end
    freq.MCO1CLKDIV = freq.MCO1CLK / MCO1DIV

    -- calculate MCO2 frequency ------------------------------------------------
    if MCO2SRC == "RCC_MCO2Source_SYSCLK" then
        freq.MCO2CLK = freq.SYSCLK
    elseif MCO2SRC == "RCC_MCO2Source_PLLI2SCLK" then
        freq.MCO2CLK = freq.PLLI2SR
    elseif MCO2SRC == "RCC_MCO2Source_I2SCLK" then
        freq.MCO2CLK = freq.PLLI2SR
    elseif MCO2SRC == "RCC_MCO2Source_HSE" then
        freq.MCO2CLK = freq.HSE
    elseif MCO2SRC == "RCC_MCO2Source_PLLCLK" then
        freq.MCO2CLK = freq.PLLCLK
    else
        freq.MCO2CLK = freq.SYSCLK
    end
    freq.MCO2CLKDIV = freq.MCO2CLK / MCO2DIV

    -- calculate I2S1 clock frequency ------------------------------------------
    if   uC.NAME:match("STM32F412") or uC.NAME:match("STM32F413")
      or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446") then

        if I2S1SRC == "RCC_I2SCLKSource_PLLI2S" then
            freq.I2S1CLK = freq.PLLI2SR
        elseif I2S1SRC == "RCC_I2SCLKSource_Ext" then
            freq.I2S1CLK = freq.I2SCLKIN
        elseif I2S1SRC == "RCC_I2SCLKSource_PLL" then
            freq.I2S1CLK = freq.PLLR
        else -- RCC_I2SCLKSource_HSI_HSE
            if PLLSRC == "RCC_PLLSource_HSI" then
                freq.I2S1CLK = freq.HSI
            else
                freq.I2S1CLK = freq.HSE
            end
        end
    end

    -- calculate I2S2 clock frequency ------------------------------------------
    if   uC.NAME:match("STM32F412") or uC.NAME:match("STM32F413")
      or uC.NAME:match("STM32F423") or uC.NAME:match("STM32F446") then

        if I2S2SRC == "RCC_I2SCLKSource_PLLI2S" then
            freq.I2S2CLK = freq.PLLI2SR
        elseif I2S2SRC == "RCC_I2SCLKSource_Ext" then
            freq.I2S2CLK = freq.I2SCLKIN
        elseif I2S2SRC == "RCC_I2SCLKSource_PLL" then
            freq.I2S2CLK = freq.PLLR
        else -- RCC_I2SCLKSource_HSI_HSE
            if PLLSRC == "RCC_PLLSource_HSI" then
                freq.I2S2CLK = freq.HSI
            else
                freq.I2S2CLK = freq.HSE
            end
        end
    end

    -- calculate I2S clock frequency -------------------------------------------
    if uC.NAME:match("STM32F410") then
        if I2SSRC == "RCC_I2SAPBCLKSOURCE_PLLR" then
            freq.I2SCLK = freq.PLLI2SR
        elseif I2SSRC == "RCC_I2SAPBCLKSOURCE_EXT" then
            freq.I2SCLK = freq.I2SCLKIN
        elseif I2SSRC == "RCC_I2SAPBCLKSOURCE_PLLSRC" then
            if PLLSRC == "RCC_PLLSource_HSI" then
                freq.I2SCLK = freq.HSI
            else
                freq.I2SCLK = freq.HSE
            end
        else
            freq.I2SCLK = 0
        end

    elseif uC.NAME:match("STM32F405") or uC.NAME:match("STM32F407") or uC.NAME:match("STM32F415")
        or uC.NAME:match("STM32F417") or uC.NAME:match("STM32F427")
        or uC.NAME:match("STM32F437") or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
        or uC.NAME:match("STM32F401") or uC.NAME:match("STM32F411") or uC.NAME:match("STM32F469")
        or uC.NAME:match("STM32F479") then

        if I2SSRC == "RCC_I2S2CLKSource_PLLI2S" then
            freq.I2SCLK = freq.PLLI2SR
        elseif I2SSRC == "RCC_I2SAPBCLKSOURCE_EXT" then
            freq.I2SCLK = freq.I2SCLKIN
        else
            freq.I2SCLK = 0
        end
    end

    -- calculate SAI1 clock frequency ------------------------------------------
    if uC.NAME:match("STM32F446") then
        if SAI1SRC == "RCC_SAICLKSource_PLLSAI" then
            freq.SAI1CLK = freq.PLLSAIQ
        elseif SAI1SRC == "RCC_SAICLKSource_PLLI2S" then
            freq.SAI1CLK = freq.PLLI2SQ
        elseif SAI1SRC == "RCC_SAICLKSource_PLL" then
            freq.SAI1CLK = freq.PLLR
        elseif SAI1SRC == "RCC_SAICLKSource_HSI_HSE" then
            if PLLSRC == "RCC_PLLSource_HSI" then
                freq.SAI1CLK = freq.HSI
            else
                freq.SAI1CLK = freq.HSE
            end
        end
    end

    -- calculate SAI2 clock frequency ------------------------------------------
    if uC.NAME:match("STM32F446") then
        if SAI2SRC == "RCC_SAICLKSource_PLLSAI" then
            freq.SAI2CLK = freq.PLLSAIQ
        elseif SAI2SRC == "RCC_SAICLKSource_PLLI2S" then
            freq.SAI2CLK = freq.PLLI2SQ
        elseif SAI2SRC == "RCC_SAICLKSource_PLL" then
            freq.SAI2CLK = freq.PLLR
        elseif SAI2SRC == "RCC_SAICLKSource_HSI_HSE" then
            if PLLSRC == "RCC_PLLSource_HSI" then
                freq.SAI2CLK = freq.HSI
            else
                freq.SAI2CLK = freq.HSE
            end
        end
    end

    -- calculate SAI Block A clock frequency -----------------------------------
    if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
      or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
      or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

        if SAIBLKASRC == "RCC_SAIACLKSource_PLLI2S" then
            freq.SAIBLKACLK = freq.PLLI2SQ / SAIPLLI2SDIV
        elseif SAIBLKASRC == "RCC_SAIACLKSource_PLLSAI" then
            freq.SAIBLKACLK = freq.PLLSAIQ / SAIPLLSAIDIV
        elseif SAIBLKASRC == "RCC_SAIACLKSource_Ext" then
            freq.SAIBLKACLK = freq.I2SCLKIN
        else
            freq.SAIBLKACLK = 0
        end

    elseif uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then
        if SAIBLKASRC == "RCC_SAIACLKSource_PLLI2SR" then
            freq.SAIBLKACLK = freq.PLLI2SR / SAIPLLI2SDIVR
        elseif SAIBLKASRC == "RCC_SAIACLKSource_PLLI2S" then
            freq.SAIBLKACLK = freq.PLLI2SR
        elseif SAIBLKASRC == "RCC_SAIACLKSource_PLL" then
            freq.SAIBLKACLK = freq.PLLR / SAIPLLDIVR
        elseif SAIBLKASRC == "RCC_SAIACLKSource_HSI_HSE" then
            freq.SAIBLKACLK = iff(PLLSRC == "RCC_PLLSource_HSI", freq.HSI, freq.HSE)
        else
            freq.SAIBLKACLK = 0
        end
    end

    -- calculate SAI Block B clock frequency -----------------------------------
    if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
      or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
      or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

        if SAIBLKBSRC == "RCC_SAIBCLKSource_PLLI2S" then
            freq.SAIBLKBCLK = freq.PLLI2SQ / SAIPLLI2SDIV
        elseif SAIBLKBSRC == "RCC_SAIBCLKSource_PLLSAI" then
            freq.SAIBLKBCLK = freq.PLLSAIQ / SAIPLLSAIDIV
        elseif SAIBLKBSRC == "RCC_SAIBCLKSource_Ext" then
            freq.SAIBLKBCLK = freq.I2SCLKIN
        else
            freq.SAIBLKBCLK = 0
        end

    elseif uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then
        if SAIBLKBSRC == "RCC_SAIBCLKSource_PLLI2SR" then
            freq.SAIBLKBCLK = freq.PLLI2SR / SAIPLLI2SDIVR
        elseif SAIBLKBSRC == "RCC_SAIBCLKSource_PLLI2S" then
            freq.SAIBLKBCLK = freq.PLLI2SR
        elseif SAIBLKBSRC == "RCC_SAIBCLKSource_PLL" then
            freq.SAIBLKBCLK = freq.PLLR / SAIPLLDIVR
        elseif SAIBLKBSRC == "RCC_SAIBCLKSource_HSI_HSE" then
            freq.SAIBLKBCLK = iff(PLLSRC == "RCC_PLLSource_HSI", freq.HSI, freq.HSE)
        else
            freq.SAIBLKBCLK = 0
        end
    end

    -- calculate LTDC clock frequency ------------------------------------------
    if   uC.NAME:match("STM32F427") or uC.NAME:match("STM32F437")
      or uC.NAME:match("STM32F429") or uC.NAME:match("STM32F439")
      or uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

        freq.LTDCCLK = freq.PLLSAIR / LTDCDIV
    end

    -- calculate output clocks -------------------------------------------------
    freq.HCLK     = freq.SYSCLK / AHBPRE:gsub("RCC_SYSCLK_Div", "")
    freq.PCLK1    = freq.HCLK / APB1PRE:gsub("RCC_HCLK_Div", "")
    freq.PCLK2    = freq.HCLK / APB2PRE:gsub("RCC_HCLK_Div", "")
    freq.TIMxCLK1 = iff(APB1PRE == "RCC_HCLK_Div1", freq.PCLK1, freq.PCLK1 * 2)
    freq.TIMxCLK2 = iff(APB2PRE == "RCC_HCLK_Div1", freq.PCLK1, freq.PCLK1 * 2)

    -- calculate DFSDM1/2 clocks frequency -------------------------------------
    if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F413")
      or uC.NAME:match("STM32F423") then

        if DFSDM1CLKSRC == "RCC_DFSDMCLKSource_APB" then
            freq.DFSDM1CLK = freq.PCLK1
        elseif DFSDM1CLKSRC == "RCC_DFSDMCLKSource_SYS" then
            freq.DFSDM1CLK = freq.SYSCLK
        else
            freq.DFSDM1CLK = 0
        end

        if DFSDM1ACLKSRC == "RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB1" then
            freq.DFSDM1ACLK = freq.PCLK1
        elseif DFSDM1ACLKSRC == "RCC_DFSDM1AUDIOCLKSOURCE_I2SAPB2" then
            freq.DFSDM1ACLK = freq.PCLK2
        else
            freq.DFSDM1ACLK = 0
        end
    end

    if uC.NAME:match("STM32F413") or uC.NAME:match("STM32F423") then
        if DFSDM2ACLKSRC == "RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB1" then
            freq.DFSDM2ACLK = freq.PCLK1
        elseif DFSDM2ACLKSRC == "RCC_DFSDM2AUDIOCLKSOURCE_I2SAPB2" then
            freq.DFSDM2ACLK = freq.PCLK2
        else
            freq.DFSDM2ACLK = 0
        end
    end

    -- calculate DSI clock -----------------------------------------------------
    if uC.NAME:match("STM32F469") or uC.NAME:match("STM32F479") then

        if DSISRC == "RCC_DSICLKSource_PHY" then
            freq.DSICLK = freq.DSIPHYCLK
        elseif DSISRC == "RCC_DSICLKSource_PLLR" then
            freq.DSICLK = freq.PLLR
        else
            freq.DSICLK = 0
        end
    end

    -- calculate 48MHz clock frequency -----------------------------------------
    if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
      or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
      or uC.NAME:match("STM32F469")   or uC.NAME:match("STM32F479") then

        if CLK48MHZSRC == "RCC_48MHZCLKSource_PLL" then
            freq.CLK48MHZ = freq.PLLQ
        elseif CLK48MHZSRC == "RCC_48MHZCLKSource_PLLSAI" then
            freq.CLK48MHZ = freq.PLLSAIP
        elseif CLK48MHZSRC == "RCC_CK48CLKSOURCE_PLLI2SQ" then
            freq.CLK48MHZ = freq.PLLI2SQ
        else
            freq.CLK48MHZ = 0
        end
    end

    -- calculate SDIO clock frequency ------------------------------------------
    if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
      or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
      or uC.NAME:match("STM32F469")   or uC.NAME:match("STM32F479") then

        if SDIOCLKSRC == "RCC_SDIOCLKSource_48MHZ" then
            freq.SDIOCLK = freq.CLK48MHZ
        elseif SDIOCLKSRC == "RCC_SDIOCLKSource_SYSCLK" then
            freq.SDIOCLK = freq.SYSCLK
        else
            freq.SDIOCLK = 0
        end
    end

    -- calculate SPDIF RX clock frequency --------------------------------------
    if uC.NAME:match("STM32F446") then

        if SPIDFSRC == "RCC_SPDIFRXCLKSource_PLLR" then
            freq.SPDIF = freq.PLLR
        elseif SPIDFSRC == "RCC_SPDIFRXCLKSource_PLLI2SP" then
            freq.SPDIF = freq.PLLI2SP
        else
            freq.SPDIF = 0
        end
    end

    -- calculate CEC clock frequency -------------------------------------------
    if uC.NAME:match("STM32F446") then

        if CECSRC == "RCC_CECCLKSource_HSIDiv488" then
            freq.CEC = freq.HSI / 488
        elseif CECSRC == "RCC_CECCLKSource_LSE" then
            freq.CEC = freq.LSE
        else
            freq.CEC =  0
        end
    end

    -- calculate FMPI2C clock frequency ----------------------------------------
    if   uC.NAME:match("STM32F412.G") or uC.NAME:match("STM32F446")
      or uC.NAME:match("STM32F413")   or uC.NAME:match("STM32F423")
      or uC.NAME:match("STM32F410") then

        if FMPI2C1SRC == "RCC_FMPI2C1CLKSource_APB1" then
            freq.FMPI2C1 = freq.PCLK1
        elseif FMPI2C1SRC == "RCC_FMPI2C1CLKSource_SYSCLK" then
            freq.FMPI2C1 = freq.SYSCLK
        elseif FMPI2C1SRC == "RCC_FMPI2C1CLKSource_HSI" then
            freq.FMPI2C1 = freq.HSI
        else
            freq.FMPI2C1 = 0
        end
    end

    -- calculate flash latency -------------------------------------------------
    local FLV = 0
    if CPUVOLTAGE == "3p6V" then
        FLV = 30e6
    elseif CPUVOLTAGE == "2p7V" then
        FLV = 24e6
    elseif CPUVOLTAGE == "2p4V" then
        FLV = 22e6
    else --CPUVOLTAGE == "2p1V"
        FLV = 20e6
    end

    freq.FLASHLATENCY = math.ceil(freq.HCLK / FLV) - 1
    if freq.FLASHLATENCY < 0 then freq.FLASHLATENCY = 0 end
    this:SetFlagValue("__CLK_FLASH_LATENCY__", tostring(freq.FLASHLATENCY))
    freq.FLASHLATENCY = freq.FLASHLATENCY.." Flash wait-states"

    this:SetFlagValue("__CLK_HCLK_FREQ__", tostring(math.floor(freq.HCLK)))

    this.freq = freq
end

-- trigger frequency calculation after loading all flags
this.CalculateFreq()
++*/

#endif /* _CLK_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
