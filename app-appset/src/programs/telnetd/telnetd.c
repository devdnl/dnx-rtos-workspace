/*==============================================================================
@file   telnetd.c

@author Daniel Zorychta

@brief  Telnet server. Each connection gets own terminal (TTY driver instance
        connected to pipes), so line editing and full-screen programs work.

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
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dnx/net.h>
#include <dnx/thread.h>
#include <dnx/os.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define NUMBER_OF_CONNECTIONS 3
#define BUF_SIZE              128
#define PATH_LEN              24
#define PROGRAM_NAME          "dsh"
#define RECEIVE_TIMEOUT       100
#define SEND_TIMEOUT          3000
#define TELNET_PORT           23
#define TTY_MAJOR_BASE        10 /* TTY driver major numbers of connections */
#define TTY_RELEASE_ATTEMPTS  10
#define KILL_TIMEOUT          5000

/* telnet protocol (RFC 854, 857, 858) */
#define T_IAC  255
#define T_DONT 254
#define T_DO   253
#define T_WONT 252
#define T_WILL 251
#define T_SB   250
#define T_SE   240
#define T_ECHO 1
#define T_SGA  3

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef enum {
        TELNET_DATA,
        TELNET_IAC,
        TELNET_OPTION,
        TELNET_SB,
        TELNET_SB_IAC,
} telnet_state_t;

typedef struct {
        telnet_state_t state;
        uint8_t cmd;
} telnet_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        const char *msg;
        mutex_t *mtx;
        uint8_t conn_cnt;
        bool slot_used[NUMBER_OF_CONNECTIONS];
};

static const NET_INET_sockaddr_t IP_ADDR_ANY = {.addr = NET_INET_IPv4_ANY, .port = TELNET_PORT};

static const thread_attr_t thread_attr = {.priority = PRIORITY_NORMAL,
        .stack_depth = STACK_DEPTH_LOW,
        .detached = true};

