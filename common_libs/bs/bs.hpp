/*==============================================================================
File    bs.hpp

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

/**
@defgroup BS_BS_HPP_ BS_BS_HPP_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include "bs.h"

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
class bs {
private:
        bs_t ctx{};

public:
        bs() = default;

        void read_init(uint8_t *chunk, size_t chunk_len)
        {
                this->ctx = bs_read_init(chunk, chunk_len);
        }

        void write_init(uint8_t *chunk, size_t chunk_len)
        {
                this->ctx = bs_write_init(chunk, chunk_len);
        }

        int8_t get_int8_t(const char *key)
        {
                return bs_get_int8_t(&this->ctx, key, 0, NULL);
        }

        int8_t get_int8_t(const char *key, int8_t df)
        {
                return bs_get_int8_t(&this->ctx, key, df, NULL);
        }

        int8_t get_int8_t(const char *key, int8_t df, int *err)
        {
                return bs_get_int8_t(&this->ctx, key, df, err);
        }

        uint8_t get_uint8_t(const char *key)
        {
                return bs_get_uint8_t(&this->ctx, key, 0, NULL);
        }

        uint8_t get_uint8_t(const char *key, uint8_t df)
        {
                return bs_get_uint8_t(&this->ctx, key, df, NULL);
        }

        uint8_t get_uint8_t(const char *key, uint8_t df, int *err)
        {
                return bs_get_uint8_t(&this->ctx, key, df, err);
        }

        int16_t get_int16_t(const char *key)
        {
                return bs_get_int16_t(&this->ctx, key, 0, NULL);
        }

        int16_t get_int16_t(const char *key, int16_t df)
        {
                return bs_get_int16_t(&this->ctx, key, df, NULL);
        }

        int16_t get_int16_t(const char *key, int16_t df, int *err)
        {
                return bs_get_int16_t(&this->ctx, key, df, err);
        }

        uint16_t get_uint16_t(const char *key)
        {
                return bs_get_uint16_t(&this->ctx, key, 0, NULL);
        }

        uint16_t get_uint16_t(const char *key, uint16_t df)
        {
                return bs_get_uint16_t(&this->ctx, key, df, NULL);
        }

        uint16_t get_uint16_t(const char *key, uint16_t df, int *err)
        {
                return bs_get_uint16_t(&this->ctx, key, df, err);
        }

        int32_t get_int32_t(const char *key)
        {
                return bs_get_int32_t(&this->ctx, key, 0, NULL);
        }

        int32_t get_int32_t(const char *key, int32_t df)
        {
                return bs_get_int32_t(&this->ctx, key, df, NULL);
        }

        int32_t get_int32_t(const char *key, int32_t df, int *err)
        {
                return bs_get_int32_t(&this->ctx, key, df, err);
        }

        uint32_t get_uint32_t(const char *key)
        {
                return bs_get_uint32_t(&this->ctx, key, 0, NULL);
        }

        uint32_t get_uint32_t(const char *key, uint32_t df)
        {
                return bs_get_uint32_t(&this->ctx, key, df, NULL);
        }

        uint32_t get_uint32_t(const char *key, uint32_t df, int *err)
        {
                return bs_get_uint32_t(&this->ctx, key, df, err);
        }

        int64_t get_int64_t(const char *key)
        {
                return bs_get_int64_t(&this->ctx, key, 0, NULL);
        }

        int64_t get_int64_t(const char *key, int64_t df)
        {
                return bs_get_int64_t(&this->ctx, key, df, NULL);
        }

        int64_t get_int64_t(const char *key, int64_t df, int *err)
        {
                return bs_get_int64_t(&this->ctx, key, df, err);
        }

        uint64_t get_uint64_t(const char *key)
        {
                return bs_get_uint64_t(&this->ctx, key, 0, NULL);
        }

        uint64_t get_uint64_t(const char *key, uint64_t df)
        {
                return bs_get_uint64_t(&this->ctx, key, df, NULL);
        }

        uint64_t get_uint64_t(const char *key, uint64_t df, int *err)
        {
                return bs_get_uint64_t(&this->ctx, key, df, err);
        }

        float get_float(const char *key)
        {
                return bs_get_float(&this->ctx, key, 0, NULL);
        }

        float get_float(const char *key, float df)
        {
                return bs_get_float(&this->ctx, key, df, NULL);
        }

        float get_float(const char *key, float df, int *err)
        {
                return bs_get_float(&this->ctx, key, df, err);
        }

        double get_double(const char *key)
        {
                return bs_get_double(&this->ctx, key, 0, NULL);
        }

        double get_double(const char *key, double df)
        {
                return bs_get_double(&this->ctx, key, df, NULL);
        }

        double get_double(const char *key, double df, int *err)
        {
                return bs_get_double(&this->ctx, key, df, err);
        }

        bool get_bool(const char *key)
        {
                return bs_get_bool(&this->ctx, key, false, NULL);
        }

        bool get_bool(const char *key, bool df)
        {
                return bs_get_bool(&this->ctx, key, df, NULL);
        }

        bool get_bool(const char *key, bool df, int *err)
        {
                return bs_get_bool(&this->ctx, key, df, err);
        }

        const char *get_str(const char *key)
        {
                return bs_get_str(&this->ctx, key, "", NULL);
        }

        const char *get_str(const char *key, const char *df)
        {
                return bs_get_str(&this->ctx, key, df, NULL);
        }

        const char *get_str(const char *key, const char *df, int *err)
        {
                return bs_get_str(&this->ctx, key, df, err);
        }

        const void *get_blob(const char *key, size_t *len)
        {
                return bs_get_blob(&this->ctx, key, len, NULL);
        }

        const void *get_blob(const char *key, size_t *len, int *err)
        {
                return bs_get_blob(&this->ctx, key, len, err);
        }

        bs_type_t get_type(const char *key)
        {
                return bs_get_type(&this->ctx, key);
        }

        int get_next(uint8_t **next, const char **key, const void **value, uint16_t *value_len,
                bs_type_t *type)
        {
                return bs_get_next(&this->ctx, next, key, value, value_len, type);
        }

        int get_chunk_length()
        {
                return bs_get_chunk_length(&this->ctx);
        }

        int set_int8_t(const char *key, int8_t value)
        {
                return bs_set_int8_t(&this->ctx, key, value);
        }

        int set_uint8_t(const char *key, uint8_t value)
        {
                return bs_set_uint8_t(&this->ctx, key, value);
        }

        int set_int16_t(const char *key, int16_t value)
        {
                return bs_set_int16_t(&this->ctx, key, value);
        }

        int set_uint16_t(const char *key, uint16_t value)
        {
                return bs_set_uint16_t(&this->ctx, key, value);
        }

        int set_int32_t(const char *key, int32_t value)
        {
                return bs_set_int32_t(&this->ctx, key, value);
        }

        int set_uint32_t(const char *key, uint32_t value)
        {
                return bs_set_uint32_t(&this->ctx, key, value);
        }

        int set_int64_t(const char *key, int64_t value)
        {
                return bs_set_int64_t(&this->ctx, key, value);
        }

        int set_uint64_t(const char *key, uint64_t value)
        {
                return bs_set_uint64_t(&this->ctx, key, value);
        }

        int set_float(const char *key, float value)
        {
                return bs_set_float(&this->ctx, key, value);
        }

        int set_double(const char *key, double value)
        {
                return bs_set_double(&this->ctx, key, value);
        }

        int set_str(const char *key, const char *str)
        {
                return bs_set_str(&this->ctx, key, str);
        }

        int set_str(const char *key, const char *str, size_t str_len)
        {
                return bs_set_str_len(&this->ctx, key, str, str_len);
        }

        int set_bool(const char *key, bool value)
        {
                return bs_set_bool(&this->ctx, key, value);
        }

        int set_blob(const char *key, const void *blob, size_t len)
        {
                return bs_set_blob(&this->ctx, key, blob, len);
        }
};

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
