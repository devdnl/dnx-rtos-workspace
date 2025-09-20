/*==============================================================================
File    ping.c

Author  Daniel Zorychta

Brief   Ping.

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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <dnx/os.h>
#include <dnx/net.h>
#include <dnx/misc.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define ICMP_ECHOREPLY        0
#define ICMP_DESTUNREACH      3
#define ICMP_SOURCEQUENCH     4
#define ICMP_REDIRECT         5
#define ICMP_ECHO             8
#define ICMP_TIMEEXCEEDED     11
#define ICMP_PARAMETERPROBLEM 12
#define ICMP_TIMESTAMP        13
#define ICMP_TIMESTAMPREPLY   14
#define ICMP_INFOREQUEST      15
#define ICMP_INFOREPLY        16
#define ICMP_ADDRMASKREQUEST  17
#define ICMP_ADDRMASKREPLY    18

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct PACKED {
        u8_t  version_ihl;
        u8_t  tos;
        u16_t len;
        u16_t id;
        u16_t offset;
        u8_t  ttl;
        u8_t  proto;
        u16_t chsum;
        u32_t src;
        u32_t dest;
        u8_t  data[];
} ip_header_t;

typedef struct PACKED {
        u8_t  type;
        u8_t  code;
        u16_t chksum;
        u16_t id;
        u16_t seqno;
        u8_t  data[56];
} icmp_packet_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION_BEGIN
SOCKET             *socket;
const char         *hostname;
const char         *netname;
NET_INET_sockaddr_t hostname_addr;
icmp_packet_t       icmp;
u32_t               icmp_num;
u32_t               packets_sent;
u32_t               packets_received;
u32_t               tmin;
u32_t               tmax;
u32_t               tsum;
GLOBAL_VARIABLES_SECTION_END

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(ping, STACK_DEPTH_LOW);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Calculate ICMP packet checksum.
 *
 * @param  buf          buffer
 * @param  len          buffer length
 *
 * @return Checksum.
 */
//==============================================================================
static u16_t cal_chksum(void *buf, int len)
{
        int    nleft  = len;
        int    sum    = 0;
        u16_t *w      = buf;
        u16_t  answer = 0;

        while (nleft > 1) {
                sum += *w++;
                nleft -= 2;
        }

        if (nleft == 1) {
                *(u16_t *)(&answer) = *(u16_t *)w;
                sum += answer;
        }

        sum = (sum >> 16) + (sum & 0xffff);
        sum += (sum >> 16);
        answer = ~sum;

        return answer;
}

//==============================================================================
/**
 * @brief  Send ICMCP echo packet.
 */
//==============================================================================
static void send_icmp_echo_packet(void)
{
        memset(&global->icmp, 0, sizeof(global->icmp));

        ++global->icmp_num;

        global->icmp.type   = ICMP_ECHO;
        global->icmp.code   = 0;
        global->icmp.chksum = 0;
        global->icmp.seqno  = global->icmp_num;
        global->icmp.id     = getpid();

        clock_t now = get_uptime_ms();
        memcpy(&global->icmp.data, &now, sizeof(now));

        global->icmp.chksum = cal_chksum(&global->icmp, sizeof(global->icmp));

        errno = 0;
        if (socket_sendto(global->socket, &global->icmp, sizeof(global->icmp), 0, &global->hostname_addr,
                    sizeof(global->hostname_addr)) < 0) {
                perror("send error");
        } else {
                global->packets_sent++;
        }
}

//==============================================================================
/**
 * @brief  Receive ICMP reply packet.
 */
