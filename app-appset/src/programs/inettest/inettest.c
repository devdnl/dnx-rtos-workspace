/*==============================================================================
File    inettest.c

Author  Daniel Zorychta

Brief   TCP/IP (INET) socket test and benchmark program. All tests use the
        loopback interface (127.0.0.1).

        Usage: inettest [-t|-p|-l] [-v] [-n netname] [-d ms] [filter]
          (none)  functional tests, then performance measurements
          -t      functional tests only
          -p      performance measurements only
          -l      list tests and measurements
          -v      verbose mode
          -n      INET network name (default: inet)
          -d      duration of single measurement in ms (default: 2000)
          filter  run only tests/measurements which name contains given text

        Functional tests check the network manager and INET socket layer:
        separate receive and send locks (full duplex operation of single
        socket), close of socket used by other thread (wake-up of blocked
        operations), shutdown of receive direction, timeouts, datagram
        addressing (sendto on not connected socket), interface references and
        kill of process blocked in socket operation.

        The INET interface must be added (ifadd), the address does not have to
        be configured and link does not have to be connected (loopback is
        used). Ports are selected by process ID, so many instances can run at
        the same time.

        NOTE: functional tests expect kernel with INET/NETM fixes (separate
              RX/TX socket locks). Older kernel fails full duplex and wake-up
              tests and close of socket used by other thread can corrupt
              kernel memory there, so use only -p with it.

        NOTE: loopback does not use Ethernet driver, so transmission of
              chained packets by interface is not tested.

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
  Configuration
==============================================================================*/
/** Default network name of INET interface (created by netmgr or initd). */
#ifndef INETTEST_NETNAME
#define INETTEST_NETNAME "inet"
#endif

/** Default duration of single performance measurement [ms]. */
#ifndef INETTEST_DURATION
#define INETTEST_DURATION 2000
#endif

/** Maximum time of wake-up of blocked operation [ms]. */
#ifndef INETTEST_WAKEUP_LIMIT
#define INETTEST_WAKEUP_LIMIT 500
#endif

/** Maximum time of termination of killed process [ms]. */
#ifndef INETTEST_KILL_LIMIT
#define INETTEST_KILL_LIMIT 2000
#endif

/** Size of data transferred in bulk transfer tests (each direction). */
#ifndef INETTEST_BULK_SIZE
#define INETTEST_BULK_SIZE (64 * 1024)
#endif

/** Size of data that fills TCP buffers of not reading peer. */
#ifndef INETTEST_FILL_SIZE
#define INETTEST_FILL_SIZE (32 * 1024)
#endif

/** Number of client threads in concurrent clients test. */
#ifndef INETTEST_CLIENTS
#define INETTEST_CLIENTS 3
#endif

/** Number of connections of each client in concurrent clients test. */
#ifndef INETTEST_CLIENT_ROUNDS
#define INETTEST_CLIENT_ROUNDS 10
#endif

#define IO_BUF_SIZE          4096
#define BULK_CHUNK           1024
#define SAFE_TIMEOUT         5000 /* timeout that protects test from hang [ms] */
#define BENCH_TIMEOUT        1000 /* receive timeout of benchmark helpers [ms] */
#define MAX_DATAGRAM         1024 /* maximum datagram size of INET stack */
#define DGRAM_END_MARK       1    /* size of datagram that ends datagram stream */
#define PORT_BASE            20000
#define PORTS_PER_PID        128
#define PROGRAM_NAME         "inettest"
#define CMD_BUF_SIZE         64
#define KERNEL_MEM_TOLERANCE 64 /* [B] */

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/os.h>
#include <dnx/misc.h>
#include <dnx/thread.h>
#include <dnx/net.h>
#include <dnx/vt100.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define SKIP_PREFIX   "SKIP: "
#define SKIP(_reason) (SKIP_PREFIX _reason)

/**
 * Check condition. On failure set error message (with errno) and jump to
 * cleanup label. errno is cleared before condition evaluation.
 */
#define CHECK(_cond, ...)                                        \
        do {                                                     \
                errno = 0;                                       \
                if (!(_cond)) {                                  \
                        exit_msg = make_msg(errno, __VA_ARGS__); \
                        goto cleanup;                            \
                }                                                \
        } while (0)

/**
 * Operation is expected to fail with selected errno. _ret is result of
 * operation (-1 on failure).
 */
#define EXPECT_ERR(_ret, _errno, _what)                                                          \
        do {                                                                                     \
                errno = 0;                                                                       \
                int _r = (_ret);                                                                 \
                int _e = errno;                                                                  \
                if (_r >= 0) {                                                                   \
                        exit_msg = make_msg(0, "%s: succeeded (%d) but should fail", _what, _r); \
                        goto cleanup;                                                            \
                }                                                                                \
                if (_e != (_errno)) {                                                            \
                        exit_msg = make_msg(0, "%s: errno %d (%s), expected %d (%s)", _what, _e, \
                                strerror(_e), (_errno), strerror(_errno));                       \
                        goto cleanup;                                                            \
                }                                                                                \
        } while (0)

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        MODE_ALL,
        MODE_TESTS,
        MODE_PERF,
        MODE_LIST,
        MODE_CHILD,
} run_mode_t;

typedef struct {
        const char *name;
        const char *(*fn)(void);
} test_entry_t;

typedef struct {
        const char *name;
        void (*fn)(size_t param);
        size_t param;
} bench_entry_t;

/** Job executed by helper thread. All results are stored in job. */
typedef struct {
        tid_t tid;
        SOCKET *sock;       //!< socket used by job
        SOCKET *acc;        //!< accepted socket
        u16_t port;         //!< port (connect, sendto)
        u8_t *buf;          //!< data buffer
        size_t len;         //!< data/chunk length
        size_t count;       //!< number of repetitions
        u32_t seed;         //!< data pattern seed
        u32_t duration;     //!< job duration [ms]
        volatile bool stop; //!< stop request
        int ret;            //!< operation result
        int err;            //!< errno of failed operation (0: success)
        size_t bytes;       //!< transferred bytes
        size_t ops;         //!< number of operations
        u32_t elapsed;      //!< duration of operation [ms]
        clock_t t_end;      //!< time when operation finished
} job_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_usage(const char *name);
static bool selected(const char *name, const char *filter);
static size_t run_tests(const char *filter);
static size_t run_benchmarks(const char *filter);
static int child_main(const char *mode, u16_t port);
static const char *make_msg(int err, const char *fmt, ...);
static void res_printf(const char *fmt, ...);
static u32_t elapsed_ms(clock_t tref);
static u16_t next_port(void);
static void mkaddr(NET_INET_sockaddr_t *addr, u16_t port);
static SOCKET *sock_new(NET_socket_type_t type);
static void sock_close(SOCKET **sock);
static int sock_bind(SOCKET *sock, u16_t port);
static int sock_connect(SOCKET *sock, u16_t port);
static SOCKET *sock_listen(u16_t port);
static int sock_pair(u16_t port, SOCKET **server, SOCKET **client, SOCKET **accepted);
static int send_all(SOCKET *sock, const void *buf, size_t len);
static int recv_all(SOCKET *sock, void *buf, size_t len);
static bool is_conn_end(int err);
static bool is_abort(int err);
static void pattern_fill(u8_t *buf, size_t len, size_t offset, u32_t seed);
static size_t pattern_check(const u8_t *buf, size_t len, size_t offset, u32_t seed);
static bool job_start(job_t *job, thread_func_t func);
static void job_join(job_t *job);
static bool mem_usage(size_t *network, size_t *kernel);
static pid_t start_child(const char *mode, u16_t port);

static int th_accept(void *arg);
static int th_recv(void *arg);
static int th_send(void *arg);
static int th_bulk_send(void *arg);
static int th_bulk_recv(void *arg);
static int th_echo_server(void *arg);
static int th_echo_client(void *arg);
static int th_stream_echo(void *arg);
static int th_stream_sink_sender(void *arg);
static int th_dgram_echo(void *arg);
static int th_dgram_sender(void *arg);
static int th_conn_server(void *arg);

static const char *test_interface(void);
static const char *test_dgram_sendto(void);
static const char *test_dgram_connected(void);
static const char *test_dgram_sizes(void);
static const char *test_dgram_timeout(void);
static const char *test_stream_basic(void);
static const char *test_stream_timeouts(void);
static const char *test_stream_peer_close(void);
static const char *test_full_duplex_wakeup(void);
static const char *test_full_duplex_bulk(void);
static const char *test_close_wakes_recv(void);
static const char *test_close_wakes_send(void);
static const char *test_close_wakes_accept(void);
static const char *test_shutdown_wakes_recv(void);
static const char *test_concurrent_clients(void);
static const char *test_kill_dgram_recv(void);
static const char *test_kill_stream_send(void);
static const char *test_memory_leak(void);

static void bench_stream_throughput(size_t chunk);
static void bench_stream_duplex(size_t chunk);
static void bench_stream_rtt(size_t size);
static void bench_dgram_throughput(size_t size);
static void bench_dgram_rtt(size_t size);
static void bench_connect_rate(size_t unused);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t passed;
        size_t failed;
        size_t skipped;
        bool verbose;
        const char *netname;
        u32_t duration;
        u16_t port_base;
        u16_t port_next;
        char msg_buf[160];
        char res_buf[160]; //!< result of performance measurement
        u8_t *txbuf;
        u8_t *rxbuf;
};

static const char *const SUCCESS_MSG = "SUCCESS";

static const thread_attr_t THREAD_ATTR = {
        .stack_depth = STACK_DEPTH_MEDIUM,
        .priority = PRIORITY_NORMAL,
        .detached = false,
};

static const test_entry_t TEST_TAB[] = {
        {.name = "Interface: errors and references", .fn = test_interface},
        {.name = "UDP: sendto without bind and reply", .fn = test_dgram_sendto},
        {.name = "UDP: sendto on connected socket", .fn = test_dgram_connected},
        {.name = "UDP: datagram sizes, NOCOPY and size limit", .fn = test_dgram_sizes},
        {.name = "UDP: receive timeout", .fn = test_dgram_timeout},
        {.name = "TCP: connect, accept, echo, addresses", .fn = test_stream_basic},
        {.name = "TCP: receive timeout and partial send", .fn = test_stream_timeouts},
        {.name = "TCP: peer close and reset", .fn = test_stream_peer_close},
        {.name = "TCP: send while other thread receives", .fn = test_full_duplex_wakeup},
        {.name = "TCP: full duplex bulk transfer", .fn = test_full_duplex_bulk},
        {.name = "TCP: close wakes blocked receive", .fn = test_close_wakes_recv},
        {.name = "TCP: close wakes blocked send", .fn = test_close_wakes_send},
        {.name = "TCP: close wakes blocked accept", .fn = test_close_wakes_accept},
        {.name = "TCP: shutdown(RD) wakes blocked receive", .fn = test_shutdown_wakes_recv},
        {.name = "TCP: concurrent clients", .fn = test_concurrent_clients},
        {.name = "Kill: process blocked in UDP receive", .fn = test_kill_dgram_recv},
        {.name = "Kill: process blocked in TCP send", .fn = test_kill_stream_send},
        {.name = "Memory released", .fn = test_memory_leak},
};

