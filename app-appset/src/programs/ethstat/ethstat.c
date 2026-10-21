/*==============================================================================
File    ethstat.c

Author  Daniel Zorychta

Brief   Ethernet info

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
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>

/*==============================================================================
  Local macros
==============================================================================*/

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        ETHIF_status_t status;
};

static const char *ETH_STATE[] = {"RESET", "READY", "ERROR"};
static const char *ETH_LINK[] = {"DISCONNECTED", "CONNECTED", "PHY ERROR"};
static const char *ETH_SPEED[] = {"10", "100"};
static const char *ETH_DUPLEX[] = {"HALF", "FULL"};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(ethstat, STACK_DEPTH_LOW);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * Main program function.
 *
 * Note: Please adjust stack size according to programs needs.
 *
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int err = EXIT_FAILURE;

        if (argc == 1) {
                printf("Usage: %s <ethernet-interface-path>\n", argv[0]);

        } else {
                int fd = open(argv[1], O_RDONLY);
                if (fd != -1) {
                        err = ioctl(fd, IOCTL_ETHIF__GET_STATUS, &global->status);
                        if (!err) {
                                printf("Configured: %s\n", global->status.configured ? "YES" : "NO");
                                printf("State     : %s\n", ETH_STATE[global->status.state]);
                                printf("Link      : %s\n", ETH_LINK[global->status.link_status]);
                                printf("Duplex    : %s\n", ETH_DUPLEX[global->status.duplex]);
                                printf("Speed     : %s Mbps\n", ETH_SPEED[global->status.speed]);
                                printf("MAC       : %02x:%02x:%02x:%02x:%02x:%02x\n", global->status.MAC[0],
                                        global->status.MAC[1], global->status.MAC[2], global->status.MAC[3],
                                        global->status.MAC[4], global->status.MAC[5]);
                                printf("RX packets: %llu, %u dropped\n", global->status.rx_packets,
                                        global->status.rx_dropped_frames);
                                printf("RX bytes  : %llu\n", global->status.rx_bytes);
                                printf("TX packets: %llu\n", global->status.tx_packets);
                                printf("TX bytes  : %llu\n", global->status.tx_bytes);

                                err = EXIT_SUCCESS;

                        } else {
                                perror(argv[1]);
                        }

                        close(fd);

                } else {
                        perror(argv[1]);
                }
        }

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
