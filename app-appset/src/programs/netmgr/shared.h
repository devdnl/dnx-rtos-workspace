/*==============================================================================
File     shared.h

Author   Daniel Zorychta

Brief    Network Manager shared.

         Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the  Free Software  Foundation;  either version 2 of the License, or
         any later version.

         This  program  is  distributed  in the hope that  it will be useful,
         but  WITHOUT  ANY  WARRANTY;  without  even  the implied warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         You  should  have received a copy  of the GNU General Public License
         along  with  this  program;  if not,  write  to  the  Free  Software
         Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#define NETMGR_SHM                                         "netmgr"

#define NETMGR_CONF_KEY__INTERFACE_PATH                    "interface.path"
#define NETMGR_CONF_KEY__INTERFACE_HWADDR                  "interface.hwaddr"
#define NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION         "interface.autonegotiation"
#define NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION_ENABLE  "enable"
#define NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION_DISABLE "disable"
#define NETMGR_CONF_KEY__INTERFACE_SPEED                   "interface.speed"
#define NETMGR_CONF_KEY__INTERFACE_SPEED_10M               "10M"
#define NETMGR_CONF_KEY__INTERFACE_SPEED_100M              "100M"
#define NETMGR_CONF_KEY__INTERFACE_DUPLEX                  "interface.duplex"
#define NETMGR_CONF_KEY__INTERFACE_DUPLEX_HALF             "half"
#define NETMGR_CONF_KEY__INTERFACE_DUPLEX_FULL             "full"
#define NETMGR_CONF_KEY__IP_MODE                           "IP.mode"
#define NETMGR_CONF_KEY__IP_MODE_STATIC                    "static"
#define NETMGR_CONF_KEY__IP_MODE_DHCP                      "DHCP"
#define NETMGR_CONF_KEY__IP_ADDRESS                        "IP.address"
#define NETMGR_CONF_KEY__IP_NETMASK                        "IP.netmask"
#define NETMGR_CONF_KEY__IP_GATEWAY                        "IP.gateway"

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct {
        bool        ready;
        bool        reload_conf;
        const char *conf_path;
} netmgr_shared_t;

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

/*==============================================================================
  End of file
==============================================================================*/
