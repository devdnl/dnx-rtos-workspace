/*==============================================================================
@file    afm_flags.h

@author  Daniel Zorychta

@brief   SYSCFG Module configuration.

@note    Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _AFM_FLAGS_H_
#define _AFM_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > AFM", "The AFM module configuration.")
++*/

/*--
this:AddWidget("Combobox", "PC3 Switch open")
this:AddItem("Closed (PC3 and PC3_C connected)", "(0 << SYSCFG_PMCR_PC3SO_Pos)")
this:AddItem("Open (PC3 and PC3_C disconnected)", "(1 << SYSCFG_PMCR_PC3SO_Pos)")
--*/
#define __AFM_PC3SO__ (0 << SYSCFG_PMCR_PC3SO_Pos)

/*--
this:AddWidget("Combobox", "PC2 Switch open")
this:AddItem("Closed (PC2 and PC2_C connected)", "(0 << SYSCFG_PMCR_PC2SO_Pos)")
this:AddItem("Open (PC2 and PC2_C disconnected)", "(1 << SYSCFG_PMCR_PC2SO_Pos)")
--*/
#define __AFM_PC2SO__ (0 << SYSCFG_PMCR_PC2SO_Pos)

/*--
this:AddWidget("Combobox", "PA1 Switch open")
this:AddItem("Closed (PA1 and PA1_C connected)", "(0 << SYSCFG_PMCR_PA1SO_Pos)")
this:AddItem("Open (PA1 and PA1_C disconnected)", "(1 << SYSCFG_PMCR_PA1SO_Pos)")
--*/
#define __AFM_PA1SO__ (0 << SYSCFG_PMCR_PA1SO_Pos)

/*--
this:AddWidget("Combobox", "PA0 Switch open")
this:AddItem("Closed (PA0 and PA0_C connected)", "(0 << SYSCFG_PMCR_PA0SO_Pos)")
this:AddItem("Open (PA0 and PA0_C disconnected)", "(1 << SYSCFG_PMCR_PA0SO_Pos)")
--*/
#define __AFM_PA0SO__ (0 << SYSCFG_PMCR_PA0SO_Pos)

/*--
this:AddWidget("Combobox", "Ethernet PHY interface")
this:AddItem("MII interface", "(0 << SYSCFG_PMCR_EPIS_SEL_Pos)")
this:AddItem("RMII interface", "(4 << SYSCFG_PMCR_EPIS_SEL_Pos)")
--*/
#define __AFM_MII_RMII_SEL__ (0 << SYSCFG_PMCR_EPIS_SEL_Pos)

/*--
this:AddWidget("Combobox", "Analog switch Booster voltage selection")
this:AddItem("VDD", "(0 << SYSCFG_PMCR_BOOSTVDDSEL_Pos)")
this:AddItem("VDDA", "(1 << SYSCFG_PMCR_BOOSTVDDSEL_Pos)")
--*/
#define __AFM_BOOSTVDDSEL__ (0 << SYSCFG_PMCR_BOOSTVDDSEL_Pos)

/*--
this:AddWidget("Combobox", "Analog switch Booster")
this:AddItem("Disable", "(0 << SYSCFG_PMCR_BOOSTEN_Pos)")
this:AddItem("Enable", "(1 << SYSCFG_PMCR_BOOSTEN_Pos)")
--*/
#define __AFM_BOOSTEREN__ (0 << SYSCFG_PMCR_BOOSTEN_Pos)

/*--
this:AddWidget("Combobox", "PB9 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C_PB9_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on PB9 pin", "(1 << SYSCFG_PMCR_I2C_PB9_FMP_Pos)")
--*/
#define __AFM_PB9_FMP__ (0 << SYSCFG_PMCR_I2C_PB9_FMP_Pos)

/*--
this:AddWidget("Combobox", "PB8 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C_PB8_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on PB8 pin", "(1 << SYSCFG_PMCR_I2C_PB8_FMP_Pos)")
--*/
#define __AFM_PB8_FMP__ (0 << SYSCFG_PMCR_I2C_PB8_FMP_Pos)

/*--
this:AddWidget("Combobox", "PB7 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C_PB7_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on PB7 pin", "(1 << SYSCFG_PMCR_I2C_PB7_FMP_Pos)")
--*/
#define __AFM_PB7_FMP__ (0 << SYSCFG_PMCR_I2C_PB7_FMP_Pos)

