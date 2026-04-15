/*==============================================================================
File    libmqtt.h

Author  Daniel Zorychta

Brief   MQTT library (MQTT v3.1.1).

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
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <dnx/misc.h>
#include <stdio.h>
#include <dnx/net.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
#define LIBMQTT_VERSION 20260415

/*==============================================================================
  Exported object types
==============================================================================*/
typedef struct libmqtt libmqtt_t;

typedef enum {
        libmqtt_reason__success = 0x00,
        libmqtt_reason__normal_disconnection = 0x00,
        libmqtt_reason__granted_qos_0 = 0x00,
        libmqtt_reason__granted_qos_1 = 0x01,
        libmqtt_reason__granted_qos_2 = 0x02,
        libmqtt_reason__disconnect_with_will_message = 0x04,
        libmqtt_reason__no_matching_subscribers = 0x10,
        libmqtt_reason__no_subscription_existed = 0x11,
        libmqtt_reason__continue_authentication = 0x18,
        libmqtt_reason__re_authenticate = 0x19,
        libmqtt_reason__unspecified_error = 0x80,
        libmqtt_reason__malformed_packet = 0x81,
        libmqtt_reason__protocol_error = 0x82,
        libmqtt_reason__implementation_specific_error = 0x83,
        libmqtt_reason__unsupported_protocol_version = 0x84,
        libmqtt_reason__client_identifier_not_valid = 0x85,
        libmqtt_reason__bad_user_name_or_password = 0x86,
        libmqtt_reason__not_authorized = 0x87,
        libmqtt_reason__server_unavailable = 0x88,
        libmqtt_reason__server_busy = 0x89,
        libmqtt_reason__banned = 0x8a,
        libmqtt_reason__server_shutting_down = 0x8b,
        libmqtt_reason__bad_authentication_method = 0x8c,
        libmqtt_reason__keep_alive_timeout = 0x8d,
        libmqtt_reason__session_taken_over = 0x8e,
        libmqtt_reason__topic_filter_invalid = 0x8f,
        libmqtt_reason__topic_name_invalid = 0x90,
        libmqtt_reason__packet_identifier_in_use = 0x91,
        libmqtt_reason__packet_identifier_not_found = 0x92,
        libmqtt_reason__receive_maximum_exceeded = 0x93,
        libmqtt_reason__topic_alias_invalid = 0x94,
        libmqtt_reason__packet_too_large = 0x95,
        libmqtt_reason__message_rate_too_high = 0x96,
        libmqtt_reason__quota_exceeded = 0x97,
        libmqtt_reason__administrative_action = 0x98,
        libmqtt_reason__payload_format_invalid = 0x99,
        libmqtt_reason__retain_not_supported = 0x9a,
        libmqtt_reason__qos_not_supported = 0x9b,
        libmqtt_reason__use_another_server = 0x9c,
        libmqtt_reason__server_moved = 0x9d,
        libmqtt_reason__shared_subscriptions_not_supported = 0x9e,
        libmqtt_reason__connection_rate_exceeded = 0x9f,
        libmqtt_reason__maximum_connect_time = 0xa0,
        libmqtt_reason__subscription_identifiers_not_supported = 0xa1,
        libmqtt_reason__wildcard_subscriptions_not_supported = 0xa2,
} libmqtt_reason_t;

typedef struct {
        const char *user_name;
        const char *password;
        const char *client_id;
        const char *will_topic;
        const char *will_msg;
} libmqtt_connect_params_t;

typedef struct libmqtt_conf {
        u32_t timeout_ms;   // MQTT receive timeout in milliseconds
        u32_t rx_buf_size;  // MQTT frame receive buffer
        u32_t tx_buf_size;  // MQTT frame transmit buffer
        u32_t out_buf_size; // socket output buffer
        void *context;
        void *socket;
        int (*socket_recv)(struct libmqtt_conf *conf, void *buf, u16_t len);
        int (*socket_send)(struct libmqtt_conf *conf, const void *buf, u16_t len);
        i64_t (*get_time_ms)(void);
        u16_t max_subscribe_topics;
        u16_t keep_alive;
} libmqtt_conf_t;

typedef enum {
        libmqtt_qos__QOS0,
        libmqtt_qos__QOS1,
        libmqtt_qos__QOS2,
} libmqtt_qos_t;

typedef struct {
        libmqtt_t *self;
        void *context;
        const char *topic;
        const void *payload;
        size_t topic_len;
        size_t payload_len;
} libmqtt_subscribe_msg_t;

typedef struct {
        char *topic;
        void (*callback)(libmqtt_subscribe_msg_t *msg);
        void *context;
        libmqtt_qos_t qos;
} libmqtt_subscribe_t;

typedef struct {
        const char *topic;
        libmqtt_qos_t qos;
        bool retain;
        const void *payload;
        size_t payload_len;
} libmqtt_publish_t;

typedef struct {
        const char *topic;
        libmqtt_qos_t qos;
        bool retain;
        const char *path;
} libmqtt_publish_file_t;

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  Exported functions
==============================================================================*/
extern libmqtt_t *libmqtt__new(const libmqtt_conf_t *conf);
extern libmqtt_t *libmqtt__delete(libmqtt_t *self);
extern int libmqtt__connect(libmqtt_t *self, const libmqtt_connect_params_t *conn_params);
extern int libmqtt__disconnect(libmqtt_t *self);
extern int libmqtt__subscribe(libmqtt_t *self, const libmqtt_subscribe_t *topic, size_t topics);
extern int libmqtt__unsubscribe(libmqtt_t *self, const char *topic[], size_t topics);
extern int libmqtt__publish(libmqtt_t *self, const libmqtt_publish_t *publish);
extern int libmqtt__publish_file(libmqtt_t *self, const libmqtt_publish_file_t *publish);
extern int libmqtt__loop_once(libmqtt_t *self);

/*==============================================================================
  Exported inline functions
==============================================================================*/

#ifdef __cplusplus
}
#endif

/*==============================================================================
  End of file
==============================================================================*/
