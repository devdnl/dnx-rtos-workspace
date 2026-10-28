/*==============================================================================
@file   fstest.c

@author Daniel Zorychta

@brief  File system test application.

        Usage: fstest [-w|-R|-r|-g|-c|-l] [-v] [filter]

        Program must be started in a directory located on the tested file
        system (e.g. cd /mnt/newfs; fstest -v). All test objects are created
        with the "fst_" prefix and are removed after each test.

        Tests are divided into three groups:
          W  - write tests: create, modify and remove own objects,
          R  - read tests: only read reference data set (fst_ref/), never
               modify file system,
          RO - read-only checks: every modification of reference data set
               must be rejected (only in -r mode).

        (none)  write tests, then read tests on temporary reference data
        -w      write tests only
        -R      read tests only on existing reference data (any file system,
                nothing is modified)
        -r      read-only file system: read tests and read-only checks on
                existing reference data
        -g      create reference data (fst_ref/) and exit
        -c      remove reference data and exit
        -l      list tests
        -v      verbose mode (additional diagnostic output)
        filter  run only tests which name contains given text

        File system capabilities (case sensitivity of names, supported
        permission bits, maximum name length) are detected at start by the
        file system name returned by statfs() (FAT/exFAT: case insensitive,
        only read-only attribute; EEFS: names up to 20 characters). Detection
        can be overridden by FSTEST_CASE_SENSITIVE, FSTEST_FULL_PERMISSIONS
        and FSTEST_NAME_MAX configuration. Empty directories are not stored by
        ROMFS (FPKG image), so fst_ref/empty.dir is not checked on ROMFS
        (FSTEST_EMPTY_DIRS configuration).

        Testing of read-only file system:
          1. mount file system read-write (or use image generator on host
             machine, e.g. fstest compiled for host or fstest_ref.py),
          2. cd <mount point>; fstest -g
          3. remount file system read-only,
          4. cd <mount point>; fstest -r

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
/** Size of file used in big file and free space tests. */
#ifndef FSTEST_BIG_FILE_SIZE
#define FSTEST_BIG_FILE_SIZE (32 * 1024)
#endif

/** Number of files created in directory listing test (max 63). */
#ifndef FSTEST_DIR_FILES
#define FSTEST_DIR_FILES 40
#endif

/** Longest tested file name (FAT LFN: 255). */
#ifndef FSTEST_LONG_NAME_MAX
#define FSTEST_LONG_NAME_MAX 200
#endif

/**
 * Maximum length of file name supported by file system (without path).
 * Longer names are not tested, instead it is checked that name longer by one
 * character is rejected with ENAMETOOLONG.
 * >0: limit, -1: detect at run time by file system name (statfs()).
 */
#ifndef FSTEST_NAME_MAX
#define FSTEST_NAME_MAX -1
#endif

/** Enable free space test (requires statfs()). */
#ifndef FSTEST_USE_STATFS
#define FSTEST_USE_STATFS 1
#endif

/** Enable concurrent access test (requires threads). */
#ifndef FSTEST_USE_THREADS
#define FSTEST_USE_THREADS 1
#endif

/** Enable timestamp test (takes FSTEST_TIME_STEP seconds). */
#ifndef FSTEST_USE_TIMESTAMPS
#define FSTEST_USE_TIMESTAMPS 1
#endif

/**
 * File names are case sensitive.
 *  1: yes, 0: no (FAT), -1: detect at run time by file system name (statfs()).
 */
#ifndef FSTEST_CASE_SENSITIVE
#define FSTEST_CASE_SENSITIVE -1
#endif

/**
 * File system stores all permission bits.
 *  1: yes, 0: only write permission (FAT read-only attribute),
 * -1: detect at run time by file system name (statfs()).
 */
#ifndef FSTEST_FULL_PERMISSIONS
#define FSTEST_FULL_PERMISSIONS -1
#endif

/**
 * File system stores empty directories (read tests of reference data).
 *  1: yes, 0: no (ROMFS image skips empty directories),
 * -1: detect at run time by file system name (statfs()).
 */
#ifndef FSTEST_EMPTY_DIRS
#define FSTEST_EMPTY_DIRS -1
#endif

/** errno expected when read-only file system rejects modification (0: any). */
#ifndef FSTEST_RO_ERRNO
#define FSTEST_RO_ERRNO EROFS
#endif

#if FSTEST_DIR_FILES > 63
#error "FSTEST_DIR_FILES must be <= 63"
#endif

#define FSTEST_THREADS          3
#define FSTEST_THREAD_ROUNDS    4
#define FSTEST_THREAD_FILE_SIZE 3000
#define FSTEST_THREAD_BUF_SIZE  48
#define FSTEST_TIME_VALID_MIN   1577836800 /* 2020-01-01 */
#define FSTEST_TIME_TOLERANCE   5          /* seconds */
#define FSTEST_TIME_STEP        3          /* seconds */
#define FSTEST_IO_BUF_SIZE      1024
#define FSTEST_PATH_BUF_LEN     (FSTEST_LONG_NAME_MAX + 48)

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dnx/misc.h>
#include <dnx/thread.h>
#include <dnx/vt100.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#if FSTEST_USE_STATFS
#include <sys/statfs.h>
#endif

/*==============================================================================
  Local macros
==============================================================================*/
#define FILE_A         "fst_a.file"
#define FILE_B         "fst_b.file"
#define DIR_A          "fst_a.dir"
#define DIR_B          "fst_b.dir"
#define NO_FILE        "fst_none.file"
#define NO_DIR         "fst_none.dir"
#define DIR_A_FILE     DIR_A "/f.file"
#define DIR_A_SUB      DIR_A "/sub"
#define DIR_A_SUB_FILE DIR_A "/sub/f.file"
#define DIR_B_SUB      DIR_B "/sub"
#define DIR_B_SUB_FILE DIR_B "/sub/f.file"
#define MOVED_FILE     DIR_A "/moved.file"
#define NEST_1         DIR_A "/l1"
#define NEST_2         NEST_1 "/l2"
#define NEST_3         NEST_2 "/l3"
#define NEST_FILE      NEST_3 "/deep.file"

/*
 * Reference data set used by read tests. Content must be identical to the
 * one generated by fstest_ref.py.
 */
#define REF_DIR        "fst_ref"
#define REF_EMPTY_FILE REF_DIR "/empty.file"
#define REF_SMALL_FILE REF_DIR "/small.file"
#define REF_BIG_FILE   REF_DIR "/big.file"
#define REF_EMPTY_DIR  REF_DIR "/empty.dir"
#define REF_LIST_DIR   REF_DIR "/list.dir"
#define REF_NAMES_DIR  REF_DIR "/names.dir"
#define REF_SIZES_DIR  REF_DIR "/sizes.dir"
#define REF_NEST_1     REF_DIR "/l1"
#define REF_NEST_2     REF_NEST_1 "/l2"
#define REF_NEST_3     REF_NEST_2 "/l3"
#define REF_DEEP_FILE  REF_NEST_3 "/deep.file"
#define REF_NO_FILE    REF_DIR "/none.file"
#define REF_NO_DIR     REF_DIR "/none.dir"
#define REF_NEW_FILE   REF_DIR "/new.file"
#define REF_NEW_DIR    REF_DIR "/new.dir"
#define REF_SMALL_SIZE 100
#define REF_SMALL_SEED 0x01
#define REF_BIG_SIZE   (FSTEST_BIG_FILE_SIZE + 3)
#define REF_BIG_SEED   0x02
#define REF_DEEP_SIZE  1000
#define REF_DEEP_SEED  0x5A
#define REF_SIZES_SEED 0x10
#define REF_NAME_SIZE  20
#define REF_ROOT_ITEMS 8

#define SKIP_PREFIX   "SKIP: "
#define SKIP(_reason) (SKIP_PREFIX _reason)

/* network file system type (kernel enum SYS_FS_TYPE__NET, see struct statfs) */
#define FS_TYPE_NET 4

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
 * Operation is expected to fail. _failed is a condition that is true when
 * operation failed. _errno is expected errno value (0: any).
 */
#define EXPECT_FAIL(_failed, _errno, _what)                                                            \
        do {                                                                                           \
                errno = 0;                                                                             \
                bool _f = (_failed);                                                                   \
                int _e = errno;                                                                        \
                if (!_f) {                                                                             \
                        exit_msg = make_msg(0, "%s: succeeded but should fail", _what);                \
                        goto cleanup;                                                                  \
                }                                                                                      \
                if (((_errno) != 0) && (_e != (_errno))) {                                             \
                        exit_msg = make_msg(0, "%s: wrong errno %d (%s), expected %d (%s)", _what, _e, \
                                strerror(_e), (_errno), strerror(_errno));                             \
                        goto cleanup;                                                                  \
                }                                                                                      \
                if (global->verbose && (_errno) == 0) {                                                \
                        printf("\n  %s: errno %d (%s)", _what, _e, strerror(_e));                      \
                }                                                                                      \
        } while (0)

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        TEST_W,  //!< write test (modifies file system)
        TEST_R,  //!< read test (reads reference data only)
        TEST_RO, //!< read-only check (modification must be rejected)
} test_type_t;

typedef enum {
        MODE_ALL,       //!< write tests + read tests on temporary reference data
        MODE_WRITE,     //!< write tests only
        MODE_READ,      //!< read tests on existing reference data
        MODE_READ_ONLY, //!< read tests + read-only checks on existing reference data
        MODE_GENERATE,  //!< create reference data
        MODE_CLEAN,     //!< remove reference data
        MODE_LIST,      //!< list tests
} run_mode_t;

typedef struct {
        const char *name;
        const char *(*fn)(void);
        test_type_t type;
} test_entry_t;

typedef struct {
        const char *path;
        size_t size;
        u8_t seed;
} ref_file_t;

#if FSTEST_USE_THREADS
typedef struct {
        const char *path;
        size_t size;
        u8_t seed;
        size_t chunk;
        bool write;
        const char *result;
} fs_job_t;
#endif

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_usage(const char *name);
static void detect_fs_caps(void);
static mode_t expected_mode(mode_t set, bool dir);
static bool test_selected(const test_entry_t *test_entry, const char *filter);
static bool group_selected(test_type_t type, const char *filter);
static void list_tests(const char *filter);
static size_t run_group(test_type_t type, const char *filter);
static bool run_test(size_t no, const test_entry_t *test_entry);
static void print_byte_array(const char *title, const void *array, size_t array_size);
static const char *make_msg(int err, const char *fmt, ...);
static void remove_leftovers(void);
static const char *ref_generate(void);
static bool ref_remove(void);

static const char *test_file_create_and_remove(void);
static const char *test_nonexistent(void);
static const char *test_file_seek(void);
static const char *test_seek_tell_eof(void);
static const char *test_big_file(void);
static const char *test_truncate(void);
static const char *test_append(void);
static const char *test_update_modes(void);
static const char *test_two_handles(void);
static const char *test_multiple_open_files(void);
static const char *test_file_mode(void);
static const char *test_dir_create_and_remove(void);
static const char *test_dir_mode(void);
static const char *test_dir_not_empty(void);
static const char *test_dir_nested(void);
static const char *test_dir_listing(void);
static const char *test_file_rename(void);
static const char *test_dir_rename(void);
static const char *test_file_names(void);
#if FSTEST_USE_TIMESTAMPS
static const char *test_timestamps(void);
#endif
#if FSTEST_USE_STATFS
static const char *test_free_space(void);
#endif
#if FSTEST_USE_THREADS
static const char *test_threads(void);
#endif

static const char *test_ref_structure(void);
static const char *test_ref_nonexistent(void);
static const char *test_ref_read_data(void);
static const char *test_ref_seek(void);
static const char *test_ref_stat(void);
static const char *test_ref_two_handles(void);
static const char *test_ref_multiple_open_files(void);
static const char *test_ref_listing(void);
static const char *test_ref_nested(void);
static const char *test_ref_names(void);
#if FSTEST_USE_STATFS
static const char *test_ref_statfs(void);
#endif
#if FSTEST_USE_THREADS
static const char *test_ref_threads(void);
#endif

static const char *test_ro_file_ops(void);
static const char *test_ro_dir_ops(void);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t passed;
        size_t failed;
        size_t skipped;
        bool verbose;
        bool case_sensitive;    //!< file names are case sensitive
        bool full_permissions;  //!< all permission bits are stored
        size_t name_max;        //!< maximum tested file name length
        bool empty_dirs;        //!< empty directories are stored (reference data)
        char msg_buf[192];
        char path[2][FSTEST_PATH_BUF_LEN];
        u8_t wbuf[FSTEST_IO_BUF_SIZE];
        u8_t rbuf[FSTEST_IO_BUF_SIZE];
};

static const char *SUCCESS_MSG = "SUCCESS";

static const test_entry_t TEST_TAB[] = {
        // write tests
        {.type = TEST_W, .name = "File create and remove", .fn = test_file_create_and_remove},
        {.type = TEST_W, .name = "Operations on non-existing objects", .fn = test_nonexistent},
        {.type = TEST_W, .name = "Seek/Write/Read file operations", .fn = test_file_seek},
        {.type = TEST_W, .name = "Seek/tell and EOF handling", .fn = test_seek_tell_eof},
        {.type = TEST_W, .name = "Big file data integrity", .fn = test_big_file},
        {.type = TEST_W, .name = "File truncation by fopen(\"w\")", .fn = test_truncate},
        {.type = TEST_W, .name = "Append modes \"a\" and \"a+\"", .fn = test_append},
        {.type = TEST_W, .name = "Update modes \"r+\" and \"w+\"", .fn = test_update_modes},
        {.type = TEST_W, .name = "Two handles to the same file", .fn = test_two_handles},
        {.type = TEST_W, .name = "Several files written alternately", .fn = test_multiple_open_files},
        {.type = TEST_W, .name = "File mode change", .fn = test_file_mode},
        {.type = TEST_W, .name = "Directory create and remove", .fn = test_dir_create_and_remove},
        {.type = TEST_W, .name = "Directory mode change", .fn = test_dir_mode},
        {.type = TEST_W, .name = "Remove non-empty directory", .fn = test_dir_not_empty},
        {.type = TEST_W, .name = "Nested directories", .fn = test_dir_nested},
        {.type = TEST_W, .name = "Directory listing", .fn = test_dir_listing},
        {.type = TEST_W, .name = "File rename", .fn = test_file_rename},
        {.type = TEST_W, .name = "Directory rename", .fn = test_dir_rename},
        {.type = TEST_W, .name = "File names (length, characters)", .fn = test_file_names},
#if FSTEST_USE_TIMESTAMPS
        {.type = TEST_W, .name = "File timestamps", .fn = test_timestamps},
#endif
#if FSTEST_USE_STATFS
        {.type = TEST_W, .name = "Free space accounting", .fn = test_free_space},
#endif
#if FSTEST_USE_THREADS
        {.type = TEST_W, .name = "Concurrent access from threads", .fn = test_threads},
#endif

        // read tests (reference data)
        {.type = TEST_R, .name = "Reference data structure", .fn = test_ref_structure},
        {.type = TEST_R, .name = "Read of non-existing objects", .fn = test_ref_nonexistent},
        {.type = TEST_R, .name = "Read data integrity", .fn = test_ref_read_data},
        {.type = TEST_R, .name = "Read with seek/tell/EOF", .fn = test_ref_seek},
        {.type = TEST_R, .name = "Read stat()/fstat()", .fn = test_ref_stat},
        {.type = TEST_R, .name = "Read by two handles", .fn = test_ref_two_handles},
        {.type = TEST_R, .name = "Read several files alternately", .fn = test_ref_multiple_open_files},
        {.type = TEST_R, .name = "Read directory listing", .fn = test_ref_listing},
        {.type = TEST_R, .name = "Read nested directories", .fn = test_ref_nested},
        {.type = TEST_R, .name = "Read file names", .fn = test_ref_names},
#if FSTEST_USE_STATFS
        {.type = TEST_R, .name = "Read statfs()", .fn = test_ref_statfs},
#endif
#if FSTEST_USE_THREADS
        {.type = TEST_R, .name = "Read concurrently from threads", .fn = test_ref_threads},
#endif

        // read-only checks
        {.type = TEST_RO, .name = "Read-only: file modifications rejected", .fn = test_ro_file_ops},
        {.type = TEST_RO, .name = "Read-only: directory modifications rejected", .fn = test_ro_dir_ops},
};