/*--
this:AddWidget("Combobox", "PB6 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C_PB6_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on PB6 pin", "(1 << SYSCFG_PMCR_I2C_PB6_FMP_Pos)")
--*/
#define __AFM_PB6_FMP__ (0 << SYSCFG_PMCR_I2C_PB6_FMP_Pos)

/*--
this:AddWidget("Combobox", "I2C4 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C4_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on I2C4", "(1 << SYSCFG_PMCR_I2C4_FMP_Pos)")
--*/
#define __AFM_I2C4_FMP__ (0 << SYSCFG_PMCR_I2C4_FMP_Pos)

/*--
this:AddWidget("Combobox", "I2C3 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C3_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on I2C3", "(1 << SYSCFG_PMCR_I2C3_FMP_Pos)")
--*/
#define __AFM_I2C3_FMP__ (0 << SYSCFG_PMCR_I2C3_FMP_Pos)

/*--
this:AddWidget("Combobox", "I2C2 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C2_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on I2C2", "(1 << SYSCFG_PMCR_I2C2_FMP_Pos)")
--*/
#define __AFM_I2C2_FMP__ (0 << SYSCFG_PMCR_I2C2_FMP_Pos)

/*--
this:AddWidget("Combobox", "I2C1 Fast Mode + Enable")
this:AddItem("Default value", "(0 << SYSCFG_PMCR_I2C1_FMP_Pos)")
this:AddItem("It forces FM+ drive capability on I2C1", "(1 << SYSCFG_PMCR_I2C1_FMP_Pos)")
--*/
#define __AFM_I2C1_FMP__ (0 << SYSCFG_PMCR_I2C1_FMP_Pos)

/*--
this:AddWidget("Combobox", "D1 AXI-SRAM dobule ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_AXISRAML_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_AXISRAML_Pos)")
--*/
#define __AFM_AXISRAML__ (0 << SYSCFG_CFGR_AXISRAML_Pos)

/*--
this:AddWidget("Combobox", "D1 ITCM doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_ITCML_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_ITCML_Pos)")
--*/
#define __AFM_ITCML__ (0 << SYSCFG_CFGR_ITCML_Pos)

/*--
this:AddWidget("Combobox", "D1 DTCM doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_DTCML_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_DTCML_Pos)")
--*/
#define __AFM_DTCML__ (0 << SYSCFG_CFGR_DTCML_Pos)

/*--
this:AddWidget("Combobox", "D2 SRAM1 doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_SRAM1L_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_SRAM1L_Pos)")
--*/
#define __AFM_SRAM1L__ (0 << SYSCFG_CFGR_SRAM1L_Pos)

/*--
this:AddWidget("Combobox", "D2 SRAM2 doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_SRAM2L_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_SRAM2L_Pos)")
--*/
#define __AFM_SRAM2L__ (0 << SYSCFG_CFGR_SRAM2L_Pos)

/*--
this:AddWidget("Combobox", "D2 SRAM3 doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_SRAM3L_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_SRAM3L_Pos)")
--*/
#define __AFM_SRAM3L__ (0 << SYSCFG_CFGR_SRAM3L_Pos)

/*--
this:AddWidget("Combobox", "D2 SRAM4 doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_SRAM4L_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_SRAM4L_Pos)")
--*/
#define __AFM_SRAM4L__ (0 << SYSCFG_CFGR_SRAM4L_Pos)

/*--
this:AddWidget("Combobox", "D2 BKRAM doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_BKRAML_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_BKRAML_Pos)")
--*/
#define __AFM_BKRAML__ (0 << SYSCFG_CFGR_BKRAML_Pos)

/*--
this:AddWidget("Combobox", "CM7 Lockup (HardFault) output")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_CM7L_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_CM7L_Pos)")
--*/
#define __AFM_CM7L__ (0 << SYSCFG_CFGR_CM7L_Pos)

/*--
this:AddWidget("Combobox", "D2 FLASH doubler ECC error lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_FLASHL_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_FLASHL_Pos)")
--*/
#define __AFM_FLASHL__ (0 << SYSCFG_CFGR_FLASHL_Pos)

