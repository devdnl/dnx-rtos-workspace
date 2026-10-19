/*==============================================================================
File    fpkg.h

Author  Daniel Zorychta

Brief   File package container.

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
/*
 * Return values of fpkg__readdir() (other functions return 0 or -1).
 */
#define FPKG_OK                 0       /* item returned */
#define FPKG_ERR_ARG            (-1)    /* invalid argument */
#define FPKG_ERR_IO             (-2)    /* read error or corrupted image */
#define FPKG_ERR_NOMEM          (-3)    /* out of memory */
#define FPKG_END_OF_DIR         (-5)    /* no more items */

/*
 * Notes:
 *  - Paths are absolute ("/dir/file"); trailing '/' is ignored, "/" is root.
 *  - Name in fpkg_dirent_t is valid until next fpkg__readdir()/fpkg__dirclose().
 *  - Library is not thread safe: all objects created from one fpkg_t share
 *    file and buffer of that container; use one fpkg_t per thread or lock.
 *  - Directory and file objects must be closed before container is closed.
 *  - "Encryption" is an obfuscation only (no key, all seeds are in the image).
 */

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct fpkg fpkg_t;

typedef struct {
        const char *name;
        uint32_t size;
        bool dir;
} fpkg_dirent_t;

typedef enum {
        fpkg_seek_mode__set,
        fpkg_seek_mode__cur,
        fpkg_seek_mode__end,
} fpkg_seek_mode_t;

typedef struct {
        uint32_t size;
        bool dir;
} fpkg_stat_t;

typedef struct fpkg_dir fpkg_dir_t;
typedef struct fpkg_file fpkg_file_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern fpkg_t *fpkg__open(const char *path, bool validate);
extern fpkg_t *fpkg__close(fpkg_t *fpkg);
extern int fpkg__get_number_of_files(fpkg_t *fpkg);
extern bool fpkg__is_valid(fpkg_t *fpkg);
extern fpkg_dir_t *fpkg__diropen(fpkg_t *fpkg, const char *dirname);
extern fpkg_dir_t *fpkg__dirclose(fpkg_dir_t *dir);
extern int fpkg__readdir(fpkg_dir_t *dir, fpkg_dirent_t *entry);
extern void fpkg__rewinddir(fpkg_dir_t *dir);
extern fpkg_file_t *fpkg__fopen(fpkg_t *fpkg, const char *filename);
extern int fpkg__fseek(fpkg_file_t *file, int32_t seek, fpkg_seek_mode_t mode);
extern int32_t fpkg__ftell(fpkg_file_t *file);
extern size_t fpkg__fread(fpkg_file_t *file, void *dst, uint32_t count);
extern int fpkg__stat(fpkg_t *fpkg, const char *filename, fpkg_stat_t *stat);
extern int fpkg__fstat(fpkg_file_t *file, fpkg_stat_t *stat);
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
