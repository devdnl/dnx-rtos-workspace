/* Negative test of drvset: kernel internal function is not available (link error). MIT License. */
#include "drivers/driver.h"

extern int _vfs_init(void); /* kernel internal function, not exported */


API_MOD_INIT(DSNEGUNDEF, void **hdl, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG4(hdl, major, minor, config);
        return _vfs_init();
}

#include "../neg_common.h"
API_MOD_COMMON(DSNEGUNDEF)
