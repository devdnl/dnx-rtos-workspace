/*==============================================================================
@file    ETH_flags.h

@author  Daniel Zorychta

@brief   ETHMAC module configuration flags.

@note    MIT License

         Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _ETH_FLAGS_H_
#define _ETH_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > ETH", "The ETH module configuration.")
++*/

/*--
this:AddWidget("Spinbox", 3, 256, "Number of RX buffers")
this:SetToolTip("Each buffer is 1524 bytes long.")
--*/
#define __ETH_RXBUFNB__ 10

/*--
this:AddWidget("Spinbox", 2, 256, "Number of Tx buffers")
this:SetToolTip("Each buffer is 1524 bytes long.")
--*/
#define __ETH_TXBUFNB__ 10

#define DP83848 0
#define LAN8700 1
#define STE100P 2
#define LAN8742A 3
#define KSZ9893R 4

/*--
this:AddWidget("Combobox", "Device")
this:AddItem("DP83848", "DP83848")
this:AddItem("LAN8700", "LAN8700")
this:AddItem("STE100P", "STE100P")
this:AddItem("LAN8742A", "LAN8742A")
this:AddItem("KSZ9893R", "KSZ9893R")
this:SetEvent("clicked",
    function()
        local dev = this:GetFlagValue("__ETH_DEVICE__")
        local DEVICE = {}
        DEVICE["DP83848"]  = {SR = "16", SPEED_BM = "0x0002", DUPLEX_BM = "0x0004"}
        DEVICE["LAN8700"]  = {SR = "31", SPEED_BM = "0x0004", DUPLEX_BM = "0x0010"}
        DEVICE["STE100P"]  = {SR = "19", SPEED_BM = "0x0004", DUPLEX_BM = "0x0010"}
        DEVICE["LAN8742A"] = {SR = "31", SPEED_BM = "0x0004", DUPLEX_BM = "0x0010"}
        DEVICE["KSZ9893R"] = {SR = "31", SPEED_BM = "0x0010", DUPLEX_BM = "0x0008"}

        this:SetFlagValue("__ETH_PHY_SR__", DEVICE[dev].SR)
        this:SetFlagValue("__ETH_PHY_SPEED_STATUS_BM__", DEVICE[dev].SPEED_BM)
        this:SetFlagValue("__ETH_PHY_DUPLEX_STATUS_BM__", DEVICE[dev].DUPLEX_BM)
    end
)

--*/
#define __ETH_DEVICE__ LAN8742A

/*-- Value set by __ETH_DEVICE__ event
this:AddWidget("Value")
--*/
#define __ETH_PHY_SR__ 31

/*-- Value set by __ETH_DEVICE__ event
this:AddWidget("Value")
--*/
#define __ETH_PHY_SPEED_STATUS_BM__ 0x0004

/*-- Value set by __ETH_DEVICE__ event
this:AddWidget("Value")
--*/
#define __ETH_PHY_DUPLEX_STATUS_BM__ 0x0010

/*--
this:AddWidget("Spinbox", 0, 31, "Address")
--*/
#define __ETH_PHY_ADDRESS__ 0

/*--
this:AddWidget("Spinbox", 100, 2000, "Reset delay [ms]")
--*/
#define __ETH_PHY_RESET_DELAY__ 250

/*--
this:AddWidget("Spinbox", 100, 2000, "Setup delay [ms]")
--*/
#define __ETH_PHY_CONFIG_DELAY__ 500

/*--
this:AddWidget("Combobox", "Auto negotiation")
this:AddItem("Disable", "ETH_AUTONEGOTIATION_DISABLE")
this:AddItem("Enable", "ETH_AUTONEGOTIATION_ENABLE")
this:SetToolTip("If auto negotiation is enabled then link should be connected to PHY. "..
                "If link is not connected then module does not initialize.")
--*/
#define __ETH_PHY_AUTONEGOTIATION__ ETH_AUTONEGOTIATION_ENABLE

#endif /* _ETH_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
