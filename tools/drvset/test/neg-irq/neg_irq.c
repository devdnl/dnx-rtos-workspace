/* Negative test of drvset: interrupt handled by the kernel image (check error). MIT License. */
#include "drivers/driver.h"


static void irq_handler(void)
{
}

DRV_IRQ_SHARED(TEST_IRQ_USED, irq_handler);

API_MOD_INIT(DSNEGIRQ, void **hdl, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG4(hdl, major, minor, config);
        return ESUCC;
}

#include "../neg_common.h"
API_MOD_COMMON(DSNEGIRQ)
