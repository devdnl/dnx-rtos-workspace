/*==============================================================================
@file    i2c_flags.h

@author  Daniel Zorychta

@brief   This driver support I2C peripherals.

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _I2C_FLAGS_H_
#define _I2C_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > I²C", "The I²C module configuration.")
++*/

/*----------------------------------------------------------------------------*/
/*--
this:AddWidget("Editline", false, "I2C1 SCL frequency [Hz]")
--*/
#define __I2C1_SCL_FREQ__ 100000

/*--
this:AddWidget("Combobox", "I2C1 Analog filter")
this:AddItem("DISABLE", "I2C_CR1_ANFOFF")
this:AddItem("ENABLE", "0")
--*/
#define __I2C1_ANALOG_FILTER_DISABLE__ 0

/*--
this:AddWidget("Combobox", "I2C1 Digital filter")
this:AddItem("DISABLE", "0")
for i=1,15 do this:AddItem(tostring(i).." clock periods", tostring(i)) end
--*/
#define __I2C1_DIGITAL_FILTER__ 0

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C1 SCL rising time [ns]")
--*/
#define __I2C1_SCL_RISE_TIME_ns__ 300

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C1 SCL falling time [ns]")
--*/
#define __I2C1_SCL_FALL_TIME_ns__ 300

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C1 Data hold time SCL-SDA [ns]")
--*/
#define __I2C1_DATA_HOLD_TIME_ns__ 50

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C1 Data setup time SCL-SDA [ns]")
--*/
#define __I2C1_DATA_SETUP_TIME_ns__ 50

/*--
this:AddWidget("Combobox", "I2C1 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __I2C1_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

/*----------------------------------------------------------------------------*/
/*--
this:AddExtraWidget("Label", "I2C2_GAP", "")
this:AddWidget("Editline", false, "I2C2 SCL frequency [Hz]")
--*/
#define __I2C2_SCL_FREQ__ 100000

/*--
this:AddWidget("Combobox", "I2C2 Analog filter")
this:AddItem("DISABLE", "I2C_CR1_ANFOFF")
this:AddItem("ENABLE", "0")
--*/
#define __I2C2_ANALOG_FILTER_DISABLE__ 0

/*--
this:AddWidget("Combobox", "I2C2 Digital filter")
this:AddItem("DISABLE", "0")
for i=1,15 do this:AddItem(tostring(i).." clock periods", tostring(i)) end
--*/
#define __I2C2_DIGITAL_FILTER__ 0

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C2 SCL rising time [ns]")
--*/
#define __I2C2_SCL_RISE_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C2 SCL falling time [ns]")
--*/
#define __I2C2_SCL_FALL_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C2 Data hold time SCL-SDA [ns]")
--*/
#define __I2C2_DATA_HOLD_TIME_ns__ 10

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C2 Data setup time SCL-SDA [ns]")
--*/
#define __I2C2_DATA_SETUP_TIME_ns__ 10

/*--
this:AddWidget("Combobox", "I2C2 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __I2C2_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

/*----------------------------------------------------------------------------*/
/*--
this:AddExtraWidget("Label", "I2C3_GAP", "")
this:AddWidget("Editline", false, "I2C3 SCL frequency [Hz]")
uC.AddPriorityItems(this)
--*/
#define __I2C3_SCL_FREQ__ 100000

/*--
this:AddWidget("Combobox", "I2C3 Analog filter")
this:AddItem("DISABLE", "I2C_CR1_ANFOFF")
this:AddItem("ENABLE", "0")
--*/
#define __I2C3_ANALOG_FILTER_DISABLE__ 0

/*--
this:AddWidget("Combobox", "I2C3 Digital filter")
this:AddItem("DISABLE", "0")
for i=1,15 do this:AddItem(tostring(i).." clock periods", tostring(i)) end
--*/
#define __I2C3_DIGITAL_FILTER__ 0

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C3 SCL rising time [ns]")
--*/
#define __I2C3_SCL_RISE_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C3 SCL falling time [ns]")
--*/
#define __I2C3_SCL_FALL_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C3 Data hold time SCL-SDA [ns]")
--*/
#define __I2C3_DATA_HOLD_TIME_ns__ 10

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C3 Data setup time SCL-SDA [ns]")
--*/
#define __I2C3_DATA_SETUP_TIME_ns__ 10

/*--
this:AddWidget("Combobox", "I2C3 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __I2C3_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__


/*----------------------------------------------------------------------------*/
/*--
this:AddExtraWidget("Label", "I2C4_GAP", "")
this:AddWidget("Editline", false, "I2C4 SCL frequency [Hz]")
uC.AddPriorityItems(this)
--*/
#define __I2C4_SCL_FREQ__ 100000

/*--
this:AddWidget("Combobox", "I2C4 Analog filter")
this:AddItem("DISABLE", "I2C_CR1_ANFOFF")
this:AddItem("ENABLE", "0")
--*/
#define __I2C4_ANALOG_FILTER_DISABLE__ 0

/*--
this:AddWidget("Combobox", "I2C4 Digital filter")
this:AddItem("DISABLE", "0")
for i=1,15 do this:AddItem(tostring(i).." clock periods", tostring(i)) end
--*/
#define __I2C4_DIGITAL_FILTER__ 0

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C4 SCL rising time [ns]")
--*/
#define __I2C4_SCL_RISE_TIME_ns__ 640

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C4 SCL falling time [ns]")
--*/
#define __I2C4_SCL_FALL_TIME_ns__ 20

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C4 Data hold time SCL-SDA [ns]")
--*/
#define __I2C4_DATA_HOLD_TIME_ns__ 10

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C4 Data setup time SCL-SDA [ns]")
--*/
#define __I2C4_DATA_SETUP_TIME_ns__ 10

/*--
this:AddWidget("Combobox", "I2C4 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __I2C4_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__


/*----------------------------------------------------------------------------*/
/*--
this:AddExtraWidget("Label", "I2C5_GAP", "")
this:AddWidget("Editline", false, "I2C5 SCL frequency [Hz]")
uC.AddPriorityItems(this)
--*/
#define __I2C5_SCL_FREQ__ 100000

/*--
this:AddWidget("Combobox", "I2C5 Analog filter")
this:AddItem("DISABLE", "I2C_CR1_ANFOFF")
this:AddItem("ENABLE", "0")
--*/
#define __I2C5_ANALOG_FILTER_DISABLE__ 0

/*--
this:AddWidget("Combobox", "I2C5 Digital filter")
this:AddItem("DISABLE", "0")
for i=1,15 do this:AddItem(tostring(i).." clock periods", tostring(i)) end
--*/
#define __I2C5_DIGITAL_FILTER__ 0

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C5 SCL rising time [ns]")
--*/
#define __I2C5_SCL_RISE_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 0, 1000, "I2C5 SCL falling time [ns]")
--*/
#define __I2C5_SCL_FALL_TIME_ns__ 100

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C5 Data hold time SCL-SDA [ns]")
--*/
#define __I2C5_DATA_HOLD_TIME_ns__ 10

/*--
this:AddWidget("Spinbox", 10, 1000, "I2C5 Data setup time SCL-SDA [ns]")
--*/
#define __I2C5_DATA_SETUP_TIME_ns__ 10

/*--
this:AddWidget("Combobox", "I2C5 IRQ priority")
uC.AddPriorityItems(this, false)
--*/
#define __I2C5_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__

#endif /* _I2C_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
