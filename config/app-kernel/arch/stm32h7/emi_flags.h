/*==============================================================================
File     emi_flags.h

Author   Daniel Zorychta

Brief    External Memory Interface

         Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the  Free Software  Foundation;  either version 2 of the License, or
         any later version.

         This  program  is  distributed  in the hope that  it will be useful,
         but  WITHOUT  ANY  WARRANTY;  without  even  the implied warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         You  should  have received a copy  of the GNU General Public License
         along  with  this  program;  if not,  write  to  the  Free  Software
         Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


==============================================================================*/

/*
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as prefix
 *       and suffix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _EMI_FLAGS_H_
#define _EMI_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > EMI", "The EMI module configuration.")
++*/

/*----------------------------------------------------------------------------*/
// BANK 1
/*----------------------------------------------------------------------------*/
/*--
++*/

/*----------------------------------------------------------------------------*/
// BANK 2
/*----------------------------------------------------------------------------*/
/*--
++*/

/*----------------------------------------------------------------------------*/
// BANK 3
/*----------------------------------------------------------------------------*/
/*--
++*/

/*----------------------------------------------------------------------------*/
// BANK 4
/*----------------------------------------------------------------------------*/
/*--
++*/

/*----------------------------------------------------------------------------*/
// BANK 5 & 6
/*----------------------------------------------------------------------------*/
/*--
this:AddWidget("Combobox", "SDRAM: Read pipe delay")
this:AddItem("No delay", "0")
this:AddItem("1 HCLK delay", "1")
this:AddItem("2 HCLK delay", "2")
--*/
#define __EMI_SDRAM_RPIPE__ 2

/*--
this:AddWidget("Combobox", "SDRAM: Burst read mode")
this:AddItem("Disable", "0")
this:AddItem("Enable", "1")
--*/
#define __EMI_SDRAM_RBURST__ 1

/*--
this:AddWidget("Combobox", "SDRAM: SDRAM clock period")
this:AddItem("SDCLK disabled", "0")
this:AddItem("2 x HCLK periods", "2")
this:AddItem("3 x HCLK periods", "3")
--*/
#define __EMI_SDRAM_SDCLK__ 2

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Row precharge delay (tRP)")
--*/
#define __EMI_SDRAM_TRP__ 2

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Row cycle delay (tRC)")
--*/
#define __EMI_SDRAM_TRC__ 7

/*--
this:AddWidget("Spinbox", 1, 3, "SDRAM: CAS Latency (CAS)")
--*/
#define __EMI_SDRAM_CAS__ 3

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Self refresh time (tRAS)")
--*/
#define __EMI_SDRAM_TRAS__ 4

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Exit Self-refresh delay (tXSR)")
--*/
#define __EMI_SDRAM_TXSR__ 7

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Write Recovery Time (tWR)")
--*/
#define __EMI_SDRAM_TWR__ 2

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Load to Active delay (tMRD)")
this:SetToolTip("Define the delay between a Load Mode Register command and an "..
                "Active or Refresh command.")
--*/
#define __EMI_SDRAM_TMRD__ 2

/*--
this:AddWidget("Spinbox", 1, 15, "SDRAM: Number of Auto-refresh")
--*/
#define __EMI_SDRAM_NRFS__ 8

/*--
this:AddWidget("Spinbox", 1, 128, "SDRAM: Refresh rate [ms]")
--*/
#define __EMI_SDRAM_REFRESH_RATE_MS__ 64

/*----------------------------------------------------------------------------*/
/*--
this:AddWidget("Combobox", "SDRAM1: Memory bank")
this:AddItem("Disable", "0")
this:AddItem("Enable", "1")
--*/
#define __EMI_SDRAM_1_ENABLE__ 0

/*--
this:AddWidget("Combobox", "SDRAM1: Number of internal banks")
this:AddItem("2 internal banks", "0")
this:AddItem("4 internal banks", "1")
--*/
#define __EMI_SDRAM_1_NB__ 1

/*--
this:AddWidget("Combobox", "SDRAM1: Data bus width")
this:AddItem("8 bits", "0")
this:AddItem("16 bits", "1")
this:AddItem("32 bits", "2")
--*/
#define __EMI_SDRAM_1_MWID__ 1

/*--
this:AddWidget("Combobox", "SDRAM1: Number of row address bits")
this:AddItem("11 bits", "0")
this:AddItem("12 bits", "1")
this:AddItem("13 bits", "2")
--*/
#define __EMI_SDRAM_1_NR__ 1

/*--
this:AddWidget("Combobox", "SDRAM1: Number of column address bits")
this:AddItem("8 bits", "0")
this:AddItem("9 bits", "1")
this:AddItem("10 bits", "2")
this:AddItem("11 bits", "3")
--*/
#define __EMI_SDRAM_1_NC__ 0

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM1: Row to column delay (tRCD)")
--*/
#define __EMI_SDRAM_1_TRCD__ 2

/*--
this:AddWidget("Editline", true, "SDRAM1: Memory regions")
this:SetToolTip("Edit memory regions: [HEAP|NOHEAP]=<NAME>:<ADDRESS>:<SIZE>,... Example: HEAP=SDRAM:0xC0000000:8388608")
--*/
#define __EMI_SDRAM_1_REGIONS__ "HEAP=SDRAM:0xC0000000:8388608"

/*----------------------------------------------------------------------------*/
/*--
this:AddWidget("Combobox", "SDRAM2: Memory bank")
this:AddItem("Disable", "0")
this:AddItem("Enable", "1")
--*/
#define __EMI_SDRAM_2_ENABLE__ 1

/*--
this:AddWidget("Combobox", "SDRAM2: Number of internal banks")
this:AddItem("2 internal banks", "0")
this:AddItem("4 internal banks", "1")
--*/
#define __EMI_SDRAM_2_NB__ 1

/*--
this:AddWidget("Combobox", "SDRAM2: Data bus width")
this:AddItem("8 bits", "0")
this:AddItem("16 bits", "1")
this:AddItem("32 bits", "2")
--*/
#define __EMI_SDRAM_2_MWID__ 2

/*--
this:AddWidget("Combobox", "SDRAM2: Number of row address bits")
this:AddItem("11 bits", "0")
this:AddItem("12 bits", "1")
this:AddItem("13 bits", "2")
--*/
#define __EMI_SDRAM_2_NR__ 1

/*--
this:AddWidget("Combobox", "SDRAM2: Number of column address bits")
this:AddItem("8 bits", "0")
this:AddItem("9 bits", "1")
this:AddItem("10 bits", "2")
this:AddItem("11 bits", "3")
--*/
#define __EMI_SDRAM_2_NC__ 1

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM2: Row to column delay (tRCD)")
--*/
#define __EMI_SDRAM_2_TRCD__ 2

/*--
this:AddWidget("Editline", true, "SDRAM2: Memory regions")
this:SetToolTip("Edit memory regions: [HEAP|NOHEAP]=<NAME>:<ADDRESS>:<SIZE>,... Example: HEAP=SDRAM:0xC0000000:8388608")
--*/
#define __EMI_SDRAM_2_REGIONS__ "HEAP=SDRAM:0xD0000000:524288,HEAP=SDRAM:0xD0080000:524288,HEAP=SDRAM:0xD0100000:1048576,HEAP=SDRAM:0xD0200000:2097152,HEAP=SDRAM:0xD0400000:4194304,HEAP=SDRAM:0xD0800000:8388608,HEAP=SDRAM:0xD1000000:16777216"

#endif /* _EMI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
