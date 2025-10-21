/*==============================================================================
@file   bootloader.c

@author Daniel Zorychta

@brief  Boot loader.

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dnx/misc.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/net.h>
#include <dnx/vt100.h>
#include <unistd.h>
#include <fpkg/fpkg.h>
#include <utcl/utcl.h>
#include <time.h>
#include <sys/time.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define BOOTLOADER_VERSION    "1.1"
#define APP_BASE_ADDRESS      0x08040000
#define QSPI_BASE_ADDRESS     0x90000000
#define PATH_VENDOR           "/sd/vendor"
#define PATH_UPDATE_FILE      PATH_VENDOR "/project.fpkg"
#define PATH_APP_FLASH        "/dev/app-flash"
#define PATH_APP_QSPI         "/dev/app-qspi"
#define FPKG_SECTION_MANIFEST "/manifest"
#define SECTOR_SIZE           128 * 1024

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char str[80];
        u8_t buf[4096];
};

static const QSPI_command_t qspi_dread = {
        .instruction_mode = QSPI_MODE__SINGLE,
        .address_mode = QSPI_MODE__DOUBLE,
        .alternate_byte_mode = QSPI_MODE__NONE,
        .data_mode = QSPI_MODE__DOUBLE,
        .address_size = QSPI_SIZE__24_BITS,
        .alternate_bytes_size = QSPI_SIZE__8_BITS,
        .instruction = 0xBB,
        .address = 0,
        .alternate_bytes = 0,
        .dummy_cycles = 4,
        .data_size = 0,
        .data = NULL,
        .instruction_only_once = false,
        .ddr = false,
        .ddr_hold_cycle = QSPI_DDR_HOLD_CYCLE__ANALOG_DELAY,
        .direction = QSPI_DIRECTION__READ,
};

static const QSPI_command_t qspi_pp = {
        .instruction_mode = QSPI_MODE__SINGLE,
        .address_mode = QSPI_MODE__SINGLE,
        .alternate_byte_mode = QSPI_MODE__NONE,
        .data_mode = QSPI_MODE__SINGLE,
        .address_size = QSPI_SIZE__24_BITS,
        .alternate_bytes_size = QSPI_SIZE__8_BITS,
        .instruction = 0x02,
        .address = 0,
        .alternate_bytes = 0,
        .dummy_cycles = 0,
        .data_size = 0,
        .data = NULL,
        .instruction_only_once = false,
        .ddr = false,
        .ddr_hold_cycle = QSPI_DDR_HOLD_CYCLE__ANALOG_DELAY,
        .direction = QSPI_DIRECTION__WRITE,
};

static const QSPI_command_t qspi_wren = {
        .instruction_mode = QSPI_MODE__SINGLE,
        .address_mode = QSPI_MODE__NONE,
        .alternate_byte_mode = QSPI_MODE__NONE,
        .data_mode = QSPI_MODE__NONE,
        .address_size = QSPI_SIZE__8_BITS,
        .alternate_bytes_size = QSPI_SIZE__8_BITS,
        .instruction = 0x06,
        .address = 0,
        .alternate_bytes = 0,
        .dummy_cycles = 0,
        .data_size = 0,
        .data = NULL,
        .instruction_only_once = false,
        .ddr = false,
        .ddr_hold_cycle = QSPI_DDR_HOLD_CYCLE__ANALOG_DELAY,
        .direction = QSPI_DIRECTION__WRITE,
};

static const QSPI_command_t qspi_rdsr = {
        .instruction_mode = QSPI_MODE__SINGLE,
        .address_mode = QSPI_MODE__NONE,
        .alternate_byte_mode = QSPI_MODE__NONE,
        .data_mode = QSPI_MODE__SINGLE,
        .address_size = QSPI_SIZE__8_BITS,
        .alternate_bytes_size = QSPI_SIZE__8_BITS,
        .instruction = 0x05,
        .address = 0,
        .alternate_bytes = 0,
        .dummy_cycles = 0,
        .data_size = 0,
        .data = NULL,
        .instruction_only_once = false,
        .ddr = false,
        .ddr_hold_cycle = QSPI_DDR_HOLD_CYCLE__ANALOG_DELAY,
        .direction = QSPI_DIRECTION__READ,
};

static const QSPI_command_t qspi_se4k = {
        .instruction_mode = QSPI_MODE__SINGLE,
        .address_mode = QSPI_MODE__SINGLE,
        .alternate_byte_mode = QSPI_MODE__NONE,
        .data_mode = QSPI_MODE__NONE,
        .address_size = QSPI_SIZE__24_BITS,
        .alternate_bytes_size = QSPI_SIZE__8_BITS,
        .instruction = 0x20,
        .address = 0,
        .alternate_bytes = 0,
        .dummy_cycles = 0,
        .data_size = 0,
        .data = NULL,
        .instruction_only_once = false,
        .ddr = false,
        .ddr_hold_cycle = QSPI_DDR_HOLD_CYCLE__HALF_CLK_DELAY,
        .direction = QSPI_DIRECTION__WRITE,
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(bootloader, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Function initializes basic file system.
 */
