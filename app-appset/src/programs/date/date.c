/*==============================================================================
@file   date.c

@autho  Daniel Zorychta

@brief  Program set and read date (time)

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
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define TIME_FORMAT_BUF_LEN 100

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
        char buf[TIME_FORMAT_BUF_LEN];
};

static const char *wrong_time_fm = "Wrong time format!";

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(date, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        bool show_date = true;

        time_t t = time(NULL);

        struct tm tm;
        localtime_r(&t, &tm);

        for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-u") == 0) {
                        gmtime_r(&t, &tm);
                        continue;
                }

                if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                        printf("%s <options|+format>\n", argv[0]);
                        puts("Options:");
                        puts("  --help                      this help");
                        puts("  --set=DD/MM/YYYY,HH:MM:SS   set date and time (UTC)");
                        puts("  --set-tz=TIMEOFFSET         set timezone offset e.g. +0400, -0100\n");
                        puts("  -u                          UTC time");
                        puts("Formats start with % character (e.g. date \"+Time: %X\"):");
                        puts("  %nHIJMSAaBbhCYydpjmXxFDzs");
                        show_date = false;
                        break;
                }

                if (strncmp("--set=", argv[i], 6) == 0) {
                        int day, month, year, hour, minute, second;
                        day = month = year = hour = minute = second = 0xFFFF;

                        sscanf(&argv[i][6], "%d/%d/%d,%d:%d:%d", &day, &month, &year, &hour, &minute,
                                &second);

                        if (day > 31 || month > 12 || month < 1 || year < 1970 || hour > 23 || minute > 59 ||
                                second > 59) {

                                puts(wrong_time_fm);
                                return EXIT_FAILURE;
                        }

                        struct tm t;
                        t.tm_hour = hour;
                        t.tm_min = minute;
                        t.tm_sec = second;
                        t.tm_mday = day;
                        t.tm_mon = month - 1;
                        t.tm_year = year - 1900;

                        errno = 0;
                        struct timeval tv;
                        tv.tv_sec = mktime(&t);
                        tv.tv_usec = 0;
                        if (settimeofday(&tv, NULL) != 0) {
                                perror(argv[0]);
                                return EXIT_FAILURE;
                        }

                        show_date = false;
                        break;
                }

                if (strncmp("--set-tz=", argv[i], 9) == 0) {
                        if (strlen(&argv[i][9]) != 5) {
                                puts(wrong_time_fm);
                                return EXIT_FAILURE;
                        }

                        uint sign, hour, minute;
                        sign = hour = minute = 0xFF;

                        sscanf(&argv[i][9], "%c%2u%2u", &sign, &hour, &minute);

                        if (!(sign == '-' || sign == '+') || hour > 23 || minute > 59) {
                                puts(wrong_time_fm);
                                return EXIT_FAILURE;
                        }

                        int diff = cast(int, (hour * 3600 + minute * 60)) * (sign == '-' ? -1 : 1);

                        errno = 0;
                        struct timezone tz;
                        tz.tz_minuteswest = (diff / 60);
                        tz.tz_dsttime = 0;
                        if (settimeofday(NULL, &tz) != 0) {
                                perror(argv[0]);
                                return EXIT_FAILURE;
                        }

                        show_date = false;
                        break;
                }

                if (argv[i][0] == '+') {
                        char *fmt = &argv[i][1];
                        char *out_buf = global->buf;
                        int out_len = sizeof(global->buf);

                        while ((*fmt != '\0') and (out_len > 0)) {
                                if (*fmt == '%') {
                                        fmt++;
                                        char specifier = *fmt++;
                                        size_t n = 0;

                                        switch (specifier) {
                                        case '\0':
                                                break;

                                        case 's':
                                                n = snprintf(out_buf, out_len, "%lli", t);
                                                out_buf += n;
                                                out_len -= n;
                                                break;

                                        default:
                                                n = strftime(out_buf, out_len,
                                                        (char[3]) {'%', specifier, '\0'}, &tm);
                                                out_buf += n;
                                                out_len -= n;
                                        }
                                } else {
                                        *out_buf++ = *fmt++;
                                        out_len--;
                                }
                        }

                        puts(global->buf);

                        show_date = false;
                        break;
                }
        }

        if (show_date) {
                strftime(global->buf, sizeof(global->buf), "%a %B %d %X %Y %z\n", &tm);
                printf(global->buf);
        }

        return EXIT_SUCCESS;
}

/*==============================================================================
  End of file
==============================================================================*/
