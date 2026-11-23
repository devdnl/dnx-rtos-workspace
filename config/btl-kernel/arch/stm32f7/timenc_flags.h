/*==============================================================================
File    timenc_flags.h

Author  Daniel Zorychta

Brief   Timer driven AB encoder

        MIT License

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
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as prefix
 *       and suffix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _TIMENC_FLAGS_H_
#define _TIMENC_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > TIMENC", "The TIMENC module configuration.")
++*/

/*--
this:AddWidget("Spinbox", 100, 100000, "Sample frequency [Hz]")
--*/
#define __TIMENC_FREQ_HZ__ 500

/*--
this:AddWidget("Spinbox", 1, 256, "Maximal number of encoders")
--*/
#define __TIMENC_MAX_ENCODERS__ 8

#endif /* _TIMENC_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
