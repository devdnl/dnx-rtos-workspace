/*==============================================================================
File    netmgr.h

Author  Daniel Zorychta

Brief   Network Manager.

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
#include <dnx/net.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <conf_parser.h>
#include <sys/shm.h>
#include <dnx/os.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "shared.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define debug(...) printf("NM: "__VA_ARGS__);

#define MAX_NETWORKS 4
#define NAME_LEN     16
#define MAIN_NETWORK "inet"

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
/**
 * Network of configuration: sections [interface] and [IP] configure network
 * "inet", sections [interface:<name>] and [IP:<name>] other networks.
 */
typedef struct {
        char name[NAME_LEN];
        NET_INET_state_t last_state;
        bool failed; /* interface not configured (message printed once) */
} network_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void handle_interfaces(const char *config_file);
static int configure_ethernet(network_t *net);
static void ifup_by_config(network_t *net);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        conf_parser_t *conf;
        netmgr_shared_t *shared;
        network_t net[MAX_NETWORKS];
        size_t nets;
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

        char *option = argv[1];
        char *conf_file = argv[2];

        if (isstreq(option, "--daemon")) {

                if (not conf_file) {
                        debug("no config file path given!\n", option);
                        return EXIT_FAILURE;
                }

                void *mem;
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
                global->shared->conf_path = conf_file;
                global->shared->ready = true;

                handle_interfaces(conf_file);

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
 * @brief  Get configuration value of network.
 *
 * @param  net          network
 * @param  key          key of NETMGR_CONF_KEY__* ("<section>.<key>")
 *
 * @return Value (empty string if not exists).
 */
//==============================================================================
static const char *get(const network_t *net, const char *key)
{
        char k[64];
        const char *dot = strchr(key, '.');

        if (isstreq(net->name, MAIN_NETWORK) or not dot) {
                snprintf(k, sizeof(k), "%s", key);
        } else {
                snprintf(k, sizeof(k), "%.*s:%s%s", (int)(dot - key), key, net->name, dot);
        }

        const char *value = conf_parser__get_value(global->conf, k);
        return value ? value : "";
}

//==============================================================================
/**
 * @brief  Find networks of configuration: "inet" ([interface] section) and
 *         networks of sections [interface:<name>].
 */
//==============================================================================
static void find_networks(void)
{
        network_t found[MAX_NETWORKS];
        size_t n = 0;

        memset(found, 0, sizeof(found));

        const char *key;
        const char *val;
        bool main_found = false;

        for (size_t i = 0; conf_parser__get_pair(global->conf, i, &key, &val) == 0; i++) {
                const char *name = NULL;
                size_t len = 0;

                if (isstreqn(key, "interface.", 10)) {
                        if (main_found) {
                                continue;
                        }
                        main_found = true;
                        name = MAIN_NETWORK;
                        len = strlen(MAIN_NETWORK);

                } else if (isstreqn(key, "interface:", 10)) {
                        name = key + 10;
                        const char *dot = strchr(name, '.');
                        len = dot ? cast(size_t, dot - name) : 0;
                }

                if (!name || (len == 0) || (len >= NAME_LEN)) {
                        continue;
                }

                bool exists = false;
                for (size_t j = 0; j < n; j++) {
                        if ((strlen(found[j].name) == len) && isstreqn(found[j].name, name, len)) {
                                exists = true;
                        }
                }

                if (!exists) {
                        if (n < MAX_NETWORKS) {
                                memcpy(found[n].name, name, len);
                                found[n].last_state = NET_INET_STATE__LINK_DISCONNECTED;
                                n++;
                        } else {
                                debug("too many networks, '%.*s' ignored\n", (int)len, name);
                        }
                }
        }

        /* networks removed from configuration */
        for (size_t i = 0; i < global->nets; i++) {
                bool exists = false;
                for (size_t j = 0; j < n; j++) {
                        exists |= isstreq(global->net[i].name, found[j].name);
                }

                if (!exists) {
                        ifdown(global->net[i].name);
                        ifrm(global->net[i].name);
                }
        }

        for (size_t j = 0; j < n; j++) {
                for (size_t i = 0; i < global->nets; i++) {
                        if (isstreq(global->net[i].name, found[j].name)) {
                                found[j].failed = global->net[i].failed;
                        }
                }
        }

        memcpy(global->net, found, sizeof(found));
        global->nets = n;
}

//==============================================================================
/**
 * @brief  Create default configuration (network "inet", DHCP).
 *
 * @param  config_file  configuration file
 *
 * @return true on success.
 */
//==============================================================================
static bool create_default_config(const char *config_file)
{
        global->conf = conf_parser__new(NULL);
        if (!global->conf) {
                debug("unable to create configuration file!");
                return false;
        }

        srand(get_uptime_ms());
        char hwaddr[32] = "50:67:21";
        u8_t n = strlen(hwaddr);
        for (size_t i = 0; i < 3; i++) {
                n += snprintf(hwaddr + n, sizeof(hwaddr) - n, ":%02X", rand() & 0xFF);
        }

        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__INTERFACE_PATH, "/dev/eth");
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__INTERFACE_HWADDR, hwaddr);
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION,
                NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION_ENABLE);
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__INTERFACE_SPEED,
                NETMGR_CONF_KEY__INTERFACE_SPEED_100M);
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__INTERFACE_DUPLEX,
                NETMGR_CONF_KEY__INTERFACE_DUPLEX_FULL);
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__IP_MODE, NETMGR_CONF_KEY__IP_MODE_DHCP);
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__IP_ADDRESS, "0.0.0.0");
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__IP_NETMASK, "0.0.0.0");
        conf_parser__set_value(global->conf, NETMGR_CONF_KEY__IP_GATEWAY, "0.0.0.0");

        conf_parser__set_file_path(global->conf, config_file);
        conf_parser__save(global->conf);

        return true;
}

