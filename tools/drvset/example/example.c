/*==============================================================================
File    example.c

Author  Daniel Zorychta

Brief   EXAMPLE driver: example of driver built separately from the kernel
        (drvset partition).

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
 * The driver is written in the same way as drivers of the kernel: it uses
 * only public kernel API (kernel/sysapi.h, drivers/driver.h). Kernel
 * functions are called through the kernel symbol table, so printk() or
 * sys_strfmt() do not increase the driver size. The interrupt handler is
 * registered as shared handler (DRV_IRQ_SHARED()), so other drivers of the
 * partition can use the same interrupt. Exclusive handler would have the name
 * of the kernel vector table (CRS_IRQHandler).
 */

/*==============================================================================
  Include files
==============================================================================*/
#include "drivers/driver.h"
#include "example_ioctl.h"

#if defined(ARCH_stm32h7)
#include "stm32h7/stm32h7xx.h"
#endif

/*==============================================================================
  Local macros
==============================================================================*/
#if defined(ARCH_stm32h7)
#define EXAMPLE_IRQn CRS_IRQn /* interrupt not used by the kernel, triggered by software */
#endif

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        volatile u32_t irq_count;
} example_t;

/*==============================================================================
  Local objects
==============================================================================*/
MODULE_NAME(EXAMPLE);

/* driver instance used by interrupt handler (.bss in drvset RAM) */
static example_t *example;

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Initialize device.
 *
 * @param[out]          **device_handle        device allocated memory
 * @param[in ]            major                major device number
 * @param[in ]            minor                minor device number
 * @param[in ]           *config               optional module configuration
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_INIT(EXAMPLE, void **device_handle, u8_t major, u8_t minor, const void *config)
{
        UNUSED_ARG1(config);

        if ((major != 0) || (minor != 0)) {
                return ENODEV;
        }

        int err = sys_zalloc(sizeof(example_t), device_handle);
        if (!err) {
                example = *device_handle;

#if defined(EXAMPLE_IRQn)
                NVIC_SetPriority(EXAMPLE_IRQn, _CPU_IRQ_SAFE_PRIORITY_);
                NVIC_EnableIRQ(EXAMPLE_IRQn);
#endif
                printk("EXAMPLE: driver from drvset partition initialized");
        }

        return err;
}

//==============================================================================
/**
 * @brief Release device.
 *
 * @param[in ]          *device_handle          device allocated memory
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_RELEASE(EXAMPLE, void *device_handle)
{
#if defined(EXAMPLE_IRQn)
        NVIC_DisableIRQ(EXAMPLE_IRQn);
#endif
        example = NULL;

        return sys_free(&device_handle);
}

//==============================================================================
/**
 * @brief Open device.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[in ]           flags                  file operation flags (O_RDONLY, O_WRONLY, O_RDWR)
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_OPEN(EXAMPLE, void *device_handle, u32_t flags)
{
        UNUSED_ARG2(device_handle, flags);

        return ESUCC;
}

//==============================================================================
/**
 * @brief Close device.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[in ]           force                  device force close (true)
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_CLOSE(EXAMPLE, void *device_handle, bool force)
{
        UNUSED_ARG2(device_handle, force);

        return ESUCC;
}

//==============================================================================
/**
 * @brief Write data to device.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[in ]          *src                    data source
 * @param[in ]           count                  number of bytes to write
 * @param[in ][out]     *fpos                   file position
 * @param[out]          *wrcnt                  number of written bytes
 * @param[in ]           fattr                  file attributes
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_WRITE(EXAMPLE, void *device_handle, const u8_t *src, size_t count, fpos_t *fpos, size_t *wrcnt,
        struct vfs_fattr fattr)
{
        UNUSED_ARG6(device_handle, src, count, fpos, wrcnt, fattr);

        return EPERM;
}

//==============================================================================
/**
 * @brief Read data from device.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[out]          *dst                    data destination
 * @param[in ]           count                  number of bytes to read
 * @param[in ][out]     *fpos                   file position
 * @param[out]          *rdcnt                  number of read bytes
 * @param[in ]           fattr                  file attributes
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_READ(EXAMPLE, void *device_handle, u8_t *dst, size_t count, fpos_t *fpos, size_t *rdcnt,
        struct vfs_fattr fattr)
{
        UNUSED_ARG1(fattr);

        example_t *hdl = device_handle;

        char text[24];
        size_t len = sys_strfmt(text, sizeof(text), "irq: %u\n", (uint)hdl->irq_count);

        size_t pos = (size_t)*fpos;
        size_t n = 0;

        if (pos < len) {
                n = min(count, len - pos);
                memcpy(dst, &text[pos], n);
                *fpos += n;
        }

        *rdcnt = n;

        return ESUCC;
}

//==============================================================================
/**
 * @brief IO control.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[in ]           request                request
 * @param[in ][out]     *arg                    request's argument
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_IOCTL(EXAMPLE, void *device_handle, int request, void *arg)
{
        UNUSED_ARG2(device_handle, arg);

        switch (request) {
        case IOCTL_EXAMPLE__TRIGGER_IRQ:
#if defined(EXAMPLE_IRQn)
                NVIC_SetPendingIRQ(EXAMPLE_IRQn);
                return ESUCC;
#else
                return ENOTSUP;
#endif
        default:
                return EBADRQC;
        }
}

//==============================================================================
/**
 * @brief Flush device.
 *
 * @param[in ]          *device_handle          device allocated memory
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_FLUSH(EXAMPLE, void *device_handle)
{
        UNUSED_ARG1(device_handle);

        return ESUCC;
}

//==============================================================================
/**
 * @brief Device information.
 *
 * @param[in ]          *device_handle          device allocated memory
 * @param[out]          *device_stat            device status
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
API_MOD_STAT(EXAMPLE, void *device_handle, struct vfs_dev_stat *device_stat)
{
        UNUSED_ARG1(device_handle);

        device_stat->st_size = 0;

        return ESUCC;
}

#if defined(EXAMPLE_IRQn)
//==============================================================================
/**
 * @brief Shared interrupt handler, called by the drvset interrupt dispatcher
 *        of the kernel. Handler of real device shall check interrupt flags of
 *        its device, interrupt of this example is triggered only by software.
 */
//==============================================================================
static void example_irq(void)
{
        if (example) {
                example->irq_count++;
        }
}

DRV_IRQ_SHARED(EXAMPLE_IRQn, example_irq);
#endif

/*==============================================================================
  End of file
==============================================================================*/
