/*==============================================================================
@file   memmon.c

@author Daniel Zorychta

@brief  Memory monitor - periodically samples system memory indicators,
        reports trends that may indicate a memory leak and points out the
        probable location of the leak (memory category, driver, process,
        handles of process).

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
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DEFAULT_INTERVAL_S    10
#define DEFAULT_WINDOW        30
#define MIN_WINDOW            4
#define MAX_WINDOW            120
#define DEFAULT_THRESHOLD_BPH 1024
#define MAX_PROCESSES         16
#define PROC_NAME_LEN         12
#define HEADER_EVERY_LINES    20
#define KEY_POLL_MS           100
#define HIGHS_SUSPECT         5
#define MAX_SUSPECTS          6

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef struct {
        u32_t t;    //!< sample time [s] relative to start
        u32_t used; //!< used memory [B]
} sample_t;

/**
 * Value tracker. The value is observed in consecutive windows of samples.
 * The minimum of the window (floor) does not change when memory is only
 * allocated temporarily, but it rises when memory leaks.
 */
typedef struct {
        i32_t base;     //!< first value
        i32_t last;     //!< last value
        i32_t max;      //!< highest value
        i32_t win_min;  //!< minimum in the current window
        i32_t floor0;   //!< minimum of the first complete window
        i32_t floor;    //!< minimum of the last complete window
        u16_t highs;    //!< number of new maximums
        u16_t wsamples; //!< number of samples in the current window
        u8_t rises;     //!< consecutive windows with rising floor
        u8_t windows;   //!< number of complete windows (saturated)
        bool valid;     //!< tracker initialized
} trk_t;

/** memory categories reported by the kernel, and memory not owned by processes */
enum {
        CAT_KERNEL,
        CAT_FS,
        CAT_NET,
        CAT_MOD,
        CAT_PROG,
        CAT_SHM,
        CAT_CACHE,
        CAT_UNOWNED,
        CAT_COUNT
};

/** resources (handles) owned by process */
enum {
        HND_FILE,
        HND_DIR,
        HND_MUTEX,
        HND_SEM,
        HND_QUEUE,
        HND_SOCKET,
        HND_THREAD,
        HND_COUNT
};

typedef struct {
        pid_t pid;                //!< process ID (0 = free slot)
        bool seen;                //!< process found in the last scan
        bool zombie;              //!< process has no threads
        u16_t base_blocks;        //!< number of memory blocks at first sight
        u16_t last_blocks;        //!< number of memory blocks in the last scan
        u16_t h_base[HND_COUNT];  //!< number of handles at first sight
        u16_t h_last[HND_COUNT];  //!< number of handles in the last scan
        u16_t h_max;              //!< highest total number of handles
        trk_t mem;                //!< memory usage tracker
        char name[PROC_NAME_LEN]; //!< process name (truncated)
} proc_entry_t;

typedef enum {
        SUSP_CAT,
        SUSP_DRV,
        SUSP_PROC,
} susp_kind_t;

