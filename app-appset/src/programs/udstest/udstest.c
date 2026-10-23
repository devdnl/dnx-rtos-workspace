/*==============================================================================
File    udstest.c

Author  Daniel Zorychta

Brief   Unix Domain Socket (UDS) test and benchmark program.

        Usage: udstest [-t|-p|-l] [-v] [-n netname] [-d ms] [filter]
          (none)  functional tests, then performance measurements
          -t      functional tests only
          -p      performance measurements only (also works with older UDS
                  implementations, so results of kernels can be compared)
          -l      list tests and measurements
          -v      verbose mode
          -n      UDS network name (default: unix)
          -d      duration of single measurement in ms (default: 2000)
          filter  run only tests/measurements which name contains given text

        Functional tests check behaviour of the UDS stack including closing
        of sockets that are used by other threads (wake-up of blocked
        operations). Test socket paths have prefix "udst<pid>." so many
        instances can run at the same time.

        NOTE: functional tests expect UDS with lifetime and wake-up fixes.
              Older UDS fails most of tests and closing of connected sockets
              there can corrupt kernel memory, so use only -p with it.

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
/** Default network name of UDS interface (created by initd). */
#ifndef UDSTEST_NETNAME
#define UDSTEST_NETNAME "unix"
#endif

/** Default duration of single performance measurement [ms]. */
#ifndef UDSTEST_DURATION
#define UDSTEST_DURATION 2000
#endif

/** Maximum time of wake-up of blocked operation [ms]. */
#ifndef UDSTEST_WAKEUP_LIMIT
#define UDSTEST_WAKEUP_LIMIT 500
#endif

/** Size of data transferred in bulk transfer test (each direction). */
#ifndef UDSTEST_BULK_SIZE
#define UDSTEST_BULK_SIZE (24 * 1024)
#endif

/** Number of client threads in concurrent clients test. */
#ifndef UDSTEST_CLIENTS
#define UDSTEST_CLIENTS 3
#endif

/** Number of connections of each client in concurrent clients test. */
#ifndef UDSTEST_CLIENT_ROUNDS
#define UDSTEST_CLIENT_ROUNDS 10
#endif

#define IO_BUF_SIZE    4096
#define SAFE_TIMEOUT   5000 /* timeout that protects test from hang [ms] */
#define BENCH_TIMEOUT  1000 /* receive timeout of benchmark helpers [ms] */
#define DGRAM_END_MARK 1    /* size of datagram that ends datagram stream */
#define PREFIX_LEN     20

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
        const char *name;   //!< socket name (connect, sendto)
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
} job_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_usage(const char *name);
static bool selected(const char *name, const char *filter);
static size_t run_tests(const char *filter);
static size_t run_benchmarks(const char *filter);
static const char *make_msg(int err, const char *fmt, ...);
static void res_printf(const char *fmt, ...);
static u32_t elapsed_ms(clock_t tref);
static void mkaddr(NET_UDS_sockaddr_t *addr, const char *name);
static SOCKET *sock_new(NET_socket_type_t type);
static void sock_close(SOCKET **sock);
static int sock_bind(SOCKET *sock, const char *name);
static int sock_connect(SOCKET *sock, const char *name);
static SOCKET *sock_listen(const char *name);
static int sock_pair(const char *name, SOCKET **server, SOCKET **client, SOCKET **accepted);
static int send_all(SOCKET *sock, const void *buf, size_t len);
static int recv_all(SOCKET *sock, void *buf, size_t len);
static void pattern_fill(u8_t *buf, size_t len, size_t offset, u32_t seed);
static size_t pattern_check(const u8_t *buf, size_t len, size_t offset, u32_t seed);
static bool job_start(job_t *job, thread_func_t func);
static void job_join(job_t *job);
static size_t net_mem_usage(void);

static int th_accept(void *arg);
static int th_connect(void *arg);
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

static const char *test_dgram_basic(void);
static const char *test_dgram_errors(void);
static const char *test_dgram_boundaries(void);
static const char *test_dgram_timeout(void);
static const char *test_stream_basic(void);
static const char *test_stream_conn_errors(void);
static const char *test_stream_bulk(void);
static const char *test_stream_partial_send(void);
static const char *test_stream_shutdown(void);
static const char *test_stream_peer_close_recv(void);
static const char *test_stream_peer_close_send(void);
static const char *test_stream_listener_close(void);
static const char *test_stream_connect_timeout(void);
static const char *test_concurrent_clients(void);
static const char *test_memory_leak(void);

