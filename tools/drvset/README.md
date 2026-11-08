# Drvset build base

The directory builds the drvset partition: drivers (modules) and file systems
built separately from the kernel and programmed to flash at
`__OS_DRVSET_FLASH_ADDRESS__`. The kernel registers them at start, after
built-in modules and file systems, so they are used in the same way
(`driver_init("EXAMPLE", 0, 0, "/dev/example")`, `mount("-", "myfs", "/mnt")`).
Sources of drvset drivers and file systems do not have to be published with the
kernel.

Parts that define the binary interface with the kernel are in the kernel
repository (`kernel/src/drvset/`):

| File | Description |
|------|-------------|
| `drvset_header.c` | partition header (`drivers/drvset.h`): module table, interrupt table, memory layout |
| `drvset_fs.c` | file system table (`fs/fsif.h`) |
| `ksymstub.S` | stubs of kernel functions (`kernel/ksymtab.h`), 16 B (Cortex-M3/4/7, simulator) or 20 B (Cortex-M0+) of flash per used function, no RAM |
| `drvset.ld` | linker script of partition |
| `mkirqtab.awk` | generates interrupt list from the kernel vector table |

## Kernel configuration
Drvset is built for the kernel configuration (`config/app-kernel` by default),
in which external drivers are enabled (`__OS_ENABLE_DRVSET__`) and the following
values are set:

| Flag | Description |
|------|-------------|
| `__OS_DRVSET_FLASH_ADDRESS__` | partition address |
| `__OS_DRVSET_FLASH_SIZE__` | partition size [KiB] |
| `__OS_DRVSET_RAM_SIZE__` | RAM of drivers (.data, .bss) [KiB], placed by the kernel at the start of the main RAM region |
| `__OS_DRVSET_MAX_MODULES__` | maximum number of modules |

The kernel vector table address and the address of the main RAM region are
taken from `<config>/custom_linker_script.ld`. A partition built for another
configuration is rejected by the kernel at start (message on the system
console), it is never started.

## POSIX simulator
Drvset can be built for the simulator configuration (`config/sim-kernel`,
i386) and started with the simulator option `--drvset`: the simulator maps the
image at `__OS_DRVSET_FLASH_ADDRESS__` (0x50000000) and the partition RAM after
it, the kernel registers modules as on the microcontroller:
```
make build-sim build-sim-drvset [SIM_DRVSET_DIR=<drvset>]   # default: tools/drvset/test/sim
build/dnx-sim --disk 0=build/sim-romfs.bin:ro --drvset build/sim-drvset.bin
$ modprobe -i EXAMPLE 0 0 /dev/example
$ cat /dev/example
```
The simulator does not emulate interrupts, a partition with interrupt handlers
is rejected. `make test-sim` builds `test/sim` (example driver and ramfs of the
kernel as file system `dsramfs`) and runs `drvset: example driver`,
`fstest: dsramfs (drvset)` and `drvset: incorrect image` tests of
`tools/simtest.py`.

## Drivers
`DRVSET_DIR/Makefile.in`:
```make
DRVSET_MODULES += EXAMPLE                       # NAME or NAME:blk (block device)
CSRC_DRVSET    += $(DRVSET_ROOT)example.c
HDRLOC_DRVSET  += $(DRVSET_ROOT)
IOCTL_DRVSET   += $(DRVSET_ROOT)example_ioctl.h # copied to output/ioctl
```

Drivers are written as drivers of the kernel (`drivers/driver.h`,
`MODULE_NAME()`, `API_MOD_*()`), they can use only the module API
(`kernel/sysapi.h`, `sys_*` and `drv_*` functions) and driver interfaces of
kernel drivers (see below). Use of other kernel
function ends with "undefined reference" at link. Kernel functions (also
`memcpy()`, `sys_strfmt()`, `printk()`) are not linked into the partition,
they are called through the kernel symbol table.

## File systems
`DRVSET_DIR/Makefile.in`:
```make
DRVSET_FILESYSTEMS += myfs              # name used by mount and by API_FS_*(myfs, ...)
DRVSET_FILESYSTEMS += dsramfs:ramfs     # mount name dsramfs, source uses API_FS_*(ramfs, ...)
CSRC_DRVSET        += $(DRVSET_ROOT)myfs.c
```
File systems are written as file systems of the kernel (`fs/fs.h`, `API_FS_*()`,
`fs_*` and `sys_*` functions). A file system is mounted by its name when it is
not found among file systems of the kernel; names shall differ from file systems
of the kernel. The interface depends on kernel configuration (`__OS_ENABLE_*__`
flags of file operations), so the partition shall be built with the kernel
configuration (the kernel checks the interface at start).

Restrictions:
- C only, C++ constructors are not supported,
- no libc (newlib) functions other than kernel exported ones, `libgcc` is
  linked (division, 64-bit and floating point helpers),
- configuration of a driver cannot use kernel configuration (configtool):
  use the `config` argument of `API_MOD_INIT()` or own header.

## Interrupts
Exclusive interrupt handlers have names of the kernel vector table (e.g.
`USART1_IRQHandler`), so a driver of the kernel can be moved to drvset without
changes. The build generates the interrupt table of the partition (flash,
4 bytes per CPU interrupt). Interrupts that are not handled by the kernel image
go to the default handler of the kernel vector table, which calls the handlers
of the partition. The partition can use only interrupts that are not used by
the kernel image (checked at start). No RAM is used, latency is a few cycles
longer than of a kernel interrupt.

