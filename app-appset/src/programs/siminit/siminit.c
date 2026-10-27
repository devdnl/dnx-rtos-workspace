/*==============================================================================
@file   siminit.c

@author Daniel Zorychta

@brief  Initialization daemon of the POSIX simulator.

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
#include <sys/time.h>
#include <dnx/misc.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/net.h>
#include <dnx/vt100.h>
#include <unistd.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
/* host disk with ROMFS image (simulator option: --disk 0=<file>:ro) */
#define ROMFS_HOST_DISK 0

/* host disks connected by the simulator --disk option (1 -> sda, 2 -> sdb, ...) */
#define FIRST_DATA_HOST_DISK 1
#define LAST_DATA_HOST_DISK  3

/* network configuration file (ROMFS) */
#define NETMGR_CONFIG_FILE "/etc/netmgr.conf"

/* number of primary partitions in MBR */
#define NUMBER_OF_PARTITIONS 4

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
        char str[100];
        char disk_path[16];
        char part_path[16];
        char mnt_path[24];
};

static const char *const FILE_SYSTEMS[] = {"fatfs", "ext4fs"};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(siminit, STACK_DEPTH_LOW);

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
        mkdir("/tmp", 0666);
        mkdir("/run", 0666);
        mkdir("/mnt", 0666);
        mkdir("/proc", 0666);
        mkdir("/etc", 0666);

        mount("procfs", "", "/proc", "");
}

//==============================================================================
/**
 * @brief Function initialize basic drivers. The UART0 is connected to the
 *        host terminal.
 */
//==============================================================================
static void initialize_basic_drivers(void)
{
        driver_init("RTC", 0, 0, "/dev/rtc");

        driver_init("GPIO", 0, 0, "/dev/gpio");

        driver_init("UART", 0, 0, "/dev/ttyS0");

        static const TTY_config_t TTY_config = {
                .input_file = "/dev/ttyS0",
                .output_file = "/dev/ttyS0",
        };
        driver_init2("TTY", 0, 0, "/dev/tty0", &TTY_config);
}

//==============================================================================
/**
 * @brief Function open output stream.
 */
//==============================================================================
static void open_output_stream(void)
{
        stdout = fopen("/dev/tty0", "r+");
        stderr = stdout;
}

//==============================================================================
/**
 * @brief Function initialize additional drivers.
 */
//==============================================================================
static void initialize_additional_drivers(void)
{
        /* UARTs connected by the simulator --uart option (not connected UART drops data) */
        driver_init("UART", 1, 0, "/dev/ttyS1");
        driver_init("UART", 2, 0, "/dev/ttyS2");
        driver_init("UART", 3, 0, "/dev/ttyS3");

        driver_init("TTY", 0, 1, "/dev/tty1");
        driver_init("TTY", 0, 2, "/dev/tty2");
        driver_init("TTY", 0, 3, "/dev/tty3");

        driver_init("ZERO", 0, 0, "/dev/zero");
        driver_init("ZERO", 1, 0, "/dev/null");
        driver_init("URANDOM", 0, 0, "/dev/urandom");
}

//==============================================================================
/**
 * @brief Function try to mount device by using all known file systems.
 *
 * @param dev           device path
 * @param mnt           mount point path
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool mount_device(const char *dev, const char *mnt)
{
        struct stat info;
        if ((stat(dev, &info) != 0) || (info.st_size == 0)) {
                return false;
        }

        mkdir(mnt, 0777);

        for (size_t i = 0; i < ARRAY_SIZE(FILE_SYSTEMS); i++) {
                if (mount(FILE_SYSTEMS[i], dev, mnt, "") == 0) {
                        return true;
                }
        }

        remove(mnt);

        return false;
}

//==============================================================================
/**
 * @brief Function initialize data disk connected by the simulator --disk
 *        option. Partitions are created according to MBR (/dev/sdX1..4) and
 *        mounted in the /mnt/sdX1..4 folders. If disk has not partitions then
 *        entire disk is mounted in the /mnt/sdX folder.
 *
 * @param disk          host disk number
 */
