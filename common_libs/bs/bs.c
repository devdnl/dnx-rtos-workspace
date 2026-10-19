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
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define BS_CLASS_INT  0
#define BS_CLASS_BOOL 1
#define BS_CLASS_REAL 2

#define IS_FINITE(_d_) (((_d_) <= DBL_MAX) and ((_d_) >= -DBL_MAX))

/*
 * Getter template. Stored value is decoded to canonical form (int64/uint64/
 * double) and then converted to requested type. Conversion of a floating
 * point value to an integer type is range-checked (out of range is UB in C).
 * Integer to integer conversion keeps C semantic (truncation).
 */
#define bs_get_template(_type_, _class_, _min_, _max_)                                                \
        _type_ bs_get_##_type_(bs_t *self, const char *key, _type_ Default, int *err)                 \
        {                                                                                             \
                bs_num_t n;                                                                           \
                _type_ value = Default;                                                               \
                int e = bs_get_num(self, key, bs_type__##_type_, &n);                                 \
                if (not e) {                                                                          \
                        if (n.kind == bs_num__sint) {                                                 \
                                value = cast(_type_, n.v.s);                                          \
                        } else if (n.kind == bs_num__uint) {                                          \
                                value = cast(_type_, n.v.u);                                          \
                        } else if ((_class_) == BS_CLASS_INT) {                                       \
                                if ((n.v.d >= cast(double, _min_)) and                                \
                                        (n.v.d < (cast(double, _max_) + 1.0))) {                      \
                                        value = cast(_type_, n.v.d);                                  \
                                } else {                                                              \
                                        e = bs_err_out_of_range;                                      \
                                }                                                                     \
                        } else if ((_class_) == BS_CLASS_REAL) {                                      \
                                if (IS_FINITE(n.v.d) and ((n.v.d > (_max_)) or (n.v.d < -(_max_)))) { \
                                        e = bs_err_out_of_range;                                      \
                                } else {                                                              \
                                        value = cast(_type_, n.v.d);                                  \
                                }                                                                     \
                        } else {                                                                      \
                                value = cast(_type_, n.v.d);                                          \
                        }                                                                             \
                }                                                                                     \
                if (err) {                                                                            \
                        *err = e;                                                                     \
                }                                                                                     \
                return value;                                                                         \
        }

#define bs_set_template(_type_)                                                         \
        int bs_set_##_type_(bs_t *self, const char *key, _type_ value)                  \
        {                                                                               \
                return bs_set_raw(self, key, &value, sizeof(value), bs_type__##_type_); \
        }

#define bs_decode(_type_, _field_, _kind_)        \
        {                                         \
                _type_ x;                         \
                if (blen != sizeof(x)) {          \
                        return bs_err_value_size; \
                }                                 \
                memcpy(&x, blob, sizeof(x));      \
                num->kind = _kind_;               \
                num->v._field_ = x;               \
                return 0;                         \
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

typedef enum {
        bs_num__sint,
        bs_num__uint,
        bs_num__real,
} bs_num_kind_t;

typedef struct {
        bs_num_kind_t kind;
        union {
                int64_t s;
                uint64_t u;
                double d;
        } v;
} bs_num_t;

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
 * @brief  Validate item located at p.
 *
 * @param  p            item address
 * @param  end          end of chunk
 * @param  total        total item length (output)
 *
 * @return On success 0 is returned, otherwise bs_err_corrupted.
 */
//==============================================================================
static int bs_item_check(const uint8_t *p, const uint8_t *end, size_t *total)
{
        size_t avail = cast(size_t, end - p);

        if (avail < sizeof(item_t)) {
                return bs_err_corrupted;
        }

        const item_t *item = cast(const item_t *, p);
        size_t len = sizeof(item_t) + item->key_len + item->data_len;

        if ((item->key_len == 0) or (len > avail)) {
                return bs_err_corrupted;
        }

        if (item->key_data[item->key_len - 1] != '\0') {
                return bs_err_corrupted;
        }

        if (item->type == bs_type__string) {
                if ((item->data_len == 0) or (item->key_data[item->key_len + item->data_len - 1] != '\0')) {
                        return bs_err_corrupted;
                }
        }

        *total = len;

        return 0;
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
static int bs_get_raw(bs_t *self, const char *key, const void **blob, size_t *len, bs_type_t *type)
{
        if (not self or not key or not blob or not len or not type) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_len) {
                return bs_err_invalid_value;
        }

        const uint8_t *p = self->chunk;
        const uint8_t *end = self->chunk + self->chunk_len;

        while (p < end) {
                size_t total;
                int err = bs_item_check(p, end, &total);
                if (err) {
                        return err;
                }

                const item_t *item = cast(const item_t *, p);

                if (strcmp(key, cast(const char *, item->key_data)) == 0) {
                        *type = cast(bs_type_t, item->type);
                        *len = item->data_len;
                        *blob = &item->key_data[item->key_len];
                        return 0;
                }

                p += total;
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
        if (not self or not key) {
                return bs_err_invalid_value;
        }

        if (not blob and (len > 0)) {
                return bs_err_invalid_value;
        }

        if ((type != bs_type__string) and (len == 0)) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_max_len or not self->item) {
                return bs_err_invalid_value;
        }

        // write cursor must be consistent with chunk length
        size_t used = cast(uintptr_t, self->item) - cast(uintptr_t, self->chunk);
        if ((used != self->chunk_len) or (self->chunk_len > self->chunk_max_len)) {
                return bs_err_invalid_value;
        }

        if (self->chunk_len > 0) {
                const void *b;
                size_t l;
                bs_type_t t;
                int err = bs_get_raw(self, key, &b, &l, &t);
                if (err == 0) {
                        return bs_err_exist;
                } else if (err != bs_err_value_not_found) {
                        return err;
                }
        }

        size_t str_term = (type == bs_type__string) ? 1 : 0;
        size_t key_len = strlen(key) + 1;

        if (key_len > UINT8_MAX) {
                return bs_err_value_size;
        }

        if (len > (UINT16_MAX - str_term)) {
                return bs_err_value_size;
        }

        size_t data_len = len + str_term;
        size_t item_len = sizeof(item_t) + key_len + data_len;

        if (item_len > (self->chunk_max_len - used)) {
                return bs_err_chunk_full;
        }

        item_t *item = self->item;
        item->type = cast(uint8_t, type);
        item->key_len = cast(uint8_t, key_len);
        item->data_len = cast(uint16_t, data_len);
        memcpy(&item->key_data[0], key, key_len);

        if (len > 0) {
                memcpy(&item->key_data[key_len], blob, len);
        }

        if (str_term) {
                item->key_data[key_len + len] = '\0';
        }

        self->item = cast(uint8_t *, self->item) + item_len;
        self->chunk_len += item_len;

        return 0;
}

//==============================================================================
/**
 * @brief  Convert string to canonical number.
 *
 *         Semantic compatible with atol()/atof(): leading white spaces are
 *         skipped, characters after the number are ignored, so a floating
 *         point string requested as integer gives its integer part
 *         ("3.7" -> 3, "-3.9" -> -3, "1e3" -> 1, as atol()). Values are parsed
 *         as 64-bit (no truncation on 32-bit targets); overflow of 64-bit
 *         range gives bs_err_out_of_range. A string without any number gives
 *         bs_err_unexpected_type.
 *
 * @param  str          string (NUL terminated)
 * @param  rettype      requested type
 * @param  num          result
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int bs_str_to_num(const char *str, bs_type_t rettype, bs_num_t *num)
{
        if (rettype == bs_type__bool) {
                num->kind = bs_num__uint;
                num->v.u = ((strcasecmp(str, "true") == 0) or (strcasecmp(str, "on") == 0) or
                        (strcasecmp(str, "yes") == 0) or (strcasecmp(str, "enable") == 0) or
                        (strcasecmp(str, "set") == 0) or (strcasecmp(str, "high") == 0) or
                        (strcasecmp(str, "1") == 0));
                return 0;
        }

        const char *s = str;
        while (isspace(cast(unsigned char, *s))) {
                s++;
        }

        char *end = NULL;
        int e = 0;
        errno = 0;

        if ((rettype == bs_type__float) or (rettype == bs_type__double)) {
                double d = strtod(s, &end);
                if ((errno == ERANGE) and not IS_FINITE(d)) {
                        e = bs_err_out_of_range;
                }
                num->kind = bs_num__real;
                num->v.d = d;

        } else if (((rettype == bs_type__uint8_t) or (rettype == bs_type__uint16_t) or
                           (rettype == bs_type__uint32_t) or (rettype == bs_type__uint64_t)) and
                (*s != '-')) {
                u64_t u = strtoull(s, &end, 10);
                if (errno == ERANGE) {
                        e = bs_err_out_of_range;
                }
                num->kind = bs_num__uint;
                num->v.u = u;

        } else {
                i64_t x = strtoll(s, &end, 10);
                if (errno == ERANGE) {
                        e = bs_err_out_of_range;
                }
                num->kind = bs_num__sint;
                num->v.s = x;
        }

        /*
         * Integer requested: integer part is parsed (as atol() does), so
         * "3.7" -> 3, "-3.9" -> -3. Strings without leading digit like ".5"
         * or "-.5" are parsed as double and truncated toward zero.
         */
        if ((num->kind != bs_num__real) and (end == s)) {
                char *dend = NULL;
                errno = 0;
                double d = strtod(s, &dend);

                if (dend > s) {
                        end = dend;
                        if ((d > -9223372036854775808.0) and (d < 9223372036854775808.0)) {
                                num->kind = bs_num__sint;
                                num->v.s = cast(int64_t, d);
                                e = 0;
                        } else {
                                e = bs_err_out_of_range;
                        }
                }
        }

        if (end == s) {
                return bs_err_unexpected_type;
        }

        return e;
}

//==============================================================================
/**
 * @brief  Get value and decode to canonical number.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  rettype      requested type
 * @param  num          result
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int bs_get_num(bs_t *self, const char *key, bs_type_t rettype, bs_num_t *num)
{
        const void *blob;
        size_t blen;
        bs_type_t btype;

        int e = bs_get_raw(self, key, &blob, &blen, &btype);
        if (e) {
                return e;
        }

        switch (btype) {
        case bs_type__bool:
                if (blen != sizeof(uint8_t)) {
                        return bs_err_value_size;
                }
                num->kind = bs_num__uint;
                num->v.u = (*cast(const uint8_t *, blob) != 0);
                return 0;

        case bs_type__int8_t:
                bs_decode(int8_t, s, bs_num__sint);
        case bs_type__uint8_t:
                bs_decode(uint8_t, u, bs_num__uint);
        case bs_type__int16_t:
                bs_decode(int16_t, s, bs_num__sint);
        case bs_type__uint16_t:
                bs_decode(uint16_t, u, bs_num__uint);
        case bs_type__int32_t:
                bs_decode(int32_t, s, bs_num__sint);
        case bs_type__uint32_t:
                bs_decode(uint32_t, u, bs_num__uint);
        case bs_type__int64_t:
                bs_decode(int64_t, s, bs_num__sint);
        case bs_type__uint64_t:
                bs_decode(uint64_t, u, bs_num__uint);
        case bs_type__float:
                bs_decode(float, d, bs_num__real);
        case bs_type__double:
                bs_decode(double, d, bs_num__real);

        case bs_type__string:
                return bs_str_to_num(blob, rettype, num);

        default:
                return bs_err_unexpected_type;
        }
}

//==============================================================================
/**
 * @brief  Set bs to read chunk. Write operations on this context are not
 *         possible (bs_err_chunk_full is returned).
 *
 * @param  chunk        chunk buffer
 * @param  chunk_len    exact length of serialized data
 *
 * @return Initialized bs read.
 */
//==============================================================================
bs_t bs_read_init(uint8_t *chunk, size_t chunk_len)
{
        bs_t bsrd = {
                .chunk = chunk,
                .item = chunk ? chunk + chunk_len : NULL,
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
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(int8_t, BS_CLASS_INT, INT8_MIN, INT8_MAX);

//==============================================================================
/**
 * @brief  Get u8 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(uint8_t, BS_CLASS_INT, 0, UINT8_MAX);

//==============================================================================
/**
 * @brief  Get i16 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(int16_t, BS_CLASS_INT, INT16_MIN, INT16_MAX);

//==============================================================================
/**
 * @brief  Get u16 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(uint16_t, BS_CLASS_INT, 0, UINT16_MAX);

//==============================================================================
/**
 * @brief  Get i32 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(int32_t, BS_CLASS_INT, INT32_MIN, INT32_MAX);

//==============================================================================
/**
 * @brief  Get u32 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(uint32_t, BS_CLASS_INT, 0, UINT32_MAX);

//==============================================================================
/**
 * @brief  Get i64 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(int64_t, BS_CLASS_INT, INT64_MIN, INT64_MAX);

//==============================================================================
/**
 * @brief  Get u64 type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(uint64_t, BS_CLASS_INT, 0, UINT64_MAX);

//==============================================================================
/**
 * @brief  Get float type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(float, BS_CLASS_REAL, -FLT_MAX, FLT_MAX);

//==============================================================================
/**
 * @brief  Get double type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(double, BS_CLASS_REAL, -DBL_MAX, DBL_MAX);

//==============================================================================
/**
 * @brief  Get bool type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return Value or Default on error.
 */
//==============================================================================
bs_get_template(bool, BS_CLASS_BOOL, 0, 1);

//==============================================================================
/**
 * @brief  Get string type value.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  Default      default value if not found or on error
 * @param  err          error value (optional)
 *
 * @return String or Default on error.
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
                }
                // NUL termination is guaranteed by bs_item_check()
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
 * @brief  Get raw value of any type (no type check). Returned pointer
 *         may be unaligned.
 *
 * @param  self         bs instance
 * @param  key          key to find
 * @param  len          pointer to blob length (set to 0 on error)
 * @param  err          error value (optional)
 *
 * @return Pointer to data or NULL on error.
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
        if (e) {
                blob = NULL;
                *len = 0;
        }

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
 * @return Key type or bs_type__unknown.
 */
//==============================================================================
bs_type_t bs_get_type(bs_t *self, const char *key)
{
        const void *blob;
        bs_type_t btype;
        size_t blen;

        if (bs_get_raw(self, key, &blob, &blen, &btype) != 0) {
                return bs_type__unknown;
        }

        if ((btype < bs_type__bool) or (btype > bs_type__blob)) {
                return bs_type__unknown;
        }

        return btype;
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
 * @return On success 0 is returned. At end of chunk bs_err_value_not_found
 *         is returned.
 */
//==============================================================================
int bs_get_next(bs_t *self, uint8_t **next, const char **key, const void **value, uint16_t *value_len,
        bs_type_t *type)
{
        if (not self or not next or not key or not value or not value_len or not type) {
                return bs_err_invalid_value;
        }

        if (not self->chunk or not self->chunk_len) {
                return bs_err_invalid_value;
        }

        if (*next == NULL) {
                *next = self->chunk;
        }

        uintptr_t start = cast(uintptr_t, self->chunk);
        uintptr_t end = start + self->chunk_len;
        uintptr_t pos = cast(uintptr_t, *next);

        if ((pos < start) or (pos > end)) {
                return bs_err_invalid_value;
        }

        if (pos == end) {
                return bs_err_value_not_found;
        }

        size_t total;
        int err = bs_item_check(*next, self->chunk + self->chunk_len, &total);
        if (err) {
                return err;
        }

        const item_t *item = cast(const item_t *, *next);

        *type = cast(bs_type_t, item->type);
        *key = cast(const char *, &item->key_data[0]);
        *value = &item->key_data[item->key_len];
        *value_len = item->data_len;
        *next += total;

        return 0;
}

//==============================================================================
/**
 * @brief  Prepare serializer to write.
 *
 * @param  chunk        chunk
 * @param  chunk_len    chunk buffer capacity
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

        return cast(int, self->chunk_len);
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
 * @brief  Add bool to buffer (stored as 1 byte: 0 or 1).
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  value        value
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int bs_set_bool(bs_t *self, const char *key, bool value)
{
        uint8_t v = value ? 1 : 0;
        return bs_set_raw(self, key, &v, sizeof(v), bs_type__bool);
}

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
        if (not str) {
                return bs_err_invalid_value;
        }

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
 * @brief  Add blob to buffer. Empty blob (len == 0) is not allowed.
 *
 * @param  self         bs instance
 * @param  key          key
 * @param  blob         blob
 * @param  len          blob length
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
