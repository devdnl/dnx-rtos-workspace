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
/* host disk connected by the simulator --rom option */
#define ROMFS_HOST_DISK 0

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
};

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
        driver_init("TTY", 0, 1, "/dev/tty1");
        driver_init("TTY", 0, 2, "/dev/tty2");
        driver_init("TTY", 0, 3, "/dev/tty3");

        driver_init("ZERO", 0, 0, "/dev/zero");
        driver_init("ZERO", 1, 0, "/dev/null");
        driver_init("URANDOM", 0, 0, "/dev/urandom");
}

//==============================================================================
/**
 * @brief Mount file systems. ROMFS image is connected by the simulator --rom
 *        option and it is mounted in the /etc folder. Host directory selected
 *        by the --hostfs option is mounted in the /host folder.
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
