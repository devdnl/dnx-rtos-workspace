/*==============================================================================
@file    uart_flags.h

@author  Daniel Zorychta

@brief   UART module configuration flags.

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

#ifndef _UART_FLAGS_H_
#define _UART_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > UART", "The UART module configuration.")

this.PortExist = function(this, devNo)
    return devNo <= 16
end
++*/

/*--
this:AddWidget("Combobox", "Default Parity bit")
this:AddItem("Off", "UART_PARITY__OFF")
this:AddItem("Odd", "UART_PARITY__ODD")
this:AddItem("Even", "UART_PARITY__EVEN")
--*/
#define __UART_DEFAULT_PARITY__ UART_PARITY__OFF

/*--
this:AddWidget("Combobox", "Default Stop bit")
this:AddItem("1", "UART_STOP_BIT__1")
this:AddItem("2", "UART_STOP_BIT__2")
--*/
#define __UART_DEFAULT_STOP_BITS__ UART_STOP_BIT__1

/*--
this:AddWidget("Combobox", "Default Tx line enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __UART_DEFAULT_TX_ENABLE__ _YES_

/*--
this:AddWidget("Combobox", "Default Rx line enable")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __UART_DEFAULT_RX_ENABLE__ _YES_

/*--
this:AddWidget("Editline", false, "Default Baud [bps]")
this:AddItem("110", "")
this:AddItem("150", "")
this:AddItem("300", "")
this:AddItem("1200", "")
this:AddItem("2400", "")
this:AddItem("4800", "")
this:AddItem("9600", "")
this:AddItem("19200", "")
this:AddItem("38400", "")
this:AddItem("57600", "")
this:AddItem("115200", "")
this:AddItem("230400", "")
this:AddItem("460800", "")
this:AddItem("921600", "")
this:AddItem("1000000", "")
this:AddItem("2000000", "")
this:AddItem("4000000", "")
--*/
#define __UART_DEFAULT_BAUD__ 115200


/*--
if this:PortExist(1) then
    this:AddExtraWidget("Label", "UART0_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART0 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR0_RX_BUFFER_LEN__ 4096

/*--
if this:PortExist(1) then
    this:AddWidget("Spinbox", 16, 65536, "UART0 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR0_TX_BUFFER_LEN__ 4096

/*--
if this:PortExist(2) then
    this:AddExtraWidget("Label", "UART1_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART1 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR1_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(2) then
    this:AddWidget("Spinbox", 16, 65536, "UART1 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR1_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(3) then
    this:AddExtraWidget("Label", "UART2_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART2 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR2_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(3) then
    this:AddWidget("Spinbox", 16, 65536, "UART2 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR2_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(4) then
    this:AddExtraWidget("Label", "UART3_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART3 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR3_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(4) then
    this:AddWidget("Spinbox", 16, 65536, "UART3 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR3_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(5) then
    this:AddExtraWidget("Label", "UART4_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART4 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR4_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(5) then
    this:AddWidget("Spinbox", 16, 65536, "UART4 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR4_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(6) then
    this:AddExtraWidget("Label", "UART5_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART5 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR5_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(6) then
    this:AddWidget("Spinbox", 16, 65536, "UART5 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR5_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(7) then
    this:AddExtraWidget("Label", "UART6_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART6 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR6_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(7) then
    this:AddWidget("Spinbox", 16, 65536, "UART6 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR6_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(8) then
    this:AddExtraWidget("Label", "UART7_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART7 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR7_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(8) then
    this:AddWidget("Spinbox", 16, 65536, "UART7 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR7_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(9) then
    this:AddExtraWidget("Label", "UART8_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART8 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR8_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(9) then
    this:AddWidget("Spinbox", 16, 65536, "UART8 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR8_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(10) then
    this:AddExtraWidget("Label", "UART9_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART9 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR9_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(10) then
    this:AddWidget("Spinbox", 16, 65536, "UART9 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR9_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(11) then
    this:AddExtraWidget("Label", "UART10_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART10 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR10_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(11) then
    this:AddWidget("Spinbox", 16, 65536, "UART10 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR10_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(12) then
    this:AddExtraWidget("Label", "UART11_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART11 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR11_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(12) then
    this:AddWidget("Spinbox", 16, 65536, "UART11 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR11_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(13) then
    this:AddExtraWidget("Label", "UART12_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART12 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR12_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(13) then
    this:AddWidget("Spinbox", 16, 65536, "UART12 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR12_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(14) then
    this:AddExtraWidget("Label", "UART13_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART13 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR13_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(14) then
    this:AddWidget("Spinbox", 16, 65536, "UART13 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR13_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(15) then
    this:AddExtraWidget("Label", "UART14_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART14 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR14_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(15) then
    this:AddWidget("Spinbox", 16, 65536, "UART14 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR14_TX_BUFFER_LEN__ 1024

/*--
if this:PortExist(16) then
    this:AddExtraWidget("Label", "UART15_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART15 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR15_RX_BUFFER_LEN__ 1024

/*--
if this:PortExist(16) then
    this:AddWidget("Spinbox", 16, 65536, "UART15 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR15_TX_BUFFER_LEN__ 1024

#endif /* _UART_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