//==============================================================================
static void create_base_file_system_structure(void)
{
        /*
         * 1. Mount base file system as root.
         */
        mount("ramfs", "", "/", "");

        /*
         * 2. Create basic folders. This folders can be used to store device
         *    files or to mount next file systems e.g. SD Card etc.
         */
        mkdir("/dev", 0666);
        mkdir("/tmp", 0666);
        mkdir("/run", 0666);
        mkdir("/etc", 0666);
        mkdir("/proc", 0666);
}

//==============================================================================
/**
 * @brief Function initialize basic drivers needed by CPU configuration or board
 *        specification.
 */
//==============================================================================
static void initialize_basic_drivers(void)
{
        /*
         * 1. Initialize GPIO drivers. Number of GPIO drivers depends on microcontroller
         *    pinout. If microcontroller has only 2 GPIO ports then only those
         *    two should be initialized. Not all GPIOs should have file representation.
         *    If file is not needed (because port is not used from application)
         *    then node is not necessary (use NULL or empty string as file name).
         */
        driver_init("GPIO", 0, 0, "/dev/gpio");
        driver_init("GPIO", 1, 0, NULL);
        driver_init("GPIO", 2, 0, NULL);
        driver_init("GPIO", 3, 0, NULL);
        driver_init("GPIO", 4, 0, NULL);
        driver_init("GPIO", 5, 0, NULL);
        driver_init("GPIO", 6, 0, NULL);
        driver_init("GPIO", 7, 0, NULL);

        /*
         * 2. Next part of drivers that can be initialized at this early stage.
         */
        driver_init("AFM", 0, 0, NULL);       // alternative function configuration
        driver_init("CLK", 0, 0, "/dev/clk"); // system clock configuration
        driver_init("DMA", 0, 0, NULL);       // DMA configuration

        /*
         * NOTE: make sure that UART1 is used as terminal output!
         *       Some BSPs use UART2 as default terminal output.
         */
        driver_init2("UART", __CPU_UART_TERM__, 0, "/dev/ttyS0",
                &(UART_config_t) {
                        .mode = UART_MODE__NORMAL,
                        .parity = UART_PARITY__OFF,
                        .stop_bits = UART_STOP_BIT__1,
                        .tx_enable = true,
                        .rx_enable = true,
                        .baud = 460800,
                        .features = 0,
                });

        static const TTY_config_t TTY_config = {
                .input_file = "/dev/ttyS0",
                .output_file = "/dev/ttyS0",
        };
        driver_init2("TTY", 0, 0, "/dev/tty0", &TTY_config); // first user terminal
}

//==============================================================================
/**
 * @brief Function open output stream. Stream is should be opened as soon as
 *        possible to present user system messages.
 */
//==============================================================================
static void open_output_stream(void)
{
        /*
         * 1. Open standard output and standard error streams to see system
         *    messages. Device that can be used as output stream was initialized
         *    in the previous stage (tty0 connected to UART2).
         */
        stdout = fopen("/dev/tty0", "r+");
        stderr = stdout;

        printf("Bootloader version %s\n", BOOTLOADER_VERSION);
}

//==============================================================================
/**
 * @brief Show kernel panic message.
 */
