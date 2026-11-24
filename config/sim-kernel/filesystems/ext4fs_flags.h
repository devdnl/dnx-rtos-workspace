/*==============================================================================
@file    ext4fs_flags.h

@author  Daniel Zorychta

@brief   EXT4 file system

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
 * NOTE: All flags defined as: __FLAG_NAME__ (with doubled underscore as prefix
 *       and suffix) are exported to the single configuration file
 *       (by using Configtool) when entire project configuration is exported.
 *       All other flag definitions and statements are ignored.
 */

#ifndef _EXT4FS_FLAGS_H_
#define _EXT4FS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > ext4fs", "The ext4fs file system configuration.")
++*/

/*--
this:AddWidget("Combobox", "EXT feature set")
this:AddItem("EXT2", "2")
this:AddItem("EXT3", "3")
this:AddItem("EXT4", "4")
--*/
#define __EXT4FS_CFG_FEATURE__ 4

/*--
this:AddWidget("Combobox", "Journaling")
this:AddItem("Disable", "0")
this:AddItem("Enable", "1")
--*/
#define __EXT4FS_CFG_JOURNALING__ 1

/*--
this:AddWidget("Spinbox", 1, 128, "Number of cache blocks")
this:SetToolTip("This value determines how many blocks is cached. The 4 blocks "..
                "is good enough to get nice read/write speed. Single cache "..
                "allocate the size of block (at least 1 KiB).")
--*/
#define __EXT4FS_CFG_BLK_CACHE_SIZE__ 32

/*--
this:AddWidget("Combobox", "Write buffer strategy")
this:AddItem("Write Through", "0")
this:AddItem("Write Back", "1")
--*/
#define __EXT4FS_CFG_WR_BUF_STRATEGY__ 0

#endif /* _EXT4FS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
