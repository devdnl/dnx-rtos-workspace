/*==============================================================================
File     netmgr.h

Author   Daniel Zorychta

Brief    Network Manager.

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

/*==============================================================================
  Include files
==============================================================================*/
#include <dnx/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <conf_parser.h>
#include <sys/shm.h>
#include <dnx/os.h>
#include <time.h>
#include "shared.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define debug(...) printf("NM: "__VA_ARGS__);

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void handle_interface(const char *config_file);
static int  configure_ethernet(void);
static void ifup_by_config(void);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        conf_parser_t   *conf;
        netmgr_shared_t *shared;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(netmgr, STACK_DEPTH_LOW);

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
        if (argc < 2) {
                printf("Usage: %s <option> <config-file>\n", argv[0]);
                puts("  --daemon        daemon mode");
                puts("  --conf-reload   reload configuration");
                return EXIT_FAILURE;
        }

        char *option    = argv[1];
        char *conf_file = argv[2];

        if (isstreq(option, "--daemon")) {

                if (not conf_file) {
                        debug("not config file path!\n", option);
                        return EXIT_FAILURE;
                }

                void  *mem;
                size_t mem_size;
                if (shmat(NETMGR_SHM, &mem, &mem_size) == 0) {
                        debug("daemon is already running!\n");
                        shmdt(NETMGR_SHM);
                        return EXIT_FAILURE;
                }

                if (shmget(NETMGR_SHM, sizeof(netmgr_shared_t)) != 0) {
                        perror(argv[0]);
                        return EXIT_FAILURE;
                }

                if (shmat(NETMGR_SHM, cast(void **, &global->shared), &mem_size) != 0) {
                        perror(argv[0]);
                        shmrm(NETMGR_SHM);
                        return EXIT_FAILURE;
                }

                global->shared->reload_conf = true;

                handle_interface(conf_file);

                shmdt(NETMGR_SHM);
                shmrm(NETMGR_SHM);

        } else if (isstreq(option, "--conf-reload")) {

                size_t mem_size;
                if (shmat(NETMGR_SHM, cast(void **, &global->shared), &mem_size) != 0) {
                        perror(argv[0]);
                        debug("deamon is not running.\n");
                        return EXIT_FAILURE;
                }

                global->shared->reload_conf = true;

                shmdt(NETMGR_SHM);

        } else {
                debug("Unkown option: '%s'\n", option);
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

//==============================================================================
/**
 * @brief  Handle network interface.
 *
 * @param  config_file  configuration file
 */
//==============================================================================
static void handle_interface(const char *config_file)
{
        global->conf = conf_parser__new(config_file);
        if (!global->conf) {
                global->conf = conf_parser__new(NULL);
                if (!global->conf) {
                        debug("unable to create configuration file!");
                        return;
                }

                srand(get_uptime_ms());
                srand(time(NULL));
                char hwaddr[32];
                u8_t n = 0;
                for (size_t i = 0; i < 6; i++) {
                        n += snprintf(hwaddr + n, sizeof(hwaddr) - n,
                                "%02X:", rand() & 0xFF);
                }
                hwaddr[n - 1] = '\0';

                conf_parser__set_value(global->conf, "interface.path", "/dev/eth");
                conf_parser__set_value(global->conf, "interface.hwaddr", hwaddr);
                conf_parser__set_value(global->conf, "interface.autonegotiation", "true");
                conf_parser__set_value(global->conf, "interface.speed", "100M");
                conf_parser__set_value(global->conf, "interface.duplex", "full");
                conf_parser__set_value(global->conf, "IP.mode", "DHCP");
                conf_parser__set_value(global->conf, "IP.address", "0.0.0.0");
                conf_parser__set_value(global->conf, "IP.netmask", "0.0.0.0");
                conf_parser__set_value(global->conf, "IP.gateway", "0.0.0.0");

                conf_parser__set_file_path(global->conf, config_file);
                conf_parser__save(global->conf);
        }

        if (configure_ethernet() != 0) {
                return;
        }

        NET_INET_state_t last_state = NET_INET_STATE__LINK_DISCONNECTED;

        while (true) {

                if (global->shared->reload_conf) {
                        ifdown("inet");
                        if (conf_parser__reload(global->conf) > 0) {
                                global->shared->reload_conf = false;
                        } else {
                                sleep(5);
                                continue;
                        }
                }

                NET_INET_status_t status;
                NET_family_t      family;
                if (ifstatus("inet", &family, &status, sizeof(status)) == 0) {

                        switch (status.state) {
                        case NET_INET_STATE__NOT_CONFIGURED:
                                ifup_by_config();
                                break;

                        case NET_INET_STATE__LINK_DISCONNECTED:
                                if (last_state != status.state) {
                                        debug("link disconnected\n");
                                }
                                ifdown("inet");
                                break;

                        case NET_INET_STATE__DHCP_CONFIGURING:
                                break;

                        case NET_INET_STATE__STATIC_IP:
                        case NET_INET_STATE__DHCP_CONFIGURED:
                                if (last_state != status.state) {
                                        debug("estabilished IP connection.\n");
                                        debug("Address: %u.%u.%u.%u\n",
                                                NET_INET_IPv4_a(status.address),
                                                NET_INET_IPv4_b(status.address),
                                                NET_INET_IPv4_c(status.address),
                                                NET_INET_IPv4_d(status.address));
                                        debug("Mask   : %u.%u.%u.%u\n",
                                                NET_INET_IPv4_a(status.mask),
                                                NET_INET_IPv4_b(status.mask),
                                                NET_INET_IPv4_c(status.mask),
                                                NET_INET_IPv4_d(status.mask));
                                        debug("Gateway: %u.%u.%u.%u\n",
                                                NET_INET_IPv4_a(status.gateway),
                                                NET_INET_IPv4_b(status.gateway),
                                                NET_INET_IPv4_c(status.gateway),
                                                NET_INET_IPv4_d(status.gateway));
                                }
                                break;

                        default:
                                ifdown("inet");
                                break;
                        }

                        last_state = status.state;
                } else {
                        ifdown("inet");
                        ifup_by_config();
                }

                sleep(2);
        }
}

//==============================================================================
/**
 * @brief  Configure ethernet.
 */
//==============================================================================
static int configure_ethernet(void)
{
        const char *conf_mac = conf_parser__get_value(global->conf, "interface.hwaddr");

        const char *conf_autoneg = conf_parser__get_value(global->conf,
                "interface.autonegotiation");

        const char *conf_speed = conf_parser__get_value(global->conf, "interface.speed");

        const char *conf_duplex = conf_parser__get_value(global->conf,
                "interface.duplex");

        const char *conf_if_path = conf_parser__get_value(global->conf, "interface.path");

        ETHIF_config_t conf = {0};

        if (sscanf(conf_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &conf.MAC[0], &conf.MAC[1],
                    &conf.MAC[2], &conf.MAC[3], &conf.MAC[4],
                    &conf.MAC[5]) != ARRAY_SIZE(conf.MAC)) {

                debug("%s: invalid MAC address: '%s'\n", conf_if_path, conf_mac);
                return -1;
        }

        conf.auto_negotiation = isstreq(conf_autoneg, "true") ? true : false;
        conf.speed = isstreq(conf_speed, "100M") ? ETHIF_SPEED__100Mbps : ETHIF_SPEED__10Mbps;
        conf.duplex = isstreq(conf_duplex, "full") ? ETHIF_DUPLEX__FULL : ETHIF_DUPLEX__HALF;

        int fd = open(conf_if_path, O_RDWR);
        if (fd >= 0) {
                int err = ioctl(fd, IOCTL_ETHIF__CONFIGURE, &conf);

                if (not err) {
                        err = ioctl(fd, IOCTL_ETHIF__START);
                }

                if (not err) {
                        errno = 0;
                        err   = ifadd("inet", NET_FAMILY__INET, conf_if_path);
                        debug("ifadd: %s\n", conf_if_path, strerror(errno));
                }

                close(fd);
        } else {
                debug("%s: %s\n", conf_if_path, strerror(errno));
                return -2;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Function configure network interface according to current configuration.
 */
//==============================================================================
static void ifup_by_config(void)
{
        const char *conf_ip_address = conf_parser__get_value(global->conf, "IP.address");
        const char *conf_ip_gateway = conf_parser__get_value(global->conf, "IP.gateway");
        const char *conf_ip_netmask = conf_parser__get_value(global->conf, "IP.netmask");
        const char *conf_ip_mode    = conf_parser__get_value(global->conf, "IP.mode");

        bool dhcp_enable = true;
        if (isstreq(conf_ip_mode, "DHCP")) {
                dhcp_enable = true;
        } else if (isstreq(conf_ip_mode, "static")) {
                dhcp_enable = false;
        } else {
                debug("unknown mode value: '%s', used DHCP\n", conf_ip_mode);
        }

        u8_t ip_address[4];
        if (sscanf(conf_ip_address, "%hhu.%hhu.%hhu.%hhu", &ip_address[0], &ip_address[1],
                    &ip_address[2], &ip_address[3]) != ARRAY_SIZE(ip_address)) {
                debug("invalid ip address: '%s', used DHCP\n", conf_ip_address);
        }

        u8_t ip_gateway[4];
        if (sscanf(conf_ip_gateway, "%hhu.%hhu.%hhu.%hhu", &ip_gateway[0], &ip_gateway[1],
                    &ip_gateway[2], &ip_gateway[3]) != ARRAY_SIZE(ip_gateway)) {
                debug("invalid ip address: '%s', used DHCP\n", conf_ip_gateway);
        }

        u8_t ip_netmask[4];
        if (sscanf(conf_ip_netmask, "%hhu.%hhu.%hhu.%hhu", &ip_netmask[0], &ip_netmask[1],
                    &ip_netmask[2], &ip_netmask[3]) != ARRAY_SIZE(ip_netmask)) {
                debug("invalid ip address: '%s', used DHCP\n", conf_ip_netmask);
        }

        NET_INET_config_t netcfg;

        if (dhcp_enable) {
                debug("applying DHCP configuration...\n");
                netcfg.mode    = NET_INET_MODE__DHCP_START;
                netcfg.address = NET_INET_IPv4_ANY;
                netcfg.mask    = NET_INET_IPv4_ANY;
                netcfg.gateway = NET_INET_IPv4_ANY;

        } else {
                debug("applying static configuration...\n");
                netcfg.mode    = NET_INET_MODE__STATIC;
                netcfg.address = NET_INET_IPv4(ip_address[0], ip_address[1],
                        ip_address[2], ip_address[3]);
                netcfg.mask = NET_INET_IPv4(ip_netmask[0], ip_netmask[1], ip_netmask[2],
                        ip_netmask[3]);
                netcfg.gateway = NET_INET_IPv4(ip_gateway[0], ip_gateway[1],
                        ip_gateway[2], ip_gateway[3]);
        }

        if (ifup("inet", &netcfg, sizeof(netcfg)) != 0) {
                ifdown("inet");
        }
}

/*==============================================================================
  End of file
==============================================================================*/
