/*==============================================================================
@file    ramfs_flags.h

@author  Daniel Zorychta

@brief   RAM file system configuration.

@note    MIT License

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

#ifndef _RAMFS_FLAGS_H_
#define _RAMFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > ramfs", "The ramfs file system configuration.")
++*/

/*--
this:AddWidget("Spinbox", 8, 4096, "File chain size (bytes)")
--*/
#define __RAMFS_FILE_CHAIN_SIZE__ 64

#endif /* _RAMFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
