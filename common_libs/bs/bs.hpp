/*==============================================================================
File     bs.hpp

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
class bs_rd {
private:
        bs_rd_t bs;

public:
        bs_rd(const uint8_t *chunk, size_t chunk_len)
        {
                bs_read_chunk(&this->bs, chunk, chunk_len);
        }


        int8_t get_int8_t(const char *key)
        {
                return bs_get_int8_t(&this->bs, key, 0, NULL);
        }

        int8_t get_int8_t(const char *key, int8_t df)
        {
                return bs_get_int8_t(&this->bs, key, df, NULL);
        }

        int8_t get_int8_t(const char *key, int8_t df, int *err)
        {
                return bs_get_int8_t(&this->bs, key, df, err);
        }


        uint8_t get_uint8_t(const char *key)
        {
                return bs_get_uint8_t(&this->bs, key, 0, NULL);
        }

        uint8_t get_uint8_t(const char *key, uint8_t df)
        {
                return bs_get_uint8_t(&this->bs, key, df, NULL);
        }

        uint8_t get_uint8_t(const char *key, uint8_t df, int *err)
        {
                return bs_get_uint8_t(&this->bs, key, df, err);
        }


        int16_t get_int16_t(const char *key)
        {
                return bs_get_int16_t(&this->bs, key, 0, NULL);
        }

        int16_t get_int16_t(const char *key, int16_t df)
        {
                return bs_get_int16_t(&this->bs, key, df, NULL);
        }

        int16_t get_int16_t(const char *key, int16_t df, int *err)
        {
                return bs_get_int16_t(&this->bs, key, df, err);
        }


        uint16_t get_uint16_t(const char *key)
        {
                return bs_get_uint16_t(&this->bs, key, 0, NULL);
        }

        uint16_t get_uint16_t(const char *key, uint16_t df)
        {
                return bs_get_uint16_t(&this->bs, key, df, NULL);
        }

        uint16_t get_uint16_t(const char *key, uint16_t df, int *err)
        {
                return bs_get_uint16_t(&this->bs, key, df, err);
        }


        int32_t get_int32_t(const char *key)
        {
                return bs_get_int32_t(&this->bs, key, 0, NULL);
        }

        int32_t get_int32_t(const char *key, int32_t df)
        {
                return bs_get_int32_t(&this->bs, key, df, NULL);
        }

        int32_t get_int32_t(const char *key, int32_t df, int *err)
        {
                return bs_get_int32_t(&this->bs, key, df, err);
        }


        uint32_t get_uint32_t(const char *key)
        {
                return bs_get_uint32_t(&this->bs, key, 0, NULL);
        }

        uint32_t get_uint32_t(const char *key, uint32_t df)
        {
                return bs_get_uint32_t(&this->bs, key, df, NULL);
        }

        uint32_t get_uint32_t(const char *key, uint32_t df, int *err)
        {
                return bs_get_uint32_t(&this->bs, key, df, err);
        }


        int64_t get_int64_t(const char *key)
        {
                return bs_get_int64_t(&this->bs, key, 0, NULL);
        }

        int64_t get_int64_t(const char *key, int64_t df)
        {
                return bs_get_int64_t(&this->bs, key, df, NULL);
        }

        int64_t get_int64_t(const char *key, int64_t df, int *err)
        {
                return bs_get_int64_t(&this->bs, key, df, err);
        }


        uint64_t get_uint64_t(const char *key)
        {
                return bs_get_uint64_t(&this->bs, key, 0, NULL);
        }

        uint64_t get_uint64_t(const char *key, uint64_t df)
        {
                return bs_get_uint64_t(&this->bs, key, df, NULL);
        }

        uint64_t get_uint64_t(const char *key, uint64_t df, int *err)
        {
                return bs_get_uint64_t(&this->bs, key, df, err);
        }


        float get_float(const char *key)
        {
                return bs_get_float(&this->bs, key, 0, NULL);
        }

        float get_float(const char *key, float df)
        {
                return bs_get_float(&this->bs, key, df, NULL);
        }

        float get_float(const char *key, float df, int *err)
        {
                return bs_get_float(&this->bs, key, df, err);
        }


        double get_double(const char *key) {
                return bs_get_double(&this->bs, key, 0, NULL);
        }

        double get_double(const char *key, double df) {
                return bs_get_double(&this->bs, key, df, NULL);
        }

        double get_double(const char *key, double df, int *err)
        {
                return bs_get_double(&this->bs, key, df, err);
        }


        bool get_bool(const char *key)
        {
                return bs_get_bool(&this->bs, key, false, NULL);
        }

        bool get_bool(const char *key, bool df)
        {
                return bs_get_bool(&this->bs, key, df, NULL);
        }

        bool get_bool(const char *key, bool df, int *err)
        {
                return bs_get_bool(&this->bs, key, df, err);
        }


        const char *get_str(const char *key)
        {
                return *bs_get_str(&this->bs, key, "", NULL);
        }

        const char *get_str(const char *key, const char *df)
        {
                return *bs_get_str(&this->bs, key, df, NULL);
        }

        const char *get_str(const char *key, const char *df, int *err)
        {
                return *bs_get_str(&this->bs, key, df, err);
        }


        const void *get_blob(const char *key, size_t *len)
        {
                return *bs_get_blob(&this->bs, key, len, NULL);
        }

        const void *get_blob(const char *key, size_t *len, int *err)
        {
                return *bs_get_blob(&this->bs, key, len, err);
        }

        bs_type_t get_type(const char *key)
        {
                return bs_get_type(&this->bs, key);
        }

        int get_next(uint8_t **next, const char **key, const void **value, bs_type_t *type)
        {
                return bs_get_next(&this->bs, next, key, value, type);
        }
};

class bs_wr {
private:
        bs_wr_t bs;

public:
        bs_wr(uint8_t *chunk, size_t chunk_len)
        {
                bs_write_chunk(&this->bs, chunk, chunk_len);
        }

        int get_chunk_length()
        {
                return bs_get_chunk_length(&this->bs);
        }

        int set_int8_t(const char *key, int8_t value)
        {
                return bs_set_int8_t(&this->bs, key, value);
        }

        int set_uint8_t(const char *key, uint8_t value)
        {
                return bs_set_uint8_t(&this->bs, key, value);
        }

        int set_int16_t(const char *key, int16_t value)
        {
                return bs_set_int16_t(&this->bs, key, value);
        }

        int set_uint16_t(const char *key, uint16_t value)
        {
                return bs_set_uint16_t(&this->bs, key, value);
        }

        int set_int32_t(const char *key, int32_t value)
        {
                return bs_set_int32_t(&this->bs, key, value);
        }

        int set_uint32_t(const char *key, uint32_t value)
        {
                return bs_set_uint32_t(&this->bs, key, value);
        }

        int set_int64_t(const char *key, int64_t value)
        {
                return bs_set_int64_t(&this->bs, key, value);
        }

        int set_uint64_t(const char *key, uint64_t value)
        {
                return bs_set_uint64_t(&this->bs, key, value);
        }

        int set_float(const char *key, float value)
        {
                return bs_set_float(&this->bs, key, value);
        }

        int set_double(const char *key, double value)
        {
                return bs_set_double(&this->bs, key, value);
        }

        int set_str(const char *key, const char *str)
        {
                return bs_set_str(&this->bs, key, str);
        }

        int set_bool(const char *key, bool value)
        {
                return bs_set_bool(&this->bs, key, value);
        }

        int set_blob(const char *key, const void *blob, size_t len)
        {
                return bs_set_blob(&this->bs, key, blob, len);
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
