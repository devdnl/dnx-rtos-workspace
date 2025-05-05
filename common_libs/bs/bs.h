/*==============================================================================
File     bs.h

Author   Daniel Zorychta

Brief    Binary serialization.

         Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the  Free Software  Foundation;  either version 2 of the License, or
         any later version.

         This  program  is  distributed  in the hope that  it will be useful,
         but  WITHOUT  ANY  WARRANTY;  without  even  the implied warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         You  should  have received a copy  of the GNU General Public License
         along  with  this  program;  if not,  write  to  the  Free  Software
         Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================*/

/**
@defgroup BS_BS_H_ BS_BS_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct {
        const uint8_t *chunk;
        size_t         chunk_len;
} bs_rd_t;

typedef struct {
        uint8_t *chunk;
        void    *item;
        size_t   chunk_len;
        size_t   chunk_max_len;
} bs_wr_t;

typedef enum {
        bs_err_ok,
        bs_err_invalid_value,
        bs_err_unexpected_type,
        bs_err_value_size,
        bs_err_value_not_found,
        bs_err_chunk_full,
        bs_err_exist,
} bs_err_t;

typedef enum {
        bs_type__unknown = -1,
        bs_type__bool    = 0,
        bs_type__int8_t,
        bs_type__uint8_t,
        bs_type__int16_t,
        bs_type__uint16_t,
        bs_type__int32_t,
        bs_type__uint32_t,
        bs_type__int64_t,
        bs_type__uint64_t,
        bs_type__float,
        bs_type__double,
        bs_type__string,
        bs_type__blob,
} bs_type_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
/*
 * Read methods.
 */
extern int         bs_read_chunk(bs_rd_t *self, const uint8_t *chunk, size_t chunk_len);
extern bs_rd_t     bs_read_init(const uint8_t *chunk, size_t chunk_len);
extern int8_t      bs_get_int8_t(bs_rd_t *self, const char *key, int8_t df, int *err);
extern uint8_t     bs_get_uint8_t(bs_rd_t *self, const char *key, uint8_t df, int *err);
extern int16_t     bs_get_int16_t(bs_rd_t *self, const char *key, int16_t df, int *err);
extern uint16_t    bs_get_uint16_t(bs_rd_t *self, const char *key, uint16_t df, int *err);
extern int32_t     bs_get_int32_t(bs_rd_t *self, const char *key, int32_t df, int *err);
extern uint32_t    bs_get_uint32_t(bs_rd_t *self, const char *key, uint32_t df, int *err);
extern int64_t     bs_get_int64_t(bs_rd_t *self, const char *key, int64_t df, int *err);
extern uint64_t    bs_get_uint64_t(bs_rd_t *self, const char *key, uint64_t df, int *err);
extern float       bs_get_float(bs_rd_t *self, const char *key, float df, int *err);
extern double      bs_get_double(bs_rd_t *self, const char *key, double df, int *err);
extern bool        bs_get_bool(bs_rd_t *self, const char *key, bool df, int *err);
extern const char *bs_get_str(bs_rd_t *self, const char *key, const char *df, int *err);
extern const void *bs_get_blob(bs_rd_t *self, const char *key, size_t *len, int *err);
extern bs_type_t   bs_get_type(bs_rd_t *self, const char *key);
extern int bs_get_next(bs_rd_t *self, uint8_t **next, const char **key, const void **value, bs_type_t *type);

/*
 * Write methods.
 */
extern int      bs_write_chunk(bs_wr_t *self, uint8_t *chunk, size_t chunk_len);
extern bs_wr_t  bs_write_init(uint8_t *chunk, size_t chunk_len);
extern int      bs_get_chunk_length(bs_wr_t *self);
extern uint8_t *bs_get_chunk_data(bs_wr_t *self);
extern int      bs_set_int8_t(bs_wr_t *self, const char *key, int8_t value);
extern int      bs_set_uint8_t(bs_wr_t *self, const char *key, uint8_t value);
extern int      bs_set_int16_t(bs_wr_t *self, const char *key, int16_t value);
extern int      bs_set_uint16_t(bs_wr_t *self, const char *key, uint16_t value);
extern int      bs_set_int32_t(bs_wr_t *self, const char *key, int32_t value);
extern int      bs_set_uint32_t(bs_wr_t *self, const char *key, uint32_t value);
extern int      bs_set_int64_t(bs_wr_t *self, const char *key, int64_t value);
extern int      bs_set_uint64_t(bs_wr_t *self, const char *key, uint64_t value);
extern int      bs_set_float(bs_wr_t *self, const char *key, float value);
extern int      bs_set_double(bs_wr_t *self, const char *key, double value);
extern int      bs_set_str(bs_wr_t *self, const char *key, const char *str);
extern int      bs_set_str_len(bs_wr_t *self, const char *key, const char *str, size_t str_len);
extern int      bs_set_bool(bs_wr_t *self, const char *key, bool value);
extern int      bs_set_blob(bs_wr_t *self, const char *key, const void *blob, size_t len);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