static const bench_entry_t BENCH_TAB[] = {
        {.name = "TCP throughput, chunk", .fn = bench_stream_throughput, .param = 128},
        {.name = "TCP throughput, chunk", .fn = bench_stream_throughput, .param = 1024},
        {.name = "TCP throughput, chunk", .fn = bench_stream_throughput, .param = 4096},
        {.name = "TCP full duplex, chunk", .fn = bench_stream_duplex, .param = 1024},
        {.name = "TCP round trip, msg", .fn = bench_stream_rtt, .param = 1},
        {.name = "TCP round trip, msg", .fn = bench_stream_rtt, .param = 64},
        {.name = "TCP round trip, msg", .fn = bench_stream_rtt, .param = 1024},
        {.name = "UDP throughput, size", .fn = bench_dgram_throughput, .param = 128},
        {.name = "UDP throughput, size", .fn = bench_dgram_throughput, .param = 1024},
        {.name = "UDP round trip, size", .fn = bench_dgram_rtt, .param = 16},
        {.name = "UDP round trip, size", .fn = bench_dgram_rtt, .param = 1024},
        {.name = "TCP connect/accept/close rate", .fn = bench_connect_rate, .param = 0},
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(inettest, STACK_DEPTH_MEDIUM);

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
 * @param argc      argument count
 * @param argv      arguments
 *
 * @return EXIT_SUCCESS when all tests passed, otherwise EXIT_FAILURE.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *filter = NULL;
        const char *child_mode = NULL;
        u16_t child_port = 0;
        run_mode_t mode = MODE_ALL;

        global->netname = INETTEST_NETNAME;
        global->duration = INETTEST_DURATION;

        for (int i = 1; i < argc; i++) {
                const char *arg = argv[i];

                if (isstreq(arg, "-v")) {
                        global->verbose = true;
                } else if (isstreq(arg, "-t")) {
                        mode = MODE_TESTS;
                } else if (isstreq(arg, "-p")) {
                        mode = MODE_PERF;
                } else if (isstreq(arg, "-l")) {
                        mode = MODE_LIST;
                } else if (isstreq(arg, "-n") && (i + 1 < argc)) {
                        global->netname = argv[++i];
                } else if (isstreq(arg, "-d") && (i + 1 < argc)) {
                        global->duration = strtoul(argv[++i], NULL, 0);
                        if (global->duration < 100) {
                                global->duration = 100;
                        }
                } else if (isstreq(arg, "-c") && (i + 2 < argc)) {
                        // internal: process used by kill tests
                        mode = MODE_CHILD;
                        child_mode = argv[++i];
                        child_port = (u16_t)strtoul(argv[++i], NULL, 0);
                } else if (isstreq(arg, "-h")) {
                        print_usage(argv[0]);
                        return EXIT_SUCCESS;
                } else if (arg[0] == '-') {
                        printf("Unknown option: %s\n", arg);
                        print_usage(argv[0]);
                        return EXIT_FAILURE;
                } else {
                        filter = arg;
                }
        }

        if (mode == MODE_CHILD) {
                return child_main(child_mode, child_port);
        }

        if (mode == MODE_LIST) {
                puts("Tests:");
                for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                        if (selected(TEST_TAB[i].name, filter)) {
                                printf("  %2u: %s\n", (unsigned)(i + 1), TEST_TAB[i].name);
                        }
                }
                puts("Performance measurements:");
                for (size_t i = 0; i < ARRAY_SIZE(BENCH_TAB); i++) {
                        if (selected(BENCH_TAB[i].name, filter)) {
                                printf("  %s %u\n", BENCH_TAB[i].name, (unsigned)BENCH_TAB[i].param);
                        }
                }
                return EXIT_SUCCESS;
        }

        global->port_base = (u16_t)(PORT_BASE + (getpid() % 300) * PORTS_PER_PID);

        global->txbuf = malloc(IO_BUF_SIZE);
        global->rxbuf = malloc(IO_BUF_SIZE);
        if (!global->txbuf || !global->rxbuf) {
                puts("Not enough free memory.");
                free(global->txbuf);
                free(global->rxbuf);
                return EXIT_FAILURE;
        }

        // check that INET network exists
        SOCKET *probe = sock_new(NET_SOCKET_TYPE__DATAGRAM);
        if (!probe) {
                printf(VT100_FONT_COLOR_RED
                        "Unable to create socket in '%s' network: %s" VT100_RESET_ATTRIBUTES "\n",
                        global->netname, strerror(errno));
                free(global->txbuf);
                free(global->rxbuf);
                return EXIT_FAILURE;
        }
        sock_close(&probe);

        size_t executed = 0;

        if ((mode == MODE_ALL) || (mode == MODE_TESTS)) {
                executed += run_tests(filter);
        }

        if ((mode == MODE_ALL) || (mode == MODE_PERF)) {
                executed += run_benchmarks(filter);
        }

        if ((mode == MODE_ALL) || (mode == MODE_TESTS)) {
                puts("");
                printf(VT100_FONT_COLOR_YELLOW "SUMMARY:" VT100_RESET_ATTRIBUTES
                                               " executed: %u, passed: %u, failed: %u, skipped: %u\n",
                        (unsigned)executed, (unsigned)global->passed, (unsigned)global->failed,
                        (unsigned)global->skipped);
        }

        free(global->txbuf);
        free(global->rxbuf);

        return (global->failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//==============================================================================
/**
 * @brief  Print program usage.
 *
 * @param  name         program name
 */
//==============================================================================
static void print_usage(const char *name)
{
        printf("Usage: %s [-t|-p|-l] [-v] [-n netname] [-d ms] [filter]\n", name);
        puts("  (none)  functional tests, then performance measurements");
        puts("  -t      functional tests only");
        puts("  -p      performance measurements only");
        puts("  -l      list tests and measurements");
        puts("  -v      verbose mode");
        puts("  -n      INET network name (default: " INETTEST_NETNAME ")");
        puts("  -d      duration of single measurement in ms");
        puts("  filter  run only tests/measurements which name contains given text");
}

//==============================================================================
/**
 * @brief  Check if test is selected by filter.
 *
 * @param  name         test name
 * @param  filter       filter (can be NULL)
 *
 * @return true if selected.
 */
//==============================================================================
static bool selected(const char *name, const char *filter)
{
        return (filter == NULL) || (strstr(name, filter) != NULL);
}

//==============================================================================
/**
 * @brief  Run functional tests.
 *
 * @param  filter       filter (can be NULL)
 *
 * @return Number of executed tests.
 */
//==============================================================================
static size_t run_tests(const char *filter)
{
        size_t executed = 0;

        printf("\n" VT100_FONT_COLOR_YELLOW
               "FUNCTIONAL TESTS (network '%s', 127.0.0.1)" VT100_RESET_ATTRIBUTES "\n",
                global->netname);

        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                const test_entry_t *test = &TEST_TAB[i];

                if (!selected(test->name, filter)) {
                        continue;
                }

                /*
                 * NOTE: test line is printed at once after test, because not
                 *       finished line printed before long operation is
                 *       duplicated by some terminals. In verbose mode the name
                 *       of started test is printed in separate line (helps to
                 *       find test that hangs).
                 */
                if (global->verbose) {
                        printf("TEST %2u: started\n", (unsigned)(i + 1));
                }

                clock_t tref = get_uptime_ms();
                const char *msg = test->fn();
                u32_t dt = elapsed_ms(tref);

                printf(VT100_FONT_COLOR_CYAN "TEST %2u:" VT100_RESET_ATTRIBUTES " %s... ", (unsigned)(i + 1),
                        test->name);

                if (isstreq(msg, SUCCESS_MSG)) {
                        printf(VT100_FONT_COLOR_GREEN "[SUCCESS]" VT100_RESET_ATTRIBUTES);
                        global->passed++;

                } else if (strncmp(msg, SKIP_PREFIX, strlen(SKIP_PREFIX)) == 0) {
                        printf(VT100_FONT_COLOR_YELLOW "[SKIPPED]" VT100_RESET_ATTRIBUTES " %s",
                                msg + strlen(SKIP_PREFIX));
                        global->skipped++;

                } else {
                        printf(VT100_FONT_COLOR_RED "[FAILURE]" VT100_RESET_ATTRIBUTES "\n");
                        printf(VT100_FONT_COLOR_RED "=> %s" VT100_RESET_ATTRIBUTES, msg);
                        global->failed++;
                }

                if (global->verbose) {
                        printf(" (%u ms)", (unsigned)dt);
                }

                puts("");
                executed++;
        }

        return executed;
}

//==============================================================================
/**
 * @brief  Run performance measurements.
 *
 * @param  filter       filter (can be NULL)
 *
 * @return Number of executed measurements.
 */
//==============================================================================
static size_t run_benchmarks(const char *filter)
{
        size_t executed = 0;

        printf("\n" VT100_FONT_COLOR_YELLOW
               "PERFORMANCE (network '%s', 127.0.0.1, %u ms per measurement)" VT100_RESET_ATTRIBUTES "\n",
                global->netname, (unsigned)global->duration);

        for (size_t i = 0; i < ARRAY_SIZE(BENCH_TAB); i++) {
                const bench_entry_t *bench = &BENCH_TAB[i];

                if (!selected(bench->name, filter)) {
                        continue;
                }

                /*
                 * NOTE: line is printed at once after measurement, because
                 *       not finished line printed before long measurement
                 *       is duplicated by some terminals.
                 */
                global->res_buf[0] = '\0';

                bench->fn(bench->param);

                if (bench->param) {
                        printf("  %s %4u B: %s\n", bench->name, (unsigned)bench->param, global->res_buf);
                } else {
                        printf("  %s: %s\n", bench->name, global->res_buf);
                }

                executed++;
        }

        return executed;
}

//==============================================================================
/**
 * @brief  Child process used by kill tests. Process signals that it is ready
 *         and blocks in selected socket operation until it is killed.
 *
 *         Modes:
 *         udp: sends datagram "R" to port and waits for datagram (forever),
 *         tcp: connects to port, sends "R" and sends data until peer buffers
 *              are full (peer does not read, operation blocks forever).
 *
 * @param  mode         child mode
 * @param  port         port of parent process
 *
 * @return Exit code (only when operation was not blocked as expected).
 */
