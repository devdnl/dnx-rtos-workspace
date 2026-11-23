/*==============================================================================
@file    uds_flags.h

@author  Daniel Zorychta

@brief   This file contains configuration of Unix Domain Socket stack.
         Hybrid file: included both by Make and CC.

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

#ifndef _UDS_FLAGS_H_
#define _UDS_FLAGS_H_

/*--
this:SetTitle("Home > Network > UDS Stack", "The Unix Domanin Socket Stack configuration.")
++*/

/*--
this:AddWidget("Combobox", "Debug messages")
this:AddItem("Disable", "_NO_")
this:AddItem("Enable", "_YES_")
--*/
#define __NETWORK_UDS_DEBUG_ON__ _NO_

/*--
this:AddWidget("Spinbox", 128, 65536, "Stream receive buffer size [B]")
--*/
#define __NETWORK_UDS_BUF_SIZE__ 2048

/*--
this:AddWidget("Spinbox", 16, 65536, "Datagram max receive buffer size [B]")
--*/
#define __NETWORK_UDS_DATAGRAM_MAX_SIZE__ 1500

/*--
this:AddWidget("Spinbox", 1, 128, "Max number of datagrams in buffer")
--*/
#define __NETWORK_UDS_DATAGRAM_BUFFERS_SIZE__ 16

#endif /* _UDS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