static void bench_stream_throughput(size_t chunk);
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
        size_t mem_start;
        char prefix[PREFIX_LEN];
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
        {.name = "Datagram: send and receive", .fn = test_dgram_basic},
        {.name = "Datagram: error handling", .fn = test_dgram_errors},
        {.name = "Datagram: message boundaries and order", .fn = test_dgram_boundaries},
        {.name = "Datagram: receive timeout", .fn = test_dgram_timeout},
        {.name = "Stream: connect, accept, echo", .fn = test_stream_basic},
        {.name = "Stream: connection errors", .fn = test_stream_conn_errors},
        {.name = "Stream: concurrent bulk transfers", .fn = test_stream_bulk},
        {.name = "Stream: partial send on timeout", .fn = test_stream_partial_send},
        {.name = "Stream: shutdown of write direction", .fn = test_stream_shutdown},
        {.name = "Stream: peer close wakes blocked receive", .fn = test_stream_peer_close_recv},
        {.name = "Stream: peer close wakes blocked send", .fn = test_stream_peer_close_send},
        {.name = "Stream: listener close refuses waiting client", .fn = test_stream_listener_close},
        {.name = "Stream: connect timeout and stale request", .fn = test_stream_connect_timeout},
        {.name = "Stream: concurrent clients", .fn = test_concurrent_clients},
        {.name = "Network memory released", .fn = test_memory_leak},
};

