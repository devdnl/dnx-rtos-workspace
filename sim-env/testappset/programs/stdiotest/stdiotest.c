/*==============================================================================
@file    stdiotest.c

@author  Daniel Zorychta

@brief   Tests of libc streams (FILE): buffering, setvbuf(), ungetc(),
         fseek()/ftell() of buffered streams, flush at exit, pipe EOF and
         stream locking. Test files are created in /tmp.

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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <dirent.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define FILE_A       "/tmp/stdiotest.a"
#define FILE_B       "/tmp/stdiotest.b"
#define FIFO         "/tmp/stdiotest.fifo"
#define TIMEOUT      5000
#define BIG_SIZE     1000 /* bigger than BUFSIZ */
#define THREAD_LINES 200
#define CMD_LEN      96
#define TMP_FILES    16

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef const char *(*test_fn_t)(void);

typedef struct {
        const char *name;
        test_fn_t fn;
} test_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static const char *test_rw_chars(void);
static const char *test_full_buffering(void);
static const char *test_line_buffering(void);
static const char *test_unbuffered(void);
static const char *test_user_buffer(void);
static const char *test_seek_tell(void);
static const char *test_update_mode(void);
static const char *test_ungetc(void);
static const char *test_fgets(void);
static const char *test_big_blocks(void);
static const char *test_flush_all(void);
static const char *test_exit_flush(void);
static const char *test_pipe_eof(void);
static const char *test_threads(void);
static const char *test_remove_open(void);
static const char *test_tmpfile(void);
static const char *test_tmpfile_unique(void);
static const char *test_mkstemp_tmpnam(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        const char *progname;
        char msg[96];
        char line[64];
        unsigned char big[BIG_SIZE];
        unsigned char big2[BIG_SIZE];
        char userbuf[32];
        FILE *shared;
};