/* server echoes characters and does not use go-ahead: client works in character mode */
static const uint8_t TELNET_INIT[] = {T_IAC, T_WILL, T_ECHO, T_IAC, T_WILL, T_SGA};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(telnetd, STACK_DEPTH_VERY_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Create pipe with individual name and open created file
 * @param[in]  slot         connection slot used to create pipe name
 * @param[in]  c            additional character used to create pipe name
 * @param[out] f            opened pipe
 * @return On success pointer to name buffer is returned, otherwise NULL.
 */
//==============================================================================
static char *create_and_open_pipe(int slot, char c, FILE **f)
{
        char *name = calloc(1, PATH_LEN);
        if (name) {
                snprintf(name, PATH_LEN, "/run/telnet%d%c", slot, c);

                remove(name);

                if (mkfifo(name, 0666) == 0) {
                        *f = fopen(name, "r+");
                        if (*f) {
                                return name;
                        }

                        remove(name);
                }

                free(name);
                name = NULL;
        }

        return name;
}

//==============================================================================
/**
 * @brief  Allocate connection slot (terminal number).
 * @return Slot number or -1 if all slots are used.
 */
//==============================================================================
static int slot_alloc(void)
{
        int slot = -1;

        if (mutex_lock(global->mtx, MAX_DELAY_MS)) {
                for (int i = 0; i < NUMBER_OF_CONNECTIONS; i++) {
                        if (!global->slot_used[i]) {
                                global->slot_used[i] = true;
                                slot = i;
                                break;
                        }
                }

                mutex_unlock(global->mtx);
        }

        return slot;
}

//==============================================================================
/**
 * @brief  Release connection slot.
 * @param  slot         slot number
 */
//==============================================================================
static void slot_free(int slot)
{
        if ((slot >= 0) && mutex_lock(global->mtx, MAX_DELAY_MS)) {
                global->slot_used[slot] = false;
                mutex_unlock(global->mtx);
        }
}

//==============================================================================
/**
 * @brief  Release terminal of connection. Terminal can be busy for a while
 *         when killed program was using it, so release is repeated.
 * @param  slot         slot number
 * @return true on success.
 */
//==============================================================================
static bool tty_release(int slot)
{
        for (int i = 0; i < TTY_RELEASE_ATTEMPTS; i++) {
                if (driver_release("TTY", TTY_MAJOR_BASE + slot, 0) == 0) {
                        return true;
                }

                msleep(100);
        }

        return false;
}

//==============================================================================
/**
 * @brief  Parse data received from telnet client. Protocol commands are
 *         removed, answers to option negotiation are prepared.
 *
 * @param  t            telnet parser state
 * @param  buf          received data, data for terminal are stored in place
 * @param  len          number of received bytes
 * @param  reply        answer buffer (min. len + 3 bytes)
 * @param  reply_len    number of answer bytes (increased)
 *
 * @return Number of data bytes for terminal.
 */
//==============================================================================
static size_t telnet_input(telnet_t *t, uint8_t *buf, size_t len, uint8_t *reply, size_t *reply_len)
{
        size_t n = 0;

        for (size_t i = 0; i < len; i++) {
                uint8_t c = buf[i];

                switch (t->state) {
                case TELNET_DATA:
                        if (c == T_IAC) {
                                t->state = TELNET_IAC;
                        } else {
                                buf[n++] = c;
                        }
                        break;

                case TELNET_IAC:
                        if (c == T_IAC) {
                                buf[n++] = c; /* escaped 0xFF */
                                t->state = TELNET_DATA;
                        } else if ((c >= T_WILL) && (c <= T_DONT)) {
                                t->cmd = c;
                                t->state = TELNET_OPTION;
                        } else if (c == T_SB) {
                                t->state = TELNET_SB;
                        } else {
                                t->state = TELNET_DATA; /* NOP, GA, BRK, ... */
                        }
                        break;

                case TELNET_OPTION: {
                        /*
                         * Echo and suppress go-ahead are supported (offered at
                         * connection start). Other options are refused.
                         */
                        uint8_t answer = 0;
                        bool supported = (c == T_ECHO) || (c == T_SGA);

                        if ((t->cmd == T_DO) && !supported) {
                                answer = T_WONT;
                        } else if (t->cmd == T_WILL) {
                                answer = (c == T_SGA) ? T_DO : T_DONT;
                        }

                        if (answer) {
                                reply[(*reply_len)++] = T_IAC;
                                reply[(*reply_len)++] = answer;
                                reply[(*reply_len)++] = c;
                        }

                        t->state = TELNET_DATA;
                        break;
                }

                case TELNET_SB:
                        if (c == T_IAC) {
                                t->state = TELNET_SB_IAC;
                        }
                        break;

                case TELNET_SB_IAC:
                        t->state = (c == T_SE) ? TELNET_DATA : TELNET_SB;
                        break;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief  Send data to telnet client (0xFF byte is escaped).
 *
 * @param  sock         socket
 * @param  data         data
 * @param  len          number of bytes
 * @param  tmp          temporary buffer (min. 2 * len bytes)
 *
 * @return true on success.
 */
//==============================================================================
static bool telnet_send(SOCKET *sock, const uint8_t *data, size_t len, uint8_t *tmp)
{
        size_t n = 0;

        for (size_t i = 0; i < len; i++) {
                tmp[n++] = data[i];

                if (data[i] == T_IAC) {
                        tmp[n++] = T_IAC;
                }
        }

        return socket_write(sock, tmp, n) == (int)n;
}

//==============================================================================
/**
 * @brief  Telnet thread
 * @param  arg   thread argument - connection
 * @return None
 */
//==============================================================================
static int telnet_thread(void *arg)
{
        SOCKET *sock = arg;
        FILE *fin = NULL;
        FILE *fout = NULL;
        uint8_t *buf = NULL;
        uint8_t *tmp = NULL;
        pid_t proc = 0;
        char *pipe_in_name = NULL;
        char *pipe_out_name = NULL;
        char tty_path[PATH_LEN] = {0};
        bool tty_ok = false;
        telnet_t telnet = {.state = TELNET_DATA};
        int slot = slot_alloc();

        // print client IP
        NET_INET_sockaddr_t addr;
        socket_get_address(sock, &addr, sizeof(addr));

        printf("New connection from: %d.%d.%d.%d\n", NET_INET_IPv4_a(addr.addr), NET_INET_IPv4_b(addr.addr),
                NET_INET_IPv4_c(addr.addr), NET_INET_IPv4_d(addr.addr));

        if (slot < 0) {
                goto exit;
        }

        // buffers for data and for escaped data (or answers)
        buf = malloc(BUF_SIZE);
        tmp = malloc(BUF_SIZE * 2 + 3);
        if (!buf || !tmp) {
                goto exit;
        }

        // terminal input pipe (data from client)
        pipe_in_name = create_and_open_pipe(slot, 'i', &fin);
        if (!pipe_in_name) {
                goto exit;
        }

        // terminal output pipe (data to client)
        pipe_out_name = create_and_open_pipe(slot, 'o', &fout);
        if (!pipe_out_name) {
                goto exit;
        }

        // terminal of connection: TTY driver instance connected to pipes
        snprintf(tty_path, sizeof(tty_path), "/dev/telnet%d", slot);
        remove(tty_path);

        TTY_config_t tty_config = {.input_file = pipe_in_name, .output_file = pipe_out_name};
        if (driver_init2("TTY", TTY_MAJOR_BASE + slot, 0, tty_path, &tty_config) != 0) {
                // terminal left by previous connection (release failed)
                tty_release(slot);
                remove(tty_path);
                tty_ok = driver_init2("TTY", TTY_MAJOR_BASE + slot, 0, tty_path, &tty_config) == 0;
        } else {
                tty_ok = true;
        }

        if (!tty_ok) {
                perror("telnetd: terminal not initialized");
                goto exit;
        }

        // start shell on terminal
        process_attr_t process_attr = {.cwd = "/",
                .fd_stdin = -1,
                .fd_stdout = -1,
                .fd_stderr = -1,
                .p_stdin = tty_path,
                .p_stdout = tty_path,
                .p_stderr = tty_path,
                .priority = PRIORITY_NORMAL,
                .detached = false};

        proc = process_create(PROGRAM_NAME, &process_attr);
        if (proc == 0) {
                goto exit;
        }

        socket_set_send_timeout(sock, SEND_TIMEOUT);

        if (socket_write(sock, TELNET_INIT, sizeof(TELNET_INIT)) != (int)sizeof(TELNET_INIT)) {
                goto exit;
        }

        u32_t recv_timeout = RECEIVE_TIMEOUT;

        // handle telnet connection
        while (true) {
                socket_set_recv_timeout(sock, recv_timeout);

                if (recv_timeout < RECEIVE_TIMEOUT) {
                        recv_timeout += 10;
                        recv_timeout = min(recv_timeout, RECEIVE_TIMEOUT);
                }

                // receive input packet from telnet client
                errno = 0;
                int len = socket_read(sock, buf, BUF_SIZE);

                if ((len == -1) && (errno != ETIME)) {
                        break;
                }

                // data to terminal, answers to client
                if (len > 0) {
                        size_t reply_len = 0;
                        size_t n = telnet_input(&telnet, buf, len, tmp, &reply_len);

                        if (n > 0) {
                                fwrite(buf, 1, n, fin);
                        }

                        if ((reply_len > 0) && (socket_write(sock, tmp, reply_len) != (int)reply_len)) {
                                break;
                        }

                        recv_timeout = 0;
                }

                // send data from terminal
                bool sent = true;
                do {
                        ioctl(fileno(fout), IOCTL_VFS__NON_BLOCKING_RD_MODE);
                        len = fread(buf, 1, BUF_SIZE, fout);
                        if (len > 0) {
                                sent = telnet_send(sock, buf, len, tmp);
                                recv_timeout = 0;
                        }
                } while ((len > 0) && sent);

                // check if program is finished
                if (!sent || (process_wait(proc, NULL, 0) == 0)) {
                        break;
                }
        }

exit:
        // shell is killed when client disconnected, process is removed from zombie list
        if (proc && (process_kill(proc) == 0)) {
                process_wait(proc, NULL, KILL_TIMEOUT);
        }

        // terminal is released before its pipes
        if (tty_ok) {
                tty_release(slot);
                remove(tty_path);
        }

        if (fin) {
                fclose(fin);
        }

        if (fout) {
                fclose(fout);
        }

        if (pipe_in_name) {
                remove(pipe_in_name);
                free(pipe_in_name);
        }

        if (pipe_out_name) {
                remove(pipe_out_name);
                free(pipe_out_name);
        }

        free(buf);
        free(tmp);

        NET_INET_sockaddr_t sockaddr;
        socket_get_address(sock, &sockaddr, sizeof(sockaddr));
        printf("Connection closed: %d.%d.%d.%d\n", NET_INET_IPv4_a(sockaddr.addr),
                NET_INET_IPv4_b(sockaddr.addr), NET_INET_IPv4_c(sockaddr.addr),
                NET_INET_IPv4_d(sockaddr.addr));

        socket_close(sock);

        slot_free(slot);

        if (mutex_lock(global->mtx, MAX_DELAY_MS)) {
                global->conn_cnt--;
                mutex_unlock(global->mtx);
        }

        return 0;
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
        (void)argc;
        (void)argv;

        errno = 0;

        mkdir("/run", 0777);

        NET_INET_status_t netstat = {0};

        do {
                NET_family_t family;
                ifstatus("inet", &family, &netstat, sizeof(netstat));
                msleep(500);
        } while (!((netstat.state == NET_INET_STATE__DHCP_CONFIGURED) ||
                (netstat.state == NET_INET_STATE__STATIC_IP)));

        SOCKET *listener = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
        if (!listener) {
                global->msg = "Connection failed";
                goto exit;
        }

        global->mtx = mutex_new(MUTEX_TYPE_NORMAL);
        if (!global->mtx) {
                global->msg = "Mutex not created";
                goto exit;
        }

        if (socket_bind(listener, &IP_ADDR_ANY, sizeof(IP_ADDR_ANY)) != 0) {
                global->msg = "Bind failed";
                goto exit;
        }

        if (socket_listen(listener) != 0) {
                global->msg = "Listen error";
                goto exit;
        }

        for (;;) {
                puts("Waiting for connection...");

                // wait for connection
                SOCKET *client = NULL;
                if (socket_accept(listener, &client) != 0) {
                        puts("Connection accept error");
                        continue;
                }

                // check number of connections (counter is decreased by connection thread)
                bool accepted = false;
                if (mutex_lock(global->mtx, MAX_DELAY_MS)) {
                        if (global->conn_cnt < NUMBER_OF_CONNECTIONS) {
                                global->conn_cnt++;
                                accepted = true;
                        }

                        mutex_unlock(global->mtx);
                }

                if (!accepted) {
                        puts("Reached maximum number of connections.");
                        socket_close(client);
                        continue;
                }

                // create new thread to handle connection
                if (thread_create(telnet_thread, &thread_attr, client) == 0) {
                        puts("Thread not started");
                        socket_close(client);

                        if (mutex_lock(global->mtx, MAX_DELAY_MS)) {
                                global->conn_cnt--;
                                mutex_unlock(global->mtx);
                        }
                }
        }

exit:
        if (errno) {
                perror(global->msg);
        }

        if (listener) {
                socket_close(listener);
        }

        if (global->mtx) {
                mutex_delete(global->mtx);
        }

        return EXIT_FAILURE;
}

/*==============================================================================
  End of file
==============================================================================*/