typedef struct {
        susp_kind_t kind;
        uint idx;
        i32_t growth;
} suspect_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_help(const char *name);
static bool parse_args(int argc, char *argv[]);
static void reset_baseline(void);
static void take_sample(void);
static i32_t calc_trend_bph(void);
static void print_header(void);
static void print_categories(void);
static void update_categories(void);
static u32_t scan_processes(void);
static void scan_drivers(void);
static void print_summary(void);
static uint find_suspects(suspect_t *s);
static void print_suspect(const suspect_t *s);
static void print_location(void);
static void report_location(void);
static void suspect_add(suspect_t *s, uint *n, susp_kind_t kind, uint idx, const trk_t *t);
static void trk_update(trk_t *t, i32_t val, bool *new_high);
static i32_t trk_growth(const trk_t *t);
static bool trk_suspect(const trk_t *t);
static const char *cat_name(uint cat);
static const char *cat_hint(uint cat);
static const char *hnd_name(uint hnd);
static void handles_str(const proc_entry_t *e, char *buf, size_t len);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        /* configuration */
        u32_t interval_s;
        u32_t count;
        u32_t window;
        u32_t threshold_bph;
        bool show_categories;
        bool monitor_processes;
        bool monitor_drivers;

        /* state */
        clock_t start_ms;
        memstat_t mem;
        memstat_t base;
        u32_t prev_used;
        u32_t min_free;
        u32_t max_used;
        u32_t samples;
        u32_t lines;
        u32_t first_floor;
        bool first_floor_valid;
        bool leak_suspected;
        int rep_kind;  //!< kind of reported location, -1 = none
        uint rep_idx;  //!< index of reported location
        pid_t rep_pid; //!< PID of reported process

        sample_t *ring;
        u32_t ring_head;
        u32_t ring_len;

        trk_t cat[CAT_COUNT];

        trk_t *drv;
        uint drv_count;

        proc_entry_t *proc;
        process_stat_t pstat;
        char cmdline[32];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(memmon, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Main function.
 *
 * @param[in]  argc     argument count
 * @param[in]  argv     arguments
 *
 * @return Program exit status.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int status = EXIT_FAILURE;

        if (!parse_args(argc, argv)) {
                print_help(argv[0]);
                return EXIT_FAILURE;
        }

        global->ring = calloc(global->window, sizeof(sample_t));
        if (!global->ring) {
                goto finish;
        }

        if (global->monitor_processes) {
                global->proc = calloc(MAX_PROCESSES, sizeof(proc_entry_t));
                if (!global->proc) {
                        goto finish;
                }
        }

        if (global->monitor_drivers) {
                global->drv_count = get_number_of_drivers();
                global->drv = calloc(max(global->drv_count, 1), sizeof(trk_t));
                if (!global->drv) {
                        goto finish;
                }
        }

        printf("Memory monitor: interval %u s, trend window %u samples, threshold %u B/h\n",
                (uint)global->interval_s, (uint)global->window, (uint)global->threshold_bph);
        printf("Keys: q/Ctrl+C - quit, s - summary, r - reset baseline\n\n");

        ioctl(fileno(stdin), IOCTL_TTY__ECHO_OFF);

        reset_baseline();
        take_sample();

        clock_t next = get_uptime_ms() + (clock_t)global->interval_s * 1000;
        bool run = true;

        while (run) {
                ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);
                clearerr(stdin);
                int key = getchar();
                ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);

                switch (key) {
                case 'q':
                case ETX:
                        run = false;
                        continue;

                case 's':
                        print_summary();
                        break;

                case 'r':
                        reset_baseline();
                        printf("Baseline reset.\n");
                        global->lines = 0;
                        take_sample();
                        next = get_uptime_ms() + (clock_t)global->interval_s * 1000;
                        break;

                default:
                        break;
                }

                if (get_uptime_ms() >= next) {
                        next += (clock_t)global->interval_s * 1000;
                        take_sample();

                        if ((global->count > 0) && (global->samples >= global->count)) {
                                run = false;
                        }
                } else {
                        msleep(KEY_POLL_MS);
                }
        }

        ioctl(fileno(stdin), IOCTL_TTY__ECHO_ON);

        print_summary();

        status = global->leak_suspected ? 2 : EXIT_SUCCESS;

finish:
        if (status == EXIT_FAILURE) {
                perror(NULL);
        }

        free(global->ring);
        free(global->proc);
        free(global->drv);

        return status;
}

//==============================================================================
/**
 * @brief  Print program usage.
 *
 * @param[in]  name     program name
 */
//==============================================================================
static void print_help(const char *name)
{
        printf("Usage: %s [options]\n", name);
        printf("  -i <sec>   sampling interval in seconds (default %u)\n", DEFAULT_INTERVAL_S);
        printf("  -n <num>   number of samples, 0 = infinite (default 0)\n");
        printf("  -w <num>   trend window in samples, %u..%u (default %u)\n", MIN_WINDOW, MAX_WINDOW,
                DEFAULT_WINDOW);
        printf("  -t <B/h>   leak threshold of the used-memory trend (default %u)\n", DEFAULT_THRESHOLD_BPH);
        printf("  -c         show change of memory categories since baseline\n");
        printf("  -p         monitor memory and handles of processes\n");
        printf("  -d         monitor memory of drivers\n");
        printf("  -h         this help\n");
        printf("Use -p and -d to find out where the leak is.\n");
        printf("Exit status: 0 - no leak suspected, 1 - error, 2 - leak suspected\n");
}