//==============================================================================
static void show_kernel_panic_message(void)
{
        /*
         * 1. Show kernel panic message if occurred in the last cycle.
         *    Kernel panic message is redirected to the standard output.
         *    Implementation hold kernel panic message by 3 seconds.
         */
        kernel_panic_info_t info;
        if (get_kernel_panic_info(&info)) {

                printf(VT100_FONT_COLOR_RED);
                printf("*** %s ***\n",
                        info.flags & KERNEL_PANIC_FLAG__KERNEL_PANIC ? "KERNEL PANIC" : "APP CRASH");
                printf("  Cause  : %s (%d)\n", info.cause_str, info.cause);
                printf("  PID    : %u (%.*s)\n", info.pid, sizeof(info.name), info.name);
                printf("  TID    : %u\n", info.tid);
                printf("  SPACE  : %s\n", info.flags & KERNEL_PANIC_FLAG__KERNEL_SPACE ? "kernel" : "user");
                printf("  SYSCALL: %u\n", info.syscall);
                printf("  FLAGS  : %s%s%s\n",
                        ((info.flags & KERNEL_PANIC_FLAG__KERNEL_SPACE) ? "KS:1 " : "KS:0 "),
                        ((info.flags & KERNEL_PANIC_FLAG__KERNEL_PANIC) ? "KP:1 " : "KP:0 "),
                        ((info.flags & KERNEL_PANIC_FLAG__PANIC_LOOP) ? "PL:1 " : "PL:0 "));
                printf("  CORE   : %u\n", info.core);
                printf(VT100_RESET_ATTRIBUTES "\n");

                sleep(3);
        }
}

//==============================================================================
/**
 * @brief Function initialize additional drivers.
 */
//==============================================================================
static void initialize_additional_drivers(void)
{
        driver_init("TTY", 0, 1, "/dev/tty1");
        driver_init("TTY", 0, 2, "/dev/tty2");
        driver_init("RTC", 0, 0, "/dev/rtc");

        driver_init2("QSPI", 0, 0, PATH_APP_QSPI,
                &(const QSPI_config_t) {
                        .clock_prescaller = 2,
                        .sample_shifting = QSPI_SAMPLE_SHIFTING__NONE,
                        .flash_size_bits = 24,
                        .chip_select_high_time = 5,
                        .clock_mode = QSPI_CLOCK_MODE__0,
                        .dual_flash = false,
                        .page_size = 256,
                        .write_in_progress_mask = 0x01,
                        .write_in_progress_match = 0x00,
                        .write_in_progress_register_size = QSPI_SIZE__8_BITS,
                        .read_command = &qspi_dread,
                        .page_program_command = &qspi_pp,
                        .write_enable_command = &qspi_wren,
                        .read_status_register_command = &qspi_rdsr,
                });

        driver_init2("NVM", 0, 0, PATH_APP_FLASH,
                &(const NVM_config_t) {
                        .start_sector = 6,
                        .number_of_sectors = 2,
                });
}

//==============================================================================
/**
 * @brief Mount ROMFS file system.
 */
//==============================================================================
static void mount_file_systems(void)
{
        /*
         * SD card
         */
        driver_init("SDMMC", 0, 0, "/dev/sd0");

        driver_init2("PART", 0, 0, "/dev/sd0p1", &(const PART_config_t) {.path = "/dev/sd0"});

        scope (int fd = open("/dev/sd0p1", O_RDWR), fd > 0, close(fd)) {
                ioctl(fd, IOCTL_PART__READ_MBR);
        }

        mkdir("/sd", 0666);
        mount("fatfs", "/dev/sd0p1", "/sd", "");
}

//==============================================================================
/**
 * @brief Function print system log messages.
 */
//==============================================================================
static void print_system_log_messages(void)
{
        /*
         * 1. This function prints all system log messages that were stored
         *    at system startup. It can be used in debug purposes. It can be
         *    also disabled if not needed. Function run in thread.
         */
        struct timeval t = {0, 0};

        while (true) {
                if (syslog_read(global->str, sizeof(global->str), &t, &t)) {
                        printf("[%lld.%06lu] %s\n", t.tv_sec, t.tv_usec, global->str);
                } else {
                        break;
                }
        }
}

