/*==============================================================================
@file    dstd.c

@author  Daniel Zorychta

@brief   Day light saving time deamon.

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
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define TIMEZONE_SUMMER TIMEZONE_CEST
#define TIMEZONE_WINTER TIMEZONE_CET

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
        time_t current_time_unix;
        int    current_year;
        time_t beginning_of_summer_time;
        time_t end_of_summer_time;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(dstd, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  The function find the first time of summer time.
 */
//==============================================================================
static void calculate_beginning_of_daylight_saving_time(void)
{
        struct tm tm;
        tm.tm_hour = 1;
        tm.tm_min  = 0;
        tm.tm_sec  = 0;
        tm.tm_mday = 31;
        tm.tm_mon  = 2;
        tm.tm_year = global->current_year;

        time_t time = mktime(&tm);
        gmtime_r(&time, &tm);

        tm.tm_mday -= tm.tm_wday;

        global->beginning_of_summer_time = mktime(&tm);
}

//==============================================================================
/**
 * @brief  Function find the last time of summer time.
 */
//==============================================================================
static void calculate_end_of_daylight_saving_time(void)
{
        struct tm tm;
        tm.tm_hour = 0;
        tm.tm_min  = 59;
        tm.tm_sec  = 59;
        tm.tm_mday = 31;
        tm.tm_mon  = 9;
        tm.tm_year = global->current_year;

        time_t time = mktime(&tm);
        gmtime_r(&time, &tm);

        tm.tm_mday -= tm.tm_wday;

        global->end_of_summer_time = mktime(&tm);
}

//==============================================================================
/**
 * @brief  Function check that now is summer time or not.
 *
 * @return Return true if summer time is now, otherwise false.
 */
//==============================================================================
static bool is_summer_time(void)
{
        return (global->current_time_unix >= global->beginning_of_summer_time &&
                global->current_time_unix <= global->end_of_summer_time);
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc != 2) {
                printf("Usage: %s <interval-sec>\n", argv[0]);
                return EXIT_FAILURE;

        } else {

                int interval = 0;
                sscanf(argv[1], "%d", &interval);

                do {
                        global->current_time_unix = time(NULL);

                        struct tm tm;
                        gmtime_r(&global->current_time_unix, &tm);
                        global->current_year = tm.tm_year;

                        calculate_beginning_of_daylight_saving_time();
                        calculate_end_of_daylight_saving_time();

                        struct timezone tz;

                        if (is_summer_time()) {
                                tz.tz_minuteswest = TIMEZONE_SUMMER / 60;
                                tz.tz_dsttime     = 1;

                        } else {
                                tz.tz_minuteswest = TIMEZONE_WINTER / 60;
                                tz.tz_dsttime     = 1;
                        }

                        settimeofday(NULL, &tz);

                        sleep(interval);

                } while (interval);

                return EXIT_SUCCESS;
        }
}

/*==============================================================================
  End of file
==============================================================================*/
