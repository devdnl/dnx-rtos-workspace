/*==============================================================================
File    perf.c

Author  Daniel Zorychta

Brief   Performance measurement and stress test program.

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
  Configuration
==============================================================================*/
/** Default duration of single performance measurement [ms]. */
#ifndef PERF_BENCH_DURATION
#define PERF_BENCH_DURATION 1000
#endif

/** Default duration of single stress test [ms]. */
#ifndef PERF_STRESS_DURATION
#define PERF_STRESS_DURATION 3000
#endif

/** Allowed delay of wake-up after sleep or timeout [ms]. */
#ifndef PERF_TIMING_TOLERANCE
#define PERF_TIMING_TOLERANCE 20
#endif

#define SAFE_TIMEOUT      5000  /* timeout that protects test from hang [ms] */
#define SETTLE_TIME       2000  /* time of resource release after test [ms] */
#define CANCEL_TIMEOUT    2000  /* max time of thread termination after cancel [ms] */
#define CHILD_LIFETIME    30000 /* max lifetime of child process [ms] */
#define MAX_WORKERS       8     /* max number of worker threads of single test */
#define MAX_THREADS_PROBE 64    /* max number of threads in slot exhaustion test */
#define HEAP_SLOTS        32    /* number of blocks of single heap worker */
#define HEAP_BUDGET_MAX   (48 * 1024)
#define HEAP_BLOCK_MAX    4096
#define CHILD_HEAP_BUDGET (4 * 1024)
#define SYNC_SLOTS        12          /* number of objects of single sync worker */
#define RING_SIZE         8           /* ring buffer of producer/consumer test */
#define QUEUE_LEN         8           /* queue length of queue tests */
#define NESTED_DEPTH      3           /* depth of nested thread creation tree */
#define THREAD_BURST      8           /* threads created without pause */
#define THREAD_MEM_LIMIT  (32 * 1024) /* max memory of finished threads not released yet */
#define CMD_LEN           64          /* child process command line */
#define CMDLINE_LEN       48          /* command line read from process statistics */

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/misc.h>
#include <dnx/vt100.h>
#include <sys/ioctl.h>
#include <sha256.h>

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

/** Check failure reported by worker threads. */
#define CHECK_FAIL(_fail)                           \
        do {                                        \
                if ((_fail)->set) {                 \
                        exit_msg = fail_msg(_fail); \
                        goto cleanup;               \
                }                                   \
        } while (0)

/** Compiler barrier (prevents removing of measured operations). */
#define BARRIER() __asm__ volatile("" ::: "memory")

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        MODE_ALL,
        MODE_BENCH,
        MODE_STRESS,
        MODE_LIST,
} run_mode_t;

typedef struct {
        const char *name;
        const char *(*fn)(void);
        bool heavy; //!< test exhausts system resources or abuses kernel (option -x)
} test_entry_t;

typedef struct {
        const char *name;
        void (*fn)(size_t param);
        size_t param;
        const char *unit; //!< unit of param (NULL: param not printed)
} bench_entry_t;

/** The first failure detected by worker threads. */
typedef struct {
        futex_t lock;
        volatile bool set;
        const char *what;
        u32_t a;
        u32_t b;
} fail_t;

/** Worker thread of stress test. */
typedef struct {
        tid_t tid;
        u8_t id;
        i16_t priority;
        u32_t seed;
        clock_t deadline;    //!< worker finishes work after deadline
        volatile bool *stop; //!< optional stop request
        fail_t *fail;        //!< failure container
        void *ctx;           //!< test context
        void *obj;           //!< worker's object
        u32_t value;         //!< generic input value
        volatile u32_t res;  //!< generic result
        volatile u32_t ops;  //!< number of operations
        u32_t sum;           //!< checksum of processed data
} worker_t;

/** Resource usage of this process. */
typedef struct {
        size_t mem;
        u16_t blocks;
        u16_t threads;
        u16_t mutexes;
        u16_t semaphores;
        u16_t queues;
        u16_t files;
} snapshot_t;

/** CPU score job. */
typedef struct {
        u8_t buf[256];
        u8_t hash[SHA256_BLOCK_SIZE];
        tid_t tid;
        sem_t *start;
        unsigned int seed;
        u32_t duration;
        u32_t points;
        u32_t elapsed;
} cpu_job_t;

/** Heap block of heap worker. */
typedef struct {
        u8_t *ptr;
        u32_t size;
        u32_t seed;
        u16_t align;
} hslot_t;

/** Heap worker context. */
typedef struct {
        hslot_t slot[HEAP_SLOTS];
        fail_t *fail;
        size_t budget; //!< max size of allocated blocks
        size_t live;   //!< size of allocated blocks
        u32_t seed;
        u32_t ops;
        u32_t allocs;
        u32_t nomem;
        bool check; //!< check heap consistency
} heap_ctx_t;

/** Message of cross-thread heap test. */
typedef struct {
        u8_t *ptr;
        u32_t size;
        u32_t seed;
} heap_msg_t;

/** Synchronization object of sync worker. */
typedef enum {
        SOBJ_NONE,
        SOBJ_MUTEX,
        SOBJ_MUTEX_REC,
        SOBJ_SEMAPHORE,
        SOBJ_QUEUE,
} sobj_type_t;

typedef struct {
        sobj_type_t type;
        void *obj;
        u16_t p1; //!< semaphore: max count, queue: length
        u16_t p2; //!< semaphore: value, queue: item size
} sobj_t;

typedef struct {
        sobj_t slot[SYNC_SLOTS];
        fail_t *fail;
        u32_t seed;
        u32_t ops;
        u32_t created;
        u32_t nomem;
} sync_ctx_t;

/** Lock type of exclusion test. */
typedef enum {
        LOCK_MUTEX,
        LOCK_MUTEX_REC,
        LOCK_FUTEX,
} lock_type_t;

typedef struct {
        lock_type_t type;
        mutex_t *mtx;
        futex_t futex;
        volatile u32_t counter;
        volatile int owner;
} excl_ctx_t;

/** Victim kind of cancel test. */
typedef enum {
        VICTIM_SEMAPHORE,
        VICTIM_QUEUE,
        VICTIM_MUTEX,
        VICTIM_SLEEP,
        VICTIM_BUSY,
        VICTIM_SYSCALL,
        VICTIM_COUNT,
} victim_t;

typedef struct {
        sem_t *sem;
        queue_t *queue;
        mutex_t *mutex;
        volatile bool stop;
} cancel_ctx_t;

/** Node of nested thread creation tree. */
typedef struct {
        u32_t depth;
        u32_t value;
        u32_t result;
        u32_t threads;
        u32_t fallbacks;
        u32_t errors;
} node_t;

/** Producer/consumer test context. */
typedef struct {
        u32_t ring[RING_SIZE];
        u32_t head;
        u32_t tail;
        sem_t *empty;
        sem_t *full;
        mutex_t *mtx;
        queue_t *queue;
        volatile bool done;
} pc_ctx_t;

/** Item of queue test. */
typedef struct {
        u8_t producer;
        u8_t pad;
        u16_t check;
        u32_t seq;
} qitem_t;

/** Ping-pong measurement context. */
typedef struct {
        sem_t *ping;
        sem_t *pong;
        volatile bool stop;
} pingpong_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_usage(const char *name);
static bool selected(const char *name, const char *filter);
static size_t run_tests(const char *filter);
static size_t run_benchmarks(const char *filter);
static const char *make_msg(int err, const char *fmt, ...);
static void res_printf(const char *fmt, ...);
static void res_speed(u64_t bytes, u32_t dt);
static void res_rate(u32_t ops, u32_t dt, const char *what);
static void info_printf(const char *fmt, ...);
static u32_t elapsed_ms(clock_t tref);
static bool time_ok(u32_t dt, u32_t timeout);
static u32_t get_cpu_freq(void);
static u8_t get_cpu_cores(void);
static u32_t new_seed(void);
static u32_t rnd(u32_t *seed);
static u32_t rnd_range(u32_t *seed, u32_t lo, u32_t hi);
static i16_t rnd_priority(u32_t *seed, i16_t lo, i16_t hi);
static void shuffle(u8_t *tab, size_t n, u32_t *seed);
static u32_t compute(u32_t value);
static void pattern_fill(u8_t *buf, size_t len, u32_t seed);
static size_t pattern_check(const u8_t *buf, size_t len, u32_t seed);
static void fail_init(fail_t *fail);
static bool fail_set(fail_t *fail, const char *what, u32_t a, u32_t b);
static const char *fail_msg(const fail_t *fail);
static bool snapshot_take(snapshot_t *snap);
static const char *snapshot_check(const snapshot_t *ref);
static bool sys_mem_get(memstat_t *stat);
static const char *sys_mem_check(const memstat_t *ref);
static size_t count_children(void);
static void worker_init(worker_t *w, u8_t id, fail_t *fail, void *ctx, u32_t duration);
static bool worker_start(worker_t *w, thread_func_t func, i16_t priority, size_t stack_depth);
static bool worker_running(const worker_t *w);
static void worker_pace(worker_t *w);
static bool workers_join(worker_t *w, size_t count, u32_t timeout);
static void thread_join_or_cancel(tid_t tid, u32_t timeout);
static size_t heap_budget(size_t workers);
static heap_ctx_t *heap_ctx_new(fail_t *fail, size_t budget, bool check);
static u32_t heap_block_size(heap_ctx_t *h);
static bool heap_step(heap_ctx_t *h);
static bool heap_release(heap_ctx_t *h);
static sync_ctx_t *sync_ctx_new(fail_t *fail);
static void sobj_delete(sobj_t *o);
static bool sync_step(sync_ctx_t *s);
static void sync_release(sync_ctx_t *s);
static bool lock_acquire(excl_ctx_t *ctx);
static void lock_release(excl_ctx_t *ctx);
static bool mutex_probe(worker_t *probe, mutex_t *mtx, u32_t timeout);
static void node_run(node_t *node);
static u32_t node_expected(u32_t depth, u32_t value);
static pid_t spawn_child(const char *args);
static int child_main(int argc, char *argv[]);
static int child_work(void);

static int th_cpu_score(void *arg);
static int th_noop(void *arg);
static int th_compute(void *arg);
static int th_pong(void *arg);
static int th_queue_sink(void *arg);
static int th_wait_sem(void *arg);
static int th_node(void *arg);
static int th_victim(void *arg);
static int th_heap_worker(void *arg);
static int th_heap_producer(void *arg);
static int th_heap_consumer(void *arg);
static int th_mutex_probe(void *arg);
static int th_mutex_holder(void *arg);
static int th_mutex_unlock(void *arg);
static int th_exclusion(void *arg);
static int th_sem_producer(void *arg);
static int th_sem_consumer(void *arg);
static int th_queue_producer(void *arg);
static int th_queue_consumer(void *arg);
static int th_sync_worker(void *arg);
static int th_busy(void *arg);
static int th_sleeper(void *arg);

static void bench_cpu_score(size_t all_cores);
static void bench_int(size_t unused);
static void bench_float(size_t dbl);
static void bench_memcpy(size_t size);
static void bench_memset(size_t size);
static void bench_malloc(size_t size);
static void bench_syscall(size_t unused);
static void bench_thread(size_t unused);
static void bench_ctx_switch(size_t unused);
static void bench_mutex(size_t unused);
static void bench_queue(size_t unused);
static void bench_process(size_t unused);
static float measure_tick_us(void);

static const char *test_thread_create_join(void);
static const char *test_thread_memory(void);
static const char *test_thread_exhaustion(void);
static const char *test_thread_nested(void);
static const char *test_thread_cancel(void);
static const char *test_heap_single(void);
static const char *test_heap_concurrent(void);
static const char *test_heap_cross(void);
static const char *test_heap_exhaustion(void);
static const char *test_mutex_functional(void);
static const char *exclusion_test(lock_type_t type);
static const char *test_mutex_exclusion(void);
static const char *test_mutex_rec_exclusion(void);
static const char *test_futex_exclusion(void);
static const char *test_mutex_non_owner(void);
static const char *test_semaphore_pc(void);
static const char *test_queue_mpmc(void);
static const char *test_sync_storm(void);
static const char *test_timing(void);
static const char *test_process_spawn(void);
static const char *test_process_kill(void);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t passed;
        size_t failed;
        size_t skipped;
        bool verbose;
        bool heavy;
        u32_t bench_duration;
        u32_t stress_duration;
        const char *progname;
        u32_t cpu_freq;
        u32_t seed_ctr;
        float cpu_single_pt;
        volatile u32_t sink;
        char msg_buf[200];
        char res_buf[120];  //!< result of performance measurement
        char info_buf[120]; //!< additional information of test (verbose mode)
};

static const char *const SUCCESS_MSG = "SUCCESS";

static const char *const VICTIM_NAME[VICTIM_COUNT] = {
        [VICTIM_SEMAPHORE] = "semaphore wait",
        [VICTIM_QUEUE] = "queue receive",
        [VICTIM_MUTEX] = "mutex lock",
        [VICTIM_SLEEP] = "sleep",
        [VICTIM_BUSY] = "busy loop",
        [VICTIM_SYSCALL] = "syscall loop",
};

/** Priorities of workers of concurrent tests (index: worker ID). */
static const i16_t WORKER_PRIORITY[MAX_WORKERS] = {
        PRIORITY_LOWEST,
        PRIORITY_NORMAL,
        PRIORITY_NORMAL,
        PRIORITY_NORMAL + 1,
        PRIORITY_NORMAL - 1,
        PRIORITY_NORMAL,
        PRIORITY_NORMAL + 2,
        PRIORITY_NORMAL,
};

/** Timeouts of timing test [ms]. */
static const u32_t TIMING_TIMEOUT[] = {1, 10, 50};

static const test_entry_t TEST_TAB[] = {
        {.name = "Thread: create and join", .fn = test_thread_create_join},
        {.name = "Thread: memory release of finished threads", .fn = test_thread_memory},
        {.name = "Thread: slot exhaustion and reuse", .fn = test_thread_exhaustion},
        {.name = "Thread: nested creation", .fn = test_thread_nested},
        {.name = "Thread: cancel in various states", .fn = test_thread_cancel},
        {.name = "Heap: random operations", .fn = test_heap_single},
        {.name = "Heap: concurrent operations", .fn = test_heap_concurrent},
        {.name = "Heap: free by other thread", .fn = test_heap_cross},
        {.name = "Heap: exhaustion and recovery", .fn = test_heap_exhaustion, .heavy = true},
        {.name = "Mutex: recursion, trylock, timeout", .fn = test_mutex_functional},
        {.name = "Mutex: mutual exclusion", .fn = test_mutex_exclusion},
        {.name = "Mutex: mutual exclusion (recursive)", .fn = test_mutex_rec_exclusion},
        {.name = "Mutex: unlock by non-owner", .fn = test_mutex_non_owner, .heavy = true},
        {.name = "Futex: mutual exclusion", .fn = test_futex_exclusion},
        {.name = "Semaphore: producers and consumers", .fn = test_semaphore_pc},
        {.name = "Queue: producers and consumers", .fn = test_queue_mpmc},
        {.name = "Sync objects: create/delete storm", .fn = test_sync_storm},
        {.name = "Timing: sleep and timeouts under load", .fn = test_timing},
        {.name = "Process: spawn and exit status", .fn = test_process_spawn},
        {.name = "Process: kill under load", .fn = test_process_kill},
};

