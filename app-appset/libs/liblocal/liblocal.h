/*==============================================================================
@file    liblocal.h

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 Include files
==============================================================================*/
#include <stdint.h>
#include <stdbool.h>

/*==============================================================================
 Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct liblocal liblocal_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
 Exported functions
==============================================================================*/
extern liblocal_t *liblocal__new(void);
extern void liblocal__delete(liblocal_t *self);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif
