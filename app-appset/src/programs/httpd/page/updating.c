/*=========================================================================*//**
@file    upload.c

@author  Daniel Zorychta

@brief   Firmware updating page.

@note    Copyright (C) 2023 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


*//*==========================================================================*/

// #pragma GCC optimize("O1")

/*==============================================================================
  Include files
==============================================================================*/
#include "../httpd.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define PATH_UPDATE_FILE_PART  "/mmc/vendor/fw_app.bin.part"
#define PATH_UPDATE_FILE_FINAL "/mmc/vendor/fw_app.bin"

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int reboot_for_update(void *arg);

/*==============================================================================
  Local object definitions
==============================================================================*/
// clang-format off
static const char HTML[] =
"<!DOCTYPE html>"
"<html>"
        "<title>dnx RTOS Web Page</title>"
        "<head>"
                "<meta http-equiv=\"refresh\" content=\"20; URL=index.html\">"
        "</head>"
        "<body>"
                "<h1><center>Firmware Updater</center></h1>"
        "<center>"
                "Update file was successfully uploaded to the device. "
                "The device will start update automatically. Please wait..."
        "</center>"
        "</body>"
"</html>";
// clang-format on

/*==============================================================================
  Exported object definitions
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Callback called when data is parsed.
 *
 * @param  parser       http parser
 * @param  data         buffer positon
 * @param  size         data size
 *
 * @return On success 0.
 */
//==============================================================================
int httpd_multipart_on_data__updating(multipartparser *parser, const char *data,
        size_t size)
{
        DEBUG("%s: %lu bytes long\n", __func__, size);

        http_thread_t *self = parser->data;

        httpd_write_multipart_file(self, PATH_UPDATE_FILE_PART, data, size);

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called on part end.
 *
 * @param  parser       http parser
 *
 * @return On success 0.
 */
//==============================================================================
int httpd_multipart_on_part_end__updating(multipartparser *parser)
{
        http_thread_t *self = parser->data;

        httpd_flush_multipart_file_buffer(self, PATH_UPDATE_FILE_PART);
        remove(PATH_UPDATE_FILE_FINAL);
        rename(PATH_UPDATE_FILE_PART, PATH_UPDATE_FILE_FINAL);

        thread_create(reboot_for_update,
                &(const thread_attr_t) {
                        .priority    = PRIORITY_NORMAL,
                        .stack_depth = STACK_DEPTH_LOW,
                        .detached    = true,
                },
                NULL);

        return 0;
}

//==============================================================================
/**
 * @brief  Callback called when entire message of HTTP request is parsed.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
int httpd_on_message_complete__updating(http_parser *parser)
{
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        socket_write(self->http_request.socket, HTTPD_CONTENT_TYPE_OK_TEXT_HTML,
                strlen(HTTPD_CONTENT_TYPE_OK_TEXT_HTML));

        socket_write(self->http_request.socket, HTML, sizeof(HTML));

        return 0;
}

//==============================================================================
/**
 * @brief  Thread reboot system after 2 seconds.
 *
 * @param  arg          thread argument
 *
 * @return Thread exit status.
 */
//==============================================================================
static int reboot_for_update(void *arg)
{
        UNUSED_ARG1(arg);
        sleep(2);
        system_reboot();
        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
