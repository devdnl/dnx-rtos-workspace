/*==============================================================================
@file    i2c_flags.h

@author  Daniel Zorychta

@brief   I2C module configuration flags (POSIX simulator).

@note    MIT License

         Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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

#ifndef _I2C_FLAGS_H_
#define _I2C_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > I2C", "The I2C module configuration.")
++*/

/*--
this:AddExtraWidget("Label", "I2C_POSIX_INFO", "Buses I2C1..I2C4 (major 0..3, master mode only) are connected to the board model (simulator option: --board <script>), without the board model buses are empty.")
--*/

#endif /* _I2C_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