static const test_t TESTS[] = {
        {.name = "fputc()/getc() on regular file", .fn = test_rw_chars},
        {.name = "Regular file fully buffered", .fn = test_full_buffering},
        {.name = "setvbuf(_IOLBF)", .fn = test_line_buffering},
        {.name = "setvbuf(_IONBF)", .fn = test_unbuffered},
        {.name = "setvbuf() with user buffer", .fn = test_user_buffer},
        {.name = "fseek()/ftell() of buffered stream", .fn = test_seek_tell},
        {.name = "Update mode (r+): read, write, read", .fn = test_update_mode},
        {.name = "ungetc()", .fn = test_ungetc},
        {.name = "fgets() of buffered stream", .fn = test_fgets},
        {.name = "fread()/fwrite() of big blocks", .fn = test_big_blocks},
        {.name = "fflush(NULL)", .fn = test_flush_all},
        {.name = "Flush at return from main() and exit()", .fn = test_exit_flush},
        {.name = "EOF of closed pipe (no extra bytes)", .fn = test_pipe_eof},
        {.name = "Stream shared by threads", .fn = test_threads},
        {.name = "remove() of open file (ramfs /tmp)", .fn = test_remove_open},
        {.name = "tmpfile(): write, read, no file left", .fn = test_tmpfile},
        {.name = "tmpfile(): unique files", .fn = test_tmpfile_unique},
        {.name = "mkstemp(), tmpnam()", .fn = test_mkstemp_tmpnam},
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(stdiotest, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Set error message.
 *
 * @param  what         description
 * @param  value        value
 *
 * @return Error message.
 */
//==============================================================================
static const char *fail(const char *what, long value)
{
        snprintf(global->msg, sizeof(global->msg), "%s (%ld)", what, value);
        return global->msg;
}

//==============================================================================
/**
 * @brief  Size of file read by stat() (data written to the file system).
 *
 * @param  path         file path
 *
 * @return File size or -1.
 */
//==============================================================================
static long file_size(const char *path)
{
        struct stat st;
        return (stat(path, &st) == 0) ? (long)st.st_size : -1;
}

//==============================================================================
/**
 * @brief  Read file content by read() (without stream).
 *
 * @param  path         file path
 * @param  buf          buffer
 * @param  size         buffer size
 *
 * @return Number of read bytes or -1.
 */
//==============================================================================
static int read_raw(const char *path, char *buf, size_t size)
{
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
                return -1;
        }

        ssize_t n = read(fd, buf, size - 1);
        close(fd);

        buf[(n > 0) ? n : 0] = '\0';
        return n;
}

//==============================================================================
/**
 * @brief  Characters written by fputc() are read back by getc().
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_rw_chars(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen(w) failed";
        }

        for (int i = 0; i < BIG_SIZE; i++) {
                if (fputc('A' + (i % 26), f) == EOF) {
                        fclose(f);
                        return fail("fputc failed", i);
                }
        }

        if (fclose(f) != 0) {
                return "fclose failed";
        }

        if (file_size(FILE_A) != BIG_SIZE) {
                return fail("wrong file size", file_size(FILE_A));
        }

        f = fopen(FILE_A, "r");
        if (!f) {
                return "fopen(r) failed";
        }

        int i = 0;
        int c;
        while ((c = getc(f)) != EOF) {
                if (c != 'A' + (i % 26)) {
                        fclose(f);
                        return fail("wrong character at", i);
                }
                i++;
        }

        bool eof = feof(f);
        bool again = (getc(f) == EOF);
        fclose(f);

        if (i != BIG_SIZE) {
                return fail("wrong number of read characters", i);
        }

        return (eof && again) ? NULL : "EOF indicator not set";
}

//==============================================================================
/**
 * @brief  Data of fully buffered stream is written by fflush().
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_full_buffering(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen failed";
        }

        fputs("buffered\n", f);

        long before = file_size(FILE_A);
        fflush(f);
        long after = file_size(FILE_A);
        fclose(f);

        if (before != 0) {
                return fail("data written before fflush()", before);
        }

        return (after == 9) ? NULL : fail("data not written by fflush()", after);
}

//==============================================================================
/**
 * @brief  Line buffered stream is written at new line character.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_line_buffering(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen failed";
        }

        if (setvbuf(f, NULL, _IOLBF, 0) != 0) {
                fclose(f);
                return "setvbuf failed";
        }

        fputs("abc", f);
        long part = file_size(FILE_A);
        fputs("def\nxyz", f);
        long line = file_size(FILE_A);
        fclose(f);
        long all = file_size(FILE_A);

        if (part != 0) {
                return fail("data without new line written", part);
        }

        if (line != 7) {
                return fail("line not written", line);
        }

        return (all == 10) ? NULL : fail("rest not written by fclose()", all);
}

//==============================================================================
/**
 * @brief  Unbuffered stream writes data immediately.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_unbuffered(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen failed";
        }

        setbuf(f, NULL);

        fputc('x', f);
        long n = file_size(FILE_A);
        fclose(f);

        return (n == 1) ? NULL : fail("data not written immediately", n);
}

//==============================================================================
/**
 * @brief  User buffer of setvbuf() is used (buffer full: data written).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_user_buffer(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen failed";
        }

        if (setvbuf(f, global->userbuf, _IOFBF, sizeof(global->userbuf)) != 0) {
                fclose(f);
                return "setvbuf failed";
        }

        for (size_t i = 0; i < sizeof(global->userbuf) - 1; i++) {
                fputc('u', f);
        }

        long below = file_size(FILE_A);
        bool in_buf = (memchr(global->userbuf, 'u', sizeof(global->userbuf)) != NULL);

        fputc('u', f); /* buffer full */
        long full = file_size(FILE_A);
        fclose(f);

        if (below != 0) {
                return fail("data written before buffer is full", below);
        }

        if (!in_buf) {
                return "user buffer not used";
        }

        return (full == (long)sizeof(global->userbuf)) ? NULL : fail("full buffer not written", full);
}

