/*==============================================================================
@file    tui_port.h

@author  Daniel Zorychta

@brief   Text user interface library: platform layer (internal header).

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

#ifndef _TUI_PORT_H_
#define _TUI_PORT_H_

/*==============================================================================
  Include files
==============================================================================*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported functions
==============================================================================*/
/* switch terminal to raw mode (input and output descriptors); 0 on success, -1 on error (errno) */
extern int tui_port_open(void **ctx, int in_fd, int out_fd);

/* restore terminal mode and release context */
extern void tui_port_close(void *ctx);

/* enable/disable raw mode (e.g. when other program is started); 0 on success */
extern int tui_port_set_raw(void *ctx, bool raw);

/*
 * read available bytes; waits for at least one byte for timeout_ms
 * (timeout_ms < 0: forever); returns number of bytes, 0 on timeout, -1 on error
 */
extern int tui_port_read(void *ctx, uint8_t *buf, size_t size, int timeout_ms);

/* write all bytes; 0 on success, -1 on error */
extern int tui_port_write(void *ctx, const char *buf, size_t len);

/* monotonic time in milliseconds */
extern uint32_t tui_port_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* _TUI_PORT_H_ */
/*==============================================================================
  End of file
==============================================================================*/