//==============================================================================
/**
 * @brief  Handle state of network.
 *
 * @param  net          network
 */
//==============================================================================
static void handle_network(network_t *net)
{
        NET_INET_status_t status;
        NET_family_t family;

        if (ifstatus(net->name, &family, &status, sizeof(status)) == 0) {

                switch (status.state) {
                case NET_INET_STATE__NOT_CONFIGURED:
                        ifup_by_config(net);
                        break;

                case NET_INET_STATE__LINK_DISCONNECTED:
                        if (net->last_state != status.state) {
                                debug("%s: link disconnected\n", net->name);
                        }
                        ifdown(net->name);
                        break;

                case NET_INET_STATE__DHCP_CONFIGURING:
                        break;

                case NET_INET_STATE__STATIC_IP:
                case NET_INET_STATE__DHCP_CONFIGURED:
                        if (net->last_state != status.state) {
                                debug("%s: estabilished IP connection.\n", net->name);
                                debug("Address: %u.%u.%u.%u\n", NET_INET_IPv4_a(status.address),
                                        NET_INET_IPv4_b(status.address), NET_INET_IPv4_c(status.address),
                                        NET_INET_IPv4_d(status.address));
                                debug("Mask   : %u.%u.%u.%u\n", NET_INET_IPv4_a(status.mask),
                                        NET_INET_IPv4_b(status.mask), NET_INET_IPv4_c(status.mask),
                                        NET_INET_IPv4_d(status.mask));
                                debug("Gateway: %u.%u.%u.%u\n", NET_INET_IPv4_a(status.gateway),
                                        NET_INET_IPv4_b(status.gateway), NET_INET_IPv4_c(status.gateway),
                                        NET_INET_IPv4_d(status.gateway));
                        }
                        break;

                default:
                        ifdown(net->name);
                        break;
                }

                net->last_state = status.state;
        } else {
                ifdown(net->name);
                if (configure_ethernet(net) == 0) {
                        ifup_by_config(net);
                }
        }
}

//==============================================================================
/**
 * @brief  Handle network interfaces.
 *
 * @param  config_file  configuration file
 */
//==============================================================================
static void handle_interfaces(const char *config_file)
{
        global->conf = conf_parser__new(config_file);
        if (!global->conf && !create_default_config(config_file)) {
                return;
        }

        while (true) {

                if (global->shared->reload_conf) {
                        for (size_t i = 0; i < global->nets; i++) {
                                ifdown(global->net[i].name);
                        }

                        if (conf_parser__reload(global->conf) > 0) {
                                global->shared->reload_conf = false;
                                find_networks();
                                for (size_t i = 0; i < global->nets; i++) {
                                        configure_ethernet(&global->net[i]);
                                }
                        } else {
                                sleep(5);
                                continue;
                        }
                }

                for (size_t i = 0; i < global->nets; i++) {
                        handle_network(&global->net[i]);
                }

                sleep(2);
        }
}

//==============================================================================
/**
 * @brief  Configure Ethernet interface of network and add network.
 *
 * @param  net          network
 *
 * @return 0 on success.
 */
