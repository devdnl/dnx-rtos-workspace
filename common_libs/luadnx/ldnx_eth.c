/*==============================================================================
File    ldnx_eth.c

Author  Daniel Zorychta

Brief   Lua bindings to dnx RTOS services: dnx.eth module (Ethernet frames,
        interfaces of class ETHIF).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

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
#include "luadnx.h"
#include "lauxlib.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Push fail result: nil, message, errno.
 */
//==============================================================================
static int fail(lua_State *L, int err)
{
        err = err ? err : EINVAL;
        luaL_pushfail(L);
        lua_pushstring(L, strerror(err));
        lua_pushinteger(L, err);
        return 3;
}

#ifdef IOCTL_ETHIF__START
//==============================================================================
/**
 * @brief  Execute request on interface file (path: argument 1).
 */
//==============================================================================
static int request(lua_State *L, int rq, void *arg)
{
        FILE *f = fopen(luaL_checkstring(L, 1), "r+");
        if (!f) {
                return errno;
        }

        int err = (ioctl(fileno(f), rq, arg) != 0) ? errno : 0;
        fclose(f);
        return err;
}
#endif

//==============================================================================
/**
 * @brief  dnx.eth.start(path, mac [, speed [, full_duplex]]): configure and
 *         start interface (speed 10 or 100: fixed mode, nil: auto-negotiation).
 *
 * @return true, or nil, message, errno.
 */
//==============================================================================
static int eth_start(lua_State *L)
{
#ifdef IOCTL_ETHIF__START
        ETHIF_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));

        const char *mac = luaL_checkstring(L, 2);
        luaL_argcheck(L,
                sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &cfg.MAC[0], &cfg.MAC[1], &cfg.MAC[2],
                        &cfg.MAC[3], &cfg.MAC[4], &cfg.MAC[5]) == 6,
                2, "MAC address expected");

        cfg.auto_negotiation = lua_isnoneornil(L, 3);
        cfg.speed = (luaL_optinteger(L, 3, 100) == 100) ? ETHIF_SPEED__100Mbps : ETHIF_SPEED__10Mbps;
        cfg.duplex = (lua_isnoneornil(L, 4) || lua_toboolean(L, 4)) ? ETHIF_DUPLEX__FULL : ETHIF_DUPLEX__HALF;

        request(L, IOCTL_ETHIF__STOP, NULL);

        int err = request(L, IOCTL_ETHIF__CONFIGURE, &cfg);
        if (!err) {
                err = request(L, IOCTL_ETHIF__START, NULL);
        }

        if (err) {
                return fail(L, err);
        }

        lua_pushboolean(L, true);
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.eth.stop(path): stop interface.
 *
 * @return true, or nil, message, errno.
 */
