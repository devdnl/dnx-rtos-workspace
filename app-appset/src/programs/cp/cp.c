/*==============================================================================
@file   cp.c

@author Daniel Zorychta

@brief  Program to copy files

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
#include <dirent.h>
#include <dnx/misc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <dnx/os.h>
#include <sys/stat.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define BUFFER_MAX_SIZE 32768
#define BUFFER_MIN_SIZE 512
#define MAX_DEPTH       32
#define PATH_LEN        256

#define VERBOSE(...)                         \
        do {                                 \
                if (global->verbose)         \
                        printf(__VA_ARGS__); \
        } while (0)

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void print_help(const char *name);
static bool copy_any(const char *src, const char *dst, int depth);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        size_t buffer_size;
        u8_t *buffer;
        bool recursive;
        bool force;
        bool verbose;
        char abs_src[PATH_LEN];
        char abs_dst[PATH_LEN];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(cp, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Print help message.
 *
 * @param  name         program name
 */
//==============================================================================
static void print_help(const char *name)
{
        printf("Usage: %s [-rRfv] <source> <destination>\n", name);
        printf("       %s [-rRfv] <source>... <directory>\n", name);
        printf(" -r, -R  copy directories recursively\n");
        printf(" -f      continue on errors\n");
        printf(" -v      verbose\n");
}

//==============================================================================
/**
 * @brief  Print error message.
 *
 * @param  msg          message
 * @param  path         file path
 *
 * @return Always false.
 */
//==============================================================================
static bool error(const char *msg, const char *path)
{
        if (errno) {
                printf("cp: %s '%s': %s\n", msg, path, strerror(errno));
        } else {
                printf("cp: %s '%s'\n", msg, path);
        }

        return false;
}

//==============================================================================
/**
 * @brief  Read file mode.
 *
 * @param  path         file path
 * @param  mode         file mode
 *
 * @return true if file exists.
 */
//==============================================================================
static bool file_mode(const char *path, mode_t *mode)
{
        struct stat st;
        if (stat(path, &st) != 0) {
                return false;
        }

        *mode = st.st_mode;
        return true;
}

//==============================================================================
/**
 * @brief  Check if path is a directory.
 *
 * @param  path         file path
 *
 * @return true if path is a directory.
 */
//==============================================================================
static bool is_dir(const char *path)
{
        mode_t mode;
        return file_mode(path, &mode) && S_ISDIR(mode);
}

//==============================================================================
/**
 * @brief  Create new path: directory + '/' + name.
 *
 * @param  dir          directory
 * @param  name         file name
 *
 * @return Allocated path or NULL on error.
 */
//==============================================================================
static char *path_join(const char *dir, const char *name)
{
        size_t dlen = strlen(dir);
        while ((dlen > 1) && (dir[dlen - 1] == '/')) {
                dlen--;
        }

        bool slash = !((dlen > 0) && (dir[dlen - 1] == '/'));
        size_t nlen = strlen(name);

        char *path = malloc(dlen + nlen + 2);
        if (path) {
                memcpy(path, dir, dlen);
                if (slash) {
                        path[dlen++] = '/';
                }
                memcpy(&path[dlen], name, nlen + 1);
        }

        return path;
}

//==============================================================================
/**
 * @brief  Return last path component (trailing slashes are skipped).
 *
 * @param  path         file path
 * @param  buf          buffer for name
 * @param  size         buffer size
 *
 * @return Name (can be empty for root).
 */
//==============================================================================
static const char *base_name(const char *path, char *buf, size_t size)
{
        size_t len = strlen(path);
        while ((len > 0) && (path[len - 1] == '/')) {
                len--;
        }

        size_t start = len;
        while ((start > 0) && (path[start - 1] != '/')) {
                start--;
        }

        len -= start;
        if (len >= size) {
                len = size - 1;
        }

        memcpy(buf, &path[start], len);
        buf[len] = '\0';

        return buf;
}

//==============================================================================
/**
 * @brief  Create absolute, normalized path ('.', '..' and repeated slashes
 *         are removed). Used to compare paths.
 *
 * @param  path         file path
 * @param  buf          buffer
 * @param  size         buffer size
 *
 * @return true on success.
 */
