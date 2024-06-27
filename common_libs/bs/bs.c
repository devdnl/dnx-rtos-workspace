/*==============================================================================
File     bs.c

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

/*==============================================================================
  Include files
==============================================================================*/
#include "bs.h"
#include <dnx/misc.h>
#include <stdlib.h>
#include <string.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define bs_get_template(_type_)                                                          \
        _type_ bs_get_##_type_(bs_rd_t *self, const char *key, _type_ Default, int *err) \
        {                                                                                \
                typeof(Default) value = Default;                                         \
                const void     *ptr;                                                     \
                int e = bs_get_raw_with_type_check(self, key, &ptr, sizeof(value),       \
                        bs_type__##_type_);                                              \
                if (not e) {                                                             \
                        value = *cast(typeof(value) *, ptr);                             \
                }                                                                        \
                if (err) {                                                               \
                        if (e) {                                                         \
                                value = Default;                                         \
                        }                                                                \
                        *err = e;                                                        \
                }                                                                        \
                return value;                                                            \
        }

#define bs_set_template(_type_)                                                         \
        int bs_set_##_type_(bs_wr_t *self, const char *key, _type_ value)               \
        {                                                                               \
                return bs_set_raw(self, key, &value, sizeof(value), bs_type__##_type_); \
        }

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        bs_type__bool,
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

typedef struct PACKED {
        uint8_t  type;
        uint8_t  key_len;
        uint16_t data_len;
        uint8_t  key_data[];
} item_t;

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
 * @brief  Get raw data blob.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  blob         pointer to blob result pointer
 * @param  len          size of blob
 * @param  type         type of blob
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int bs_get_raw(bs_rd_t *self, const char *key, const void **blob, size_t *len,
        bs_type_t *type)
{
        if (not self or not key or not blob or not len or not type) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_len) {
                return bs_err_invalid_value;
        }

        item_t *item = cast(typeof(item), self->chunk);

        while (cast(uintptr_t, item) < cast(uintptr_t, self->chunk) + self->chunk_len) {

                if (strcmp(key, cast(char *, item->key_data)) == 0) {
                        *type = item->type;
                        *len  = item->data_len;
                        *blob = &item->key_data[item->key_len];
                        return 0;

                } else {
                        item = cast(typeof(item),
                                cast(uint8_t *, item) + sizeof(*item) + item->key_len +
                                        item->data_len);
                }
        }

        return bs_err_value_not_found;
}

//==============================================================================
/**
 * @brief  Get raw data blob.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  blob         pointer to blob result pointer
 * @param  len          size of blob
 * @param  type         type of blob
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int bs_get_raw_with_type_check(bs_rd_t *self, const char *key, const void **blob,
        size_t len, bs_type_t type)
{
        size_t    blen;
        bs_type_t btype;

        int err = bs_get_raw(self, key, blob, &blen, &btype);
        if (err) {
                return err;
        }

        if (type != btype) {
                return bs_err_unexpected_type;
        }

        if (len != blen) {
                return bs_err_value_size;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Add raw item.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  blob         blob
 * @param  len          blob length
 * @param  type         blob type
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int bs_set_raw(bs_wr_t *self, const char *key, const void *blob, size_t len,
        bs_type_t type)
{
        if (not self or not key or not blob or not len) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_max_len) {
                return bs_err_invalid_value;
        }

        if (not self->item) {
                return bs_err_invalid_value;
        }

        const void *b;
        size_t      l;
        bs_type_t   t;
        bs_rd_t     bs;
        bs.chunk     = self->chunk;
        bs.chunk_len = self->chunk_len;
        if (bs_get_raw(&bs, key, &b, &l, &t) == 0) {
                return bs_err_exist;
        }

        size_t key_len  = strlen(key) + 1;
        size_t item_len = sizeof(item_t) + key_len + len;

        if ((cast(uintptr_t, self->item) + item_len) >=
                (cast(uintptr_t, self->chunk) + self->chunk_max_len)) {
                return bs_err_chunk_full;
        }

        item_t *item   = self->item;
        item->data_len = len;
        item->key_len  = key_len;
        item->type     = type;
        memcpy(&item->key_data[0], key, key_len);
        memcpy(&item->key_data[key_len], blob, len);

        self->item = cast(uint8_t *, self->item) + item_len;
        self->chunk_len += item_len;

        return 0;
}

//==============================================================================
/**
 * @brief  Set bs to read chunk.
 *
 * @param  self         bs instance
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_read_chunk(bs_rd_t *self, const uint8_t *chunk, size_t chunk_len)
{
        if (not self) {
                return bs_err_invalid_value;
        }

        if (not chunk or not chunk_len or (chunk_len < sizeof(item_t))) {
                return bs_err_invalid_value;
        }

        self->chunk     = chunk;
        self->chunk_len = chunk_len;

        return 0;
}

//==============================================================================
/**
 * @brief  Get i8 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(int8_t);

//==============================================================================
/**
 * @brief  Get u8 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(uint8_t);

//==============================================================================
/**
 * @brief  Get i16 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(int16_t);

//==============================================================================
/**
 * @brief  Get u16 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(uint16_t);

//==============================================================================
/**
 * @brief  Get i32 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(int32_t);

//==============================================================================
/**
 * @brief  Get u32 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(uint32_t);

//==============================================================================
/**
 * @brief  Get i64 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(int64_t);

//==============================================================================
/**
 * @brief  Get u64 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(uint64_t);

//==============================================================================
/**
 * @brief  Get float type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(float);

//==============================================================================
/**
 * @brief  Get double type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found
 * @param  err          error value (optional)
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(double);

//==============================================================================
/**
 * @brief  Get bool type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  value        value
 * @param  Default      default value if not found
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_get_template(bool);

//==============================================================================
/**
 * @brief  Get string type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  string       pointer to string
 *
 * @return On success 0 is returned.
 */
//==============================================================================
const char *bs_get_str(bs_rd_t *self, const char *key, const char *Default, int *err)
{
        size_t      blen;
        const void *str = Default;
        bs_type_t   btype;

        int e = bs_get_raw(self, key, &str, &blen, &btype);
        if (not e) {
                if (bs_type__string != btype) {
                        e = bs_err_unexpected_type;

                } else if (blen == 0) {
                        e = bs_err_value_size;
                }
        } else if (err) {
                str  = Default;
                *err = e;
        }

        return str;
}

//==============================================================================
/**
 * @brief  Get blob type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  blob         pointer to blob
 * @param  len          pointer to blob length
 *
 * @return On success 0 is returned.
 */
//==============================================================================
const void *bs_get_blob(bs_rd_t *self, const char *key, size_t *len, int *err)
{
        if (not len) {
                if (err) {
                        *err = bs_err_invalid_value;
                }

                return NULL;
        }

        const void *blob = NULL;
        bs_type_t   btype;

        int e = bs_get_raw(self, key, &blob, len, &btype);
        if (not e) {
                if (bs_type__blob != btype) {
                        e = bs_err_unexpected_type;
                }
        } else if (err) {
                blob = NULL;
                *err = e;
        }

        return blob;
}

//==============================================================================
/**
 * @brief  Prepare serializer to write.
 *
 * @param  self         bs instance
 * @param  chunk        chunk
 * @param  chunk_len    chunk length
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_write_chunk(bs_wr_t *self, uint8_t *chunk, size_t chunk_len)
{
        if (not self) {
                return bs_err_invalid_value;
        }

        if (not chunk or not chunk_len or (chunk_len < sizeof(item_t))) {
                return bs_err_invalid_value;
        }

        self->chunk         = chunk;
        self->chunk_max_len = chunk_len;
        self->chunk_len     = 0;
        self->item          = chunk;

        return 0;
}

//==============================================================================
/**
 * @brief  Return total chunk length.
 *
 * @param  self         bs instance
 *
 * @return Return total chunk length.
 */
//==============================================================================
int bs_get_chunk_length(bs_wr_t *self)
{
        if (not self) {
                return 0;
        }

        return self->chunk_len;
}

//==============================================================================
/**
 * @brief  Add i8 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(int8_t);

//==============================================================================
/**
 * @brief  Add u8 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(uint8_t);

//==============================================================================
/**
 * @brief  Add i16 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(int16_t);

//==============================================================================
/**
 * @brief  Add u16 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(uint16_t);

//==============================================================================
/**
 * @brief  Add i32 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(int32_t);

//==============================================================================
/**
 * @brief  Add u32 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(uint32_t);

//==============================================================================
/**
 * @brief  Add i64 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(int64_t);

//==============================================================================
/**
 * @brief  Add u64 to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(uint64_t);

//==============================================================================
/**
 * @brief  Add float to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(float);

//==============================================================================
/**
 * @brief  Add double to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(double);

//==============================================================================
/**
 * @brief  Add bool to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
bs_set_template(bool);

//==============================================================================
/**
 * @brief  Add string to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_set_str(bs_wr_t *self, const char *key, const char *str)
{
        return bs_set_raw(self, key, str, strlen(str) + 1, bs_type__string);
}

//==============================================================================
/**
 * @brief  Add blob to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_set_blob(bs_wr_t *self, const char *key, const void *blob, size_t len)
{
        return bs_set_raw(self, key, blob, len, bs_type__blob);
}

/*==============================================================================
  End of file
==============================================================================*/
