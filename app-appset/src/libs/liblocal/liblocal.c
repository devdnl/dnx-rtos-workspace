/*==============================================================================
@file    liblocal.c

@author  Daniel Zorychta

@brief   Local library example.

@note    Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


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
