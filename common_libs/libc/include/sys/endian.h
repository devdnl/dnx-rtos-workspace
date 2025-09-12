/*==============================================================================
@file   endian.h

@author Daniel Zorychta

@brief  CPU endianness.

        MIT License

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

/**
\defgroup sys-endian-h <sys/endian.h>

The library is used to check CPU endianness.

*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
/**
 * @brief Little endian switch
 */
#define LITTLE_ENDIAN           0xAABB

/**
 * @brief Big endian switch
 */
#define BIG_ENDIAN              0xCCDD

/**
 * @brief Macro determines current CPU endianness
 *
 * Only one of defined byte orders can be used in the same time:
 * @ref LITTLE_ENDIAN or @ref BIG_ENDIAN.
 *
 * @b Example
 * @code
         #if BYTE_ORDER == LITTLE_ENDIAN
                 // code for little endian CPU

         #elif BYTE_ORDER == BIG_ENDIAN
                 // code for big endian CPU

         #else
                 #error "Unknown CPU endianness"
         #endif
   @endcode
 */
#define BYTE_ORDER              LITTLE_ENDIAN

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
