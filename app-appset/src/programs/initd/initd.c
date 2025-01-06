/*==============================================================================
@file    initd.c

@author  Daniel Zorychta

@brief   Initialization daemon - example.

@note    Copyright (C) 2017 Daniel Zorychta <daniel.zorychta@gmail.com>

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

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/

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
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(initd, STACK_DEPTH_LOW);

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
         * NOTE: Number and naming of folders depends on user needs. There can
         *       be mounted additional file systems that does not requires
         *       drivers to work (e.g. procfs, devfs, etc). Remember that this
         *       stage does not give access to drivers because drivers will be
         *       initialized in the next runlevel.
         *
         *       Mounting file system is not necessary if user just want to use
         *       basic drivers without using files. In this case drivers can
         *       be initialized by using special path:
         *       "dev://<DRIVER-NAME>:<major>:<minor>"
         */

        /*
         * 1. Mount base file system as root.
         */
        mount("ramfs", "", "/", "");

        /*
         * 2. Create basic folders. This folders can be used to store device
         *    files or to mount next file systems e.g. SD Card etc.
         */
        mkdir("/dev", 0666);  // this folder store devices drivers
        mkdir("/tmp", 0666);  // this folder store temporary files
        mkdir("/run", 0666);  // this folder store runtime files
        mkdir("/mnt", 0666);  // this folder store SD card content
        mkdir("/proc", 0666); // this folder store SD card content
        mkdir("/etc", 0666);  // this folder store SD card content

        mount("procfs", "", "/proc", "");
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
         * NOTE: Drivers that are initialized in this stage can be reduced or
         *       added because this depends on user needs. There are drivers
         *       that cannot be initialized in this runlevel because are using
         *       access to basic drivers e.g. SDSPI (SD Card over SPI).
         *       The configuration of drivers can be done by using project
         *       configuration -- Configtool, or by editing configuration files
         *       directly. Some drivers need additional runtime configuration.
         *       In this case see driver's help.
         */

        /*
         * 1. Initialize GPIO drivers. Number of GPIO drivers depends on microcontroller
         *    pinout. If microcontroller has only 2 GPIO ports then only those
         *    two should be initialized. Not all GPIOs should have file representation.
         *    If file is not needed (because port is not used from application)
         *    then node is not necessary (use NULL or empty string as file name).
         */
        driver_init("GPIO", 0, 0, "/dev/gpio");
        // driver_init("GPIO", 1, 0, NULL);
        // driver_init("GPIO", 2, 0, NULL);
        // driver_init("GPIO", 3, 0, NULL);
        // driver_init("GPIO", 4, 0, NULL);
        // driver_init("GPIO", 5, 0, NULL);
        // driver_init("GPIO", 6, 0, NULL);
        // driver_init("GPIO", 7, 0, NULL);
        // driver_init("GPIO", 8, 0, NULL);
        // driver_init("GPIO", 9, 0, NULL);
        // driver_init("GPIO", 10, 0, NULL);
        // driver_init("GPIO", 11, 0, NULL);

        /*
         * 2. Next part of drivers that can be initialized at this early stage.
         */
        driver_init("AFM", 0, 0, NULL); // alternative function configuration
        driver_init("CLK", 0, 0, "/dev/clk"); // system clock configuration
        driver_init("DMA", 0, 0, NULL); // DMA configuration

        /*
         * NOTE: make sure that UART1 is used as terminal output!
         *       Some BSPs use UART2 as default terminal output.
         */
        driver_init("UART", __CPU_UART_TERM__, 0,
                "/dev/ttyS0"); // UART1 will be used as TTY I/O

        static const TTY_config_t TTY_config = {
                .input_file  = "/dev/ttyS0",
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
        int fd;

        /*
         * 1. Initialize next terminals that can be used to hold user applications.
         *    Number of terminals can be configured in the TTY module configuration
         *    by using Configtool. Number of terminals depends on user needs.
         *    In special cases TTY terminals does not need to be initialized.
         */
        driver_init("TTY", 0, 1, "/dev/tty1");
        driver_init("TTY", 0, 2, "/dev/tty2");
        driver_init("TTY", 0, 3, "/dev/tty3");

        /*
         * 2. If real time clock is needed then RTC driver should be initialized.
         */
        driver_init("RTC", 0, 0, "/dev/rtc");

        /*
         * 3. Ethernet interface initialization and MAC setup
         */
        driver_init("ETH", 0, 0, "/dev/eth");

        fd = open("/dev/eth", O_RDWR);
        if (fd >= 0) {
                ETHIF_config_t conf = {
                        .MAC              = {0xC2, 0x70, 0x50, 0xFF, 0xFF, 0x78},
                        .auto_negotiation = true,
                        .speed            = ETHIF_SPEED__100Mbps,
                        .duplex           = ETHIF_DUPLEX__FULL,
                };

                ioctl(fd, IOCTL_ETHIF__CONFIGURE, &conf);
                ioctl(fd, IOCTL_ETHIF__START);

                close(fd);
        } else {
                perror("eth");
        }
}