//==============================================================================
static void recv_icmp_reply_packet(void)
{
        socket_set_recv_timeout(global->socket, 2000);

        u8_t buf[sizeof(ip_header_t) + sizeof(icmp_packet_t)];

        NET_INET_sockaddr_t from;

        errno = 0;
        int n = socket_recvfrom(global->socket, &buf, sizeof(buf), 0, &from, sizeof(from));
        if (n > 0) {
                ip_header_t   *ip   = cast(typeof(ip), buf);
                icmp_packet_t *icmp = cast(typeof(icmp), ip->data);

                if ((icmp->type == ICMP_ECHOREPLY) and (icmp->id == getpid())) {
                        clock_t tref;
                        memcpy(&tref, icmp->data, sizeof(tref));

                        u32_t travel_time = get_uptime_ms() - tref;

                        global->tmin = min(global->tmin, travel_time);
                        global->tmax = max(global->tmax, travel_time);
                        global->tsum += travel_time;

                        global->packets_received++;

                        printf("%u bytes from %s (%u.%u.%u.%u): icmp_seq=%u ttl=%u time=%lu ms\n", n,
                                global->hostname, NET_INET_IPv4_a(global->hostname_addr.addr),
                                NET_INET_IPv4_b(global->hostname_addr.addr),
                                NET_INET_IPv4_c(global->hostname_addr.addr),
                                NET_INET_IPv4_d(global->hostname_addr.addr), icmp->seqno, ip->ttl,
                                travel_time);
                }
        } else {
                perror("ICMP ECHO REPLY");
        }
}

//==============================================================================
/**
 * Main program function.
 *
 * Note: Please adjust stack size according to programs needs.
 *
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        global->tmax = 0;
        global->tmin = 1000;
        global->tsum = 0;

        if (argc < 2) {
                printf("Usage: ping <target-address> [network-name]\n", argv[0]);
                return EXIT_FAILURE;
        }

        int err          = EXIT_FAILURE;
        global->hostname = argv[1];

        if (argc == 3) {
                global->netname = argv[2];
        } else {
                global->netname = "inet";
        }

        u8_t a = 0, b = 0, c = 0, d = 0;
        if (sscanf(global->hostname, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) == 4) {
                global->hostname_addr.addr = NET_INET_IPv4(a, b, c, d);

        } else {
                if (get_host_by_name(global->netname, global->hostname, &global->hostname_addr,
                            sizeof(global->hostname_addr)) != 0) {
                        printf("Unable to resolve '%s' host address.\n", global->hostname);
                        goto cleanup;
                }
        }

        global->socket = socket_open(global->netname, NET_SOCKET_TYPE__RAW, NET_INET_PROTOCOL_ICMP);
        if (not global->socket) {
                printf("Unable to create ICMP socket!\n", socket);
                goto cleanup;
        }

        printf("PING %s (%u.%u.%u.%u) %u(%u) bytes of data.\n", global->hostname,
                NET_INET_IPv4_a(global->hostname_addr.addr), NET_INET_IPv4_b(global->hostname_addr.addr),
                NET_INET_IPv4_c(global->hostname_addr.addr), NET_INET_IPv4_d(global->hostname_addr.addr),
                sizeof(global->icmp.data), sizeof(icmp_packet_t) + sizeof(ip_header_t));

        clock_t start_time = get_uptime_ms();

        while (true) {
                clock_t tref = get_uptime_ms();

                ioctl(fileno(stdin), IOCTL_VFS__NON_BLOCKING_RD_MODE);
                clearerr(stdin);
                if (getchar() == ETX) {
                        break;
                }

                send_icmp_echo_packet();
                recv_icmp_reply_packet();

                i32_t wait = 1000 - (get_uptime_ms() - tref);
                if (wait > 0) {
                        msleep(wait);
                }
        }

        ioctl(fileno(stdin), IOCTL_VFS__DEFAULT_RD_MODE);

        u32_t work_time = get_uptime_ms() - start_time;
        u32_t loss      = ((global->packets_sent - global->packets_received) * 100) / global->packets_sent;

        printf("--- %s ping statistics ---\n", global->hostname);

        printf("%u packets transmitted, %u received, %u%% packet loss, time %lu ms\n", global->packets_sent,
                global->packets_received, loss, work_time);

        printf("rtt min/avg/max = %lu/%lu/%lu ms\n", global->tmin, global->tsum / global->packets_received,
                global->tmax);

cleanup:
        socket_close(global->socket);

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