//==============================================================================
/**
 * @brief  Extract selected file from FPKG package.
 *
 * @param  fpkg         FPKG instance
 * @param  file_src     file source path
 * @param  file_dest    file destination path
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int fpkg_extract(fpkg_t *fpkg, const char *file_src, const char *file_dest)
{
        int err = ENOENT;

        fpkg_stat_t stat;
        if (fpkg__stat(fpkg, file_src, &stat) != 0) {
                stat.size = 0;
        }

        fpkg_file_t *fsrc = fpkg__fopen(fpkg, file_src);
        FILE *fdest = fopen(file_dest, "w");

        size_t size = 0;

        while (fsrc and fdest) {
                size_t n = fpkg__fread(fsrc, global->buf, sizeof(global->buf));
                if (n > 0) {
                        errno = 0;
                        if (fwrite(global->buf, n, 1, fdest) != 1) {
                                err = errno;
                                break;
                        } else {
                                size += n;
                        }
                } else {
                        if (stat.size == size) {
                                printf("Extracting '%s' -> '%s' %u%% (%lu bytes).\n", file_src, file_dest,
                                        (size * 100) / stat.size, size);
                                err = 0;

                        } else {
                                printf("Extracting '%s' -> '%s' error.\n", file_src, file_dest);
                                err = EIO;
                        }
                        break;
                }
        }

        fpkg__fclose(fsrc);
        fclose(fdest);

        return err;
}

//==============================================================================
/**
 * @brief  Erase QSPI flash memory.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_qspi_flash_erase(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        UNUSED_ARG1(ctx);

        tcl_value_t *tcl_addr = tcl_list_at(args, 1);
        tcl_value_t *tcl_size = tcl_list_at(args, 2);

        uint32_t address = strtoul(tcl_addr, NULL, 0);
        uint32_t size = strtoul(tcl_size, NULL, 0);

        if (address >= QSPI_BASE_ADDRESS) {
                address -= QSPI_BASE_ADDRESS;
        } else {
                address = 0;
        }

        tcl_free(tcl_addr);
        tcl_free(tcl_size);

        int err = ENOENT;

        scope (FILE *nor = fopen(PATH_APP_QSPI, "r+"), nor, fclose(nor)) {

                const size_t sector_size = 4096;
                const size_t sectors_to_erase = CEILING(size, sector_size);

                for (size_t n = 0; n < sectors_to_erase; n++) {

                        printf("QSPI flash: erasing %d%%\n", (n * 100) / sectors_to_erase);

                        errno = 0;
                        err = ioctl(fileno(nor), IOCTL_QSPI__COMMAND, &qspi_wren);
                        if (err) {
                                err = errno;
                                break;
                        }

                        QSPI_command_t erase = qspi_se4k;
                        erase.address = (sector_size * n) + address;

                        errno = 0;
                        err = ioctl(fileno(nor), IOCTL_QSPI__COMMAND, &erase);
                        if (err) {
                                err = errno;
                                break;
                        }

                        errno = 0;
                        err = ioctl(fileno(nor), IOCTL_QSPI__WAIT_FOR_MEMORY_TO_BE_READY, &(u32_t) {1000});
                        if (err) {
                                err = errno;
                                break;
                        }
                }

                if (not err) {
                        printf("QSPI flash: erasing 100%%\n");
                } else {
                        printf("QSPI flash: erase error %d.\n", err);
                }
        }

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Write QSPI flash memory.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_qspi_flash_write(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        fpkg_t *fpkg = ctx;

        tcl_value_t *file_path = tcl_list_at(args, 1);
        tcl_value_t *tcl_addr = tcl_list_at(args, 2);

        uint32_t address = strtoul(tcl_addr, NULL, 0);

        if (address >= QSPI_BASE_ADDRESS) {
                address -= QSPI_BASE_ADDRESS;
        } else {
                address = 0;
        }

        int err = ENOENT;

        fpkg_file_t *img = fpkg__fopen(fpkg, file_path);
        FILE *nor = fopen(PATH_APP_QSPI, "r+");

        while (img and nor) {

                fpkg_stat_t stat;
                err = fpkg__fstat(img, &stat);
                if (err) {
                        break;
                }

                err = fpkg__fseek(img, 0, fpkg_seek_mode__set);
                if (err) {
                        break;
                }

                fseek(nor, address, SEEK_SET);

                size_t size = 0;

                while (true) {
                        u8_t buf[256];
                        size_t n = fpkg__fread(img, buf, sizeof(buf));
                        if (n > 0) {
                                if (fwrite(buf, 1, n, nor) == n) {
                                        size += n;
                                        printf("QSPI flash: programming %s... %u%% (%lu "
                                               "bytes).\n",
                                                file_path, (size * 100) / stat.size, size);
                                } else {
                                        puts("QSPI flash: write error.");
                                        break;
                                }

                        } else if (n == 0) {
                                puts("QSPI flash: done.");
                                break;

                        } else {
                                puts("QSPI flash: error.");
                                break;
                        }
                }

                break;
        }

        fpkg__fclose(img);
        fclose(nor);

        tcl_free(file_path);
        tcl_free(tcl_addr);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Erase flash memory.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_flash_erase(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        UNUSED_ARG1(ctx);

        tcl_value_t *tcl_addr = tcl_list_at(args, 1);
        tcl_value_t *tcl_size = tcl_list_at(args, 2);

        uint32_t address = strtoul(tcl_addr, NULL, 0);
        uint32_t size = strtoul(tcl_size, NULL, 0);

        if (address >= APP_BASE_ADDRESS) {
                address -= APP_BASE_ADDRESS;
        } else {
                address = 0;
        }

        tcl_free(tcl_addr);
        tcl_free(tcl_size);

        int err = ENOENT;

        scope (FILE *nor = fopen(PATH_APP_FLASH, "r+"), nor, fclose(nor)) {

                const size_t sector_size = SECTOR_SIZE;
                const size_t sectors_to_erase = CEILING(size, sector_size);
                const size_t first_sector = address / sector_size;
                const size_t last_sector = first_sector + sectors_to_erase;

                for (size_t n = first_sector; n < last_sector; n++) {

                        printf("Flash: erasing %d%%\n", ((n - first_sector) * 100) / sectors_to_erase);

                        errno = 0;
                        err = ioctl(fileno(nor), IOCTL_NVM__SECTOR_ERASE, &(const size_t) {n});
                        if (err) {
                                err = errno;
                                break;
                        }
                }

                if (not err) {
                        printf("Flash: erasing 100%%\n");
                } else {
                        printf("Flash: erase error %d.\n", err);
                }
        }

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Write flash memory.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_flash_write(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        fpkg_t *fpkg = ctx;

        tcl_value_t *file_path = tcl_list_at(args, 1);
        tcl_value_t *tcl_addr = tcl_list_at(args, 2);

        uint32_t address = strtoul(tcl_addr, NULL, 0);

        if (address >= APP_BASE_ADDRESS) {
                address -= APP_BASE_ADDRESS;
        } else {
                address = 0;
        }

        int err = ENOENT;

        fpkg_file_t *img = fpkg__fopen(fpkg, file_path);
        FILE *nor = fopen(PATH_APP_FLASH, "r+");

        while (img and nor) {

                fpkg_stat_t stat;
                err = fpkg__fstat(img, &stat);
                if (err) {
                        break;
                }

                err = fpkg__fseek(img, 0, fpkg_seek_mode__set);
                if (err) {
                        break;
                }

                fseek(nor, address, SEEK_SET);

                size_t size = 0;

                while (true) {
                        size_t n = fpkg__fread(img, global->buf, sizeof(global->buf));
                        if (n > 0) {
                                if (fwrite(global->buf, 1, n, nor) == n) {
                                        size += n;
                                        printf("Flash: programming %s... %u%% (%lu "
                                               "bytes).\n",
                                                file_path, (size * 100) / stat.size, size);
                                } else {
                                        puts("Flash: write error.");
                                        break;
                                }

                        } else if (n == 0) {
                                puts("Flash: done.");
                                break;

                        } else {
                                puts("Flash: error.");
                                break;
                        }
                }

                break;
        }

        fpkg__fclose(img);
        fclose(nor);

        tcl_free(file_path);
        tcl_free(tcl_addr);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Get file size from FPKG.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_get_file_size(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        fpkg_t *fpkg = ctx;

        tcl_value_t *file_path = tcl_list_at(args, 1);

        fpkg_stat_t stat;
        if (fpkg__stat(fpkg, file_path, &stat) != 0) {
                stat.size = 0;
        }

        tcl_free(file_path);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%lu", stat.size);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Extract file from FPKG.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of F* values.
 */