//==============================================================================
/**
 * @brief  File position of buffered stream (read and write).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_seek_tell(void)
{
        FILE *f = fopen(FILE_A, "w+");
        if (!f) {
                return "fopen failed";
        }

        const char *err = NULL;

        fputs("0123456789", f);
        if (ftell(f) != 10) {
                err = fail("ftell after write", (long)ftell(f));
                goto finish;
        }

        rewind(f);
        if ((getc(f) != '0') || (getc(f) != '1') || (ftell(f) != 2)) {
                err = fail("ftell after read", (long)ftell(f));
                goto finish;
        }

        if ((fseek(f, 3, SEEK_CUR) != 0) || (getc(f) != '5')) {
                err = "fseek(SEEK_CUR) of buffered stream";
                goto finish;
        }

        if ((fseek(f, -2, SEEK_END) != 0) || (getc(f) != '8') || (ftell(f) != 9)) {
                err = "fseek(SEEK_END)";
                goto finish;
        }

        if ((getc(f) != '9') || (getc(f) != EOF) || !feof(f)) {
                err = "EOF after last character";
                goto finish;
        }

        if ((fseek(f, 0, SEEK_SET) != 0) || feof(f)) {
                err = "EOF not cleared by fseek()";
        }

finish:
        fclose(f);
        return err;
}

//==============================================================================
/**
 * @brief  Update mode: write after read is placed at stream position.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_update_mode(void)
{
        FILE *f = fopen(FILE_A, "w");
        if (!f) {
                return "fopen(w) failed";
        }

        fputs("abcdefgh", f);
        fclose(f);

        f = fopen(FILE_A, "r+");
        if (!f) {
                return "fopen(r+) failed";
        }

        const char *err = NULL;

        if ((getc(f) != 'a') || (getc(f) != 'b')) {
                err = "read failed";
                goto finish;
        }

        fseek(f, 0, SEEK_CUR);
        fputs("XY", f);
        fseek(f, 0, SEEK_CUR);

        if (getc(f) != 'e') {
                err = "wrong character after write";
        }

finish:
        fclose(f);

        if (!err) {
                read_raw(FILE_A, global->line, sizeof(global->line));
                if (strcmp(global->line, "abXYefgh") != 0) {
                        snprintf(global->msg, sizeof(global->msg), "file content: '%s'", global->line);
                        err = global->msg;
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Character pushed back by ungetc() is read again.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_ungetc(void)
{
        FILE *f = fopen(FILE_A, "w+");
        if (!f) {
                return "fopen failed";
        }

        const char *err = NULL;

        fputs("12", f);
        rewind(f);

        int c = getc(f);
        if ((c != '1') || (ungetc(c, f) != '1')) {
                err = "ungetc failed";
                goto finish;
        }

        if (ftell(f) != 0) {
                err = fail("ftell after ungetc", (long)ftell(f));
                goto finish;
        }

        if (ungetc('x', f) != EOF) {
                err = "second ungetc accepted";
                goto finish;
        }

        if ((getc(f) != '1') || (getc(f) != '2') || (getc(f) != EOF)) {
                err = "wrong characters after ungetc";
                goto finish;
        }

        /* ungetc at EOF clears EOF indicator */
        if ((ungetc('z', f) != 'z') || feof(f) || (getc(f) != 'z')) {
                err = "ungetc at EOF";
        }

finish:
        fclose(f);
        return err;
}

