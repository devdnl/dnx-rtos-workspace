/*==============================================================================
File    dstest.c

Author  Daniel Zorychta

Brief   Test drivers of drvset partition (tools/drvset/test/test.sh).

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

/*
 * Drivers are not started: the test builds the partition and checks it
 * against the kernel image (drvset_check.py). The code uses all features of
 * the partition: kernel functions (also variadic and string functions),
 * driver interfaces of kernel drivers (GPIO, DMA, IRQ), .data and .bss in
 * partition RAM, exclusive and shared interrupt handlers, character and block
 * device modules.
 */

/*==============================================================================
  Include files
==============================================================================*/
#include "drivers/driver.h"
#include "gpio_ddi.h"
#include "irq_ddi.h"

#if defined(ARCH_stm32f1)
#include "stm32f1/dma_ddi.h"
#elif defined(ARCH_stm32f3)
#include "stm32f3/dma_ddi.h"
#elif defined(ARCH_stm32f4)
#include "stm32f4/dma_ddi.h"
#elif defined(ARCH_stm32f7)
#include "stm32f7/dma_ddi.h"
#elif defined(ARCH_stm32h7)
#include "stm32h7/dma_ddi.h"
#endif

/*==============================================================================
  Local macros
==============================================================================*/
#define API_MOD_ALL(name)                                                                                              \
        API_MOD_RELEASE(name, void *hdl)                                                                               \
        {                                                                                                              \
                return sys_free(&hdl);                                                                                 \
        }                                                                                                              \
        API_MOD_OPEN(name, void *hdl, u32_t flags)                                                                     \
        {                                                                                                              \
                UNUSED_ARG2(hdl, flags);                                                                               \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_CLOSE(name, void *hdl, bool force)                                                                     \
        {                                                                                                              \
                UNUSED_ARG2(hdl, force);                                                                               \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_WRITE(name, void *hdl, const u8_t *src, size_t count, fpos_t *fpos, size_t *wrcnt,                    \
                struct vfs_fattr fattr)                                                                                \
        {                                                                                                              \
                UNUSED_ARG4(hdl, src, fpos, fattr);                                                                    \
                *wrcnt = count;                                                                                        \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_READ(name, void *hdl, u8_t *dst, size_t count, fpos_t *fpos, size_t *rdcnt, struct vfs_fattr fattr)    \
        {                                                                                                              \
                UNUSED_ARG3(hdl, fpos, fattr);                                                                         \
                memset(dst, 0, count);                                                                                 \
                *rdcnt = count;                                                                                        \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_FLUSH(name, void *hdl)                                                                                 \
        {                                                                                                              \
                UNUSED_ARG1(hdl);                                                                                      \
                return ESUCC;                                                                                          \
        }                                                                                                              \
        API_MOD_STAT(name, void *hdl, struct vfs_dev_stat *stat)                                                       \
        {                                                                                                              \
                UNUSED_ARG1(hdl);                                                                                      \
                stat->st_size = 0;                                                                                     \
                return ESUCC;                                                                                          \
        }

/*==============================================================================
  Local objects
==============================================================================*/
MODULE_NAME(DSTEST);

static u32_t irq_counter;              /* .bss */
static u32_t init_counter = 0x5A5A5A5A; /* .data */

/*==============================================================================
  Function definitions
==============================================================================*/
static bool exti_cb(u8_t line, void *arg)
{
        UNUSED_ARG2(line, arg);
        irq_counter++;
        return false;
}

static void shared_irq(void)
{
        irq_counter++;
}

static void shared_irq2(void)
{
        irq_counter += 2;
}

DRV_IRQ_SHARED(TEST_IRQ, shared_irq);
DRV_IRQ_SHARED(TEST_IRQ, shared_irq2);
DRV_IRQ_SHARED(TEST_IRQ2, shared_irq);

/* exclusive handler: name of the kernel vector table */
void TEST_IRQ_HANDLER(void)
{
        irq_counter += 3;
}

API_MOD_INIT(DSTEST, void **hdl, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG1(config);

        char name[16];
        sys_strfmt(name, sizeof(name), "dstest%u-%u", major, minor);
        sys_strlcat(name, "x", sizeof(name));
        printk("%s: %u", name, init_counter++);

        _GPIO_DDI_set_pin(0, 0);

#if defined(ARCH_stm32f4) || defined(ARCH_stm32f7)
        u32_t dmad = _DMA_DDI_reserve(0, 0);
#elif defined(ARCH_stm32h7)
        u32_t dmad = _DMA_DDI_reserve(_DMA_DDI_DMA1);
#else
        u32_t dmad = 0;
#endif
        if (dmad) {
                _DMA_DDI_release(dmad);
        }

        int err = _IRQ_DDI_attach(13, 2, IRQ_CONFIG__TRIGGER_ON_FALLING_EDGE, exti_cb, NULL);
        if (!err) {
                _IRQ_DDI_detach(13);
        }

        return sys_zalloc(16, hdl);
}

API_MOD_IOCTL(DSTEST, void *hdl, int request, void *arg)
{
        UNUSED_ARG3(hdl, request, arg);
        return EBADRQC;
}

API_MOD_ALL(DSTEST)

API_MOD_INIT(DSTESTBLK, void **hdl, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG3(major, minor, config);
        return sys_zalloc(512, hdl);
}

API_MOD_IOCTL(DSTESTBLK, void *hdl, int request, void *arg)
{
        UNUSED_ARG3(hdl, request, arg);
        return EBADRQC;
}

API_MOD_ALL(DSTESTBLK)

/*==============================================================================
  End of file
==============================================================================*/
