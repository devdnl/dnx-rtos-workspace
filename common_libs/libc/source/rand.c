/*==============================================================================
File    rand.c

Author  Daniel Zorychta

Brief   Pseudo-random generator functions.

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

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define _rand_seed      (((_libc_app_ctx_t*)(_libc_appctx_get()))->_rand_seed)

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Function returns a pseudo-random integer.
 *
 * The rand() function returns a pseudo-random integer in the range 0 to
 * @ref RAND_MAX inclusive (i.e., the mathematical range [0, RAND_MAX]).
 *
 * @return The rand() function return a value between @b 0 and @ref RAND_MAX (inclusive).
 *
 * @b Example
 * @code
        // ...
        int foo = rand();
        // ...
   @endcode
 *
 * @see RAND_MAX
 */
//==============================================================================
int rand_r(unsigned int *seed)
{
        const u64_t a = 1103515245;
        const u32_t c = 12345;
        const u64_t m = (1ULL << 32);

        *seed = (a * *seed + c) % m;

        return *seed;
}

//==============================================================================
/**
 * @brief Function returns a pseudo-random integer.
 *
 * The rand() function returns a pseudo-random integer in the range 0 to
 * @ref RAND_MAX inclusive (i.e., the mathematical range [0, RAND_MAX]).
 *
 * @return The rand() function return a value between @b 0 and @ref RAND_MAX (inclusive).
 *
 * @b Example
 * @code
        // ...
        int foo = rand();
        // ...
   @endcode
 *
 * @see RAND_MAX
 */
//==============================================================================
int rand(void)
{
        return rand_r(&_rand_seed);
}

//==============================================================================
/**
 * @brief Function sets the seed for rand() function.
 *
 * The srand() function sets its argument as the seed for a new sequence
 * of pseudo-random integers to be returned by rand().  These sequences
 * are repeatable by calling srand() with the same seed value.
 *
 * @param seed      new seed value
 *
 * @b Example
 * @code
        // ...
        #include <stdlib.h>
        #include <dnx/os.h>

        srand(get_tick_counter());
        int foo = rand();
        // ...
   @endcode
 */
//==============================================================================
void srand(unsigned int s)
{
        _rand_seed = s;
}

/*==============================================================================
  End of file
==============================================================================*/