//==============================================================================
static bool abs_path(const char *path, char *buf, size_t size)
{
        char *tmp = malloc(size);
        if (!tmp) {
                return false;
        }

        tmp[0] = '\0';

        if (path[0] != '/') {
                if (!getcwd(tmp, size)) {
                        free(tmp);
                        return false;
                }
                strlcat(tmp, "/", size);
        }

        bool ok = strlcat(tmp, path, size) < size;

        size_t n = 0;
        buf[n++] = '/';

        const char *p = tmp;
        while (ok && *p) {
                while (*p == '/') {
                        p++;
                }

                const char *seg = p;
                while (*p && (*p != '/')) {
                        p++;
                }

                size_t len = p - seg;

                if ((len == 0) || ((len == 1) && (seg[0] == '.'))) {
                        continue;

                } else if ((len == 2) && (seg[0] == '.') && (seg[1] == '.')) {
                        while ((n > 1) && (buf[n - 1] != '/')) {
                                n--;
                        }
                        if (n > 1) {
                                n--;
                        }

                } else if (n + len + 2 < size) {
                        if (n > 1) {
                                buf[n++] = '/';
                        }
                        memcpy(&buf[n], seg, len);
                        n += len;

                } else {
                        ok = false;
                }
        }

        buf[n] = '\0';

        free(tmp);

        return ok;
}

//==============================================================================
/**
 * @brief  Read directory entry names. Directory is read before copy, so
 *         created files do not change iteration.
 *
 * @param  path         directory path
 *
 * @return NULL terminated array of names or NULL on error.
 */
//==============================================================================
static char **read_names(const char *path)
{
        DIR *dir = opendir(path);
        if (!dir) {
                return NULL;
        }

        size_t cap = 8;
        size_t cnt = 0;
        char **names = calloc(cap + 1, sizeof(char *));

        dirent_t *dirent = NULL;
        while (names && (dirent = readdir(dir))) {
                if (isstreq(dirent->d_name, ".") || isstreq(dirent->d_name, "..")) {
                        continue;
                }

                if (cnt == cap) {
                        char **n = realloc(names, (cap * 2 + 1) * sizeof(char *));
                        if (!n) {
                                break;
                        }
                        names = n;
                        cap *= 2;
                }

                size_t len = strlen(dirent->d_name);
                names[cnt] = malloc(len + 1);
                if (!names[cnt]) {
                        break;
                }
                memcpy(names[cnt++], dirent->d_name, len + 1);
                names[cnt] = NULL;
        }

        closedir(dir);

        // not complete list is an error
        if (names && dirent) {
                for (size_t i = 0; i < cnt; i++) {
                        free(names[i]);
                }
                free(names);
                names = NULL;
                errno = ENOMEM;
        }

        return names;
}

//==============================================================================
/**
 * @brief  Free names array.
 *
 * @param  names        names
 */
//==============================================================================
static void free_names(char **names)
{
        for (size_t i = 0; names[i]; i++) {
                free(names[i]);
        }

        free(names);
}

//==============================================================================
/**
 * @brief  Copy selected file.
 *
 * @param  src          source path
 * @param  dst          destination path
 * @param  mode         source file mode
 *
 * @return true on success.
 */
//==============================================================================
static bool copy_file(const char *src, const char *dst, mode_t mode)
{
        VERBOSE("'%s' -> '%s'\n", src, dst);

        errno = 0;
        FILE *in = fopen(src, "r");
        if (!in) {
                return error("cannot open", src);
        }

        errno = 0;
        FILE *out = fopen(dst, "w");
        if (!out) {
                fclose(in);
                return error("cannot create", dst);
        }

        bool ok = true;

        for (;;) {
                errno = 0;
                size_t n = fread(global->buffer, 1, global->buffer_size, in);
                if (n == 0) {
                        if (ferror(in)) {
                                ok = error("cannot read", src);
                        }
                        break;
                }

                errno = 0;
                if (fwrite(global->buffer, 1, n, out) != n) {
                        ok = error("cannot write", dst);
                        break;
                }
        }

        fclose(in);

        errno = 0;
        if ((fclose(out) != 0) && ok) {
                ok = error("cannot write", dst);
        }

        if (ok) {
                chmod(dst, mode & 0777);
        }

        return ok;
}

//==============================================================================
/**
 * @brief  Copy directory.
 *
 * @param  src          source directory
 * @param  dst          destination directory
 * @param  mode         source directory mode
 * @param  depth        recursion depth
 *
 * @return true on success.
 */