//==============================================================================
/**
 * @brief  Parse program arguments.
 *
 * @param[in]  argc     argument count
 * @param[in]  argv     arguments
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool parse_args(int argc, char *argv[])
{
        global->interval_s = DEFAULT_INTERVAL_S;
        global->window = DEFAULT_WINDOW;
        global->threshold_bph = DEFAULT_THRESHOLD_BPH;

        for (int i = 1; i < argc; i++) {
                const char *arg = argv[i];
                bool val = (i + 1 < argc);

                if (isstreq(arg, "-i") && val) {
                        global->interval_s = strtoul(argv[++i], NULL, 0);

                } else if (isstreq(arg, "-n") && val) {
                        global->count = strtoul(argv[++i], NULL, 0);

                } else if (isstreq(arg, "-w") && val) {
                        global->window = strtoul(argv[++i], NULL, 0);

                } else if (isstreq(arg, "-t") && val) {
                        global->threshold_bph = strtoul(argv[++i], NULL, 0);

                } else if (isstreq(arg, "-c")) {
                        global->show_categories = true;

                } else if (isstreq(arg, "-p")) {
                        global->monitor_processes = true;

                } else if (isstreq(arg, "-d")) {
                        global->monitor_drivers = true;

                } else {
                        return false;
                }
        }

        if ((global->interval_s == 0) || (global->window < MIN_WINDOW) || (global->window > MAX_WINDOW)) {
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief  Reset baseline - current state becomes the reference point.
 */
//==============================================================================
static void reset_baseline(void)
{
        get_memory_usage_details(&global->base);

        global->start_ms = get_uptime_ms();
        global->prev_used = global->base.used_memory;
        global->min_free = global->base.free_memory;
        global->max_used = global->base.used_memory;
        global->samples = 0;
        global->ring_head = 0;
        global->ring_len = 0;
        global->first_floor = 0;
        global->first_floor_valid = false;
        global->leak_suspected = false;
        global->rep_kind = -1;

        memset(global->cat, 0, sizeof(global->cat));

        if (global->drv) {
                memset(global->drv, 0, max(global->drv_count, 1) * sizeof(trk_t));
        }

        if (global->proc) {
                memset(global->proc, 0, MAX_PROCESSES * sizeof(proc_entry_t));
        }
}

//==============================================================================
/**
 * @brief  Take one sample of memory indicators and print it.
 */
//==============================================================================
static void take_sample(void)
{
        if (get_memory_usage_details(&global->mem) != 0) {
                perror("memstat");
                return;
        }

        u32_t t = (get_uptime_ms() - global->start_ms) / 1000;
        u32_t used = global->mem.used_memory;
        u32_t fre = global->mem.free_memory;

        global->samples++;
        global->min_free = min(global->min_free, fre);
        global->max_used = max(global->max_used, used);

        /* trend window (ring buffer) */
        global->ring[global->ring_head].t = t;
        global->ring[global->ring_head].used = used;
        global->ring_head = (global->ring_head + 1) % global->window;
        global->ring_len = min(global->ring_len + 1, global->window);

        /* floor = the lowest used memory in the window; leaks lift the floor */
        u32_t floor = UINT32_MAX;
        for (u32_t i = 0; i < global->ring_len; i++) {
                floor = min(floor, global->ring[i].used);
        }

        if (!global->first_floor_valid && (global->ring_len == global->window)) {
                global->first_floor = floor;
                global->first_floor_valid = true;
        }

        i32_t trend = calc_trend_bph();
        bool full = (global->ring_len == global->window);
        bool rising = global->first_floor_valid && (floor > global->first_floor);
        bool leak = full && rising && (trend > (i32_t)global->threshold_bph);

        if (leak) {
                global->leak_suspected = true;
        }

        if ((global->lines % HEADER_EVERY_LINES) == 0) {
                print_header();
        }
        global->lines++;

        printf("%4u:%02u:%02u %9u %9u %+7d %+8d %9u %+8d %s\n", (uint)(t / 3600), (uint)((t / 60) % 60),
                (uint)(t % 60), (uint)used, (uint)fre, (int)((i32_t)used - (i32_t)global->prev_used),
                (int)((i32_t)used - (i32_t)global->base.used_memory), (uint)global->min_free, (int)trend,
                !full            ? "warmup"
                        : leak   ? "LEAK?"
                        : rising ? "floor+"
                                 : "ok");

        global->prev_used = used;

        if (global->show_categories) {
                print_categories();
        }

        update_categories();

        if (global->monitor_drivers) {
                scan_drivers();
        }

        if (global->monitor_processes) {
                /* program memory that is not owned by any running process */
                u32_t owned = scan_processes();
                trk_update(&global->cat[CAT_UNOWNED], global->mem.programs_memory_usage - (i32_t)owned, NULL);
        }

        if (leak) {
                report_location();
        }
}

