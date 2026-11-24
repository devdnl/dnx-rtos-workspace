/*==============================================================================
@file   stream.c
@author Daniel Zorychta
@brief  Buffered streams (FILE).

@note   Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

        MIT License

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

        Buffering rules:
        - regular files are fully buffered (_IOFBF, BUFSIZ bytes),
        - other files (terminals, pipes, devices) and stderr are unbuffered
          (_IONBF): data is transferred immediately as before, so programs
          that share these files with child processes or use read()/write()
          on the same descriptor work as before,
        - setvbuf()/setbuf() select other mode or own buffer,
        - buffer is allocated at first read or write,
        - __LIBC_STREAM_BUFFERING__ == _NO_: all streams are unbuffered
          (setvbuf() is accepted, buffer is not used),
        - all output streams are flushed by exit(), at return from main() and
          before new process is created (process_create(), system()).

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <dnx/thread.h>
#include <dnx/misc.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"
#include "atomic.h"

/*==============================================================================
  Local macros
==============================================================================*/
/* default: enabled (flag not defined in configuration) */
#ifndef __LIBC_STREAM_BUFFERING__
#define __LIBC_STREAM_BUFFERING__ 1
#endif

#define NO_OWNER     (-2)
#define get_appctx() ((_libc_app_ctx_t *)_libc_appctx_get())

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
int _libc_fclose(FILE *file);
int _libc_fflush(FILE *file);

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/
extern void *_libc_malloc(size_t size);
extern void _libc_free(void *ptr);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Convert mode string to open flags.
 *
 * @param  str          mode string
 *
 * @return Open flags.
 */
//==============================================================================
static u32_t parse_flags(const char *str)
{
        if (strcmp("r", str) == 0 || strcmp("rb", str) == 0) {
                return O_RDONLY;
        }

        if (strcmp("r+", str) == 0 || strcmp("rb+", str) == 0 || strcmp("r+b", str) == 0) {
                return O_RDWR;
        }

        if (strcmp("w", str) == 0 || strcmp("wb", str) == 0) {
                return O_WRONLY | O_CREAT | O_TRUNC;
        }

        if (strcmp("w+", str) == 0 || strcmp("wb+", str) == 0 || strcmp("w+b", str) == 0) {
                return O_RDWR | O_CREAT | O_TRUNC;
        }

        if (strcmp("a", str) == 0 || strcmp("ab", str) == 0) {
                return O_WRONLY | O_CREAT | O_APPEND;
        }

        if (strcmp("a+", str) == 0 || strcmp("ab+", str) == 0 || strcmp("a+b", str) == 0) {
                return O_RDWR | O_CREAT | O_APPEND;
        }

        return O_RDONLY;
}

//==============================================================================
/**
 * @brief  Lock stream (recursive with flockfile()).
 *
 * @param  file         stream
 *
 * @return true if lock was taken (must be released by unlock_stream()),
 *         false if stream is already locked by flockfile() of this thread.
 */
//==============================================================================
static bool lock_stream(FILE *file)
{
        if (!_libc_atomic_flag_test_and_set(&file->lock)) {
                return true;
        }

        if ((file->owner != NO_OWNER) && (file->owner == thread_current())) {
                return false;
        }

        futex_lock(&file->lock, WAIT_FOREVER_MS);
        return true;
}

//==============================================================================
/**
 * @brief  Unlock stream locked by lock_stream().
 *
 * @param  file         stream
 * @param  locked       value returned by lock_stream()
 */
//==============================================================================
static void unlock_stream(FILE *file, bool locked)
{
        if (locked) {
                futex_unlock(&file->lock);
        }
}

//==============================================================================
/**
 * @brief  Add stream to the list of process streams.
 *
 * @param  file         stream
 */
//==============================================================================
static void list_add(FILE *file)
{
        _libc_app_ctx_t *ctx = get_appctx();
        if (ctx) {
                futex_lock(&ctx->_files_lock, WAIT_FOREVER_MS);
                file->next = ctx->_files;
                ctx->_files = file;
                futex_unlock(&ctx->_files_lock);
        }
}

