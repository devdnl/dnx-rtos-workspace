/*==============================================================================
File    flatfs_flags.h

Author  daniel.zorychta@gmail.com

Brief   Daniel Zorychta

        MIT License

        Copyright (C) 2021 daniel.zorychta@gmail.com <>

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

#ifndef _FLATFS_FLAGS_H_
#define _FLATFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > flatfs", "The flatfs file system configuration.")
++*/

/*--
this:AddWidget("Combobox", "Enable file system debug")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_LOG_ENABLE__ _NO_

/*--
this:AddWidget("Spinbox", 0, 512, "Number of index entries")
--*/
#define __FLATFS_INDEX_SIZE__ 32

/*--
this:AddWidget("Combobox", "Enable option async (deferred update of file size)")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_ASYNC_ENABLE__ _NO_

/*--
this:AddWidget("Combobox", "Enable option cache=N (write-through block cache)")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
--*/
#define __FLATFS_CACHE_ENABLE__ _NO_

#endif /* _FLATFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
