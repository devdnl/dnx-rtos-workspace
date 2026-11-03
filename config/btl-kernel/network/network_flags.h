#/*=============================================================================
# @file    network_flags.h
#
# @author  Daniel Zorychta
#
# @brief   This file contains list of existing network stacks.
#          Hybrid file: included both by Make and CC.
#
# @note    Copyright (C) 2015 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          This program is free software; you can redistribute it and/or modify
#          it under the terms of the GNU General Public License as published by
#          the Free Software Foundation and modified by the dnx RTOS exception.
#
#          NOTE: The modification  to the GPL is  included to allow you to
#                distribute a combined work that includes dnx RTOS without
#                being obliged to provide the source  code for proprietary
#                components outside of the dnx RTOS.
#
#          The dnx RTOS  is  distributed  in the hope  that  it will be useful,
#          but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
#          MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
#          GNU General Public License for more details.
#
#          Full license text is available on the following file: doc/license.txt.
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
#define __ENABLE_UDS_STACK__ _NO_
#/*
__ENABLE_UDS_STACK__=_NO_
#*/

#/*--
# this:AddWidget("Combobox", "TCP/IP: enable")
# this:AddItem("Yes", "_YES_")
# this:AddItem("No", "_NO_")
# this:SetToolTip("The TCP/IP is a global internet/local network stack. "..
#                 "Can be used in localhost domain as IPC.")
# this:AddExtraWidget("Submenu", "TCPIPConfigure", "TCP/IP: configure", "network/tcpip_flags.h")
#--*/
#define __ENABLE_TCPIP_STACK__ _NO_
#/*
__ENABLE_TCPIP_STACK__=_NO_
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
