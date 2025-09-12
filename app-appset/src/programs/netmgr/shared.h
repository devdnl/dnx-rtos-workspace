/*==============================================================================
File    shared.h

Author  Daniel Zorychta

Brief   Network Manager shared.

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
