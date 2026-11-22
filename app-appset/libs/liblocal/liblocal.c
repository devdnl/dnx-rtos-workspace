/*==============================================================================
@file    liblocal.c

@author  Daniel Zorychta

@brief   Local library example.

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

/*==============================================================================
 Include files
 =============================================================================*/
#include "liblocal.h"
#include <dnx/os.h>
#include <errno.h>
#include <dnx/os.h>
#include <dnx/misc.h>
#include <stdio.h>
#include <stdlib.h>

/*==============================================================================
 Local macros
==============================================================================*/

/*==============================================================================
 Local object types
 =============================================================================*/
struct liblocal {
        bool initialized;
};

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
 * Creates new liblocal context.
 *
 * @return Object or NULL in case of error.
 */
//==============================================================================
liblocal_t *liblocal__new(void)
{
        liblocal_t *self = calloc(sizeof(*self), 1);
        if (not self) {
                return NULL;
        }

        self->initialized = true;

        return self;
}

//==============================================================================
/**
 * Releases context memory.
 *
 * @param self                  context of the library
 */
//==============================================================================
void liblocal__delete(liblocal_t *self)
{
        if (not self) {
                return;
        }

        self->initialized = false;

        free(self);
}

/*==============================================================================
  End of file
==============================================================================*/