static const char *const TEST_TYPE_NAME[] = {
        [TEST_W] = "W",
        [TEST_R] = "R",
        [TEST_RO] = "RO",
};

static const char *const TEST_GROUP_NAME[] = {
        [TEST_W] = "WRITE TESTS",
        [TEST_R] = "READ TESTS (" REF_DIR "/)",
        [TEST_RO] = "READ-ONLY CHECKS (" REF_DIR "/)",
};

static const char *const REF_DIRS[] = {
        REF_DIR,
        REF_EMPTY_DIR,
        REF_LIST_DIR,
        REF_NAMES_DIR,
        REF_SIZES_DIR,
        REF_NEST_1,
        REF_NEST_2,
        REF_NEST_3,
};

static const char *const REF_ROOT_NAMES[REF_ROOT_ITEMS] = {
        "empty.file",
        "small.file",
        "big.file",
        "empty.dir",
        "list.dir",
        "names.dir",
        "sizes.dir",
        "l1",
};

static const ref_file_t REF_FIXED_FILES[] = {
        {.path = REF_EMPTY_FILE, .size = 0, .seed = 0},
        {.path = REF_SMALL_FILE, .size = REF_SMALL_SIZE, .seed = REF_SMALL_SEED},
        {.path = REF_BIG_FILE, .size = REF_BIG_SIZE, .seed = REF_BIG_SEED},
        {.path = REF_DEEP_FILE, .size = REF_DEEP_SIZE, .seed = REF_DEEP_SEED},
};

/** Sizes around typical sector/block/cluster boundaries. */
static const size_t REF_BLOCK_SIZES[] = {1, 511, 512, 513, 1023, 1024, 1025, 4095, 4096, 4097};

static const size_t NAME_LEN[] = {1, 8, 12, 13, 16, 31, 32, 64, 128, FSTEST_LONG_NAME_MAX};

static const char *const SPECIAL_NAME[] = {
        "with space.txt",
        "multi.dot.name.ext",
        "UPPER.TXT",
        "MiXeD.CaSe",
        "noext",
        ".hidden",
        "dash-under_score~1",
        "plus+comma,eq=semi;",
        "brackets[(1)]",
        "hash#excl!at@",
};

static const char *const MULTI_FILE[] = {"fst_m0.file", "fst_m1.file", "fst_m2.file", "fst_m3.file"};

