/*==============================================================================
@file   assert.h

@author Daniel Zorychta

@brief  This file provides assert macro.

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
\defgroup assert-h <assert.h>

The library provides assert macro.

*/
/**@{*/

#ifndef _ASSERT_H_
#define _ASSERT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>

/*==============================================================================
  Exported macros
==============================================================================*/
//==============================================================================
/**
 * @brief Macro creates assertion functionality.
 *
 * Aborts the program if user-defined condition is not <b>true</b>. Macro can be
 * disabled if <b>NDEBUG</b> macro is defined.
 *
 * @param ignore        user-defined condition
 *
 * @b Example @b 1
 * @code
        #include <assert.h>

        // ...

        void some_function(int *number)
        {
                assert(number != NULL);      // pointer to number must be not NULL

                // ...
        }

        // ...
   @endcode
 *
 * @b Example @b 2
 * @code
         #define NDEBUG
         #include <assert.h>

         // all assert macros are disabled

         assert(false); // has no effect for program flow

         // ...
   @endcode
 */
//==============================================================================
#if !defined(NDEBUG)
#   ifdef DOXYGEN
#       define assert(ignore)
#   else
#       define assert(ignore) do{if ((ignore)) break; printf("Assertion failed: %s, file %s, line %d\n", #ignore, __FILE__, __LINE__); abort();}while(0)
#   endif
#endif

#ifndef DOXYGEN
#define static_assert(_expr, _msg) _Static_assert(_expr, _msg)
#endif

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

#endif /* _ASSERT_H_ */

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
