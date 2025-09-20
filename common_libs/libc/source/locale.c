/*==============================================================================
File    locale.c

Author  Daniel Zorychta

Brief   The library provides location specific settings.

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
#include "locale.h"
#include "stddef.h"
#include <dnx/misc.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
static const struct lconv locale = {
        .decimal_point     = ".",
        .thousands_sep     = " ",
        .grouping          = NULL,
        .int_curr_symbol   = "???",
        .currency_symbol   = "?",
        .mon_decimal_point = ".",
        .mon_thousands_sep = " ",
        .mon_grouping      = NULL,
        .positive_sign     = "",
        .negative_sign     = "-",
        .int_frac_digits   = 2,
        .frac_digits       = 2,
        .p_cs_precedes     = 0,
        .p_sep_by_space    = 0,
        .n_cs_precedes     = 0,
        .n_sep_by_space    = 0,
        .p_sign_posn       = 0,
        .n_sign_posn       = 0
};

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
 * @brief  Function set locale values.
 *
 * Sets locale information to be used by the current program, either changing
 * the entire locale or portions of it. The function can also be used to
 * retrieve the current locale's name by passing NULL as the value for argument
 * locale.
 *
 * @note Function is not supported by dnx RTOS. Function always return NULL.
 *
 * @param  category     category to set
 * @param  locale       value to set
 *
 * @return On success, a pointer to a C string identifying the locale currently
 *         set for the category.
 */
//==============================================================================
char *setlocale(int category, const char *locale)
{
        UNUSED_ARG2(category, locale);
        return NULL;
}

//==============================================================================
/**
 * @brief  Function get locale formatting parameters for quantities.
 *
 * Retrieves the values provided in the current locale object to format
 * parameters for quantities. These are returned in an object of type
 * struct lconv.
 *
 * @param  category     category to set
 * @param  locale       value to set
 *
 * @return On success, a pointer to a C string identifying the locale currently
 *         set for the category.
 */
//==============================================================================
struct lconv *localeconv(void)
{
        return const_cast(&locale);
}

/*==============================================================================
  End of file
==============================================================================*/
