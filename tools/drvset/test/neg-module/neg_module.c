/* Negative test of drvset: module name used by module of the kernel (check error). MIT License. */
#include "drivers/driver.h"


API_MOD_INIT(GPIO, void **hdl, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG4(hdl, major, minor, config);
        return ESUCC;
}

#include "../neg_common.h"
API_MOD_COMMON(GPIO)