Interrupt lines shared by peripherals (e.g. `TIM6_DAC`, `TIM8_UP_TIM13`,
`UART9_LPTIM4`) can be used by many drivers of the partition with shared
handlers (8 bytes of flash per handler). Each shared handler checks flags of
own device:
```c
static void dac_irq(void)
{
        if (DAC1->SR & DAC_SR_DMAUDR1) {
                DAC1->SR = DAC_SR_DMAUDR1;
                // ...
        }
}

DRV_IRQ_SHARED(TIM6_DAC_IRQn, dac_irq);
```

Shared handlers can be used also by drivers of the kernel: the driver Makefile
adds the driver to `IRQ_SHARED` (`IRQ_SHARED += <driver>`), then the kernel is
built with the shared interrupt dispatcher (`kernel/src/kernel/kirq.c`, the
IRQ must not have a handler in the kernel vector table). Kernel and partition
drivers can share one interrupt line: the default handler calls shared handlers
of the kernel and then handlers of the partition. The kernel prints at start
`IRQ n: shared handler is not used` when the vector of the IRQ is used by
another handler. A driver must not disable the shared interrupt in NVIC on
release (other drivers use it). Without drvset support and without
`IRQ_SHARED` the dispatcher is not built (no flash or RAM used).
`drvset_check.py -v` marks partition interrupts shared with kernel drivers,
`--kernel-info` lists them (`shared_irqs=`).

## Driver interfaces of kernel drivers
Drivers of the partition can use driver interfaces of drivers built into the
kernel (the same headers as kernel drivers):

| Header | Functions |
|--------|-----------|
| `gpio_ddi.h` | `_GPIO_DDI_*`: pin state and configuration (pin names of kernel configuration) |
| `<arch>/dma_ddi.h` | `_DMA_DDI_*`: DMA streams/channels of the kernel DMA driver |
| `irq_ddi.h` | `_IRQ_DDI_attach()`, `_IRQ_DDI_detach()`: callbacks of EXTI lines (also lines of shared interrupts `EXTI9_5`, `EXTI15_10`), interrupts are handled by the kernel IRQ driver |

When the driver is not built into the kernel, functions return an error
(`ENODEV`, 0 or -1), e.g. `_IRQ_DDI_attach()` returns `ENODEV` when the IRQ
module is disabled. `_MDMA_DDI_*` are declared but not implemented by the kernel
DMA driver yet, they always return an error.

```c
#include "irq_ddi.h"

static bool button_cb(u8_t line, void *arg)
{
        // ... sys_semaphore_signal_from_ISR()
        return false;
}

// PC13, falling edge
int err = _IRQ_DDI_attach(13, 2, IRQ_CONFIG__TRIGGER_ON_FALLING_EDGE, button_cb, hdl);
```

## Usage
```
make build-app-drvset [DRVSET_DIR=<drvset>]
make -C tools/drvset all DRVSET_DIR=<drvset> [CONFIG_DIR=<kernel config>] [BUILD_DIR=<build>] [KERNEL_VECTORS=<address>]
```

| Variable | Description |
|----------|-------------|
| `DRVSET_DIR` | directory with `Makefile.in` of drivers |
| `CONFIG_DIR` | kernel configuration, default: `config/app-kernel` |
| `CFLAGS_DRVSET` | additional compiler flags of drivers (also in `Makefile.in`) |
| `BUILD_DIR` | build directory, default: `build/<drvset name>` |
| `KERNEL_VECTORS` | kernel vector table address, default: origin of the kernel flash region (e.g. vector table after a boot stage of RP2040 shall be given) |

Output: `<build>/output/drvset.{elf,hex,bin}` (hex and elf contain the
partition address), `<build>/output/ioctl/` ioctl headers for programs.

## Check
`drvset_check.py` checks the partition image against the kernel image before
programming (no hardware needed): the same checks as the kernel at start
(header, memory layout, modules, interrupts) and the binary interface: each
kernel function used by drivers shall select the kernel symbol table entry of
the function with the same name.
```
make -C tools/drvset check DRVSET_DIR=<drvset> [CONFIG_DIR=<kernel config>] KERNEL_ELF=build/app-kernel.elf
tools/drvset/drvset_check.py [-v] <kernel.elf> <drvset.elf> [--flash ADDR] [--size BYTES] [--ram BYTES] [--max-modules N]
```
The simulator image is checked against `build/dnx-sim` (also by `make test-sim`).
`make build-app-drvset` checks the partition when `build/app-kernel.elf`
exists.

## Tests
`make test-drvset` (`test/test.sh`) builds the test kernel (`config/app-kernel`
with drvset and IRQ module, `test/kernel.bsp`), the example partition, test
drivers (`test/drivers`: kernel functions, driver interfaces, exclusive and
shared interrupt handlers, block device, file system) and negative tests (not
exported kernel function, interrupt of the kernel image, module name of the
kernel, file system name of the kernel, other kernel address) and checks the results by `drvset_check.py`. Logs are
in `build/drvset-test`. Kernel configurations with drvset support are also
built by `kernel/tools/tests.sh`.

## Example
`example/` contains a character device driver `EXAMPLE`. Read returns the
number of handled interrupts (shared handler). On STM32H7 `IOCTL_EXAMPLE__TRIGGER_IRQ` triggers
the CRS interrupt by software, so the interrupt dispatcher can be tested
without additional hardware.
