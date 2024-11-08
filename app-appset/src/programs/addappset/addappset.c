/*==============================================================================
@file    addappset.c

@author  Daniel Zorychta

@brief   Add appset to system register.

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


==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dnx/misc.h>
#include <dnx/os.h>

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
GLOBAL_VARIABLES_SECTION {
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(addappset, STACK_DEPTH_LOW);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * Main program function.
 *
 * Note: Please adjust stack size according to programs needs.
 *
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc < 2) {
                printf("Usage: %s <appset-address-1> [appset-address-2] [...]\n", argv[0]);
                return EXIT_FAILURE;

        } else  {
                for (int i = 1; i < argc; i++) {
                        const void *appset[] = {cast(void *, strtoul(argv[i], NULL, 0))};

                        if (register_appset(appset, ARRAY_SIZE(appset)) == 0) {
                                printf("Appset @ %p successfully added.\n", appset[0]);
                        } else {
                                printf("Unable to add appset @ %p.\n", appset[0]);
                        }
                }

                return EXIT_SUCCESS;
        }
}

/*==============================================================================
  End of file
==============================================================================*/