//==============================================================================
/**
 * @brief  Calculate used memory trend using linear regression over window.
 *
 * @return Trend in bytes per hour.
 */
//==============================================================================
static i32_t calc_trend_bph(void)
{
        u32_t n = global->ring_len;
        if (n < 2) {
                return 0;
        }

        /* oldest sample is the reference point to keep numbers small */
        u32_t oldest = (global->ring_head + global->window - n) % global->window;
        sample_t *ref = &global->ring[oldest];

        double st = 0, su = 0, stt = 0, stu = 0;

        for (u32_t i = 0; i < n; i++) {
                sample_t *s = &global->ring[(oldest + i) % global->window];
                double x = (double)(s->t - ref->t);
                double y = (double)((i32_t)s->used - (i32_t)ref->used);

                st += x;
                su += y;
                stt += x * x;
                stu += x * y;
        }

        double den = (double)n * stt - st * st;
        if (den <= 0) {
                return 0;
        }

        double slope = ((double)n * stu - st * su) / den * 3600.0;

        if (slope > (double)INT32_MAX) {
                return INT32_MAX;
        } else if (slope < (double)INT32_MIN) {
                return INT32_MIN;
        } else {
                return (i32_t)slope;
        }
}

//==============================================================================
/**
 * @brief  Print table header.
 */
//==============================================================================
static void print_header(void)
{
        printf("      TIME      USED      FREE   dPREV    dBASE   MINFREE  TREND/h STATUS\n");
}

//==============================================================================
/**
 * @brief  Print change of memory categories since baseline.
 */
//==============================================================================
static void print_categories(void)
{
        memstat_t *m = &global->mem;
        memstat_t *b = &global->base;

        printf("          krn %+d fs %+d net %+d mod %+d prg %+d shm %+d cache %+d\n",
                (int)(m->kernel_memory_usage - b->kernel_memory_usage),
                (int)(m->filesystems_memory_usage - b->filesystems_memory_usage),
                (int)(m->network_memory_usage - b->network_memory_usage),
                (int)(m->modules_memory_usage - b->modules_memory_usage),
                (int)(m->programs_memory_usage - b->programs_memory_usage),
                (int)(m->shared_memory_usage - b->shared_memory_usage),
                (int)(m->cached_memory_usage - b->cached_memory_usage));
}

//==============================================================================
/**
 * @brief  Update trackers of memory categories.
 */
//==============================================================================
static void update_categories(void)
{
        memstat_t *m = &global->mem;

        trk_update(&global->cat[CAT_KERNEL], m->kernel_memory_usage, NULL);
        trk_update(&global->cat[CAT_FS], m->filesystems_memory_usage, NULL);
        trk_update(&global->cat[CAT_NET], m->network_memory_usage, NULL);
        trk_update(&global->cat[CAT_MOD], m->modules_memory_usage, NULL);
        trk_update(&global->cat[CAT_PROG], m->programs_memory_usage, NULL);
        trk_update(&global->cat[CAT_SHM], m->shared_memory_usage, NULL);
        trk_update(&global->cat[CAT_CACHE], m->cached_memory_usage, NULL);
}

//==============================================================================
/**
 * @brief  Scan memory usage of drivers and print these with new maximum.
 */
//==============================================================================
static void scan_drivers(void)
{
        for (uint i = 0; i < global->drv_count; i++) {
                i32_t usage = 0;
                if (get_driver_memory_usage(i, &usage) != 0) {
                        continue;
                }

                trk_t *t = &global->drv[i];
                bool new_high = false;
                trk_update(t, usage, &new_high);

                if (new_high) {
                        printf("          drv %-*s new max %d (base %+d)%s\n", PROC_NAME_LEN + 4,
                                get_driver_name(i), (int)usage, (int)(usage - t->base),
                                trk_suspect(t) ? " GROWING" : "");
                }
        }
}

//==============================================================================
/**
 * @brief  Scan processes and print these whose memory or handles grow.
 *
 * @return Sum of memory owned by all processes (including this one).
 */