static const bench_entry_t BENCH_TAB[] = {
        {.name = "CPU score (single thread)", .fn = bench_cpu_score, .param = 0},
        {.name = "CPU score (all cores)", .fn = bench_cpu_score, .param = 1},
        {.name = "integer arithmetic", .fn = bench_int},
        {.name = "float arithmetic", .fn = bench_float, .param = 0},
        {.name = "double arithmetic", .fn = bench_float, .param = 1},
        {.name = "memcpy, block", .fn = bench_memcpy, .param = 16, .unit = "B"},
        {.name = "memcpy, block", .fn = bench_memcpy, .param = 256, .unit = "B"},
        {.name = "memcpy, block", .fn = bench_memcpy, .param = 4096, .unit = "B"},
        {.name = "memset, block", .fn = bench_memset, .param = 4096, .unit = "B"},
        {.name = "malloc/free, block", .fn = bench_malloc, .param = 16, .unit = "B"},
        {.name = "malloc/free, block", .fn = bench_malloc, .param = 256, .unit = "B"},
        {.name = "malloc/free, block", .fn = bench_malloc, .param = 2048, .unit = "B"},
        {.name = "syscall (getpid)", .fn = bench_syscall},
        {.name = "thread create/join", .fn = bench_thread},
        {.name = "context switch (semaphores)", .fn = bench_ctx_switch},
        {.name = "mutex lock/unlock", .fn = bench_mutex},
        {.name = "queue transfer (2 threads)", .fn = bench_queue},
        {.name = "process create/wait", .fn = bench_process},
};

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(perf, STACK_DEPTH_MEDIUM);

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
        if ((argc >= 2) && isstreq(argv[1], "--child")) {
                return child_main(argc, argv);
        }

        const char *filter = NULL;
        run_mode_t mode = MODE_ALL;
        u32_t repeat = 1;

        global->progname = argv[0];
        global->bench_duration = PERF_BENCH_DURATION;
        global->stress_duration = PERF_STRESS_DURATION;

        for (int i = 1; i < argc; i++) {
                const char *arg = argv[i];

                if (isstreq(arg, "-v")) {
                        global->verbose = true;
                } else if (isstreq(arg, "-b")) {
                        mode = MODE_BENCH;
                } else if (isstreq(arg, "-s")) {
                        mode = MODE_STRESS;
                } else if (isstreq(arg, "-l")) {
                        mode = MODE_LIST;
                } else if (isstreq(arg, "-x")) {
                        global->heavy = true;
                } else if (isstreq(arg, "-d") && (i + 1 < argc)) {
                        u32_t ms = strtoul(argv[++i], NULL, 0);
                        global->bench_duration = max(100U, ms);
                } else if (isstreq(arg, "-t") && (i + 1 < argc)) {
                        u32_t ms = strtoul(argv[++i], NULL, 0);
                        global->stress_duration = max(100U, ms);
                } else if (isstreq(arg, "-r") && (i + 1 < argc)) {
                        repeat = strtoul(argv[++i], NULL, 0);
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
                puts("Performance measurements:");
                for (size_t i = 0; i < ARRAY_SIZE(BENCH_TAB); i++) {
                        if (selected(BENCH_TAB[i].name, filter)) {
                                if (BENCH_TAB[i].unit) {
                                        printf("  %s %u %s\n", BENCH_TAB[i].name,
                                                (unsigned)BENCH_TAB[i].param, BENCH_TAB[i].unit);
                                } else {
                                        printf("  %s\n", BENCH_TAB[i].name);
                                }
                        }
                }
                puts("Stress tests:");
                for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                        if (selected(TEST_TAB[i].name, filter)) {
                                printf("  %2u: %s%s\n", (unsigned)(i + 1), TEST_TAB[i].name,
                                        TEST_TAB[i].heavy ? " (-x)" : "");
                        }
                }
                return EXIT_SUCCESS;
        }

        /*
         * NOTE: heap of program is created at the first allocation. It is
         *       created here, so it is not reported as resource leak of the
         *       first test.
         */
        free(malloc(1));

        global->cpu_freq = get_cpu_freq();

        memstat_t mem;
        memset(&mem, 0, sizeof(mem));
        get_memory_usage_details(&mem);

        printf(VT100_FONT_COLOR_YELLOW "%s %s, %s" VT100_RESET_ATTRIBUTES "\n", get_OS_name(),
                get_OS_version(), get_platform_name());
        printf("CPU: %u core(s), ", (unsigned)get_cpu_cores());
        if (global->cpu_freq) {
                printf("%u MHz, ", (unsigned)(global->cpu_freq / 1000000));
        } else {
                printf("frequency unknown, ");
        }
        printf("memory: %u KiB free of %u KiB\n", (unsigned)(mem.free_memory / 1024),
                (unsigned)(mem.memory_size / 1024));

        size_t executed = 0;

        if ((mode == MODE_ALL) || (mode == MODE_BENCH)) {
                run_benchmarks(filter);
        }

        if ((mode == MODE_ALL) || (mode == MODE_STRESS)) {
                for (u32_t round = 1; (repeat == 0) || (round <= repeat); round++) {
                        if (repeat != 1) {
                                printf("\n" VT100_FONT_COLOR_YELLOW "ROUND %u" VT100_RESET_ATTRIBUTES,
                                        (unsigned)round);
                                if (repeat) {
                                        printf(" of %u", (unsigned)repeat);
                                }
                                puts("");
                        }

                        executed += run_tests(filter);

                        if ((repeat == 0) && global->failed) {
                                break;
                        }
                }

                puts("");
                printf(VT100_FONT_COLOR_YELLOW "SUMMARY:" VT100_RESET_ATTRIBUTES
                                               " executed: %u, passed: %u, failed: %u, skipped: %u\n",
                        (unsigned)executed, (unsigned)global->passed, (unsigned)global->failed,
                        (unsigned)global->skipped);

                if (global->verbose) {
                        thread_stat_t stat;
                        if (thread_stat(getpid(), 0, &stat) == 0) {
                                printf("Stack usage of main thread: %u of %u levels\n",
                                        (unsigned)stat.stack_max_usage, (unsigned)stat.stack_size);
                        }
                }
        }

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
        printf("Usage: %s [-b|-s|-l] [-v] [-x] [-d ms] [-t ms] [-r n] [filter]\n", name);
        puts("  (none)  performance measurements, then stress tests");
        puts("  -b      performance measurements only");
        puts("  -s      stress tests only");
        puts("  -l      list measurements and tests");
        puts("  -v      verbose mode");
        puts("  -x      also tests that exhaust system memory or abuse the kernel");
        puts("          (other programs can be affected, kernel panic possible)");
        puts("  -d      duration of single measurement in ms");
        puts("  -t      duration of single stress test in ms");
        puts("  -r      repeat stress tests n times (0: until failure)");
        puts("  filter  run only measurements/tests which name contains given text");
        puts("Stress tests load the system heavily, other programs can be slowed down.");
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
 * @brief  Run stress tests. Resources of the program are checked after each
 *         test (memory, threads, synchronization objects, files).
 *
 * @param  filter       filter (can be NULL)
 *
 * @return Number of executed tests.
 */
