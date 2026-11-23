/*==============================================================================
@file    cannet_flags.h

@author  Daniel Zorychta

@brief   This file contains configuration of CANNET stack.
         Hybrid file: included both by Make and CC.

@note    MIT License

         Copyright (C) 2021 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _CANNET_FLAGS_H_
#define _CANNET_FLAGS_H_

/*--
this:SetTitle("Home > Network > CANNET Stack", "The CANNET stack configuration.")
++*/

/*--
this:AddWidget("Spinbox", 128, 65536, "Max socket buffer length [B]")
--*/
#define __NETWORK_CANNET_MAX_SOCKET_BUFFER_LENGTH__ 256

/*--
this:AddWidget("Spinbox", 4, 256, "Maximum number of sockets")
--*/
#define __NETWORK_CANNET_MAX_SOCKETS__ 16

/*--
this:AddWidget("Spinbox", 3, 16, "Packet send repetitions")
--*/
#define __NETWORK_CANNET_SEND_REPETITIONS__ 3

/*--
this:AddWidget("Spinbox", 5, 1000, "Packet repetition interval min [ms]")
--*/
#define __NETWORK_CANNET_SEND_REPETITIONS_INTERVAL_MIN__ 3

/*--
this:AddWidget("Spinbox", 5, 1000, "Packet repetition interval max [ms]")
--*/
#define __NETWORK_CANNET_SEND_REPETITIONS_INTERVAL_MAX__ 250

/*--
this:AddWidget("Combobox", "Debug messages")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
--*/
#define __NETWORK_CANNET_DEBUG_ON__ _NO_

#endif /* _CANNET_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