//==============================================================================
/**
 * @brief Mount file systems.
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

        /*
         * ROMFS at address 0x08070000
         */
        const u32_t        romfs_address   = 0x100C0000;
        const u32_t       *romfs_ptr       = cast(typeof(romfs_ptr), romfs_address);
        static const char *romfs_signature = "FPKG\n";

        if (memcmp(romfs_ptr, romfs_signature, strlen(romfs_signature)) == 0) {

                const u32_t romfs_size_offset_words = 16; // romfs header size
                u32_t       romfs_size              = *(romfs_ptr + romfs_size_offset_words);

                if (romfs_size != UINT32_MAX) {
                        driver_init2("MEMBLOCK", 0, 0, "/dev/romfs",
                                &(MEMBLOCK_config_t) {
                                        .memory_address = romfs_address,
                                        .block_size     = romfs_size,
                                        .access         = O_RDONLY,
                                });

                        mount("romfs", "/dev/romfs", "/etc", "validate");
                }
        }
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
 * @brief Function start DHCP client to get network address.
 */
//==============================================================================
static void initialize_network(void)
{
        ifadd("unix", NET_FAMILY__UNIX, NULL);
        ifup("unix", NULL, 0);

        /*
         * Internet/local network is configured in netmgr program by using
         * configuration files. If needed then "inet" configuration can be moved
         * here without using of netmgr program, example:
         *
         * ifadd("inet", NET_FAMILY__INET, "/dev/eth");
         *
         * static const NET_INET_config_t cfg_dhcp = {
         *         .mode    = NET_INET_MODE__DHCP_START,
         *         .address = NET_INET_IPv4_ANY,
         *         .mask    = NET_INET_IPv4_ANY,
         *         .gateway = NET_INET_IPv4_ANY,
         * };
         *
         * if (ifup("inet", &cfg_dhcp, sizeof(cfg_dhcp)) != 0) {
         *         perror("ifup");
         * }
         */
}

//==============================================================================
/**
 * @brief Function start user programs.
 */
//==============================================================================
static void start_user_programs(void)
{
        /*
         * 1. Start the first terminal emulator that can be accessed by clicking F1
         *    key. Terminal is connected to the tty0.
         */
        static const process_attr_t attr0 = {
                .cwd       = "/",
                .fd_stderr = -1,
                .fd_stdin  = -1,
                .fd_stdout = -1,
                .detached  = true,
                .p_stderr  = "/dev/tty0",
                .p_stdin   = "/dev/tty0",
                .p_stdout  = "/dev/tty0",
        };

        process_create("dsh", &attr0);

        /*
         * 2. Start second terminal emulator that can be accessed by clicking F2
         *    key. Terminal is connected to the tty1.
         */
        static const process_attr_t attr1 = {
                .cwd       = "/",
                .fd_stderr = -1,
                .fd_stdin  = -1,
                .fd_stdout = -1,
                .detached  = true,
                .p_stderr  = "/dev/tty1",
                .p_stdin   = "/dev/tty1",
                .p_stdout  = "/dev/tty1",
        };

        process_create("dsh", &attr1);

        /*
         * 3. Start user application that can be accessed by clicking F3
         *    key. Application is connected to the tty2.
         */
        static const process_attr_t attr2 = {
                .cwd       = "/",
                .fd_stderr = -1,
                .fd_stdin  = -1,
                .fd_stdout = -1,
                .detached  = true,
                .p_stderr  = "/dev/tty2",
                .p_stdin   = "/dev/tty2",
                .p_stdout  = "/dev/tty2",
        };

        process_create("netmgr --daemon /conf/netmgr.conf", &attr2);
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

        /*
         * NOTE: This procedure can be used as example. The order of functions
         *       should be preserved, because this order allows all drivers to
         *       be initialized correctly because of hierarchy.
         */
        create_base_file_system_structure();
        initialize_basic_drivers();
        open_output_stream();
        show_kernel_panic_message();
        initialize_additional_drivers();
        mount_file_systems();
        print_system_log_messages();
        initialize_network();
        start_user_programs();

        // If needed, the initd can continue work. It can be used as daemon.
        // while (true) {...}
        // Or can be closed if not needed anymore.
        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
