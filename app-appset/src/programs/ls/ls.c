/*==============================================================================
@file   ls.c

@author Daniel Zorychta

@brief  List files in specified directory

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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <dnx/vt100.h>
#include <dnx/os.h>
#include <dnx/misc.h>
#include <sys/stat.h>
#include <sys/mount.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define CWD_MAX_LEN          128

#define KiB                  (u32_t)(1024)
#define MiB                  (u32_t)(1024 * 1024)
#define GiB                  (u64_t)(1024 * 1024 * 1024)
#define CONVERT_TO_KiB(_val) (_val >> 10)
#define CONVERT_TO_MiB(_val) (_val >> 20)
#define CONVERT_TO_GiB(_val) (_val >> 30)

#define CURSOR_AT(_col)      VT100_CURSOR_BACKWARD(999) VT100_CURSOR_FORWARD(_col)

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
        char cwd[CWD_MAX_LEN];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(ls, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Cat main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        getcwd(global->cwd, CWD_MAX_LEN - 1);

        bool human_readable = false;

        for (int i = 0; i < argc; i++) {

                if ((i == 0) and (argc >= 2)) {
                        continue;
                }

                if (isstreq(argv[i], "-h")) {
                        human_readable = true;

                        if (argc > 2) {
                                continue;
                        } else {
                                argv[i] = "./";
                        }
                }

                if (argc >= 2) {
                        if (argc >= 3) {
                                printf("%s:\n", argv[i]);
                        }

                        char *path = argv[i];

                        if (path[0] == '/') {
                                strcpy(global->cwd, path);

                        } else {
                                if (LAST_CHARACTER(global->cwd) != '/') {
                                        strcat(global->cwd, "/");
                                }

                                if (strncmp(path, "./", 2) == 0) {
                                        path += 2;
                                }

                                strcat(global->cwd, path);
                        }

                        chdir(global->cwd);
                }

                DIR *dir = opendir(""); // path determined by CWD
                if (dir) {
                        errno = 0;

                        u16_t count = 0;

                        dirent_t *dirent;
                        while (!errno && (dirent = readdir(dir))) {

                                if (isstreq(dirent->d_name, ".") || isstreq(dirent->d_name, "..")) {
                                        continue;
                                }

                                struct stat st;
                                if (stat(dirent->d_name, &st) == 0) {

                                        const char *type;
                                        switch (st.st_mode & S_IFMT) {
                                        case S_IFDIR:
                                                type = VT100_FONT_COLOR_LIGHT_BLUE "d";
                                                break;
                                        case S_IFDEV:
                                                type = VT100_FONT_COLOR_MAGENTA "c";
                                                break;
                                        case S_IFLNK:
                                                type = VT100_FONT_COLOR_CYAN "l";
                                                break;
                                        case S_IFREG:
                                                type = VT100_FONT_COLOR_GREEN "-";
                                                break;
                                        case S_IFPROG:
                                                type = VT100_FONT_BOLD "*";
                                                break;
                                        case S_IFIFO:
                                                type = VT100_FONT_COLOR_BROWN "p";
                                                break;
                                        default:
                                                type = "?";
                                                break;
                                        }

                                        char mode[10];
                                        mode[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
                                        mode[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
                                        mode[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
                                        mode[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
                                        mode[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
                                        mode[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
                                        mode[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
                                        mode[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
                                        mode[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
                                        mode[9] = '\0';

                                        u64_t size = st.st_size;
                                        const char *unit = "B";

                                        if (human_readable) {
                                                if (st.st_size >= (u64_t)(1 * GiB)) {
                                                        size = CONVERT_TO_MiB(st.st_size);
                                                        unit = "MiB";
                                                } else if (st.st_size >= 1 * MiB) {
                                                        size = CONVERT_TO_KiB(st.st_size);
                                                        unit = "KiB";
                                                } else {
                                                        size = st.st_size;
                                                        unit = "B";
                                                }
                                        }

                                        int mod_id = get_driver_ID2(st.st_dev);
                                        int mod_major = get_driver_major(st.st_dev);
                                        int mod_minor = get_driver_minor(st.st_dev);

                                        char mod[12];
                                        memset(mod, 0, sizeof(mod));

                                        if (S_ISDEV(st.st_mode)) {
                                                snprintf(mod, sizeof(mod), "%2d,%2d,%2d", mod_id, mod_major,
                                                        mod_minor);
                                        }

                                        struct tm tm;
                                        localtime_r(&st.st_mtime, &tm);

                                        char time[24];
                                        strftime(time, sizeof(time), "%d-%m-%Y %H:%M", &tm);

                                        printf("%s%s %12llu %s%s%s%s%s%s%s" VT100_RESET_ATTRIBUTES "\n", type,
                                                mode, size, unit, CURSOR_AT(28), mod, CURSOR_AT(38), time,
                                                CURSOR_AT(55), dirent->d_name);

                                        count++;
                                } else {
                                        errno = EFAULT;
                                        break;
                                }
                        }

                        if (!(errno == ESUCC || errno == ENOENT)) {
                                perror("Read dir");
                        }

                        printf("total %d%c\n", count, (argc >= 3) ? '\n' : ' ');

                        closedir(dir);
                } else {
                        perror(global->cwd);
                }
        }

        return errno ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
