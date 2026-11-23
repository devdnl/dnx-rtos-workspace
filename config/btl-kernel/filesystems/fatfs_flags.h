/*==============================================================================
@file    fatfs_flags.h

@author  Daniel Zorychta

@brief   FAT file system configuration.

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

#ifndef _FATFS_FLAGS_H_
#define _FATFS_FLAGS_H_

/*--
this:SetTitle("Home > File Systems > fatfs", "The fatfs file system configuration.")
++*/

/*--
this:AddWidget("Combobox", "Code page")
this:AddItem("U.S. (437)",                        "437" )
this:AddItem("Arabic (720)",                      "720" )
this:AddItem("Greek (737)",                       "737" )
this:AddItem("KBL (771)",                         "771" )
this:AddItem("Baltic (775)",                      "775" )
this:AddItem("Latin 1 (850)",                     "850" )
this:AddItem("Latin 2 (852)",                     "852" )
this:AddItem("Cyrillic (855)",                    "855" )
this:AddItem("Turkish (857)",                     "857" )
this:AddItem("Portuguese (860)",                  "860" )
this:AddItem("Icelandic (861)",                   "861" )
this:AddItem("Hebrew (862)",                      "862" )
this:AddItem("Canadian French (863)",             "863" )
this:AddItem("Arabic (864)",                      "864" )
this:AddItem("Nordic (865)",                      "865" )
this:AddItem("Russian (866)",                     "866" )
this:AddItem("Greek 2 (869)",                     "869" )
this:AddItem("Japanese (DBCS) (932)",             "932" )
this:AddItem("Simplified Chinese (DBCS) (936)",   "936" )
this:AddItem("Korean (DBCS) (949)",               "949" )
this:AddItem("Traditional Chinese (DBCS) (950)",  "950" )
--*/
#define __FATFS_LFN_CODEPAGE__ 852

/*--
this:AddWidget("Combobox", "Buffered files (fast)")
this:AddItem("Yes", "_YES_")
this:AddItem("No", "_NO_")
this:SetToolTip("When option is disabled, file system uses the sector buffer in the file system "..
                "object instead of the sector buffer in the individual file object for file "..
                "data transfer. This reduces memory consumption 512 bytes each file object.")
--*/
#define __FATFS_BUFFERED_FILE_ENABLE__ _YES_

#endif /* _FATFS_FLAGS_H_ */
/*==============================================================================
  End of file
==============================================================================*/
