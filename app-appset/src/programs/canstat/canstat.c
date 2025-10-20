/*==============================================================================
File    canstat.c

Author  Daniel Zorychta

Brief   CAN info

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
#include <errno.h>

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
        CAN_status_t status;
};

static const char *CAN_MODE[] = {"INIT", "NORMAL", "SLEEP"};
static const char *BUS_STATUS[] = {"OK", "WARNING", "PASSIVE", "OFF"};
static const char *CAN_FRAME_MODE[] = {"CLASSIC", "FD without BRS", "FD with BRS"};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(canstat, STACK_DEPTH_LOW);

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
                printf("Usage: %s <can-interface-path>\n", argv[0]);

        } else {
                int fd = open(argv[1], O_RDONLY);
                if (fd != -1) {
                        errno = 0;
                        err = ioctl(fd, IOCTL_CAN__GET_STATUS, &global->status);
                        if (!err) {
                                printf("Nominal speed: %lu bps\n", global->status.nominal_baud_bps);
                                printf("Data speed   : %lu bps\n", global->status.data_baud_bps);
                                printf("Mode         : %s\n", CAN_MODE[global->status.mode]);
                                printf("Frame mode   : %s\n", CAN_FRAME_MODE[global->status.frame_mode]);
                                printf("Bus status   : %s\n", BUS_STATUS[global->status.can_bus_status]);
                                printf("RX errors    : %u\n", global->status.rx_error_ctr);
                                printf("TX errors    : %u\n", global->status.tx_error_ctr);
                                printf("RX overruns  : %lu\n", global->status.rx_overrun_ctr);
                                printf("TX frames    : %llu (%llu bytes)\n", global->status.tx_frames,
                                        global->status.tx_bytes);
                                printf("RX frames    : %llu (%llu bytes)\n", global->status.rx_frames,
                                        global->status.rx_bytes);
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
