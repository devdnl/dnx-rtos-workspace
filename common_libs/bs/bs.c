/*==============================================================================
File    bs.c

Author  Daniel Zorychta

Brief   Binary serialization.

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
        _type_ bs_get_##_type_(bs_t *self, const char *key, _type_ Default, int *err) \
        {                                                                                \
                size_t blen;                                                             \
                const void *blob;                                                        \
                bs_type_t rettype = bs_type__##_type_;                                   \
                bs_type_t btype;                                                         \
                _type_ value = 0;                                                        \
                int e = bs_get_raw(self, key, &blob, &blen, &btype);                     \
                if (not e) {                                                             \
                        union {                                                          \
                                bool boolean;                                            \
                                int8_t int8;                                             \
                                uint8_t uint8;                                           \
                                int16_t int16;                                           \
                                uint16_t uint16;                                         \
                                int32_t int32;                                           \
                                uint32_t uint32;                                         \
                                int64_t int64;                                           \
                                uint64_t uint64;                                         \
                                float float32;                                           \
                                double float64;                                          \
                        } v;                                                             \
                        memcpy(&v, blob, min(blen, sizeof(v)));                          \
                        if (btype == bs_type__bool) {                                    \
                                value = v.boolean;                                       \
                        } else if (btype == bs_type__int8_t) {                           \
                                value = v.int8;                                          \
                        } else if (btype == bs_type__uint8_t) {                          \
                                value = v.uint8;                                         \
                        } else if (btype == bs_type__int16_t) {                          \
                                value = v.int16;                                         \
                        } else if (btype == bs_type__uint16_t) {                         \
                                value = v.uint16;                                        \
                        } else if (btype == bs_type__int32_t) {                          \
                                value = v.int32;                                         \
                        } else if (btype == bs_type__uint32_t) {                         \
                                value = v.uint32;                                        \
                        } else if (btype == bs_type__int64_t) {                          \
                                value = v.int64;                                         \
                        } else if (btype == bs_type__uint64_t) {                         \
                                value = v.uint64;                                        \
                        } else if (btype == bs_type__float) {                            \
                                value = v.float32;                                       \
                        } else if (btype == bs_type__double) {                           \
                                value = v.float64;                                       \
                        } else if (btype == bs_type__string) {                           \
                                if (rettype == bs_type__bool) {                          \
                                        value = ((strcasecmp(blob, "true") == 0) ||      \
                                                (strcasecmp(blob, "on") == 0) ||         \
                                                (strcasecmp(blob, "yes") == 0) ||        \
                                                (strcasecmp(blob, "enable") == 0) ||     \
                                                (strcasecmp(blob, "set") == 0) ||        \
                                                (strcasecmp(blob, "high") == 0) ||       \
                                                (strcasecmp(blob, "1") == 0));           \
                                } else if (rettype == bs_type__float) {                  \
                                        value = atof(blob);                              \
                                } else if (rettype == bs_type__double) {                 \
                                        value = atof(blob);                              \
                                } else if (rettype == bs_type__blob) {                   \
                                        value = 0;                                       \
                                } else {                                                 \
                                        value = atol(blob);                              \
                                }                                                        \
                        } else {                                                         \
                                value = Default;                                         \
                        }                                                                \
                }                                                                        \
                if (e) {                                                                 \
                        value = Default;                                                 \
                }                                                                        \
                if (err) {                                                               \
                        *err = e;                                                        \
                }                                                                        \
                return value;                                                            \
        }

#define bs_set_template(_type_)                                                         \
        int bs_set_##_type_(bs_t *self, const char *key, _type_ value)               \
        {                                                                               \
                return bs_set_raw(self, key, &value, sizeof(value), bs_type__##_type_); \
        }

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct PACKED {
        uint8_t type;
        uint8_t key_len;
        uint16_t data_len;
        uint8_t key_data[];
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
static int bs_get_raw(bs_t *self, const char *key, const void **blob, size_t *len, bs_type_t *type)
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
                        *len = item->data_len;
                        *blob = &item->key_data[item->key_len];
                        return 0;

                } else {
                        item = cast(typeof(item),
                                cast(uint8_t *, item) + sizeof(*item) + item->key_len + item->data_len);
                }
        }

        return bs_err_value_not_found;
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
static int bs_set_raw(bs_t *self, const char *key, const void *blob, size_t len, bs_type_t type)
{
        if (not self or not key or not blob) {
                return bs_err_invalid_value;
        }

        if ((type != bs_type__string) and (len == 0)) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_max_len) {
                return bs_err_invalid_value;
        }

        if (not self->item) {
                return bs_err_invalid_value;
        }

        const void *b;
        size_t l;
        bs_type_t t;
        bs_t bs;
        bs.chunk = self->chunk;
        bs.chunk_len = self->chunk_len;
        if (bs_get_raw(&bs, key, &b, &l, &t) == 0) {
                return bs_err_exist;
        }

        size_t key_len = strlen(key) + 1;
        size_t item_len = sizeof(item_t) + key_len + len + ((type == bs_type__string) ? 1 : 0);

        if ((cast(uintptr_t, self->item) + item_len) >=
                (cast(uintptr_t, self->chunk) + self->chunk_max_len)) {
                return bs_err_chunk_full;
        }

        item_t *item = self->item;
        item->data_len = len + ((type == bs_type__string) ? 1 : 0);
        item->key_len = key_len;
        item->type = type;
        memcpy(&item->key_data[0], key, key_len);
        memcpy(&item->key_data[key_len], blob, len);

        if (type == bs_type__string) {
                item->key_data[key_len + len] = '\0';
        }

        self->item = cast(uint8_t *, self->item) + item_len;
        self->chunk_len += item_len;

        return 0;
}

//==============================================================================
/**
 * @brief  Set bs to read chunk (unsafe).
 *
 * @param  chunk        chunk buffer
 * @param  chunk_len    chunk buffer length
 *
 * @return Initialized bs read.
 */