//==============================================================================
/**
 * @brief  fgets() reads lines of buffered stream.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_fgets(void)
{
        FILE *f = fopen(FILE_A, "w+");
        if (!f) {
                return "fopen failed";
        }

        const char *err = NULL;

        for (int i = 0; i < 50; i++) {
                fprintf(f, "line %d\n", i);
        }
        fputs("last", f);
        rewind(f);

        int n = 0;
        while (fgets(global->line, sizeof(global->line), f)) {
                char expected[16];
                snprintf(expected, sizeof(expected), (n < 50) ? "line %d\n" : "last", n);
                if (strcmp(global->line, expected) != 0) {
                        err = fail("wrong line", n);
                        break;
                }
                n++;
        }

        fclose(f);

        if (!err && (n != 51)) {
                err = fail("wrong number of lines", n);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Blocks bigger than buffer are written and read correctly, also
 *         after buffered data.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_big_blocks(void)
{
        for (int i = 0; i < BIG_SIZE; i++) {
                global->big[i] = (unsigned char)(i * 7 + 3);
        }

        FILE *f = fopen(FILE_A, "w+");
        if (!f) {
                return "fopen failed";
        }

        const char *err = NULL;

        fwrite(global->big, 1, 5, f);
        fwrite(global->big + 5, 1, BIG_SIZE - 5, f);

        if (ftell(f) != BIG_SIZE) {
                err = fail("ftell after write", (long)ftell(f));
                goto finish;
        }

        rewind(f);
        memset(global->big2, 0, sizeof(global->big2));

        size_t n = fread(global->big2, 1, 3, f);
        n += fread(global->big2 + 3, 1, BIG_SIZE - 3, f);

        if ((n != BIG_SIZE) || (memcmp(global->big, global->big2, BIG_SIZE) != 0)) {
                err = fail("wrong data", (long)n);
                goto finish;
        }

        if ((fread(global->big2, 1, 1, f) != 0) || !feof(f)) {
                err = "EOF not set";
        }

finish:
        fclose(f);
        return err;
}

//==============================================================================
/**
 * @brief  fflush(NULL) writes all output streams.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_flush_all(void)
{
        FILE *a = fopen(FILE_A, "w");
        FILE *b = fopen(FILE_B, "w");
        const char *err = NULL;

        if (a && b) {
                fputs("aaa", a);
                fputs("bb", b);
                fflush(NULL);

                if ((file_size(FILE_A) != 3) || (file_size(FILE_B) != 2)) {
                        err = "data not written";
                }

        } else {
                err = "fopen failed";
        }

        if (a) {
                fclose(a);
        }

        if (b) {
                fclose(b);
        }

        remove(FILE_B);

        return err;
}

//==============================================================================
/**
 * @brief  Start program with stdout redirected to file and wait for exit.
 *
 * @param  cmd          command
 * @param  out          stdout file
 *
 * @return true on success.
 */
//==============================================================================
static bool run(const char *cmd, const char *out)
{
        process_attr_t attr;
        memset(&attr, 0, sizeof(attr));
        attr.fd_stdin = -1;
        attr.fd_stdout = -1;
        attr.p_stdout = out;
        attr.fd_stderr = STDERR_FILENO;
        attr.priority = PRIORITY_NORMAL;

        pid_t pid = process_create(cmd, &attr);
        if (pid == 0) {
                return false;
        }

        int status = -1;
        return (process_wait(pid, &status, TIMEOUT) == 0) && (status == 0);
}

//==============================================================================
/**
 * @brief  Buffered stdout of child (regular file) is written at exit.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_exit_flush(void)
{
        for (int i = 0; i < 2; i++) {
                const char *mode = (i == 0) ? "return" : "exit";

                char cmd[CMD_LEN];
                snprintf(cmd, sizeof(cmd), "%s --child %s", global->progname, mode);

                FILE *f = fopen(FILE_A, "w");
                if (f) {
                        fclose(f);
                }

                if (!run(cmd, FILE_A)) {
                        return "child process failed";
                }

                read_raw(FILE_A, global->line, sizeof(global->line));
                if (strcmp(global->line, "child output") != 0) {
                        snprintf(global->msg, sizeof(global->msg), "%s: '%s'", mode, global->line);
                        return global->msg;
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Closed pipe returns data and then EOF, no extra bytes are read.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_pipe_eof(void)
{
        remove(FIFO);

        if (mkfifo(FIFO, 0666) != 0) {
                return "mkfifo failed";
        }

        const char *err = NULL;

        FILE *p = fopen(FIFO, "r+");
        if (!p) {
                remove(FIFO);
                return "fopen failed";
        }

        fputs("ab", p);
        ioctl(fileno(p), IOCTL_PIPE__CLOSE);

        if ((getc(p) != 'a') || (getc(p) != 'b')) {
                err = "data not read";
                goto finish;
        }

        for (int i = 0; i < 3; i++) {
                int c = getc(p);
                if (c != EOF) {
                        err = fail("byte read after EOF", c);
                        goto finish;
                }
        }

        /* pipe closed when empty: blocking read returns EOF */
        ioctl(fileno(p), IOCTL_PIPE__CLEAR);
        ioctl(fileno(p), IOCTL_PIPE__CLOSE);
        clearerr(p);

        if ((getc(p) != EOF) || (getc(p) != EOF)) {
                err = "byte read from empty closed pipe";
        }

finish:
        fclose(p);
        remove(FIFO);
        return err;
}

//==============================================================================
/**
 * @brief  Thread writes lines to shared stream.
 *
 * @param  arg          character of thread
 *
 * @return 0.
 */