#if FSTEST_USE_THREADS
static const char *const THREAD_FILE[FSTEST_THREADS] = {"fst_t0.file", "fst_t1.file", "fst_t2.file"};
#endif

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(fstest, STACK_DEPTH_MEDIUM);

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

        for (int i = 1; i < argc; i++) {
                const char *arg = argv[i];

                if (isstreq(arg, "-v")) {
                        global->verbose = true;
                } else if (isstreq(arg, "-w")) {
                        mode = MODE_WRITE;
                } else if (isstreq(arg, "-R")) {
                        mode = MODE_READ;
                } else if (isstreq(arg, "-r")) {
                        mode = MODE_READ_ONLY;
                } else if (isstreq(arg, "-g")) {
                        mode = MODE_GENERATE;
                } else if (isstreq(arg, "-c")) {
                        mode = MODE_CLEAN;
                } else if (isstreq(arg, "-l")) {
                        mode = MODE_LIST;
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

        switch (mode) {
        case MODE_LIST:
                list_tests(filter);
                return EXIT_SUCCESS;

        case MODE_GENERATE: {
                const char *err = ref_generate();
                if (err) {
                        printf(VT100_FONT_COLOR_RED
                                "Unable to create reference data: %s" VT100_RESET_ATTRIBUTES "\n",
                                err);
                        return EXIT_FAILURE;
                }
                printf("Reference data created in '%s/'.\n", REF_DIR);
                return EXIT_SUCCESS;
        }

        case MODE_CLEAN:
                if (!ref_remove()) {
                        printf(VT100_FONT_COLOR_RED "Unable to remove reference data." VT100_RESET_ATTRIBUTES
                                                    "\n");
                        return EXIT_FAILURE;
                }
                printf("Reference data removed.\n");
                return EXIT_SUCCESS;

        default:
                break;
        }

        detect_fs_caps();

        process_stat_t proc_stat = {0};
        process_stat(getpid(), &proc_stat);
        u16_t stack_max_usage = proc_stat.stack_max_usage;

        size_t executed = 0;

        if ((mode == MODE_ALL) || (mode == MODE_WRITE)) {
                remove_leftovers();
                executed += run_group(TEST_W, filter);
        }

        if ((mode == MODE_ALL) && group_selected(TEST_R, filter)) {
                ref_remove();

                const char *err = ref_generate();
                if (err) {
                        printf("\n" VT100_FONT_COLOR_RED
                               "Unable to create reference data: %s" VT100_RESET_ATTRIBUTES "\n",
                                err);
                        global->failed++;
                } else {
                        executed += run_group(TEST_R, filter);
                }

                if (!ref_remove()) {
                        printf(VT100_FONT_COLOR_RED "Unable to remove reference data." VT100_RESET_ATTRIBUTES
                                                    "\n");
                        global->failed++;
                }
        }

        if ((mode == MODE_READ) || (mode == MODE_READ_ONLY)) {
                struct stat st;
                if ((stat(REF_DIR, &st) != 0) || !S_ISDIR(st.st_mode)) {
                        printf(VT100_FONT_COLOR_RED "Reference data '%s/' not found." VT100_RESET_ATTRIBUTES
                                                    "\n"
                                                    "Create it on writable file system by 'fstest -g' "
                                                    "(or fstest_ref.py on host machine).\n",
                                REF_DIR);
                        return EXIT_FAILURE;
                }

                executed += run_group(TEST_R, filter);

                if (mode == MODE_READ_ONLY) {
                        executed += run_group(TEST_RO, filter);
                }
        }

        process_stat(getpid(), &proc_stat);
        int fs_stack_usage = proc_stat.stack_max_usage - stack_max_usage;

        puts("");
        printf(VT100_FONT_COLOR_YELLOW "SUMMARY:" VT100_RESET_ATTRIBUTES
                                       " executed: %lu, passed: %lu, failed: %lu, skipped: %lu\n",
                (unsigned long)executed, (unsigned long)global->passed, (unsigned long)global->failed,
                (unsigned long)global->skipped);

        printf("Stack used for file system: %i levels.\n", fs_stack_usage);

        return (global->failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//==============================================================================
/**
 * @brief  Print program usage.
 */
//==============================================================================
static void print_usage(const char *name)
{
        printf("Usage: %s [-w|-R|-r|-g|-c|-l] [-v] [filter]\n", name);
        puts("  (none)  write tests, then read tests on temporary reference data");
        puts("  -w      write tests only");
        puts("  -R      read tests only on existing reference data (nothing is modified)");
        puts("  -r      read-only file system: read tests and read-only checks");
        puts("  -g      create reference data (" REF_DIR "/) and exit");
        puts("  -c      remove reference data and exit");
        puts("  -l      list tests");
        puts("  -v      verbose mode");
        puts("  filter  run only tests which name contains given text");
}

//==============================================================================
/**
 * @brief  Detect capabilities of tested file system.
 *
 * FAT (FAT12/16/32, exFAT) does not distinguish case of file names and has
 * only read-only attribute instead of permission bits.
 */
//==============================================================================
static void detect_fs_caps(void)
{
        bool fat = false;
        bool eefs = false;
        bool romfs = false;
        const char *fsname = "?";

#if FSTEST_USE_STATFS
        char *cwd = global->path[1];
        struct statfs sfs;

        if ((getcwd(cwd, FSTEST_PATH_BUF_LEN) != NULL) && (statfs(cwd, &sfs) == 0) && sfs.f_fsname) {
                fsname = sfs.f_fsname;
                fat = (strstr(fsname, "FAT") != NULL) || (strstr(fsname, "fat") != NULL);
                eefs = (strcmp(fsname, "eefs") == 0);
                romfs = (strcmp(fsname, "romfs") == 0);
        }
#endif

#if FSTEST_CASE_SENSITIVE < 0
        global->case_sensitive = !fat;
#else
        global->case_sensitive = FSTEST_CASE_SENSITIVE;
#endif

#if FSTEST_FULL_PERMISSIONS < 0
        global->full_permissions = !fat;
#else
        global->full_permissions = FSTEST_FULL_PERMISSIONS;
#endif

#if FSTEST_NAME_MAX < 0
        global->name_max = eefs ? 20 : FSTEST_LONG_NAME_MAX; // EEFS: name[21] with nul
#else
        global->name_max = FSTEST_NAME_MAX;
#endif
        (void)eefs;

#if FSTEST_EMPTY_DIRS < 0
        global->empty_dirs = !romfs;
#else
        global->empty_dirs = FSTEST_EMPTY_DIRS;
#endif
        (void)romfs;

        if (global->name_max > FSTEST_LONG_NAME_MAX) {
                global->name_max = FSTEST_LONG_NAME_MAX;
        }

        printf("File system: %s (names: %s, max %u chars, permissions: %s%s)\n", fsname,
                global->case_sensitive ? "case sensitive" : "case insensitive", (unsigned)global->name_max,
                global->full_permissions ? "full" : "write bit only",
                global->empty_dirs ? "" : ", no empty directories");
}

//==============================================================================
/**
 * @brief  Mode expected after chmod() on tested file system.
 *
 * @param  set          mode given to chmod()
 * @param  dir          object is a directory
 *
 * @return Expected permission bits.
 */
//==============================================================================
static mode_t expected_mode(mode_t set, bool dir)
{
        if (global->full_permissions) {
                return set & S_IPMT;
        }

        /*
         * Read-only attribute only: write permission is stored, other bits are
         * fixed (files rw-rw-rw-, directories rwxrwxrwx).
         */
        mode_t mode = (set & S_IWUSR) ? 0666 : 0444;

        if (dir) {
                mode |= 0111;
        }

        return mode;
}

//==============================================================================
/**
 * @brief  Check if test is selected by filter.
 */
//==============================================================================
static bool test_selected(const test_entry_t *test_entry, const char *filter)
{
        return !filter || strstr(test_entry->name, filter);
}

//==============================================================================
/**
 * @brief  Check if any test of given type is selected by filter.
 */
//==============================================================================
static bool group_selected(test_type_t type, const char *filter)
{
        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                if ((TEST_TAB[i].type == type) && test_selected(&TEST_TAB[i], filter)) {
                        return true;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief  Print list of tests.
 */
//==============================================================================
static void list_tests(const char *filter)
{
        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                if (test_selected(&TEST_TAB[i], filter)) {
                        printf("%2u  %s  %s\n", (unsigned)(i + 1), TEST_TYPE_NAME[TEST_TAB[i].type],
                                TEST_TAB[i].name);
                }
        }
}

//==============================================================================
/**
 * @brief  Run all selected tests of given type.
 *
 * @return Number of executed tests.
 */
//==============================================================================
static size_t run_group(test_type_t type, const char *filter)
{
        size_t executed = 0;

        if (!group_selected(type, filter)) {
                return 0;
        }

        printf("\n" VT100_FONT_COLOR_YELLOW "%s" VT100_RESET_ATTRIBUTES "\n", TEST_GROUP_NAME[type]);

        for (size_t i = 0; i < ARRAY_SIZE(TEST_TAB); i++) {
                if ((TEST_TAB[i].type == type) && test_selected(&TEST_TAB[i], filter)) {
                        run_test(i + 1, &TEST_TAB[i]);
                        executed++;
                }
        }

        return executed;
}

//==============================================================================
/**
 * @brief  Run single test and print result.
 *
 * @param  no           test number
 * @param  test_entry   test entry
 *
 * @return true if test passed or was skipped.
 */
//==============================================================================
static bool run_test(size_t no, const test_entry_t *test_entry)
{
        printf(VT100_FONT_COLOR_CYAN "TEST %2lu:" VT100_RESET_ATTRIBUTES " %s... ", (unsigned long)no,
                test_entry->name);
        fflush(stdout);

        const char *msg = test_entry->fn();

        if (global->verbose) {
                puts("");
        }

        if (isstreq(msg, SUCCESS_MSG)) {
                printf(VT100_FONT_COLOR_GREEN "[SUCCESS]" VT100_RESET_ATTRIBUTES "\n");
                global->passed++;
                return true;

        } else if (strncmp(msg, SKIP_PREFIX, strlen(SKIP_PREFIX)) == 0) {
                printf(VT100_FONT_COLOR_YELLOW "[SKIPPED]" VT100_RESET_ATTRIBUTES " %s\n",
                        msg + strlen(SKIP_PREFIX));
                global->skipped++;
                return true;

        } else {
                printf(VT100_FONT_COLOR_RED "[FAILURE]" VT100_RESET_ATTRIBUTES "\n");
                printf(VT100_FONT_COLOR_RED "=> %s" VT100_RESET_ATTRIBUTES "\n", msg);
                global->failed++;
                return false;
        }
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
 * @brief  Print byte array.
 *
 * @param  title        title
 * @param  array        array to print
 * @param  array_size   array size
 */
//==============================================================================
static void print_byte_array(const char *title, const void *array, size_t array_size)
{
        const u8_t *byte = array;
        size_t offset = 0;

        if (title) {
                printf("%s (%lu bytes long):\n", title, (unsigned long)array_size);
        }

        while (array_size > 0) {
                size_t col_len = min(16, array_size);

                printf("%04X: ", (unsigned)offset);

                for (size_t col = 0; col < col_len; col++) {
                        printf("%02X ", byte[col]);
                }

                for (size_t col = 0; col < 16 - col_len; col++) {
                        printf("   ");
                }

                for (size_t col = 0; col < col_len; col++) {
                        unsigned char c = byte[col];

                        if (c == ' ') {
                                // as is
                        } else if (isspace(c)) {
                                c = ' ';
                        } else if (isprint(c)) {
                                // as is
                        } else {
                                c = '.';
                        }

                        printf("%c", c);
                }

                printf("\n");

                array_size -= col_len;
                offset += col_len;
                byte += col_len;
        }
}

//==============================================================================
/**
 * @brief  Print the most important stat fields.
 */
//==============================================================================
static void print_stat(const char *title, const struct stat *st)
{
        printf("\n  %-8s size=%llu mode=%lo uid=%u gid=%u dev=%li mtime=%lli ctime=%lli", title,
                (unsigned long long)st->st_size, (unsigned long)st->st_mode, (unsigned)st->st_uid,
                (unsigned)st->st_gid, (long)st->st_dev, (long long)st->st_mtime, (long long)st->st_ctime);
}

//==============================================================================
/**
 * @brief  Close file (if opened) and clear pointer.
 *
 * @return fclose() result or 0 if file was not opened.
 */
//==============================================================================
static int fclose_null(FILE **fp)
{
        int r = 0;

        if (*fp) {
                r = fclose(*fp);
                *fp = NULL;
        }

        return r;
}

//==============================================================================
/**
 * @brief  Return file size or -1 on error.
 */
//==============================================================================
static long long file_size(const char *path)
{
        struct stat st;
        return (stat(path, &st) == 0) ? (long long)st.st_size : -1;
}

//==============================================================================
/**
 * @brief  Test data pattern. Byte depends on absolute file position, so any
 *         misplaced block (wrong cluster/offset) is detected.
 */
//==============================================================================
static u8_t pattern(size_t pos, u8_t seed)
{
        return (u8_t)((pos * 7u) ^ (pos >> 7) ^ (pos >> 13) ^ (seed * 29u));
}

//==============================================================================
/**
 * @brief  Write pattern to file at current position.
 *
 * @param  fp       file
 * @param  pos      pattern position of first byte
 * @param  size     number of bytes
 * @param  seed     pattern seed
 * @param  buf      work buffer (at least chunk bytes)
 * @param  chunk    single write size
 *
 * @return NULL on success, otherwise static error description.
 */
//==============================================================================
static const char *write_pattern(FILE *fp, size_t pos, size_t size, u8_t seed, u8_t *buf, size_t chunk)
{
        while (size > 0) {
                size_t n = min(chunk, size);

                for (size_t i = 0; i < n; i++) {
                        buf[i] = pattern(pos + i, seed);
                }

                if (fwrite(buf, 1, n, fp) != n) {
                        return "write error";
                }

                pos += n;
                size -= n;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Read data from current position and compare with pattern.
 *
 * @return NULL on success, otherwise static error description.
 */
//==============================================================================
static const char *check_pattern(FILE *fp, size_t pos, size_t size, u8_t seed, u8_t *buf, size_t chunk)
{
        while (size > 0) {
                size_t n = min(chunk, size);

                if (fread(buf, 1, n, fp) != n) {
                        return ferror(fp) ? "read error" : "unexpected end of file";
                }

                for (size_t i = 0; i < n; i++) {
                        if (buf[i] != pattern(pos + i, seed)) {
                                return "data mismatch";
                        }
                }

                pos += n;
                size -= n;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Create file (mode "w") filled with pattern.
 *
 * @return NULL on success, otherwise static error description.
 */
//==============================================================================
static const char *write_pattern_file(const char *path, size_t size, u8_t seed, u8_t *buf, size_t chunk)
{
        FILE *fp = fopen(path, "w");
        if (!fp) {
                return "unable to create file";
        }

        const char *err = write_pattern(fp, 0, size, seed, buf, chunk);
        int e = errno;

        if ((fclose(fp) != 0) && !err) {
                err = "unable to close file";
        } else if (err) {
                errno = e;
        }

        return err;
}

//==============================================================================
/**
 * @brief  Check that file contains exactly size bytes of pattern.
 *
 * @return NULL on success, otherwise static error description.
 */
//==============================================================================
static const char *verify_pattern_file(const char *path, size_t size, u8_t seed, u8_t *buf, size_t chunk)
{
        FILE *fp = fopen(path, "r");
        if (!fp) {
                return "unable to open file for reading";
        }

        const char *err = check_pattern(fp, 0, size, seed, buf, chunk);

        if (!err && ((fgetc(fp) != EOF) || !feof(fp))) {
                err = "file is longer than expected";
        }

        fclose(fp);

        return err;
}

//==============================================================================
/**
 * @brief  Check if directory contains entry with exactly given name.
 */
//==============================================================================
static bool dir_contains(const char *dir_path, const char *name)
{
        bool found = false;

        DIR *dir = opendir(dir_path);
        if (dir) {
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL) {
                        if (isstreq(ent->d_name, name)) {
                                found = true;
                                break;
                        }
                }

                closedir(dir);
        }

        return found;
}

//==============================================================================
/**
 * @brief  Path of file used in directory listing test.
 */
//==============================================================================
static char *listing_path(char *buf, const char *dir, unsigned idx)
{
        snprintf(buf, FSTEST_PATH_BUF_LEN, "%s/f%03u", dir, idx);
        return buf;
}

//==============================================================================
/**
 * @brief  Parse name of file used in directory listing test ("fNNN").
 */
//==============================================================================
static bool parse_listing_name(const char *name, unsigned *idx)
{
        if ((name[0] != 'f') || (strlen(name) != 4)) {
                return false;
        }

        unsigned v = 0;
        for (int i = 1; i < 4; i++) {
                if (!isdigit((unsigned char)name[i])) {
                        return false;
                }
                v = v * 10 + (unsigned)(name[i] - '0');
        }

        if (v >= FSTEST_DIR_FILES) {
                return false;
        }

        *idx = v;
        return true;
}

//==============================================================================
/**
 * @brief  Read directory and check that exactly expected files are listed.
 *
 * @param  path         directory path
 * @param  expected     bit mask of expected "fNNN" files
 *
 * @return NULL on success, otherwise static error description.
 */
//==============================================================================
static const char *scan_listing(const char *path, u64_t expected)
{
        const char *err = NULL;
        u64_t seen = 0;

        errno = 0;
        DIR *dir = opendir(path);
        if (!dir) {
                return "opendir() failed";
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
                const char *name = ent->d_name;
                unsigned idx;

                if (isstreq(name, ".") || isstreq(name, "..")) {
                        continue;
                }

                if (!parse_listing_name(name, &idx)) {
                        if (global->verbose) {
                                printf("\n  unexpected entry: '%s'", name);
                        }
                        err = "unexpected entry in directory";
                        break;
                }

                u64_t bit = (u64_t)1 << idx;

                if (seen & bit) {
                        if (global->verbose) {
                                printf("\n  entry listed twice: '%s'", name);
                        }
                        err = "entry listed twice";
                        break;
                }

                seen |= bit;
        }

        closedir(dir);

        if (!err && (seen != expected)) {
                if (global->verbose) {
                        printf("\n  listed: %016llX, expected: %016llX", (unsigned long long)seen,
                                (unsigned long long)expected);
                }

                err = (expected & ~seen) ? "entry missing in directory listing"
                                         : "removed entry still listed";
        }

        return err;
}

//==============================================================================
/**
 * @brief  Build path of long name file: <dir>/<len characters>.
 *
 * @return Pointer to name part of path.
 */
//==============================================================================
static const char *long_name_path(char *buf, const char *dir, size_t len)
{
        static const char CHARS[] = "abcdefghijklmnopqrstuvwxyz0123456789";

        int n = snprintf(buf, FSTEST_PATH_BUF_LEN, "%s/", dir);

        for (size_t k = 0; k < len; k++) {
                buf[n + k] = CHARS[k % (sizeof(CHARS) - 1)];
        }

        buf[n + len] = '\0';

        return buf + n;
}

//==============================================================================
/**
 * @brief  Remove objects that could remain after interrupted test session.
 */
//==============================================================================
static void remove_leftovers(void)
{
        static const char *const PATHS[] = {
                NEST_FILE,
                NEST_3,
                NEST_2,
                NEST_1,
                DIR_A_SUB_FILE,
                DIR_A_SUB,
                DIR_A_FILE,
                MOVED_FILE,
                DIR_B_SUB_FILE,
                DIR_B_SUB,
                DIR_A,
                DIR_B,
                FILE_A,
                FILE_B,
        };

        char *path = global->path[0];

        chmod(FILE_A, 0666);
        chmod(DIR_A, 0777);

        for (unsigned i = 0; i < FSTEST_DIR_FILES; i++) {
                remove(listing_path(path, DIR_A, i));
        }

        for (size_t i = 0; i < ARRAY_SIZE(NAME_LEN); i++) {
                if (NAME_LEN[i] <= global->name_max) {
                        long_name_path(path, DIR_A, NAME_LEN[i]);
                        remove(path);
                }
        }

        for (size_t i = 0; i < ARRAY_SIZE(SPECIAL_NAME); i++) {
                snprintf(path, FSTEST_PATH_BUF_LEN, DIR_A "/%s", SPECIAL_NAME[i]);
                remove(path);
        }

        for (size_t i = 0; i < ARRAY_SIZE(PATHS); i++) {
                remove(PATHS[i]);
        }

        for (size_t i = 0; i < ARRAY_SIZE(MULTI_FILE); i++) {
                remove(MULTI_FILE[i]);
        }

#if FSTEST_USE_THREADS
        for (size_t i = 0; i < ARRAY_SIZE(THREAD_FILE); i++) {
                remove(THREAD_FILE[i]);
        }
#endif
}

//==============================================================================
/**
 * @brief  Number of reference root directory entries (empty directory is not
 *         stored by some file systems).
 *
 * @return Number of entries.
 */
//==============================================================================
static int ref_root_items(void)
{
        return global->empty_dirs ? REF_ROOT_ITEMS : (REF_ROOT_ITEMS - 1);
}

//==============================================================================
/**
 * @brief  Count directory entries (without "." and "..").
 *
 * @return Number of entries or -1 on error.
 */
//==============================================================================
static int count_entries(const char *dir_path)
{
        int count = 0;

        DIR *dir = opendir(dir_path);
        if (!dir) {
                return -1;
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
                if (!isstreq(ent->d_name, ".") && !isstreq(ent->d_name, "..")) {
                        count++;
                }
        }

        closedir(dir);

        return count;
}

//==============================================================================
/**
 * @brief  Get reference file description.
 *
 * Order of files: fixed files, block size files, listing files, long name
 * files, special name files.
 *
 * @param  idx      file index
 * @param  buf      path buffer (FSTEST_PATH_BUF_LEN), used by generated names
 * @param  rf       file description
 *
 * @return 1 if entry exists, 0 if entry is not used (skip), -1 end of list.
 */
//==============================================================================
static int ref_file_get(size_t idx, char *buf, ref_file_t *rf)
{
        if (idx < ARRAY_SIZE(REF_FIXED_FILES)) {
                *rf = REF_FIXED_FILES[idx];
                return 1;
        }
        idx -= ARRAY_SIZE(REF_FIXED_FILES);

        if (idx < ARRAY_SIZE(REF_BLOCK_SIZES)) {
                snprintf(buf, FSTEST_PATH_BUF_LEN, REF_SIZES_DIR "/%u", (unsigned)REF_BLOCK_SIZES[idx]);
                rf->path = buf;
                rf->size = REF_BLOCK_SIZES[idx];
                rf->seed = (u8_t)(REF_SIZES_SEED + idx);
                return 1;
        }
        idx -= ARRAY_SIZE(REF_BLOCK_SIZES);

        if (idx < FSTEST_DIR_FILES) {
                rf->path = listing_path(buf, REF_LIST_DIR, idx);
                rf->size = idx;
                rf->seed = (u8_t)idx;
                return 1;
        }
        idx -= FSTEST_DIR_FILES;

        if (idx < ARRAY_SIZE(NAME_LEN)) {
                if (NAME_LEN[idx] > global->name_max) {
                        return 0;
                }
                long_name_path(buf, REF_NAMES_DIR, NAME_LEN[idx]);
                rf->path = buf;
                rf->size = NAME_LEN[idx];
                rf->seed = (u8_t)NAME_LEN[idx];
                return 1;
        }
        idx -= ARRAY_SIZE(NAME_LEN);

        if (idx < ARRAY_SIZE(SPECIAL_NAME)) {
                snprintf(buf, FSTEST_PATH_BUF_LEN, REF_NAMES_DIR "/%s", SPECIAL_NAME[idx]);
                rf->path = buf;
                rf->size = REF_NAME_SIZE;
                rf->seed = (u8_t)idx;
                return 1;
        }

        return -1;
}

//==============================================================================
/**
 * @brief  Number of entries expected in names directory.
 */
//==============================================================================
static int ref_names_count(void)
{
        int count = ARRAY_SIZE(SPECIAL_NAME);

        for (size_t i = 0; i < ARRAY_SIZE(NAME_LEN); i++) {
                if (NAME_LEN[i] <= global->name_max) {
                        count++;
                }
        }

        return count;
}

//==============================================================================
/**
 * @brief  Create reference data set (existing files are overwritten).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *ref_generate(void)
{
        char *path = global->path[1];

        for (size_t i = 0; i < ARRAY_SIZE(REF_DIRS); i++) {
                errno = 0;
                if ((mkdir(REF_DIRS[i], 0777) != 0) && (errno != EEXIST)) {
                        return make_msg(errno, "mkdir(\"%s\") failed", REF_DIRS[i]);
                }
        }

        ref_file_t rf;
        int r;

        for (size_t i = 0; (r = ref_file_get(i, path, &rf)) >= 0; i++) {
                if (r == 0) {
                        continue;
                }

                errno = 0;
                const char *err = write_pattern_file(rf.path, rf.size, rf.seed, global->wbuf,
                        FSTEST_IO_BUF_SIZE);
                if (err) {
                        return make_msg(errno, "%s: %s", rf.path, err);
                }
        }

        sync();

        return NULL;
}

//==============================================================================
/**
 * @brief  Remove reference data set.
 *
 * @return true if reference data does not exist after operation.
 */
//==============================================================================
static bool ref_remove(void)
{
        char *path = global->path[1];
        ref_file_t rf;
        int r;
        struct stat st;

        for (size_t i = 0; (r = ref_file_get(i, path, &rf)) >= 0; i++) {
                if (r > 0) {
                        remove(rf.path);
                }
        }

        remove(REF_NEW_FILE);
        remove(REF_NEW_DIR);

        for (size_t i = ARRAY_SIZE(REF_DIRS); i > 0; i--) {
                remove(REF_DIRS[i - 1]);
        }

        return stat(REF_DIR, &st) != 0;
}

/*==============================================================================
  Tests
==============================================================================*/

//==============================================================================
/**
 * @brief  Create empty file, check it and remove.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_create_and_remove(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat st;

        CHECK((fp = fopen(FILE_A, "w")) != NULL, "unable to create test file");
        CHECK(fclose_null(&fp) == 0, "unable to close file");

        CHECK(stat(FILE_A, &st) == 0, "created file does not exist");
        CHECK(S_ISREG(st.st_mode), "created object is not a regular file (mode: %lo)",
                (unsigned long)st.st_mode);
        CHECK(st.st_size == 0, "new file is not empty (%llu)", (unsigned long long)st.st_size);

        CHECK(remove(FILE_A) == 0, "unable to remove file");

        EXPECT_FAIL(stat(FILE_A, &st) != 0, ENOENT, "stat() of removed file");
        EXPECT_FAIL((fp = fopen(FILE_A, "r")) == NULL, ENOENT, "fopen() of removed file");

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Operations on non-existing files and directories must fail with ENOENT.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_nonexistent(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat st;

        EXPECT_FAIL((fp = fopen(NO_FILE, "r")) == NULL, ENOENT, "fopen(\"r\") of non-existing file");
        EXPECT_FAIL((fp = fopen(NO_FILE, "r+")) == NULL, ENOENT, "fopen(\"r+\") of non-existing file");
        EXPECT_FAIL(stat(NO_FILE, &st) != 0, ENOENT, "stat() of non-existing file");
        EXPECT_FAIL(remove(NO_FILE) != 0, ENOENT, "remove() of non-existing file");
        EXPECT_FAIL(rename(NO_FILE, FILE_B) != 0, ENOENT, "rename() of non-existing file");
        EXPECT_FAIL(chmod(NO_FILE, 0666) != 0, ENOENT, "chmod() of non-existing file");
        EXPECT_FAIL((fp = fopen(NO_DIR "/" NO_FILE, "w")) == NULL, ENOENT,
                "fopen(\"w\") in non-existing directory");
        EXPECT_FAIL(mkdir(NO_DIR "/" NO_DIR, 0777) != 0, ENOENT, "mkdir() in non-existing directory");

        CHECK(stat(FILE_B, &st) != 0, "rename() of non-existing file created destination");

cleanup:
        fclose_null(&fp);
        remove(FILE_B);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Seek beyond end of new file, write blocks and read back (gap must
 *         be filled with zeros).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_seek(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat st;
        u8_t buf[16];

        static const u8_t block[5] = {0x51, 0x52, 0x53, 0x54, 0x55};
        const size_t file_seek = 10; // must be <= sizeof(buf)
        const size_t blocks = 10;
        const size_t expected_size = file_seek + (blocks * sizeof(block));

        /*
         * Write test.
         */
        CHECK((fp = fopen(FILE_A, "w")) != NULL, "unable to open test file");
        CHECK(fseek(fp, file_seek, SEEK_SET) == 0, "unable to seek file position");
        CHECK(ftell(fp) == (long)file_seek, "ftell() returned wrong position");

        for (size_t i = 0; i < blocks; i++) {
                CHECK(fwrite(block, 1, sizeof(block), fp) == sizeof(block), "block %u write error",
                        (unsigned)i);
        }

        CHECK(fflush(fp) == 0, "fflush() error");
        CHECK(fstat(fileno(fp), &st) == 0, "unable to get file statistics");
        CHECK((long long)st.st_size == (long long)expected_size,
                "file size is not correct: %llu, expected %u", (unsigned long long)st.st_size,
                (unsigned)expected_size);

        /*
         * Read test.
         */
        CHECK((fp = freopen(FILE_A, "r", fp)) != NULL, "unable to reopen file to read");

        CHECK(fread(buf, 1, file_seek, fp) == file_seek, "unable to read file gap");

        for (size_t i = 0; i < file_seek; i++) {
                CHECK(buf[i] == 0, "gap byte %u is not zero (0x%02X)", (unsigned)i, buf[i]);
        }

        for (size_t i = 0; i < blocks; i++) {
                CHECK(fread(buf, 1, sizeof(block), fp) == sizeof(block), "read block %u error", (unsigned)i);
                CHECK(memcmp(buf, block, sizeof(block)) == 0, "invalid block %u data", (unsigned)i);
        }

        CHECK((fread(buf, 1, 1, fp) == 0) && feof(fp), "data after expected end of file");

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  fseek()/ftell() in all modes, partial read at end, EOF flag.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_seek_tell_eof(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        long long size;
        const u8_t seed = 0x77;

        CHECK((err = write_pattern_file(FILE_A, 200, seed, global->wbuf, 64)) == NULL, "%s", err);

        CHECK((fp = fopen(FILE_A, "r")) != NULL, "unable to open file");
        CHECK(ftell(fp) == 0, "initial position is not 0");

        CHECK((fseek(fp, 50, SEEK_SET) == 0) && (ftell(fp) == 50), "SEEK_SET failed");
        CHECK(fgetc(fp) == pattern(50, seed), "wrong byte at position 50");
        CHECK(ftell(fp) == 51, "position not incremented after read");

        CHECK((fseek(fp, -11, SEEK_CUR) == 0) && (ftell(fp) == 40), "SEEK_CUR backward failed");
        CHECK(fgetc(fp) == pattern(40, seed), "wrong byte at position 40");

        CHECK((fseek(fp, 20, SEEK_CUR) == 0) && (ftell(fp) == 61), "SEEK_CUR forward failed");
        CHECK(fgetc(fp) == pattern(61, seed), "wrong byte at position 61");

        CHECK((fseek(fp, -1, SEEK_END) == 0) && (ftell(fp) == 199), "SEEK_END failed");
        CHECK(fgetc(fp) == pattern(199, seed), "wrong last byte");
        CHECK((fgetc(fp) == EOF) && feof(fp) && !ferror(fp), "EOF not reported at end of file");

        CHECK((fseek(fp, 0, SEEK_END) == 0) && (ftell(fp) == 200), "SEEK_END to file end failed");
        CHECK(!feof(fp), "fseek() did not clear EOF flag");

        EXPECT_FAIL(fseek(fp, -1, SEEK_SET) != 0, EINVAL, "fseek() to negative position");
        CHECK(ftell(fp) == 200, "position changed after failed fseek()");

        CHECK(fseek(fp, 190, SEEK_SET) == 0, "fseek() failed");
        CHECK(fread(global->rbuf, 1, 32, fp) == 10, "partial read at end returned wrong size");
        CHECK(feof(fp) && !ferror(fp), "partial read at end did not set EOF");

        CHECK(fseek(fp, 1000, SEEK_SET) == 0, "seek beyond end of file failed");
        CHECK((fread(global->rbuf, 1, 1, fp) == 0) && feof(fp), "read beyond end of file returned data");

        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 200, "file size changed after reading: %lld", size);

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Write/read file bigger than FS block/cluster, unaligned chunks,
 *         overwrite with smaller file, grow again.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_big_file(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        long long size;
        const size_t size2 = (FSTEST_BIG_FILE_SIZE / 2) + 1;
        u8_t *wbuf = global->wbuf;
        u8_t *rbuf = global->rbuf;

        // pass 1: unaligned writes, aligned and unaligned reads
        CHECK((err = write_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE, 0x11, wbuf, 97)) == NULL,
                "pass 1 write: %s", err);
        CHECK((size = file_size(FILE_A)) == FSTEST_BIG_FILE_SIZE, "pass 1: wrong size %lld", size);
        CHECK((err = verify_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE, 0x11, rbuf, FSTEST_IO_BUF_SIZE)) ==
                        NULL,
                "pass 1 read: %s", err);
        CHECK((err = verify_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE, 0x11, rbuf, 13)) == NULL,
                "pass 1 read (13 B chunks): %s", err);

        // pass 2: shrink (truncation + free of blocks)
        CHECK((err = write_pattern_file(FILE_A, size2, 0x22, wbuf, FSTEST_IO_BUF_SIZE)) == NULL,
                "pass 2 write: %s", err);
        CHECK((size = file_size(FILE_A)) == (long long)size2, "pass 2: wrong size %lld", size);
        CHECK((err = verify_pattern_file(FILE_A, size2, 0x22, rbuf, 61)) == NULL, "pass 2 read: %s", err);

        // pass 3: grow again (reallocation of freed blocks)
        CHECK((err = write_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE + 3, 0x33, wbuf, 1)) == NULL,
                "pass 3 write: %s", err);
        CHECK((size = file_size(FILE_A)) == FSTEST_BIG_FILE_SIZE + 3, "pass 3: wrong size %lld", size);
        CHECK((err = verify_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE + 3, 0x33, rbuf, FSTEST_IO_BUF_SIZE)) ==
                        NULL,
                "pass 3 read: %s", err);

cleanup:
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Opening existing file with "w" must truncate it.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_truncate(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        long long size;

        CHECK((err = write_pattern_file(FILE_A, 1000, 0x44, global->wbuf, 64)) == NULL, "%s", err);
        CHECK((size = file_size(FILE_A)) == 1000, "wrong initial size: %lld", size);

        CHECK((fp = fopen(FILE_A, "w")) != NULL, "unable to reopen file");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 0, "file not truncated, size: %lld", size);

        CHECK((fp = fopen(FILE_A, "r")) != NULL, "unable to open file");
        CHECK((fgetc(fp) == EOF) && feof(fp) && !ferror(fp), "truncated file is not empty");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        // write after truncation must not reveal old data
        CHECK((err = write_pattern_file(FILE_A, 10, 0x45, global->wbuf, 64)) == NULL, "%s", err);
        CHECK((err = verify_pattern_file(FILE_A, 10, 0x45, global->rbuf, 64)) == NULL, "after rewrite: %s",
                err);

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Append modes: writes always go to end of file.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_append(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        long long size;
        const u8_t seed = 0x55;
        u8_t *wbuf = global->wbuf;
        u8_t *rbuf = global->rbuf;

        CHECK((err = write_pattern_file(FILE_A, 100, seed, wbuf, 64)) == NULL, "%s", err);

        // "a": seek must not affect write position
        CHECK((fp = fopen(FILE_A, "a")) != NULL, "unable to open file in \"a\" mode");
        CHECK(fseek(fp, 0, SEEK_SET) == 0, "fseek() failed");
        CHECK((err = write_pattern(fp, 100, 50, seed, wbuf, 64)) == NULL, "\"a\" write: %s", err);
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 150, "wrong size after \"a\": %lld", size);
        CHECK((err = verify_pattern_file(FILE_A, 150, seed, rbuf, 64)) == NULL, "after \"a\": %s", err);

        // "a+": read from beginning, write at end
        CHECK((fp = fopen(FILE_A, "a+")) != NULL, "unable to open file in \"a+\" mode");
        CHECK(fseek(fp, 0, SEEK_SET) == 0, "fseek() failed");
        CHECK((err = check_pattern(fp, 0, 10, seed, rbuf, 10)) == NULL, "\"a+\" read: %s", err);
        CHECK(fseek(fp, 0, SEEK_CUR) == 0, "fseek() failed"); // required between read and write
        CHECK((err = write_pattern(fp, 150, 10, seed, wbuf, 10)) == NULL, "\"a+\" write: %s", err);
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 160, "wrong size after \"a+\": %lld", size);
        CHECK((err = verify_pattern_file(FILE_A, 160, seed, rbuf, 64)) == NULL, "after \"a+\": %s", err);

        // "a" creates file if not exists
        CHECK((fp = fopen(FILE_B, "a")) != NULL, "\"a\" did not create new file");
        CHECK((err = write_pattern(fp, 0, 10, seed, wbuf, 10)) == NULL, "%s", err);
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_B)) == 10, "wrong size of new file: %lld", size);

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        remove(FILE_B);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  "r+" overwrites without truncation, extension with gap; "w+".
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_update_modes(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        struct stat st;
        long long size;
        const u8_t seed = 0x66;
        u8_t *wbuf = global->wbuf;
        u8_t *rbuf = global->rbuf;

        CHECK((err = write_pattern_file(FILE_A, 300, seed, wbuf, 64)) == NULL, "%s", err);

        CHECK((fp = fopen(FILE_A, "r+")) != NULL, "unable to open file in \"r+\" mode");
        CHECK(fseek(fp, 100, SEEK_SET) == 0, "fseek() failed");
        memset(wbuf, 0xEE, 50);
        CHECK(fwrite(wbuf, 1, 50, fp) == 50, "overwrite in \"r+\" mode failed");
        CHECK(fflush(fp) == 0, "fflush() failed");
        CHECK(fstat(fileno(fp), &st) == 0, "fstat() failed");
        CHECK(st.st_size == 300, "file size changed after overwrite: %llu", (unsigned long long)st.st_size);

        CHECK(fseek(fp, 400, SEEK_SET) == 0, "fseek() beyond end of file failed");
        CHECK(fputc(0xAB, fp) == 0xAB, "write beyond end of file failed");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 401, "wrong size after file extension: %lld", size);

        CHECK((fp = fopen(FILE_A, "r")) != NULL, "unable to open file");
        CHECK((err = check_pattern(fp, 0, 100, seed, rbuf, 64)) == NULL, "region 0..99: %s", err);

        CHECK(fread(rbuf, 1, 50, fp) == 50, "read error");
        for (size_t i = 0; i < 50; i++) {
                CHECK(rbuf[i] == 0xEE, "overwritten byte %u is 0x%02X", (unsigned)(100 + i), rbuf[i]);
        }

        CHECK((err = check_pattern(fp, 150, 150, seed, rbuf, 64)) == NULL, "region 150..299: %s", err);

        CHECK(fread(rbuf, 1, 100, fp) == 100, "gap read error");
        for (size_t i = 0; i < 100; i++) {
                CHECK(rbuf[i] == 0, "gap byte %u is not zero (0x%02X)", (unsigned)(300 + i), rbuf[i]);
        }

        CHECK(fgetc(fp) == 0xAB, "last byte is incorrect");
        CHECK((fgetc(fp) == EOF) && feof(fp), "data after expected end of file");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        // "w+": truncate, write and read back by the same handle
        CHECK((fp = fopen(FILE_A, "w+")) != NULL, "unable to open file in \"w+\" mode");
        CHECK((err = write_pattern(fp, 0, 64, (u8_t)(seed + 1), wbuf, 64)) == NULL, "\"w+\" write: %s", err);
        CHECK(fseek(fp, 0, SEEK_SET) == 0, "fseek() failed");
        CHECK((err = check_pattern(fp, 0, 64, (u8_t)(seed + 1), rbuf, 64)) == NULL, "\"w+\" read: %s", err);
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK((size = file_size(FILE_A)) == 64, "\"w+\" did not truncate file: %lld", size);

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Data written by one handle is visible by second handle after fflush().
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_two_handles(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp1 = NULL;
        FILE *fp2 = NULL;
        long long size;
        const u8_t seed = 0x88;

        CHECK((fp1 = fopen(FILE_A, "w")) != NULL, "unable to create file");
        CHECK((err = write_pattern(fp1, 0, 100, seed, global->wbuf, 64)) == NULL, "%s", err);
        CHECK(fflush(fp1) == 0, "fflush() failed");
        CHECK((size = file_size(FILE_A)) == 100, "size not updated after fflush(): %lld", size);

        CHECK((fp2 = fopen(FILE_A, "r")) != NULL, "unable to open second handle");
        CHECK((err = check_pattern(fp2, 0, 100, seed, global->rbuf, 64)) == NULL, "second handle read: %s",
                err);

        CHECK((err = write_pattern(fp1, 100, 50, seed, global->wbuf, 64)) == NULL, "%s", err);
        CHECK(fflush(fp1) == 0, "fflush() failed");

        CHECK(fseek(fp2, 100, SEEK_SET) == 0, "fseek() failed");
        CHECK((err = check_pattern(fp2, 100, 50, seed, global->rbuf, 64)) == NULL,
                "second handle read of appended data: %s", err);

        CHECK(fclose_null(&fp2) == 0, "fclose() of second handle failed");
        CHECK(fclose_null(&fp1) == 0, "fclose() of first handle failed");
        CHECK((err = verify_pattern_file(FILE_A, 150, seed, global->rbuf, 64)) == NULL, "%s", err);

cleanup:
        fclose_null(&fp2);
        fclose_null(&fp1);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several files written in small chunks alternately (interleaved
 *         block allocation).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_multiple_open_files(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp[ARRAY_SIZE(MULTI_FILE)] = {NULL};
        long long size;
        const size_t total = 2000;
        const size_t chunk = 37;

        for (size_t f = 0; f < ARRAY_SIZE(MULTI_FILE); f++) {
                CHECK((fp[f] = fopen(MULTI_FILE[f], "w")) != NULL, "unable to create %s", MULTI_FILE[f]);
        }

        for (size_t pos = 0; pos < total; pos += chunk) {
                size_t n = min(chunk, total - pos);

                for (size_t f = 0; f < ARRAY_SIZE(MULTI_FILE); f++) {
                        CHECK((err = write_pattern(fp[f], pos, n, (u8_t)(0x80 + f), global->wbuf, n)) == NULL,
                                "%s: %s", MULTI_FILE[f], err);
                        CHECK(fflush(fp[f]) == 0, "%s: fflush() failed", MULTI_FILE[f]);
                }
        }

        for (size_t f = 0; f < ARRAY_SIZE(MULTI_FILE); f++) {
                CHECK(fclose_null(&fp[f]) == 0, "%s: fclose() failed", MULTI_FILE[f]);
        }

        for (size_t f = 0; f < ARRAY_SIZE(MULTI_FILE); f++) {
                CHECK((size = file_size(MULTI_FILE[f])) == (long long)total, "%s: wrong size %lld",
                        MULTI_FILE[f], size);
                CHECK((err = verify_pattern_file(MULTI_FILE[f], total, (u8_t)(0x80 + f), global->rbuf,
                               FSTEST_IO_BUF_SIZE)) == NULL,
                        "%s: %s", MULTI_FILE[f], err);
        }

cleanup:
        for (size_t f = 0; f < ARRAY_SIZE(MULTI_FILE); f++) {
                fclose_null(&fp[f]);
                remove(MULTI_FILE[f]);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  fstat() vs stat() consistency and file mode change.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_mode(void)
{
        static const struct {
                mode_t set;
                mode_t expected;
        } MODE[] = {
                {0666 | S_IFDIR, 0666}, // file type bits must be ignored
                {0421, 0421},
                {0, 0},
                {0644, 0644},
        };

        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat stat_fp = {0};
        struct stat stat_path = {0};

        CHECK((fp = fopen(FILE_A, "w")) != NULL, "unable to create test file");
        CHECK(fputs("mode", fp) >= 0, "write error");
        CHECK(fflush(fp) == 0, "fflush() failed");
        CHECK(fstat(fileno(fp), &stat_fp) == 0, "unable to perform fstat()");
        CHECK(stat(FILE_A, &stat_path) == 0, "unable to perform stat()");

        if (global->verbose) {
                print_stat("fstat()", &stat_fp);
                print_stat("stat()", &stat_path);
                puts("");
                print_byte_array("fstat()", &stat_fp, sizeof(stat_fp));
                print_byte_array("stat()", &stat_path, sizeof(stat_path));
        }

        CHECK(stat_fp.st_size == 4, "fstat(): wrong size %llu", (unsigned long long)stat_fp.st_size);
        CHECK(stat_fp.st_size == stat_path.st_size, "fstat()/stat(): st_size differs");
        CHECK(stat_fp.st_mode == stat_path.st_mode, "fstat()/stat(): st_mode differs (%lo vs %lo)",
                (unsigned long)stat_fp.st_mode, (unsigned long)stat_path.st_mode);
        CHECK(stat_fp.st_dev == stat_path.st_dev, "fstat()/stat(): st_dev differs");
        CHECK(stat_fp.st_uid == stat_path.st_uid, "fstat()/stat(): st_uid differs");
        CHECK(stat_fp.st_gid == stat_path.st_gid, "fstat()/stat(): st_gid differs");
        CHECK(stat_fp.st_mtime == stat_path.st_mtime, "fstat()/stat(): st_mtime differs");
        CHECK(stat_fp.st_ctime == stat_path.st_ctime, "fstat()/stat(): st_ctime differs");
        CHECK(S_ISREG(stat_path.st_mode), "file is not reported as regular file (mode: %lo)",
                (unsigned long)stat_path.st_mode);

        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        for (size_t i = 0; i < ARRAY_SIZE(MODE); i++) {
                CHECK(chmod(FILE_A, MODE[i].set) == 0, "unable to chmod(%lo)", (unsigned long)MODE[i].set);
                CHECK(stat(FILE_A, &stat_path) == 0, "unable to perform stat()");

                if (global->verbose) {
                        printf("\n  chmod(%lo) -> st_mode %lo", (unsigned long)MODE[i].set,
                                (unsigned long)stat_path.st_mode);
                }

                CHECK(S_ISREG(stat_path.st_mode), "chmod(%lo) changed file type (mode: %lo)",
                        (unsigned long)MODE[i].set, (unsigned long)stat_path.st_mode);
                mode_t expected = expected_mode(MODE[i].expected, false);
                CHECK((stat_path.st_mode & S_IPMT) == expected, "chmod(%lo): mode %lo, expected %lo",
                        (unsigned long)MODE[i].set, (unsigned long)(stat_path.st_mode & S_IPMT),
                        (unsigned long)expected);
        }

        CHECK(file_size(FILE_A) == 4, "chmod() changed file size");

cleanup:
        fclose_null(&fp);
        chmod(FILE_A, 0666);
        remove(FILE_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Directory create/remove and invalid operations on directories.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_create_and_remove(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat st;

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");
        CHECK(stat(DIR_A, &st) == 0, "unable to stat directory");
        CHECK(S_ISDIR(st.st_mode), "created object is not a directory (mode: %lo)",
                (unsigned long)st.st_mode);

        EXPECT_FAIL(mkdir(DIR_A, 0777) != 0, EEXIST, "mkdir() of existing directory");
        EXPECT_FAIL((fp = fopen(DIR_A, "w")) == NULL, 0, "fopen(\"w\") of directory"); // EISDIR

        CHECK((fp = fopen(FILE_A, "w")) != NULL, "unable to create file");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        EXPECT_FAIL(mkdir(FILE_A, 0777) != 0, EEXIST, "mkdir() with name of existing file");
        EXPECT_FAIL((fp = fopen(FILE_A "/x", "w")) == NULL, 0,
                "fopen() with regular file as directory"); // ENOTDIR

        CHECK(stat(DIR_A, &st) == 0 && S_ISDIR(st.st_mode), "directory damaged");
        CHECK(stat(FILE_A, &st) == 0 && S_ISREG(st.st_mode), "file damaged");

        CHECK(remove(DIR_A) == 0, "unable to remove directory");
        EXPECT_FAIL(stat(DIR_A, &st) != 0, ENOENT, "stat() of removed directory");

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        remove(DIR_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Directory mode change (type bits must be preserved).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_mode(void)
{
        static const mode_t MODE[] = {0666, 0421, 0777, 0};

        const char *exit_msg = SUCCESS_MSG;
        struct stat st;

        CHECK(mkdir(DIR_A, 0666) == 0, "unable to create test directory");
        CHECK(stat(DIR_A, &st) == 0, "unable to perform stat()");
        CHECK(S_ISDIR(st.st_mode), "created object is not a directory (mode: %lo)",
                (unsigned long)st.st_mode);

        if (global->verbose) {
                printf("\n  mkdir(0666) -> st_mode %lo", (unsigned long)st.st_mode);
        }

        for (size_t i = 0; i < ARRAY_SIZE(MODE); i++) {
                CHECK(chmod(DIR_A, MODE[i]) == 0, "unable to chmod(%lo)", (unsigned long)MODE[i]);
                CHECK(stat(DIR_A, &st) == 0, "unable to perform stat()");

                if (global->verbose) {
                        printf("\n  chmod(%lo) -> st_mode %lo", (unsigned long)MODE[i],
                                (unsigned long)st.st_mode);
                }

                CHECK(S_ISDIR(st.st_mode), "chmod(%lo) changed directory type (mode: %lo)",
                        (unsigned long)MODE[i], (unsigned long)st.st_mode);
                mode_t expected = expected_mode(MODE[i], true);
                CHECK((st.st_mode & S_IPMT) == expected, "chmod(%lo): mode %lo, expected %lo",
                        (unsigned long)MODE[i], (unsigned long)(st.st_mode & S_IPMT), (unsigned long)expected);
        }

cleanup:
        chmod(DIR_A, 0777);
        remove(DIR_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Non-empty directory cannot be removed.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_not_empty(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        struct stat st;

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");
        CHECK((err = write_pattern_file(DIR_A_FILE, 10, 1, global->wbuf, 10)) == NULL, "%s", err);

        EXPECT_FAIL(remove(DIR_A) != 0, 0, "remove() of directory with file"); // ENOTEMPTY
        CHECK(stat(DIR_A, &st) == 0 && S_ISDIR(st.st_mode), "directory damaged after failed remove()");
        CHECK((err = verify_pattern_file(DIR_A_FILE, 10, 1, global->rbuf, 10)) == NULL,
                "file damaged after failed remove(): %s", err);

        CHECK(remove(DIR_A_FILE) == 0, "unable to remove file");
        CHECK(mkdir(DIR_A_SUB, 0777) == 0, "unable to create subdirectory");

        EXPECT_FAIL(remove(DIR_A) != 0, 0, "remove() of directory with subdirectory");
        CHECK(stat(DIR_A_SUB, &st) == 0 && S_ISDIR(st.st_mode), "subdirectory damaged");

        CHECK(remove(DIR_A_SUB) == 0, "unable to remove subdirectory");
        CHECK(remove(DIR_A) == 0, "unable to remove empty directory");

cleanup:
        remove(DIR_A_FILE);
        remove(DIR_A_SUB);
        remove(DIR_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several levels of nested directories with file at the bottom.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_nested(void)
{
        static const char *const DIRS[] = {DIR_A, NEST_1, NEST_2, NEST_3};

        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        struct stat st;
        long long size;

        for (size_t i = 0; i < ARRAY_SIZE(DIRS); i++) {
                CHECK(mkdir(DIRS[i], 0777) == 0, "unable to create %s", DIRS[i]);
        }

        CHECK((err = write_pattern_file(NEST_FILE, 1000, 0x5A, global->wbuf, 100)) == NULL, "%s", err);
        CHECK((size = file_size(NEST_FILE)) == 1000, "wrong file size: %lld", size);
        CHECK((err = verify_pattern_file(NEST_FILE, 1000, 0x5A, global->rbuf, 128)) == NULL, "%s", err);

        for (size_t i = 0; i < ARRAY_SIZE(DIRS); i++) {
                CHECK(stat(DIRS[i], &st) == 0 && S_ISDIR(st.st_mode), "%s is not a directory", DIRS[i]);
        }

        CHECK(remove(NEST_FILE) == 0, "unable to remove file");

        for (size_t i = ARRAY_SIZE(DIRS); i > 0; i--) {
                CHECK(remove(DIRS[i - 1]) == 0, "unable to remove %s", DIRS[i - 1]);
                EXPECT_FAIL(stat(DIRS[i - 1], &st) != 0, ENOENT, "stat() of removed directory");
        }

cleanup:
        remove(NEST_FILE);
        for (size_t i = ARRAY_SIZE(DIRS); i > 0; i--) {
                remove(DIRS[i - 1]);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  readdir() lists each file exactly once, also after removing
 *         entries from the middle of directory.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_listing(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        char *path = global->path[0];
        const u64_t all = ((u64_t)1 << FSTEST_DIR_FILES) - 1;
        u64_t even = 0;
        long long size;

        for (unsigned i = 0; i < FSTEST_DIR_FILES; i += 2) {
                even |= (u64_t)1 << i;
        }

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");
        CHECK((err = scan_listing(DIR_A, 0)) == NULL, "empty directory: %s", err);

        for (unsigned i = 0; i < FSTEST_DIR_FILES; i++) {
                listing_path(path, DIR_A, i);
                CHECK((err = write_pattern_file(path, i, (u8_t)i, global->wbuf, 64)) == NULL, "%s: %s", path,
                        err);
        }

        CHECK((err = scan_listing(DIR_A, all)) == NULL, "after create: %s", err);

        for (unsigned i = 0; i < FSTEST_DIR_FILES; i++) {
                listing_path(path, DIR_A, i);
                CHECK((size = file_size(path)) == (long long)i, "%s: wrong size %lld", path, size);
                CHECK((err = verify_pattern_file(path, i, (u8_t)i, global->rbuf, 64)) == NULL, "%s: %s", path,
                        err);
        }

        for (unsigned i = 0; i < FSTEST_DIR_FILES; i += 2) {
                CHECK(remove(listing_path(path, DIR_A, i)) == 0, "unable to remove %s", path);
        }

        CHECK((err = scan_listing(DIR_A, all & ~even)) == NULL, "after removing half: %s", err);

        for (unsigned i = 1; i < FSTEST_DIR_FILES; i += 2) {
                CHECK(remove(listing_path(path, DIR_A, i)) == 0, "unable to remove %s", path);
        }

        CHECK((err = scan_listing(DIR_A, 0)) == NULL, "after removing all: %s", err);
        CHECK(remove(DIR_A) == 0, "unable to remove directory");

cleanup:
        for (unsigned i = 0; i < FSTEST_DIR_FILES; i++) {
                remove(listing_path(path, DIR_A, i));
        }
        remove(DIR_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Rename file in the same directory and between directories.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_rename(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        struct stat st;
        const u8_t seed = 0x99;

        CHECK((err = write_pattern_file(FILE_A, 500, seed, global->wbuf, 64)) == NULL, "%s", err);

        CHECK(rename(FILE_A, FILE_B) == 0, "rename() in the same directory failed");
        EXPECT_FAIL(stat(FILE_A, &st) != 0, ENOENT, "stat() of old name");
        CHECK((err = verify_pattern_file(FILE_B, 500, seed, global->rbuf, 64)) == NULL, "renamed file: %s",
                err);

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");
        CHECK(rename(FILE_B, MOVED_FILE) == 0, "rename() to subdirectory failed");
        EXPECT_FAIL(stat(FILE_B, &st) != 0, ENOENT, "stat() of moved file old name");
        CHECK(dir_contains(DIR_A, "moved.file"), "moved file not listed in directory");
        CHECK((err = verify_pattern_file(MOVED_FILE, 500, seed, global->rbuf, 64)) == NULL, "moved file: %s",
                err);

        CHECK(rename(MOVED_FILE, FILE_A) == 0, "rename() to parent directory failed");
        CHECK(!dir_contains(DIR_A, "moved.file"), "moved file still listed in old directory");
        CHECK((err = verify_pattern_file(FILE_A, 500, seed, global->rbuf, 64)) == NULL, "file moved back: %s",
                err);

        CHECK(remove(DIR_A) == 0, "directory not empty after moving file out");

cleanup:
        remove(FILE_A);
        remove(FILE_B);
        remove(MOVED_FILE);
        remove(DIR_A);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Rename directory with content.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_dir_rename(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        struct stat st;
        const u8_t seed = 0xA5;

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");
        CHECK(mkdir(DIR_A_SUB, 0777) == 0, "unable to create subdirectory");
        CHECK((err = write_pattern_file(DIR_A_SUB_FILE, 300, seed, global->wbuf, 64)) == NULL, "%s", err);

        CHECK(rename(DIR_A, DIR_B) == 0, "rename() of directory failed");
        EXPECT_FAIL(stat(DIR_A, &st) != 0, ENOENT, "stat() of old directory name");
        CHECK(stat(DIR_B, &st) == 0 && S_ISDIR(st.st_mode), "renamed directory is not a directory");
        CHECK(stat(DIR_B_SUB, &st) == 0 && S_ISDIR(st.st_mode), "subdirectory lost after rename");
        CHECK((err = verify_pattern_file(DIR_B_SUB_FILE, 300, seed, global->rbuf, 64)) == NULL,
                "file in renamed directory: %s", err);

        EXPECT_FAIL(rename(DIR_B, DIR_B_SUB "/inside") != 0, 0,
                "rename() of directory into its own subdirectory"); // EINVAL
        CHECK(stat(DIR_B_SUB, &st) == 0 && S_ISDIR(st.st_mode), "directory tree damaged");

        CHECK(remove(DIR_B_SUB_FILE) == 0, "unable to remove file");
        CHECK(remove(DIR_B_SUB) == 0, "unable to remove subdirectory");
        CHECK(remove(DIR_B) == 0, "unable to remove directory");

cleanup:
        remove(DIR_A_SUB_FILE);
        remove(DIR_A_SUB);
        remove(DIR_A);
        remove(DIR_B_SUB_FILE);
        remove(DIR_B_SUB);
        remove(DIR_B);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Short/long names and names with special characters (case must be
 *         preserved, name must not be mangled).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_file_names(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        const char *name;
        char *path = global->path[0];
        long long size;
        FILE *fp = NULL;

        path[0] = '\0';

        CHECK(mkdir(DIR_A, 0777) == 0, "unable to create directory");

        for (size_t i = 0; i < ARRAY_SIZE(NAME_LEN); i++) {
                const size_t len = NAME_LEN[i];

                if (len > global->name_max) {
                        continue;
                }

                name = long_name_path(path, DIR_A, len);

                CHECK((err = write_pattern_file(path, len, (u8_t)len, global->wbuf, 64)) == NULL,
                        "name length %u: %s", (unsigned)len, err);
                CHECK((size = file_size(path)) == (long long)len, "name length %u: wrong size %lld",
                        (unsigned)len, size);
                CHECK((err = verify_pattern_file(path, len, (u8_t)len, global->rbuf, 64)) == NULL,
                        "name length %u: %s", (unsigned)len, err);
                CHECK(dir_contains(DIR_A, name), "name length %u: name not found (or mangled) in listing",
                        (unsigned)len);
                CHECK(remove(path) == 0, "name length %u: unable to remove", (unsigned)len);
                CHECK(!dir_contains(DIR_A, name), "name length %u: removed name still listed", (unsigned)len);
        }

        /*
         * File system with limited name length: the longest name works, name
         * longer by one character is rejected (not truncated).
         */
        if (global->name_max < FSTEST_LONG_NAME_MAX) {
                const size_t len = global->name_max;

                name = long_name_path(path, DIR_A, len);

                CHECK((err = write_pattern_file(path, len, (u8_t)len, global->wbuf, 64)) == NULL,
                        "name length %u (limit): %s", (unsigned)len, err);
                CHECK((err = verify_pattern_file(path, len, (u8_t)len, global->rbuf, 64)) == NULL,
                        "name length %u (limit): %s", (unsigned)len, err);
                CHECK(dir_contains(DIR_A, name), "name length %u (limit): name not found (or mangled) in listing",
                        (unsigned)len);
                CHECK(remove(path) == 0, "name length %u (limit): unable to remove", (unsigned)len);

                name = long_name_path(path, DIR_A, len + 1);

                EXPECT_FAIL((fp = fopen(path, "w")) == NULL, ENAMETOOLONG, "fopen(\"w\") of name longer than limit");
                EXPECT_FAIL(mkdir(path, 0777) != 0, ENAMETOOLONG, "mkdir() of name longer than limit");

                CHECK(count_entries(DIR_A) == 0, "truncated name created in directory");
                path[0] = '\0';
        }

        for (size_t i = 0; i < ARRAY_SIZE(SPECIAL_NAME); i++) {
                snprintf(path, FSTEST_PATH_BUF_LEN, DIR_A "/%s", SPECIAL_NAME[i]);
                name = SPECIAL_NAME[i];

                CHECK((err = write_pattern_file(path, 20, (u8_t)i, global->wbuf, 64)) == NULL, "'%s': %s",
                        name, err);
                CHECK((err = verify_pattern_file(path, 20, (u8_t)i, global->rbuf, 64)) == NULL, "'%s': %s",
                        name, err);
                CHECK(dir_contains(DIR_A, name), "'%s': name not found (or mangled) in listing", name);
                CHECK(remove(path) == 0, "'%s': unable to remove", name);
        }

        path[0] = '\0';
        CHECK(remove(DIR_A) == 0, "unable to remove directory (not empty?)");

cleanup:
        if (fp) {
                fclose(fp);
        }
        if (path[0]) {
                remove(path);
        }
        remove(DIR_A);
        return exit_msg;
}

#if FSTEST_USE_TIMESTAMPS
//==============================================================================
/**
 * @brief  Modification time is set on create and updated on write.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_timestamps(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        struct stat st1, st2;
        long long diff;

        time_t t0 = time(NULL);
        if (t0 < (time_t)FSTEST_TIME_VALID_MIN) {
                return SKIP("system time is not set");
        }

        CHECK((err = write_pattern_file(FILE_A, 10, 1, global->wbuf, 10)) == NULL, "%s", err);
        CHECK(stat(FILE_A, &st1) == 0, "stat() failed");

        diff = (long long)st1.st_mtime - (long long)t0;

        if (global->verbose) {
                printf("\n  time: %lld, mtime: %lld, ctime: %lld", (long long)t0, (long long)st1.st_mtime,
                        (long long)st1.st_ctime);
        }

        CHECK((diff >= -FSTEST_TIME_TOLERANCE) && (diff <= FSTEST_TIME_TOLERANCE),
                "mtime differs from system time by %lld s", diff);

        sleep(FSTEST_TIME_STEP);

        CHECK((fp = fopen(FILE_A, "a")) != NULL, "unable to open file");
        CHECK(fputc('x', fp) == 'x', "write error");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");
        CHECK(stat(FILE_A, &st2) == 0, "stat() failed");

        CHECK(st2.st_mtime > st1.st_mtime, "mtime not updated after write (%lld -> %lld)",
                (long long)st1.st_mtime, (long long)st2.st_mtime);

cleanup:
        fclose_null(&fp);
        remove(FILE_A);
        return exit_msg;
}
#endif

#if FSTEST_USE_STATFS
//==============================================================================
/**
 * @brief  Free space decreases after write and is recovered after remove.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_free_space(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        char *cwd = global->path[1];
        struct statfs sfs;
        u64_t bsize, free0, free1, free2;

        CHECK(getcwd(cwd, FSTEST_PATH_BUF_LEN) != NULL, "getcwd() failed");
        CHECK(statfs(cwd, &sfs) == 0, "statfs(\"%s\") failed", cwd);
        CHECK((sfs.f_bsize > 0) && (sfs.f_blocks > 0) && (sfs.f_bfree <= sfs.f_blocks),
                "statfs() returned inconsistent values");

        /*
         * Free space of network/shared file system (e.g. hostfs) is changed
         * also by other clients (host processes), so exact accounting cannot
         * be checked.
         */
        if (sfs.f_type == FS_TYPE_NET) {
                return SKIP("free space of shared file system is changed by other clients");
        }

        bsize = (u64_t)sfs.f_bsize;
        free0 = (u64_t)sfs.f_bfree * bsize;

        CHECK((err = write_pattern_file(FILE_A, FSTEST_BIG_FILE_SIZE, 0x13, global->wbuf,
                       FSTEST_IO_BUF_SIZE)) == NULL,
                "%s", err);
        sync();

        CHECK(statfs(cwd, &sfs) == 0, "statfs() failed");
        free1 = (u64_t)sfs.f_bfree * bsize;

        CHECK(remove(FILE_A) == 0, "unable to remove file");
        sync();

        CHECK(statfs(cwd, &sfs) == 0, "statfs() failed");
        free2 = (u64_t)sfs.f_bfree * bsize;

        if (global->verbose) {
                printf("\n  bsize: %llu, free: %llu -> %llu -> %llu B", (unsigned long long)bsize,
                        (unsigned long long)free0, (unsigned long long)free1, (unsigned long long)free2);
        }

        CHECK(free1 < free0, "free space did not decrease after write");
        CHECK((free0 - free1 + bsize) >= FSTEST_BIG_FILE_SIZE,
                "free space decreased by %llu B only (file: %u B)", (unsigned long long)(free0 - free1),
                (unsigned)FSTEST_BIG_FILE_SIZE);
        CHECK((free2 + bsize) >= free0, "free space not recovered after remove (%llu B missing)",
                (unsigned long long)(free0 - free2));

cleanup:
        remove(FILE_A);
        return exit_msg;
}
#endif

#if FSTEST_USE_THREADS
//==============================================================================
/**
 * @brief  Thread: (write and) verify file several times.
 */
//==============================================================================
static int fs_job_thread(void *arg)
{
        fs_job_t *job = arg;
        u8_t buf[FSTEST_THREAD_BUF_SIZE];

        job->result = NULL;

        for (unsigned r = 0; (r < FSTEST_THREAD_ROUNDS) && !job->result; r++) {
                u8_t seed = job->write ? (u8_t)(job->seed + r) : job->seed;

                if (job->write) {
                        job->result = write_pattern_file(job->path, job->size, seed, buf, job->chunk);
                }

                if (!job->result) {
                        job->result = verify_pattern_file(job->path, job->size, seed, buf, job->chunk);
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Start jobs in separate threads and wait for completion.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *run_jobs(fs_job_t *job, size_t count)
{
        static const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_MEDIUM,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        tid_t tid[FSTEST_THREADS];
        size_t created = 0;

        for (size_t i = 0; i < count; i++) {
                job[i].result = "thread not started";
        }

        for (; created < count; created++) {
                tid[created] = thread_create(fs_job_thread, &attr, &job[created]);
                if (tid[created] == 0) {
                        break;
                }
        }

        // threads use caller's job[] so they must be joined before return
        for (size_t i = 0; i < created; i++) {
                thread_join(tid[i], NULL);
        }

        if (created != count) {
                return make_msg(0, "unable to create thread %u", (unsigned)created);
        }

        for (size_t i = 0; i < count; i++) {
                if (job[i].result) {
                        return make_msg(0, "thread %u (%s): %s", (unsigned)i, job[i].path, job[i].result);
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Several threads write and read different files at the same time.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_threads(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        fs_job_t job[FSTEST_THREADS];

        for (size_t i = 0; i < FSTEST_THREADS; i++) {
                job[i].path = THREAD_FILE[i];
                job[i].size = FSTEST_THREAD_FILE_SIZE;
                job[i].seed = (u8_t)(0x30 + (i * 16));
                job[i].chunk = FSTEST_THREAD_BUF_SIZE - (i * 5);
                job[i].write = true;
        }

        if ((err = run_jobs(job, FSTEST_THREADS)) != NULL) {
                exit_msg = err; // already in msg_buf
                goto cleanup;
        }

cleanup:
        for (size_t i = 0; i < FSTEST_THREADS; i++) {
                remove(THREAD_FILE[i]);
        }
        return exit_msg;
}
#endif

/*==============================================================================
  Read tests (reference data, file system is not modified)
==============================================================================*/

//==============================================================================
/**
 * @brief  All reference objects exist and have correct type and size.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_structure(void)
{
        const char *exit_msg = SUCCESS_MSG;
        char *path = global->path[0];
        struct stat st;
        ref_file_t rf;
        int r;

        for (size_t i = 0; i < ARRAY_SIZE(REF_DIRS); i++) {
                if (!global->empty_dirs && (strcmp(REF_DIRS[i], REF_EMPTY_DIR) == 0)) {
                        continue;
                }

                CHECK(stat(REF_DIRS[i], &st) == 0, "directory '%s' not found", REF_DIRS[i]);
                CHECK(S_ISDIR(st.st_mode), "'%s' is not a directory (mode: %lo)", REF_DIRS[i],
                        (unsigned long)st.st_mode);
        }

        for (size_t i = 0; (r = ref_file_get(i, path, &rf)) >= 0; i++) {
                if (r == 0) {
                        continue;
                }

                CHECK(stat(rf.path, &st) == 0, "file '%s' not found", rf.path);
                CHECK(S_ISREG(st.st_mode), "'%s' is not a regular file (mode: %lo)", rf.path,
                        (unsigned long)st.st_mode);
                CHECK((long long)st.st_size == (long long)rf.size, "'%s': size %llu, expected %u", rf.path,
                        (unsigned long long)st.st_size, (unsigned)rf.size);
        }

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Access to non-existing objects and invalid paths.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_nonexistent(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        DIR *dir = NULL;
        struct stat st;

        EXPECT_FAIL((fp = fopen(REF_NO_FILE, "r")) == NULL, ENOENT, "fopen() of non-existing file");
        EXPECT_FAIL(stat(REF_NO_FILE, &st) != 0, ENOENT, "stat() of non-existing file");
        EXPECT_FAIL((dir = opendir(REF_NO_DIR)) == NULL, ENOENT, "opendir() of non-existing directory");
        EXPECT_FAIL((fp = fopen(REF_NO_DIR "/x.file", "r")) == NULL, ENOENT,
                "fopen() in non-existing directory");
        EXPECT_FAIL(stat(REF_NO_DIR "/x.file", &st) != 0, ENOENT, "stat() in non-existing directory");

        if (global->case_sensitive) {
                EXPECT_FAIL((fp = fopen(REF_DIR "/SMALL.FILE", "r")) == NULL, ENOENT,
                        "fopen() with different case");
        } else {
                // case insensitive: name with different case is the same file
                CHECK((fp = fopen(REF_DIR "/SMALL.FILE", "r")) != NULL, "fopen() with different case failed");
                CHECK(fclose_null(&fp) == 0, "fclose() failed");
                CHECK(stat(REF_DIR "/SMALL.FILE", &st) == 0, "stat() with different case failed");
                CHECK(st.st_size == REF_SMALL_SIZE, "name with different case points to other file (size %llu)",
                        (unsigned long long)st.st_size);
        }

        // file used as directory (ENOTDIR)
        EXPECT_FAIL((dir = opendir(REF_SMALL_FILE)) == NULL, 0, "opendir() of regular file");
        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE "/x.file", "r")) == NULL, 0,
                "fopen() with regular file as directory");
        EXPECT_FAIL(stat(REF_SMALL_FILE "/x.file", &st) != 0, 0, "stat() with regular file as directory");

        // name prefix and extension of existing name
        EXPECT_FAIL((fp = fopen(REF_DIR "/small.fil", "r")) == NULL, ENOENT, "fopen() of name prefix");
        EXPECT_FAIL((fp = fopen(REF_DIR "/small.file2", "r")) == NULL, ENOENT, "fopen() of extended name");

cleanup:
        fclose_null(&fp);
        if (dir) {
                closedir(dir);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Content of all reference files read with different chunk sizes.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_read_data(void)
{
        static const size_t CHUNK[] = {FSTEST_IO_BUF_SIZE, 13, 1};

        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        char *path = global->path[0];
        ref_file_t rf;
        int r;

        for (size_t i = 0; (r = ref_file_get(i, path, &rf)) >= 0; i++) {
                if (r == 0) {
                        continue;
                }

                for (size_t c = 0; c < ARRAY_SIZE(CHUNK); c++) {
                        // byte by byte only for small files (speed)
                        if ((CHUNK[c] == 1) && (rf.size > 5000)) {
                                continue;
                        }

                        CHECK((err = verify_pattern_file(rf.path, rf.size, rf.seed, global->rbuf,
                                       CHUNK[c])) == NULL,
                                "'%s' (%u B chunks): %s", rf.path, (unsigned)CHUNK[c], err);
                }
        }

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Random access to big file, seek in all modes, EOF handling.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_seek(void)
{
        static const long POS[] = {0, 1, 511, 512, 513, 1023, 1024, 4095, 4096, 4097, 8191, 8192,
                REF_BIG_SIZE / 2, REF_BIG_SIZE - 2, REF_BIG_SIZE - 1, 3, 4096, 0};

        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        const long size = REF_BIG_SIZE;
        const u8_t seed = REF_BIG_SEED;
        int c;

        CHECK((fp = fopen(REF_BIG_FILE, "r")) != NULL, "unable to open '%s'", REF_BIG_FILE);
        CHECK(ftell(fp) == 0, "initial position is not 0");

        // random access (also backward and around block boundaries)
        for (size_t i = 0; i < ARRAY_SIZE(POS); i++) {
                const long pos = POS[i];

                if (pos >= size) {
                        continue;
                }

                CHECK(fseek(fp, pos, SEEK_SET) == 0, "fseek(%ld, SEEK_SET) failed", pos);
                CHECK(ftell(fp) == pos, "ftell() after fseek(%ld): %ld", pos, ftell(fp));
                CHECK((c = fgetc(fp)) == pattern(pos, seed), "wrong byte at position %ld: 0x%02X", pos, c);
                CHECK(ftell(fp) == pos + 1, "position not incremented after read at %ld", pos);
        }

        // backward reading of blocks from end of file
        for (long pos = size - 37; pos >= 0; pos -= 1531) {
                CHECK(fseek(fp, pos, SEEK_SET) == 0, "fseek(%ld) failed", pos);
                CHECK((err = check_pattern(fp, pos, 37, seed, global->rbuf, 37)) == NULL,
                        "backward read at %ld: %s", pos, err);
        }

        // relative seeks
        const long p1 = size / 4;
        const long p2 = p1 - (size / 8);
        const long p3 = p2 + 1 + (size / 2);

        CHECK((fseek(fp, p1, SEEK_SET) == 0) && (ftell(fp) == p1), "SEEK_SET failed");
        CHECK((fseek(fp, p2 - p1, SEEK_CUR) == 0) && (ftell(fp) == p2), "SEEK_CUR backward failed");
        CHECK(fgetc(fp) == pattern(p2, seed), "wrong byte at position %ld", p2);
        CHECK((fseek(fp, p3 - p2 - 1, SEEK_CUR) == 0) && (ftell(fp) == p3), "SEEK_CUR forward failed");
        CHECK(fgetc(fp) == pattern(p3, seed), "wrong byte at position %ld", p3);
        CHECK((fseek(fp, -1, SEEK_END) == 0) && (ftell(fp) == size - 1), "SEEK_END failed");
        CHECK(fgetc(fp) == pattern(size - 1, seed), "wrong last byte");
        CHECK((fgetc(fp) == EOF) && feof(fp) && !ferror(fp), "EOF not reported at end of file");

        CHECK((fseek(fp, 0, SEEK_END) == 0) && (ftell(fp) == size), "SEEK_END to file end failed");
        CHECK(!feof(fp), "fseek() did not clear EOF flag");

        EXPECT_FAIL(fseek(fp, -1, SEEK_SET) != 0, EINVAL, "fseek() to negative position");
        CHECK(ftell(fp) == size, "position changed after failed fseek()");

        // partial read at end of file
        CHECK(fseek(fp, size - 10, SEEK_SET) == 0, "fseek() failed");
        CHECK(fread(global->rbuf, 1, 32, fp) == 10, "partial read at end returned wrong size");
        CHECK(feof(fp) && !ferror(fp), "partial read at end did not set EOF");
        for (size_t i = 0; i < 10; i++) {
                CHECK(global->rbuf[i] == pattern(size - 10 + i, seed), "partial read: wrong byte %u",
                        (unsigned)i);
        }

        // read beyond end of file
        CHECK(fseek(fp, size + 1000, SEEK_SET) == 0, "seek beyond end of file failed");
        CHECK((fread(global->rbuf, 1, 1, fp) == 0) && feof(fp), "read beyond end of file returned data");

        // rewind
        rewind(fp);
        CHECK((ftell(fp) == 0) && !feof(fp), "rewind() failed");
        CHECK((err = check_pattern(fp, 0, 100, seed, global->rbuf, 100)) == NULL, "after rewind(): %s", err);

        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        // empty file
        CHECK((fp = fopen(REF_EMPTY_FILE, "r")) != NULL, "unable to open '%s'", REF_EMPTY_FILE);
        CHECK((fgetc(fp) == EOF) && feof(fp) && !ferror(fp), "empty file: EOF not reported");
        CHECK((fseek(fp, 0, SEEK_END) == 0) && (ftell(fp) == 0), "empty file: SEEK_END failed");

cleanup:
        fclose_null(&fp);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  stat() and fstat() of reference objects.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_stat(void)
{
        const char *exit_msg = SUCCESS_MSG;
        FILE *fp = NULL;
        struct stat stat_fp = {0};
        struct stat stat_path = {0};
        struct stat stat_dir = {0};

        CHECK((fp = fopen(REF_SMALL_FILE, "r")) != NULL, "unable to open '%s'", REF_SMALL_FILE);
        CHECK(fstat(fileno(fp), &stat_fp) == 0, "unable to perform fstat()");
        CHECK(stat(REF_SMALL_FILE, &stat_path) == 0, "unable to perform stat()");
        CHECK(stat(REF_DIR, &stat_dir) == 0, "unable to perform stat() of directory");

        if (global->verbose) {
                print_stat("fstat()", &stat_fp);
                print_stat("stat()", &stat_path);
                print_stat("dir", &stat_dir);
        }

        CHECK(stat_fp.st_size == REF_SMALL_SIZE, "fstat(): wrong size %llu",
                (unsigned long long)stat_fp.st_size);
        CHECK(stat_fp.st_size == stat_path.st_size, "fstat()/stat(): st_size differs");
        CHECK(stat_fp.st_mode == stat_path.st_mode, "fstat()/stat(): st_mode differs (%lo vs %lo)",
                (unsigned long)stat_fp.st_mode, (unsigned long)stat_path.st_mode);
        CHECK(stat_fp.st_dev == stat_path.st_dev, "fstat()/stat(): st_dev differs");
        CHECK(stat_fp.st_uid == stat_path.st_uid, "fstat()/stat(): st_uid differs");
        CHECK(stat_fp.st_gid == stat_path.st_gid, "fstat()/stat(): st_gid differs");
        CHECK(stat_fp.st_mtime == stat_path.st_mtime, "fstat()/stat(): st_mtime differs");
        CHECK(stat_fp.st_ctime == stat_path.st_ctime, "fstat()/stat(): st_ctime differs");
        CHECK(S_ISREG(stat_fp.st_mode), "fstat(): not a regular file (mode: %lo)",
                (unsigned long)stat_fp.st_mode);
        CHECK(S_ISDIR(stat_dir.st_mode), "stat(): '%s' not a directory (mode: %lo)", REF_DIR,
                (unsigned long)stat_dir.st_mode);
        CHECK(stat_fp.st_dev == stat_dir.st_dev, "st_dev of file and its directory differs");

        // read does not change file statistics
        CHECK(fread(global->rbuf, 1, REF_SMALL_SIZE, fp) == REF_SMALL_SIZE, "read error");
        CHECK(fstat(fileno(fp), &stat_fp) == 0, "unable to perform fstat()");
        CHECK(stat_fp.st_size == stat_path.st_size, "file size changed after read");
        CHECK(stat_fp.st_mtime == stat_path.st_mtime, "st_mtime changed after read");
        CHECK(fclose_null(&fp) == 0, "fclose() failed");

        // second stat() gives the same result
        CHECK(stat(REF_SMALL_FILE, &stat_fp) == 0, "unable to perform stat()");
        CHECK((stat_fp.st_size == stat_path.st_size) && (stat_fp.st_mode == stat_path.st_mode) &&
                        (stat_fp.st_mtime == stat_path.st_mtime),
                "repeated stat() gives different result");

        // empty file
        CHECK((fp = fopen(REF_EMPTY_FILE, "r")) != NULL, "unable to open '%s'", REF_EMPTY_FILE);
        CHECK(fstat(fileno(fp), &stat_fp) == 0, "unable to perform fstat() of empty file");
        CHECK(stat_fp.st_size == 0, "empty file size: %llu", (unsigned long long)stat_fp.st_size);
        CHECK(S_ISREG(stat_fp.st_mode), "empty file is not a regular file");

cleanup:
        fclose_null(&fp);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Two handles of the same file have independent positions.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_two_handles(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp1 = NULL;
        FILE *fp2 = NULL;
        const size_t chunk = 61;
        const size_t rounds = 40;
        const size_t pos2 = REF_BIG_SIZE / 2;

        CHECK((fp1 = fopen(REF_BIG_FILE, "r")) != NULL, "unable to open first handle");
        CHECK((fp2 = fopen(REF_BIG_FILE, "r")) != NULL, "unable to open second handle");
        CHECK(fseek(fp2, pos2, SEEK_SET) == 0, "fseek() failed");

        for (size_t i = 0; i < rounds; i++) {
                CHECK((err = check_pattern(fp1, i * chunk, chunk, REF_BIG_SEED, global->rbuf, chunk)) == NULL,
                        "first handle, round %u: %s", (unsigned)i, err);
                CHECK((err = check_pattern(fp2, pos2 + i * chunk, chunk, REF_BIG_SEED, global->rbuf,
                               chunk)) == NULL,
                        "second handle, round %u: %s", (unsigned)i, err);
        }

        CHECK(ftell(fp1) == (long)(rounds * chunk), "first handle position: %ld", ftell(fp1));
        CHECK(ftell(fp2) == (long)(pos2 + rounds * chunk), "second handle position: %ld", ftell(fp2));

        // closing one handle does not affect the other
        CHECK(fclose_null(&fp1) == 0, "fclose() of first handle failed");
        CHECK((err = check_pattern(fp2, pos2 + rounds * chunk, chunk, REF_BIG_SEED, global->rbuf, chunk)) ==
                        NULL,
                "second handle after close of first: %s", err);

cleanup:
        fclose_null(&fp2);
        fclose_null(&fp1);
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Several files opened at the same time and read alternately.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_multiple_open_files(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp[ARRAY_SIZE(REF_BLOCK_SIZES)] = {NULL};
        char *path = global->path[0];
        const size_t chunk = 37;
        size_t max_size = 0;

        for (size_t f = 0; f < ARRAY_SIZE(REF_BLOCK_SIZES); f++) {
                snprintf(path, FSTEST_PATH_BUF_LEN, REF_SIZES_DIR "/%u", (unsigned)REF_BLOCK_SIZES[f]);
                CHECK((fp[f] = fopen(path, "r")) != NULL, "unable to open '%s' (%u files opened)", path,
                        (unsigned)f);
                if (REF_BLOCK_SIZES[f] > max_size) {
                        max_size = REF_BLOCK_SIZES[f];
                }
        }

        for (size_t pos = 0; pos < max_size; pos += chunk) {
                for (size_t f = 0; f < ARRAY_SIZE(REF_BLOCK_SIZES); f++) {
                        const size_t size = REF_BLOCK_SIZES[f];

                        if (pos < size) {
                                CHECK((err = check_pattern(fp[f], pos, min(chunk, size - pos),
                                               (u8_t)(REF_SIZES_SEED + f), global->rbuf, chunk)) == NULL,
                                        "file %u B at %u: %s", (unsigned)size, (unsigned)pos, err);
                        }
                }
        }

        for (size_t f = 0; f < ARRAY_SIZE(REF_BLOCK_SIZES); f++) {
                CHECK((fgetc(fp[f]) == EOF) && feof(fp[f]), "file %u B: data after end of file",
                        (unsigned)REF_BLOCK_SIZES[f]);
        }

cleanup:
        for (size_t f = 0; f < ARRAY_SIZE(REF_BLOCK_SIZES); f++) {
                fclose_null(&fp[f]);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Directory listing of reference directories.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_listing(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        const u64_t all = ((u64_t)1 << FSTEST_DIR_FILES) - 1;
        int n;

        CHECK((err = scan_listing(REF_LIST_DIR, all)) == NULL, "%s: %s", REF_LIST_DIR, err);
        CHECK((err = scan_listing(REF_LIST_DIR, all)) == NULL, "%s (second pass): %s", REF_LIST_DIR, err);
        if (global->empty_dirs) {
                CHECK((err = scan_listing(REF_EMPTY_DIR, 0)) == NULL, "%s: %s", REF_EMPTY_DIR, err);
        }

        CHECK((n = count_entries(REF_DIR)) == ref_root_items(), "%s: %d entries, expected %d", REF_DIR, n,
                ref_root_items());

        for (size_t i = 0; i < ARRAY_SIZE(REF_ROOT_NAMES); i++) {
                if (!global->empty_dirs && (strcmp(REF_ROOT_NAMES[i], "empty.dir") == 0)) {
                        continue;
                }

                CHECK(dir_contains(REF_DIR, REF_ROOT_NAMES[i]), "%s: entry '%s' not listed", REF_DIR,
                        REF_ROOT_NAMES[i]);
        }

        CHECK((n = count_entries(REF_SIZES_DIR)) == (int)ARRAY_SIZE(REF_BLOCK_SIZES),
                "%s: %d entries, expected %d", REF_SIZES_DIR, n, (int)ARRAY_SIZE(REF_BLOCK_SIZES));

        // two directories listed at the same time
        {
                DIR *d1 = opendir(REF_LIST_DIR);
                DIR *d2 = opendir(REF_DIR);
                int n1 = 0, n2 = 0;
                bool ok = d1 && d2;

                while (ok) {
                        struct dirent *e1 = readdir(d1);
                        if (e1 && !isstreq(e1->d_name, ".") && !isstreq(e1->d_name, "..")) {
                                n1++;
                        }

                        struct dirent *e2 = readdir(d2);
                        if (e2 && !isstreq(e2->d_name, ".") && !isstreq(e2->d_name, "..")) {
                                n2++;
                        }

                        if (!e1 && !e2) {
                                break;
                        }
                }

                if (d1) {
                        closedir(d1);
                }
                if (d2) {
                        closedir(d2);
                }

                CHECK(ok, "unable to open two directories at the same time");
                CHECK((n1 == FSTEST_DIR_FILES) && (n2 == ref_root_items()),
                        "interleaved listing: %d and %d entries, expected %d and %d", n1, n2,
                        FSTEST_DIR_FILES, ref_root_items());
        }

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Nested reference directories.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_nested(void)
{
        static const struct {
                const char *dir;
                const char *entry;
        } LEVEL[] = {
                {REF_DIR, "l1"},
                {REF_NEST_1, "l2"},
                {REF_NEST_2, "l3"},
                {REF_NEST_3, "deep.file"},
        };

        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        int n;

        for (size_t i = 0; i < ARRAY_SIZE(LEVEL); i++) {
                CHECK(dir_contains(LEVEL[i].dir, LEVEL[i].entry), "'%s' not listed in '%s'", LEVEL[i].entry,
                        LEVEL[i].dir);

                if (i > 0) {
                        CHECK((n = count_entries(LEVEL[i].dir)) == 1, "'%s': %d entries, expected 1",
                                LEVEL[i].dir, n);
                }
        }

        CHECK((err = verify_pattern_file(REF_DEEP_FILE, REF_DEEP_SIZE, REF_DEEP_SEED, global->rbuf, 100)) ==
                        NULL,
                "%s", err);

cleanup:
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Long and special file names are listed without changes and can be
 *         opened.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_names(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        char *path = global->path[0];
        const char *name;
        int n;

        CHECK((n = count_entries(REF_NAMES_DIR)) == ref_names_count(), "%s: %d entries, expected %d",
                REF_NAMES_DIR, n, ref_names_count());

        for (size_t i = 0; i < ARRAY_SIZE(NAME_LEN); i++) {
                const size_t len = NAME_LEN[i];

                if (len > global->name_max) {
                        continue;
                }

                name = long_name_path(path, REF_NAMES_DIR, len);

                CHECK(dir_contains(REF_NAMES_DIR, name),
                        "name length %u: name not found (or mangled) in listing", (unsigned)len);
                CHECK((err = verify_pattern_file(path, len, (u8_t)len, global->rbuf, 64)) == NULL,
                        "name length %u: %s", (unsigned)len, err);
        }

        for (size_t i = 0; i < ARRAY_SIZE(SPECIAL_NAME); i++) {
                snprintf(path, FSTEST_PATH_BUF_LEN, REF_NAMES_DIR "/%s", SPECIAL_NAME[i]);
                name = SPECIAL_NAME[i];

                CHECK(dir_contains(REF_NAMES_DIR, name), "'%s': name not found (or mangled) in listing",
                        name);
                CHECK((err = verify_pattern_file(path, REF_NAME_SIZE, (u8_t)i, global->rbuf, 64)) == NULL,
                        "'%s': %s", name, err);
        }

cleanup:
        return exit_msg;
}

#if FSTEST_USE_STATFS
//==============================================================================
/**
 * @brief  statfs() returns consistent values.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_statfs(void)
{
        const char *exit_msg = SUCCESS_MSG;
        char *cwd = global->path[1];
        struct statfs sfs;

        CHECK(getcwd(cwd, FSTEST_PATH_BUF_LEN) != NULL, "getcwd() failed");
        CHECK(statfs(cwd, &sfs) == 0, "statfs(\"%s\") failed", cwd);

        if (global->verbose) {
                printf("\n  bsize: %llu, blocks: %llu, bfree: %llu", (unsigned long long)sfs.f_bsize,
                        (unsigned long long)sfs.f_blocks, (unsigned long long)sfs.f_bfree);
        }

        CHECK(sfs.f_bsize > 0, "f_bsize is 0");
        CHECK(sfs.f_blocks > 0, "f_blocks is 0");
        CHECK(sfs.f_bfree <= sfs.f_blocks, "f_bfree > f_blocks");
        CHECK((u64_t)(sfs.f_blocks - sfs.f_bfree) * sfs.f_bsize >= REF_BIG_SIZE,
                "used space (%llu B) smaller than reference data",
                (unsigned long long)((u64_t)(sfs.f_blocks - sfs.f_bfree) * sfs.f_bsize));

cleanup:
        return exit_msg;
}
#endif

#if FSTEST_USE_THREADS
//==============================================================================
/**
 * @brief  Several threads read files at the same time (also the same file).
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ref_threads(void)
{
        static const ref_file_t RFILE[FSTEST_THREADS] = {
                {.path = REF_BIG_FILE, .size = REF_BIG_SIZE, .seed = REF_BIG_SEED},
                {.path = REF_DEEP_FILE, .size = REF_DEEP_SIZE, .seed = REF_DEEP_SEED},
                {.path = REF_BIG_FILE, .size = REF_BIG_SIZE, .seed = REF_BIG_SEED},
        };

        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        fs_job_t job[FSTEST_THREADS];

        for (size_t i = 0; i < FSTEST_THREADS; i++) {
                job[i].path = RFILE[i].path;
                job[i].size = RFILE[i].size;
                job[i].seed = RFILE[i].seed;
                job[i].chunk = FSTEST_THREAD_BUF_SIZE - (i * 7);
                job[i].write = false;
        }

        if ((err = run_jobs(job, FSTEST_THREADS)) != NULL) {
                exit_msg = err; // already in msg_buf
                goto cleanup;
        }

cleanup:
        return exit_msg;
}
#endif

/*==============================================================================
  Read-only checks
==============================================================================*/

//==============================================================================
/**
 * @brief  Modifications of files are rejected by read-only file system.
 *
 * The first operation creates a new file. If it succeeds, the file system is
 * writable and the test stops before any reference file can be damaged.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ro_file_ops(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        struct stat st;
        bool new_file_created = false;

        errno = 0;
        if ((fp = fopen(REF_NEW_FILE, "w")) != NULL) {
                new_file_created = true;
                exit_msg = "fopen(\"w\") of new file: succeeded but should fail (file system is writable?)";
                goto cleanup;
        }
        int e = errno;
        CHECK((FSTEST_RO_ERRNO == 0) || (e == FSTEST_RO_ERRNO),
                "fopen(\"w\") of new file: wrong errno %d (%s)", e, strerror(e));

        EXPECT_FAIL(stat(REF_NEW_FILE, &st) != 0, ENOENT, "stat() of rejected new file");

        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE, "w")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"w\") of existing file");
        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE, "w+")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"w+\") of existing file");
        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE, "a")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"a\") of existing file");
        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE, "a+")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"a+\") of existing file");
        EXPECT_FAIL((fp = fopen(REF_SMALL_FILE, "r+")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"r+\") of existing file");
        EXPECT_FAIL(remove(REF_SMALL_FILE) != 0, FSTEST_RO_ERRNO, "remove() of file");
        EXPECT_FAIL(rename(REF_SMALL_FILE, REF_NEW_FILE) != 0, FSTEST_RO_ERRNO, "rename() of file");
        EXPECT_FAIL(chmod(REF_SMALL_FILE, 0) != 0, FSTEST_RO_ERRNO, "chmod() of file");

        // writing by handle opened for reading must fail
        CHECK((fp = fopen(REF_SMALL_FILE, "r")) != NULL, "unable to open '%s'", REF_SMALL_FILE);
        EXPECT_FAIL((fputc('x', fp) == EOF) || (fflush(fp) != 0), 0, "write to file opened by \"r\"");
        fclose_null(&fp);

        // nothing changed
        CHECK(stat(REF_SMALL_FILE, &st) == 0, "file disappeared");
        CHECK(st.st_size == REF_SMALL_SIZE, "file size changed: %llu", (unsigned long long)st.st_size);
        CHECK((err = verify_pattern_file(REF_SMALL_FILE, REF_SMALL_SIZE, REF_SMALL_SEED, global->rbuf, 64)) ==
                        NULL,
                "file content changed: %s", err);
        CHECK(stat(REF_NEW_FILE, &st) != 0, "new file exists");

cleanup:
        fclose_null(&fp);
        if (new_file_created) {
                // file system is not read-only: remove only object created by this test
                remove(REF_NEW_FILE);
        }
        return exit_msg;
}

//==============================================================================
/**
 * @brief  Modifications of directories are rejected by read-only file system.
 *
 * @return Error message.
 */
//==============================================================================
static const char *test_ro_dir_ops(void)
{
        const char *exit_msg = SUCCESS_MSG;
        const char *err;
        FILE *fp = NULL;
        struct stat st;
        const u64_t all = ((u64_t)1 << FSTEST_DIR_FILES) - 1;
        bool new_dir_created = false;

        errno = 0;
        if (mkdir(REF_NEW_DIR, 0777) == 0) {
                new_dir_created = true;
                exit_msg = "mkdir(): succeeded but should fail (file system is writable?)";
                goto cleanup;
        }
        int e = errno;
        CHECK((FSTEST_RO_ERRNO == 0) || (e == FSTEST_RO_ERRNO), "mkdir(): wrong errno %d (%s)", e,
                strerror(e));

        EXPECT_FAIL(stat(REF_NEW_DIR, &st) != 0, ENOENT, "stat() of rejected new directory");
        EXPECT_FAIL((fp = fopen(REF_LIST_DIR "/new.file", "w")) == NULL, FSTEST_RO_ERRNO,
                "fopen(\"w\") in subdirectory");
        EXPECT_FAIL(mkdir(REF_NEST_3 "/new.dir", 0777) != 0, FSTEST_RO_ERRNO, "mkdir() in nested directory");
        // empty directory is used if file system stores it, otherwise directory with files
        const char *dir = global->empty_dirs ? REF_EMPTY_DIR : REF_LIST_DIR;

        if (global->empty_dirs) {
                EXPECT_FAIL(remove(REF_EMPTY_DIR) != 0, FSTEST_RO_ERRNO, "remove() of empty directory");
        }
        EXPECT_FAIL(rename(dir, REF_NEW_DIR) != 0, FSTEST_RO_ERRNO, "rename() of directory");
        EXPECT_FAIL(chmod(dir, 0) != 0, FSTEST_RO_ERRNO, "chmod() of directory");

        // nothing changed
        CHECK(stat(dir, &st) == 0 && S_ISDIR(st.st_mode), "directory '%s' damaged", dir);
        CHECK(stat(REF_NEW_DIR, &st) != 0, "new directory exists");
        CHECK((err = scan_listing(REF_LIST_DIR, all)) == NULL, "%s: %s", REF_LIST_DIR, err);
        if (global->empty_dirs) {
                CHECK((err = scan_listing(REF_EMPTY_DIR, 0)) == NULL, "%s: %s", REF_EMPTY_DIR, err);
        }

cleanup:
        fclose_null(&fp);
        if (new_dir_created) {
                remove(REF_NEW_DIR);
        }
        return exit_msg;
}

/*==============================================================================
  End of file
==============================================================================*/