//==============================================================================
static bool copy_dir(const char *src, const char *dst, mode_t mode, int depth)
{
        errno = 0;

        if (depth > MAX_DEPTH) {
                return error("directory too deep", src);
        }

        // directory cannot be copied into itself
        if (abs_path(src, global->abs_src, PATH_LEN) && abs_path(dst, global->abs_dst, PATH_LEN)) {
                size_t len = strlen(global->abs_src);
                if ((strncmp(global->abs_src, global->abs_dst, len) == 0) &&
                        ((global->abs_dst[len] == '/') || (global->abs_dst[len] == '\0') || (len == 1))) {
                        return error("cannot copy directory into itself", src);
                }
        }

        mode_t dst_mode;
        if (file_mode(dst, &dst_mode)) {
                if (!S_ISDIR(dst_mode)) {
                        return error("cannot overwrite non-directory with directory", dst);
                }
        } else {
                errno = 0;
                if (mkdir(dst, (mode & 0777) | 0700) != 0) {
                        return error("cannot create directory", dst);
                }

                VERBOSE("'%s' -> '%s' (directory)\n", src, dst);
        }

        errno = 0;
        char **names = read_names(src);
        if (!names) {
                return error("cannot read directory", src);
        }

        bool ok = true;

        for (size_t i = 0; names[i] && (ok || global->force); i++) {
                char *s = path_join(src, names[i]);
                char *d = path_join(dst, names[i]);

                if (s && d) {
                        ok = copy_any(s, d, depth + 1) && ok;
                } else {
                        errno = ENOMEM;
                        ok = error("cannot copy", names[i]);
                }

                free(s);
                free(d);
        }

        free_names(names);

        return ok;
}

//==============================================================================
/**
 * @brief  Copy file or directory.
 *
 * @param  src          source path
 * @param  dst          destination path
 * @param  depth        recursion depth
 *
 * @return true on success.
 */
//==============================================================================
static bool copy_any(const char *src, const char *dst, int depth)
{
        mode_t mode;

        errno = 0;
        if (!file_mode(src, &mode)) {
                return error("cannot stat", src);
        }

        if (S_ISDIR(mode)) {
                if (!global->recursive) {
                        errno = 0;
                        return error("-r not specified; omitting directory", src);
                }

                return copy_dir(src, dst, mode, depth);
        }

        if (is_dir(dst)) {
                errno = EISDIR;
                return error("cannot overwrite directory", dst);
        }

        // the same file
        if (abs_path(src, global->abs_src, PATH_LEN) && abs_path(dst, global->abs_dst, PATH_LEN) &&
                isstreq(global->abs_src, global->abs_dst)) {
                errno = 0;
                return error("source and destination are the same file", src);
        }

        return copy_file(src, dst, mode);
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        int first = 1;

        // options
        for (; (first < argc) && (argv[first][0] == '-') && argv[first][1]; first++) {
                if (isstreq(argv[first], "--")) {
                        first++;
                        break;
                }

                if (isstreq(argv[first], "--help")) {
                        print_help(argv[0]);
                        return EXIT_SUCCESS;
                }

                for (const char *o = &argv[first][1]; *o; o++) {
                        switch (*o) {
                        case 'r':
                        case 'R':
                                global->recursive = true;
                                break;
                        case 'f':
                                global->force = true;
                                break;
                        case 'v':
                                global->verbose = true;
                                break;
                        default:
                                printf("cp: invalid option -- '%c'\n", *o);
                                print_help(argv[0]);
                                return EXIT_FAILURE;
                        }
                }
        }

        int nsrc = argc - first - 1;
        if (nsrc < 1) {
                print_help(argv[0]);
                return EXIT_FAILURE;
        }

        const char *dst = argv[argc - 1];
        bool dst_dir = is_dir(dst);

        if ((nsrc > 1) && !dst_dir) {
                printf("cp: target '%s' is not a directory\n", dst);
                return EXIT_FAILURE;
        }

        // operation buffer
        global->buffer_size = BUFFER_MAX_SIZE;
        while ((global->buffer = malloc(global->buffer_size)) == NULL) {
                global->buffer_size /= 2;

                if (global->buffer_size < BUFFER_MIN_SIZE) {
                        perror(NULL);
                        return EXIT_FAILURE;
                }
        }

        bool ok = true;

        for (int i = first; (i < argc - 1) && (ok || global->force); i++) {
                const char *src = argv[i];

                // file copied to directory keeps own name
                if (dst_dir) {
                        char name[PATH_LEN];
                        base_name(src, name, sizeof(name));

                        char *target = path_join(dst, name);
                        if (target) {
                                ok = copy_any(src, target, 0) && ok;
                                free(target);
                        } else {
                                errno = ENOMEM;
                                ok = error("cannot copy", src);
                        }
                } else {
                        ok = copy_any(src, dst, 0) && ok;
                }
        }

        free(global->buffer);

        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*==============================================================================
  End of file
==============================================================================*/
