/*==============================================================================
@file   fgets.h

@author Daniel Zorychta

@brief  String get functions.

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
#include <string.h>
#include <sys/stat.h>
#include <dnx/misc.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define EOF                                     (-1)
#define ETX                                     0x03
#define EOT                                     0x04

/*==============================================================================
  Local object types
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief Function receive string from selected file using buffer 'buf' of size
 *        'buflen'.
 *
 * @param[out] *str          buffer with string
 * @param[in]   size         buffer size
 * @param[in]  *stream       source stream
 * @param[in]  *buf          buffer
 * @param[in]   buflen       buffer length
 *
 * @retval NULL if error, otherwise pointer to str.
 */
//==============================================================================
char *fgets_buffered(char *str, int size, FILE *stream, char *buf, size_t buflen)
{
        if (!str || size < 1 || !stream || !buf || !buflen) {
                return NULL;
        }

        struct stat file_stat;
        if (fstat(fileno(stream), &file_stat) == 0) {

                char *p = str;
                int   c = EOF;

                size--;

                if (S_ISFIFO(file_stat.st_mode) || S_ISDEV(file_stat.st_mode)) {

                        while ((c != '\n') && size--) {

                                c = fgetc(stream);
                                if (c == EOF) {
                                        break;
                                } else if (c == ETX) {
                                        str = NULL;
                                        break;
                                } else {
                                        *p++ = c;
                                }
                        }

                } else {
                        int n = 0;

                        i64_t fpos = ftell(stream);

                        fseek(stream, 0, SEEK_END);
                        i64_t end = ftell(stream);

                        fseek(stream, fpos, SEEK_SET);

                        while ((c != '\n') && (size > 0) && !(ferror(stream) || feof(stream))) {

                                n = fread(buf, 1, buflen, stream);

                                for (int i = 0; c != '\n' && size > 0 && i < n; size--, i++) {
                                        c    = buf[i];
                                        *p++ = c;
                                        fpos++;
                                }
                        }

                        if ((c == '\n' || size == 0) && fpos < end) {
                                fseek(stream, fpos, SEEK_SET);
                        }
                }

                *p = '\0';

                if (ferror(stream)) {
                        str = NULL;

                } else if (feof(stream)) {
                        str = (p == str) ? NULL : str;
                }

                return str;
        }

        return NULL;
}

//==============================================================================
/**
 * @brief Function receive string from selected file.
 *
 * @param[out] *str          buffer with string
 * @param[in]   size         buffer size
 * @param[in]  *stream       source stream
 *
 * @retval NULL if error, otherwise pointer to str
 */
//==============================================================================
char *fgets(char *str, int size, FILE *stream)
{
        char chunk[16];
        return fgets_buffered(str, size, stream, chunk, sizeof(chunk));
}

/*==============================================================================
  End of file
==============================================================================*/
