/*==============================================================================
@file    tui_port.c

@author  Daniel Zorychta

@brief   Text user interface library: platform layer.

         dnx RTOS port is used by default. POSIX port (termios) is compiled
         when TUI_PORT_POSIX is defined, it allows to develop and test TUI
         programs on PC.

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
#include "tui_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#if defined(TUI_PORT_POSIX)
#include <termios.h>
#include <time.h>
#include <sys/select.h>
#else
#include <sys/ioctl.h>
#include <dnx/os.h>
#endif

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define POLL_INTERVAL_MS 5

/*==============================================================================
  Function definitions
==============================================================================*/

typedef struct {
        int in;
        int out;
#if defined(TUI_PORT_POSIX)
        struct termios saved;
#endif
} port_t;

#if defined(TUI_PORT_POSIX)
/*------------------------------------------------------------------------------
  POSIX port
------------------------------------------------------------------------------*/
int tui_port_open(void **ctx, int in_fd, int out_fd)
{
        port_t *port = malloc(sizeof(port_t));
        if (!port) {
                return -1;
        }

        port->in = in_fd;
        port->out = out_fd;

        if (tcgetattr(in_fd, &port->saved) != 0) {
                free(port);
                return -1;
        }

        struct termios raw = port->saved;
        cfmakeraw(&raw); /* no output processing, as dnx raw mode */
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(in_fd, TCSAFLUSH, &raw) != 0) {
                free(port);
                return -1;
        }

        *ctx = port;

        return 0;
}

void tui_port_close(void *ctx)
{
        port_t *port = ctx;

        if (port) {
                tcsetattr(port->in, TCSAFLUSH, &port->saved);
                free(port);
        }
}

int tui_port_set_raw(void *ctx, bool raw)
{
        port_t *port = ctx;

        struct termios mode = port->saved;
        if (raw) {
                cfmakeraw(&mode);
                mode.c_cc[VMIN] = 1;
                mode.c_cc[VTIME] = 0;
        }

        return tcsetattr(port->in, TCSAFLUSH, &mode);
}

int tui_port_read(void *ctx, uint8_t *buf, size_t size, int timeout_ms)
{
        port_t *port = ctx;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(port->in, &fds);

        struct timeval tv = {.tv_sec = timeout_ms / 1000, .tv_usec = (timeout_ms % 1000) * 1000};

        int r = select(port->in + 1, &fds, NULL, NULL, (timeout_ms < 0) ? NULL : &tv);
        if (r < 0) {
                return (errno == EINTR) ? 0 : -1;
        } else if (r == 0) {
                return 0;
        }

        ssize_t n = read(port->in, buf, size);

        return (n <= 0) ? -1 : (int)n;
}

int tui_port_write(void *ctx, const char *buf, size_t len)
{
        port_t *port = ctx;

        while (len) {
                ssize_t n = write(port->out, buf, len);
                if (n < 0) {
                        if (errno == EINTR) {
                                continue;
                        }

                        return -1;
                }

                buf += n;
                len -= n;
        }

        return 0;
}

uint32_t tui_port_time_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#else
/*------------------------------------------------------------------------------
  dnx RTOS port
------------------------------------------------------------------------------*/
int tui_port_open(void **ctx, int in_fd, int out_fd)
{
        port_t *port = malloc(sizeof(port_t));
        if (!port) {
                return -1;
        }

        port->in = in_fd;
        port->out = out_fd;

        if (ioctl(in_fd, IOCTL_TTY__RAW_MODE_ON) != 0) {
                free(port);
                return -1;
        }

        *ctx = port;

        return 0;
}

void tui_port_close(void *ctx)
{
        port_t *port = ctx;

        if (port) {
                ioctl(port->in, IOCTL_TTY__RAW_MODE_OFF);
                free(port);
        }
}

int tui_port_set_raw(void *ctx, bool raw)
{
        port_t *port = ctx;

        return ioctl(port->in, raw ? IOCTL_TTY__RAW_MODE_ON : IOCTL_TTY__RAW_MODE_OFF);
}

int tui_port_read(void *ctx, uint8_t *buf, size_t size, int timeout_ms)
{
        port_t *port = ctx;

        if (timeout_ms < 0) {
                /* in raw mode read waits for the first byte and returns all available bytes */
                ssize_t n = read(port->in, buf, size);
                return (n <= 0) ? -1 : (int)n;
        }

        ioctl(port->in, IOCTL_VFS__NON_BLOCKING_RD_MODE);

        uint32_t tref = tui_port_time_ms();
        ssize_t n;

        for (;;) {
                n = read(port->in, buf, size);
                if (n != 0) {
                        break;
                }

                if ((tui_port_time_ms() - tref) >= (uint32_t)timeout_ms) {
                        break;
                }

                msleep(POLL_INTERVAL_MS);
        }

        ioctl(port->in, IOCTL_VFS__DEFAULT_RD_MODE);

        return (n < 0) ? -1 : (int)n;
}

int tui_port_write(void *ctx, const char *buf, size_t len)
{
        port_t *port = ctx;

        while (len) {
                ssize_t n = write(port->out, buf, len);
                if (n <= 0) {
                        return -1;
                }

                buf += n;
                len -= n;
        }

        return 0;
}

uint32_t tui_port_time_ms(void)
{
        return (uint32_t)get_uptime_ms();
}
#endif

/*==============================================================================
  End of file
==============================================================================*/