//==============================================================================
static int child_main(const char *mode, u16_t port)
{
        int status = EXIT_FAILURE;
        u8_t *buf = NULL;
        NET_INET_sockaddr_t addr;
        mkaddr(&addr, port);

        if (isstreq(mode, "udp")) {
                SOCKET *sock = socket_open(global->netname, NET_SOCKET_TYPE__DATAGRAM, 0);
                if (sock) {
                        if (socket_sendto(sock, "R", 1, NET_FLAGS__NONE, &addr, sizeof(addr)) == 1) {
                                u8_t c;
                                // default receive timeout is infinite
                                socket_recv(sock, &c, 1, NET_FLAGS__NONE);
                        }
                        socket_close(sock);
                }

        } else if (isstreq(mode, "tcp")) {
                buf = malloc(IO_BUF_SIZE);
                SOCKET *sock = socket_open(global->netname, NET_SOCKET_TYPE__STREAM, 0);
                if (sock && buf && (socket_connect(sock, &addr, sizeof(addr)) == 0) &&
                        (socket_send(sock, "R", 1, NET_FLAGS__NONE) == 1)) {

                        memset(buf, 0x55, IO_BUF_SIZE);

                        // default send timeout is infinite, so operation blocks
                        while (socket_send(sock, buf, IO_BUF_SIZE, NET_FLAGS__NONE) > 0) {
                        }
                }

                if (sock) {
                        socket_close(sock);
                }
        }

        free(buf);

        return status;
}

//==============================================================================
/**
 * @brief  Create error message in global buffer.
 *
 * @param  err          errno value (appended to message if > 0)
 * @param  fmt          format
 *
 * @return Pointer to message buffer.
 */
//==============================================================================
static const char *make_msg(int err, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(global->msg_buf, sizeof(global->msg_buf), fmt, args);
        va_end(args);

        if ((err > 0) && (n >= 0) && ((size_t)n < sizeof(global->msg_buf))) {
                snprintf(global->msg_buf + n, sizeof(global->msg_buf) - n, " (errno: %s)", strerror(err));
        }

        return global->msg_buf;
}

//==============================================================================
/**
 * @brief  Append text to result of performance measurement.
 *
 * @param  fmt          format
 */
//==============================================================================
static void res_printf(const char *fmt, ...)
{
        size_t len = strlen(global->res_buf);

        if (len < sizeof(global->res_buf) - 1) {
                va_list args;
                va_start(args, fmt);
                vsnprintf(global->res_buf + len, sizeof(global->res_buf) - len, fmt, args);
                va_end(args);
        }
}

//==============================================================================
/**
 * @brief  Return time elapsed since reference time.
 *
 * @param  tref         reference time
 *
 * @return Elapsed time [ms].
 */
//==============================================================================
static u32_t elapsed_ms(clock_t tref)
{
        return (u32_t)(get_uptime_ms() - tref);
}

//==============================================================================
/**
 * @brief  Return next free port of this program instance. Each test uses own
 *         ports, because closed TCP connections stay in TIME_WAIT state.
 *
 * @return Port number.
 */
//==============================================================================
static u16_t next_port(void)
{
        u16_t port = global->port_base + global->port_next;
        global->port_next = (global->port_next + 1) % PORTS_PER_PID;
        return port;
}

//==============================================================================
/**
 * @brief  Create loopback socket address.
 *
 * @param  addr         address
 * @param  port         port
 */
//==============================================================================
static void mkaddr(NET_INET_sockaddr_t *addr, u16_t port)
{
        memset(addr, 0, sizeof(*addr));
        addr->addr = NET_INET_IPv4_LOOPBACK;
        addr->port = port;
}

//==============================================================================
/**
 * @brief  Create new socket.
 *
 * @param  type         socket type
 *
 * @return Socket or NULL on error.
 */
//==============================================================================
static SOCKET *sock_new(NET_socket_type_t type)
{
        SOCKET *sock = socket_open(global->netname, type, 0);

        if (sock) {
                // protects test from hang when operation is not finished
                socket_set_recv_timeout(sock, SAFE_TIMEOUT);
                socket_set_send_timeout(sock, SAFE_TIMEOUT);
        }

        return sock;
}

//==============================================================================
/**
 * @brief  Close socket (if open) and clear pointer.
 *
 * @param  sock         socket pointer
 */
//==============================================================================
static void sock_close(SOCKET **sock)
{
        if (*sock) {
                socket_close(*sock);
                *sock = NULL;
        }
}

//==============================================================================
/**
 * @brief  Bind socket to loopback address.
 *
 * @param  sock         socket
 * @param  port         port
 *
 * @return 0 on success, -1 on error (errno is set).
 */
//==============================================================================
static int sock_bind(SOCKET *sock, u16_t port)
{
        NET_INET_sockaddr_t addr;
        mkaddr(&addr, port);
        return socket_bind(sock, &addr, sizeof(addr));
}

//==============================================================================
/**
 * @brief  Connect socket to loopback port.
 *
 * @param  sock         socket
 * @param  port         port
 *
 * @return 0 on success, -1 on error (errno is set).
 */
//==============================================================================
static int sock_connect(SOCKET *sock, u16_t port)
{
        NET_INET_sockaddr_t addr;
        mkaddr(&addr, port);
        return socket_connect(sock, &addr, sizeof(addr));
}

//==============================================================================
/**
 * @brief  Create listening stream socket.
 *
 * @param  port         port
 *
 * @return Socket or NULL on error (errno is set).
 */
//==============================================================================
static SOCKET *sock_listen(u16_t port)
{
        SOCKET *sock = sock_new(NET_SOCKET_TYPE__STREAM);

        if (sock) {
                if ((sock_bind(sock, port) != 0) || (socket_listen(sock) != 0)) {
                        int err = errno;
                        sock_close(&sock);
                        errno = err;
                }
        }

        return sock;
}

//==============================================================================
/**
 * @brief  Create connected pair of stream sockets.
 *
 * @param  port         port of server socket
 * @param  server       listening socket
 * @param  client       client socket
 * @param  accepted     accepted (server side) socket
 *
 * @return 0 on success, otherwise errno value. On error all sockets are closed.
 */
//==============================================================================
static int sock_pair(u16_t port, SOCKET **server, SOCKET **client, SOCKET **accepted)
{
        job_t job;
        memset(&job, 0, sizeof(job));

        *client = NULL;
        *accepted = NULL;

        *server = sock_listen(port);
        if (!*server) {
                return errno ? errno : EIO;
        }

        int err = 0;

        *client = sock_new(NET_SOCKET_TYPE__STREAM);
        if (!*client) {
                err = errno ? errno : EIO;
                goto error;
        }

        job.sock = *server;
        if (!job_start(&job, th_accept)) {
                err = ENOMEM;
                goto error;
        }

        if (sock_connect(*client, port) != 0) {
                err = errno ? errno : EIO;
        }

        job_join(&job);

        if (!err && job.err) {
                err = job.err;
        }

        if (!err) {
                *accepted = job.acc;
                socket_set_recv_timeout(*accepted, SAFE_TIMEOUT);
                socket_set_send_timeout(*accepted, SAFE_TIMEOUT);
                return 0;
        }

        sock_close(&job.acc);

error:
        sock_close(client);
        sock_close(server);
        return err;
}

//==============================================================================
/**
 * @brief  Send whole buffer.
 *
 * @param  sock         socket
 * @param  buf          data
 * @param  len          data length
 *
 * @return 0 on success, otherwise errno value.
 */