//==============================================================================
static u32_t scan_processes(void)
{
        for (int i = 0; i < MAX_PROCESSES; i++) {
                global->proc[i].seen = false;
        }

        global->pstat.cmdline_buf = global->cmdline;
        global->pstat.cmdline_len = sizeof(global->cmdline);

        pid_t self = getpid();
        size_t seek = 0;
        u32_t owned = 0;

        while (process_stat_seek(seek++, &global->pstat) == 0) {
                process_stat_t *ps = &global->pstat;

                owned += ps->memory_usage;

                if (ps->pid == self) {
                        continue;
                }

                proc_entry_t *e = NULL;
                proc_entry_t *slot = NULL;

                for (int i = 0; i < MAX_PROCESSES; i++) {
                        if (global->proc[i].pid == ps->pid) {
                                e = &global->proc[i];
                                break;
                        } else if (!slot && (global->proc[i].pid == 0)) {
                                slot = &global->proc[i];
                        }
                }

                u16_t h[HND_COUNT];
                h[HND_FILE] = ps->files_count;
                h[HND_DIR] = ps->dir_count;
                h[HND_MUTEX] = ps->mutexes_count;
                h[HND_SEM] = ps->semaphores_count;
                h[HND_QUEUE] = ps->queue_count;
                h[HND_SOCKET] = ps->socket_count;
                h[HND_THREAD] = ps->threads_count;

                u16_t h_total = 0;
                for (int i = 0; i < HND_COUNT; i++) {
                        h_total += h[i];
                }

                if (!e) {
                        if (!slot) {
                                continue; /* table full */
                        }

                        e = slot;
                        e->pid = ps->pid;
                        e->base_blocks = ps->memory_block_count;
                        e->h_max = h_total;
                        memcpy(e->h_base, h, sizeof(h));

                        /* program name = first word of command line */
                        size_t len = strcspn(global->cmdline, " ");
                        len = min(len, sizeof(e->name) - 1);
                        memcpy(e->name, global->cmdline, len);
                        e->name[len] = '\0';
                }

                e->seen = true;
                e->last_blocks = ps->memory_block_count;
                memcpy(e->h_last, h, sizeof(h));

                bool zombie = (ps->threads_count == 0);
                bool new_high = false;
                trk_update(&e->mem, ps->memory_usage, &new_high);

                /* leaked handles do not have to increase memory of process */
                bool new_handles = (h_total > e->h_max);
                e->h_max = max(e->h_max, h_total);

                if (new_high || new_handles || (zombie != e->zombie)) {
                        char hbuf[48];
                        handles_str(e, hbuf, sizeof(hbuf));

                        printf("          pid %3d %-*s mem %u (base %+d) blk %u (%+d)%s%s%s\n", (int)e->pid,
                                PROC_NAME_LEN, e->name, (uint)ps->memory_usage,
                                (int)(e->mem.last - e->mem.base), (uint)e->last_blocks,
                                (int)e->last_blocks - (int)e->base_blocks, hbuf,
                                trk_suspect(&e->mem) ? " GROWING" : "", zombie ? " ZOMBIE" : "");
                }

                e->zombie = zombie;
        }

        /* release slots of finished processes */
        for (int i = 0; i < MAX_PROCESSES; i++) {
                if (global->proc[i].pid && !global->proc[i].seen) {
                        printf("          pid %3d %-*s exited\n", (int)global->proc[i].pid, PROC_NAME_LEN,
                                global->proc[i].name);
                        memset(&global->proc[i], 0, sizeof(proc_entry_t));
                }
        }

        return owned;
}

//==============================================================================
/**
 * @brief  Print summary since baseline.
 */
//==============================================================================
static void print_summary(void)
{
        u32_t t = (get_uptime_ms() - global->start_ms) / 1000;

        printf("\nSummary after %u:%02u:%02u (%u samples):\n", (uint)(t / 3600), (uint)((t / 60) % 60),
                (uint)(t % 60), (uint)global->samples);
        printf("  Total memory : %u\n", (uint)global->mem.memory_size);
        printf("  Used         : %u (baseline %u, %+d, max %u)\n", (uint)global->mem.used_memory,
                (uint)global->base.used_memory,
                (int)((i32_t)global->mem.used_memory - (i32_t)global->base.used_memory),
                (uint)global->max_used);
        printf("  Free         : %u (lowest %u)\n", (uint)global->mem.free_memory, (uint)global->min_free);
        printf("  Trend        : %+d B/h over last %u samples\n", (int)calc_trend_bph(),
                (uint)global->ring_len);

        if (global->first_floor_valid) {
                u32_t floor = UINT32_MAX;
                for (u32_t i = 0; i < global->ring_len; i++) {
                        floor = min(floor, global->ring[i].used);
                }
                printf("  Used floor   : %u (first window %u, %+d)\n", (uint)floor, (uint)global->first_floor,
                        (int)((i32_t)floor - (i32_t)global->first_floor));
        }

        printf("  Verdict      : %s\n", global->leak_suspected ? "LEAK SUSPECTED" : "no leak detected");

        print_location();

        printf("\n");
}

