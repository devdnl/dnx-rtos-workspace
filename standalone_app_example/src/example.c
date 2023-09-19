/*=========================================================================*//**
@file    sync.c

@author  Daniel Zorychta

@brief   Synchronize buffer of all file systems

@note    Copyright (C) 2015 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
static int  result = 1000;
static u8_t buffer[4096];

/*==============================================================================
  Exported object definitions
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        printf("Hello world from RAM!\n");

        for (int i = 0; i < argc; i++) {
                printf("%u: '%s'\n", i, argv[i]);

                if (i > 0) {
                        result += atoi(argv[i]);
                }
        }

        printf("Result is: %d/%Xh\n", result, result);

        printf("Result address: %p\n", &result);
        printf("Buffer address: %p\n", buffer);

        memset(buffer, 0xAA, sizeof(buffer));

        puts("Sleeping...");
        sleep(5);
        puts("Bye!");

        return -200;
}

/*==============================================================================
  End of file
==============================================================================*/
