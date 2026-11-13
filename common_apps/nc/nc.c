/*==============================================================================
File    nc.c

Author  Daniel Zorychta

Brief   Simple netcat: TCP and UDP client and server (INET network).

        Usage: nc [-u] [-l] [-s <text>] [-f <file>] [-o <file>] [-w <s>]
                  [-c <n>] [-h] [-v] [-n <netname>] [<host>] <port>

          -u            UDP (default: TCP)
          -l            listen: TCP accepts one connection, UDP receives
                        datagrams on <port>
          -s <text>     send text (TCP: after connection, UDP: datagram to
                        <host>:<port>, broadcast: 255.255.255.255)
          -f <file>     send file content (UDP: datagrams of 1024 bytes)
          -o <file>     write received data to file (default: stdout)
          -w <s>        receive timeout in seconds (default 5, 0: send only)
          -c <n>        exit after <n> received bytes (TCP) or datagrams (UDP)
          -h            TCP: close sending direction after sent data (FIN),
                        data are still received
          -v            information messages (connection, datagram sender)
          -n <netname>  network name (default: inet)

        Received data are written to stdout (UDP: each datagram is followed
        by new line, with -v prefixed by "<ip>:<port> ").

        Exit status: 0 on success (with -c: <n> bytes or datagrams received),
        1 on error or timeout.

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
#include <dnx/os.h>
#include <dnx/net.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define BUF_SIZE        1024
#define DEFAULT_TIMEOUT 5

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
        u8_t buf[BUF_SIZE];
        bool udp;
        bool listen;
        bool half_close;
        bool verbose;
        const char *text;
        const char *in_file;
        const char *out_file;
        const char *netname;
        const char *host;
        long timeout;
        long count;
        u16_t port;
        FILE *out;
        long received;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(nc, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Print usage.
 *
 * @param  name         program name
 */
//==============================================================================
static void usage(const char *name)
{
        printf("Usage: %s [-u] [-l] [-s <text>] [-f <file>] [-o <file>] [-w <s>] [-c <n>] [-h] [-v]\n"
               "          [-n <netname>] [<host>] <port>\n",
                name);
}

//==============================================================================
/**
 * @brief  Resolve host address (IPv4 number or name).
 *
 * @param  host         host
 * @param  addr         address (port is not changed)
 *
 * @return true on success.
 */
//==============================================================================
static bool resolve(const char *host, NET_INET_sockaddr_t *addr)
{
        unsigned a, b, c, d;
        char end;
        if ((sscanf(host, "%u.%u.%u.%u%c", &a, &b, &c, &d, &end) == 4) and (a < 256) and (b < 256) and
                (c < 256) and (d < 256)) {
                addr->addr = NET_INET_IPv4(a, b, c, d);
                return true;
        }

        NET_INET_sockaddr_t res;
        if (get_host_by_name(global->netname, host, &res, sizeof(res)) == 0) {
                addr->addr = res.addr;
                return true;
        }

        fprintf(stderr, "nc: unable to resolve host '%s'\n", host);
        return false;
}

//==============================================================================
/**
 * @brief  Print address of host (verbose mode).
 *
 * @param  prefix       message prefix
 * @param  addr         address
 */
//==============================================================================
static void print_address(const char *prefix, const NET_INET_sockaddr_t *addr)
{
        if (global->verbose) {
                fprintf(stderr, "nc: %s %u.%u.%u.%u:%u\n", prefix, NET_INET_IPv4_a(addr->addr),
                        NET_INET_IPv4_b(addr->addr), NET_INET_IPv4_c(addr->addr), NET_INET_IPv4_d(addr->addr),
                        addr->port);
        }
}

//==============================================================================
/**
 * @brief  Send text and file content by TCP.
 *
 * @param  sock         connected socket
 *
 * @return true on success.
 */
