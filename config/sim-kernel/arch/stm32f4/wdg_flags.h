/*==============================================================================
File     wdg_flags.h

Author   Daniel Zorychta

Brief    Watchdog driver configuration.

         MIT License

         Copyright (C) 2017 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _WDG_FLAGS_H_
#define _WDG_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > WDG", "The WDG module configuration.")

-- timeout to register
local T2R     = {}
T2R[#T2R + 1] = {TIME = "4 ms",   DIV = 4,   RELOAD = 32  }
T2R[#T2R + 1] = {TIME = "8 ms",   DIV = 4,   RELOAD = 64  }
T2R[#T2R + 1] = {TIME = "16 ms",  DIV = 4,   RELOAD = 128 }
T2R[#T2R + 1] = {TIME = "32 ms",  DIV = 4,   RELOAD = 256 }
T2R[#T2R + 1] = {TIME = "64 ms",  DIV = 8,   RELOAD = 256 }
T2R[#T2R + 1] = {TIME = "125 ms", DIV = 16,  RELOAD = 250 }
T2R[#T2R + 1] = {TIME = "250 ms", DIV = 32,  RELOAD = 250 }
T2R[#T2R + 1] = {TIME = "500 ms", DIV = 64,  RELOAD = 250 }
T2R[#T2R + 1] = {TIME = "1 s",    DIV = 256, RELOAD = 125 }
T2R[#T2R + 1] = {TIME = "2 s",    DIV = 256, RELOAD = 250 }
T2R[#T2R + 1] = {TIME = "5 s",    DIV = 256, RELOAD = 625 }
T2R[#T2R + 1] = {TIME = "10 s",   DIV = 256, RELOAD = 1250}
T2R[#T2R + 1] = {TIME = "15 s",   DIV = 256, RELOAD = 1875}
T2R[#T2R + 1] = {TIME = "20 s",   DIV = 256, RELOAD = 2500}
T2R[#T2R + 1] = {TIME = "25 s",   DIV = 256, RELOAD = 3125}
T2R[#T2R + 1] = {TIME = "30 s",   DIV = 256, RELOAD = 3750}

this.AddTimeouts = function(this, flagName)
    for i = 1, #T2R do
        this:AddItem(T2R[i].TIME, T2R[i].TIME, flagName)
    end
end

this.GetDivReload = function(this, time)
    for i = 1, #T2R do
        if T2R[i].TIME == time then
            return T2R[i].DIV, T2R[i].RELOAD
        end
    end
end

this.GetTime = function(this, div, reload)
    for i = 1, #T2R do
        if T2R[i].DIV == tonumber(div) and T2R[i].RELOAD == tonumber(reload) then
            return T2R[i].TIME
        end
    end

    return T2R[1].TIME
end
++*/

/*--
this:AddWidget("Combobox", "Lock device at open")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __WDG_DEVICE_LOCK_AT_OPEN__ _YES_

/*--
this:AddWidget("Combobox", "Disable Watchdog on debug")
this:AddItem("No", "_NO_")
this:AddItem("Yes", "_YES_")
--*/
#define __WDG_DISABLE_ON_DEBUG__ _YES_

/*--
this:AddWidget("Value")
--*/
#define __WDG_CLK_DIVIDER__ 256

/*--
this:AddWidget("Value")
--*/
#define __WDG_RELOAD_VALUE__ 625

/*--
-- widget with timeout selection
this:AddExtraWidget("Combobox", "ComboboxTimeout", "Watchdog timeout")
this:AddTimeouts("ComboboxTimeout")
this:SetEvent("clicked", "ComboboxTimeout",
    function()
        local selection   = this:GetFlagValue("ComboboxTimeout")
        local DIV, RELOAD = this:GetDivReload(selection)

        this:SetFlagValue("__WDG_CLK_DIVIDER__", tostring(DIV))
        this:SetFlagValue("__WDG_RELOAD_VALUE__", tostring(RELOAD))
    end
)

-- set timeout selection widget according to selected divider and reload value
local DIV    = this:GetFlagValue("__WDG_CLK_DIVIDER__")
local RELOAD = this:GetFlagValue("__WDG_RELOAD_VALUE__")
local time   = this:GetTime(DIV, RELOAD)
this:SetFlagValue("ComboboxTimeout", time)
++*/

#endif /* _WDG_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
