/*==============================================================================
File    fpkg.h

Author  Daniel Zorychta

Brief   File package container.

        Copyright (C) 2021 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*==============================================================================
  Include files
==============================================================================*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct fpkg fpkg_t;

typedef struct {
        const char *name;
        uint32_t    size;
        bool        dir;
} fpkg_dirent_t;

typedef enum {
        fpkg_seek_mode__set,
        fpkg_seek_mode__cur,
        fpkg_seek_mode__end,
} fpkg_seek_mode_t;

typedef struct {
        uint32_t size;
} fpkg_stat_t;

typedef struct fpkg_dir  fpkg_dir_t;
typedef struct fpkg_file fpkg_file_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern fpkg_t      *fpkg__open(const char *path, bool validate);
extern fpkg_t      *fpkg__close(fpkg_t *fpkg);
extern int          fpkg__get_number_of_files(fpkg_t *fpkg);
extern bool         fpkg__is_valid(fpkg_t *fpkg);
extern fpkg_dir_t  *fpkg__diropen(fpkg_t *fpkg, const char *dirname);
extern fpkg_dir_t  *fpkg__dirclose(fpkg_dir_t *dir);
extern int          fpkg__readdir(fpkg_dir_t *dir, fpkg_dirent_t *entry);
extern fpkg_file_t *fpkg__fopen(fpkg_t *fpkg, const char *filename);
extern int          fpkg__fseek(fpkg_file_t *file, int32_t seek, fpkg_seek_mode_t mode);
extern size_t       fpkg__fread(fpkg_file_t *file, uint8_t *dst, uint32_t count);
extern int          fpkg__stat(fpkg_t *fpkg, const char *filename, fpkg_stat_t *stat);
extern int          fpkg__fstat(fpkg_file_t *file, fpkg_stat_t *stat);
extern fpkg_file_t *fpkg__fclose(fpkg_file_t *file);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/*==============================================================================
  End of file
==============================================================================*/