//==============================================================================
static int thread_func(void *arg)
{
        char c = *(const char *)arg;

        for (int i = 0; i < THREAD_LINES; i++) {
                flockfile(global->shared);
                for (int j = 0; j < 9; j++) {
                        putc_unlocked(c, global->shared);
                }
                fputc('\n', global->shared); /* recursive lock */
                funlockfile(global->shared);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Lines written by two threads are not mixed (flockfile()).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_threads(void)
{
        global->shared = fopen(FILE_A, "w");
        if (!global->shared) {
                return "fopen failed";
        }

        static const char CHARS[2] = {'1', '2'};
        const thread_attr_t attr = {
                .stack_depth = STACK_DEPTH_LOW,
                .priority = PRIORITY_NORMAL,
                .detached = false,
        };

        tid_t t1 = thread_create(thread_func, &attr, (void *)&CHARS[0]);
        tid_t t2 = thread_create(thread_func, &attr, (void *)&CHARS[1]);

        if (t1) {
                thread_join2(t1, NULL, TIMEOUT);
        }

        if (t2) {
                thread_join2(t2, NULL, TIMEOUT);
        }

        fclose(global->shared);

        if (!t1 || !t2) {
                return "thread not created";
        }

        FILE *f = fopen(FILE_A, "r");
        if (!f) {
                return "fopen(r) failed";
        }

        const char *err = NULL;
        int lines = 0;

        while (fgets(global->line, sizeof(global->line), f)) {
                if ((strlen(global->line) != 10) ||
                        (strspn(global->line, "1") != 9 && strspn(global->line, "2") != 9)) {
                        err = fail("mixed line", lines);
                        break;
                }
                lines++;
        }

        fclose(f);

        if (!err && (lines != 2 * THREAD_LINES)) {
                err = fail("wrong number of lines", lines);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Number of entries in directory.
 *
 * @param  path         directory path
 *
 * @return Number of entries or -1.
 */
//==============================================================================
static int dir_entries(const char *path)
{
        DIR *dir = opendir(path);
        if (!dir) {
                return -1;
        }

        int n = 0;
        while (readdir(dir)) {
                n++;
        }

        closedir(dir);
        return n;
}

//==============================================================================
/**
 * @brief  Removed open file: name is free at once (new file with the same
 *         name is other file), data of open file are kept until close.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_remove_open(void)
{
        FILE *old = fopen(FILE_B, "w+");
        if (!old) {
                return "fopen failed";
        }

        const char *err = NULL;
        FILE *new = NULL;

        if ((fputs("old", old) == EOF) || (fflush(old) != 0) || (remove(FILE_B) != 0)) {
                err = "write or remove failed";

        } else if (file_size(FILE_B) != -1) {
                err = "name exists after remove()";

        } else if (!(new = fopen(FILE_B, "w+")) || (fputs("new data", new) == EOF) || (fflush(new) != 0)) {
                err = "new file with the same name not created";

        } else {
                rewind(old);
                if (!fgets(global->line, sizeof(global->line), old) || (strcmp(global->line, "old") != 0)) {
                        err = "data of removed open file changed";
                }
        }

        fclose(old);

        if (new) {
                if (!err && (file_size(FILE_B) != 8)) {
                        err = fail("wrong size of new file", file_size(FILE_B));
                }
                fclose(new);
        }

        remove(FILE_B);
        return err;
}

//==============================================================================
/**
 * @brief  Temporary file is read back and its name is removed just after
 *         creation (P_tmpdir on ramfs).
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_tmpfile(void)
{
        int entries = dir_entries(P_tmpdir);

        FILE *f = tmpfile();
        if (!f) {
                return fail("tmpfile failed, errno", errno);
        }

        const char *err = NULL;

        if (dir_entries(P_tmpdir) != entries) {
                err = fail("file left in " P_tmpdir ", entries", dir_entries(P_tmpdir));

        } else if ((fputs("temporary data", f) == EOF) || (fflush(f) != 0)) {
                err = "write failed";

        } else {
                rewind(f);
                if (!fgets(global->line, sizeof(global->line), f) || (strcmp(global->line, "temporary data") != 0)) {
                        err = "wrong data read back";
                }
        }

        fclose(f);

        if (!err && (dir_entries(P_tmpdir) != entries)) {
                err = "directory changed after fclose()";
        }

        return err;
}

//==============================================================================
/**
 * @brief  Temporary files created one after another (the same millisecond)
 *         are different files.
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_tmpfile_unique(void)
{
        FILE *f[TMP_FILES];
        const char *err = NULL;
        int n;

        for (n = 0; n < TMP_FILES; n++) {
                f[n] = tmpfile();
                if (!f[n]) {
                        err = fail("tmpfile failed, file", n);
                        break;
                }

                if (fputc('a' + n, f[n]) == EOF) {
                        err = fail("fputc failed, file", n);
                        n++;
                        break;
                }
        }

        for (int i = 0; i < n; i++) {
                rewind(f[i]);
                int c = getc(f[i]);
                if (!err && ((c != 'a' + i) || (getc(f[i]) != EOF))) {
                        err = fail("wrong content of file", i);
                }
                fclose(f[i]);
        }

        return err;
}

//==============================================================================
/**
 * @brief  mkstemp() creates unique file (mode 0600), wrong template, tmpnam().
 *
 * @return NULL on success, otherwise error message.
 */
//==============================================================================
static const char *test_mkstemp_tmpnam(void)
{
        char name1[] = P_tmpdir "/stdiotestXXXXXX";
        char name2[] = P_tmpdir "/stdiotestXXXXXX";
        const char *err = NULL;

        int fd1 = mkstemp(name1);
        int fd2 = mkstemp(name2);
        struct stat st;

        if ((fd1 < 0) || (fd2 < 0)) {
                err = fail("mkstemp failed, errno", errno);

        } else if (strcmp(name1, name2) == 0) {
                err = "the same names";

        } else if (strstr(name1, "XXXXXX")) {
                err = "template not replaced";

        } else if ((stat(name1, &st) != 0) || ((st.st_mode & S_IPMT) != (S_IRUSR | S_IWUSR))) {
                err = fail("wrong mode of file", (long)(st.st_mode & S_IPMT));

        } else if ((write(fd1, "x", 1) != 1) || (lseek(fd1, 0, SEEK_SET) != 0) ||
                   (read(fd1, global->line, 2) != 1)) {
                err = "read/write failed";
        }

        if (fd1 >= 0) {
                close(fd1);
                remove(name1);
        }

        if (fd2 >= 0) {
                close(fd2);
                remove(name2);
        }

        if (!err) {
                char bad[] = P_tmpdir "/stdiotestXXXX";
                errno = 0;
                if ((mkstemp(bad) != -1) || (errno != EINVAL)) {
                        err = "mkstemp() accepted wrong template";
                }
        }

        if (!err) {
                char name[L_tmpnam];
                if (tmpnam(name) != name) {
                        err = fail("tmpnam failed, errno", errno);

                } else if ((strncmp(name, P_tmpdir "/", sizeof(P_tmpdir)) != 0) || (stat(name, &st) == 0)) {
                        err = "wrong name of tmpnam()";

                } else if (tmpnam(NULL) != NULL) {
                        err = "tmpnam(NULL) returned name";
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Program main function.
 *
 * @param  argc         argument count
 * @param  argv         arguments
 *
 * @return Program status.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->progname = argv[0];

        /* child process: buffered output written at return or exit() */
        if ((argc == 3) && (strcmp(argv[1], "--child") == 0)) {
                printf("child output");

                if (strcmp(argv[2], "exit") == 0) {
                        exit(0);
                }

                return 0;
        }

        unsigned executed = 0, passed = 0, failed = 0;

        for (size_t i = 0; i < sizeof(TESTS) / sizeof(TESTS[0]); i++) {
                const char *err = TESTS[i].fn();
                executed++;

                if (err) {
                        failed++;
                        printf("  %-44s FAIL: %s\n", TESTS[i].name, err);
                } else {
                        passed++;
                        printf("  %-44s PASS\n", TESTS[i].name);
                }
        }

        remove(FILE_A);

        printf("SUMMARY: executed: %u, passed: %u, failed: %u, skipped: 0\n", executed, passed, failed);

        return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