//==============================================================================
/**
 * @brief  Remove stream from the list of process streams.
 *
 * @param  file         stream
 */
//==============================================================================
static void list_remove(FILE *file)
{
        _libc_app_ctx_t *ctx = get_appctx();
        if (ctx) {
                futex_lock(&ctx->_files_lock, WAIT_FOREVER_MS);

                for (FILE **f = &ctx->_files; *f; f = &(*f)->next) {
                        if (*f == file) {
                                *f = file->next;
                                break;
                        }
                }

                futex_unlock(&ctx->_files_lock);
        }
}

//==============================================================================
/**
 * @brief  Create stream object for file descriptor.
 *
 * @param  fd           file descriptor
 *
 * @return Stream or NULL when no memory.
 */
//==============================================================================
static FILE *stream_new(int fd)
{
        FILE *f = _libc_malloc(sizeof(*f));
        if (f) {
                memset(f, 0, sizeof(*f));
                f->fd = fd;
                f->unget = EOF;
                f->owner = NO_OWNER;
                list_add(f);
        }

        return f;
}

//==============================================================================
/**
 * @brief  Release stream object (descriptor is not closed).
 *
 * @param  file         stream
 */
//==============================================================================
static void stream_free(FILE *file)
{
        if (file->flag.own_buf) {
                _libc_free(file->buf);
        }

        _libc_free(file);
}

//==============================================================================
/**
 * @brief  Select default buffer mode of stream (at first operation).
 *
 * @param  file         stream
 */
//==============================================================================
static void stream_setup(FILE *file)
{
        if (!file->flag.setup) {
                file->flag.setup = true;
                file->flag.mode = _IONBF;

                struct stat st;
                if ((fstat(file->fd, &st) == 0) && S_ISREG(st.st_mode)) {
                        file->flag.mode = _IOFBF;
                        file->bufsize = BUFSIZ;
                }
        }
}

//==============================================================================
/**
 * @brief  Allocate stream buffer if needed.
 *
 * @param  file         stream
 *
 * @return true if stream is buffered (buffer exists), false if unbuffered.
 */
//==============================================================================
static bool stream_buffer(FILE *file)
{
#if __LIBC_STREAM_BUFFERING__ == 0
        // buffering is disabled: all streams are unbuffered
        (void)file;
        return false;
#endif
        stream_setup(file);

        if (file->flag.mode == _IONBF) {
                return false;
        }

        if (!file->buf) {
                file->buf = _libc_malloc(file->bufsize);
                if (!file->buf) {
                        /* no memory: stream works as unbuffered */
                        file->flag.mode = _IONBF;
                        return false;
                }

                file->flag.own_buf = true;
        }

        return true;
}

//==============================================================================
/**
 * @brief  Write buffered data to the file.
 *
 * @param  file         stream
 *
 * @return true on success, false on error (buffer is dropped).
 */