//==============================================================================
static int configure_ethernet(network_t *net)
{
        const char *conf_mac = get(net, NETMGR_CONF_KEY__INTERFACE_HWADDR);
        const char *conf_autoneg = get(net, NETMGR_CONF_KEY__INTERFACE_AUTONEGOTIATION);
        const char *conf_speed = get(net, NETMGR_CONF_KEY__INTERFACE_SPEED);
        const char *conf_duplex = get(net, NETMGR_CONF_KEY__INTERFACE_DUPLEX);
        const char *conf_if_path = get(net, NETMGR_CONF_KEY__INTERFACE_PATH);

        ETHIF_config_t conf = {0};

        if (sscanf(conf_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &conf.MAC[0], &conf.MAC[1], &conf.MAC[2],
                    &conf.MAC[3], &conf.MAC[4], &conf.MAC[5]) != ARRAY_SIZE(conf.MAC)) {

                if (!net->failed) {
                        debug("%s: invalid MAC address: '%s'\n", net->name, conf_mac);
                }
                net->failed = true;
                return -1;
        }

        conf.auto_negotiation = conf_parser__is_value_true(conf_autoneg);
        conf.speed = isstreq(conf_speed, NETMGR_CONF_KEY__INTERFACE_SPEED_100M) ? ETHIF_SPEED__100Mbps
                                                                                : ETHIF_SPEED__10Mbps;
        conf.duplex = isstreq(conf_duplex, NETMGR_CONF_KEY__INTERFACE_DUPLEX_FULL) ? ETHIF_DUPLEX__FULL
                                                                                   : ETHIF_DUPLEX__HALF;

        int fd = open(conf_if_path, O_RDWR);
        if (fd >= 0) {
                ioctl(fd, IOCTL_ETHIF__STOP);

                int err = ioctl(fd, IOCTL_ETHIF__CONFIGURE, &conf);

                if (not err) {
                        err = ioctl(fd, IOCTL_ETHIF__START);
                }

                if (not err) {
                        errno = 0;
                        err = ifadd(net->name, NET_FAMILY__INET, conf_if_path);
                        debug("%s: ifadd %s: %s\n", net->name, conf_if_path, strerror(errno));
                }

                close(fd);
        } else {
                if (!net->failed) {
                        debug("%s: %s: %s\n", net->name, conf_if_path, strerror(errno));
                }
                net->failed = true;
                return -2;
        }

        net->failed = false;
        return 0;
}

//==============================================================================
/**
 * @brief  Function parses IPv4 address.
 *
 * @param  str          address string
 * @param  addr         address
 *
 * @return true on success.
 */
//==============================================================================
static bool parse_ip(const char *str, u32_t *addr)
{
        u8_t a[4];
        if (sscanf(str, "%hhu.%hhu.%hhu.%hhu", &a[0], &a[1], &a[2], &a[3]) != 4) {
                return false;
        }

        *addr = NET_INET_IPv4(a[0], a[1], a[2], a[3]);
        return true;
}

//==============================================================================
/**
 * @brief  Function configures network according to current configuration.
 *
 * @param  net          network
 */
//==============================================================================
static void ifup_by_config(network_t *net)
{
        const char *conf_ip_mode = get(net, NETMGR_CONF_KEY__IP_MODE);

        NET_INET_config_t netcfg;
        netcfg.mode = NET_INET_MODE__DHCP_START;
        netcfg.address = NET_INET_IPv4_ANY;
        netcfg.mask = NET_INET_IPv4_ANY;
        netcfg.gateway = NET_INET_IPv4_ANY;

        if (isstreq(conf_ip_mode, NETMGR_CONF_KEY__IP_MODE_STATIC)) {
                if (parse_ip(get(net, NETMGR_CONF_KEY__IP_ADDRESS), &netcfg.address) &&
                        parse_ip(get(net, NETMGR_CONF_KEY__IP_NETMASK), &netcfg.mask) &&
                        parse_ip(get(net, NETMGR_CONF_KEY__IP_GATEWAY), &netcfg.gateway)) {
                        netcfg.mode = NET_INET_MODE__STATIC;
                } else {
                        debug("%s: invalid IP address, used DHCP\n", net->name);
                }
        } else if (!isstreq(conf_ip_mode, NETMGR_CONF_KEY__IP_MODE_DHCP)) {
                debug("%s: unknown mode value: '%s', used DHCP\n", net->name, conf_ip_mode);
        }

        debug("%s: applying %s configuration...\n", net->name,
                (netcfg.mode == NET_INET_MODE__STATIC) ? "static" : "DHCP");

        if (ifup(net->name, &netcfg, sizeof(netcfg)) != 0) {
                ifdown(net->name);
        }
}

/*==============================================================================
  End of file
==============================================================================*/
