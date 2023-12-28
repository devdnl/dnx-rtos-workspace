/*==============================================================================
File    bootloader.c

Author  Daniel Zorychta

Brief   Bootloader application example.

        Copyright (C) 2023 Daniel Zorychta <daniel.zorychta@gmail.com>

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation and modified by the dnx RTOS exception.

        NOTE: The modification  to the GPL is  included to allow you to
              distribute a combined work that includes dnx RTOS without
              being obliged to provide the source  code for proprietary
              components outside of the dnx RTOS.

        The dnx RTOS  is  distributed  in the hope  that  it will be useful,
        but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
        MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
        GNU General Public License for more details.

        Full license text is available on the following file: doc/license.txt.

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <dnx/misc.h>
#include <dnx/net.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/vt100.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define BOOTLOADER_VERSION "1.0"
#define APP_BASE_ADDRESS   0x08010000
#define PATH_UPDATE_FILE   "/sd/vendor/app.img"
#define PATH_NVM_APP       "/dev/app"

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
        u8_t buf[65536];

        struct {
                u32_t total;
                u32_t current;
        } progress;
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
        mount("ramfs", "", "/", "");
        mkdir("/dev", 0666);
        mkdir("/sd", 0666);
        mkdir("/sd/vendor", 0666);
}

//==============================================================================
/**
 * @brief Function initialize basic drivers needed by CPU configuration or board
 *        specification.
 */
//==============================================================================
static void initialize_basic_drivers(void)
{
        driver_init("GPIO", 0, 0, "/dev/gpio");
        driver_init("GPIO", 1, 0, NULL);
        driver_init("GPIO", 2, 0, NULL);
        driver_init("GPIO", 3, 0, NULL);
        driver_init("GPIO", 4, 0, NULL);
        driver_init("GPIO", 5, 0, NULL);
        driver_init("GPIO", 6, 0, NULL);
        driver_init("GPIO", 7, 0, NULL);
        driver_init("AFM", 0, 0, NULL);
        driver_init("CLK", 0, 0, "/dev/clk");
        driver_init("DMA", 0, 0, NULL);
        driver_init("UART", __CPU_UART_TERM__, 0, "/dev/ttyS0");
        driver_init("ZERO", 0, 0, "/dev/zero");
        driver_init("ZERO", 1, 0, "/dev/null");

        driver_init2("TTY", 0, 0, "/dev/tty0",
                &(const TTY_config_t) {
                        .input_file  = "/dev/ttyS0",
                        .output_file = "/dev/ttyS0",
                });

        stdout = fopen("/dev/tty0", "r+");
        stderr = stdout;

        printf("Bootloader version %s\n", BOOTLOADER_VERSION);

        driver_init2("NVM", 0, 0, PATH_NVM_APP,
                &(const NVM_config_t) {
                        .start_sector      = 6,
                        .number_of_sectors = 18,
                });

        driver_init("TTY", 0, 1, "/dev/tty1");

        driver_init("SDMMC", 0, 0, "/dev/sd");
}

//==============================================================================
/**
 * @brief Show kernel panic message.
 */
//==============================================================================
static void show_kernel_panic_message(void)
{
        kernel_panic_info_t info;
        if (get_kernel_panic_info(&info)) {
                printf(VT100_FONT_COLOR_RED);
                printf("*** %s ***\n", info.kernel_panic ? "KERNEL PANIC" : "APP CRASH");
                printf("  Cause: %s (%d)\n", info.cause_str, info.cause);
                printf("  PID  : %u (%.*s)\n", info.pid, 256, info.name);
                printf("  TID  : %u\n", info.tid);
                printf("  SPACE: %s\n", info.kernelspace ? "kernel" : "user");
                printf(VT100_RESET_ATTRIBUTES "\n");

                sleep(3);
        }
}

//==============================================================================
/**
 * @brief Mount additional file systems.
 */
//==============================================================================
static void mount_file_systems(void)
{
        if (mount("flatfs", "/dev/sd", "/sd", "") != 0) {
                perror("/dev/sd");
        }
}

//==============================================================================
/**
 * @brief  Calculate total progress.
 */
