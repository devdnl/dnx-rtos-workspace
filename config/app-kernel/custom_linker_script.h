/*==============================================================================
@file    custom_linker_script.h

@author  Daniel Zorychta

@brief   Custom linker script configuration.

@note    MIT License

         Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*
 * NOTE: Purpose of this file is to generate custom linker script instead of
 *       including that file into the project.
 */

#define __ARCH__ stm32h7
#define __MEMORY_1__ "flash   (rx)  : org = 0x08000000, len = 512k"
#define __MEMORY_2__ "axisram (rwx) : org = 0x24000000, len = 512k"
#define __MEMORY_3__ "sram1   (rwx) : org = 0x30000000, len = 128k"
#define __MEMORY_4__ "sram2   (rwx) : org = 0x30020000, len = 128k"
#define __MEMORY_5__ "sram3   (rwx) : org = 0x30040000, len = 32k"
#define __MEMORY_6__ "sram4   (rwx) : org = 0x38000000, len = 64k"
#define __MEMORY_7__ "itcm    (rwx) : org = 0x00000000, len = 64k"
#define __MEMORY_8__ "dtcm    (rw)  : org = 0x20000000, len = 128k"
#define __MEMORY_9__ "bkp     (rw)  : org = 0x38800000, len = 4k"
#define __MEMORY_10__ ""
#define __MEMORY_11__ ""
#define __MEMORY_12__ ""
#define __MEMORY_13__ ""
#define __MEMORY_14__ ""
#define __MEMORY_15__ ""
#define __MEMORY_16__ ""

/*==============================================================================
  End of file
==============================================================================*/
