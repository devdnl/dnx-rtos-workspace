/*==============================================================================
@file    uart_flags.h

@author  Daniel Zorychta

@brief   UART module configuration flags.

@note    MIT License

         Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

         Permission is hereby granted, free of charge, to any person obtaining
         a copy of this software and associated documentation files (the
         "Software"), to deal in the Software without restriction, including
         without limitation the rights to use, copy, modify, merge, publish,
         distribute, sublicense, and/or sell copies of the Software, and to
         permit persons to whom the Software is furnished to do so, subject to
         the following conditions:

         The above copyright notice and this permission notice shall be included
         in all copies or substantial portions of the Software.

         THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
         OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
         FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
         THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
         OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
         ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
         OTHER DEALINGS IN THE SOFTWARE.


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
    local UART_PER = {}
    UART_PER["RP2040"] = 2

    return UART_PER[uC.NAME] >= devNo
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
#define __UART_MAJOR0_RX_BUFFER_LEN__ 128

/*--
if this:PortExist(1) then
    this:AddWidget("Spinbox", 16, 65536, "UART0 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR0_TX_BUFFER_LEN__ 128

/*--
if this:PortExist(1) then
    this:AddWidget("Combobox", "UART0 IRQ priority")
    uC.AddPriorityItems(this, false)
end
--*/
#define __UART_UART0_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__



/*--
if this:PortExist(2) then
    this:AddExtraWidget("Label", "UART1_GAP", "")
    this:AddWidget("Spinbox", 16, 65536, "UART1 Rx buffer length [B]")
end
--*/
#define __UART_MAJOR1_RX_BUFFER_LEN__ 128

/*--
if this:PortExist(2) then
    this:AddWidget("Spinbox", 16, 65536, "UART1 Tx buffer length [B]")
end
--*/
#define __UART_MAJOR1_TX_BUFFER_LEN__ 128

/*--
if this:PortExist(2) then
    this:AddWidget("Combobox", "UART1 IRQ priority")
    uC.AddPriorityItems(this, false)
end
--*/
#define __UART_UART1_IRQ_PRIORITY__ __CPU_DEFAULT_IRQ_PRIORITY__


#endif /* _UART_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