//==============================================================================
static int eth_stop(lua_State *L)
{
#ifdef IOCTL_ETHIF__STOP
        int err = request(L, IOCTL_ETHIF__STOP, NULL);
        if (err) {
                return fail(L, err);
        }
        lua_pushboolean(L, true);
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.eth.send(path, frame): send frame (string, without FCS).
 *
 * @return true, or nil, message, errno.
 */
//==============================================================================
static int eth_send(lua_State *L)
{
#ifdef IOCTL_ETHIF__SEND_PACKET
        size_t len;
        const char *frame = luaL_checklstring(L, 2, &len);
        luaL_argcheck(L, len <= IOCTL_ETHIF_MAX_PACKET_SIZE, 2, "frame too long");

        ETHIF_packet_t pkt = {.payload = (void *)frame, .length = len};

        int err = request(L, IOCTL_ETHIF__SEND_PACKET, &pkt);
        if (err) {
                return fail(L, err);
        }
        lua_pushboolean(L, true);
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.eth.receive(path [, timeout_ms]): receive frame.
 *
 * @return Frame (string), or nil, message, errno (ETIME: timeout).
 */
//==============================================================================
static int eth_receive(lua_State *L)
{
#ifdef IOCTL_ETHIF__RECEIVE_PACKET
        lua_Integer timeout = luaL_optinteger(L, 2, 0);
        luaL_argcheck(L, timeout >= 0, 2, "invalid timeout");

        u8_t *buf = malloc(IOCTL_ETHIF_MAX_PACKET_SIZE);
        if (!buf) {
                return fail(L, ENOMEM);
        }

        int err = 0;
        ETHIF_packet_t pkt = {.payload = buf, .length = IOCTL_ETHIF_MAX_PACKET_SIZE};

        FILE *f = fopen(luaL_checkstring(L, 1), "r+");
        if (f) {
                u32_t tmo = timeout;
                if ((ioctl(fileno(f), IOCTL_ETHIF__SET_RX_TIMEOUT, &tmo) != 0) ||
                        (ioctl(fileno(f), IOCTL_ETHIF__RECEIVE_PACKET, &pkt) != 0)) {
                        err = errno;
                }
                fclose(f);
        } else {
                err = errno;
        }

        if (!err) {
                lua_pushlstring(L, (const char *)buf, pkt.length);
        }

        free(buf);
        return err ? fail(L, err) : 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  dnx.eth.status(path): interface status.
 *
 * @return Table {state = "reset"|"ready"|"error", link = "disconnected"|
 *         "connected"|"phy_error", speed = 10|100, full_duplex, configured,
 *         mac, rx_packets, tx_packets, rx_bytes, tx_bytes, rx_dropped}, or
 *         nil, message, errno.
 */
//==============================================================================
static int eth_status(lua_State *L)
{
#ifdef IOCTL_ETHIF__GET_STATUS
        static const char *const STATE[] = {"reset", "ready", "error"};
        static const char *const LINK[] = {"disconnected", "connected", "phy_error"};

        ETHIF_status_t st;
        memset(&st, 0, sizeof(st));

        int err = request(L, IOCTL_ETHIF__GET_STATUS, &st);
        if (err) {
                return fail(L, err);
        }

        char mac[18];
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", st.MAC[0], st.MAC[1], st.MAC[2],
                st.MAC[3], st.MAC[4], st.MAC[5]);

        lua_createtable(L, 0, 11);
        lua_pushstring(L, STATE[st.state % 3]);
        lua_setfield(L, -2, "state");
        lua_pushstring(L, LINK[st.link_status % 3]);
        lua_setfield(L, -2, "link");
        lua_pushinteger(L, (st.speed == ETHIF_SPEED__100Mbps) ? 100 : 10);
        lua_setfield(L, -2, "speed");
        lua_pushboolean(L, st.duplex == ETHIF_DUPLEX__FULL);
        lua_setfield(L, -2, "full_duplex");
        lua_pushboolean(L, st.configured);
        lua_setfield(L, -2, "configured");
        lua_pushstring(L, mac);
        lua_setfield(L, -2, "mac");
        lua_pushinteger(L, (lua_Integer)st.rx_packets);
        lua_setfield(L, -2, "rx_packets");
        lua_pushinteger(L, (lua_Integer)st.tx_packets);
        lua_setfield(L, -2, "tx_packets");
        lua_pushinteger(L, (lua_Integer)st.rx_bytes);
        lua_setfield(L, -2, "rx_bytes");
        lua_pushinteger(L, (lua_Integer)st.tx_bytes);
        lua_setfield(L, -2, "tx_bytes");
        lua_pushinteger(L, st.rx_dropped_frames);
        lua_setfield(L, -2, "rx_dropped");
        return 1;
#else
        luaL_checkstring(L, 1);
        return fail(L, ENOTSUP);
#endif
}

//==============================================================================
/**
 * @brief  Open dnx.eth module.
 *
 * @param  L            Lua state
 *
 * @return Number of results (module table).
 */
//==============================================================================
int luaopen_dnx_eth(lua_State *L)
{
        static const luaL_Reg eth_lib[] = {
                {"start", eth_start},
                {"stop", eth_stop},
                {"send", eth_send},
                {"receive", eth_receive},
                {"status", eth_status},
                {NULL, NULL},
        };

        luaL_newlib(L, eth_lib);
        return 1;
}

/*==============================================================================
  End of file
==============================================================================*/