static const bench_entry_t BENCH_TAB[] = {
        {.name = "stream throughput, chunk", .fn = bench_stream_throughput, .param = 1},
        {.name = "stream throughput, chunk", .fn = bench_stream_throughput, .param = 16},
        {.name = "stream throughput, chunk", .fn = bench_stream_throughput, .param = 128},
        {.name = "stream throughput, chunk", .fn = bench_stream_throughput, .param = 1024},
        {.name = "stream throughput, chunk", .fn = bench_stream_throughput, .param = 4096},
        {.name = "stream round trip, msg", .fn = bench_stream_rtt, .param = 1},
        {.name = "stream round trip, msg", .fn = bench_stream_rtt, .param = 64},
        {.name = "stream round trip, msg", .fn = bench_stream_rtt, .param = 1024},
        {.name = "datagram throughput, size", .fn = bench_dgram_throughput, .param = 16},
        {.name = "datagram throughput, size", .fn = bench_dgram_throughput, .param = 128},
        {.name = "datagram throughput, size", .fn = bench_dgram_throughput, .param = 1024},
        {.name = "datagram round trip, size", .fn = bench_dgram_rtt, .param = 16},
        {.name = "datagram round trip, size", .fn = bench_dgram_rtt, .param = 1024},
        {.name = "connect/accept/close rate", .fn = bench_connect_rate, .param = 0},
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(udstest, STACK_DEPTH_MEDIUM);

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
        run_mode_t mode = MODE_ALL;

        global->netname = UDSTEST_NETNAME;
        global->duration = UDSTEST_DURATION;

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

        snprintf(global->prefix, sizeof(global->prefix), "udst%u.", (unsigned)getpid());

        global->txbuf = malloc(IO_BUF_SIZE);
        global->rxbuf = malloc(IO_BUF_SIZE);
        if (!global->txbuf || !global->rxbuf) {
                puts("Not enough free memory.");
                free(global->txbuf);
                free(global->rxbuf);
                return EXIT_FAILURE;
        }

        // check that UDS network exists
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

        global->mem_start = net_mem_usage();

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
        puts("  -n      UDS network name (default: " UDSTEST_NETNAME ")");
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

        printf("\n" VT100_FONT_COLOR_YELLOW "FUNCTIONAL TESTS (network '%s')" VT100_RESET_ATTRIBUTES "\n",
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
               "PERFORMANCE (network '%s', %u ms per measurement)" VT100_RESET_ATTRIBUTES "\n",
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
 * @brief  Create socket address of this program instance.
 *
 * @param  addr         address
 * @param  name         socket name
 */
//==============================================================================
static void mkaddr(NET_UDS_sockaddr_t *addr, const char *name)
{
        memset(addr, 0, sizeof(*addr));
        snprintf(addr->path, sizeof(addr->path), "%s%s", global->prefix, name);
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
                // protects test from hang when operation is not finished (e.g. older UDS)
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
 * @brief  Bind socket to name of this program instance.
 *
 * @param  sock         socket
 * @param  name         socket name
 *
 * @return 0 on success, -1 on error (errno is set).
 */
//==============================================================================
static int sock_bind(SOCKET *sock, const char *name)
{
        NET_UDS_sockaddr_t addr;
        mkaddr(&addr, name);
        return socket_bind(sock, &addr, sizeof(addr));
}

//==============================================================================
/**
 * @brief  Connect socket to name of this program instance.
 *
 * @param  sock         socket
 * @param  name         socket name
 *
 * @return 0 on success, -1 on error (errno is set).
 */
//==============================================================================
static int sock_connect(SOCKET *sock, const char *name)
{
        NET_UDS_sockaddr_t addr;
        mkaddr(&addr, name);
        return socket_connect(sock, &addr, sizeof(addr));
}

//==============================================================================
/**
 * @brief  Create listening stream socket.
 *
 * @param  name         socket name
 *
 * @return Socket or NULL on error (errno is set).
 */
//==============================================================================
static SOCKET *sock_listen(const char *name)
{
        SOCKET *sock = sock_new(NET_SOCKET_TYPE__STREAM);

        if (sock) {
                if ((sock_bind(sock, name) != 0) || (socket_listen(sock) != 0)) {
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
 * @param  name         name of server socket
 * @param  server       listening socket
 * @param  client       client socket
 * @param  accepted     accepted (server side) socket
 *
 * @return 0 on success, otherwise errno value. On error all sockets are closed.
 */
//==============================================================================
static int sock_pair(const char *name, SOCKET **server, SOCKET **client, SOCKET **accepted)
{
        job_t job;
        memset(&job, 0, sizeof(job));

        *client = NULL;
        *accepted = NULL;

        *server = sock_listen(name);
        if (!*server) {
                return errno ? errno : EIO;
        }

        int err = 0;

        *client = sock_new(NET_SOCKET_TYPE__STREAM);
        if (!*client) {
                err = errno ? errno : EIO;
                goto error;
        }

        socket_set_recv_timeout(*server, SAFE_TIMEOUT);
        socket_set_recv_timeout(*client, SAFE_TIMEOUT);

        job.sock = *server;
        if (!job_start(&job, th_accept)) {
                err = ENOMEM;
                goto error;
        }

        if (sock_connect(*client, name) != 0) {
                err = errno ? errno : EIO;
        }

        job_join(&job);

        if (!err && job.err) {
                err = job.err;
        }

        if (!err) {
                *accepted = job.acc;
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
 * @brief  Return memory used by network subsystem.
 *
 * @return Number of bytes.
 */
//==============================================================================
static size_t net_mem_usage(void)
{
        memstat_t stat;

        if (get_memory_usage_details(&stat) == 0) {
                return (size_t)stat.network_memory_usage;
        }

        return 0;
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
        job->elapsed = elapsed_ms(tref);

        return 0;
}

//==============================================================================
/**
 * @brief  Connect socket (job->sock) to job->name.
 *
 * @param  arg          job
 *
 * @return 0
 */
//==============================================================================
static int th_connect(void *arg)
{
        job_t *job = arg;

        clock_t tref = get_uptime_ms();
        job->ret = sock_connect(job->sock, job->name);
        job->err = (job->ret != 0) ? (errno ? errno : EIO) : 0;
        job->elapsed = elapsed_ms(tref);

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
        job->elapsed = elapsed_ms(tref);

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
        job->elapsed = elapsed_ms(tref);

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

                for (;;) {
                        int n = socket_recv(conn, buf, sizeof(buf), NET_FLAGS__NONE);
                        if (n <= 0) {
                                if ((n == 0) || (errno != ECONNRESET)) {
                                        job->err = (n == 0) ? ETIME : (errno ? errno : EIO);
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
 * @brief  Echo client: job->count times connects to job->name, sends pattern
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

                if (sock_connect(sock, job->name) != 0) {
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
        NET_UDS_sockaddr_t from;

        while (!job->stop) {
                memset(&from, 0, sizeof(from));

                int n = socket_recvfrom(job->sock, job->buf, job->len, NET_FLAGS__NONE, &from, sizeof(from));
                if (n == 0) {
                        continue;
                }

                if ((n < 0) ||
                        (socket_sendto(job->sock, job->buf, n, NET_FLAGS__NONE, &from, sizeof(from)) != n)) {
                        job->err = errno ? errno : EIO;
                        break;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Send job->len datagrams to job->name for job->duration ms,
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
        NET_UDS_sockaddr_t to;

        mkaddr(&to, job->name);
        pattern_fill(job->buf, job->len, 0, job->seed);

        job->bytes = 0;
        job->ops = 0;
        clock_t tref = get_uptime_ms();

        while (!job->stop && (elapsed_ms(tref) < job->duration)) {
                int n = socket_sendto(job->sock, job->buf, job->len, NET_FLAGS__NONE, &to, sizeof(to));
                if (n != (int)job->len) {
                        job->err = (n < 0) ? (errno ? errno : EIO) : EIO;
                        break;
                }

                job->bytes += n;
                job->ops++;
        }

        job->elapsed = elapsed_ms(tref);

        socket_sendto(job->sock, job->buf, DGRAM_END_MARK, NET_FLAGS__NONE, &to, sizeof(to));

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

                if ((recv_all(conn, &byte, 1) == 0) && (send_all(conn, &byte, 1) == 0)) {
                        job->ops++;
                }

                sock_close(&conn);
        }

        return 0;
}

/*==============================================================================
  Functional tests
==============================================================================*/

//==============================================================================
/**
 * @brief  Datagram send and receive, sender address and reply.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_basic(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        NET_UDS_sockaddr_t to, from, exp;
        char buf[16];

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, "da") == 0, "socket_bind(a)");
        CHECK(sock_bind(b, "db") == 0, "socket_bind(b)");
        socket_set_recv_timeout(a, SAFE_TIMEOUT);
        socket_set_recv_timeout(b, SAFE_TIMEOUT);

        mkaddr(&to, "da");
        CHECK(socket_sendto(b, "hello", 5, NET_FLAGS__NONE, &to, sizeof(to)) == 5, "socket_sendto()");

        memset(&from, 0, sizeof(from));
        int n = socket_recvfrom(a, buf, sizeof(buf), NET_FLAGS__NONE, &from, sizeof(from));
        CHECK((n == 5) && (memcmp(buf, "hello", 5) == 0), "socket_recvfrom(): %d", n);

        mkaddr(&exp, "db");
        CHECK(isstreq(from.path, exp.path), "wrong sender address '%s', expected '%s'", from.path, exp.path);

        CHECK(socket_sendto(a, "reply", 5, NET_FLAGS__NONE, &from, sizeof(from)) == 5,
                "socket_sendto(reply)");
        n = socket_recv(b, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 5) && (memcmp(buf, "reply", 5) == 0), "socket_recv(reply): %d", n);

        // send to itself
        mkaddr(&to, "db");
        CHECK(socket_sendto(b, "self", 4, NET_FLAGS__NONE, &to, sizeof(to)) == 4, "socket_sendto(self)");
        n = socket_recv(b, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 4) && (memcmp(buf, "self", 4) == 0), "socket_recv(self): %d", n);

cleanup:
        sock_close(&a);
        sock_close(&b);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram error handling.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_errors(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        SOCKET *x = NULL;
        u8_t *big = NULL;
        NET_UDS_sockaddr_t addr;

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");

        CHECK(sock_bind(a, "de") == 0, "socket_bind()");
        EXPECT_ERR(sock_bind(b, "de"), EADDRINUSE, "bind to used address");

        memset(&addr, 0, sizeof(addr));
        EXPECT_ERR(socket_bind(b, &addr, sizeof(addr)), EINVAL, "bind to empty path");

        memset(&addr, 'x', sizeof(addr)); // not terminated path
        EXPECT_ERR(socket_bind(b, &addr, sizeof(addr)), EINVAL, "bind to not terminated path");
        EXPECT_ERR(socket_sendto(b, "x", 1, NET_FLAGS__NONE, &addr, sizeof(addr)), EINVAL,
                "sendto not terminated path");

        mkaddr(&addr, "nonexistent");
        EXPECT_ERR(socket_sendto(b, "x", 1, NET_FLAGS__NONE, &addr, sizeof(addr)), ENOENT,
                "sendto nonexistent socket");

        // datagram longer than maximum datagram size of every configuration
        size_t big_size = 16 * 1024;
        big = malloc(big_size);
        if (big) {
                mkaddr(&addr, "de");
                EXPECT_ERR(socket_sendto(b, big, big_size, NET_FLAGS__NONE, &addr, sizeof(addr)), EINVAL,
                        "sendto too big datagram");
        }

        // receive on not bound socket, stream operation on datagram socket
        char c;
        EXPECT_ERR(socket_recv(b, &c, 1, NET_FLAGS__NONE), ECONNREFUSED, "recv on not bound socket");
        EXPECT_ERR((socket_accept(a, &x) == 0) ? 0 : -1, EINVAL, "accept on datagram socket");

cleanup:
        sock_close(&x);
        free(big);
        sock_close(&a);
        sock_close(&b);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram message boundaries, order and truncation.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_boundaries(void)
{
        static const u16_t SIZE[] = {1, 2, 15, 16, 100, 255, 256, 511, 1000, 1024};

        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        NET_UDS_sockaddr_t to;

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, "dr") == 0, "socket_bind()");
        socket_set_recv_timeout(a, SAFE_TIMEOUT);
        mkaddr(&to, "dr");

        for (size_t round = 0; round < 3; round++) {
                // send several datagrams, receiver does not read in the meantime
                for (size_t i = 0; i < ARRAY_SIZE(SIZE); i++) {
                        pattern_fill(global->txbuf, SIZE[i], 0, (u32_t)(i + round));
                        int n = socket_sendto(b, global->txbuf, SIZE[i], NET_FLAGS__NONE, &to, sizeof(to));
                        CHECK(n == SIZE[i], "socket_sendto(%u B): %d", (unsigned)SIZE[i], n);
                }

                for (size_t i = 0; i < ARRAY_SIZE(SIZE); i++) {
                        // last round: buffer smaller than datagram (rest of datagram is dropped)
                        size_t len = (round == 2) ? max(1, SIZE[i] / 2) : IO_BUF_SIZE;
                        size_t exp = min(len, SIZE[i]);

                        int n = socket_recv(a, global->rxbuf, len, NET_FLAGS__NONE);
                        CHECK(n == (int)exp, "datagram %u: received %d B, expected %u B", (unsigned)i, n,
                                (unsigned)exp);
                        CHECK(pattern_check(global->rxbuf, exp, 0, (u32_t)(i + round)) == exp,
                                "datagram %u: wrong content", (unsigned)i);
                }
        }

cleanup:
        sock_close(&a);
        sock_close(&b);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Datagram receive timeout and send timeout of full queue.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dgram_timeout(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *a = NULL;
        SOCKET *b = NULL;
        NET_UDS_sockaddr_t to;
        char c;

        CHECK((a = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK((b = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        CHECK(sock_bind(a, "dt") == 0, "socket_bind()");

        CHECK(socket_set_recv_timeout(a, 200) == 0, "socket_set_recv_timeout()");
        clock_t tref = get_uptime_ms();
        int n = socket_recv(a, &c, 1, NET_FLAGS__NONE);
        u32_t dt = elapsed_ms(tref);
        CHECK(n == 0, "socket_recv(): %d, expected 0 (timeout)", n);
        CHECK((dt >= 180) && (dt < 200 + UDSTEST_WAKEUP_LIMIT), "receive timeout: %u ms, expected 200 ms",
                (unsigned)dt);

        // fill receive queue of a
        mkaddr(&to, "dt");
        CHECK(socket_set_send_timeout(b, 100) == 0, "socket_set_send_timeout()");
        size_t queued = 0;
        int err = 0;
        for (; queued < 1000; queued++) {
                n = socket_sendto(b, "q", 1, NET_FLAGS__NONE, &to, sizeof(to));
                if (n != 1) {
                        err = errno;
                        break;
                }
        }
        CHECK((queued > 0) && (queued < 1000) && (n < 0) && (err == ETIME),
                "send to full queue: queued %u, result %d (%s)", (unsigned)queued, n, strerror(err));

        for (size_t i = 0; i < queued; i++) {
                CHECK(socket_recv(a, &c, 1, NET_FLAGS__NONE) == 1, "socket_recv(%u)", (unsigned)i);
        }

cleanup:
        sock_close(&a);
        sock_close(&b);
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
        NET_UDS_sockaddr_t addr, exp;
        char buf[16];

        int err = sock_pair("sb", &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(send_all(cli, "abcdef", 6) == 0, "send(client)");
        int n = socket_recv(acc, buf, 4, NET_FLAGS__NONE);
        CHECK((n == 4) && (memcmp(buf, "abcd", 4) == 0), "recv(4): %d", n);
        n = socket_recv(acc, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 2) && (memcmp(buf, "ef", 2) == 0), "recv(rest): %d", n);

        CHECK(send_all(acc, "123", 3) == 0, "send(server)");
        CHECK(recv_all(cli, buf, 3) == 0 && (memcmp(buf, "123", 3) == 0), "recv(client)");

        // receive returns available data, does not wait for full buffer
        CHECK(send_all(cli, "xy", 2) == 0, "send(client)");
        clock_t tref = get_uptime_ms();
        n = socket_recv(acc, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK((n == 2) && (elapsed_ms(tref) < UDSTEST_WAKEUP_LIMIT), "recv(available): %d", n);

        // connected sockets return address of peer: client is connected to accepted
        // socket and the client is not bound, so both addresses are empty
        memset(&addr, 'x', sizeof(addr));
        CHECK(socket_get_address(cli, &addr, sizeof(addr)) == 0, "socket_get_address(client)");
        CHECK(addr.path[0] == '\0', "client peer address '%s'", addr.path);
        memset(&addr, 'x', sizeof(addr));
        CHECK(socket_get_address(acc, &addr, sizeof(addr)) == 0, "socket_get_address(accepted)");
        CHECK(addr.path[0] == '\0', "accepted peer address '%s'", addr.path);

        // not connected socket returns own address
        CHECK(socket_get_address(srv, &addr, sizeof(addr)) == 0, "socket_get_address(server)");
        mkaddr(&exp, "sb");
        CHECK(isstreq(addr.path, exp.path), "server address '%s', expected '%s'", addr.path, exp.path);

        EXPECT_ERR(sock_connect(cli, "sb"), EISCONN, "connect of connected socket");

cleanup:
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Stream connection errors.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_conn_errors(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *nl = NULL;
        SOCKET *dg = NULL;
        char c;

        CHECK((cli = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");
        socket_set_recv_timeout(cli, 1000);

        EXPECT_ERR(sock_connect(cli, "nonexistent"), ENOENT, "connect to nonexistent socket");

        CHECK((nl = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");
        CHECK(sock_bind(nl, "snl") == 0, "socket_bind()");
        EXPECT_ERR(sock_connect(cli, "snl"), ENOENT, "connect to not listening socket");

        CHECK((srv = sock_listen("sce")) != NULL, "listen");
        CHECK((dg = sock_new(NET_SOCKET_TYPE__DATAGRAM)) != NULL, "socket_open()");
        EXPECT_ERR(sock_connect(dg, "sce"), ECONNREFUSED, "connect datagram to stream socket");

        EXPECT_ERR(socket_listen(cli), EINVAL, "listen on not bound socket");
        EXPECT_ERR(socket_recv(cli, &c, 1, NET_FLAGS__NONE), ECONNREFUSED, "recv on not connected socket");
        EXPECT_ERR(socket_send(cli, &c, 1, NET_FLAGS__NONE), ECONNREFUSED, "send on not connected socket");

cleanup:
        sock_close(&dg);
        sock_close(&nl);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Two connections transfer data at the same time in opposite
 *         directions with different send and receive chunks. Each socket is
 *         used by one thread only (network layer serializes operations of
 *         single socket).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_bulk(void)
{
        static const char *const JOB_NAME[] = {"1: client send", "1: server recv", "2: server send",
                "2: client recv"};

        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv[2] = {NULL, NULL};
        SOCKET *cli[2] = {NULL, NULL};
        SOCKET *acc[2] = {NULL, NULL};
        job_t job[4];
        size_t started = 0;

        memset(job, 0, sizeof(job));

        int err = sock_pair("sbk1", &srv[0], &cli[0], &acc[0]);
        CHECK(err == 0, "connection 1: %s", strerror(err));
        err = sock_pair("sbk2", &srv[1], &cli[1], &acc[1]);
        CHECK(err == 0, "connection 2: %s", strerror(err));

        // connection 1: client -> server, sent by 7 B, received by up to 1000 B
        job[0].sock = cli[0];
        job[0].buf = global->txbuf;
        job[0].len = 7;
        job[1].sock = acc[0];
        job[1].buf = global->rxbuf;
        job[1].len = 1000;

        // connection 2: server -> client, sent by 1000 B, received by up to 61 B
        job[2].sock = acc[1];
        job[2].buf = global->txbuf + 2048;
        job[2].len = 1000;
        job[3].sock = cli[1];
        job[3].buf = global->rxbuf + 2048;
        job[3].len = 61;

        static const thread_func_t FUNC[4] = {th_bulk_send, th_bulk_recv, th_bulk_send, th_bulk_recv};

        for (; started < 4; started++) {
                job[started].count = UDSTEST_BULK_SIZE;
                job[started].seed = (started < 2) ? 1 : 2;

                if (!job_start(&job[started], FUNC[started])) {
                        break;
                }
        }

        for (size_t i = 0; i < started; i++) {
                job_join(&job[i]);
        }

        CHECK(started == 4, "unable to start thread");

        for (size_t i = 0; i < 4; i++) {
                CHECK(job[i].err == 0, "%s: %s after %u B", JOB_NAME[i],
                        (job[i].err == EILSEQ) ? "wrong data" : strerror(job[i].err), (unsigned)job[i].bytes);
                CHECK(job[i].bytes == UDSTEST_BULK_SIZE, "%s: %u B", JOB_NAME[i], (unsigned)job[i].bytes);
        }

cleanup:
        for (size_t i = 0; i < 2; i++) {
                sock_close(&acc[i]);
                sock_close(&cli[i]);
                sock_close(&srv[i]);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Send returns number of queued bytes when timeout expires.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_partial_send(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        size_t total = 0;

        int err = sock_pair("sps", &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(socket_set_send_timeout(cli, 100) == 0, "socket_set_send_timeout()");

        // peer does not read: send until buffer is full, last send is partial
        // (when buffer size is not multiple of IO_BUF_SIZE) or fails with ETIME
        pattern_fill(global->txbuf, IO_BUF_SIZE, 0, 3);
        int n;
        int err2 = 0;
        do {
                n = socket_send(cli, global->txbuf, IO_BUF_SIZE, NET_FLAGS__NONE);
                if (n > 0) {
                        total += n;
                } else {
                        err2 = errno;
                }
        } while ((n == IO_BUF_SIZE) && (total < 1024 * 1024));

        CHECK((total > 0) && (total < 1024 * 1024), "send to not reading peer: total %u B", (unsigned)total);
        CHECK((n > 0) || (err2 == ETIME), "last send: %d (%s)", n, strerror(err2));

        errno = 0;
        n = socket_send(cli, global->txbuf, 1, NET_FLAGS__NONE);
        err2 = errno;
        CHECK((n < 0) && (err2 == ETIME), "send to full buffer: %d (%s), expected ETIME", n, strerror(err2));

        // everything that was reported as sent must be received
        socket_set_recv_timeout(acc, 200);
        size_t recved = 0;
        while (recved < total) {
                n = socket_recv(acc, global->rxbuf, IO_BUF_SIZE, NET_FLAGS__NONE);
                if (n <= 0) {
                        break;
                }

                for (int i = 0; i < n; i++) {
                        CHECK(global->rxbuf[i] == global->txbuf[(recved + i) % IO_BUF_SIZE],
                                "wrong data at %u", (unsigned)(recved + i));
                }

                recved += n;
        }
        CHECK(recved == total, "received %u B, sent %u B", (unsigned)recved, (unsigned)total);

cleanup:
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Shutdown of write direction: peer receives remaining data and then
 *         ECONNRESET; other direction still works.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_shutdown(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        char buf[16];

        int err = sock_pair("ssd", &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(send_all(cli, "data", 4) == 0, "send()");
        CHECK(socket_shutdown(cli, NET_SHUT__WR) == 0, "socket_shutdown(WR)");
        EXPECT_ERR(socket_send(cli, "x", 1, NET_FLAGS__NONE), EPERM, "send after shutdown");

        int n = socket_recv(acc, buf, sizeof(buf), NET_FLAGS__NONE);
        CHECK(n == 4, "recv(remaining data): %d", n);
        clock_t tref = get_uptime_ms();
        EXPECT_ERR(socket_recv(acc, buf, sizeof(buf), NET_FLAGS__NONE), ECONNRESET,
                "recv after peer shutdown");
        CHECK(elapsed_ms(tref) < UDSTEST_WAKEUP_LIMIT, "end of data detected too late");

        CHECK(send_all(acc, "back", 4) == 0, "send(back)");
        CHECK(recv_all(cli, buf, 4) == 0 && (memcmp(buf, "back", 4) == 0), "recv(back)");

        CHECK(socket_shutdown(acc, NET_SHUT__RD) == 0, "socket_shutdown(RD)");
        EXPECT_ERR(socket_recv(acc, buf, 1, NET_FLAGS__NONE), EPERM, "recv after shutdown");

cleanup:
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Peer close wakes thread blocked in receive (without timeout).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_peer_close_recv(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;
        char buf[8];

        memset(&job, 0, sizeof(job));

        int err = sock_pair("spr", &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        CHECK(socket_set_recv_timeout(cli, SAFE_TIMEOUT * 2) == 0, "socket_set_recv_timeout()");
        CHECK(send_all(acc, "last", 4) == 0, "send()");

        // first remaining data, then blocking receive interrupted by close
        CHECK(recv_all(cli, buf, 4) == 0, "recv(data)");

        job.sock = cli;
        job.buf = (u8_t *)buf;
        job.len = sizeof(buf);
        CHECK(job_start(&job, th_recv), "unable to start thread");
        msleep(100);

        clock_t tref = get_uptime_ms();
        sock_close(&acc);
        job_join(&job);
        u32_t dt = elapsed_ms(tref);

        CHECK((job.ret < 0) && (job.err == ECONNRESET), "blocked recv: %d (%s), expected ECONNRESET", job.ret,
                strerror(job.err));
        CHECK(dt < UDSTEST_WAKEUP_LIMIT, "blocked recv woken after %u ms", (unsigned)dt);
        EXPECT_ERR(socket_send(cli, "x", 1, NET_FLAGS__NONE), ECONNRESET, "send to closed peer");

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Peer close wakes thread blocked in send (buffer full).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_peer_close_send(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        SOCKET *acc = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        int err = sock_pair("sps2", &srv, &cli, &acc);
        CHECK(err == 0, "connection: %s", strerror(err));

        // fill peer buffer
        CHECK(socket_set_send_timeout(cli, 100) == 0, "socket_set_send_timeout()");
        while (socket_send(cli, global->txbuf, IO_BUF_SIZE, NET_FLAGS__NONE) > 0) {
        }

        CHECK(socket_set_send_timeout(cli, SAFE_TIMEOUT * 2) == 0, "socket_set_send_timeout()");
        job.sock = cli;
        job.buf = global->txbuf;
        job.len = 16;
        CHECK(job_start(&job, th_send), "unable to start thread");
        msleep(100);

        clock_t tref = get_uptime_ms();
        sock_close(&acc);
        job_join(&job);
        u32_t dt = elapsed_ms(tref);

        CHECK((job.ret < 0) && (job.err == ECONNRESET), "blocked send: %d (%s), expected ECONNRESET", job.ret,
                strerror(job.err));
        CHECK(dt < UDSTEST_WAKEUP_LIMIT, "blocked send woken after %u ms", (unsigned)dt);

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Close of listening socket refuses client waiting for accept.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_listener_close(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *cli = NULL;
        job_t job;

        memset(&job, 0, sizeof(job));

        CHECK((srv = sock_listen("slc")) != NULL, "listen");
        CHECK((cli = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");
        CHECK(socket_set_recv_timeout(cli, SAFE_TIMEOUT * 2) == 0, "socket_set_recv_timeout()");

        job.sock = cli;
        job.name = "slc";
        CHECK(job_start(&job, th_connect), "unable to start thread");
        msleep(100);

        clock_t tref = get_uptime_ms();
        sock_close(&srv);
        job_join(&job);
        u32_t dt = elapsed_ms(tref);

        CHECK((job.ret != 0) && (job.err == ECONNREFUSED), "waiting connect: %d (%s), expected ECONNREFUSED",
                job.ret, strerror(job.err));
        CHECK(dt < UDSTEST_WAKEUP_LIMIT, "waiting connect woken after %u ms", (unsigned)dt);

cleanup:
        job_join(&job);
        sock_close(&cli);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Connect timeout. Accept ignores requests of clients that gave up
 *         or were closed and accepts next valid client.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_stream_connect_timeout(void)
{
        const char *exit_msg = SUCCESS_MSG;
        SOCKET *srv = NULL;
        SOCKET *c1 = NULL;
        SOCKET *c2 = NULL;
        SOCKET *c3 = NULL;
        SOCKET *acc = NULL;
        job_t job;
        char buf[4];

        memset(&job, 0, sizeof(job));

        CHECK((srv = sock_listen("sct")) != NULL, "listen");
        CHECK((c1 = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");
        CHECK((c2 = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");
        CHECK((c3 = sock_new(NET_SOCKET_TYPE__STREAM)) != NULL, "socket_open()");

        socket_set_recv_timeout(c1, 200);
        clock_t tref = get_uptime_ms();
        EXPECT_ERR(sock_connect(c1, "sct"), ETIME, "connect without accept");
        u32_t dt = elapsed_ms(tref);
        CHECK((dt >= 180) && (dt < 200 + UDSTEST_WAKEUP_LIMIT), "connect timeout: %u ms, expected 200 ms",
                (unsigned)dt);

        socket_set_recv_timeout(c2, 100);
        EXPECT_ERR(sock_connect(c2, "sct"), ETIME, "connect without accept");
        sock_close(&c2); // closed client remains in accept queue

        socket_set_recv_timeout(c3, SAFE_TIMEOUT);
        job.sock = c3;
        job.name = "sct";
        CHECK(job_start(&job, th_connect), "unable to start thread");
        msleep(50);

        socket_set_recv_timeout(srv, SAFE_TIMEOUT);
        CHECK(socket_accept(srv, &acc) == 0, "socket_accept()");
        job_join(&job);
        CHECK(job.err == 0, "connect: %s", strerror(job.err));

        // accepted connection belongs to c3
        socket_set_recv_timeout(c3, SAFE_TIMEOUT);
        CHECK(send_all(acc, "c3", 2) == 0, "send()");
        CHECK(recv_all(c3, buf, 2) == 0 && (memcmp(buf, "c3", 2) == 0), "connection of wrong client");

        // no more pending requests
        SOCKET *x = NULL;
        socket_set_recv_timeout(srv, 100);
        CHECK(socket_accept(srv, &x) != 0, "accepted request of client that gave up");
        sock_close(&x);

cleanup:
        job_join(&job);
        sock_close(&acc);
        sock_close(&c1);
        sock_close(&c2);
        sock_close(&c3);
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
        job_t client[UDSTEST_CLIENTS];
        size_t started = 0;

        memset(&server, 0, sizeof(server));
        memset(client, 0, sizeof(client));

        CHECK((srv = sock_listen("scc")) != NULL, "listen");
        socket_set_recv_timeout(srv, SAFE_TIMEOUT);

        server.sock = srv;
        server.count = UDSTEST_CLIENTS * UDSTEST_CLIENT_ROUNDS;
        CHECK(job_start(&server, th_echo_server), "unable to start server thread");

        for (; started < UDSTEST_CLIENTS; started++) {
                client[started].name = "scc";
                client[started].count = UDSTEST_CLIENT_ROUNDS;
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

        CHECK(started == UDSTEST_CLIENTS, "unable to start client thread");

        for (size_t i = 0; i < UDSTEST_CLIENTS; i++) {
                CHECK(client[i].err == 0, "client %u, round %u: %s", (unsigned)i, (unsigned)client[i].ops,
                        (client[i].err == EILSEQ) ? "wrong echo" : strerror(client[i].err));
        }

        CHECK(server.err == 0, "server: %s", strerror(server.err));
        CHECK(server.ops == UDSTEST_CLIENTS * UDSTEST_CLIENT_ROUNDS, "server handled %u connections",
                (unsigned)server.ops);

cleanup:
        job_join(&server);
        sock_close(&srv);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  All network memory allocated by tests is released.
 *
 * @note   Memory of other network stacks (e.g. TCP/IP) is included, so
 *         activity of other programs can cause false result.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_memory_leak(void)
{
        const char *exit_msg = SUCCESS_MSG;

        size_t usage = net_mem_usage();

        if ((usage == 0) && (global->mem_start == 0)) {
                return SKIP("memory statistics not available");
        }

        for (int i = 0; (i < 5) && (usage > global->mem_start); i++) {
                msleep(200); // e.g. detached threads finish
                usage = net_mem_usage();
        }

        CHECK(usage <= global->mem_start, "network memory: %u B at start, %u B now (+%u B)",
                (unsigned)global->mem_start, (unsigned)usage, (unsigned)(usage - global->mem_start));

cleanup:
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

        int err = sock_pair("pst", &srv, &cli, &acc);
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

        int err = sock_pair("prt", &srv, &cli, &acc);
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
        NET_UDS_sockaddr_t from;
        job_t job;

        memset(&job, 0, sizeof(job));

        rx = sock_new(NET_SOCKET_TYPE__DATAGRAM);
        tx = sock_new(NET_SOCKET_TYPE__DATAGRAM);

        if (!rx || !tx || (sock_bind(rx, "pdr") != 0)) {
                res_printf("socket error: %s", strerror(errno));
                goto cleanup;
        }

        socket_set_recv_timeout(rx, BENCH_TIMEOUT);

        job.sock = tx;
        job.name = "pdr";
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
                // recvfrom() is used, older UDS supports recv() on connected sockets only
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

        if (job.err && (count == 0)) {
                res_printf("not supported (%s)", strerror(job.err));
        } else {
                res_printf("%9.1f KiB/s (%u datagrams/s)", (double)bytes * 1000.0 / 1024.0 / dt,
                        (unsigned)(count * 1000 / dt));

                if (bad || (count != job.ops)) {
                        res_printf(VT100_FONT_COLOR_RED
                                " DATA ERROR (sent %u, received %u, wrong %u)" VT100_RESET_ATTRIBUTES,
                                (unsigned)job.ops, (unsigned)count, (unsigned)bad);
                }
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
        NET_UDS_sockaddr_t to, from;
        job_t job;

        memset(&job, 0, sizeof(job));

        srv = sock_new(NET_SOCKET_TYPE__DATAGRAM);
        cli = sock_new(NET_SOCKET_TYPE__DATAGRAM);

        if (!srv || !cli || (sock_bind(srv, "pes") != 0) || (sock_bind(cli, "pec") != 0)) {
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

        mkaddr(&to, "pes");
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
                        err = (n < 0) ? errno : ETIME;
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
        job_t job;

        memset(&job, 0, sizeof(job));

        srv = sock_listen("pcr");
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

                if (sock_connect(sock, "pcr") != 0) {
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