//==============================================================================
static bool tcp_send_data(SOCKET *sock)
{
        if (global->text) {
                size_t len = strlen(global->text);
                if ((len > 0) and (socket_write(sock, global->text, len) != (int)len)) {
                        perror("nc: send");
                        return false;
                }
        }

        if (global->in_file) {
                FILE *f = fopen(global->in_file, "r");
                if (not f) {
                        perror(global->in_file);
                        return false;
                }

                bool ok = true;
                size_t n;
                while (ok and ((n = fread(global->buf, 1, sizeof(global->buf), f)) > 0)) {
                        ok = (socket_write(sock, global->buf, n) == (int)n);
                }

                fclose(f);

                if (not ok) {
                        perror("nc: send");
                        return false;
                }
        }

        if (global->half_close) {
                socket_shutdown(sock, NET_SHUT__WR);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Receive TCP data until connection is closed, timeout or count.
 *
 * @param  sock         connected socket
 *
 * @return true on success.
 */
//==============================================================================
static bool tcp_receive(SOCKET *sock)
{
        if (global->timeout == 0) {
                return true;
        }

        while ((global->count == 0) or (global->received < global->count)) {
                size_t len = sizeof(global->buf);
                if (global->count > 0) {
                        len = min(len, (size_t)(global->count - global->received));
                }

                int n = socket_read(sock, global->buf, len);
                if (n > 0) {
                        fwrite(global->buf, 1, n, global->out);
                        global->received += n;
                } else if (n == 0) {
                        if (global->verbose) {
                                fprintf(stderr, "nc: connection closed by remote host\n");
                        }
                        break;
                } else {
                        if (errno == ETIME) {
                                if (global->verbose) {
                                        fprintf(stderr, "nc: receive timeout\n");
                                }
                                break;
                        }
                        if ((errno == ECONNRESET) or (errno == ECONNABORTED)) {
                                if (global->verbose) {
                                        fprintf(stderr, "nc: connection closed by remote host\n");
                                }
                                break;
                        }
                        perror("nc: receive");
                        return false;
                }
        }

        return (global->count == 0) or (global->received >= global->count);
}

//==============================================================================
/**
 * @brief  TCP client or server.
 *
 * @param  sock         socket
 * @param  addr         address
 *
 * @return true on success.
 */
//==============================================================================
static bool tcp(SOCKET *sock, NET_INET_sockaddr_t *addr)
{
        bool ok = false;

        if (global->listen) {
                if ((socket_bind(sock, addr, sizeof(*addr)) != 0) or (socket_listen(sock) != 0)) {
                        perror("nc: listen");
                        return false;
                }

                print_address("listening on", addr);

                SOCKET *conn = NULL;
                if (socket_accept(sock, &conn) != 0) {
                        perror("nc: accept");
                        return false;
                }

                socket_set_recv_timeout(conn, global->timeout * 1000);
                socket_set_send_timeout(conn, DEFAULT_TIMEOUT * 1000);

                NET_INET_sockaddr_t peer;
                if (socket_get_address(conn, &peer, sizeof(peer)) == 0) {
                        print_address("connection from", &peer);
                }

                ok = tcp_send_data(conn) and tcp_receive(conn);
                socket_close(conn);

        } else {
                if (socket_connect(sock, addr, sizeof(*addr)) != 0) {
                        perror("nc: connect");
                        return false;
                }

                print_address("connected to", addr);

                ok = tcp_send_data(sock) and tcp_receive(sock);
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Send datagram.
 *
 * @param  sock         socket
 * @param  addr         destination
 * @param  data         data
 * @param  len          data length
 *
 * @return true on success.
 */
//==============================================================================
static bool udp_send(SOCKET *sock, const NET_INET_sockaddr_t *addr, const void *data, size_t len)
{
        if (socket_sendto(sock, data, len, NET_FLAGS__COPY, addr, sizeof(*addr)) != (int)len) {
                perror("nc: send");
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief  UDP client or server.
 *
 * @param  sock         socket
 * @param  addr         address
 *
 * @return true on success.
 */
//==============================================================================
static bool udp(SOCKET *sock, NET_INET_sockaddr_t *addr)
{
        if (global->listen) {
                NET_INET_sockaddr_t local = {.addr = NET_INET_IPv4_ANY, .port = addr->port};
                if (socket_bind(sock, &local, sizeof(local)) != 0) {
                        perror("nc: bind");
                        return false;
                }

                print_address("listening on", &local);
        }

        if (global->host) {
                if (global->text and not udp_send(sock, addr, global->text, strlen(global->text))) {
                        return false;
                }

                if (global->in_file) {
                        FILE *f = fopen(global->in_file, "r");
                        if (not f) {
                                perror(global->in_file);
                                return false;
                        }

                        bool ok = true;
                        size_t n;
                        while (ok and ((n = fread(global->buf, 1, sizeof(global->buf), f)) > 0)) {
                                ok = udp_send(sock, addr, global->buf, n);
                        }

                        fclose(f);

                        if (not ok) {
                                return false;
                        }
                }
        }

        if (global->timeout == 0) {
                return true;
        }

        while ((global->count == 0) or (global->received < global->count)) {
                NET_INET_sockaddr_t from;
                int n = socket_recvfrom(sock, global->buf, sizeof(global->buf), NET_FLAGS__NONE, &from,
                        sizeof(from));
                if (n < 0) {
                        if (errno == ETIME) {
                                if (global->verbose) {
                                        fprintf(stderr, "nc: receive timeout\n");
                                }
                                break;
                        }
                        perror("nc: receive");
                        return false;
                }

                if (global->verbose) {
                        fprintf(global->out, "%u.%u.%u.%u:%u ", NET_INET_IPv4_a(from.addr),
                                NET_INET_IPv4_b(from.addr), NET_INET_IPv4_c(from.addr),
                                NET_INET_IPv4_d(from.addr), from.port);
                }

                fwrite(global->buf, 1, n, global->out);
                fputc('\n', global->out);
                global->received++;
        }

        return (global->count == 0) or (global->received >= global->count);
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->netname = "inet";
        global->timeout = DEFAULT_TIMEOUT;
        global->out = stdout;

        const char *args[2] = {NULL, NULL};
        int nargs = 0;

        for (int i = 1; i < argc; i++) {
                const char *a = argv[i];
                bool value = (i + 1 < argc);

                if (strcmp(a, "-u") == 0) {
                        global->udp = true;
                } else if (strcmp(a, "-l") == 0) {
                        global->listen = true;
                } else if (strcmp(a, "-h") == 0) {
                        global->half_close = true;
                } else if (strcmp(a, "-v") == 0) {
                        global->verbose = true;
                } else if ((strcmp(a, "-s") == 0) and value) {
                        global->text = argv[++i];
                } else if ((strcmp(a, "-f") == 0) and value) {
                        global->in_file = argv[++i];
                } else if ((strcmp(a, "-o") == 0) and value) {
                        global->out_file = argv[++i];
                } else if ((strcmp(a, "-w") == 0) and value) {
                        global->timeout = strtol(argv[++i], NULL, 10);
                } else if ((strcmp(a, "-c") == 0) and value) {
                        global->count = strtol(argv[++i], NULL, 10);
                } else if ((strcmp(a, "-n") == 0) and value) {
                        global->netname = argv[++i];
                } else if ((a[0] == '-') or (nargs == 2)) {
                        usage(argv[0]);
                        return EXIT_FAILURE;
                } else {
                        args[nargs++] = a;
                }
        }

        if (nargs == 0) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }

        global->host = (nargs == 2) ? args[0] : NULL;
        long port = strtol(args[nargs - 1], NULL, 10);
        if ((port <= 0) or (port > 65535)) {
                fprintf(stderr, "nc: invalid port: %s\n", args[nargs - 1]);
                return EXIT_FAILURE;
        }
        global->port = port;

        if (not global->listen and not global->host) {
                fprintf(stderr, "nc: host is required\n");
                return EXIT_FAILURE;
        }

        NET_INET_sockaddr_t addr = {.addr = NET_INET_IPv4_ANY, .port = global->port};
        if (global->host and not global->listen and not resolve(global->host, &addr)) {
                return EXIT_FAILURE;
        }

        if (global->host and global->listen and global->udp) {
                if (not resolve(global->host, &addr)) {
                        return EXIT_FAILURE;
                }
        }

        if (global->out_file) {
                global->out = fopen(global->out_file, "w");
                if (not global->out) {
                        perror(global->out_file);
                        return EXIT_FAILURE;
                }
        }

        SOCKET *sock = socket_open(global->netname,
                global->udp ? NET_SOCKET_TYPE__DATAGRAM : NET_SOCKET_TYPE__STREAM, 0);
        if (not sock) {
                perror("nc: socket");
                if (global->out != stdout) {
                        fclose(global->out);
                }
                return EXIT_FAILURE;
        }

        socket_set_recv_timeout(sock, (global->timeout > 0 ? global->timeout : DEFAULT_TIMEOUT) * 1000);
        socket_set_send_timeout(sock, DEFAULT_TIMEOUT * 1000);

        bool ok = global->udp ? udp(sock, &addr) : tcp(sock, &addr);

        socket_close(sock);

        if (global->out != stdout) {
                fclose(global->out);
        }

        if (global->verbose) {
                fprintf(stderr, "nc: received %ld %s\n", global->received,
                        global->udp ? "datagram(s)" : "byte(s)");
        }

        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*==============================================================================
  End of file
==============================================================================*/