//==============================================================================
static int tcl_fpkg_extract(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        fpkg_t *fpkg = ctx;

        tcl_value_t *file_src = tcl_list_at(args, 1);
        tcl_value_t *file_dest = tcl_list_at(args, 2);

        int err = ENOENT;

        fpkg_stat_t stat;
        if (fpkg__stat(fpkg, file_src, &stat) != 0) {
                stat.size = 0;
        }

        fpkg_file_t *fsrc = fpkg__fopen(fpkg, file_src);
        FILE *fdest = fopen(file_dest, "w");

        size_t size = 0;

        while (fsrc and fdest) {
                size_t n = fpkg__fread(fsrc, global->buf, sizeof(global->buf));
                if (n > 0) {
                        errno = 0;
                        if (fwrite(global->buf, n, 1, fdest) != 1) {
                                err = errno;
                                break;
                        } else {
                                size += n;
                        }
                } else {
                        if (stat.size == size) {
                                printf("Extracting '%s' -> '%s' %u%% (%lu bytes).\n", file_src, file_dest,
                                        (size * 100) / stat.size, size);
                                err = 0;

                        } else {
                                printf("Extracting '%s' -> '%s' error.\n", file_src, file_dest);
                                err = EIO;
                        }
                        break;
                }
        }

        fpkg__fclose(fsrc);
        fclose(fdest);

        tcl_free(file_src);
        tcl_free(file_dest);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Remove file from FS.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of TCL_F* values.
 */
//==============================================================================
static int tcl_remove(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        UNUSED_ARG1(ctx);

        tcl_value_t *path = tcl_list_at(args, 1);

        printf("Removing '%s'...\n", path);

        int err = remove(path);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Create new dir on FS.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of TCL_F* values.
 */
//==============================================================================
static int tcl_mkdir(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        UNUSED_ARG1(ctx);

        tcl_value_t *path = tcl_list_at(args, 1);
        tcl_value_t *perm = tcl_list_at(args, 2);

        printf("Creating dir '%s'...\n", path);

        u32_t mode = strtoul(perm, NULL, 0);

        int err = mkdir(path, mode);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Rename file on FS.
 *
 * @param  tcl          TCL instance
 * @param  args         argument list
 * @param  ctx          user context
 *
 * @return One of TCL_F* values.
 */
//==============================================================================
static int tcl_rename(struct tcl *tcl, tcl_value_t *args, void *ctx)
{
        UNUSED_ARG1(ctx);

        tcl_value_t *path_old = tcl_list_at(args, 1);
        tcl_value_t *path_new = tcl_list_at(args, 2);

        printf("Renaming '%s' -> '%s'...\n", path_old, path_new);

        int err = rename(path_old, path_new);

        char r_str[8];
        snprintf(r_str, sizeof(r_str), "%d", err);
        return tcl_result(tcl, TCL_FNORMAL, tcl_alloc(r_str, strlen(r_str)));
}

//==============================================================================
/**
 * @brief  Start application.
 */
//==============================================================================
static void start_application(void)
{
        printf("Starting application at address %08lxh...\n", APP_BASE_ADDRESS);
        msleep(250);

        uint32_t *vect_table = cast(uint32_t *, APP_BASE_ADDRESS);

        if (vect_table[1] == 0xFFFFFFFF) {
                printf("No application in flash memory!\n");
                while (true) {
                        // action here
                        sleep(1);
                }
        }

        system_reboot_to_address(vect_table[1]);
}

//==============================================================================
/**
 * @brief Program main function
 *
 * @param  argc         count of arguments
 * @param *argv[]       argument table
 *
 * @return program status
 */
//==============================================================================
int main(int argc, char *argv[])
{
        UNUSED_ARG2(argc, argv);

        create_base_file_system_structure();
        initialize_basic_drivers();
        open_output_stream();
        show_kernel_panic_message();
        initialize_additional_drivers();
        mount_file_systems();
        print_system_log_messages();

        struct tcl tcl;
        fpkg_t *fpkg = fpkg__open(PATH_UPDATE_FILE, true);
        fpkg_file_t *manifest = fpkg__fopen(fpkg, FPKG_SECTION_MANIFEST);

        if (fpkg and manifest and (tcl_init(&tcl) == TCL_FNORMAL)) {

                tcl_register_const(&tcl, "qspi_flash_erase", tcl_qspi_flash_erase, 3, fpkg);

                tcl_register_const(&tcl, "qspi_flash_write", tcl_qspi_flash_write, 3, fpkg);

                tcl_register_const(&tcl, "flash_erase", tcl_flash_erase, 3, fpkg);

                tcl_register_const(&tcl, "flash_write", tcl_flash_write, 3, fpkg);

                tcl_register_const(&tcl, "get_file_size", tcl_get_file_size, 2, fpkg);

                tcl_register_const(&tcl, "fpkg_extract", tcl_fpkg_extract, 3, fpkg);

                tcl_register_const(&tcl, "remove", tcl_remove, 2, fpkg);

                tcl_register_const(&tcl, "mkdir", tcl_mkdir, 3, fpkg);

                tcl_register_const(&tcl, "rename", tcl_rename, 3, fpkg);

                fpkg_extract(fpkg, FPKG_SECTION_MANIFEST, PATH_VENDOR "/manifest.tmp");
                tcl_loadfile(&tcl, PATH_VENDOR "/manifest.tmp");
                remove(PATH_VENDOR "/manifest.tmp");

                tcl_destroy(&tcl);

                remove(PATH_UPDATE_FILE ".bak");
                rename(PATH_UPDATE_FILE, PATH_UPDATE_FILE ".bak");
        }

        fpkg__fclose(manifest);
        fpkg__close(fpkg);

        sync();
        umount("/sd");

        start_application();

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
