/*==============================================================================
File    vformat.h

Author  Daniel Zorychta

Brief   Formatted output core used by the printf() family.

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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
@defgroup _LIBC_SOURCE_VFORMAT_H_ _LIBC_SOURCE_VFORMAT_H_

Formatted output core. The core parses the format string and passes the
produced text to the output sink. The sink decides where characters go
(string buffer, file). The core counts all produced characters, also these
not accepted by the sink, so the result is the POSIX printf() return value.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct _libc_fmt_out _libc_fmt_out_t;

/**
 * @brief Output sink. Embed it as the first member of the sink context.
 */
struct _libc_fmt_out {
        /** write function; return false on output error */
        bool (*write)(_libc_fmt_out_t *out, const char *str, size_t len);
        size_t count; //!< number of produced characters (managed by core)
        bool error;   //!< output error occurred (managed by core)
};

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern int _libc_vformat(_libc_fmt_out_t *out, const char *format, va_list arg);

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