//==============================================================================
/**
 * @brief  Find the most probable locations of the leak.
 *
 * Sources (memory categories, drivers, processes) are sorted by growth of
 * their memory floor. A source is suspected if its floor rises in consecutive
 * windows, or it reaches new maximum many times.
 *
 * @param[out] s        suspect list (MAX_SUSPECTS entries)
 *
 * @return Number of suspects.
 */
//==============================================================================
static uint find_suspects(suspect_t *s)
{
        uint n = 0;

        for (uint i = 0; i < CAT_COUNT; i++) {
                /* the unowned category is tracked only with process monitoring */
                if ((i != CAT_UNOWNED) || global->monitor_processes) {
                        suspect_add(s, &n, SUSP_CAT, i, &global->cat[i]);
                }
        }

        for (uint i = 0; global->drv && (i < global->drv_count); i++) {
                suspect_add(s, &n, SUSP_DRV, i, &global->drv[i]);
        }

        for (uint i = 0; global->proc && (i < MAX_PROCESSES); i++) {
                if (global->proc[i].pid) {
                        suspect_add(s, &n, SUSP_PROC, i, &global->proc[i].mem);
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief  Print description of suspected location.
 *
 * @param[in]  s        suspect
 */
//==============================================================================
static void print_suspect(const suspect_t *s)
{
        switch (s->kind) {
        case SUSP_CAT:
                printf("%-16s %s\n", cat_name(s->idx), cat_hint(s->idx));
                break;

        case SUSP_DRV: {
                trk_t *t = &global->drv[s->idx];
                printf("driver %-9s now %d B, %u new maximums\n", get_driver_name(s->idx), (int)t->last,
                        (uint)t->highs);
                break;
        }

        case SUSP_PROC: {
                proc_entry_t *e = &global->proc[s->idx];
                char hbuf[48];
                handles_str(e, hbuf, sizeof(hbuf));
                printf("pid %d %s: now %d B, blocks %+d, %u new maximums%s%s\n", (int)e->pid, e->name,
                        (int)e->mem.last, (int)e->last_blocks - (int)e->base_blocks, (uint)e->mem.highs, hbuf,
                        e->zombie ? ", ZOMBIE" : "");
                break;
        }
        }
}

//==============================================================================
/**
 * @brief  Print the most probable location of the leak during monitoring.
 *         The location is printed when it is found or changed.
 */
//==============================================================================
static void report_location(void)
{
        suspect_t s[MAX_SUSPECTS];
        uint n = find_suspects(s);

        if (n == 0) {
                return;
        }

        /* prefer exact location (driver, process) to memory category */
        uint top = 0;
        for (uint i = 0; i < n; i++) {
                if (s[i].kind != SUSP_CAT) {
                        top = i;
                        break;
                }
        }

        pid_t pid = (s[top].kind == SUSP_PROC) ? global->proc[s[top].idx].pid : 0;

        if ((global->rep_kind != (int)s[top].kind) || (global->rep_idx != s[top].idx) ||
                (global->rep_pid != pid)) {
                global->rep_kind = s[top].kind;
                global->rep_idx = s[top].idx;
                global->rep_pid = pid;

                printf("          -> look at: ");
                print_suspect(&s[top]);

                /* memory of handles is charged to kernel, file systems and network */
                if ((s[top].kind == SUSP_CAT) && global->proc &&
                        ((s[top].idx == CAT_KERNEL) || (s[top].idx == CAT_FS) || (s[top].idx == CAT_NET))) {

                        proc_entry_t *best = NULL;
                        int best_delta = 0;

                        for (uint i = 0; i < MAX_PROCESSES; i++) {
                                proc_entry_t *e = &global->proc[i];
                                int delta = 0;
                                for (uint h = 0; e->pid && (h < HND_COUNT); h++) {
                                        delta += max((int)e->h_last[h] - (int)e->h_base[h], 0);
                                }

                                if (delta > best_delta) {
                                        best_delta = delta;
                                        best = e;
                                }
                        }

                        if (best) {
                                char hbuf[48];
                                handles_str(best, hbuf, sizeof(hbuf));
                                printf("          -> handles: pid %d %s%s\n", (int)best->pid, best->name,
                                        hbuf);
                        }
                }
        }
}

//==============================================================================
/**
 * @brief  Print all suspected locations of the leak.
 */
//==============================================================================
static void print_location(void)
{
        suspect_t s[MAX_SUSPECTS];
        uint n = find_suspects(s);

        printf("  Leak location (growth of memory floor):\n");

        if (n == 0) {
                printf("    no growing source found\n");
        }

        for (uint i = 0; i < n; i++) {
                printf("    %+7d B  ", (int)s[i].growth);
                print_suspect(&s[i]);
        }

        /* handles leak: memory may be charged to kernel, file systems or network */
        bool any = false;
        for (uint i = 0; global->proc && (i < MAX_PROCESSES); i++) {
                proc_entry_t *e = &global->proc[i];
                if (!e->pid) {
                        continue;
                }

                char hbuf[48];
                handles_str(e, hbuf, sizeof(hbuf));

                if (hbuf[0]) {
                        if (!any) {
                                printf("  Handles opened since start (charged to kernel/fs/network):\n");
                                any = true;
                        }
                        printf("    pid %3d %-*s %s\n", (int)e->pid, PROC_NAME_LEN, e->name, hbuf + 2);
                }
        }

        if (!global->monitor_processes || !global->monitor_drivers) {
                printf("  Run with%s%s to search for the leak in %s.\n",
                        global->monitor_processes ? "" : " -p", global->monitor_drivers ? "" : " -d",
                        (!global->monitor_processes && !global->monitor_drivers) ? "processes and drivers"
                                : global->monitor_processes                      ? "drivers"
                                                                                 : "processes");
        }
}

//==============================================================================
/**
 * @brief  Add source to the list of suspects if it looks like a leak. The list
 *         is sorted by growth, on equal growth exact location is preferred.
 *
 * @param[in,out] s             suspect list (MAX_SUSPECTS entries)
 * @param[in,out] n             number of entries in list
 * @param[in]     kind          kind of source
 * @param[in]     idx           index of source
 * @param[in]     t             tracker of source
 */
//==============================================================================
static void suspect_add(suspect_t *s, uint *n, susp_kind_t kind, uint idx, const trk_t *t)
{
        if (!trk_suspect(t)) {
                return;
        }

        i32_t growth = trk_growth(t);

        /* insert sorted, the smallest one drops out */
        uint pos = *n;
        while ((pos > 0) &&
                ((s[pos - 1].growth < growth) ||
                        ((s[pos - 1].growth == growth) && (s[pos - 1].kind < kind)))) {
                if (pos < MAX_SUSPECTS) {
                        s[pos] = s[pos - 1];
                }
                pos--;
        }

        if (pos < MAX_SUSPECTS) {
                s[pos].kind = kind;
                s[pos].idx = idx;
                s[pos].growth = growth;
                *n = min(*n + 1, MAX_SUSPECTS);
        }
}

//==============================================================================
/**
 * @brief  Update tracker by new value.
 *
 * @param[in]  t                tracker
 * @param[in]  val              value
 * @param[out] new_high         new maximum indicator (can be NULL)
 */
//==============================================================================
static void trk_update(trk_t *t, i32_t val, bool *new_high)
{
        bool high = false;

        if (!t->valid) {
                memset(t, 0, sizeof(*t));
                t->base = val;
                t->max = val;
                t->valid = true;
        }

        t->last = val;

        if (val > t->max) {
                t->max = val;
                if (t->highs < UINT16_MAX) {
                        t->highs++;
                }
                high = true;
        }

        if (t->wsamples == 0) {
                t->win_min = val;
        } else {
                t->win_min = min(t->win_min, val);
        }

        if (++t->wsamples >= global->window) {
                if (t->windows == 0) {
                        t->floor0 = t->win_min;
                } else if (t->win_min > t->floor) {
                        t->rises += (t->rises < UINT8_MAX) ? 1 : 0;
                } else if (t->win_min < t->floor) {
                        t->rises = 0;
                }

                t->floor = t->win_min;
                t->windows += (t->windows < UINT8_MAX) ? 1 : 0;
                t->wsamples = 0;
        }

        if (new_high) {
                *new_high = high;
        }
}

//==============================================================================
/**
 * @brief  Return growth of tracked value.
 *
 * @param[in]  t                tracker
 *
 * @return Growth of floor if at least 2 windows are complete, otherwise growth
 *         of the value since the first sample.
 */
//==============================================================================
static i32_t trk_growth(const trk_t *t)
{
        if (t->windows >= 2) {
                return t->floor - t->floor0;
        } else {
                return t->last - t->base;
        }
}

//==============================================================================
/**
 * @brief  Check if tracked value looks like a leak.
 *
 * @param[in]  t                tracker
 *
 * @return If value is suspected true is returned, otherwise false.
 */
//==============================================================================
static bool trk_suspect(const trk_t *t)
{
        if (!t->valid || (trk_growth(t) <= 0)) {
                return false;
        }

        return (t->rises >= 2) || (t->highs >= HIGHS_SUSPECT);
}

//==============================================================================
/**
 * @brief  Return name of memory category.
 *
 * @param[in]  cat      category
 *
 * @return Name.
 */
//==============================================================================
static const char *cat_name(uint cat)
{
        switch (cat) {
        case CAT_KERNEL:
                return "kernel";
        case CAT_FS:
                return "filesystems";
        case CAT_NET:
                return "network";
        case CAT_MOD:
                return "modules";
        case CAT_PROG:
                return "programs";
        case CAT_SHM:
                return "shared";
        case CAT_CACHE:
                return "cache";
        case CAT_UNOWNED:
                return "programs-unowned";
        default:
                return "?";
        }
}

//==============================================================================
/**
 * @brief  Return hint where to look for the leak in selected category.
 *
 * @param[in]  cat      category
 *
 * @return Hint.
 */
//==============================================================================
static const char *cat_hint(uint cat)
{
        switch (cat) {
        case CAT_KERNEL:
                return "kernel objects: threads, mutexes, semaphores, queues (see handles)";
        case CAT_FS:
                return "files growing in RAM file system (e.g. ramfs), opened files (see handles)";
        case CAT_NET:
                return "sockets and network buffers (see handles)";
        case CAT_MOD:
                return global->monitor_drivers ? "drivers (see driver entries)" : "drivers, run with -d";
        case CAT_PROG:
                return global->monitor_processes ? "malloc() of programs (see process entries)"
                                                 : "malloc() of programs, run with -p";
        case CAT_SHM:
                return "shared memory buffers";
        case CAT_CACHE:
                return "disc cache, growth is usually not a leak";
        case CAT_UNOWNED:
                return "program memory not owned by running processes (globals, exited processes)";
        default:
                return "";
        }
}

//==============================================================================
/**
 * @brief  Return name of handle type.
 *
 * @param[in]  hnd      handle type
 *
 * @return Name.
 */
//==============================================================================
static const char *hnd_name(uint hnd)
{
        switch (hnd) {
        case HND_FILE:
                return "files";
        case HND_DIR:
                return "dirs";
        case HND_MUTEX:
                return "mutexes";
        case HND_SEM:
                return "semaphores";
        case HND_QUEUE:
                return "queues";
        case HND_SOCKET:
                return "sockets";
        case HND_THREAD:
                return "threads";
        default:
                return "?";
        }
}

//==============================================================================
/**
 * @brief  Create string with handles opened since process was found.
 *
 * @param[in]  e        process entry
 * @param[out] buf      result buffer (empty string if no handle was opened)
 * @param[in]  len      buffer length
 */
//==============================================================================
static void handles_str(const proc_entry_t *e, char *buf, size_t len)
{
        size_t pos = 0;
        buf[0] = '\0';

        for (uint i = 0; i < HND_COUNT; i++) {
                int delta = (int)e->h_last[i] - (int)e->h_base[i];
                if ((delta > 0) && (pos < len)) {
                        int n = snprintf(buf + pos, len - pos, ", %s %+d", hnd_name(i), delta);
                        pos = (n > 0) ? min(pos + (size_t)n, len) : len;
                }
        }
}

/*==============================================================================
  End of file
==============================================================================*/
