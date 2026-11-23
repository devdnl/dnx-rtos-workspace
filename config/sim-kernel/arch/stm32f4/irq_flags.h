/*==============================================================================
File    irq_flags.h

Author  Daniel Zorychta

Brief   This driver support external interrupts (EXTI).

        MIT License

        Copyright (C) 2018 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _IRQ_FLAGS_H_
#define _IRQ_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > IRQ", "The IRQ module configuration.")

this.AddIrqWidget = function(this, irqNum)
    this:AddWidget("Combobox", "IRQ "..irqNum)
    this:AddItem("Disabled", "_IRQ_MODE_DISABLED")
    this:AddItem("Trigger on falling edge", "_IRQ_MODE_FALLING_EDGE")
    this:AddItem("Trigger on rising edge", "_IRQ_MODE_RISING_EDGE")
    this:AddItem("Trigger on both edges", "_IRQ_MODE_FALLING_AND_RISING_EDGE")
end

this.AddExtiWidget = function(this, extiNum)
    this:AddWidget("Combobox", "EXTI"..extiNum.." port")
    local shift = (extiNum & 0x3) * 4
    for i = 0, 10 do
        local c = string.char(65 + i)
        this:AddItem("P"..c..extiNum, "("..tostring(i).." << "..shift..")")
    end
end

++*/

/*
 * Interrupts Default Configuration
 * 0: IRQ DISABLED                              (_IRQ_MODE_DISABLED)
 * 1: TRIGGER ON FALLING EDGE                   (_IRQ_MODE_FALLING_EDGE)
 * 2: TRIGGER ON RISING EDGE                    (_IRQ_MODE_RISING_EDGE)
 * 3: TRIGGER ON FALLING AND RISING EDGE        (_IRQ_MODE_FALLING_AND_RISING_EDGE)
 */
/*--
this:AddIrqWidget(0)
--*/
#define __IRQ_LINE_0_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(1)
--*/
#define __IRQ_LINE_1_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(2)
--*/
#define __IRQ_LINE_2_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(3)
--*/
#define __IRQ_LINE_3_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(4)
--*/
#define __IRQ_LINE_4_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(5)
--*/
#define __IRQ_LINE_5_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(6)
--*/
#define __IRQ_LINE_6_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(7)
--*/
#define __IRQ_LINE_7_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(8)
--*/
#define __IRQ_LINE_8_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(9)
--*/
#define __IRQ_LINE_9_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(10)
--*/
#define __IRQ_LINE_10_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(11)
--*/
#define __IRQ_LINE_11_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(12)
--*/
#define __IRQ_LINE_12_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(13)
--*/
#define __IRQ_LINE_13_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(14)
--*/
#define __IRQ_LINE_14_MODE__ _IRQ_MODE_DISABLED

/*--
this:AddIrqWidget(15)
--*/
#define __IRQ_LINE_15_MODE__ _IRQ_MODE_DISABLED


/*--
this:AddExtraWidget("Label", "EXTI_GAP", "")
this:AddExtiWidget(0)
--*/
#define __IRQ_EXTI0_PORT__ (0 << 0)

/*--
this:AddExtiWidget(1)
--*/
#define __IRQ_EXTI1_PORT__ (0 << 4)

/*--
this:AddExtiWidget(2)
--*/
#define __IRQ_EXTI2_PORT__ (0 << 8)

/*--
this:AddExtiWidget(3)
--*/
#define __IRQ_EXTI3_PORT__ (0 << 12)

/*--
this:AddExtiWidget(4)
--*/
#define __IRQ_EXTI4_PORT__ (0 << 0)

/*--
this:AddExtiWidget(5)
--*/
#define __IRQ_EXTI5_PORT__ (0 << 4)

/*--
this:AddExtiWidget(6)
--*/
#define __IRQ_EXTI6_PORT__ (0 << 8)

/*--
this:AddExtiWidget(7)
--*/
#define __IRQ_EXTI7_PORT__ (0 << 12)

/*--
this:AddExtiWidget(8)
--*/
#define __IRQ_EXTI8_PORT__ (0 << 0)

/*--
this:AddExtiWidget(9)
--*/
#define __IRQ_EXTI9_PORT__ (0 << 4)

/*--
this:AddExtiWidget(10)
--*/
#define __IRQ_EXTI10_PORT__ (0 << 8)

/*--
this:AddExtiWidget(11)
--*/
#define __IRQ_EXTI11_PORT__ (0 << 12)

/*--
this:AddExtiWidget(12)
--*/
#define __IRQ_EXTI12_PORT__ (0 << 0)

/*--
this:AddExtiWidget(13)
--*/
#define __IRQ_EXTI13_PORT__ (0 << 4)

/*--
this:AddExtiWidget(14)
--*/
#define __IRQ_EXTI14_PORT__ (0 << 8)

/*--
this:AddExtiWidget(15)
--*/
#define __IRQ_EXTI15_PORT__ (0 << 12)

#endif /* _IRQ_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
