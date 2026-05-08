/*==============================================================================
@file    spi_flags.h

@author  Daniel Zorychta

@brief   SPI module configuration flags.

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

#ifndef _SPI_FLAGS_H_
#define _SPI_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > SPI", "The SPI module configuration.")
++*/

/*--
this:AddWidget("Combobox", "Default Clock divider")
this:AddItem("PCLK / 2", "SPI_CLK_DIV__2")
this:AddItem("PCLK / 4", "SPI_CLK_DIV__4")
this:AddItem("PCLK / 8", "SPI_CLK_DIV__8")
this:AddItem("PCLK / 16", "SPI_CLK_DIV__16")
this:AddItem("PCLK / 32", "SPI_CLK_DIV__32")
this:AddItem("PCLK / 64", "SPI_CLK_DIV__64")
this:AddItem("PCLK / 128", "SPI_CLK_DIV__128")
this:AddItem("PCLK / 256", "SPI_CLK_DIV__256")
--*/
#define __SPI_DEFAULT_CLK_DIV__ SPI_CLK_DIV__4

/*--
this:AddWidget("Combobox", "Default SPI mode")
this:AddItem("Mode 0: SCK Low at idle, capture on rising edge", "SPI_MODE__0")
this:AddItem("Mode 1: SCK Low at idle, capture on falling edge", "SPI_MODE__1")
this:AddItem("Mode 2: SCK High at idle, capture on falling edge", "SPI_MODE__2")
this:AddItem("Mode 3: SCK High at idle, capture on rising edge", "SPI_MODE__3")
--*/
#define __SPI_DEFAULT_MODE__ SPI_MODE__0

/*--
this:AddWidget("Combobox", "Default Bit order")
this:AddItem("LSb first", "_NO_")
this:AddItem("MSb first", "_YES_")
--*/
#define __SPI_DEFAULT_MSB_FIRST__ _YES_

/*--
this:AddWidget("Combobox", "SPI1 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI1_MODE__ _SPI_MODE_DMA

/*--
this:AddWidget("Combobox", "SPI2 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI2_MODE__ _SPI_MODE_DMA

/*--
this:AddWidget("Combobox", "SPI3 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI3_MODE__ _SPI_MODE_DMA

/*--
this:AddWidget("Combobox", "SPI4 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI4_MODE__ _SPI_MODE_DMA

/*--
this:AddWidget("Combobox", "SPI5 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI5_MODE__ _SPI_MODE_DMA

/*--
this:AddWidget("Combobox", "SPI6 mode")
this:AddItem("Polling", "_SPI_MODE_POLLING")
this:AddItem("IRQ", "_SPI_MODE_IRQ")
this:AddItem("DMA", "_SPI_MODE_DMA")
--*/
#define __SPI_SPI6_MODE__ _SPI_MODE_DMA

#endif /* _SPI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
