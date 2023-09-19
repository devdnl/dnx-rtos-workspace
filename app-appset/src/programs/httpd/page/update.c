/*=========================================================================*//**
@file    upload.c

@author  Daniel Zorychta

@brief   upload page.

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

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
// clang-format off
static const char HTML[] =
"<!DOCTYPE html>"
"<html>"
        "<title>dnx RTOS Web Page</title>"
        "<body>"
        "<h1><center>Firmware Updater</center></h1>"
        "<center>"
                "<form method=\"POST\" action=\"/updating\" enctype=\"multipart/form-data\">"
                "Select file to upload:"
                "<input type=\"file\" name=\"firmware\" accept=\".bin, .gz\">"
                "<input type=\"submit\" value=\"Upload\" name=\"upload\">"
                "</form>"
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
 * @brief  Callback called when entire message of HTTP request is parsed.
 *
 * @param  parser       http parser
 * @param  at           buffer positon
 * @param  length       data length
 *
 * @return On success 0.
 */
//==============================================================================
int httpd_on_message_complete__update(http_parser *parser)
{
        http_thread_t *self = parser->data;

        DEBUG("%s\n", __func__);

        socket_write(self->http_request.socket, HTTPD_CONTENT_TYPE_OK_TEXT_HTML,
                strlen(HTTPD_CONTENT_TYPE_OK_TEXT_HTML));

        socket_write(self->http_request.socket, HTML, sizeof(HTML));

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
