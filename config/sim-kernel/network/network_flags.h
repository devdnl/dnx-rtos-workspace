#/*=============================================================================
# @file    network_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains list of existing network stacks.
#          Hybrid file: included both by Make and CC.
#
# @note    MIT License
#
#          Copyright (C) 2015 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          Permission is hereby granted, free of charge, to any person obtaining
#          a copy of this software and associated documentation files (the
#          "Software"), to deal in the Software without restriction, including
#          without limitation the rights to use, copy, modify, merge, publish,
#          distribute, sublicense, and/or sell copies of the Software, and to
#          permit persons to whom the Software is furnished to do so, subject to
#          the following conditions:
#
#          The above copyright notice and this permission notice shall be included
#          in all copies or substantial portions of the Software.
#
#          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#          OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#          FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#          THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#          OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#          ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#          OTHER DEALINGS IN THE SOFTWARE.
#
#
#=============================================================================*/

#/*
#* NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as suffix
#*       and prefix) are exported to the single configuration file
#*       (by using Configtool) when entire project configuration is exported.
#*       All other flag definitions and statements are ignored.
#*/

#ifndef _NETWORK_FLAGS_H_
#define _NETWORK_FLAGS_H_

#/*--
# this:SetTitle("Home > Network", "Network configuration menu.")
#++*/

#/*--
# this:AddWidget("Combobox", "Unix Domain Socket: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The Unix Domain Socket is a internal system network that can "..
#                 "be used as IPC.")
# this:AddExtraWidget("Submenu", "UDSConfigure", "Unix Domain Socket: configure", "network/uds_flags.h")
#--*/
#define __ENABLE_UDS_STACK__ _YES_
#/*
__ENABLE_UDS_STACK__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "TCP/IP: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The TCP/IP is a global internet/local network stack. "..
#                 "Can be used in localhost domain as IPC.")
# this:AddExtraWidget("Submenu", "TCPIPConfigure", "TCP/IP: configure", "network/tcpip_flags.h")
#--*/
#define __ENABLE_TCPIP_STACK__ _YES_
#/*
__ENABLE_TCPIP_STACK__=_YES_
#*/

#/*--
# this:AddWidget("Combobox", "SIPC: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The SIPC is a simple serial protocol used in point-to-point communication. "..
#                 "That stack is very limited and is not recommended to be used. It will be "..
#                 "removed in the future.")
# this:AddExtraWidget("Submenu", "SIPCConfigure", "SIPC: configure", "network/sipc_flags.h")
#--*/
#define __ENABLE_SIPC_STACK__ _NO_
#/*
__ENABLE_SIPC_STACK__=_NO_
#*/

#/*--
# this:AddWidget("Combobox", "CANNET: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The CANNET is a network stack that can be used in CAN networks. "..
#                 "Stack provides full TCP/UDP-like handling.")
# this:AddExtraWidget("Submenu", "CANNETConfigure", "CANNET: configure", "network/cannet_flags.h")
#--*/
#define __ENABLE_CANNET_STACK__ _NO_
#/*
__ENABLE_CANNET_STACK__=_NO_
#*/

#if __ENABLE_UDS_STACK__ == _YES_
#include "uds_flags.h"
#endif

#if __ENABLE_TCPIP_STACK__ == _YES_
#include "tcpip_flags.h"
#endif

#if __ENABLE_SIPC_STACK__ == _YES_
#include "sipc_flags.h"
#endif

#if __ENABLE_CANNET_STACK__ == _YES_
#include "cannet_flags.h"
#endif


#/* MASTER NETWORK SUBSYSTEM ENABLE */
#define _ENABLE_NETWORK_ (__ENABLE_TCPIP_STACK__ || __ENABLE_SIPC_STACK__ || __ENABLE_CANNET_STACK__ || __ENABLE_UDS_STACK__)
#/*
ifeq ($(__ENABLE_TCPIP_STACK__), _YES_)
_ENABLE_NETWORK_=_YES_
else ifeq ($(__ENABLE_SIPC_STACK__), _YES_)
_ENABLE_NETWORK_=_YES_
else ifeq ($(__ENABLE_CANNET_STACK__), _YES_)
_ENABLE_NETWORK_=_YES_
else ifeq ($(__ENABLE_UDS_STACK__), _YES_)
_ENABLE_NETWORK_=_YES_
endif
#*/

#endif /* _NETWORK_FLAGS_H_ */
#/*=============================================================================
#  End of file
#=============================================================================*/
