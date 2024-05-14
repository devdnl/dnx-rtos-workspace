/*=========================================================================*//**
@file    appset.c

@author  Daniel Zorychta

@brief   Appset functions.

@note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

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


*//*==========================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include "common.h"

/*==============================================================================
  Local macros
==============================================================================*/

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
 * @brief  Register new appset(s).
 *
 * @param  appset_array         appset vectors array
 * @param  appset_count         number of vectors
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int register_appset(const void **appset_array, size_t appset_count)
{
        int err = _libc_syscall(_LIBC_SYS_REGISTERAPPSET, appset_array, &appset_count);
        return err ? -1 : 0;
}

/*==============================================================================
  End of file
==============================================================================*/
