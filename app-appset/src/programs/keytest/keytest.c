/*==============================================================================
@file   keytest.c

@author Daniel Zorychta

@brief  Terminal raw mode test: shows bytes and decoded keys.

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
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dnx/os.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define KEY_BUF_LEN        32
#define SEQ_TIMEOUT_MS     50
#define SIZE_QUERY_TIMEOUT 500
#define POLL_INTERVAL_MS   5
#define BYTES_COLUMN_WIDTH 28
#define EXIT_KEY           0x18 /* Ctrl+X */
#define ESC                0x1B

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        const char *seq;
        const char *name;
} key_name_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static bool is_sequence_complete(const char *buf, size_t len);
static size_t read_key(char *buf, size_t size);
static void print_key(const char *buf, size_t len);
static void query_terminal_size(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        char key[KEY_BUF_LEN];
};

/* sequences sent by VT100/xterm, PuTTY/rxvt and Linux console */
static const key_name_t KEY_NAMES[] = {
        {"\033[A", "Up"},
        {"\033[B", "Down"},
        {"\033[C", "Right"},
        {"\033[D", "Left"},
        {"\033OA", "Up (app)"},
        {"\033OB", "Down (app)"},
        {"\033OC", "Right (app)"},
        {"\033OD", "Left (app)"},
        {"\033[H", "Home"},
        {"\033OH", "Home"},
        {"\033[1~", "Home"},
        {"\033[7~", "Home"},
        {"\033[F", "End"},
        {"\033OF", "End"},
        {"\033[4~", "End"},
        {"\033[8~", "End"},
        {"\033[2~", "Insert"},
        {"\033[3~", "Delete"},
        {"\033[5~", "PgUp"},
        {"\033[6~", "PgDn"},
        {"\033OP", "F1"},
        {"\033OQ", "F2"},
        {"\033OR", "F3"},
        {"\033OS", "F4"},
        {"\033[11~", "F1"},
        {"\033[12~", "F2"},
        {"\033[13~", "F3"},
        {"\033[14~", "F4"},
        {"\033[15~", "F5"},
        {"\033[16~", "F5"}, /* minicom (code 16 is not used by VT220/xterm) */
        {"\033[17~", "F6"},
        {"\033[18~", "F7"},
        {"\033[19~", "F8"},
        {"\033[20~", "F9"},
        {"\033[21~", "F10"},
        {"\033[23~", "F11"},
        {"\033[24~", "F12"},
        {"\033[[A", "F1"},
        {"\033[[B", "F2"},
        {"\033[[C", "F3"},
        {"\033[[D", "F4"},
        {"\033[[E", "F5"},
        {"\033[1;5A", "Ctrl+Up"},
        {"\033[1;5B", "Ctrl+Down"},
        {"\033[1;5C", "Ctrl+Right"},
        {"\033[1;5D", "Ctrl+Left"},
        {"\033[1;2A", "Shift+Up"},
        {"\033[1;2B", "Shift+Down"},
        {"\033[1;2C", "Shift+Right"},
        {"\033[1;2D", "Shift+Left"},
        {"\033[Z", "Shift+Tab"},
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(keytest, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Check if buffer contains complete key (byte or escape sequence).
 *
 * @param buf           buffer
 * @param len           number of bytes in buffer
 *
 * @return true if key is complete, false if more bytes are expected.
 */
//==============================================================================
static bool is_sequence_complete(const char *buf, size_t len)
{
        if (len == 0) {
                return false;
        }

        if (buf[0] != ESC) {
                /* UTF-8: number of bytes is defined by the first byte */
                u8_t c = buf[0];
                size_t n = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : (c >= 0xC0) ? 2 : 1;
                return len >= n;
        }

        if (len == 1) {
                return false; /* lone ESC is recognized by timeout */
        }

        if (buf[1] == 'O') {
                return len >= 3; /* SS3: ESC O x */
        }

        if (buf[1] != '[') {
                return true; /* Alt+key: ESC x */
        }

        if ((len >= 3) && (buf[2] == '[')) {
                return len >= 4; /* Linux console F1-F5: ESC [ [ x */
        }

        /* CSI: ESC [ parameters... final byte (0x40-0x7E) */
        for (size_t i = 2; i < len; i++) {
                if ((buf[i] >= 0x40) && (buf[i] <= 0x7E)) {
                        return true;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief Read one key. Waits for the first byte, then collects the rest of
 *        sequence for at most SEQ_TIMEOUT_MS.
 *
 * @param buf           destination buffer
 * @param size          buffer size
 *
 * @return Number of bytes of key, 0 on error.
 */
//==============================================================================
static size_t read_key(char *buf, size_t size)
{
        ssize_t n = read(fileno(stdin), buf, 1);
        if (n <= 0) {
                return 0;
        }

        size_t len = 1;

        if (!is_sequence_complete(buf, len)) {
                ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);

                clock_t tref = get_uptime_ms();

                while ((len < size) && !is_sequence_complete(buf, len) &&
                        ((get_uptime_ms() - tref) < SEQ_TIMEOUT_MS)) {

                        n = read(fileno(stdin), &buf[len], 1);
                        if (n > 0) {
                                len++;
                        } else {
                                msleep(POLL_INTERVAL_MS);
                        }
                }

                ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);
        }

        return len;
}

//==============================================================================
/**
 * @brief Print key bytes and its name.
 *
 * @param buf           key bytes
 * @param len           number of bytes
 */
//==============================================================================
static void print_key(const char *buf, size_t len)
{
        int col = 0;

        for (size_t i = 0; i < len; i++) {
                col += printf("%02X ", (u8_t)buf[i]);
        }

        printf("%*s", (col < BYTES_COLUMN_WIDTH) ? (BYTES_COLUMN_WIDTH - col) : 1, "");

        for (size_t i = 0; i < ARRAY_SIZE(KEY_NAMES); i++) {
                if ((strlen(KEY_NAMES[i].seq) == len) && (memcmp(KEY_NAMES[i].seq, buf, len) == 0)) {
                        printf("%s\r\n", KEY_NAMES[i].name);
                        return;
                }
        }

        u8_t c = buf[0];

        if (len == 1) {
                if (c == ESC) {
                        printf("Esc\r\n");
                } else if (c == '\r') {
                        printf("Enter\r\n");
                } else if (c == '\t') {
                        printf("Tab\r\n");
                } else if ((c == 0x7F) || (c == 0x08)) {
                        printf("Backspace\r\n");
                } else if (c < 0x20) {
                        printf("Ctrl+%c\r\n", c + '@');
                } else if (c < 0x7F) {
                        printf("'%c'\r\n", c);
                } else {
                        printf("?\r\n");
                }

        } else if ((c == ESC) && (len == 2) && ((u8_t)buf[1] >= 0x20) && ((u8_t)buf[1] < 0x7F)) {
                printf("Alt+%c\r\n", buf[1]);

        } else if (c >= 0xC0) {
                printf("'%.*s' (UTF-8)\r\n", (int)len, buf);

        } else if ((c == ESC) && (len >= 3) && ((buf[1] == '[') || (buf[1] == 'O'))) {
                /* readable form of not known sequence, e.g. "CSI 16 ~" */
                printf("unknown (%s %.*s %c)\r\n", (buf[1] == '[') ? "CSI" : "SS3", (int)(len - 3), &buf[2],
                        buf[len - 1]);

        } else {
                printf("unknown\r\n");
        }
}

//==============================================================================
/**
 * @brief Ask terminal about its size (cursor position report) and print it.
 *        The cursor is moved to the far bottom-right corner, then the terminal
 *        reports the real cursor position: ESC [ rows ; cols R.
 */
//==============================================================================
static void query_terminal_size(void)
{
        printf("\0337\033[999;999H\033[6n\0338");
        fflush(stdout);

        char *buf = global->key;
        size_t len = 0;

        ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);

        clock_t tref = get_uptime_ms();

        while ((len < sizeof(global->key) - 1) && ((get_uptime_ms() - tref) < SIZE_QUERY_TIMEOUT)) {
                ssize_t n = read(fileno(stdin), &buf[len], 1);
                if (n > 0) {
                        len++;
                        if (buf[len - 1] == 'R') {
                                break;
                        }
                } else {
                        msleep(POLL_INTERVAL_MS);
                }
        }

        ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);

        buf[len] = '\0';

        int rows = 0, cols = 0;
        char *csi = strstr(buf, "\033[");
        if (csi && (sscanf(csi + 2, "%d;%dR", &rows, &cols) == 2)) {
                printf("Terminal size: %d columns x %d rows\r\n", cols, rows);
        } else {
                printf("Terminal size: unknown (no cursor position report)\r\n");
        }
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        (void)argc;
        (void)argv;

        if (ioctl(fileno(stdin), IOCTL_TTY__RAW_MODE_ON) != 0) {
                perror("Raw mode is not available (stdin is not a terminal?)");
                return EXIT_FAILURE;
        }

        printf("Key test (raw mode). Press keys, Ctrl+X exits.\r\n");

        query_terminal_size();

        printf("%-*s%s\r\n", BYTES_COLUMN_WIDTH, "Bytes", "Key");

        for (;;) {
                size_t len = read_key(global->key, sizeof(global->key));
                if (len == 0) {
                        break;
                }

                print_key(global->key, len);

                if ((len == 1) && (global->key[0] == EXIT_KEY)) {
                        break;
                }
        }

        ioctl(fileno(stdin), IOCTL_TTY__RAW_MODE_OFF);

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
