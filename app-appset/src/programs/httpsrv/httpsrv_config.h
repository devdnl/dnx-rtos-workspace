/*==============================================================================
@file    httpsrv_config.h

@author  Daniel Zorychta

@brief   The HTTP server with script support.

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

/**
@defgroup HTTPSRV_CONFIG_H_ HTTPSRV_CONFIG_H_

Detailed Doxygen description.
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#define HTTPSRV_DEBUG_ON                      0
#define HTTPSRV_MULTIPART_BUFFER_SIZE         (256 * 1024)
#define HTTPSRV_INTERPRETER_FETCH_BUFFER_SIZE 8192
#define HTTPSRV_INTERPRETER_CODE_BUFFER_SIZE  8192
#define HTTPSRV_INTERPRETER_TMP_BUFFER_SIZE   1024
#define HTTPSRV_OUTPUT_BUFFER_SIZE            (4*1500)
#define HTTPSRV_HTTP_PARSER_BUFFER_SIZE       2048
#define HTTPSRV_ALLOC_SIZE                    1024
#define HTTPSRV_PORT                          80
#define HTTPSRV_CACHE_TIME_FOR_JPEG           300
#define HTTPSRV_CACHE_TIME_FOR_PNG            300
#define HTTPSRV_CACHE_TIME_FOR_ICO            300
#define HTTPSRV_CACHE_TIME_FOR_CSS            300
#define HTTPSRV_DEFAULT_PAGE                  "index.html"
#define HTTPSRV_PAGE_NOT_FOUND                "404.html"
#define HTTPSRV_ROOT_DIR                      "/mmc/srv"
#define HTTPSRV_UPLOAD_DIR                    "/mmc/srv/upload"
#define HTTPSRV_SCRIPT_BEGIN_TOKEN            "<?lua"
#define HTTPSRV_SCRIPT_EXTENSION              ".lua"

/*==============================================================================
  Exported object types
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