//==============================================================================
static bool flush_write(FILE *file)
{
#if __LIBC_STREAM_BUFFERING__ == 0
        // no buffered data
        (void)file;
        return true;
#endif
        bool ok = true;

        if (file->flag.wr) {
                size_t n = 0;

                while (n < file->pos) {
                        ssize_t wr = write(file->fd, file->buf + n, file->pos - n);
                        if (wr <= 0) {
                                file->flag.error = true;
                                ok = false;
                                break;
                        }

                        n += wr;
                }

                file->pos = 0;
                file->flag.wr = false;
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Drop read data (buffer and ungetc() character). File position is
 *         moved back to the position of stream.
 *
 * @param  file         stream
 */
//==============================================================================
static void drop_read(FILE *file)
{
        int64_t back = 0;

        if (file->flag.rd) {
                back += file->len - file->pos;
        }

        if (file->unget != EOF) {
                back += 1;
        }

        if (back > 0) {
                lseek64(file->fd, -back, SEEK_CUR);
        }

        file->pos = 0;
        file->len = 0;
        file->unget = EOF;
        file->flag.rd = false;
}

//==============================================================================
/**
 * @brief  Write data to stream buffer (buffer exists, stream locked). Full
 *         buffer is written to the file, big blocks are written directly.
 *
 * @param  file         stream
 * @param  src          data
 * @param  size         data size
 *
 * @return Number of accepted bytes.
 */
//==============================================================================
static size_t buffer_write(FILE *file, const u8_t *src, size_t size)
{
        size_t done = 0;

        while (done < size) {

                /* big block is written directly (buffered data first) */
                if ((size - done >= file->bufsize) && (file->pos == 0)) {
                        ssize_t n = write(file->fd, src + done, size - done);
                        if (n < 0) {
                                file->flag.error = true;
                                return done;
                        }

                        return done + n;
                }

                size_t n = min(file->bufsize - file->pos, size - done);
                memcpy(file->buf + file->pos, src + done, n);
                file->pos += n;
                done += n;
                file->flag.wr = true;

                if ((file->pos == file->bufsize) && !flush_write(file)) {
                        return 0;
                }
        }

        return done;
}

//==============================================================================
/**
 * @brief  Write data to stream (stream locked).
 *
 * @param  file         stream
 * @param  data         data
 * @param  size         data size
 *
 * @return Number of accepted bytes.
 */
//==============================================================================
static size_t stream_write(FILE *file, const void *data, size_t size)
{
        const u8_t *src = data;

        if (file->flag.rd || (file->unget != EOF)) {
                drop_read(file);
        }

        if (!stream_buffer(file)) {
                ssize_t n = write(file->fd, src, size);
                if (n < 0) {
                        file->flag.error = true;
                        return 0;
                }

                return n;
        }

        if (file->flag.mode == _IOLBF) {
                /* data up to the last new line character is written to the file */
                size_t line = size;
                while ((line > 0) && (src[line - 1] != '\n')) {
                        line--;
                }

                if (line > 0) {
                        size_t n = buffer_write(file, src, line);
                        if ((n != line) || !flush_write(file)) {
                                return 0;
                        }

                        return n + buffer_write(file, src + line, size - line);
                }
        }

        return buffer_write(file, src, size);
}

//==============================================================================
/**
 * @brief  Read data from stream (stream locked).
 *
 * @param  file         stream
 * @param  data         destination buffer
 * @param  size         number of bytes to read
 *
 * @return Number of read bytes.
 */
//==============================================================================
static size_t stream_read(FILE *file, void *data, size_t size)
{
        u8_t *dst = data;
        size_t done = 0;

        if (size == 0) {
                return 0;
        }

        if (file->flag.wr && !flush_write(file)) {
                return 0;
        }

        if (file->unget != EOF) {
                dst[done++] = file->unget;
                file->unget = EOF;
        }

        if (!stream_buffer(file)) {
                /*
                 * Unbuffered stream: short read sets EOF indicator (e.g.
                 * non-blocking read of terminal without data).
                 */
                if (done < size) {
                        ssize_t n = read(file->fd, dst + done, size - done);
                        if (n < 0) {
                                file->flag.error = true;

                        } else {
                                if ((size_t)n < (size - done)) {
                                        file->flag.eof = true;
                                }

                                done += n;
                        }
                }

                return done;
        }

        while (done < size) {

                if (file->flag.rd && (file->pos < file->len)) {
                        size_t n = min(file->len - file->pos, size - done);
                        memcpy(dst + done, file->buf + file->pos, n);
                        file->pos += n;
                        done += n;
                        continue;
                }

                file->pos = 0;
                file->len = 0;
                file->flag.rd = false;

                ssize_t n;
                if (size - done >= file->bufsize) {
                        /* large block is read directly */
                        n = read(file->fd, dst + done, size - done);
                        if (n > 0) {
                                done += n;
                        }

                } else {
                        n = read(file->fd, file->buf, file->bufsize);
                        if (n > 0) {
                                file->len = n;
                                file->flag.rd = true;
                        }
                }

                if (n < 0) {
                        file->flag.error = true;
                        break;

                } else if (n == 0) {
                        file->flag.eof = true;
                        break;
                }
        }

        return done;
}

//==============================================================================
/**
 * @brief Function opens file.
 *
 * The fopen() function opens the file whose name is the string pointed to by
 * <i>path</i> and associates a stream with it. The argument <i>mode</i> points
 * to a string beginning with one of the following sequences: r, r+, w, w+, a,
 * a+ (see stdio.h).
 *
 * @param path          path to file
 * @param mode          file open mode
 *
 * @return Upon successful completion fopen(), return a <b>FILE</b> pointer.
 * Otherwise, @ref NULL is returned and @ref errno is set to indicate the
 * error.
 */
//==============================================================================
FILE *_libc_fopen(const char *path, const char *mode)
{
        int fd = open(path, parse_flags(mode), 0666); // modified by umask
        if (fd < 0) {
                return NULL;
        }

        FILE *f = stream_new(fd);
        if (!f) {
                close(fd);
        }

        return f;
}

//==============================================================================
/**
 * @brief  Function open file by using selected descriptor.
 *
 * @param  fd           file descriptor
 * @param  mode         file mode
 *
 * @return Upon successful completion fdopen(), return a <b>FILE</b> pointer.
 * Otherwise, @ref NULL is returned and @ref errno is set to indicate the
 * error.
 */
//==============================================================================
FILE *_libc_fdopen(int fd, const char *mode)
{
        (void)mode;

        if (fd < 0) {
                errno = EBADF;
                return NULL;
        }

        return stream_new(fd);
}

//==============================================================================
/**
 * @brief Function closes selected file.
 *
 * The fclose() function flushes the stream pointed to by <i>file</i> and
 * closes the underlying file descriptor.
 *
 * @param file          file to close
 *
 * @return Upon successful completion \b 0 is returned. Otherwise, @ref EOF is
 * returned and @ref errno is set to indicate the error. In either case any
 * further access (including another call to fclose()) to the stream results
 * in undefined behavior.
 */
//==============================================================================
int _libc_fclose(FILE *file)
{
        if (!file) {
                errno = EINVAL;
                return EOF;
        }

        list_remove(file);

        bool locked = lock_stream(file);
        bool ok = flush_write(file);
        unlock_stream(file, locked);

        if (close(file->fd) != 0) {
                ok = false;
        }

        if (file->tmppath) {
                unlink(file->tmppath);
                _libc_free(file->tmppath);
        }

        stream_free(file);

        return ok ? 0 : EOF;
}

//==============================================================================
/**
 * @brief Function writes data to stream.
 *
 * @param ptr           pointer to data
 * @param size          element size
 * @param count         number of elements
 * @param file          stream
 *
 * @return On success, fwrite() return the number of items written. If an
 * error occurs the return value is a short item count (or zero).
 */
//==============================================================================
size_t _libc_fwrite(const void *ptr, size_t size, size_t count, FILE *file)
{
        if (!file || !ptr) {
                errno = EINVAL;
                return 0;
        }

        if ((size == 0) || (count == 0)) {
                return 0;
        }

        bool locked = lock_stream(file);
        size_t n = stream_write(file, ptr, size * count);
        unlock_stream(file, locked);

        return n / size;
}

//==============================================================================
/**
 * @brief Function reads data from stream.
 *
 * @param ptr           pointer to data
 * @param size          element size
 * @param count         number of elements
 * @param file          stream
 *
 * @return On success, fread() return the number of items read. If an error
 * occurs, or the end of the file is reached, the return value is a short item
 * count (or zero).
 */
//==============================================================================
size_t _libc_fread(void *ptr, size_t size, size_t count, FILE *file)
{
        if (!file || !ptr) {
                errno = EINVAL;
                return 0;
        }

        if ((size == 0) || (count == 0)) {
                return 0;
        }

        bool locked = lock_stream(file);
        size_t n = stream_read(file, ptr, size * count);
        unlock_stream(file, locked);

        return n / size;
}

//==============================================================================
/**
 * @brief Function sets file position of stream.
 *
 * Buffered data is written or dropped, ungetc() character is dropped and EOF
 * indicator is cleared.
 *
 * @param file          stream
 * @param offset        offset
 * @param mode          SEEK_SET, SEEK_CUR or SEEK_END
 *
 * @return Upon successful completion 0, otherwise -1.
 */
//==============================================================================
int _libc_fseek(FILE *file, int64_t offset, int mode)
{
        if (!file) {
                errno = EINVAL;
                return -1;
        }

        int status = -1;
        bool locked = lock_stream(file);

        if (flush_write(file)) {
                if (mode == SEEK_CUR) {
                        if (file->flag.rd) {
                                offset -= file->len - file->pos;
                        }

                        if (file->unget != EOF) {
                                offset -= 1;
                        }
                }

                file->pos = 0;
                file->len = 0;
                file->unget = EOF;
                file->flag.rd = false;

                if (lseek64(file->fd, offset, mode) >= 0) {
                        file->flag.eof = false;
                        status = 0;
                }
        }

        unlock_stream(file, locked);

        return status;
}

//==============================================================================
/**
 * @brief Function returns file position of stream.
 *
 * @param file          stream
 *
 * @return Upon successful completion file position, otherwise -1.
 */
//==============================================================================
int64_t _libc_ftell(FILE *file)
{
        if (!file) {
                errno = EINVAL;
                return -1;
        }

        bool locked = lock_stream(file);

        int64_t seek = lseek64(file->fd, 0, SEEK_CUR);
        if (seek >= 0) {
                if (file->flag.rd) {
                        seek -= file->len - file->pos;
                }

                if (file->unget != EOF) {
                        seek -= 1;
                }

                if (file->flag.wr) {
                        seek += file->pos;
                }

                if (seek < 0) {
                        seek = 0;
                }
        }

        unlock_stream(file, locked);

        return seek;
}

//==============================================================================
/**
 * @brief  Function writes buffered data of all output streams of process.
 *
 * @return Upon successful completion 0, otherwise @ref EOF.
 */
//==============================================================================
static int flush_all(void)
{
        int status = 0;

        _libc_app_ctx_t *ctx = get_appctx();
        if (ctx) {
                futex_lock(&ctx->_files_lock, WAIT_FOREVER_MS);

                for (FILE *f = ctx->_files; f; f = f->next) {
                        if (f->flag.wr) {
                                bool locked = lock_stream(f);
                                if (!flush_write(f)) {
                                        status = EOF;
                                }
                                unlock_stream(f, locked);
                        }
                }

                futex_unlock(&ctx->_files_lock);
        }

        return status;
}

//==============================================================================
/**
 * @brief Function forces write buffers to stream.
 *
 * For output streams fflush() writes buffered data, for input streams drops
 * buffered data (file position is set to position of stream). When
 * <i>file</i> is NULL then all output streams of process are flushed.
 *
 * @param file          stream (NULL: all streams)
 *
 * @return Upon successful completion 0 is returned. Otherwise, @ref EOF is
 * returned and @ref errno is set to indicate the error.
 */
//==============================================================================
int _libc_fflush(FILE *file)
{
        if (!file) {
                return flush_all();
        }

        bool locked = lock_stream(file);

        bool ok = flush_write(file);

        if (file->flag.rd || (file->unget != EOF)) {
                drop_read(file);
        }

        unlock_stream(file, locked);

        if (ok) {
                int err = _libc_syscall(_LIBC_SYS_FLUSH, &file->fd);
                if (!err) {
                        return 0;
                }
        }

        return EOF;
}

//==============================================================================
/**
 * @brief  Function flushes all output streams at program exit.
 */
//==============================================================================
void _libc_stdio_exit(void)
{
        flush_all();
}

//==============================================================================
/**
 * @brief Function tests the end-of-file indicator.
 *
 * @param file          stream
 *
 * @return Returns nonzero if @ref EOF indicator is set, otherwise 0 is
 * returned.
 */
//==============================================================================
int _libc_feof(FILE *file)
{
        if (file) {
                return file->flag.eof;

        } else {
                errno = EINVAL;
        }

        return 0;
}

//==============================================================================
/**
 * @brief Function clears end-of-file and error indicators.
 *
 * @param file          stream
 */
//==============================================================================
void _libc_clearerr(FILE *file)
{
        if (file) {
                file->flag.eof = false;
                file->flag.error = false;
        }
}

//==============================================================================
/**
 * @brief Function tests the error indicator.
 *
 * @param file          stream
 *
 * @return Returns nonzero if error indicator is set, otherwise 0 is returned.
 */
//==============================================================================
int _libc_ferror(FILE *file)
{
        if (file) {
                if (file->flag.error) {
                        return 1;
                } else {
                        return 0;
                }
        } else {
                errno = EINVAL;
        }

        return 1;
}

//==============================================================================
/**
 * @brief Function returns file descriptor of stream.
 *
 * @param fp            stream
 *
 * @return File descriptor or -1 on error.
 */
//==============================================================================
int _libc_fileno(FILE *fp)
{
        if (fp) {
                return fp->fd;
        } else {
                errno = EINVAL;
                return -1;
        }
}

//==============================================================================
/**
 * @brief Function sets stream buffer mode.
 *
 * Buffered data is written (or dropped for input) before the change.
 *
 * @param file      stream
 * @param buffer    buffer (NULL: allocated by libc at first operation)
 * @param mode      buffer mode (@ref _IONBF, @ref _IOLBF, @ref _IOFBF)
 * @param size      buffer size (0: @ref BUFSIZ)
 *
 * @return 0 on success, otherwise nonzero value and errno is set.
 */
//==============================================================================
int _libc_setvbuf(FILE *file, char *buffer, int mode, size_t size)
{
        if (!file || ((mode != _IOFBF) && (mode != _IOLBF) && (mode != _IONBF))) {
                errno = EINVAL;
                return -1;
        }

        bool locked = lock_stream(file);

        int status = flush_write(file) ? 0 : -1;

        if (file->flag.rd || (file->unget != EOF)) {
                drop_read(file);
        }

        if (file->flag.own_buf) {
                _libc_free(file->buf);
        }

        file->buf = NULL;
        file->flag.own_buf = false;
        file->flag.setup = true;
        file->flag.mode = mode;
        file->bufsize = 0;

        if (mode != _IONBF) {
                file->bufsize = (size > 0) ? size : BUFSIZ;
                file->buf = (unsigned char *)buffer;
        }

        unlock_stream(file, locked);

        return status;
}

//==============================================================================
/**
 * @brief Function pushes character back to stream.
 *
 * One character can be pushed back.
 *
 * @param c         character
 * @param file      stream
 *
 * @return Character on success, otherwise @ref EOF.
 */
//==============================================================================
int _libc_ungetc(int c, FILE *file)
{
        if (!file || (c == EOF)) {
                return EOF;
        }

        int status = EOF;
        bool locked = lock_stream(file);

        if ((file->unget == EOF) && flush_write(file)) {
                file->unget = (unsigned char)c;
                file->flag.eof = false;
                status = (unsigned char)c;
        }

        unlock_stream(file, locked);

        return status;
}

//==============================================================================
/**
 * @brief Function get character from stream without locking.
 *
 * @param stream            source file
 *
 * @return Character or EOF.
 */
//==============================================================================
int getc_unlocked(FILE *stream)
{
        if (!stream) {
                return EOF;
        }

        /* fast path: data in read buffer */
        if (stream->flag.rd && (stream->unget == EOF) && (stream->pos < stream->len)) {
                return stream->buf[stream->pos++];
        }

        /*
         * Character is returned when it was read: EOF indicator can be set by
         * previous read (e.g. non-blocking read of terminal without data), so
         * it cannot be checked here (CTRL+C was lost in programs that poll
         * keyboard, e.g. hexdump).
         */
        unsigned char chr = 0;
        if (stream_read(stream, &chr, 1) == 1) {
                return chr;
        }

        return EOF;
}

//==============================================================================
/**
 * @brief Function get character from stream.
 *
 * @param stream            source file
 *
 * @return Character or EOF.
 */
//==============================================================================
int getc(FILE *stream)
{
        if (!stream) {
                return EOF;
        }

        bool locked = lock_stream(stream);
        int c = getc_unlocked(stream);
        unlock_stream(stream, locked);

        return c;
}

//==============================================================================
/**
 * @brief Function put character into stream without locking.
 *
 * @param c             character
 * @param stream        file
 *
 * @return c if OK otherwise EOF
 */
//==============================================================================
int putc_unlocked(int c, FILE *stream)
{
        if (!stream) {
                return EOF;
        }

        unsigned char ch = (unsigned char)c;
        if (stream_write(stream, &ch, 1) == 1) {
                return ch;
        }

        return EOF;
}

//==============================================================================
/**
 * @brief Function put character into stream.
 *
 * @param c             character
 * @param stream        file
 *
 * @return c if OK otherwise EOF
 */
//==============================================================================
int fputc(int c, FILE *stream)
{
        /* stream is checked by fwrite() (fputc() is a builtin with nonnull argument) */
        unsigned char ch = (unsigned char)c;
        if (_libc_fwrite(&ch, 1, 1, stream) == 1) {
                return ch;
        }

        return EOF;
}

//==============================================================================
/**
 * @brief  Function locks stream for the calling thread (recursive).
 *
 * @param  file         stream
 */
//==============================================================================
void flockfile(FILE *file)
{
        if (file) {
                int tid = thread_current();

                if ((file->owner != NO_OWNER) && (file->owner == tid)) {
                        file->lockcnt++;
                } else {
                        futex_lock(&file->lock, WAIT_FOREVER_MS);
                        file->owner = tid;
                        file->lockcnt = 1;
                }
        }
}

//==============================================================================
/**
 * @brief  Function tries to lock stream for the calling thread.
 *
 * @param  file         stream
 *
 * @return 0 if stream was locked, otherwise nonzero.
 */
//==============================================================================
int ftrylockfile(FILE *file)
{
        if (file) {
                int tid = thread_current();

                if ((file->owner != NO_OWNER) && (file->owner == tid)) {
                        file->lockcnt++;
                        return 0;
                }

                if (!_libc_atomic_flag_test_and_set(&file->lock)) {
                        file->owner = tid;
                        file->lockcnt = 1;
                        return 0;
                }
        }

        return -1;
}

//==============================================================================
/**
 * @brief  Function unlocks stream locked by flockfile() or ftrylockfile().
 *
 * @param  file         stream
 */
//==============================================================================
void funlockfile(FILE *file)
{
        if (file && (file->lockcnt > 0)) {
                if (--file->lockcnt == 0) {
                        file->owner = NO_OWNER;
                        futex_unlock(&file->lock);
                }
        }
}

//==============================================================================
/**
 * @brief  Function checks if stream is buffered (used by fgets()).
 *
 * @param  file         stream
 *
 * @return true if stream uses buffer.
 */
//==============================================================================
bool _libc_stream_is_buffered(FILE *file)
{
        bool locked = lock_stream(file);
        bool buffered = stream_buffer(file);
        unlock_stream(file, locked);

        return buffered;
}

/*==============================================================================
  End of file
==============================================================================*/