//==============================================================================
static size_t run_tests(const char *filter)
{
        size_t executed = 0;

        printf("\n" VT100_FONT_COLOR_YELLOW "STRESS TESTS (%u ms per test)" VT100_RESET_ATTRIBUTES "\n",
                (unsigned)global->stress_duration);

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

                global->info_buf[0] = '\0';

                const char *msg;
                clock_t tref = get_uptime_ms();

                if (test->heavy && !global->heavy) {
                        msg = SKIP("use -x option");

                } else {
                        snapshot_t snap;
                        bool snap_valid = snapshot_take(&snap);

                        msg = test->fn();

                        if (isstreq(msg, SUCCESS_MSG) && snap_valid) {
                                const char *leak = snapshot_check(&snap);
                                if (leak) {
                                        msg = leak;
                                }
                        }
                }

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
                        printf(" (%u ms", (unsigned)dt);
                        if (global->info_buf[0]) {
                                printf(", %s", global->info_buf);
                        }
                        printf(")");
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

        printf("\n" VT100_FONT_COLOR_YELLOW "PERFORMANCE (%u ms per measurement)" VT100_RESET_ATTRIBUTES "\n",
                (unsigned)global->bench_duration);

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

                if (bench->unit) {
                        printf("  %s %u %s: %s\n", bench->name, (unsigned)bench->param, bench->unit,
                                global->res_buf);
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
 * @brief  Print transfer speed to result of performance measurement.
 *
 * @param  bytes        transferred bytes
 * @param  dt           time [ms]
 */
//==============================================================================
static void res_speed(u64_t bytes, u32_t dt)
{
        double speed = (double)bytes / (double)dt / 1000.0; // MB/s

        if (speed >= 1.0) {
                res_printf("%.2f MB/s", speed);
        } else {
                res_printf("%.1f KB/s", speed * 1000.0);
        }
}

//==============================================================================
/**
 * @brief  Print operation rate to result of performance measurement.
 *
 * @param  ops          number of operations
 * @param  dt           time [ms]
 * @param  what         operation name
 */
//==============================================================================
static void res_rate(u32_t ops, u32_t dt, const char *what)
{
        if (ops == 0) {
                res_printf("no %s", what);
                return;
        }

        double rate = (double)ops * 1000.0 / (double)dt;
        double us = (double)dt * 1000.0 / (double)ops;

        if (rate >= 10000.0) {
                res_printf("%.1f k %s/s, %.2f us each", rate / 1000.0, what, us);
        } else {
                res_printf("%.1f %s/s, %.1f us each", rate, what, us);
        }
}

//==============================================================================
/**
 * @brief  Set additional information of test (printed in verbose mode).
 *
 * @param  fmt          format
 */
//==============================================================================
static void info_printf(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vsnprintf(global->info_buf, sizeof(global->info_buf), fmt, args);
        va_end(args);
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
 * @brief  Check measured time of operation with timeout.
 *
 * @param  dt           measured time [ms]
 * @param  timeout      timeout [ms]
 *
 * @return true if time is correct.
 */
//==============================================================================
static bool time_ok(u32_t dt, u32_t timeout)
{
        // uptime has 1 ms resolution, so measured time can be shorter by 1 ms
        return (dt + 1 >= timeout) && (dt <= timeout + PERF_TIMING_TOLERANCE);
}

//==============================================================================
/**
 * @brief  Read CPU frequency from clock driver.
 *
 * @return CPU frequency [Hz] or 0 if not available.
 */
//==============================================================================
static u32_t get_cpu_freq(void)
{
        u32_t freq = 0;

        FILE *clk = fopen("/dev/clk", "r");
        if (clk) {
                CLK_info_t ck;
                ck.iterator = 0;

                while (ioctl(fileno(clk), IOCTL_CLK__GET_CLK_INFO, &ck) == 0) {
                        if (ck.name == NULL) {
                                break;

                        } else if (isstreq(ck.name, "CPUCLK")) {
                                freq = ck.freq_Hz;
                                break;
                        }
                }

                fclose(clk);
        }

        return freq;
}

//==============================================================================
/**
 * @brief  Return number of CPU cores.
 *
 * @return Number of cores (at least 1).
 */
//==============================================================================
static u8_t get_cpu_cores(void)
{
        u8_t cores = 1;

        if ((thread_get_cpu_core_count(&cores) != 0) || (cores == 0)) {
                cores = 1;
        }

        return cores;
}

//==============================================================================
/**
 * @brief  Create new seed of pseudo random generator.
 *
 * @return Seed (never 0).
 */
//==============================================================================
static u32_t new_seed(void)
{
        u32_t seed = (u32_t)get_uptime_ms() ^ ((u32_t)getpid() << 16) ^ (++global->seed_ctr * 2654435761u);
        return seed ? seed : 0x9E3779B9;
}

//==============================================================================
/**
 * @brief  Pseudo random generator (xorshift32). Each thread uses own seed.
 *
 * @param  seed         generator state
 *
 * @return Random value.
 */
//==============================================================================
static u32_t rnd(u32_t *seed)
{
        u32_t x = *seed;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        *seed = x ? x : 0x9E3779B9;
        return x;
}

//==============================================================================
/**
 * @brief  Random value from selected range (inclusive).
 *
 * @param  seed         generator state
 * @param  lo           low limit
 * @param  hi           high limit
 *
 * @return Random value.
 */
//==============================================================================
static u32_t rnd_range(u32_t *seed, u32_t lo, u32_t hi)
{
        return lo + (rnd(seed) % (hi - lo + 1));
}

//==============================================================================
/**
 * @brief  Random priority from selected range (inclusive).
 *
 * @param  seed         generator state
 * @param  lo           low limit
 * @param  hi           high limit
 *
 * @return Random priority.
 */
//==============================================================================
static i16_t rnd_priority(u32_t *seed, i16_t lo, i16_t hi)
{
        return lo + (i16_t)(rnd(seed) % (u32_t)(hi - lo + 1));
}

//==============================================================================
/**
 * @brief  Shuffle table of indexes.
 *
 * @param  tab          table
 * @param  n            number of items
 * @param  seed         generator state
 */
//==============================================================================
static void shuffle(u8_t *tab, size_t n, u32_t *seed)
{
        for (size_t i = n; i > 1; i--) {
                size_t j = rnd(seed) % i;
                u8_t t = tab[i - 1];
                tab[i - 1] = tab[j];
                tab[j] = t;
        }
}

//==============================================================================
/**
 * @brief  Short computation with result that depends on input value.
 *
 * @param  value        input value
 *
 * @return Result (positive int value).
 */
//==============================================================================
static u32_t compute(u32_t value)
{
        u32_t x = value | 1;
        u32_t n = (value & 0x3FF) + 16;

        for (u32_t i = 0; i < n; i++) {
                x ^= x << 13;
                x ^= x >> 17;
                x ^= x << 5;
        }

        return x & 0x7FFFFFFF;
}

//==============================================================================
/**
 * @brief  Fill buffer with pattern.
 *
 * @param  buf          buffer
 * @param  len          buffer length
 * @param  seed         pattern seed
 */
//==============================================================================
static void pattern_fill(u8_t *buf, size_t len, u32_t seed)
{
        u32_t x = seed | 1;

        for (size_t i = 0; i < len; i++) {
                x = x * 1103515245u + 12345u;
                buf[i] = x >> 24;
        }
}

//==============================================================================
/**
 * @brief  Check pattern of buffer.
 *
 * @param  buf          buffer
 * @param  len          buffer length
 * @param  seed         pattern seed
 *
 * @return Offset of the first incorrect byte or len if buffer is correct.
 */
//==============================================================================
static size_t pattern_check(const u8_t *buf, size_t len, u32_t seed)
{
        u32_t x = seed | 1;

        for (size_t i = 0; i < len; i++) {
                x = x * 1103515245u + 12345u;
                if (buf[i] != (u8_t)(x >> 24)) {
                        return i;
                }
        }

        return len;
}

//==============================================================================
/**
 * @brief  Initialize failure container.
 *
 * @param  fail         failure container
 */
//==============================================================================
static void fail_init(fail_t *fail)
{
        memset(fail, 0, sizeof(*fail));
        futex_init(&fail->lock);
}

//==============================================================================
/**
 * @brief  Store failure detected by worker thread (only the first one is
 *         kept).
 *
 * @param  fail         failure container
 * @param  what         failure description (static string)
 * @param  a            the first value
 * @param  b            the second value
 *
 * @return Always false (for convenience).
 */
//==============================================================================
static bool fail_set(fail_t *fail, const char *what, u32_t a, u32_t b)
{
        if (fail && futex_lock(&fail->lock, MAX_DELAY_MS)) {
                if (!fail->set) {
                        fail->what = what;
                        fail->a = a;
                        fail->b = b;
                        fail->set = true;
                }
                futex_unlock(&fail->lock);
        }

        return false;
}

//==============================================================================
/**
 * @brief  Create message of failure detected by worker thread.
 *
 * @param  fail         failure container
 *
 * @return Message.
 */
//==============================================================================
static const char *fail_msg(const fail_t *fail)
{
        return make_msg(0, "%s [%u, %u]", fail->what, (unsigned)fail->a, (unsigned)fail->b);
}

//==============================================================================
/**
 * @brief  Read resource usage of this process.
 *
 * @param  snap         snapshot
 *
 * @return true if statistics are available.
 */
//==============================================================================
static bool snapshot_take(snapshot_t *snap)
{
        process_stat_t stat;
        memset(&stat, 0, sizeof(stat));

        if (process_stat(getpid(), &stat) != 0) {
                return false;
        }

        snap->mem = stat.memory_usage;
        snap->blocks = stat.memory_block_count;
        snap->threads = stat.threads_count;
        snap->mutexes = stat.mutexes_count;
        snap->semaphores = stat.semaphores_count;
        snap->queues = stat.queue_count;
        snap->files = stat.files_count;

        return true;
}

//==============================================================================
/**
 * @brief  Check that resources of this process returned to reference state.
 *         Resources can be released in background, so check is repeated by
 *         settle time.
 *
 * @param  ref          reference snapshot
 *
 * @return NULL if resources are released, otherwise error message.
 */
//==============================================================================
static const char *snapshot_check(const snapshot_t *ref)
{
        snapshot_t now;
        clock_t tref = get_uptime_ms();

        while (true) {
                if (!snapshot_take(&now)) {
                        return NULL;
                }

                if ((now.mem <= ref->mem) && (now.blocks <= ref->blocks) && (now.threads <= ref->threads) &&
                        (now.mutexes <= ref->mutexes) && (now.semaphores <= ref->semaphores) &&
                        (now.queues <= ref->queues) && (now.files <= ref->files)) {
                        return NULL;
                }

                if (elapsed_ms(tref) >= SETTLE_TIME) {
                        break;
                }

                msleep(50);
        }

        return make_msg(0,
                "resources not released (before -> after): memory %u -> %u B (blocks %u -> %u), "
                "threads %u -> %u, mutexes %u -> %u, semaphores %u -> %u, queues %u -> %u, files %u -> %u",
                (unsigned)ref->mem, (unsigned)now.mem, (unsigned)ref->blocks, (unsigned)now.blocks,
                (unsigned)ref->threads, (unsigned)now.threads, (unsigned)ref->mutexes, (unsigned)now.mutexes,
                (unsigned)ref->semaphores, (unsigned)now.semaphores, (unsigned)ref->queues,
                (unsigned)now.queues, (unsigned)ref->files, (unsigned)now.files);
}

//==============================================================================
/**
 * @brief  Read system memory statistics.
 *
 * @param  stat         statistics
 *
 * @return true if statistics are available.
 */
//==============================================================================
static bool sys_mem_get(memstat_t *stat)
{
        memset(stat, 0, sizeof(*stat));
        return get_memory_usage_details(stat) == 0;
}

//==============================================================================
/**
 * @brief  Check that used system memory returned to reference value (used
 *         after tests of child processes, memory of process is released in
 *         background).
 *
 * @param  ref          reference statistics
 *
 * @return NULL if memory is released, otherwise error message.
 */
//==============================================================================
static const char *sys_mem_check(const memstat_t *ref)
{
        memstat_t now;
        clock_t tref = get_uptime_ms();

        while (sys_mem_get(&now) && (now.used_memory > ref->used_memory) &&
                (elapsed_ms(tref) < SETTLE_TIME)) {
                msleep(50);
        }

        if (now.used_memory > ref->used_memory) {
                return make_msg(0,
                        "system memory not released: +%d B (kernel %+d, programs %+d, file systems %+d, "
                        "network %+d, modules %+d, shared %+d)",
                        (int)(now.used_memory - ref->used_memory),
                        (int)(now.kernel_memory_usage - ref->kernel_memory_usage),
                        (int)(now.programs_memory_usage - ref->programs_memory_usage),
                        (int)(now.filesystems_memory_usage - ref->filesystems_memory_usage),
                        (int)(now.network_memory_usage - ref->network_memory_usage),
                        (int)(now.modules_memory_usage - ref->modules_memory_usage),
                        (int)(now.shared_memory_usage - ref->shared_memory_usage));
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Count existing child processes (started in child mode).
 *
 * @return Number of processes.
 */
//==============================================================================
static size_t count_children(void)
{
        char cmdline[CMDLINE_LEN];
        size_t count = 0;
        process_stat_t stat;

        for (size_t seek = 0;; seek++) {
                memset(&stat, 0, sizeof(stat));
                cmdline[0] = '\0';
                stat.cmdline_buf = cmdline;
                stat.cmdline_len = sizeof(cmdline);

                if (process_stat_seek(seek, &stat) != 0) {
                        break;
                }

                if (strstr(cmdline, "--child")) {
                        count++;
                }
        }

        return count;
}

//==============================================================================
/**
 * @brief  Initialize worker.
 *
 * @param  w            worker
 * @param  id           worker ID
 * @param  fail         failure container (can be NULL)
 * @param  ctx          test context
 * @param  duration     work duration [ms]
 */
//==============================================================================
static void worker_init(worker_t *w, u8_t id, fail_t *fail, void *ctx, u32_t duration)
{
        memset(w, 0, sizeof(*w));
        w->id = id;
        w->fail = fail;
        w->ctx = ctx;
        w->seed = new_seed();
        w->deadline = get_uptime_ms() + duration;
}

//==============================================================================
/**
 * @brief  Start worker thread.
 *
 * @param  w            worker
 * @param  func         thread function
 * @param  priority     thread priority
 * @param  stack_depth  stack depth
 *
 * @return true if thread was created.
 */
//==============================================================================
static bool worker_start(worker_t *w, thread_func_t func, i16_t priority, size_t stack_depth)
{
        const thread_attr_t attr = {
                .stack_depth = stack_depth,
                .priority = priority,
                .detached = false,
        };

        w->priority = priority;
        w->tid = thread_create(func, &attr, w);

        return w->tid != 0;
}

//==============================================================================
/**
 * @brief  Check if worker should continue work.
 *
 * @param  w            worker
 *
 * @return true if worker should continue.
 */
//==============================================================================
static bool worker_running(const worker_t *w)
{
        return !(w->stop && *w->stop) && !(w->fail && w->fail->set) && (get_uptime_ms() < w->deadline);
}

//==============================================================================
/**
 * @brief  Count operation. Workers with priority higher than normal sleep
 *         from time to time, otherwise they would starve other workers and
 *         the main thread on single core CPU.
 *
 * @param  w            worker
 */
//==============================================================================
static void worker_pace(worker_t *w)
{
        w->ops++;

        if ((w->priority > PRIORITY_NORMAL) && ((w->ops & 31) == 0)) {
                msleep(1);
        }
}

//==============================================================================
/**
 * @brief  Join workers. Worker which does not finish in timeout is canceled.
 *
 * @param  w            workers
 * @param  count        number of workers
 * @param  timeout      timeout of each worker [ms]
 *
 * @return true if all workers finished in time.
 */
//==============================================================================
static bool workers_join(worker_t *w, size_t count, u32_t timeout)
{
        bool ok = true;

        for (size_t i = 0; i < count; i++) {
                if (w[i].tid) {
                        if (thread_join2(w[i].tid, NULL, timeout) != 0) {
                                ok = false;
                                thread_cancel(w[i].tid);
                                thread_join2(w[i].tid, NULL, CANCEL_TIMEOUT);
                        }

                        w[i].tid = 0;
                }
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Join thread (cleanup). Thread is canceled when not finished in time.
 *
 * @param  tid          thread ID
 * @param  timeout      timeout [ms]
 */
//==============================================================================
static void thread_join_or_cancel(tid_t tid, u32_t timeout)
{
        if (thread_join2(tid, NULL, timeout) != 0) {
                thread_cancel(tid);
                thread_join2(tid, NULL, CANCEL_TIMEOUT);
        }
}

/*==============================================================================
  Heap worker
==============================================================================*/

//==============================================================================
/**
 * @brief  Calculate memory budget of heap workers from free system memory.
 *
 * @param  workers      number of workers
 *
 * @return Budget of single worker [B].
 */
//==============================================================================
static size_t heap_budget(size_t workers)
{
        memstat_t stat;
        size_t budget = HEAP_BUDGET_MAX;

        if (get_memory_usage_details(&stat) == 0) {
                budget = min(budget, stat.free_memory / 4);
        }

        return max(1024U, budget / workers);
}

//==============================================================================
/**
 * @brief  Create heap worker context.
 *
 * @param  fail         failure container
 * @param  budget       memory budget
 * @param  check        check heap consistency during work
 *
 * @return Context or NULL if no memory.
 */
//==============================================================================
static heap_ctx_t *heap_ctx_new(fail_t *fail, size_t budget, bool check)
{
        heap_ctx_t *h = calloc(1, sizeof(heap_ctx_t));
        if (h) {
                h->fail = fail;
                h->budget = budget;
                h->check = check;
                h->seed = new_seed();
        }

        return h;
}

//==============================================================================
/**
 * @brief  Random block size (mostly small blocks).
 *
 * @param  h            heap context
 *
 * @return Block size.
 */
//==============================================================================
static u32_t heap_block_size(heap_ctx_t *h)
{
        u32_t max_size = max(256U, min(HEAP_BLOCK_MAX, h->budget / 4));
        u32_t r = rnd(&h->seed) % 100;

        if (r < 70) {
                return rnd_range(&h->seed, 1, 64);
        } else if (r < 95) {
                return rnd_range(&h->seed, 65, min(1024U, max_size));
        } else {
                return rnd_range(&h->seed, min(1025U, max_size), max_size);
        }
}

//==============================================================================
/**
 * @brief  Single random heap operation: allocation (malloc, calloc,
 *         posix_memalign), reallocation or free of block. Content of each
 *         block is verified before free or reallocation.
 *
 * @param  h            heap context
 *
 * @return true on success, false on failure (stored in failure container).
 */
//==============================================================================
static bool heap_step(heap_ctx_t *h)
{
        hslot_t *s = &h->slot[rnd(&h->seed) % HEAP_SLOTS];
        u32_t r = rnd(&h->seed);

        h->ops++;

        if (h->check && ((h->ops & 63) == 0)) {
                if (check_heap_consistency() != 0) {
                        return fail_set(h->fail, "heap consistency check failed (operation)", h->ops, 0);
                }
        }

        if (s->ptr) {
                size_t bad = pattern_check(s->ptr, s->size, s->seed);
                if (bad < s->size) {
                        return fail_set(h->fail, "content of heap block corrupted (size, offset)", s->size,
                                bad);
                }

                if (((r & 3) == 0) && (s->align == 0)) {
                        u32_t size = heap_block_size(h);
                        if (h->live - s->size + size > h->budget) {
                                size = s->size;
                        }

                        u8_t *ptr = realloc(s->ptr, size);
                        if (!ptr) {
                                // original block is left untouched
                                h->nomem++;
                                return true;
                        }

                        size_t n = min(size, s->size);
                        bad = pattern_check(ptr, n, s->seed);
                        if (bad < n) {
                                s->ptr = ptr;
                                return fail_set(h->fail, "realloc() did not preserve content (size, offset)",
                                        n, bad);
                        }

                        h->live = h->live - s->size + size;
                        s->ptr = ptr;
                        s->size = size;
                        pattern_fill(s->ptr, s->size, s->seed);

                } else {
                        free(s->ptr);
                        h->live -= s->size;
                        s->ptr = NULL;
                }

        } else {
                u32_t size = heap_block_size(h);
                if (h->live + size > h->budget) {
                        return true;
                }

                u8_t *ptr = NULL;
                u16_t align = 0;
                u32_t kind = r & 7;

                if (kind < 5) {
                        ptr = malloc(size);

                } else if (kind == 5) {
                        ptr = calloc(1, size);
                        if (ptr) {
                                for (size_t i = 0; i < size; i++) {
                                        if (ptr[i] != 0) {
                                                free(ptr);
                                                return fail_set(h->fail,
                                                        "calloc() returned not cleared block (size, offset)",
                                                        size, i);
                                        }
                                }
                        }

                } else {
                        align = 8 << ((r >> 3) % 6);
                        void *mem = NULL;
                        if (posix_memalign(&mem, align, size) == 0) {
                                ptr = mem;
                                if ((uintptr_t)ptr & (align - 1)) {
                                        free(ptr);
                                        return fail_set(h->fail,
                                                "posix_memalign() returned not aligned block (align, "
                                                "address)",
                                                align, (u32_t)(uintptr_t)mem);
                                }
                        }
                }

                if (!ptr) {
                        h->nomem++;
                        return true;
                }

                s->ptr = ptr;
                s->size = size;
                s->align = align;
                s->seed = rnd(&h->seed);
                pattern_fill(s->ptr, s->size, s->seed);
                h->live += size;
                h->allocs++;
        }

        return true;
}

//==============================================================================
/**
 * @brief  Verify and free all blocks of heap worker.
 *
 * @param  h            heap context
 *
 * @return true if content of all blocks was correct.
 */
//==============================================================================
static bool heap_release(heap_ctx_t *h)
{
        bool ok = true;

        for (size_t i = 0; i < HEAP_SLOTS; i++) {
                hslot_t *s = &h->slot[i];

                if (s->ptr) {
                        size_t bad = pattern_check(s->ptr, s->size, s->seed);
                        if (bad < s->size) {
                                ok = fail_set(h->fail, "content of heap block corrupted (size, offset)",
                                        s->size, bad);
                        }

                        free(s->ptr);
                        s->ptr = NULL;
                }
        }

        h->live = 0;

        return ok;
}

/*==============================================================================
  Synchronization objects worker
==============================================================================*/

//==============================================================================
/**
 * @brief  Create synchronization objects worker context.
 *
 * @param  fail         failure container
 *
 * @return Context or NULL if no memory.
 */
//==============================================================================
static sync_ctx_t *sync_ctx_new(fail_t *fail)
{
        sync_ctx_t *s = calloc(1, sizeof(sync_ctx_t));
        if (s) {
                s->fail = fail;
                s->seed = new_seed();
        }

        return s;
}

//==============================================================================
/**
 * @brief  Delete synchronization object.
 *
 * @param  o            object
 */
//==============================================================================
static void sobj_delete(sobj_t *o)
{
        switch (o->type) {
        case SOBJ_MUTEX:
        case SOBJ_MUTEX_REC:
                mutex_delete(o->obj);
                break;
        case SOBJ_SEMAPHORE:
                semaphore_delete(o->obj);
                break;
        case SOBJ_QUEUE:
                queue_delete(o->obj);
                break;
        default:
                break;
        }

        o->type = SOBJ_NONE;
        o->obj = NULL;
}

//==============================================================================
/**
 * @brief  Single random operation on synchronization objects: create object
 *         (state of new object is verified), use object, delete object.
 *
 * @param  s            context
 *
 * @return true on success, false on failure (stored in failure container).
 */
//==============================================================================
static bool sync_step(sync_ctx_t *s)
{
        sobj_t *o = &s->slot[rnd(&s->seed) % SYNC_SLOTS];

        s->ops++;

        if (o->type == SOBJ_NONE) {
                sobj_type_t type = rnd_range(&s->seed, SOBJ_MUTEX, SOBJ_QUEUE);

                switch (type) {
                case SOBJ_MUTEX:
                case SOBJ_MUTEX_REC:
                        o->obj = mutex_new(type == SOBJ_MUTEX ? MUTEX_TYPE_NORMAL : MUTEX_TYPE_RECURSIVE);
                        break;

                case SOBJ_SEMAPHORE:
                        // semaphore value is kept equal to p2
                        o->p1 = rnd_range(&s->seed, 1, 10);
                        o->p2 = rnd_range(&s->seed, 0, o->p1);
                        o->obj = semaphore_new(o->p1, o->p2);
                        if (o->obj) {
                                int value = semaphore_get_value(o->obj);
                                if (value != o->p2) {
                                        semaphore_delete(o->obj);
                                        o->obj = NULL;
                                        return fail_set(s->fail,
                                                "incorrect value of new semaphore (value, expected)", value,
                                                o->p2);
                                }
                        }
                        break;

                case SOBJ_QUEUE:
                        o->p1 = rnd_range(&s->seed, 1, 8);
                        o->p2 = rnd_range(&s->seed, 1, 32);
                        o->obj = queue_new(o->p1, o->p2);
                        if (o->obj) {
                                int items = queue_get_number_of_items(o->obj);
                                int space = queue_get_space_available(o->obj);
                                if ((items != 0) || (space != o->p1)) {
                                        queue_delete(o->obj);
                                        o->obj = NULL;
                                        return fail_set(s->fail, "new queue is not empty (items, free space)",
                                                items, space);
                                }
                        }
                        break;

                default:
                        break;
                }

                if (o->obj) {
                        o->type = type;
                        s->created++;
                } else {
                        s->nomem++;
                }

                return true;
        }

        switch (o->type) {
        case SOBJ_MUTEX:
        case SOBJ_MUTEX_REC:
                if (!mutex_lock(o->obj, 0)) {
                        return fail_set(s->fail, "unable to lock free mutex (type, operation)", o->type,
                                s->ops);
                }
                if (o->type == SOBJ_MUTEX_REC) {
                        if (!mutex_lock(o->obj, 0)) {
                                mutex_unlock(o->obj);
                                return fail_set(s->fail,
                                        "unable to lock recursive mutex twice (type, operation)", o->type,
                                        s->ops);
                        }
                        if (!mutex_unlock(o->obj)) {
                                return fail_set(s->fail, "unable to unlock recursive mutex (type, operation)",
                                        o->type, s->ops);
                        }
                }
                if (!mutex_unlock(o->obj)) {
                        return fail_set(s->fail, "unable to unlock mutex (type, operation)", o->type, s->ops);
                }
                break;

        case SOBJ_SEMAPHORE:
                if (o->p2 < o->p1) {
                        if (!semaphore_signal(o->obj) || !semaphore_wait(o->obj, 0)) {
                                return fail_set(s->fail, "semaphore signal/wait failed (max, value)", o->p1,
                                        o->p2);
                        }
                }
                if (o->p2 > 0) {
                        if (!semaphore_wait(o->obj, 0) || !semaphore_signal(o->obj)) {
                                return fail_set(s->fail, "semaphore wait/signal failed (max, value)", o->p1,
                                        o->p2);
                        }
                }
                if (semaphore_get_value(o->obj) != o->p2) {
                        return fail_set(s->fail, "incorrect semaphore value (value, expected)",
                                semaphore_get_value(o->obj), o->p2);
                }
                break;

        case SOBJ_QUEUE: {
                u8_t item[32];
                u8_t back[32];
                u32_t seed = rnd(&s->seed);
                pattern_fill(item, o->p2, seed);

                if (!queue_send(o->obj, item, 0)) {
                        return fail_set(s->fail, "unable to send item to empty queue (length, item size)",
                                o->p1, o->p2);
                }
                if (!queue_receive(o->obj, back, 0)) {
                        return fail_set(s->fail, "unable to receive item from queue (length, item size)",
                                o->p1, o->p2);
                }
                if (pattern_check(back, o->p2, seed) != o->p2) {
                        return fail_set(s->fail, "item received from queue is corrupted (length, item size)",
                                o->p1, o->p2);
                }
                break;
        }

        default:
                break;
        }

        if ((rnd(&s->seed) & 1) == 0) {
                sobj_delete(o);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Delete all objects of synchronization worker.
 *
 * @param  s            context
 */
//==============================================================================
static void sync_release(sync_ctx_t *s)
{
        for (size_t i = 0; i < SYNC_SLOTS; i++) {
                sobj_delete(&s->slot[i]);
        }
}

/*==============================================================================
  Child process
==============================================================================*/

//==============================================================================
/**
 * @brief  Start child process (this program in child mode).
 *
 * @param  args         child arguments
 *
 * @return PID or 0 on error.
 */
//==============================================================================
static pid_t spawn_child(const char *args)
{
        char cmd[CMD_LEN];
        snprintf(cmd, sizeof(cmd), "%s --child %s", global->progname, args);

        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.fd_stdin = STDIN_FILENO;
        attr.fd_stdout = STDOUT_FILENO;
        attr.fd_stderr = STDERR_FILENO;
        attr.priority = PRIORITY_NORMAL;
        attr.detached = false;

        return process_create(cmd, &attr);
}

//==============================================================================
/**
 * @brief  Main function of child mode.
 *         "--child exit <status>": exit immediately with selected status,
 *         "--child work": load the system (threads, heap, synchronization
 *         objects) until killed or until CHILD_LIFETIME.
 *
 * @param  argc         argument count
 * @param  argv         arguments
 *
 * @return Exit status.
 */
//==============================================================================
static int child_main(int argc, char *argv[])
{
        if ((argc >= 4) && isstreq(argv[2], "exit")) {
                return atoi(argv[3]);
        }

        if ((argc >= 3) && isstreq(argv[2], "work")) {
                return child_work();
        }

        return EXIT_FAILURE;
}

//==============================================================================
/**
 * @brief  Work of child process. Process is expected to be killed by parent.
 *
 * @return Exit status.
 */
//==============================================================================
static int child_work(void)
{
        fail_t fail;
        worker_t w[3];
        int status = EXIT_FAILURE;

        fail_init(&fail);
        memset(w, 0, sizeof(w));

        heap_ctx_t *h1 = heap_ctx_new(&fail, CHILD_HEAP_BUDGET, false);
        heap_ctx_t *h2 = heap_ctx_new(&fail, CHILD_HEAP_BUDGET, false);
        sync_ctx_t *sc = sync_ctx_new(&fail);
        sem_t *sem = semaphore_new(1, 0);

        if (h1 && h2 && sc && sem) {
                worker_init(&w[0], 0, &fail, h1, CHILD_LIFETIME);
                worker_init(&w[1], 1, &fail, sc, CHILD_LIFETIME);
                worker_init(&w[2], 2, &fail, sem, CHILD_LIFETIME);

                worker_start(&w[0], th_heap_worker, PRIORITY_NORMAL, STACK_DEPTH_LOW);
                worker_start(&w[1], th_sync_worker, PRIORITY_NORMAL, STACK_DEPTH_LOW);
                worker_start(&w[2], th_sleeper, PRIORITY_NORMAL, STACK_DEPTH_LOW);

                clock_t deadline = get_uptime_ms() + CHILD_LIFETIME;
                while ((get_uptime_ms() < deadline) && !fail.set) {
                        heap_step(h2);
                        semaphore_signal(sem);
                }

                heap_release(h2);
                workers_join(w, ARRAY_SIZE(w), SAFE_TIMEOUT);
                status = fail.set ? EXIT_FAILURE : EXIT_SUCCESS;
        }

        free(h1);
        free(h2);
        if (sc) {
                sync_release(sc);
                free(sc);
        }
        semaphore_delete(sem);

        return status;
}

/*==============================================================================
  Threads
==============================================================================*/

//==============================================================================
/**
 * @brief  CPU score thread (SHA-256 and formatted output).
 *
 * @param  arg          job (cpu_job_t)
 *
 * @return 0.
 */
//==============================================================================
static int th_cpu_score(void *arg)
{
        cpu_job_t *job = arg;

        if (job->start) {
                semaphore_wait(job->start, SAFE_TIMEOUT);
        }

        u32_t n = 0;
        clock_t tstart = get_uptime_ms();
        clock_t now;

        while (((now = get_uptime_ms()) - tstart) < job->duration) {

                for (size_t i = 0; i < sizeof(job->buf); i++) {
                        job->buf[i] = rand_r(&job->seed);
                }

                SHA256_CTX ctx;
                sha256_init(&ctx);
                sha256_update(&ctx, job->buf, sizeof(job->buf));
                sha256_final(&ctx, job->hash);

                snprintf((char *)job->buf, sizeof(job->buf), "HASH: ");
                for (size_t i = 0; i < SHA256_BLOCK_SIZE; i++) {
                        snprintf((char *)job->buf, sizeof(job->buf), "%02X", job->hash[i]);
                }

                double d1 = (double)job->hash[0];
                double d2 = (double)job->buf[3];
                snprintf((char *)job->buf, sizeof(job->buf), "%f", d1 / d2);

                float f1 = (float)job->hash[2];
                float f2 = (float)job->buf[5];
                snprintf((char *)job->buf, sizeof(job->buf), "%f", (double)(f1 / f2));

                n++;
        }

        job->points = n;
        job->elapsed = (u32_t)(now - tstart);

        return 0;
}

//==============================================================================
/**
 * @brief  Thread that does nothing.
 *
 * @param  arg          not used
 *
 * @return 0.
 */
//==============================================================================
static int th_noop(void *arg)
{
        UNUSED_ARG1(arg);
        return 0;
}

//==============================================================================
/**
 * @brief  Thread that returns result of computation as exit status.
 *
 * @param  arg          input value
 *
 * @return Result of computation.
 */
//==============================================================================
static int th_compute(void *arg)
{
        return (int)compute((u32_t)(uintptr_t)arg);
}

//==============================================================================
/**
 * @brief  Ping-pong thread (context switch measurement).
 *
 * @param  arg          context (pingpong_t)
 *
 * @return 0.
 */
//==============================================================================
static int th_pong(void *arg)
{
        pingpong_t *pp = arg;

        while (semaphore_wait(pp->ping, SAFE_TIMEOUT) && !pp->stop) {
                semaphore_signal(pp->pong);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Queue receiver (queue measurement). Items are sequence numbers,
 *         UINT32_MAX finishes transfer.
 *
 * @param  arg          worker (obj: queue, res: 1 on sequence error)
 *
 * @return 0.
 */
//==============================================================================
static int th_queue_sink(void *arg)
{
        worker_t *w = arg;
        u32_t expected = 0;
        u32_t item;

        while (queue_receive(w->obj, &item, SAFE_TIMEOUT) && (item != UINT32_MAX)) {
                if (item != expected) {
                        w->res = 1;
                }
                expected = item + 1;
                w->ops++;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Thread waits for semaphore and returns its value.
 *
 * @param  arg          worker (ctx: semaphore)
 *
 * @return Worker value or -1 if semaphore was not signaled.
 */
//==============================================================================
static int th_wait_sem(void *arg)
{
        worker_t *w = arg;
        return semaphore_wait(w->ctx, 2 * SAFE_TIMEOUT) ? (int)w->value : -1;
}

//==============================================================================
/**
 * @brief  Node of nested thread creation tree.
 *
 * @param  arg          node
 *
 * @return 0.
 */
//==============================================================================
static int th_node(void *arg)
{
        node_run(arg);
        return 0;
}

//==============================================================================
/**
 * @brief  Compute node of tree. Child nodes are computed by new threads, when
 *         thread cannot be created the node is computed by this thread.
 *
 * @param  node         node
 */
//==============================================================================
static void node_run(node_t *node)
{
        node->result = compute(node->value);

        if (node->depth == 0) {
                return;
        }

        /*
         * NOTE: child nodes are allocated, because child thread which does not
         *       finish in time is abandoned (cancel of thread that is joined
         *       by mistake would be dangerous). Abandoned nodes are not freed.
         */
        node_t local[2];
        node_t *child = calloc(2, sizeof(node_t));
        bool allocated = child != NULL;
        bool abandoned = false;
        tid_t tid[2] = {0, 0};
        u32_t seed = node->value | 1;

        if (!allocated) {
                memset(local, 0, sizeof(local));
                child = local;
        }

        for (size_t i = 0; i < 2; i++) {
                child[i].depth = node->depth - 1;
                child[i].value = node->value * 2 + 1 + i;

                const thread_attr_t attr = {
                        .stack_depth = STACK_DEPTH_LOW,
                        .priority = rnd_priority(&seed, PRIORITY_NORMAL - 2, PRIORITY_NORMAL + 2),
                        .detached = false,
                };

                if (allocated) {
                        tid[i] = thread_create(th_node, &attr, &child[i]);
                }

                if (tid[i]) {
                        node->threads++;
                } else {
                        node_run(&child[i]);
                        node->fallbacks++;
                }
        }

        for (size_t i = 0; i < 2; i++) {
                if (tid[i] && (thread_join2(tid[i], NULL, SAFE_TIMEOUT) != 0)) {
                        node->errors++;
                        abandoned = true;
                        continue;
                }

                node->result += child[i].result;
                node->threads += child[i].threads;
                node->fallbacks += child[i].fallbacks;
                node->errors += child[i].errors;
        }

        if (allocated && !abandoned) {
                free(child);
        }
}

//==============================================================================
/**
 * @brief  Expected result of node of tree.
 *
 * @param  depth        node depth
 * @param  value        node value
 *
 * @return Result.
 */
//==============================================================================
static u32_t node_expected(u32_t depth, u32_t value)
{
        u32_t result = compute(value);

        if (depth > 0) {
                result += node_expected(depth - 1, value * 2 + 1);
                result += node_expected(depth - 1, value * 2 + 2);
        }

        return result;
}

//==============================================================================
/**
 * @brief  Victim of cancel test. Thread is blocked or works until canceled.
 *         Thread should never leave its operation (res is set if it does).
 *
 * @param  arg          worker (value: victim kind, ctx: cancel context,
 *                      obj: semaphore of syscall loop)
 *
 * @return 0.
 */
//==============================================================================
static int th_victim(void *arg)
{
        worker_t *w = arg;
        cancel_ctx_t *ctx = w->ctx;

        switch (w->value) {
        case VICTIM_SEMAPHORE:
                semaphore_wait(ctx->sem, MAX_DELAY_MS);
                break;

        case VICTIM_QUEUE: {
                u32_t item;
                queue_receive(ctx->queue, &item, MAX_DELAY_MS);
                break;
        }

        case VICTIM_MUTEX:
                if (mutex_lock(ctx->mutex, MAX_DELAY_MS)) {
                        // mutex is held by main thread
                        w->res |= 1;
                        mutex_unlock(ctx->mutex);
                }
                break;

        case VICTIM_SLEEP:
                msleep(60000);
                break;

        case VICTIM_BUSY: {
                u32_t x = w->seed;
                while (!ctx->stop) {
                        x ^= x << 13;
                        x ^= x >> 17;
                        x ^= x << 5;
                        w->ops++;
                }
                w->sum = x;
                return 0;
        }

        case VICTIM_SYSCALL:
                while (!ctx->stop) {
                        semaphore_signal(w->obj);
                        semaphore_wait(w->obj, 0);
                        get_uptime_ms();
                        w->ops++;
                }
                return 0;

        default:
                break;
        }

        w->res |= 2;

        return 0;
}

//==============================================================================
/**
 * @brief  Heap worker: random heap operations.
 *
 * @param  arg          worker (ctx: heap context)
 *
 * @return 0.
 */
//==============================================================================
static int th_heap_worker(void *arg)
{
        worker_t *w = arg;
        heap_ctx_t *h = w->ctx;

        while (worker_running(w) && heap_step(h)) {
                worker_pace(w);
        }

        heap_release(h);

        return 0;
}

//==============================================================================
/**
 * @brief  Heap producer: allocates blocks and sends them to consumers.
 *
 * @param  arg          worker (obj: queue)
 *
 * @return 0.
 */
//==============================================================================
static int th_heap_producer(void *arg)
{
        worker_t *w = arg;

        while (worker_running(w)) {
                heap_msg_t msg;
                msg.size = rnd_range(&w->seed, 1, 1024);
                msg.seed = rnd(&w->seed);
                msg.ptr = malloc(msg.size);

                if (!msg.ptr) {
                        w->res++;
                        msleep(1);
                        continue;
                }

                pattern_fill(msg.ptr, msg.size, msg.seed);

                bool sent = false;
                while (!sent && worker_running(w)) {
                        sent = queue_send(w->obj, &msg, 10);
                }

                if (!sent) {
                        free(msg.ptr);
                        break;
                }

                w->sum += msg.size;
                worker_pace(w);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Heap consumer: verifies and frees blocks allocated by producers.
 *
 * @param  arg          worker (obj: queue, ctx: done flag)
 *
 * @return 0.
 */
//==============================================================================
static int th_heap_consumer(void *arg)
{
        worker_t *w = arg;
        volatile bool *done = w->ctx;

        while (true) {
                heap_msg_t msg;

                if (queue_receive(w->obj, &msg, 10)) {
                        size_t bad = pattern_check(msg.ptr, msg.size, msg.seed);
                        if (bad < msg.size) {
                                fail_set(w->fail, "block received from other thread corrupted (size, offset)",
                                        msg.size, bad);
                        }

                        free(msg.ptr);
                        w->sum += msg.size;
                        w->ops++;

                } else if (*done) {
                        break;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Try to lock mutex (other thread than mutex owner).
 *
 * @param  arg          worker (obj: mutex, value: timeout, res: 1 if locked,
 *                      sum: lock time)
 *
 * @return 0.
 */
//==============================================================================
static int th_mutex_probe(void *arg)
{
        worker_t *w = arg;
        clock_t tref = get_uptime_ms();

        w->res = mutex_lock(w->obj, w->value) ? 1 : 0;
        w->sum = elapsed_ms(tref);

        if (w->res) {
                mutex_unlock(w->obj);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Lock mutex and hold it until semaphore is signaled.
 *
 * @param  arg          worker (obj: mutex, ctx: semaphore, res: 1 if locked)
 *
 * @return 0.
 */
//==============================================================================
static int th_mutex_holder(void *arg)
{
        worker_t *w = arg;

        if (mutex_lock(w->obj, SAFE_TIMEOUT)) {
                w->res = 1;
                semaphore_wait(w->ctx, 2 * SAFE_TIMEOUT + PERF_STRESS_DURATION);
                mutex_unlock(w->obj);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Try to unlock mutex owned by other thread.
 *
 * @param  arg          worker (obj: mutex, res: 1 if unlocked)
 *
 * @return 0.
 */
//==============================================================================
static int th_mutex_unlock(void *arg)
{
        worker_t *w = arg;
        w->res = mutex_unlock(w->obj) ? 1 : 0;
        return 0;
}

//==============================================================================
/**
 * @brief  Lock object of exclusion test.
 *
 * @param  ctx          test context
 *
 * @return true if locked.
 */
//==============================================================================
static bool lock_acquire(excl_ctx_t *ctx)
{
        switch (ctx->type) {
        case LOCK_MUTEX:
                return mutex_lock(ctx->mtx, SAFE_TIMEOUT);

        case LOCK_MUTEX_REC:
                if (mutex_lock(ctx->mtx, SAFE_TIMEOUT)) {
                        if (mutex_lock(ctx->mtx, 0)) {
                                return true;
                        }
                        mutex_unlock(ctx->mtx);
                }
                return false;

        case LOCK_FUTEX:
                return futex_lock(&ctx->futex, SAFE_TIMEOUT);
        }

        return false;
}

//==============================================================================
/**
 * @brief  Unlock object of exclusion test.
 *
 * @param  ctx          test context
 */
//==============================================================================
static void lock_release(excl_ctx_t *ctx)
{
        switch (ctx->type) {
        case LOCK_MUTEX_REC:
                mutex_unlock(ctx->mtx);
                mutex_unlock(ctx->mtx);
                break;

        case LOCK_MUTEX:
                mutex_unlock(ctx->mtx);
                break;

        case LOCK_FUTEX:
                futex_unlock(&ctx->futex);
                break;
        }
}

//==============================================================================
/**
 * @brief  Exclusion worker: increments shared counter in critical section in
 *         non-atomic way and checks that it is the only owner of section.
 *
 * @param  arg          worker (ctx: exclusion context)
 *
 * @return 0.
 */
//==============================================================================
static int th_exclusion(void *arg)
{
        worker_t *w = arg;
        excl_ctx_t *ctx = w->ctx;

        while (worker_running(w)) {
                if (!lock_acquire(ctx)) {
                        fail_set(w->fail, "lock timeout (worker, operations)", w->id, w->ops);
                        break;
                }

                if (ctx->owner != -1) {
                        fail_set(w->fail, "critical section entered by two threads (owner, intruder)",
                                ctx->owner, w->id);
                }

                ctx->owner = w->id;
                u32_t counter = ctx->counter;

                u32_t r = rnd(&w->seed);
                for (u32_t i = 0; i < (r & 63); i++) {
                        w->sum ^= rnd(&w->seed);
                }

                if (((r >> 8) % 64) == 0) {
                        msleep(1); // other threads wait for lock
                }

                if (ctx->owner != w->id) {
                        fail_set(w->fail, "critical section entered by two threads (owner, intruder)", w->id,
                                ctx->owner);
                }

                ctx->counter = counter + 1;
                ctx->owner = -1;

                lock_release(ctx);
                worker_pace(w);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Semaphore producer: puts items to ring buffer.
 *
 * @param  arg          worker (ctx: producer/consumer context)
 *
 * @return 0.
 */
//==============================================================================
static int th_sem_producer(void *arg)
{
        worker_t *w = arg;
        pc_ctx_t *pc = w->ctx;
        u32_t seq = 0;

        while (worker_running(w)) {
                if (!semaphore_wait(pc->empty, 10)) {
                        continue;
                }

                u32_t item = ((u32_t)w->id << 24) | (seq++ & 0xFFFFFF);

                if (!mutex_lock(pc->mtx, SAFE_TIMEOUT)) {
                        fail_set(w->fail, "lock timeout (producer, items)", w->id, w->ops);
                        break;
                }
                pc->ring[pc->head++ % RING_SIZE] = item;
                mutex_unlock(pc->mtx);

                if (!semaphore_signal(pc->full)) {
                        fail_set(w->fail, "counting semaphore overflow (producer, items)", w->id, w->ops);
                        break;
                }

                w->sum += item;
                worker_pace(w);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Semaphore consumer: takes items from ring buffer.
 *
 * @param  arg          worker (ctx: producer/consumer context)
 *
 * @return 0.
 */
//==============================================================================
static int th_sem_consumer(void *arg)
{
        worker_t *w = arg;
        pc_ctx_t *pc = w->ctx;

        while (!w->fail->set) {
                if (!semaphore_wait(pc->full, 10)) {
                        if (pc->done) {
                                break;
                        }
                        continue;
                }

                if (!mutex_lock(pc->mtx, SAFE_TIMEOUT)) {
                        fail_set(w->fail, "lock timeout (consumer, items)", w->id, w->ops);
                        break;
                }
                u32_t item = pc->ring[pc->tail++ % RING_SIZE];
                mutex_unlock(pc->mtx);

                if (!semaphore_signal(pc->empty)) {
                        fail_set(w->fail, "counting semaphore overflow (consumer, items)", w->id, w->ops);
                        break;
                }

                w->sum += item;
                w->ops++;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Queue producer: sends sequence of items.
 *
 * @param  arg          worker (ctx: producer/consumer context)
 *
 * @return 0.
 */
//==============================================================================
static int th_queue_producer(void *arg)
{
        worker_t *w = arg;
        pc_ctx_t *pc = w->ctx;
        qitem_t item;

        memset(&item, 0, sizeof(item));
        item.producer = w->id;
        item.seq = 1;

        while (worker_running(w)) {
                item.check = (u16_t)(item.seq * 31 + w->id);

                if (!queue_send(pc->queue, &item, rnd(&w->seed) % 3)) {
                        continue;
                }

                w->sum += item.seq;
                item.seq++;
                worker_pace(w);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Queue consumer: receives items and checks their order.
 *
 * @param  arg          worker (ctx: producer/consumer context)
 *
 * @return 0.
 */
//==============================================================================
static int th_queue_consumer(void *arg)
{
        worker_t *w = arg;
        pc_ctx_t *pc = w->ctx;
        u32_t last[MAX_WORKERS];
        qitem_t item;

        memset(last, 0, sizeof(last));

        while (!w->fail->set) {
                if (!queue_receive(pc->queue, &item, 10)) {
                        if (pc->done) {
                                break;
                        }
                        continue;
                }

                if ((item.producer >= MAX_WORKERS) ||
                        (item.check != (u16_t)(item.seq * 31 + item.producer))) {
                        fail_set(w->fail, "corrupted queue item (producer, sequence)", item.producer,
                                item.seq);
                        break;
                }

                if (item.seq <= last[item.producer]) {
                        fail_set(w->fail, "order of items of producer broken (sequence, previous)", item.seq,
                                last[item.producer]);
                        break;
                }

                last[item.producer] = item.seq;
                w->sum += item.seq;
                w->ops++;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Synchronization objects worker.
 *
 * @param  arg          worker (ctx: synchronization context)
 *
 * @return 0.
 */
//==============================================================================
static int th_sync_worker(void *arg)
{
        worker_t *w = arg;
        sync_ctx_t *s = w->ctx;

        while (worker_running(w) && sync_step(s)) {
                worker_pace(w);
        }

        sync_release(s);

        return 0;
}

//==============================================================================
/**
 * @brief  Busy thread (background load).
 *
 * @param  arg          worker
 *
 * @return 0.
 */
//==============================================================================
static int th_busy(void *arg)
{
        worker_t *w = arg;

        while (worker_running(w)) {
                for (int i = 0; i < 256; i++) {
                        w->sum ^= rnd(&w->seed);
                }
                w->ops++;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Thread sleeps and waits for semaphore with short timeouts.
 *
 * @param  arg          worker (ctx: semaphore)
 *
 * @return 0.
 */
//==============================================================================
static int th_sleeper(void *arg)
{
        worker_t *w = arg;

        while (worker_running(w)) {
                msleep(rnd_range(&w->seed, 0, 5));
                semaphore_wait(w->ctx, rnd_range(&w->seed, 0, 5));
                w->ops++;
        }

        return 0;
}

/*==============================================================================
  Performance measurements
==============================================================================*/

//==============================================================================
/**
 * @brief  CPU score: SHA-256 calculation and formatted output (the same
 *         workload as in previous versions of the program).
 *
 * @param  all_cores    0: single thread, 1: one thread per CPU core
 */
//==============================================================================
static void bench_cpu_score(size_t all_cores)
{
        u8_t cores = all_cores ? get_cpu_cores() : 1;
        u8_t started = 0;

        if (all_cores && (cores < 2)) {
                res_printf("skipped (single core CPU)");
                return;
        }

        cpu_job_t *job = calloc(cores, sizeof(cpu_job_t));
        sem_t *start = semaphore_new(cores, 0);

        if (!job || !start) {
                res_printf("not enough free memory");
                goto cleanup;
        }

        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_MEDIUM,
                .priority = PRIORITY_HIGHEST,
                .detached = false,
        };

        for (u8_t i = 0; i < cores; i++) {
                job[i].seed = new_seed();
                job[i].duration = global->bench_duration;
                job[i].start = start;
                job[i].tid = thread_create(th_cpu_score, &attr, &job[i]);
                if (!job[i].tid) {
                        break;
                }

                if (all_cores) {
                        thread_pin_to_core(job[i].tid, 1u << i);
                }

                started++;
        }

        for (u8_t i = 0; i < started; i++) {
                semaphore_signal(start);
        }

        float pt = 0;
        for (u8_t i = 0; i < started; i++) {
                thread_join(job[i].tid, NULL);
                if (job[i].elapsed) {
                        pt += (float)job[i].points * 1000.0f / (float)job[i].elapsed;
                }
        }

        if (started < cores) {
                res_printf("ERROR: thread not created");

        } else if (all_cores) {
                res_printf("%.1f pt (%u cores", (double)pt, (unsigned)cores);
                if (global->cpu_single_pt > 0) {
                        res_printf(", %.2fx single thread", (double)(pt / global->cpu_single_pt));
                }
                res_printf(")");

        } else {
                global->cpu_single_pt = pt;
                res_printf("%.1f pt", (double)pt);
                if (global->cpu_freq) {
                        res_printf(", %.3f pt/MHz", (double)(pt / ((float)global->cpu_freq / 1e6f)));
                }
        }

cleanup:
        semaphore_delete(start);
        free(job);
}

//==============================================================================
/**
 * @brief  Integer arithmetic (xorshift, multiplication).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_int(size_t unused)
{
        UNUSED_ARG1(unused);

        u32_t x = 0x12345678;
        u32_t acc = 0;
        u32_t loops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        do {
                for (int i = 0; i < 1024; i++) {
                        x ^= x << 13;
                        x ^= x >> 17;
                        x ^= x << 5;
                        acc += x * 2654435761u;
                        acc ^= acc >> 7;
                }
                loops++;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        global->sink = acc;

        res_printf("%.2f M iterations/s", (double)loops * 1024.0 / (double)dt / 1000.0);
}

//==============================================================================
/**
 * @brief  Floating point arithmetic (multiply-add chain).
 *
 * @param  dbl          0: float, 1: double
 */
//==============================================================================
static void bench_float(size_t dbl)
{
        u32_t loops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        if (dbl) {
                double a = 1.0001, b = 0.9999, c = 0.5;
                do {
                        for (int i = 0; i < 256; i++) {
                                a = a * b + c;
                                b = b * 0.99999 + 0.00001;
                        }
                        loops++;
                } while ((dt = elapsed_ms(tref)) < global->bench_duration);
                global->sink = (u32_t)a;

        } else {
                float a = 1.0001f, b = 0.9999f, c = 0.5f;
                do {
                        for (int i = 0; i < 256; i++) {
                                a = a * b + c;
                                b = b * 0.99999f + 0.00001f;
                        }
                        loops++;
                } while ((dt = elapsed_ms(tref)) < global->bench_duration);
                global->sink = (u32_t)a;
        }

        // 4 operations per iteration
        res_printf("%.2f M operations/s", (double)loops * 256.0 * 4.0 / (double)dt / 1000.0);
}

//==============================================================================
/**
 * @brief  memcpy() speed.
 *
 * @param  size         block size
 */
//==============================================================================
static void bench_memcpy(size_t size)
{
        u8_t *src = malloc(size);
        u8_t *dst = malloc(size);

        if (src && dst) {
                memset(src, 0x5A, size);

                size_t per_loop = max(1U, 16384 / size);
                u64_t bytes = 0;
                u32_t dt;
                clock_t tref = get_uptime_ms();

                do {
                        for (size_t i = 0; i < per_loop; i++) {
                                memcpy(dst, src, size);
                                BARRIER();
                        }
                        bytes += per_loop * size;
                } while ((dt = elapsed_ms(tref)) < global->bench_duration);

                global->sink = dst[size - 1];
                res_speed(bytes, dt);

        } else {
                res_printf("not enough free memory");
        }

        free(src);
        free(dst);
}

//==============================================================================
/**
 * @brief  memset() speed.
 *
 * @param  size         block size
 */
//==============================================================================
static void bench_memset(size_t size)
{
        u8_t *dst = malloc(size);

        if (dst) {
                size_t per_loop = max(1U, 16384 / size);
                u64_t bytes = 0;
                u32_t dt;
                clock_t tref = get_uptime_ms();

                do {
                        for (size_t i = 0; i < per_loop; i++) {
                                memset(dst, i, size);
                                BARRIER();
                        }
                        bytes += per_loop * size;
                } while ((dt = elapsed_ms(tref)) < global->bench_duration);

                global->sink = dst[size - 1];
                res_speed(bytes, dt);

        } else {
                res_printf("not enough free memory");
        }

        free(dst);
}

//==============================================================================
/**
 * @brief  malloc() + free() rate.
 *
 * @param  size         block size
 */
//==============================================================================
static void bench_malloc(size_t size)
{
        u32_t ops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        do {
                for (int i = 0; i < 64; i++) {
                        volatile u8_t *ptr = malloc(size);
                        if (!ptr) {
                                res_printf("ERROR: allocation failed");
                                return;
                        }
                        ptr[0] = i;
                        free((void *)ptr);
                }
                ops += 64;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        res_rate(ops, dt, "ops");
}

//==============================================================================
/**
 * @brief  Syscall rate (getpid(), get_uptime_ms() is handled by fast path).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_syscall(size_t unused)
{
        UNUSED_ARG1(unused);

        u32_t ops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        do {
                for (int i = 0; i < 64; i++) {
                        global->sink += getpid();
                }
                ops += 64;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        res_rate(ops, dt, "calls");
}

//==============================================================================
/**
 * @brief  Thread create and join rate.
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_thread(size_t unused)
{
        UNUSED_ARG1(unused);

        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        u32_t ops = 0;
        u32_t dt = 0;
        clock_t tstart = get_uptime_ms();

        /*
         * NOTE: measurement is finished by wall time. Burst can take less than
         *       1 ms, so sum of burst times would never reach the duration on
         *       fast CPU. Sum of measured burst times is the measured time,
         *       errors of 1 ms resolution are averaged.
         */
        while (elapsed_ms(tstart) < global->bench_duration) {
                clock_t tref = get_uptime_ms();

                for (int i = 0; i < THREAD_BURST; i++) {
                        tid_t tid = thread_create(th_noop, &attr, NULL);
                        if (!tid || (thread_join2(tid, NULL, SAFE_TIMEOUT) != 0)) {
                                res_printf("ERROR: %s (after %u threads)", strerror(errno), (unsigned)ops);
                                return;
                        }
                        ops++;
                }

                dt += elapsed_ms(tref);

                // memory of finished threads can be released when CPU is idle (not measured)
                msleep(1);
        }

        res_rate(ops, max(1U, dt), "threads");
}

//==============================================================================
/**
 * @brief  Context switch rate (2 threads, ping-pong with semaphores).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_ctx_switch(size_t unused)
{
        UNUSED_ARG1(unused);

        pingpong_t pp;
        memset(&pp, 0, sizeof(pp));
        pp.ping = semaphore_new(1, 0);
        pp.pong = semaphore_new(1, 0);

        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        tid_t tid = 0;
        u32_t rounds = 0;
        u32_t dt = 0;

        if (!pp.ping || !pp.pong || !(tid = thread_create(th_pong, &attr, &pp))) {
                res_printf("ERROR: %s", strerror(errno));
                goto cleanup;
        }

        clock_t tref = get_uptime_ms();

        do {
                semaphore_signal(pp.ping);
                if (!semaphore_wait(pp.pong, SAFE_TIMEOUT)) {
                        res_printf("ERROR: no response");
                        goto cleanup;
                }
                rounds++;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        // 2 context switches per round
        res_rate(rounds * 2, dt, "switches");

cleanup:
        pp.stop = true;
        if (tid) {
                semaphore_signal(pp.ping);
                thread_join_or_cancel(tid, SAFE_TIMEOUT);
        }
        semaphore_delete(pp.ping);
        semaphore_delete(pp.pong);
}

//==============================================================================
/**
 * @brief  Mutex lock and unlock rate (not contended).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_mutex(size_t unused)
{
        UNUSED_ARG1(unused);

        mutex_t *mtx = mutex_new(MUTEX_TYPE_NORMAL);
        if (!mtx) {
                res_printf("ERROR: %s", strerror(errno));
                return;
        }

        u32_t ops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        do {
                for (int i = 0; i < 64; i++) {
                        mutex_lock(mtx, 0);
                        mutex_unlock(mtx);
                }
                ops += 64;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        res_rate(ops, dt, "lock/unlock");

        mutex_delete(mtx);
}

//==============================================================================
/**
 * @brief  Queue transfer rate (sender and receiver threads).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_queue(size_t unused)
{
        UNUSED_ARG1(unused);

        worker_t w;
        u32_t item = 0;
        u32_t dt = 0;

        memset(&w, 0, sizeof(w));
        w.obj = queue_new(QUEUE_LEN, sizeof(u32_t));

        if (!w.obj || !worker_start(&w, th_queue_sink, PRIORITY_NORMAL, STACK_DEPTH_LOW)) {
                res_printf("ERROR: %s", strerror(errno));
                goto cleanup;
        }

        clock_t tref = get_uptime_ms();

        do {
                if (!queue_send(w.obj, &item, SAFE_TIMEOUT)) {
                        res_printf("ERROR: send timeout");
                        goto cleanup;
                }
                item++;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        res_rate(item, dt, "items");

cleanup:
        if (w.tid) {
                const u32_t end = UINT32_MAX;
                queue_send(w.obj, &end, SAFE_TIMEOUT);
                thread_join_or_cancel(w.tid, SAFE_TIMEOUT);

                if (w.res) {
                        global->res_buf[0] = '\0';
                        res_printf("ERROR: incorrect order of items");
                } else if (w.ops != item) {
                        global->res_buf[0] = '\0';
                        res_printf("ERROR: %u items sent, %u received", (unsigned)item, (unsigned)w.ops);
                }
        }

        queue_delete(w.obj);
}

//==============================================================================
/**
 * @brief  Process create and wait rate (this program in child mode).
 *
 * @param  unused       not used
 */
//==============================================================================
static void bench_process(size_t unused)
{
        UNUSED_ARG1(unused);

        u32_t ops = 0;
        u32_t dt;
        clock_t tref = get_uptime_ms();

        do {
                pid_t pid = spawn_child("exit 0");
                if (!pid) {
                        res_printf("ERROR: %s", strerror(errno));
                        return;
                }

                int status = -1;
                if ((process_wait(pid, &status, SAFE_TIMEOUT) != 0) || (status != 0)) {
                        res_printf("ERROR: wait failed (status %d)", status);
                        return;
                }

                ops++;
        } while ((dt = elapsed_ms(tref)) < global->bench_duration);

        res_rate(ops, dt, "processes");

        /*
         * NOTE: if one cycle takes exactly one tick, then the measurement is
         *       limited by sleep in the kernel (e.g. process_wait() of older
         *       kernel sleeps one tick), not by the cost of process.
         */
        float cycle_us = (float)dt * 1000.0f / (float)ops;
        float tick_us = measure_tick_us();

        if ((tick_us > 0) && (cycle_us >= tick_us * 0.97f) && (cycle_us <= tick_us * 1.03f)) {
                res_printf(" (limited by system tick)");
        }
}

//==============================================================================
/**
 * @brief  Measure period of system tick (msleep(1) waits for the next tick,
 *         so series of sleeps takes one tick each).
 *
 * @return Tick period [us] or 0 if not measured.
 */
//==============================================================================
static float measure_tick_us(void)
{
        enum {
                SLEEPS = 20
        };

        msleep(1); // synchronize with tick

        clock_t tref = get_uptime_ms();
        for (int i = 0; i < SLEEPS; i++) {
                msleep(1);
        }
        u32_t dt = elapsed_ms(tref);

        return (dt > 0) ? (float)dt * 1000.0f / SLEEPS : 0;
}

//==============================================================================
/**
 * @brief  Create threads with random priorities in batches and join them in
 *         random order. Exit status of each thread is checked. Thread ID of
 *         thread that is not joined yet must not be assigned to new thread.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_thread_create_join(void)
{
        const char *exit_msg = SUCCESS_MSG;
        tid_t tid[MAX_WORKERS];
        u32_t in[MAX_WORKERS];
        u8_t order[MAX_WORKERS];
        u32_t seed = new_seed();
        u32_t total = 0;
        u32_t rounds = 0;

        memset(tid, 0, sizeof(tid));

        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                size_t n = rnd_range(&seed, 1, MAX_WORKERS);

                for (size_t i = 0; i < n; i++) {
                        in[i] = rnd(&seed);
                        order[i] = i;

                        const thread_attr_t attr = {
                                .stack_depth = STACK_DEPTH_LOW,
                                .priority = rnd_priority(&seed, PRIORITY_LOWEST, PRIORITY_HIGHEST),
                                .detached = false,
                        };

                        tid[i] = thread_create(th_compute, &attr, (void *)(uintptr_t)in[i]);
                        CHECK(tid[i] != 0, "round %u: thread %u not created", (unsigned)rounds, (unsigned)i);

                        for (size_t j = 0; j < i; j++) {
                                if (tid[j] == tid[i]) {
                                        tid[i] = 0; // the same thread
                                        CHECK(false,
                                                "round %u: thread ID %u assigned to new thread before join "
                                                "of finished thread (priority %d)",
                                                (unsigned)rounds, (unsigned)tid[j], attr.priority);
                                }
                        }
                }

                shuffle(order, n, &seed);

                for (size_t k = 0; k < n; k++) {
                        size_t i = order[k];
                        int status = -1;

                        CHECK(thread_join2(tid[i], &status, SAFE_TIMEOUT) == 0,
                                "round %u: join of thread %u failed", (unsigned)rounds, (unsigned)tid[i]);

                        tid[i] = 0;

                        CHECK((u32_t)status == compute(in[i]), "round %u: thread exit status %d, expected %d",
                                (unsigned)rounds, status, (int)compute(in[i]));
                }

                total += n;
                rounds++;

                // memory of finished threads can be released when CPU is idle (separate test)
                msleep(1);
        }

        info_printf("%u threads in %u rounds", (unsigned)total, (unsigned)rounds);

cleanup:
        for (size_t i = 0; i < MAX_WORKERS; i++) {
                if (tid[i]) {
                        thread_join_or_cancel(tid[i], SAFE_TIMEOUT);
                }
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Threads are created and joined without pause. Memory of finished
 *         threads must be released, otherwise the system memory is exhausted
 *         by program that starts short threads in loop. Test is stopped
 *         before memory exhaustion.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_thread_memory(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        memstat_t stat;
        if (get_memory_usage_details(&stat) != 0) {
                return SKIP("memory statistics not available");
        }

        size_t ref = stat.used_memory;
        size_t limit = max(8U * 1024, min(THREAD_MEM_LIMIT, stat.free_memory / 8));
        size_t peak = 0;
        u32_t threads = 0;
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                tid_t tid = thread_create(th_noop, &attr, NULL);
                CHECK(tid, "thread not created after %u threads", (unsigned)threads);
                CHECK(thread_join2(tid, NULL, SAFE_TIMEOUT) == 0, "join of thread failed");
                threads++;

                memstat_t now;
                sys_mem_get(&now);
                size_t used = now.used_memory;
                size_t growth = (used > ref) ? (used - ref) : 0;
                peak = max(peak, growth);

                CHECK(growth <= limit,
                        "memory of finished threads not released: +%u B after %u threads created and "
                        "joined without pause (released only when CPU is idle?)",
                        (unsigned)growth, (unsigned)threads);
        }

        info_printf("%u threads, max +%u B", (unsigned)threads, (unsigned)peak);

cleanup:
        // let the system release memory of finished threads
        msleep(10);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Create blocked threads until thread creation fails (thread slots or
 *         memory exhausted). Then threads are released and joined. The same
 *         number of threads must be created in each round (no leak of thread
 *         slots or memory).
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_thread_exhaustion(void)
{
        const char *exit_msg = SUCCESS_MSG;
        worker_t *w = calloc(MAX_THREADS_PROBE, sizeof(worker_t));
        sem_t *sem = semaphore_new(MAX_THREADS_PROBE, 0);
        size_t n = 0;
        size_t first = 0;
        int first_err = 0;

        CHECK(w && sem, "not enough free memory");

        for (u32_t round = 0; round < 3; round++) {
                int err = 0;

                for (n = 0; n < MAX_THREADS_PROBE; n++) {
                        worker_init(&w[n], n, NULL, sem, 0);
                        w[n].value = round * 100 + n;

                        errno = 0;
                        if (!worker_start(&w[n], th_wait_sem, PRIORITY_NORMAL, STACK_DEPTH_VERY_LOW)) {
                                err = errno;
                                break;
                        }
                }

                if (round == 0) {
                        first = n;
                        first_err = err;
                }

                for (size_t i = 0; i < n; i++) {
                        semaphore_signal(sem);
                }

                for (size_t i = 0; i < n; i++) {
                        int status = -1;
                        CHECK(thread_join2(w[i].tid, &status, SAFE_TIMEOUT) == 0,
                                "round %u: join of thread %u failed", (unsigned)round, (unsigned)i);

                        w[i].tid = 0;

                        CHECK(status == (int)w[i].value, "round %u: thread %u exit status %d, expected %d",
                                (unsigned)round, (unsigned)i, status, (int)w[i].value);
                }

                CHECK(n > 0, "no thread created");
                CHECK(n == first, "round %u: %u threads created, %u in the first round", (unsigned)round,
                        (unsigned)n, (unsigned)first);

                n = 0;
        }

        if (first < MAX_THREADS_PROBE) {
                info_printf("%u threads at once, then: %s", (unsigned)first, strerror(first_err));
        } else {
                info_printf("limit not reached, %u threads at once", (unsigned)first);
        }

cleanup:
        if (w) {
                for (size_t i = 0; i < n; i++) {
                        semaphore_signal(sem);
                }
                workers_join(w, MAX_THREADS_PROBE, SAFE_TIMEOUT);
        }

        semaphore_delete(sem);
        free(w);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Threads create threads (binary tree). Result of tree is checked.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_thread_nested(void)
{
        const char *exit_msg = SUCCESS_MSG;
        u32_t seed = new_seed();
        u32_t threads = 0;
        u32_t fallbacks = 0;
        u32_t trees = 0;
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                node_t root;
                memset(&root, 0, sizeof(root));
                root.depth = NESTED_DEPTH;
                root.value = rnd(&seed) & 0xFFFFFF;

                node_run(&root);

                CHECK(root.errors == 0, "tree %u: %u threads not finished in time", (unsigned)trees,
                        (unsigned)root.errors);

                u32_t expected = node_expected(NESTED_DEPTH, root.value);
                CHECK(root.result == expected, "tree %u: result 0x%X, expected 0x%X (%u threads)",
                        (unsigned)trees, (unsigned)root.result, (unsigned)expected, (unsigned)root.threads);

                threads += root.threads;
                fallbacks += root.fallbacks;
                trees++;

                // memory of finished threads can be released when CPU is idle (separate test)
                msleep(1);
        }

        info_printf("%u trees, %u threads, %u nodes without thread", (unsigned)trees, (unsigned)threads,
                (unsigned)fallbacks);

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Cancel threads blocked on synchronization objects, sleeping, busy
 *         and in syscall loop at random moment. Canceled thread must not
 *         continue its work, objects must work correctly after cancel.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_thread_cancel(void)
{
        const char *exit_msg = SUCCESS_MSG;
        cancel_ctx_t ctx;
        worker_t w[VICTIM_COUNT];
        worker_t probe;
        thread_stat_t tstat;
        u8_t order[VICTIM_COUNT];
        u32_t seed = new_seed();
        u32_t rounds = 0;
        bool locked = false;

        memset(&ctx, 0, sizeof(ctx));
        memset(w, 0, sizeof(w));
        memset(&probe, 0, sizeof(probe));

        ctx.sem = semaphore_new(1, 0);
        ctx.queue = queue_new(2, sizeof(u32_t));
        ctx.mutex = mutex_new(MUTEX_TYPE_NORMAL);
        sem_t *own_sem = semaphore_new(1, 0);

        CHECK(ctx.sem && ctx.queue && ctx.mutex && own_sem, "unable to create objects");

        CHECK(thread_stat(getpid(), thread_current(), &tstat) == 0, "thread_stat() failed");
        i16_t own_priority = tstat.priority;

        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                CHECK(mutex_lock(ctx.mutex, 0), "round %u: unable to lock mutex", (unsigned)rounds);
                locked = true;

                for (size_t i = 0; i < VICTIM_COUNT; i++) {
                        worker_init(&w[i], i, NULL, &ctx, 0);
                        w[i].value = i;
                        w[i].obj = own_sem;
                        order[i] = i;

                        // busy threads with high priority would starve main thread
                        i16_t prio = ((i == VICTIM_BUSY) || (i == VICTIM_SYSCALL))
                                ? rnd_priority(&seed, PRIORITY_LOWEST, PRIORITY_NORMAL)
                                : rnd_priority(&seed, PRIORITY_LOWEST, PRIORITY_HIGHEST);

                        CHECK(worker_start(&w[i], th_victim, prio, STACK_DEPTH_LOW),
                                "round %u: %s thread not created", (unsigned)rounds, VICTIM_NAME[i]);
                }

                u32_t delay = rnd_range(&seed, 0, 4);
                if (delay) {
                        msleep(delay);
                }

                shuffle(order, VICTIM_COUNT, &seed);

                for (size_t k = 0; k < VICTIM_COUNT; k++) {
                        worker_t *v = &w[order[k]];

                        CHECK(thread_cancel(v->tid) == 0, "round %u: cancel of %s thread failed",
                                (unsigned)rounds, VICTIM_NAME[v->value]);

                        clock_t tcancel = get_uptime_ms();
                        CHECK(thread_join2(v->tid, NULL, CANCEL_TIMEOUT) == 0,
                                "round %u: %s thread not finished %u ms after cancel", (unsigned)rounds,
                                VICTIM_NAME[v->value], (unsigned)elapsed_ms(tcancel));

                        v->tid = 0;

                        CHECK(v->res == 0, "round %u: %s thread continued after cancel (%u)",
                                (unsigned)rounds, VICTIM_NAME[v->value], (unsigned)v->res);
                }

                // objects must work correctly after cancel of waiting threads
                CHECK(semaphore_get_value(ctx.sem) == 0, "round %u: semaphore value %d, expected 0",
                        (unsigned)rounds, semaphore_get_value(ctx.sem));
                CHECK(semaphore_signal(ctx.sem) && semaphore_wait(ctx.sem, 0),
                        "round %u: semaphore does not work after cancel of waiting thread", (unsigned)rounds);

                u32_t item = rounds;
                u32_t back = ~rounds;
                CHECK(queue_get_number_of_items(ctx.queue) == 0, "round %u: queue is not empty",
                        (unsigned)rounds);
                CHECK(queue_send(ctx.queue, &item, 0) && queue_receive(ctx.queue, &back, 0) && (item == back),
                        "round %u: queue does not work after cancel of waiting thread", (unsigned)rounds);

                semaphore_wait(own_sem, 0);
                CHECK(semaphore_signal(own_sem) && semaphore_wait(own_sem, 0),
                        "round %u: semaphore does not work after cancel of thread in syscall loop",
                        (unsigned)rounds);

                CHECK(mutex_unlock(ctx.mutex), "round %u: unable to unlock mutex", (unsigned)rounds);
                locked = false;

                CHECK(thread_stat(getpid(), thread_current(), &tstat) == 0, "thread_stat() failed");
                CHECK(tstat.priority == own_priority,
                        "round %u: priority of mutex owner after unlock %d, expected %d (inheritance not "
                        "restored)",
                        (unsigned)rounds, tstat.priority, own_priority);

                // released mutex must be available for other threads
                worker_init(&probe, 0, NULL, NULL, 0);
                probe.obj = ctx.mutex;
                probe.value = 100;
                CHECK(worker_start(&probe, th_mutex_probe, PRIORITY_NORMAL, STACK_DEPTH_LOW),
                        "probe thread not created");
                CHECK(thread_join2(probe.tid, NULL, SAFE_TIMEOUT) == 0, "probe thread not finished");
                probe.tid = 0;
                CHECK(probe.res == 1, "round %u: released mutex cannot be locked by other thread",
                        (unsigned)rounds);

                rounds++;
        }

        info_printf("%u rounds, %u threads canceled", (unsigned)rounds, (unsigned)(rounds * VICTIM_COUNT));

cleanup:
        ctx.stop = true;

        if (locked) {
                mutex_unlock(ctx.mutex);
        }

        // let blocked threads finish
        semaphore_signal(ctx.sem);
        for (size_t i = 0; i < VICTIM_COUNT; i++) {
                u32_t item = 0;
                queue_send(ctx.queue, &item, 0);
        }

        workers_join(w, VICTIM_COUNT, SAFE_TIMEOUT);
        workers_join(&probe, 1, SAFE_TIMEOUT);

        semaphore_delete(ctx.sem);
        semaphore_delete(own_sem);
        queue_delete(ctx.queue);
        mutex_delete(ctx.mutex);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Random heap operations of single thread with heap consistency
 *         checks.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_heap_single(void)
{
        const char *exit_msg = SUCCESS_MSG;
        fail_t fail;
        fail_init(&fail);

        heap_ctx_t *h = heap_ctx_new(&fail, heap_budget(1), true);
        CHECK(h, "not enough free memory");

        clock_t tref = get_uptime_ms();

        while ((elapsed_ms(tref) < global->stress_duration) && heap_step(h)) {
        }

        heap_release(h);
        CHECK_FAIL(&fail);
        CHECK(check_heap_consistency() == 0, "heap inconsistent after release of blocks");

        info_printf("%u operations, %u allocations, %u failed (budget %u B)", (unsigned)h->ops,
                (unsigned)h->allocs, (unsigned)h->nomem, (unsigned)h->budget);

cleanup:
        if (h) {
                heap_release(h);
                free(h);
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Random heap operations of several threads with different priorities
 *         (shared program heap). Main thread checks heap consistency.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_heap_concurrent(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                WORKERS = 4
        };
        worker_t w[WORKERS];
        heap_ctx_t *h[WORKERS];
        fail_t fail;
        u32_t checks = 0;

        fail_init(&fail);
        memset(w, 0, sizeof(w));
        memset(h, 0, sizeof(h));

        size_t budget = heap_budget(WORKERS);

        for (size_t i = 0; i < WORKERS; i++) {
                h[i] = heap_ctx_new(&fail, budget, false);
                CHECK(h[i], "not enough free memory");
        }

        for (size_t i = 0; i < WORKERS; i++) {
                worker_init(&w[i], i, &fail, h[i], global->stress_duration);
                CHECK(worker_start(&w[i], th_heap_worker, WORKER_PRIORITY[i], STACK_DEPTH_LOW),
                        "worker %u not created", (unsigned)i);
        }

        clock_t tref = get_uptime_ms();
        while ((elapsed_ms(tref) < global->stress_duration) && !fail.set) {
                msleep(10);
                CHECK(check_heap_consistency() == 0, "heap inconsistent during concurrent operations");
                checks++;
        }

        CHECK(workers_join(w, WORKERS, SAFE_TIMEOUT), "worker not finished in time");
        CHECK_FAIL(&fail);
        CHECK(check_heap_consistency() == 0, "heap inconsistent after release of blocks");

        u32_t ops = 0;
        for (size_t i = 0; i < WORKERS; i++) {
                ops += h[i]->ops;
        }
        info_printf("%u operations, %u consistency checks", (unsigned)ops, (unsigned)checks);

cleanup:
        workers_join(w, WORKERS, SAFE_TIMEOUT);

        for (size_t i = 0; i < WORKERS; i++) {
                if (h[i]) {
                        heap_release(h[i]);
                        free(h[i]);
                }
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Blocks allocated by producer threads are verified and freed by
 *         consumer threads.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_heap_cross(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                PRODUCERS = 2,
                CONSUMERS = 2
        };
        worker_t prod[PRODUCERS];
        worker_t cons[CONSUMERS];
        volatile bool done = false;
        queue_t *queue = queue_new(QUEUE_LEN, sizeof(heap_msg_t));
        fail_t fail;

        fail_init(&fail);
        memset(prod, 0, sizeof(prod));
        memset(cons, 0, sizeof(cons));

        CHECK(queue, "unable to create queue");

        for (size_t i = 0; i < CONSUMERS; i++) {
                worker_init(&cons[i], i, &fail, (void *)&done, 0);
                cons[i].obj = queue;
                CHECK(worker_start(&cons[i], th_heap_consumer, WORKER_PRIORITY[i + 1], STACK_DEPTH_LOW),
                        "consumer not created");
        }

        for (size_t i = 0; i < PRODUCERS; i++) {
                worker_init(&prod[i], i, &fail, NULL, global->stress_duration);
                prod[i].obj = queue;
                CHECK(worker_start(&prod[i], th_heap_producer, WORKER_PRIORITY[i * 3], STACK_DEPTH_LOW),
                        "producer not created");
        }

        CHECK(workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT), "producer not finished");
        done = true;
        CHECK(workers_join(cons, CONSUMERS, SAFE_TIMEOUT), "consumer not finished");
        CHECK_FAIL(&fail);

        u32_t produced = 0, consumed = 0, bytes_p = 0, bytes_c = 0;
        for (size_t i = 0; i < PRODUCERS; i++) {
                produced += prod[i].ops;
                bytes_p += prod[i].sum;
        }
        for (size_t i = 0; i < CONSUMERS; i++) {
                consumed += cons[i].ops;
                bytes_c += cons[i].sum;
        }

        CHECK((produced == consumed) && (bytes_p == bytes_c), "%u blocks (%u B) produced, %u (%u B) consumed",
                (unsigned)produced, (unsigned)bytes_p, (unsigned)consumed, (unsigned)bytes_c);
        CHECK(check_heap_consistency() == 0, "heap inconsistent");

        info_printf("%u blocks, %u KiB", (unsigned)produced, (unsigned)(bytes_p / 1024));

cleanup:
        done = true;
        workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT);
        workers_join(cons, CONSUMERS, SAFE_TIMEOUT);

        if (queue) {
                heap_msg_t msg;
                while (queue_receive(queue, &msg, 0)) {
                        free(msg.ptr);
                }
                queue_delete(queue);
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Allocate memory until allocation fails, check system behavior in
 *         out of memory state, free memory and check recovery.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_heap_exhaustion(void)
{
        typedef struct blk {
                struct blk *next;
                u32_t size;
                u32_t seed;
        } blk_t;

        const char *exit_msg = SUCCESS_MSG;
        blk_t *head = NULL;
        size_t size = HEAP_BLOCK_MAX;
        u32_t blocks = 0;
        u32_t total = 0;
        u32_t seed = new_seed();
        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < 10000) {
                blk_t *b = malloc(size);
                if (!b) {
                        if (size <= 2 * sizeof(blk_t)) {
                                break;
                        }
                        size /= 2;
                        continue;
                }

                b->next = head;
                b->size = size;
                b->seed = rnd(&seed);
                pattern_fill((u8_t *)(b + 1), size - sizeof(blk_t), b->seed);
                head = b;
                blocks++;
                total += size;
        }

        // out of memory state: operations must fail cleanly
        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        int thread_err = 0;
        errno = 0;
        tid_t tid = thread_create(th_compute, &attr, (void *)1);
        if (tid) {
                int status = -1;
                thread_join2(tid, &status, SAFE_TIMEOUT);
                thread_err = (status == (int)compute(1)) ? 0 : -1;
        } else {
                thread_err = errno;
        }

        sem_t *sem = semaphore_new(1, 0);
        semaphore_delete(sem);

        CHECK(check_heap_consistency() == 0, "heap inconsistent in out of memory state");

        // release
        while (head) {
                blk_t *b = head;
                head = b->next;

                u32_t len = b->size - sizeof(blk_t);
                size_t bad = pattern_check((u8_t *)(b + 1), len, b->seed);
                free(b);

                CHECK(bad == len, "content of block corrupted (size %u, offset %u)", (unsigned)len,
                        (unsigned)bad);
        }

        CHECK(thread_err != -1, "thread created in out of memory state returned incorrect status");

        void *ptr = malloc(HEAP_BLOCK_MAX);
        CHECK(ptr, "memory not available after release");
        free(ptr);

        info_printf("%u KiB in %u blocks, thread: %s", (unsigned)(total / 1024), (unsigned)blocks,
                thread_err ? strerror(thread_err) : "created");

cleanup:
        while (head) {
                blk_t *b = head;
                head = b->next;
                free(b);
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Run mutex probe thread and wait for its result.
 *
 * @param  probe        probe worker
 * @param  mtx          mutex
 * @param  timeout      lock timeout
 *
 * @return true if probe was executed.
 */
//==============================================================================
static bool mutex_probe(worker_t *probe, mutex_t *mtx, u32_t timeout)
{
        worker_init(probe, 0, NULL, NULL, 0);
        probe->obj = mtx;
        probe->value = timeout;

        if (!worker_start(probe, th_mutex_probe, PRIORITY_NORMAL, STACK_DEPTH_LOW)) {
                return false;
        }

        if (thread_join2(probe->tid, NULL, SAFE_TIMEOUT) != 0) {
                return false;
        }

        probe->tid = 0;

        return true;
}

//==============================================================================
/**
 * @brief  Functional test of mutexes: recursion, trylock by other thread,
 *         lock timeout, unlock of not locked mutex.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_mutex_functional(void)
{
        const char *exit_msg = SUCCESS_MSG;
        mutex_t *rec = mutex_new(MUTEX_TYPE_RECURSIVE);
        mutex_t *nor = mutex_new(MUTEX_TYPE_NORMAL);
        worker_t probe;
        u32_t rec_depth = 0;
        bool nor_locked = false;

        memset(&probe, 0, sizeof(probe));

        CHECK(rec && nor, "unable to create mutexes");

        // recursive mutex
        for (int i = 0; i < 5; i++) {
                CHECK(mutex_lock(rec, 0), "recursive mutex: lock %d failed", i + 1);
                rec_depth++;
        }

        CHECK(mutex_probe(&probe, rec, 0), "probe thread failed");
        CHECK(probe.res == 0, "recursive mutex locked by other thread (depth 5)");

        for (int i = 0; i < 4; i++) {
                CHECK(mutex_unlock(rec), "recursive mutex: unlock %d failed", i + 1);
                rec_depth--;
        }

        CHECK(mutex_probe(&probe, rec, 0), "probe thread failed");
        CHECK(probe.res == 0, "recursive mutex locked by other thread (depth 1)");

        CHECK(mutex_unlock(rec), "recursive mutex: last unlock failed");
        rec_depth--;

        CHECK(mutex_probe(&probe, rec, 0), "probe thread failed");
        CHECK(probe.res == 1, "released recursive mutex cannot be locked by other thread");

        CHECK(!mutex_unlock(rec), "unlock of not locked recursive mutex succeeded");

        // normal mutex
        CHECK(mutex_lock(nor, 0), "normal mutex: lock failed");
        nor_locked = true;

        CHECK(!mutex_trylock(nor), "normal mutex locked twice by the same thread");

        clock_t tref = get_uptime_ms();
        CHECK(!mutex_lock(nor, 50), "normal mutex locked twice by the same thread (timeout)");
        u32_t dt = elapsed_ms(tref);
        CHECK(time_ok(dt, 50), "lock timeout 50 ms took %u ms", (unsigned)dt);

        CHECK(mutex_probe(&probe, nor, 30), "probe thread failed");
        CHECK(probe.res == 0, "locked normal mutex locked by other thread");
        CHECK(time_ok(probe.sum, 30), "lock timeout 30 ms took %u ms (other thread)", (unsigned)probe.sum);

        CHECK(mutex_unlock(nor), "normal mutex: unlock failed");
        nor_locked = false;

        CHECK(mutex_probe(&probe, nor, 0), "probe thread failed");
        CHECK(probe.res == 1, "released normal mutex cannot be locked by other thread");

        CHECK(!mutex_unlock(nor), "unlock of not locked normal mutex succeeded");

cleanup:
        workers_join(&probe, 1, SAFE_TIMEOUT);

        while (rec_depth--) {
                mutex_unlock(rec);
        }

        if (nor_locked) {
                mutex_unlock(nor);
        }

        mutex_delete(rec);
        mutex_delete(nor);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Common part of mutual exclusion tests.
 *
 * @param  type         lock type
 *
 * @return Test result.
 */
//==============================================================================
static const char *exclusion_test(lock_type_t type)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                WORKERS = 5
        };
        worker_t w[WORKERS];
        excl_ctx_t ctx;
        fail_t fail;

        fail_init(&fail);
        memset(w, 0, sizeof(w));
        memset(&ctx, 0, sizeof(ctx));

        ctx.type = type;
        ctx.owner = -1;
        futex_init(&ctx.futex);

        if (type != LOCK_FUTEX) {
                ctx.mtx = mutex_new(type == LOCK_MUTEX ? MUTEX_TYPE_NORMAL : MUTEX_TYPE_RECURSIVE);
                CHECK(ctx.mtx, "unable to create mutex");
        }

        for (size_t i = 0; i < WORKERS; i++) {
                worker_init(&w[i], i, &fail, &ctx, global->stress_duration);
                CHECK(worker_start(&w[i], th_exclusion, WORKER_PRIORITY[i], STACK_DEPTH_LOW),
                        "worker %u not created", (unsigned)i);
        }

        CHECK(workers_join(w, WORKERS, global->stress_duration + SAFE_TIMEOUT),
                "worker not finished in time");
        CHECK_FAIL(&fail);

        u32_t ops = 0;
        for (size_t i = 0; i < WORKERS; i++) {
                ops += w[i].ops;
        }

        CHECK(ctx.counter == ops, "counter %u, expected %u (lost updates)", (unsigned)ctx.counter,
                (unsigned)ops);

        info_printf("%u locks (%u/%u/%u/%u/%u)", (unsigned)ops, (unsigned)w[0].ops, (unsigned)w[1].ops,
                (unsigned)w[2].ops, (unsigned)w[3].ops, (unsigned)w[4].ops);

cleanup:
        workers_join(w, WORKERS, SAFE_TIMEOUT);
        mutex_delete(ctx.mtx);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Mutual exclusion by normal mutex.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_mutex_exclusion(void)
{
        return exclusion_test(LOCK_MUTEX);
}

//==============================================================================
/**
 * @brief  Mutual exclusion by recursive mutex (locked twice).
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_mutex_rec_exclusion(void)
{
        return exclusion_test(LOCK_MUTEX_REC);
}

//==============================================================================
/**
 * @brief  Mutual exclusion by futex.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_futex_exclusion(void)
{
        return exclusion_test(LOCK_FUTEX);
}

//==============================================================================
/**
 * @brief  Thread that does not own mutex tries to unlock it. Operation must
 *         fail and mutex must stay locked. Test is executed only with -x
 *         option, because kernel can stop on assertion.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_mutex_non_owner(void)
{
        const char *exit_msg = SUCCESS_MSG;
        mutex_t *mtx = mutex_new(MUTEX_TYPE_NORMAL);
        bool locked = false;
        worker_t w;

        memset(&w, 0, sizeof(w));

        CHECK(mtx, "unable to create mutex");
        CHECK(mutex_lock(mtx, 0), "unable to lock mutex");
        locked = true;

        worker_init(&w, 0, NULL, NULL, 0);
        w.obj = mtx;
        CHECK(worker_start(&w, th_mutex_unlock, PRIORITY_NORMAL, STACK_DEPTH_LOW), "thread not created");
        CHECK(thread_join2(w.tid, NULL, SAFE_TIMEOUT) == 0, "thread not finished");
        w.tid = 0;

        CHECK(w.res == 0, "mutex unlocked by thread which does not own it");

        CHECK(mutex_probe(&w, mtx, 0), "probe thread failed");
        CHECK(w.res == 0, "mutex locked by other thread after unlock attempt of non-owner");

        CHECK(mutex_unlock(mtx), "owner is not able to unlock mutex");
        locked = false;

cleanup:
        workers_join(&w, 1, SAFE_TIMEOUT);

        if (locked) {
                mutex_unlock(mtx);
        }

        mutex_delete(mtx);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Producers and consumers synchronized by counting semaphores (ring
 *         buffer). No item can be lost or duplicated.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_semaphore_pc(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                PRODUCERS = 3,
                CONSUMERS = 3
        };
        worker_t prod[PRODUCERS];
        worker_t cons[CONSUMERS];
        pc_ctx_t pc;
        fail_t fail;

        fail_init(&fail);
        memset(prod, 0, sizeof(prod));
        memset(cons, 0, sizeof(cons));
        memset(&pc, 0, sizeof(pc));

        pc.empty = semaphore_new(RING_SIZE, RING_SIZE);
        pc.full = semaphore_new(RING_SIZE, 0);
        pc.mtx = mutex_new(MUTEX_TYPE_NORMAL);

        CHECK(pc.empty && pc.full && pc.mtx, "unable to create objects");

        for (size_t i = 0; i < CONSUMERS; i++) {
                worker_init(&cons[i], i, &fail, &pc, 0);
                CHECK(worker_start(&cons[i], th_sem_consumer, WORKER_PRIORITY[i + 3], STACK_DEPTH_LOW),
                        "consumer not created");
        }

        for (size_t i = 0; i < PRODUCERS; i++) {
                worker_init(&prod[i], i, &fail, &pc, global->stress_duration);
                CHECK(worker_start(&prod[i], th_sem_producer, WORKER_PRIORITY[i], STACK_DEPTH_LOW),
                        "producer not created");
        }

        CHECK(workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT), "producer not finished");
        pc.done = true;
        CHECK(workers_join(cons, CONSUMERS, SAFE_TIMEOUT), "consumer not finished");
        CHECK_FAIL(&fail);

        u32_t produced = 0, consumed = 0, sum_p = 0, sum_c = 0;
        for (size_t i = 0; i < PRODUCERS; i++) {
                produced += prod[i].ops;
                sum_p += prod[i].sum;
        }
        for (size_t i = 0; i < CONSUMERS; i++) {
                consumed += cons[i].ops;
                sum_c += cons[i].sum;
        }

        CHECK(produced == consumed, "%u items produced, %u consumed", (unsigned)produced, (unsigned)consumed);
        CHECK(sum_p == sum_c, "checksum of produced items 0x%X, consumed 0x%X", (unsigned)sum_p,
                (unsigned)sum_c);
        CHECK(semaphore_get_value(pc.empty) == RING_SIZE, "semaphore 'empty': value %d, expected %d",
                semaphore_get_value(pc.empty), RING_SIZE);
        CHECK(semaphore_get_value(pc.full) == 0, "semaphore 'full': value %d, expected 0",
                semaphore_get_value(pc.full));

        info_printf("%u items", (unsigned)produced);

cleanup:
        pc.done = true;
        workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT);
        workers_join(cons, CONSUMERS, SAFE_TIMEOUT);
        semaphore_delete(pc.empty);
        semaphore_delete(pc.full);
        mutex_delete(pc.mtx);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several producers and consumers use the same queue. Items cannot be
 *         lost or duplicated, order of items of each producer is checked.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_queue_mpmc(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                PRODUCERS = 3,
                CONSUMERS = 3
        };
        worker_t prod[PRODUCERS];
        worker_t cons[CONSUMERS];
        pc_ctx_t pc;
        fail_t fail;

        fail_init(&fail);
        memset(prod, 0, sizeof(prod));
        memset(cons, 0, sizeof(cons));
        memset(&pc, 0, sizeof(pc));

        pc.queue = queue_new(QUEUE_LEN, sizeof(qitem_t));
        CHECK(pc.queue, "unable to create queue");

        for (size_t i = 0; i < CONSUMERS; i++) {
                worker_init(&cons[i], i, &fail, &pc, 0);
                CHECK(worker_start(&cons[i], th_queue_consumer, WORKER_PRIORITY[i + 3], STACK_DEPTH_LOW),
                        "consumer not created");
        }

        for (size_t i = 0; i < PRODUCERS; i++) {
                worker_init(&prod[i], i, &fail, &pc, global->stress_duration);
                CHECK(worker_start(&prod[i], th_queue_producer, WORKER_PRIORITY[i], STACK_DEPTH_LOW),
                        "producer not created");
        }

        CHECK(workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT), "producer not finished");
        pc.done = true;
        CHECK(workers_join(cons, CONSUMERS, SAFE_TIMEOUT), "consumer not finished");
        CHECK_FAIL(&fail);

        u32_t produced = 0, consumed = 0, sum_p = 0, sum_c = 0;
        for (size_t i = 0; i < PRODUCERS; i++) {
                produced += prod[i].ops;
                sum_p += prod[i].sum;
        }
        for (size_t i = 0; i < CONSUMERS; i++) {
                consumed += cons[i].ops;
                sum_c += cons[i].sum;
        }

        CHECK(produced == consumed, "%u items sent, %u received", (unsigned)produced, (unsigned)consumed);
        CHECK(sum_p == sum_c, "checksum of sent items 0x%X, received 0x%X", (unsigned)sum_p, (unsigned)sum_c);
        CHECK(queue_get_number_of_items(pc.queue) == 0, "queue is not empty");

        info_printf("%u items", (unsigned)produced);

cleanup:
        pc.done = true;
        workers_join(prod, PRODUCERS, global->stress_duration + SAFE_TIMEOUT);
        workers_join(cons, CONSUMERS, SAFE_TIMEOUT);
        queue_delete(pc.queue);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several threads create, use and delete mutexes, semaphores and
 *         queues. All objects must be released after test.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_sync_storm(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                WORKERS = 3
        };
        worker_t w[WORKERS];
        sync_ctx_t *s[WORKERS];
        fail_t fail;

        fail_init(&fail);
        memset(w, 0, sizeof(w));
        memset(s, 0, sizeof(s));

        for (size_t i = 0; i < WORKERS; i++) {
                s[i] = sync_ctx_new(&fail);
                CHECK(s[i], "not enough free memory");
        }

        for (size_t i = 0; i < WORKERS; i++) {
                worker_init(&w[i], i, &fail, s[i], global->stress_duration);
                CHECK(worker_start(&w[i], th_sync_worker, WORKER_PRIORITY[i], STACK_DEPTH_LOW),
                        "worker %u not created", (unsigned)i);
        }

        CHECK(workers_join(w, WORKERS, global->stress_duration + SAFE_TIMEOUT),
                "worker not finished in time");
        CHECK_FAIL(&fail);

        u32_t ops = 0, created = 0, nomem = 0;
        for (size_t i = 0; i < WORKERS; i++) {
                ops += s[i]->ops;
                created += s[i]->created;
                nomem += s[i]->nomem;
        }

        info_printf("%u operations, %u objects created, %u failed", (unsigned)ops, (unsigned)created,
                (unsigned)nomem);

cleanup:
        workers_join(w, WORKERS, SAFE_TIMEOUT);

        for (size_t i = 0; i < WORKERS; i++) {
                if (s[i]) {
                        sync_release(s[i]);
                        free(s[i]);
                }
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Sleep and timeouts of synchronization objects when CPU is loaded by
 *         threads with lower priority.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_timing(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                LOADS = 2
        };
        worker_t load[LOADS];
        worker_t holder;
        worker_t sleeper;
        volatile bool stop = false;
        sem_t *sem = semaphore_new(1, 0);
        sem_t *release = semaphore_new(1, 0);
        queue_t *queue = queue_new(1, sizeof(u32_t));
        mutex_t *mtx = mutex_new(MUTEX_TYPE_NORMAL);
        futex_t futex;
        bool futex_locked = false;
        u32_t worst = 0;
        u32_t loops = 0;

        memset(load, 0, sizeof(load));
        memset(&holder, 0, sizeof(holder));
        memset(&sleeper, 0, sizeof(sleeper));
        futex_init(&futex);

        CHECK(sem && release && queue && mtx, "unable to create objects");

        for (size_t i = 0; i < LOADS; i++) {
                worker_init(&load[i], i, NULL, NULL, global->stress_duration + SAFE_TIMEOUT);
                load[i].stop = &stop;
                CHECK(worker_start(&load[i], th_busy, PRIORITY_LOWEST, STACK_DEPTH_LOW),
                        "load thread not created");
        }

        // mutex held by other thread
        worker_init(&holder, 0, NULL, release, 0);
        holder.obj = mtx;
        CHECK(worker_start(&holder, th_mutex_holder, PRIORITY_NORMAL, STACK_DEPTH_LOW), "helper not created");
        for (int i = 0; (i < 100) && !holder.res; i++) {
                msleep(1);
        }
        CHECK(holder.res, "helper did not lock mutex");

        CHECK(futex_lock(&futex, 0), "unable to lock futex");
        futex_locked = true;

        clock_t tend = get_uptime_ms() + global->stress_duration;

        do {
                for (size_t i = 0; i < ARRAY_SIZE(TIMING_TIMEOUT); i++) {
                        u32_t t = TIMING_TIMEOUT[i];
                        u32_t item;
                        clock_t tref;
                        u32_t dt;

                        tref = get_uptime_ms();
                        msleep(t);
                        dt = elapsed_ms(tref);
                        worst = max(worst, dt - min(dt, t));
                        CHECK(time_ok(dt, t), "msleep(%u) took %u ms", (unsigned)t, (unsigned)dt);

                        tref = get_uptime_ms();
                        CHECK(!semaphore_wait(sem, t), "not signaled semaphore taken");
                        dt = elapsed_ms(tref);
                        worst = max(worst, dt - min(dt, t));
                        CHECK(time_ok(dt, t), "semaphore wait timeout %u ms took %u ms", (unsigned)t,
                                (unsigned)dt);

                        tref = get_uptime_ms();
                        CHECK(!queue_receive(queue, &item, t), "item received from empty queue");
                        dt = elapsed_ms(tref);
                        worst = max(worst, dt - min(dt, t));
                        CHECK(time_ok(dt, t), "queue receive timeout %u ms took %u ms", (unsigned)t,
                                (unsigned)dt);

                        tref = get_uptime_ms();
                        CHECK(!mutex_lock(mtx, t), "mutex locked by two threads");
                        dt = elapsed_ms(tref);
                        worst = max(worst, dt - min(dt, t));
                        CHECK(time_ok(dt, t), "mutex lock timeout %u ms took %u ms", (unsigned)t,
                                (unsigned)dt);

                        tref = get_uptime_ms();
                        errno = 0;
                        bool flocked = futex_lock(&futex, t);
                        int ferr = errno;
                        dt = elapsed_ms(tref);
                        CHECK(!flocked, "locked futex locked again");
                        CHECK(ferr == ETIME, "futex timeout: errno %d, expected ETIME", ferr);
                        CHECK(time_ok(dt, t), "futex lock timeout %u ms took %u ms", (unsigned)t,
                                (unsigned)dt);
                }

                // join timeout of running thread
                worker_init(&sleeper, 0, NULL, sem, 0);
                sleeper.value = 1234;
                CHECK(worker_start(&sleeper, th_wait_sem, PRIORITY_NORMAL, STACK_DEPTH_LOW),
                        "thread not created");

                clock_t tref = get_uptime_ms();
                errno = 0;
                int ret = thread_join2(sleeper.tid, NULL, 20);
                int err = errno;
                u32_t dt = elapsed_ms(tref);

                semaphore_signal(sem);

                int status = -1;
                CHECK(thread_join2(sleeper.tid, &status, SAFE_TIMEOUT) == 0, "join of thread failed");
                sleeper.tid = 0;

                CHECK(ret != 0, "join of running thread succeeded");
                CHECK(err == ETIME, "join timeout: errno %d, expected ETIME", err);
                CHECK(time_ok(dt, 20), "join timeout 20 ms took %u ms", (unsigned)dt);
                CHECK(status == 1234, "thread exit status %d, expected 1234", status);

                loops++;
        } while (get_uptime_ms() < tend);

        info_printf("%u loops, max delay %u ms", (unsigned)loops, (unsigned)worst);

cleanup:
        stop = true;

        if (futex_locked) {
                futex_unlock(&futex);
        }

        semaphore_signal(release);
        semaphore_signal(sem);
        workers_join(&sleeper, 1, SAFE_TIMEOUT);
        workers_join(&holder, 1, SAFE_TIMEOUT);
        workers_join(load, LOADS, SAFE_TIMEOUT);

        semaphore_delete(sem);
        semaphore_delete(release);
        queue_delete(queue);
        mutex_delete(mtx);

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Start child processes sequentially and concurrently and check their
 *         exit status. Memory of processes must be released.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_process_spawn(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                CONCURRENT = 4
        };
        pid_t pid[CONCURRENT];
        int code[CONCURRENT];
        u8_t order[CONCURRENT];
        u32_t seed = new_seed();
        u32_t count = 0;
        char args[16];

        memset(pid, 0, sizeof(pid));

        // warm-up (the first start of process can allocate persistent objects)
        pid[0] = spawn_child("exit 0");
        CHECK(pid[0], "unable to start process '%s'", global->progname);
        CHECK(process_wait(pid[0], NULL, SAFE_TIMEOUT) == 0, "wait for process failed");
        pid[0] = 0;

        memstat_t mem_ref;
        sys_mem_get(&mem_ref);

        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                // sequentially
                code[0] = rnd_range(&seed, 0, 120);
                snprintf(args, sizeof(args), "exit %d", code[0]);

                pid[0] = spawn_child(args);
                CHECK(pid[0], "process %u not started", (unsigned)count);

                int status = -1;
                CHECK(process_wait(pid[0], &status, SAFE_TIMEOUT) == 0, "wait for process %u failed",
                        (unsigned)pid[0]);
                pid[0] = 0;
                CHECK(status == code[0], "process exit status %d, expected %d", status, code[0]);
                count++;

                // concurrently
                size_t n = rnd_range(&seed, 2, CONCURRENT);
                for (size_t i = 0; i < n; i++) {
                        code[i] = rnd_range(&seed, 0, 120);
                        order[i] = i;
                        snprintf(args, sizeof(args), "exit %d", code[i]);
                        pid[i] = spawn_child(args);
                        CHECK(pid[i], "process %u not started", (unsigned)count);
                        count++;
                }

                shuffle(order, n, &seed);

                for (size_t k = 0; k < n; k++) {
                        size_t i = order[k];
                        status = -1;
                        CHECK(process_wait(pid[i], &status, SAFE_TIMEOUT) == 0, "wait for process %u failed",
                                (unsigned)pid[i]);
                        pid[i] = 0;
                        CHECK(status == code[i], "process exit status %d, expected %d", status, code[i]);
                }
        }

        const char *msg = sys_mem_check(&mem_ref);
        if (msg) {
                exit_msg = msg;
                goto cleanup;
        }

        size_t left = count_children();
        CHECK(left == 0, "%u child processes still exist", (unsigned)left);

        info_printf("%u processes", (unsigned)count);

cleanup:
        for (size_t i = 0; i < CONCURRENT; i++) {
                if (pid[i]) {
                        if (process_wait(pid[i], NULL, SAFE_TIMEOUT) != 0) {
                                process_kill(pid[i]);
                                process_wait(pid[i], NULL, SAFE_TIMEOUT);
                        }
                }
        }

        return exit_msg;
}

//==============================================================================
/**
 * @brief  Child processes that load the system (threads, heap, synchronization
 *         objects) are killed at random moment. Memory of processes must be
 *         released.
 *
 * @return Test result.
 */
//==============================================================================
static const char *test_process_kill(void)
{
        const char *exit_msg = SUCCESS_MSG;
        enum {
                CHILDREN = 3
        };
        pid_t pid[CHILDREN];
        u32_t seed = new_seed();
        u32_t kills = 0;
        u32_t by_wait = 0;
        u32_t by_kill = 0;
        u32_t max_threads = 0;

        memset(pid, 0, sizeof(pid));

        // warm-up (the first start of process can allocate persistent objects)
        pid[0] = spawn_child("exit 0");
        CHECK(pid[0], "unable to start process '%s'", global->progname);
        CHECK(process_wait(pid[0], NULL, SAFE_TIMEOUT) == 0, "wait for process failed");
        pid[0] = 0;

        // kill of finished process (not waited yet) succeeds and keeps its exit status
        pid[0] = spawn_child("exit 7");
        CHECK(pid[0], "unable to start process '%s'", global->progname);

        for (int t = 0; t < 200; t++) {
                process_stat_t stat;
                memset(&stat, 0, sizeof(stat));
                if ((process_stat(pid[0], &stat) != 0) || (stat.threads_count == 0)) {
                        break;
                }
                msleep(10);
        }

        CHECK(process_kill(pid[0]) == 0, "kill of finished process failed");

        int fstatus = -1;
        CHECK(process_wait(pid[0], &fstatus, SAFE_TIMEOUT) == 0, "wait for finished process failed");
        pid[0] = 0;
        CHECK(fstatus == 7, "exit status of finished process %d after kill, expected 7", fstatus);

        errno = 0;
        CHECK((process_kill(0x7FFF) != 0) && (errno == ESRCH), "kill of not existing process: errno %d",
                errno);

        memstat_t mem_ref;
        sys_mem_get(&mem_ref);

        clock_t tref = get_uptime_ms();

        while (elapsed_ms(tref) < global->stress_duration) {
                size_t n = rnd_range(&seed, 1, CHILDREN);

                for (size_t i = 0; i < n; i++) {
                        pid[i] = spawn_child("work");
                        CHECK(pid[i], "process not started");
                }

                // the quarter of processes is killed immediately (before their threads start)
                u32_t delay = (rnd(&seed) & 3) ? rnd_range(&seed, 1, 100) : 0;
                if (delay) {
                        msleep(delay);
                }

                for (size_t i = 0; i < n; i++) {
                        process_stat_t stat;
                        memset(&stat, 0, sizeof(stat));
                        CHECK(process_stat(pid[i], &stat) == 0, "process %u terminated before kill",
                                (unsigned)pid[i]);
                        max_threads = max(max_threads, stat.threads_count);

                        CHECK(process_kill(pid[i]) == 0, "kill of process %u failed", (unsigned)pid[i]);

                        /*
                         * NOTE: killed process is waited for. If wait fails,
                         *       the process must not exist (it was removed by
                         *       kill). Both cases are counted, because only one
                         *       behavior is expected.
                         */
                        int status = -1;
                        if (process_wait(pid[i], &status, SAFE_TIMEOUT) == 0) {
                                CHECK(status == PROCESS_STATUS_KILLED,
                                        "exit status of killed process %d, expected %d", status,
                                        PROCESS_STATUS_KILLED);
                                by_wait++;
                        } else {
                                int err = errno;
                                memset(&stat, 0, sizeof(stat));
                                CHECK(process_stat(pid[i], &stat) != 0,
                                        "wait for killed process %u failed, but process exists (errno: %s)",
                                        (unsigned)pid[i], strerror(err));
                                by_kill++;
                        }

                        pid[i] = 0;
                        kills++;
                }
        }

        const char *msg = sys_mem_check(&mem_ref);
        if (msg) {
                exit_msg = msg;
                goto cleanup;
        }

        size_t left = count_children();
        CHECK(left == 0, "%u child processes still exist", (unsigned)left);

        CHECK((by_kill == 0) || (by_wait == 0),
                "process_kill() is not deterministic: %u killed processes removed by kill (wait fails), "
                "%u must be waited for",
                (unsigned)by_kill, (unsigned)by_wait);

        info_printf("%u processes killed (%u waited), max %u threads per process", (unsigned)kills,
                (unsigned)by_wait, (unsigned)max_threads);

cleanup:
        for (size_t i = 0; i < CHILDREN; i++) {
                if (pid[i]) {
                        process_kill(pid[i]);
                        process_wait(pid[i], NULL, SAFE_TIMEOUT);
                }
        }

        return exit_msg;
}

/*==============================================================================
  End of file
==============================================================================*/
