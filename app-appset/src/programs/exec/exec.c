/*=========================================================================*//**
@file    exec

@author  Daniel Zorychta

@brief   Binary application executer.

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#include <dnx/os.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef int (*mainfn)(int argc, char *argv[]);

enum {
        ARCH_CORTEX_M,
        ARCH_CORTEX_M_THUMB,
        ARCH_COUNT,
};

enum {
        TYPE_EXECUTABLE,
        TYPE_DYNLIB,
        TYPE_COUNT,
};

typedef union {
        uint8_t bytes[128];

        struct {
                char     shebang[8]; // #!exec\n\0
                uint8_t  version;    // 1
                uint8_t  arch;
                uint8_t  type;
                uint8_t  flags;
                uint32_t bss_section_size;
        };
} dnx_app_header_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        dnx_app_header_t header;
};

static const char *EXEC_SHEBANG = "#!exec\n";

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(exec, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Cat main function
 *
 * @param  argc         argument count
 * @param  argv         argument values
 *
 * @return 0 on success.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        FILE *fp          = NULL;
        u8_t *app_mem     = NULL;
        int   exit_code   = EXIT_FAILURE;
        int   addr_offset = 0;

        fp = fopen(argv[1], "r");
        if (not fp) {
                perror(argv[1]);
                goto exit;
        }

        struct stat stat;
        if (fstat(fileno(fp), &stat) != 0) {
                perror(argv[1]);
                goto exit;
        }

        size_t n = fread(&global->header, 1, sizeof(global->header), fp);
        if (n != sizeof(global->header)) {
                perror(argv[1]);
                goto exit;
        }

        if (memcmp(EXEC_SHEBANG, global->header.shebang,
                    sizeof(global->header.shebang))) {
                printf("Invalid executable file.\n");
                goto exit;
        }

        if (global->header.version != 1) {
                printf("Not supported file version.\n");
                goto exit;
        }

        if (global->header.arch >= ARCH_COUNT) {
                printf("Not supported architecture.\n");
                goto exit;
        }

        if (global->header.type >= TYPE_COUNT) {
                printf("Unknown file type.\n");
                goto exit;
        }

        if (global->header.arch == ARCH_CORTEX_M_THUMB) {
                addr_offset = 1;
        }

        size_t app_mem_size =
                stat.st_size + global->header.bss_section_size - sizeof(global->header);

        app_mem = calloc(app_mem_size, 1);
        if (not app_mem) {
                perror(argv[1]);
                goto exit;
        }

        n = fread(app_mem, 1, (stat.st_size - sizeof(global->header)), fp);
        if (n != (stat.st_size - sizeof(global->header))) {
                perror(argv[1]);
                goto exit;
        }

        cache_flush();

        mainfn main = cast(mainfn, cast(uintptr_t, app_mem) + addr_offset);

        exit_code = main(argc - 1, &argv[1]);
exit:
        free(app_mem);
        fclose(fp);

        return exit_code;
}

/*==============================================================================
  End of file
==============================================================================*/
