/*==============================================================================
@file    upload.c

@author  Daniel Zorychta

@brief   upload page.

@note    Copyright (C) 2024 Daniel Zorychta <daniel.zorychta@gmail.com>

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


==============================================================================*/

// #pragma GCC optimize("O1")

/*==============================================================================
  Include files
==============================================================================*/
#include "../httpd.h"
#include <sys/ioctl.h>

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

        socket_write(self->http_request.socket, HTTPD_COMMON_HTML_BEGIN,
                strlen(HTTPD_COMMON_HTML_BEGIN));

        httpd_sockprintf(self,
                "<meta http-equiv=\"refresh\" content=\"%lu; URL=ustawienia\">", 20);

        httpd_sockprintf(self, "<h2>Update</h2>");
        httpd_sockprintf(self, "<table>");
        httpd_sockprintf(self,
                "<form method=\"POST\" action=\"updating\" "
                "enctype=\"multipart/form-data\">");

        httpd_sockprintf(self, "<b>Select update file </b>");
        httpd_sockprintf(self,
                "<input type=\"file\" name=\"firmware\" accept=\".fpkg, .gz\">");

        httpd_sockprintf(self,
                "<input type=\"submit\" value=\"Update\" name=\"upload\">");

        httpd_sockprintf(self, "</form>");
        httpd_sockprintf(self, "</table>");

        socket_write(self->http_request.socket, HTTPD_COMMON_HTML_END,
                strlen(HTTPD_COMMON_HTML_END));

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
