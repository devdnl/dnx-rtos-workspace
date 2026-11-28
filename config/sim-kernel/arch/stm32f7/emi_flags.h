/*==============================================================================
File     emi_flags.h

Author   Daniel Zorychta

Brief    External Memory Interface

         MIT License

         Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#define __EMI_SDRAM_RPIPE__ 0

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
this:AddWidget("Spinbox", 1, 200, "SDRAM: Clock frequency (SDCLK) [MHz]")
this:SetToolTip("Frequency of the SDRAM clock: FMC clock divided by SDRAM clock period "..
                "(round up). It is used to calculate timings given in ns.")
--*/
#define __EMI_SDRAM_FREQ_MHZ__ 108

/*--
this:AddWidget("Spinbox", 1, 1000, "SDRAM: Row precharge delay (tRP) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]. Round up fractional values. "..
                "Number of clock cycles is calculated from the SDRAM clock frequency.")
--*/
#define __EMI_SDRAM_TRP_NS__ 18

/*--
this:AddWidget("Spinbox", 1, 1000, "SDRAM: Row cycle delay (tRC) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]: the greater of tRC and tRFC. "..
                "Round up fractional values.")
--*/
#define __EMI_SDRAM_TRC_NS__ 60

/*--
this:AddWidget("Spinbox", 1, 3, "SDRAM: CAS Latency (CAS)")
--*/
#define __EMI_SDRAM_CAS__ 3

/*--
this:AddWidget("Spinbox", 1, 1000, "SDRAM: Self refresh time (tRAS) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]. Round up fractional values. "..
                "Number of clock cycles is calculated from the SDRAM clock frequency.")
--*/
#define __EMI_SDRAM_TRAS_NS__ 42

/*--
this:AddWidget("Spinbox", 1, 1000, "SDRAM: Exit Self-refresh delay (tXSR) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]. Round up fractional values. "..
                "Number of clock cycles is calculated from the SDRAM clock frequency.")
--*/
#define __EMI_SDRAM_TXSR_NS__ 70

/*--
this:AddWidget("Spinbox", 0, 1000, "SDRAM: Write Recovery Time (tWR) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns] (tWR, tDPL or tRDL). Set 0 if "..
                "the datasheet gives the value only in clock cycles.")
--*/
#define __EMI_SDRAM_TWR_NS__ 12

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Write Recovery Time (tWR) minimum [cycles]")
this:SetToolTip("Minimal number of cycles. The greater value of this and tWR [ns] is used. "..
                "The driver increases it when required by FMC (tRAS - tRCD, tRC - tRCD - tRP).")
--*/
#define __EMI_SDRAM_TWR_MIN_CLK__ 2

/*--
this:AddWidget("Spinbox", 1, 16, "SDRAM: Load to Active delay (tMRD) [cycles]")
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
this:AddWidget("Spinbox", 1, 1000, "SDRAM1: Row to column delay (tRCD) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]. Round up fractional values. "..
                "Number of clock cycles is calculated from the SDRAM clock frequency.")
--*/
#define __EMI_SDRAM_1_TRCD_NS__ 18

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
this:AddWidget("Spinbox", 1, 1000, "SDRAM2: Row to column delay (tRCD) [ns]")
this:SetToolTip("Value from the SDRAM datasheet [ns]. Round up fractional values. "..
                "Number of clock cycles is calculated from the SDRAM clock frequency.")
--*/
#define __EMI_SDRAM_2_TRCD_NS__ 18

/*--
this:AddWidget("Editline", true, "SDRAM2: Memory regions")
this:SetToolTip("Edit memory regions: [HEAP|NOHEAP]=<NAME>:<ADDRESS>:<SIZE>,... Example: HEAP=SDRAM:0xC0000000:8388608")
--*/
#define __EMI_SDRAM_2_REGIONS__ "HEAP=SDRAM:0xD0000000:8388608"

#endif /* _EMI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