//==============================================================================
static void calculate_progress(void)
{
        global->progress.total   = 0;
        global->progress.current = 0;

        /*
         * Erase size
         */
        int app_nvm = open(PATH_NVM_APP, O_RDONLY);
        if (app_nvm != -1) {
                size_t sector_count;
                ioctl(app_nvm, IOCTL_NVM__GET_SECTOR_COUNT, &sector_count);

                for (size_t i = 0; i < sector_count; i++) {
                        NVM_sector_size_t sector_size;
                        sector_size.sector = i;
                        ioctl(app_nvm, IOCTL_NVM__GET_SECTOR_SIZE, &sector_size);
                        global->progress.total += sector_size.size;
                }

                close(app_nvm);
        }

        /*
         * Programming size
         */
        struct stat update_stat;
        if (stat(PATH_UPDATE_FILE, &update_stat) == 0) {
                global->progress.total += update_stat.st_size;
        }
}

//==============================================================================
/**
 * @brief  Update progress.
 *
 * @param  size         update size
 */
//==============================================================================
static void update_progress(size_t size)
{
        global->progress.current += size;
}

//==============================================================================
/**
 * @brief  Check if update file is valid.
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool is_update_file_valid(void)
{
        bool valid = stat(PATH_UPDATE_FILE, &(struct stat) {}) == 0;

        return valid;
}

//==============================================================================
/**
 * @brief  Erase application sectors.
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool erase_application(void)
{
        bool success = false;

        scope (int fd = open(PATH_NVM_APP, O_RDWR), fd != -1, close(fd)) {

                size_t num_of_sectors = 0;

                errno = 0;
                if (ioctl(fd, IOCTL_NVM__GET_SECTOR_COUNT, &num_of_sectors) != 0) {
                        perror("Get number of sectors");
                        break;
                }

                success = true;

                for (size_t i = 0; i < num_of_sectors; i++) {
                        printf("Erasing sector %lu of %lu...\n", i + 1, num_of_sectors);
                        msleep(50);

                        errno = 0;
                        if (ioctl(fd, IOCTL_NVM__SECTOR_ERASE, &i) != 0) {
                                perror("Sector erase error");
                                success = false;
                                break;
                        }

                        errno                         = 0;
                        NVM_sector_size_t sector_size = {.sector = i};
                        if (ioctl(fd, IOCTL_NVM__GET_SECTOR_SIZE, &sector_size) != 0) {
                                perror("Sector size error");
                                success = false;
                                break;
                        }

                        update_progress(sector_size.size);
                }
        }

        return success;
}

//==============================================================================
/**
 * @brief  Flash application.
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool flash_aplication(void)
{
        FILE    *update       = NULL;
        FILE    *app          = NULL;
        uint32_t flashed_size = 0;

        update = fopen(PATH_UPDATE_FILE, "r");
        if (not update) {
                printf("Unable to open update app file.\n");
                goto cleanup;
        }

        struct stat update_stat = {0};
        if (fstat(fileno(update), &update_stat) != 0) {
                printf("Unable to get update app file stats.\n");
                goto cleanup;
        }

        app = fopen(PATH_NVM_APP, "r+");
        if (not app) {
                printf("Unable to open NVM app file.\n");
                goto cleanup;
        }

        while (app and update) {
                printf("Flashing: %u%%\n", flashed_size * 100 / update_stat.st_size);

                int n = fread(global->buf, 1, min(32768, sizeof(global->buf)), update);
                if (n == 0) {
                        break;

                } else if (n < 0) {
                        printf("Programming error.\n");
                        break;
                }

                errno = 0;
                int m = fwrite(global->buf, 1, n, app);
                if (m != n) {
                        perror("Flashing error");
                        break;
                }

                update_progress(n);

                flashed_size += n;
        }

cleanup:
        fclose(app);
        fclose(update);

        return flashed_size == update_stat.st_size;
}

//==============================================================================
/**
 * @brief  Start application.
 */
//==============================================================================
static void start_application(void)
{
        printf("Starting application...\n");
        msleep(250);

        uint32_t *vect_table = cast(uint32_t *, APP_BASE_ADDRESS);

        if (vect_table[1] == 0xFFFFFFFF) {
                printf("No application in flash memory!\n");
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
        show_kernel_panic_message();
        mount_file_systems();

        if (is_update_file_valid()) {

                calculate_progress();

                bool general_success = false;

                for (int try = 0; try < 3; try++) {

                        printf("Update try %d...\n", try + 1);

                        if (not erase_application()) {
                                continue;
                        }

                        if (not flash_aplication()) {
                                continue;
                        }

                        general_success = true;

                        break;
                }

                if (general_success) {
                        printf("Update finished successfully.\n");
                } else {
                        printf("Update failed\n");
                }
        }

        remove(PATH_UPDATE_FILE);

        start_application();

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