//==============================================================================
bs_t bs_read_init(uint8_t *chunk, size_t chunk_len)
{
        bs_t bsrd = {
                .chunk = chunk,
                .item = chunk,
                .chunk_len = chunk_len,
                .chunk_max_len = chunk_len,
        };

        return bsrd;
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
const char *bs_get_str(bs_t *self, const char *key, const char *Default, int *err)
{
        size_t blen;
        const void *str = Default;
        bs_type_t btype;

        int e = bs_get_raw(self, key, &str, &blen, &btype);
        if (not e) {
                if (bs_type__string != btype) {
                        e = bs_err_unexpected_type;

                } else if (blen == 0) {
                        e = bs_err_value_size;
                }
        }

        if (e) {
                str = Default;
        }

        if (err) {
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
const void *bs_get_blob(bs_t *self, const char *key, size_t *len, int *err)
{
        if (not len) {
                if (err) {
                        *err = bs_err_invalid_value;
                }

                return NULL;
        }

        const void *blob = NULL;
        bs_type_t btype;

        int e = bs_get_raw(self, key, &blob, len, &btype);

        if (err) {
                *err = e;
        }

        return blob;
}

//==============================================================================
/**
 * @brief  Get key type.
 *
 * @param  self         bs instance
 * @param  key          key to find
 *
 * @return Key type.
 */
//==============================================================================
bs_type_t bs_get_type(bs_t *self, const char *key)
{
        const void *blob;
        bs_type_t btype;
        size_t blen;

        if (bs_get_raw(self, key, &blob, &blen, &btype) != 0) {
                return bs_type__unknown;
        } else {
                return btype;
        }
}

//==============================================================================
/**
 * @brief  Return next key-value pair.
 *
 * @param  self         bs instance
 * @param  next         next pointer (if points to NULL then from beginning)
 * @param  key          key name
 * @param  value        value data
 * @param  value_len    value length
 * @param  type         value type
 *
 * @return On sucecss 0 is returned.
 */
//==============================================================================
int bs_get_next(bs_t *self, uint8_t **next, const char **key, const void **value, uint16_t *value_len,
        bs_type_t *type)
{
        if (not self or not key or not value or not value_len or not type) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_len) {
                return bs_err_invalid_value;
        }

        if (*next == NULL) {
                *next = cast(uint8_t *, self->chunk);
        }

        item_t *item = cast(typeof(item), *next);

        if (cast(uintptr_t, item) < cast(uintptr_t, self->chunk) + self->chunk_len) {

                *type = item->type;
                *key = cast(const char *, &item->key_data[0]);
                *value = &item->key_data[item->key_len];
                *value_len = item->data_len;
                *next = cast(uint8_t *, item) + sizeof(*item) + item->key_len + item->data_len;

                return 0;
        }

        return bs_err_value_not_found;
}

//==============================================================================
/**
 * @brief  Prepare serializer to write (unsafe).
 *
 * @param  chunk        chunk
 * @param  chunk_len    chunk length
 *
 * @return Initialized bs write.
 */
//==============================================================================
bs_t bs_write_init(uint8_t *chunk, size_t chunk_len)
{
        bs_t bswr = {
                .chunk = chunk,
                .item = chunk,
                .chunk_max_len = chunk_len,
                .chunk_len = 0,
        };

        return bswr;
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
int bs_get_chunk_length(bs_t *self)
{
        if (not self) {
                return 0;
        }

        return self->chunk_len;
}

//==============================================================================
/**
 * @brief  Return chunk data pointer.
 *
 * @param  self         bs instance
 *
 * @return Return chunk data pointer.
 */
//==============================================================================
uint8_t *bs_get_chunk_data(bs_t *self)
{
        if (not self) {
                return NULL;
        }

        return self->chunk;
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
 * @param  str          string
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_set_str(bs_t *self, const char *key, const char *str)
{
        return bs_set_raw(self, key, str, strlen(str), bs_type__string);
}

//==============================================================================
/**
 * @brief  Add string to buffer.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  str          string
 * @param  str_len      string length
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_set_str_len(bs_t *self, const char *key, const char *str, size_t str_len)
{
        return bs_set_raw(self, key, str, str_len, bs_type__string);
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
int bs_set_blob(bs_t *self, const char *key, const void *blob, size_t len)
{
        return bs_set_raw(self, key, blob, len, bs_type__blob);
}

/*==============================================================================
  End of file
==============================================================================*/