//==============================================================================
static int send_all(SOCKET *sock, const void *buf, size_t len)
{
        const u8_t *ptr = buf;

        while (len > 0) {
                int n = socket_send(sock, ptr, len, NET_FLAGS__NONE);
                if (n < 0) {
                        return errno ? errno : EIO;
                }
                if (n == 0) {
                        return ETIME;
                }

                ptr += n;
                len -= n;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Receive exact number of bytes.
 *
 * @param  sock         socket
 * @param  buf          buffer
 * @param  len          number of bytes
 *
 * @return 0 on success, ETIME on timeout, otherwise errno value.
 */
//==============================================================================
static int recv_all(SOCKET *sock, void *buf, size_t len)
{
        u8_t *ptr = buf;

        while (len > 0) {
                int n = socket_recv(sock, ptr, len, NET_FLAGS__NONE);
                if (n < 0) {
                        return errno ? errno : EIO;
                }
                if (n == 0) {
                        return ETIME;
                }

                ptr += n;
                len -= n;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Check if error means end of connection (peer closed or reset).
 *
 * @param  err          errno value
 *
 * @return true if connection is finished.
 */
//==============================================================================
static bool is_conn_end(int err)
{
        return (err == ECONNABORTED) || (err == ECONNRESET);
}

//==============================================================================
/**
 * @brief  Check if error means operation aborted by close of own socket
 *         (socket closing in stack or socket already destroyed).
 *
 * @param  err          errno value
 *
 * @return true if operation was aborted.
 */
//==============================================================================
static bool is_abort(int err)
{
        return (err == ECONNABORTED) || (err == EBADF);
}

//==============================================================================
/**
 * @brief  Fill buffer with pattern.
 *
 * @param  buf          buffer
 * @param  len          length
 * @param  offset       offset of buffer in data stream
 * @param  seed         pattern seed
 */
//==============================================================================
static void pattern_fill(u8_t *buf, size_t len, size_t offset, u32_t seed)
{
        for (size_t i = 0; i < len; i++) {
                size_t pos = offset + i;
                buf[i] = (u8_t)(seed + pos + (pos >> 8) * 7);
        }
}

//==============================================================================
/**
 * @brief  Check buffer pattern.
 *
 * @param  buf          buffer
 * @param  len          length
 * @param  offset       offset of buffer in data stream
 * @param  seed         pattern seed
 *
 * @return Index of first wrong byte or len when all bytes are correct.
 */
//==============================================================================
static size_t pattern_check(const u8_t *buf, size_t len, size_t offset, u32_t seed)
{
        for (size_t i = 0; i < len; i++) {
                size_t pos = offset + i;
                if (buf[i] != (u8_t)(seed + pos + (pos >> 8) * 7)) {
                        return i;
                }
        }

        return len;
}

//==============================================================================
/**
 * @brief  Start job in new thread.
 *
 * @param  job          job
 * @param  func         thread function
 *
 * @return true on success.
 */
//==============================================================================
static bool job_start(job_t *job, thread_func_t func)
{
        job->stop = false;
        job->err = 0;
        job->tid = thread_create(func, &THREAD_ATTR, job);
        return job->tid != 0;
}

//==============================================================================
/**
 * @brief  Wait for job thread.
 *
 * @param  job          job
 */
//==============================================================================
static void job_join(job_t *job)
{
        if (job->tid) {
                thread_join(job->tid, NULL);
                job->tid = 0;
        }
}

//==============================================================================
/**
 * @brief  Return memory used by network subsystem and kernel.
 *
 * @param  network      network memory [B]
 * @param  kernel       kernel memory [B]
 *
 * @return true if statistics are available.
 */
//==============================================================================
static bool mem_usage(size_t *network, size_t *kernel)
{
        memstat_t stat;

        if (get_memory_usage_details(&stat) == 0) {
                *network = (size_t)stat.network_memory_usage;
                *kernel = (size_t)stat.kernel_memory_usage;
                return true;
        }

        return false;
}

//==============================================================================
/**
 * @brief  Start child process of this program (kill tests).
 *
 * @param  mode         child mode
 * @param  port         port of parent
 *
 * @return Process ID or 0 on error.
 */
//==============================================================================
static pid_t start_child(const char *mode, u16_t port)
{
        char cmd[CMD_BUF_SIZE];
        snprintf(cmd, sizeof(cmd), PROGRAM_NAME " -n %s -c %s %u", global->netname, mode, (unsigned)port);

        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.fd_stdin = -1;
        attr.fd_stdout = fileno(stdout);
        attr.fd_stderr = fileno(stderr);
        attr.cwd = "/";
        attr.priority = PRIORITY_NORMAL;
        attr.detached = false;

        return process_create(cmd, &attr);
}

/*==============================================================================
  Job threads
==============================================================================*/

//==============================================================================
/**
 * @brief  Accept single connection (job->sock: listening socket).
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_accept(void *arg)
{
        job_t *job = arg;

        clock_t tref = get_uptime_ms();
        job->acc = NULL;
        job->ret = socket_accept(job->sock, &job->acc);
        job->err = (job->ret != 0) ? (errno ? errno : EIO) : 0;
        job->t_end = get_uptime_ms();
        job->elapsed = (u32_t)(job->t_end - tref);

        return 0;
}

//==============================================================================
/**
 * @brief  Single receive operation (job->sock, job->buf, job->len).
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_recv(void *arg)
{
        job_t *job = arg;

        clock_t tref = get_uptime_ms();
        job->ret = socket_recv(job->sock, job->buf, job->len, NET_FLAGS__NONE);
        job->err = (job->ret < 0) ? (errno ? errno : EIO) : 0;
        job->t_end = get_uptime_ms();
        job->elapsed = (u32_t)(job->t_end - tref);

        return 0;
}

//==============================================================================
/**
 * @brief  Single send operation (job->sock, job->buf, job->len).
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_send(void *arg)
{
        job_t *job = arg;

        clock_t tref = get_uptime_ms();
        job->ret = socket_send(job->sock, job->buf, job->len, NET_FLAGS__NONE);
        job->err = (job->ret < 0) ? (errno ? errno : EIO) : 0;
        job->t_end = get_uptime_ms();
        job->elapsed = (u32_t)(job->t_end - tref);

        return 0;
}

//==============================================================================
/**
 * @brief  Send job->count bytes of pattern in chunks of job->len bytes.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_bulk_send(void *arg)
{
        job_t *job = arg;

        job->bytes = 0;

        while ((job->bytes < job->count) && !job->err) {
                size_t chunk = min(job->len, job->count - job->bytes);
                pattern_fill(job->buf, chunk, job->bytes, job->seed);
                job->err = send_all(job->sock, job->buf, chunk);
                if (!job->err) {
                        job->bytes += chunk;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Receive and check job->count bytes of pattern.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_bulk_recv(void *arg)
{
        job_t *job = arg;

        job->bytes = 0;

        while ((job->bytes < job->count) && !job->err) {
                size_t chunk = min(job->len, job->count - job->bytes);
                int n = socket_recv(job->sock, job->buf, chunk, NET_FLAGS__NONE);

                if (n < 0) {
                        job->err = errno ? errno : EIO;
                } else if (n == 0) {
                        job->err = ETIME;
                } else if (pattern_check(job->buf, n, job->bytes, job->seed) != (size_t)n) {
                        job->err = EILSEQ;
                } else {
                        job->bytes += n;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Echo server: accepts job->count connections one by one and echoes
 *         received data until client closes connection.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_echo_server(void *arg)
{
        job_t *job = arg;
        u8_t buf[64];

        for (job->ops = 0; (job->ops < job->count) && !job->stop && !job->err;) {
                SOCKET *conn = NULL;

                if (socket_accept(job->sock, &conn) != 0) {
                        if (errno != ETIME) {
                                job->err = errno ? errno : EIO;
                        }
                        continue;
                }

                socket_set_recv_timeout(conn, SAFE_TIMEOUT);
                socket_set_send_timeout(conn, SAFE_TIMEOUT);

                for (;;) {
                        int n = socket_recv(conn, buf, sizeof(buf), NET_FLAGS__NONE);
                        if (n <= 0) {
                                int err = (n == 0) ? ETIME : (errno ? errno : EIO);
                                if (!is_conn_end(err)) {
                                        job->err = err;
                                }
                                break;
                        }

                        if (send_all(conn, buf, n) != 0) {
                                break;
                        }
                }

                sock_close(&conn);
                job->ops++;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Echo client: job->count times connects to job->port, sends pattern
 *         and checks echo.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_echo_client(void *arg)
{
        job_t *job = arg;
        u8_t tx[48];
        u8_t rx[48];

        for (job->ops = 0; (job->ops < job->count) && !job->err; job->ops++) {
                SOCKET *sock = sock_new(NET_SOCKET_TYPE__STREAM);
                if (!sock) {
                        job->err = errno ? errno : ENOMEM;
                        break;
                }

                socket_set_recv_timeout(sock, SAFE_TIMEOUT * 2);

                if (sock_connect(sock, job->port) != 0) {
                        job->err = errno ? errno : EIO;

                } else {
                        size_t len = 1 + ((job->seed + job->ops * 13) % sizeof(tx));
                        pattern_fill(tx, len, job->ops, job->seed);

                        job->err = send_all(sock, tx, len);
                        if (!job->err) {
                                job->err = recv_all(sock, rx, len);
                        }
                        if (!job->err && (memcmp(tx, rx, len) != 0)) {
                                job->err = EILSEQ;
                        }
                }

                sock_close(&sock);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Stream echo of job->len byte messages until stop request
 *         (job->sock: connected socket). Receive timeout must be set.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_stream_echo(void *arg)
{
        job_t *job = arg;

        while (!job->stop) {
                int err = recv_all(job->sock, job->buf, job->len);
                if (err == ETIME) {
                        continue;
                }

                if (err || send_all(job->sock, job->buf, job->len)) {
                        break;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Send pattern in job->len chunks for job->duration ms.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_stream_sink_sender(void *arg)
{
        job_t *job = arg;

        job->bytes = 0;
        clock_t tref = get_uptime_ms();

        while (!job->stop && (elapsed_ms(tref) < job->duration)) {
                pattern_fill(job->buf, job->len, job->bytes, job->seed);
                job->err = send_all(job->sock, job->buf, job->len);
                if (job->err) {
                        break;
                }
                job->bytes += job->len;
        }

        job->elapsed = elapsed_ms(tref);

        return 0;
}

//==============================================================================
/**
 * @brief  Datagram echo until stop request (job->sock: bound socket).
 *         Receive timeout must be set.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_dgram_echo(void *arg)
{
        job_t *job = arg;
        NET_INET_sockaddr_t from;

        while (!job->stop) {
                memset(&from, 0, sizeof(from));

                int n = socket_recvfrom(job->sock, job->buf, job->len, NET_FLAGS__NONE, &from, sizeof(from));
                if ((n < 0) && (errno == ETIME)) {
                        continue;
                }

                if ((n <= 0) ||
                        (socket_sendto(job->sock, job->buf, n, NET_FLAGS__NONE, &from, sizeof(from)) != n)) {
                        job->err = errno ? errno : EIO;
                        break;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Send job->len datagrams to job->port for job->duration ms,
 *         then send end mark.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_dgram_sender(void *arg)
{
        job_t *job = arg;
        NET_INET_sockaddr_t to;

        mkaddr(&to, job->port);
        pattern_fill(job->buf, job->len, 0, job->seed);

        job->bytes = 0;
        job->ops = 0;
        clock_t tref = get_uptime_ms();

        while (!job->stop && (elapsed_ms(tref) < job->duration)) {
                int n = socket_sendto(job->sock, job->buf, job->len, NET_FLAGS__NONE, &to, sizeof(to));
                if ((n < 0) && (errno == ENOMEM)) {
                        // output queue is full (e.g. loopback), datagram is not sent
                        msleep(1);
                        continue;
                }

                if (n != (int)job->len) {
                        job->err = (n < 0) ? (errno ? errno : EIO) : EIO;
                        break;
                }

                job->bytes += n;
                job->ops++;
        }

        job->elapsed = elapsed_ms(tref);

        // end marks (datagram can be lost)
        for (int i = 0; i < 3; i++) {
                msleep(20);
                socket_sendto(job->sock, job->buf, DGRAM_END_MARK, NET_FLAGS__NONE, &to, sizeof(to));
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Connection server: accepts connections until stop request,
 *         receives 1 byte, replies and closes connection.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_conn_server(void *arg)
{
        job_t *job = arg;
        u8_t byte;

        job->ops = 0;

        while (!job->stop) {
                SOCKET *conn = NULL;

                if (socket_accept(job->sock, &conn) != 0) {
                        if (errno != ETIME) {
                                job->err = errno ? errno : EIO;
                                break;
                        }
                        continue;
                }

                socket_set_recv_timeout(conn, BENCH_TIMEOUT);
                socket_set_send_timeout(conn, BENCH_TIMEOUT);

                if ((recv_all(conn, &byte, 1) == 0) && (send_all(conn, &byte, 1) == 0)) {
                        job->ops++;
                }

                // client closes connection first (TIME_WAIT stays at client side)
                recv_all(conn, &byte, 1);
                sock_close(&conn);
        }

        return 0;
}

/*==============================================================================
  Functional tests
==============================================================================*/

//==============================================================================
/**
 * @brief  Interface management errors and interface references.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_interface(void)
{
        static const char *const NO_NET = "inettest-none";

        const char *exit_msg = SUCCESS_MSG;
        SOCKET *sock = NULL;
        NET_family_t family = _NET_FAMILY__COUNT;

        CHECK(ifstatus(global->netname, &family, NULL, 0) == 0, "ifstatus('%s')", global->netname);
        CHECK(family == NET_FAMILY__INET, "family of '%s' is %d, expected INET", global->netname,
                (int)family);

        // operations on not existing interface
        EXPECT_ERR(ifstatus(NO_NET, &family, NULL, 0), ENODEV, "ifstatus of not existing interface");
        EXPECT_ERR(ifup(NO_NET, NULL, 0), ENODEV, "ifup of not existing interface");
        EXPECT_ERR(ifdown(NO_NET), ENODEV, "ifdown of not existing interface");
        EXPECT_ERR(ifrm(NO_NET), ENODEV, "ifrm of not existing interface");
        EXPECT_ERR(socket(NO_NET, NET_SOCKET_TYPE__STREAM, 0), ENODEV, "socket in not existing interface");

        // interface with the same name already exists
        EXPECT_ERR(ifadd(global->netname, NET_FAMILY__INET, "/dev/null"), EADDRINUSE,
                "ifadd of existing name");

        // interface used by socket cannot be removed
        CHECK((sock = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        EXPECT_ERR(ifrm(global->netname), EBUSY, "ifrm of interface used by socket");

cleanup:
        sock_close(&sock);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram sent by not bound and not connected socket (sendto), sender
 *         address and reply.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_sendto(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        NET_INET_sockaddr_t to, from;
        u16_t pa = next_port();
        char buf[16];

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, pa) == 0, "socket_bind(a)");

        // b is not bound and not connected
        mkaddr(&to, pa);
        CHECK(socket_sendto(b, "hello", 5, NET_FLAGS__NONE, &to, sizeof(to)) == 5,
                "socket_sendto() of not connected socket");

        memset(&from, 0, sizeof(from));
        int n = socket_recvfrom(a, buf, sizeof(buf), NET_FLAGS__NONE, &from, sizeof(from));
        CHECK((n == 5) && (memcmp(buf, "hello", 5) == 0), "socket_recvfrom(): %d", n);
        CHECK(from.addr == NET_INET_IPv4_LOOPBACK, "sender address 0x%08X", (unsigned)from.addr);
        CHECK(from.port != 0, "sender port is 0");

        CHECK(socket_sendto(a, "reply", 5, NET_FLAGS__NONE, &from, sizeof(from)) == 5,
                "socket_sendto(reply)");
        n = socket_recv(b, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 5) && (memcmp(buf, "reply", 5) == 0), "socket_recv(reply): %d", n);

        // sending to itself
        CHECK(socket_sendto(a, "self", 4, NET_FLAGS__NONE, &to, sizeof(to)) == 4, "socket_sendto(self)");
        n = socket_recv(a, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 4) && (memcmp(buf, "self", 4) == 0), "socket_recv(self): %d", n);

cleanup:
        sock_close(&a);
        sock_close(&b);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  sendto on connected datagram socket does not change the connection.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_connected(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        SOCKET *c = NULL;
        NET_INET_sockaddr_t to, from;
        u16_t pa = next_port();
        u16_t pb = next_port();
        u16_t pc = next_port();
        char buf[16];

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((c = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, pa) == 0, "socket_bind(a)");
        CHECK(sock_bind(b, pb) == 0, "socket_bind(b)");
        CHECK(sock_bind(c, pc) == 0, "socket_bind(c)");
        CHECK(sock_connect(a, pb) == 0, "socket_connect(a -> b)");

        // sendto other address than connected one
        mkaddr(&to, pc);
        CHECK(socket_sendto(a, "to-c", 4, NET_FLAGS__NONE, &to, sizeof(to)) == 4, "socket_sendto(a -> c)");
        memset(&from, 0, sizeof(from));
        int n = socket_recvfrom(c, buf, sizeof(buf), NET_FLAGS__NONE, &from, sizeof(from));
        CHECK((n == 4) && (memcmp(buf, "to-c", 4) == 0), "socket_recvfrom(c): %d", n);
        CHECK(from.port == pa, "sender port %u, expected %u", (unsigned)from.port, (unsigned)pa);

        // connection is not changed by sendto
        CHECK(socket_send(a, "to-b", 4, NET_FLAGS__NONE) == 4, "socket_send(a -> b)");
        n = socket_recv(b, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 4) && (memcmp(buf, "to-b", 4) == 0), "socket_recv(b): %d", n);

        socket_set_recv_timeout(c, 100);
        EXPECT_ERR(socket_recv(c, buf, sizeof(buf), NET_FLAGS__NONE), ETIME, "datagram of b received by c");

        // connected peer can send to socket
        mkaddr(&to, pa);
        CHECK(socket_sendto(b, "b-a", 3, NET_FLAGS__NONE, &to, sizeof(to)) == 3, "socket_sendto(b -> a)");
        n = socket_recv(a, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 3) && (memcmp(buf, "b-a", 3) == 0), "socket_recv(a): %d", n);

cleanup:
        sock_close(&a);
        sock_close(&b);
        sock_close(&c);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram sizes, order, NOCOPY transfer and size limit.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_sizes(void)
{
        static const u16_t SIZE[] = {1, 2, 15, 16, 100, 255, 256, 511, 1000, MAX_DATAGRAM};

        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        NET_INET_sockaddr_t to;
        u16_t pa = next_port();

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, pa) == 0, "socket_bind()");
        mkaddr(&to, pa);

        for (size_t round = 0; round < 2; round++) {
                // second round: buffer is not copied by stack
                NET_flags_t flags = (round == 0) ? NET_FLAGS__NONE : NET_FLAGS__NOCOPY;

                // send several datagrams, receiver does not read in the meantime
                for (size_t i = 0; i < ARRAY_SIZE(SIZE); i++) {
                        pattern_fill(global->txbuf, SIZE[i], 0, (u32_t)(i + round));
                        int n = socket_sendto(b, global->txbuf, SIZE[i], flags, &to, sizeof(to));
                        CHECK(n == SIZE[i], "socket_sendto(%u B, flags 0x%X): %d", (unsigned)SIZE[i],
                                (unsigned)flags, n);
                }

                for (size_t i = 0; i < ARRAY_SIZE(SIZE); i++) {
                        int n = socket_recv(a, global->rxbuf, IO_BUF_SIZE, NET_FLAGS__NONE);
                        CHECK(n == SIZE[i], "datagram %u: received %d B, expected %u B", (unsigned)i, n,
                                (unsigned)SIZE[i]);
                        CHECK(pattern_check(global->rxbuf, n, 0, (u32_t)(i + round)) == (size_t)n,
                                "datagram %u: wrong content", (unsigned)i);
                }
        }

        EXPECT_ERR(socket_sendto(b, global->txbuf, MAX_DATAGRAM + 1, NET_FLAGS__NONE, &to, sizeof(to)), EFBIG,
                "sendto too big datagram");

cleanup:
        sock_close(&a);
        sock_close(&b);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram receive timeout.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_timeout(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        char c;

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, next_port()) == 0, "socket_bind()");

        CHECK(socket_set_recv_timeout(a, 200) == 0, "socket_set_recv_timeout()");
        clock_t tref = get_uptime_ms();
        EXPECT_ERR(socket_recv(a, &c, 1, NET_FLAGS__NONE), ETIME, "receive without data");
        u32_t dt = elapsed_ms(tref);
        CHECK((dt >= 180) && (dt < 200 + INETTEST_WAKEUP_LIMIT), "receive timeout: %u ms, expected 200 ms",
                (unsigned)dt);

        CHECK(socket_set_recv_timeout(a, 0) == 0, "socket_set_recv_timeout(0)");
        tref = get_uptime_ms();
        EXPECT_ERR(socket_recv(a, &c, 1, NET_FLAGS__NONE), ETIME, "receive without data (no wait)");
        dt = elapsed_ms(tref);
        CHECK(dt < 100, "receive with zero timeout: %u ms", (unsigned)dt);

cleanup:
        sock_close(&a);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Stream connect, accept, echo in both directions and addresses.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_basic(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        NET_INET_sockaddr_t addr;
        u16_t port = next_port();
        char buf[16];

        int err = sock_pair(port, &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(send_all(cli, "abcdef", 6) == 0, "send(client)");
        CHECK(recv_all(acc, buf, 6) == 0 && (memcmp(buf, "abcdef", 6) == 0), "recv(server)");

        CHECK(send_all(acc, "123", 3) == 0, "send(server)");
        CHECK(recv_all(cli, buf, 3) == 0 && (memcmp(buf, "123", 3) == 0), "recv(client)");

        // receive returns available data, does not wait for full buffer
        CHECK(send_all(cli, "xy", 2) == 0, "send(client)");
        clock_t tref = get_uptime_ms();
        int n = socket_recv(acc, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 2) && (elapsed_ms(tref) < INETTEST_WAKEUP_LIMIT), "recv(available): %d", n);

        // connected sockets return address of peer
        memset(&addr, 0, sizeof(addr));
        CHECK(socket_get_address(cli, &addr, sizeof(addr)) == 0, "socket_get_address(client)");
        CHECK((addr.addr == NET_INET_IPv4_LOOPBACK) && (addr.port == port), "client peer address 0x%08X:%u",
                (unsigned)addr.addr, (unsigned)addr.port);

        memset(&addr, 0, sizeof(addr));
        CHECK(socket_get_address(acc, &addr, sizeof(addr)) == 0, "socket_get_address(accepted)");
        CHECK((addr.addr == NET_INET_IPv4_LOOPBACK) && (addr.port != 0) && (addr.port != port),
                "accepted peer address 0x%08X:%u", (unsigned)addr.addr, (unsigned)addr.port);

cleanup:
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Stream receive timeout and partial send on send timeout.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_timeouts(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        u8_t *big = NULL;
        char c;

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        // receive timeout
        CHECK(socket_set_recv_timeout(cli, 200) == 0, "socket_set_recv_timeout()");
        clock_t tref = get_uptime_ms();
        EXPECT_ERR(socket_recv(cli, &c, 1, NET_FLAGS__NONE), ETIME, "receive without data");
        u32_t dt = elapsed_ms(tref);
        CHECK((dt >= 180) && (dt < 200 + INETTEST_WAKEUP_LIMIT), "receive timeout: %u ms, expected 200 ms",
                (unsigned)dt);

        CHECK(socket_set_recv_timeout(cli, 0) == 0, "socket_set_recv_timeout(0)");
        tref = get_uptime_ms();
        EXPECT_ERR(socket_recv(cli, &c, 1, NET_FLAGS__NONE), ETIME, "receive without data (no wait)");
        dt = elapsed_ms(tref);
        CHECK(dt < 100, "receive with zero timeout: %u ms", (unsigned)dt);

        // send timeout: peer does not read, so buffers are full
        big = malloc(INETTEST_FILL_SIZE);
        if (!big) {
                exit_msg = SKIP("not enough free memory");
                goto cleanup;
        }

        pattern_fill(big, INETTEST_FILL_SIZE, 0, 11);

        CHECK(socket_set_send_timeout(cli, 300) == 0, "socket_set_send_timeout()");
        tref = get_uptime_ms();
        int sent = socket_send(cli, big, INETTEST_FILL_SIZE, NET_FLAGS__NONE);
        dt = elapsed_ms(tref);
        CHECK((sent > 0) && (sent < INETTEST_FILL_SIZE), "send to full buffer: %d, expected partial send",
                sent);
        CHECK((dt >= 250) && (dt < 300 + INETTEST_WAKEUP_LIMIT), "partial send: %u ms, expected 300 ms",
                (unsigned)dt);

        /*
         * Next send fails on timeout. Small part of data can still be accepted
         * by stack (acknowledgment of data in flight), so send is repeated.
         */
        int n = 0;
        for (int i = 0; (i < 3) && (sent < INETTEST_FILL_SIZE); i++) {
                tref = get_uptime_ms();
                n = socket_send(cli, big + sent, INETTEST_FILL_SIZE - sent, NET_FLAGS__NONE);
                err = errno;
                dt = elapsed_ms(tref);
                if (n < 0) {
                        break;
                }
                sent += n;
        }
        CHECK((n < 0) && (err == ETIME), "send to full buffer: %d (%s), expected ETIME", n, strerror(err));
        CHECK((dt >= 250) && (dt < 300 + INETTEST_WAKEUP_LIMIT), "send timeout: %u ms, expected 300 ms",
                (unsigned)dt);

        // peer receives exactly the data that was reported as sent
        size_t recved = 0;
        while (recved < (size_t)sent) {
                int n = socket_recv(acc, global->rxbuf, min(IO_BUF_SIZE, (size_t)sent - recved),
                        NET_FLAGS__NONE);
                CHECK(n > 0, "recv(%u of %d)", (unsigned)recved, sent);
                CHECK(pattern_check(global->rxbuf, n, recved, 11) == (size_t)n, "wrong data at %u",
                        (unsigned)recved);
                recved += n;
        }

        socket_set_recv_timeout(acc, 200);
        EXPECT_ERR(socket_recv(acc, &c, 1, NET_FLAGS__NONE), ETIME, "data that was not reported as sent");

cleanup:
        free(big);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Peer close delivers remaining data and ends connection; close with
 *         unread data resets connection.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_peer_close(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        char buf[8];

        // graceful close
        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(send_all(acc, "last", 4) == 0, "send()");
        sock_close(&acc);

        CHECK(recv_all(cli, buf, 4) == 0 && (memcmp(buf, "last", 4) == 0), "remaining data not received");

        clock_t tref = get_uptime_ms();
        int n = socket_recv(cli, buf, sizeof(buf), NET_FLAGS__NONE);
        err = errno;
        CHECK((n < 0) && is_conn_end(err), "recv after peer close: %d (%s)", n, strerror(err));
        CHECK(elapsed_ms(tref) < INETTEST_WAKEUP_LIMIT, "end of connection detected after %u ms",
                (unsigned)elapsed_ms(tref));

        sock_close(&cli);
        sock_close(&srv);

        // close with unread data
        err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(send_all(cli, "unread", 6) == 0, "send()");
        msleep(50);
        sock_close(&acc);

        tref = get_uptime_ms();
        n = socket_recv(cli, buf, sizeof(buf), NET_FLAGS__NONE);
        err = errno;
        CHECK((n < 0) && is_conn_end(err), "recv after peer close: %d (%s)", n, strerror(err));
        CHECK(elapsed_ms(tref) < INETTEST_WAKEUP_LIMIT, "end of connection detected after %u ms",
                (unsigned)elapsed_ms(tref));

cleanup:
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Send is not blocked by other thread that waits in receive on the
 *         same socket (separate receive and send locks).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_full_duplex_wakeup(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;
        char buf[8];
        char rbuf[8];

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        job.sock = cli;
        job.buf = (u8_t *)rbuf;
        job.len = sizeof(rbuf);
        CHECK(job_start(&job, th_recv), "unable to start thread");
        msleep(100);

        // receive of other thread is in progress
        clock_t tref = get_uptime_ms();
        int n = socket_send(cli, "ping", 4, NET_FLAGS__NONE);
        u32_t dt = elapsed_ms(tref);
        CHECK(n == 4, "send while other thread receives: %d", n);
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "send blocked by receiving thread for %u ms", (unsigned)dt);

        CHECK(recv_all(acc, buf, 4) == 0 && (memcmp(buf, "ping", 4) == 0), "recv(server)");
        CHECK(send_all(acc, "pong", 4) == 0, "send(server)");

        job_join(&job);
        CHECK((job.ret == 4) && (memcmp(rbuf, "pong", 4) == 0), "receiving thread: %d (%s)", job.ret,
                strerror(job.err));

        // timeout setting does not wait for receiving thread
        CHECK(job_start(&job, th_recv), "unable to start thread");
        msleep(100);
        tref = get_uptime_ms();
        CHECK(socket_set_send_timeout(cli, SAFE_TIMEOUT) == 0, "socket_set_send_timeout()");
        dt = elapsed_ms(tref);
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "set timeout blocked by receiving thread for %u ms", (unsigned)dt);
        CHECK(send_all(acc, "end", 3) == 0, "send(server)");
        job_join(&job);

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Both sides of connection send and receive at the same time, each
 *         direction by own thread (the same socket used by two threads).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_full_duplex_bulk(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job[4];
        u8_t *buf = NULL;
        size_t started = 0;

        memset(job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        buf = malloc(BULK_CHUNK * ARRAY_SIZE(job));
        if (!buf) {
                exit_msg = SKIP("not enough free memory");
                goto cleanup;
        }

        // client -> server: seed 1, server -> client: seed 2
        static const struct {
                bool client;
                bool tx;
                u32_t seed;
        } CFG[4] = {
                {true, true, 1},
                {true, false, 2},
                {false, true, 2},
                {false, false, 1},
        };

        clock_t tref = get_uptime_ms();

        for (; started < ARRAY_SIZE(job); started++) {
                job[started].sock = CFG[started].client ? cli : acc;
                job[started].buf = &buf[started * BULK_CHUNK];
                job[started].len = BULK_CHUNK;
                job[started].count = INETTEST_BULK_SIZE;
                job[started].seed = CFG[started].seed;

                if (!job_start(&job[started], CFG[started].tx ? th_bulk_send : th_bulk_recv)) {
                        break;
                }
        }

        for (size_t i = 0; i < started; i++) {
                job_join(&job[i]);
        }

        u32_t dt = elapsed_ms(tref);

        CHECK(started == ARRAY_SIZE(job), "unable to start thread");

        for (size_t i = 0; i < ARRAY_SIZE(job); i++) {
                CHECK(job[i].err == 0, "%s %s: %u B, %s", CFG[i].client ? "client" : "server",
                        CFG[i].tx ? "send" : "recv", (unsigned)job[i].bytes,
                        (job[i].err == EILSEQ) ? "wrong data" : strerror(job[i].err));
                CHECK(job[i].bytes == INETTEST_BULK_SIZE, "transferred %u B", (unsigned)job[i].bytes);
        }

        if (global->verbose) {
                printf("  2 x 2 x %u B in %u ms\n", (unsigned)INETTEST_BULK_SIZE, (unsigned)dt);
        }

cleanup:
        for (size_t i = 0; i < started; i++) {
                job_join(&job[i]);
        }
        free(buf);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Close of socket wakes thread that waits in receive on the socket.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_close_wakes_recv(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;
        char buf[8];

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        job.sock = cli;
        job.buf = (u8_t *)buf;
        job.len = sizeof(buf);
        CHECK(job_start(&job, th_recv), "unable to start thread");
        msleep(200);

        clock_t tref = get_uptime_ms();
        sock_close(&cli);
        u32_t dt_close = elapsed_ms(tref);
        job_join(&job);
        u32_t dt = (u32_t)(job.t_end - tref);

        CHECK((job.ret < 0) && is_abort(job.err), "blocked recv: %d (%s), expected ECONNABORTED or EBADF",
                job.ret, strerror(job.err));
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "blocked recv woken after %u ms", (unsigned)dt);
        CHECK(dt_close < INETTEST_WAKEUP_LIMIT, "close took %u ms", (unsigned)dt_close);

        // peer sees end of connection
        char c;
        int n = socket_recv(acc, &c, 1, NET_FLAGS__NONE);
        err = errno;
        CHECK((n < 0) && is_conn_end(err), "peer recv: %d (%s)", n, strerror(err));

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Close of socket wakes thread that waits in send (buffers are full).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_close_wakes_send(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        u8_t *big = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        big = malloc(INETTEST_FILL_SIZE);
        if (!big) {
                exit_msg = SKIP("not enough free memory");
                goto cleanup;
        }

        memset(big, 0xAA, INETTEST_FILL_SIZE);

        // peer does not read, send blocks
        CHECK(socket_set_send_timeout(cli, SAFE_TIMEOUT * 2) == 0, "socket_set_send_timeout()");
        job.sock = cli;
        job.buf = big;
        job.len = INETTEST_FILL_SIZE;
        CHECK(job_start(&job, th_send), "unable to start thread");
        msleep(300);
        CHECK(job.t_end == 0, "send was not blocked (%d)", job.ret);

        clock_t tref = get_uptime_ms();
        sock_close(&cli);
        job_join(&job);
        u32_t dt = (u32_t)(job.t_end - tref);

        // part of data is sent before close, so partial send is correct result
        CHECK(((job.ret >= 0) && (job.ret < INETTEST_FILL_SIZE)) || ((job.ret < 0) && is_abort(job.err)),
                "blocked send: %d (%s)", job.ret, strerror(job.err));
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "blocked send woken after %u ms", (unsigned)dt);

cleanup:
        job_join(&job);
        free(big);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Close of listening socket wakes thread that waits in accept.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_close_wakes_accept(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        CHECK((srv = sock_listen(next_port())) != NULL, "listen");
        CHECK(socket_set_recv_timeout(srv, SAFE_TIMEOUT * 2) == 0, "socket_set_recv_timeout()");

        job.sock = srv;
        CHECK(job_start(&job, th_accept), "unable to start thread");
        msleep(200);

        clock_t tref = get_uptime_ms();
        sock_close(&srv);
        job_join(&job);
        u32_t dt = (u32_t)(job.t_end - tref);

        CHECK((job.ret != 0) && is_abort(job.err), "blocked accept: %d (%s), expected ECONNABORTED or EBADF",
                job.ret, strerror(job.err));
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "blocked accept woken after %u ms", (unsigned)dt);

cleanup:
        job_join(&job);
        sock_close(&job.acc);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Shutdown of receive direction wakes thread that waits in receive,
 *         send direction still works.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_shutdown_wakes_recv(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;
        char buf[8];

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(socket_set_recv_timeout(cli, SAFE_TIMEOUT * 2) == 0, "socket_set_recv_timeout()");
        job.sock = cli;
        job.buf = (u8_t *)buf;
        job.len = sizeof(buf);
        CHECK(job_start(&job, th_recv), "unable to start thread");
        msleep(200);

        clock_t tref = get_uptime_ms();
        CHECK(socket_shutdown(cli, NET_SHUT__RD) == 0, "socket_shutdown(RD)");
        u32_t dt_shut = elapsed_ms(tref);
        job_join(&job);
        u32_t dt = (u32_t)(job.t_end - tref);

        CHECK(job.ret < 0, "blocked recv returned %d after shutdown", job.ret);
        CHECK(dt < INETTEST_WAKEUP_LIMIT, "blocked recv woken after %u ms", (unsigned)dt);
        CHECK(dt_shut < INETTEST_WAKEUP_LIMIT, "shutdown took %u ms", (unsigned)dt_shut);

        // next receive fails immediately
        tref = get_uptime_ms();
        CHECK(socket_recv(cli, buf, sizeof(buf), NET_FLAGS__NONE) < 0, "recv after shutdown(RD) succeeded");
        CHECK(elapsed_ms(tref) < INETTEST_WAKEUP_LIMIT, "recv after shutdown(RD) took %u ms",
                (unsigned)elapsed_ms(tref));

        // send direction is not affected
        CHECK(send_all(cli, "wr", 2) == 0, "send after shutdown(RD)");
        CHECK(recv_all(acc, buf, 2) == 0 && (memcmp(buf, "wr", 2) == 0), "recv(server)");

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several client threads connect to one server at the same time.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_concurrent_clients(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        job_t server;
        job_t client[INETTEST_CLIENTS];
        size_t started = 0;
        u16_t port = next_port();

        memset(&server, 0, sizeof(server));
        memset(client, 0, sizeof(client));

        CHECK((srv = sock_listen(port)) != NULL, "listen");

        server.sock = srv;
        server.count = INETTEST_CLIENTS * INETTEST_CLIENT_ROUNDS;
        CHECK(job_start(&server, th_echo_server), "unable to start server thread");

        for (; started < INETTEST_CLIENTS; started++) {
                client[started].port = port;
                client[started].count = INETTEST_CLIENT_ROUNDS;
                client[started].seed = (u32_t)(started * 31 + 5);
                if (!job_start(&client[started], th_echo_client)) {
                        break;
                }
        }

        for (size_t i = 0; i < started; i++) {
                job_join(&client[i]);
        }

        server.stop = true;
        job_join(&server);

        CHECK(started == INETTEST_CLIENTS, "unable to start client thread");

        for (size_t i = 0; i < INETTEST_CLIENTS; i++) {
                CHECK(client[i].err == 0, "client %u, round %u: %s", (unsigned)i, (unsigned)client[i].ops,
                        (client[i].err == EILSEQ) ? "wrong echo" : strerror(client[i].err));
        }

        CHECK(server.err == 0, "server: %s", strerror(server.err));
        CHECK(server.ops == INETTEST_CLIENTS * INETTEST_CLIENT_ROUNDS, "server handled %u connections",
                (unsigned)server.ops);

cleanup:
        job_join(&server);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Kill of process that waits in datagram receive. Process ends in time
 *         and its socket is released (port can be bound again).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_kill_dgram_recv(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *sock = NULL;
        SOCKET *rebind = NULL;
        NET_INET_sockaddr_t from;
        pid_t pid = 0;
        u16_t port = next_port();
        char c;

        CHECK((sock = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(sock, port) == 0, "socket_bind()");

        pid = start_child("udp", port);
        if (pid == 0) {
                exit_msg = SKIP("unable to start " PROGRAM_NAME " process");
                goto cleanup;
        }

        memset(&from, 0, sizeof(from));
        int n = socket_recvfrom(sock, &c, 1, NET_FLAGS__NONE, &from, sizeof(from));
        CHECK((n == 1) && (c == 'R'), "child process not ready: %d", n);

        msleep(200); // child waits in receive

        clock_t tref = get_uptime_ms();
        CHECK(process_kill(pid) == 0, "process_kill()");
        int status = 0;
        CHECK(process_wait(pid, &status, INETTEST_KILL_LIMIT * 2) == 0, "process_wait()");
        u32_t dt = elapsed_ms(tref);
        pid = 0;

        CHECK(dt < INETTEST_KILL_LIMIT, "killed process finished after %u ms", (unsigned)dt);

        // socket of killed process is released
        CHECK((rebind = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(rebind, from.port) == 0, "port %u of killed process is still used",
                (unsigned)from.port);

cleanup:
        if (pid) {
                process_kill(pid);
                int status;
                process_wait(pid, &status, INETTEST_KILL_LIMIT * 2);
        }
        sock_close(&rebind);
        sock_close(&sock);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Kill of process that waits in stream send (peer does not read).
 *         Process ends in time and connection is closed.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_kill_stream_send(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *acc = NULL;
        pid_t pid = 0;
        u16_t port = next_port();
        char c;

        CHECK((srv = sock_listen(port)) != NULL, "listen");

        pid = start_child("tcp", port);
        if (pid == 0) {
                exit_msg = SKIP("unable to start " PROGRAM_NAME " process");
                goto cleanup;
        }

        CHECK(socket_accept(srv, &acc) == 0, "socket_accept()");
        socket_set_recv_timeout(acc, SAFE_TIMEOUT);
        CHECK((socket_recv(acc, &c, 1, NET_FLAGS__NONE) == 1) && (c == 'R'), "child process not ready");

        msleep(300); // child fills buffers and waits in send

        clock_t tref = get_uptime_ms();
        CHECK(process_kill(pid) == 0, "process_kill()");
        int status = 0;
        CHECK(process_wait(pid, &status, INETTEST_KILL_LIMIT * 2) == 0, "process_wait()");
        u32_t dt = elapsed_ms(tref);
        pid = 0;

        CHECK(dt < INETTEST_KILL_LIMIT, "killed process finished after %u ms", (unsigned)dt);

        // connection of killed process is closed (after data that was sent)
        size_t drained = 0;
        int err = 0;
        for (;;) {
                int n = socket_recv(acc, global->rxbuf, IO_BUF_SIZE, NET_FLAGS__NONE);
                if (n <= 0) {
                        err = (n < 0) ? errno : ETIME;
                        break;
                }
                drained += n;
        }

        CHECK(is_conn_end(err), "connection of killed process: %s after %u B", strerror(err),
                (unsigned)drained);

cleanup:
        if (pid) {
                process_kill(pid);
                int status;
                process_wait(pid, &status, INETTEST_KILL_LIMIT * 2);
        }
        sock_close(&acc);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Memory of closed sockets is released. Network memory is checked
 *         for sockets without connections (closed TCP connections stay in
 *         TIME_WAIT state for 2*MSL), kernel memory (socket locks, events)
 *         is checked for connected stream sockets.
 *
 * @note   Other programs that use network or create kernel objects can
 *         cause false result.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_memory_leak(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        size_t net_start, krn_start, net_end, krn_end;
        NET_INET_sockaddr_t to;
        char buf[8];

        if (!mem_usage(&net_start, &krn_start) || ((net_start == 0) && (krn_start == 0))) {
                return SKIP("memory statistics not available");
        }

        // sockets without connections (UDP and listening sockets release port at close)
        u16_t port = next_port();
        u16_t listen_port = next_port();

        for (int i = 0; i < 20; i++) {

                CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
                CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
                CHECK(sock_bind(a, port) == 0, "socket_bind()");
                mkaddr(&to, port);
                CHECK(socket_sendto(b, "mem", 3, NET_FLAGS__NONE, &to, sizeof(to)) == 3, "socket_sendto()");
                CHECK(socket_recv(a, buf, sizeof(buf), NET_FLAGS__NONE) == 3, "socket_recv()");
                sock_close(&a);
                sock_close(&b);

                CHECK((srv = sock_listen(listen_port)) != NULL, "listen");
                sock_close(&srv);
        }

        msleep(200);
        CHECK(mem_usage(&net_end, &krn_end), "get_memory_usage_details()");
        CHECK(net_end <= net_start, "network memory: %u B before, %u B after (+%u B)", (unsigned)net_start,
                (unsigned)net_end, (unsigned)(net_end - net_start));

        // connected stream sockets (first connection creates lazy objects)
        int err = sock_pair(next_port(), &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);

        msleep(100);
        CHECK(mem_usage(&net_start, &krn_start), "get_memory_usage_details()");

        for (int i = 0; i < 10; i++) {
                err = sock_pair(next_port(), &srv, &cli, &acc);
                CHECK(err == 0, "connection: %s", strerror(err));
                CHECK(send_all(cli, "m", 1) == 0, "send()");
                CHECK(recv_all(acc, buf, 1) == 0, "recv()");
                sock_close(&acc);
                sock_close(&cli);
                sock_close(&srv);
        }

        msleep(200);
        CHECK(mem_usage(&net_end, &krn_end), "get_memory_usage_details()");
        CHECK(krn_end <= krn_start + KERNEL_MEM_TOLERANCE, "kernel memory: %u B before, %u B after (+%u B)",
                (unsigned)krn_start, (unsigned)krn_end, (unsigned)(krn_end - krn_start));

cleanup:
        sock_close(&a);
        sock_close(&b);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

/*==============================================================================
  Performance measurements
==============================================================================*/

//==============================================================================
/**
 * @brief  Stream throughput (one direction) with selected send chunk.
 *
 * @param  chunk        send chunk size
 */
//==============================================================================
static void bench_stream_throughput(size_t chunk)
{
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        if (err) {
                res_printf("connection error: %s", strerror(err));
                return;
        }

        socket_set_recv_timeout(acc, BENCH_TIMEOUT);

        job.sock = cli;
        job.buf = global->txbuf;
        job.len = chunk;
        job.seed = 9;
        job.duration = global->duration;

        clock_t tref = get_uptime_ms();

        if (!job_start(&job, th_stream_sink_sender)) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        size_t recved = 0;
        bool data_ok = true;
        u32_t dt = 0;

        for (;;) {
                int n = socket_recv(acc, global->rxbuf, IO_BUF_SIZE, NET_FLAGS__NONE);
                if (n <= 0) {
                        break; // timeout after sender finished
                }

                if (data_ok && (pattern_check(global->rxbuf, n, recved, job.seed) != (size_t)n)) {
                        data_ok = false;
                }

                recved += n;
                dt = elapsed_ms(tref);
        }

        job.stop = true;
        job_join(&job);

        if (dt == 0) {
                dt = 1;
        }

        res_printf("%9.1f KiB/s", (double)recved * 1000.0 / 1024.0 / dt);

        if (global->verbose) {
                res_printf(" (%u B in %u ms)", (unsigned)recved, (unsigned)dt);
        }

        if (job.err) {
                res_printf(VT100_FONT_COLOR_RED " send error: %s" VT100_RESET_ATTRIBUTES, strerror(job.err));
        }

        if (!data_ok || (recved != job.bytes)) {
                res_printf(VT100_FONT_COLOR_RED
                        " DATA ERROR (sent %u B, received %u B)" VT100_RESET_ATTRIBUTES,
                        (unsigned)job.bytes, (unsigned)recved);
        }

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
}

//==============================================================================
/**
 * @brief  Stream throughput in both directions at the same time. Each side
 *         sends and receives by two threads that use the same socket.
 *
 * @param  chunk        send chunk size
 */
//==============================================================================
static void bench_stream_duplex(size_t chunk)
{
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        u8_t *buf = NULL;
        job_t tx[2];
        job_t rx[2];

        memset(tx, 0, sizeof(tx));
        memset(rx, 0, sizeof(rx));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        if (err) {
                res_printf("connection error: %s", strerror(err));
                return;
        }

        buf = malloc(chunk * 4);
        if (!buf) {
                res_printf("not enough free memory");
                goto cleanup;
        }

        socket_set_recv_timeout(cli, BENCH_TIMEOUT);
        socket_set_recv_timeout(acc, BENCH_TIMEOUT);

        SOCKET *sock[2] = {cli, acc};

        for (size_t i = 0; i < 2; i++) {
                tx[i].sock = sock[i];
                tx[i].buf = &buf[i * chunk];
                tx[i].len = chunk;
                tx[i].seed = (u32_t)(20 + i);
                tx[i].duration = global->duration;

                rx[i].sock = sock[1 - i];
                rx[i].buf = &buf[(2 + i) * chunk];
                rx[i].len = chunk;
                rx[i].count = SIZE_MAX;
                rx[i].seed = tx[i].seed;
        }

        clock_t tref = get_uptime_ms();

        bool ok = job_start(&rx[0], th_bulk_recv) && job_start(&rx[1], th_bulk_recv) &&
                job_start(&tx[0], th_stream_sink_sender) && job_start(&tx[1], th_stream_sink_sender);

        job_join(&tx[0]);
        job_join(&tx[1]);
        job_join(&rx[0]); // receivers end by timeout
        job_join(&rx[1]);

        u32_t dt = elapsed_ms(tref);
        if (dt > BENCH_TIMEOUT) {
                dt -= BENCH_TIMEOUT;
        }

        if (!ok) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        res_printf("%9.1f KiB/s (both directions)",
                (double)(rx[0].bytes + rx[1].bytes) * 1000.0 / 1024.0 / dt);

        for (size_t i = 0; i < 2; i++) {
                if (tx[i].err || (rx[i].err != ETIME) || (rx[i].bytes != tx[i].bytes)) {
                        res_printf(VT100_FONT_COLOR_RED
                                " ERROR (sent %u B: %s, received %u B: %s)" VT100_RESET_ATTRIBUTES,
                                (unsigned)tx[i].bytes, strerror(tx[i].err), (unsigned)rx[i].bytes,
                                (rx[i].err == EILSEQ) ? "wrong data" : strerror(rx[i].err));
                }
        }

cleanup:
        for (size_t i = 0; i < 2; i++) {
                tx[i].stop = true;
                job_join(&tx[i]);
                job_join(&rx[i]);
        }
        free(buf);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
}

//==============================================================================
/**
 * @brief  Stream round trip time (request and echo) of selected message size.
 *
 * @param  size         message size
 */
//==============================================================================
static void bench_stream_rtt(size_t size)
{
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        int err = sock_pair(next_port(), &srv, &cli, &acc);
        if (err) {
                res_printf("connection error: %s", strerror(err));
                return;
        }

        socket_set_recv_timeout(acc, BENCH_TIMEOUT / 4);
        socket_set_recv_timeout(cli, BENCH_TIMEOUT);

        job.sock = acc;
        job.buf = global->rxbuf;
        job.len = size;

        if (!job_start(&job, th_stream_echo)) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        pattern_fill(global->txbuf, size, 0, 4);

        size_t count = 0;
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->duration) {
                err = send_all(cli, global->txbuf, size);
                if (!err) {
                        err = recv_all(cli, global->txbuf + 2048, size);
                }
                if (err) {
                        break;
                }
                count++;
        }

        u32_t dt = elapsed_ms(tref);

        job.stop = true;
        job_join(&job);

        if (count) {
                res_printf("%9.1f us (%u round trips/s)", (double)dt * 1000.0 / count,
                        (unsigned)(count * 1000 / dt));
        }

        if (err) {
                res_printf(VT100_FONT_COLOR_RED " error: %s" VT100_RESET_ATTRIBUTES, strerror(err));
        }

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
}

//==============================================================================
/**
 * @brief  Datagram throughput of selected datagram size.
 *
 * @param  size         datagram size
 */
//==============================================================================
static void bench_dgram_throughput(size_t size)
{
        SOCKET *rx = NULL;
        SOCKET *tx = NULL;
        NET_INET_sockaddr_t from;
        u16_t port = next_port();
        job_t job;

        memset(&job, 0, sizeof(job));

        rx = sock_new(NET_SOCKET_TYPE__DATAGRAM);
        tx = sock_new(NET_SOCKET_TYPE__DATAGRAM);

        if (!rx || !tx || (sock_bind(rx, port) != 0)) {
                res_printf("socket error: %s", strerror(errno));
                goto cleanup;
        }

        socket_set_recv_timeout(rx, BENCH_TIMEOUT);

        job.sock = tx;
        job.port = port;
        job.buf = global->txbuf;
        job.len = size;
        job.seed = 5;
        job.duration = global->duration;

        clock_t tref = get_uptime_ms();

        if (!job_start(&job, th_dgram_sender)) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        size_t bytes = 0;
        size_t count = 0;
        size_t bad = 0;
        u32_t dt = 0;

        for (;;) {
                int n = socket_recvfrom(rx, global->rxbuf, IO_BUF_SIZE, NET_FLAGS__NONE, &from, sizeof(from));
                if ((n <= 0) || (n == DGRAM_END_MARK)) {
                        break;
                }

                if (((size_t)n != size) || (pattern_check(global->rxbuf, n, 0, job.seed) != (size_t)n)) {
                        bad++;
                }

                bytes += n;
                count++;
                dt = elapsed_ms(tref);
        }

        job.stop = true;
        job_join(&job);

        if (dt == 0) {
                dt = 1;
        }

        res_printf("%9.1f KiB/s (%u datagrams/s)", (double)bytes * 1000.0 / 1024.0 / dt,
                (unsigned)(count * 1000 / dt));

        if (count < job.ops) {
                // datagrams can be dropped when receiver is slower than sender
                res_printf(", lost %u of %u", (unsigned)(job.ops - count), (unsigned)job.ops);
        }

        if (job.err) {
                res_printf(VT100_FONT_COLOR_RED " send error: %s" VT100_RESET_ATTRIBUTES, strerror(job.err));
        }

        if (bad) {
                res_printf(VT100_FONT_COLOR_RED " DATA ERROR (wrong %u)" VT100_RESET_ATTRIBUTES,
                        (unsigned)bad);
        }

cleanup:
        job_join(&job);
        sock_close(&rx);
        sock_close(&tx);
}

//==============================================================================
/**
 * @brief  Datagram round trip time (request and echo).
 *
 * @param  size         datagram size
 */
//==============================================================================
static void bench_dgram_rtt(size_t size)
{
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        NET_INET_sockaddr_t to, from;
        u16_t port = next_port();
        job_t job;

        memset(&job, 0, sizeof(job));

        srv = sock_new(NET_SOCKET_TYPE__DATAGRAM);
        cli = sock_new(NET_SOCKET_TYPE__DATAGRAM);

        if (!srv || !cli || (sock_bind(srv, port) != 0)) {
                res_printf("socket error: %s", strerror(errno));
                goto cleanup;
        }

        socket_set_recv_timeout(srv, BENCH_TIMEOUT / 4);
        socket_set_recv_timeout(cli, BENCH_TIMEOUT);

        job.sock = srv;
        job.buf = global->rxbuf;
        job.len = IO_BUF_SIZE;

        if (!job_start(&job, th_dgram_echo)) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        mkaddr(&to, port);
        pattern_fill(global->txbuf, size, 0, 6);

        size_t count = 0;
        int err = 0;
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->duration) {
                int n = socket_sendto(cli, global->txbuf, size, NET_FLAGS__NONE, &to, sizeof(to));
                if (n == (int)size) {
                        n = socket_recvfrom(cli, global->txbuf + 2048, IO_BUF_SIZE - 2048, NET_FLAGS__NONE,
                                &from, sizeof(from));
                }
                if (n != (int)size) {
                        err = (n < 0) ? errno : EIO;
                        break;
                }
                count++;
        }

        u32_t dt = elapsed_ms(tref);

        job.stop = true;
        job_join(&job);

        if (count) {
                res_printf("%9.1f us (%u round trips/s)", (double)dt * 1000.0 / count,
                        (unsigned)(count * 1000 / dt));
        }

        if (err) {
                res_printf(VT100_FONT_COLOR_RED " error: %s" VT100_RESET_ATTRIBUTES, strerror(err));
        }

cleanup:
        job_join(&job);
        sock_close(&srv);
        sock_close(&cli);
}

//==============================================================================
/**
 * @brief  Connection rate: open, connect, 1 byte request/response, close.
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_connect_rate(size_t unused)
{
        UNUSED_ARG1(unused);

        SOCKET *srv = NULL;
        u16_t port = next_port();
        job_t job;

        memset(&job, 0, sizeof(job));

        srv = sock_listen(port);
        if (!srv) {
                res_printf("socket error: %s", strerror(errno));
                return;
        }

        socket_set_recv_timeout(srv, BENCH_TIMEOUT / 4);

        job.sock = srv;

        if (!job_start(&job, th_conn_server)) {
                res_printf("unable to start thread");
                goto cleanup;
        }

        size_t count = 0;
        int err = 0;
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->duration) {
                SOCKET *sock = sock_new(NET_SOCKET_TYPE__STREAM);
                if (!sock) {
                        err = errno ? errno : ENOMEM;
                        break;
                }

                u8_t byte = (u8_t)count;
                socket_set_recv_timeout(sock, BENCH_TIMEOUT);

                if (sock_connect(sock, port) != 0) {
                        err = errno ? errno : EIO;
                } else {
                        err = send_all(sock, &byte, 1);
                        if (!err) {
                                err = recv_all(sock, &byte, 1);
                        }
                }

                sock_close(&sock);

                if (err) {
                        break;
                }

                count++;
        }

        u32_t dt = elapsed_ms(tref);

        job.stop = true;
        job_join(&job);

        if (count) {
                res_printf("%9.1f connections/s (%.2f ms per connection)", (double)count * 1000.0 / dt,
                        (double)dt / count);
        }

        if (err) {
                res_printf(VT100_FONT_COLOR_RED " error: %s" VT100_RESET_ATTRIBUTES, strerror(err));
        }

cleanup:
        job_join(&job);
        sock_close(&srv);
}

/*==============================================================================
  End of file
==============================================================================*/
