/*==============================================================================
@file    spi_flags.h

@author  Daniel Zorychta

@brief   SPI module configuration flags (POSIX simulator).

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

#ifndef _SPI_FLAGS_H_
#define _SPI_FLAGS_H_

/*--
this:SetTitle("Home > Microcontroller > SPI", "The SPI module configuration.")
++*/

/*--
this:AddExtraWidget("Label", "SPI_POSIX_INFO", "Buses SPI1..SPI4 (major 0..3, master mode) are connected to the board model (simulator option: --board <script>), device is selected by the CS pin of the GPIO driver. Without the board model received data are 0xFF.")
--*/

#endif /* _SPI_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