/*--
this:AddWidget("Combobox", "PVD Lock")
this:AddItem("Disconnected from TIM1/8/15/16/17/HRTIMER", "(0 << SYSCFG_CFGR_PVDL_Pos)")
this:AddItem("Connected to TIM1/8/15/16/17/HRTIMER", "(1 << SYSCFG_CFGR_PVDL_Pos)")
--*/
#define __AFM_PVDL__ (0 << SYSCFG_CFGR_PVDL_Pos)


/*--
this:AddWidget("Combobox", "High-speed at low-voltage")
this:AddItem("No I/O speed optimization", "(0 << SYSCFG_CCCSR_HSLV_Pos)")
this:AddItem("I/O speed optimization", "(1 << SYSCFG_CCCSR_HSLV_Pos)")
--*/
#define __AFM_HSLV__ (0 << SYSCFG_CCCSR_HSLV_Pos)

/*--
this:AddWidget("Combobox", "Code selection")
this:AddItem("Code from the cell", "(0 << SYSCFG_CCCSR_CS_Pos)")
this:AddItem("User compensation code (PMOS & NMOS)", "(1 << SYSCFG_CCCSR_CS_Pos)")
--*/
#define __AFM_CS__ (0 << SYSCFG_CCCSR_CS_Pos)

/*--
this:AddWidget("Spinbox", 0, 15, "PMOS compensation code")
--*/
#define __AFM_PCC__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "NMOS compensation code")
--*/
#define __AFM_NCC__ 0

/*--
this:AddWidget("Combobox", "Compensation cell enable")
this:AddItem("I/O compensation cell disabled", "(0 << SYSCFG_CCCSR_EN_Pos)")
this:AddItem("I/O compensation cell enabled", "(1 << SYSCFG_CCCSR_EN_Pos)")
--*/
#define __AFM_CMP_EN__ (0 << SYSCFG_CCCSR_EN_Pos)


/*--
this:AddWidget("Combobox", "D1 Standby reset")
this:AddItem("A reset is generated by entering D1 Standby mode", "(0 << SYSCFG_UR13_D1SBRST_Pos)")
this:AddItem("D1 Standby mode is entered without reset generation", "(1 << SYSCFG_UR13_D1SBRST_Pos)")
--*/
#define __AFM_D1SBRST__ (0 << SYSCFG_UR13_D1SBRST_Pos)

/*--
this:AddWidget("Combobox", "D1 Stop reset")
this:AddItem("A reset is generated by entering D1 Stop mode", "(0 << SYSCFG_UR14_D1STPRST_Pos)")
this:AddItem("D1 Stop mode is entered without reset generation", "(1 << SYSCFG_UR14_D1STPRST_Pos)")
--*/
#define __AFM_D1STPRST__ (0 << SYSCFG_UR14_D1STPRST_Pos)


/*--
this:AddWidget("Spinbox", 0, 15, "Channel 1 (D2 AHB) read priority")
--*/
#define __AFM_AXI_IN1_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 1 (D2 AHB) write priority")
--*/
#define __AFM_AXI_IN1_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 2 (CM7) read priority")
--*/
#define __AFM_AXI_IN2_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 2 (CM7) write priority")
--*/
#define __AFM_AXI_IN2_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 3 (SDMMC1) read priority")
--*/
#define __AFM_AXI_IN3_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 3 (SDMMC1) write priority")
--*/
#define __AFM_AXI_IN3_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 4 (MDMA) read priority")
--*/
#define __AFM_AXI_IN4_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 4 (MDMA) write priority")
--*/
#define __AFM_AXI_IN4_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 5 (DMA2D) read priority")
--*/
#define __AFM_AXI_IN5_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 5 (DMA2D) write priority")
--*/
#define __AFM_AXI_IN5_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 6 (LTDC) read priority")
--*/
#define __AFM_AXI_IN6_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 6 (LTDC) write priority")
--*/
#define __AFM_AXI_IN6_WRITE_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 7 (ART) read priority")
--*/
#define __AFM_AXI_IN7_READ_QOS__ 0

/*--
this:AddWidget("Spinbox", 0, 15, "Channel 7 (ART) write priority")
--*/
#define __AFM_AXI_IN7_WRITE_QOS__ 0

#endif /* _AFIO_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