//==============================================================================
static void mount_data_disk(u8_t disk)
{
        char letter = 'a' + (disk - FIRST_DATA_HOST_DISK);

        snprintf(global->disk_path, sizeof(global->disk_path), "/dev/sd%c", letter);

        if (driver_init("HOSTDISK", disk, 0, global->disk_path) != 0) {
                return;
        }

        for (u8_t i = 0; i < NUMBER_OF_PARTITIONS; i++) {
                snprintf(global->part_path, sizeof(global->part_path), "%s%u", global->disk_path, i + 1);
                driver_init2("PART", disk, i, global->part_path,
                        &(const PART_config_t) {.path = global->disk_path});
        }

        bool mounted = false;

        snprintf(global->part_path, sizeof(global->part_path), "%s1", global->disk_path);
        int fd = open(global->part_path, O_RDWR);
        if (fd >= 0) {
                if (ioctl(fd, IOCTL_PART__READ_MBR) == 0) {
                        for (u8_t i = 0; i < NUMBER_OF_PARTITIONS; i++) {
                                snprintf(global->part_path, sizeof(global->part_path), "%s%u",
                                        global->disk_path, i + 1);
                                snprintf(global->mnt_path, sizeof(global->mnt_path), "/mnt/sd%c%u", letter,
                                        i + 1);
                                mounted |= mount_device(global->part_path, global->mnt_path);
                        }
                }
                close(fd);
        }

        if (!mounted) {
                snprintf(global->mnt_path, sizeof(global->mnt_path), "/mnt/sd%c", letter);
                mount_device(global->disk_path, global->mnt_path);
        }
}

//==============================================================================
/**
 * @brief Mount file systems. ROMFS image is connected by the simulator
 *        --disk 0=<file>:ro option and it is mounted in the /etc folder. Host directory selected
 *        by the --hostfs option is mounted in the /host folder. Disks selected
 *        by the --disk 1..3 options are mounted in the /mnt folder.
 */
//==============================================================================
static void mount_file_systems(void)
{
        if (driver_init("HOSTDISK", ROMFS_HOST_DISK, 0, "/dev/rom") == 0) {
                if (mount("romfs", "/dev/rom", "/etc", "validate") != 0) {
                        perror("romfs");
                }
        }

        /* host directory selected by the --hostfs option */
        mkdir("/host", 0777);
        if (mount("hostfs", "", "/host", "") != 0) {
                remove("/host");
        }

        /* disks selected by the --disk option */
        for (u8_t disk = FIRST_DATA_HOST_DISK; disk <= LAST_DATA_HOST_DISK; disk++) {
                mount_data_disk(disk);
        }
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
                printf("*** %s ***\n",
                        info.flags & KERNEL_PANIC_FLAG__KERNEL_PANIC ? "KERNEL PANIC" : "APP CRASH");
                printf("  Cause  : %s (%d)\n", info.cause_str, info.cause);
                printf("  PID    : %u (%.*s)\n", info.pid, sizeof(info.name), info.name);
                printf("  TID    : %u\n", info.tid);
                printf("  SYSCALL: %u\n", info.syscall);
                printf(VT100_RESET_ATTRIBUTES "\n");
        }
}

//==============================================================================
/**
 * @brief Function print system log messages.
 */
//==============================================================================
static void print_system_log_messages(void)
{
        struct timeval t = {0, 0};

        while (syslog_read(global->str, sizeof(global->str), &t, &t)) {
                printf("[%lld.%06lu] %s\n", t.tv_sec, t.tv_usec, global->str);
        }
}

//==============================================================================
/**
 * @brief Function initialize network.
 */
//==============================================================================
static void initialize_network(void)
{
        ifadd("unix", NET_FAMILY__UNIX, NULL);
        ifup("unix", NULL, 0);

        /*
         * Ethernet interface is available when simulator is started with
         * --tap <name> option. Interface is configured by the netmgr program.
         */
        if (driver_init("ETH", 0, 0, "/dev/eth") == 0) {
                static const process_attr_t attr = {
                        .cwd = "/",
                        .fd_stderr = -1,
                        .fd_stdin = -1,
                        .fd_stdout = -1,
                        .detached = true,
                        .p_stderr = "/dev/tty0",
                        .p_stdin = "/dev/null",
                        .p_stdout = "/dev/tty0",
                        .priority = PRIORITY_NORMAL,
                };

                process_create("netmgr --daemon " NETMGR_CONFIG_FILE, &attr);
        }
}

//==============================================================================
/**
 * @brief Function start shell on selected terminal.
 *
 * @param tty           terminal path
 */
//==============================================================================
static void start_shell(const char *tty)
{
        process_attr_t attr = {
                .cwd = "/",
                .fd_stderr = -1,
                .fd_stdin = -1,
                .fd_stdout = -1,
                .detached = true,
                .p_stderr = tty,
                .p_stdin = tty,
                .p_stdout = tty,
                .priority = PRIORITY_NORMAL,
        };

        process_create("dsh", &attr);
}

//==============================================================================
/**
 * @brief Function start user programs.
 */
//==============================================================================
static void start_user_programs(void)
{
        start_shell("/dev/tty0");
        start_shell("/dev/tty1");
        start_shell("/dev/tty2");
        start_shell("/dev/tty3");
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
        initialize_additional_drivers();
        mount_file_systems();
        show_kernel_panic_message();
        print_system_log_messages();
        initialize_network();
        start_user_programs();

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
