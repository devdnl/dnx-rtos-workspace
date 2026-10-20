/*==============================================================================
File    libmqtt.c

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

#pragma GCC optimize("Os")

/*==============================================================================
  Include files
==============================================================================*/
#include "libmqtt.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dnx/vt100.h>
#include <sys/stat.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define VERBOSE_SESSION_WARNING(...)                                     \
        do {                                                             \
                time_t _vsw_t = time(NULL);                              \
                struct tm _vsw_lt;                                       \
                localtime_r(&_vsw_t, &_vsw_lt);                          \
                char _vsw_str[32];                                       \
                strftime(_vsw_str, sizeof(_vsw_str), "%F %X", &_vsw_lt); \
                printf("[libmqtt, warning, %s] ", _vsw_str);             \
                printf(__VA_ARGS__);                                     \
        } while (false)

/* number of remembered incoming QoS2 packet IDs (duplicate detection) */
#ifndef LIBMQTT_QOS2_RX_SLOTS
#define LIBMQTT_QOS2_RX_SLOTS 8
#endif

#define MQTT_MAX_REMAINING_LENGTH 268435455UL
#define MQTT_MAX_STRING_LENGTH    0xFFFFU

#define mqtt_fixed_header_flags_mask (0x0F)
#define mqtt_fixed_header_type_pos   4

#define mqtt_connect_flag__user_name    (1 << 7)
#define mqtt_connect_flag__password     (1 << 6)
#define mqtt_connect_flag__will_retain  (1 << 5)
#define mqtt_connect_flag__will_qos_pos 3
#define mqtt_connect_flag__will_flag    (1 << 2)
#define mqtt_connect_flag__clean_start  (1 << 1)

#define mqtt_publish_flag__duplicated (1 << 3)
#define mqtt_publish_flag__qos_mask   (3 << 1)
#define mqtt_publish_flag__qos_pos    1
#define mqtt_publish_flag__retain     (1 << 0)

#define mqtt_suback_failure 0x80

#define mqtt_get_type(fixed_header) (((fixed_header) >> mqtt_fixed_header_type_pos) & 0x0F)

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        mqtt_msg_type__reserved0 = 0,
        mqtt_msg_type__connect = 1,
        mqtt_msg_type__connack = 2,
        mqtt_msg_type__publish = 3,
        mqtt_msg_type__puback = 4,
        mqtt_msg_type__pubrec = 5,
        mqtt_msg_type__pubrel = 6,
        mqtt_msg_type__pubcomp = 7,
        mqtt_msg_type__subscribe = 8,
        mqtt_msg_type__suback = 9,
        mqtt_msg_type__unsubscribe = 10,
        mqtt_msg_type__unsuback = 11,
        mqtt_msg_type__pingreq = 12,
        mqtt_msg_type__pingresp = 13,
        mqtt_msg_type__disconnect = 14,
        mqtt_msg_type__reserved15 = 15,
} mqtt_msg_type_t;

typedef struct {
        mqtt_msg_type_t type;
        u8_t fixed_header_flags;
        u32_t variable_header_length;
        u32_t payload_length; // payload == NULL: only length is declared,
                              // payload is sent separately by the caller
        const void *variable_header;
        const void *payload;
} mqtt_control_packet_t;

typedef enum {
        packet_step__fixed_header,
        packet_step__remaining_length,
        packet_step__variable_header_and_payload,
        packet_step__discard,
} packet_step_t;

typedef struct {
        u8_t *buf;
        size_t seek;
        size_t size;
        size_t level;
} buffer_t;

typedef struct {
        u8_t buffer[128];
        buffer_t fetch_buf;
        packet_step_t step;
        u8_t control_packet_type;
        u8_t remaining_length_part;
        u32_t remaining_length;
        u32_t received_length;
        u32_t discard_left;
        u8_t *variable_header_and_payload;
} packet_t;

typedef struct {
        char *topic;
        void (*callback)(libmqtt_subscribe_msg_t *msg);
        void *context;
        libmqtt_qos_t qos;
        u16_t pending_id; // != 0: added by SUBSCRIBE waiting for SUBACK
} subscription_t;

struct libmqtt {
        i64_t keep_alive_tref;
        i64_t server_response_tref;
        subscription_t *subfilter;
        libmqtt_conf_t conf;
        packet_t rx_packet;
        buffer_t rx_buf;
        buffer_t tx_buf;
        buffer_t push_buf;
        u32_t keep_alive_ms;
        u16_t packet_id_ctr;
        u16_t qos2_rx_id[LIBMQTT_QOS2_RX_SLOTS];
        u8_t qos2_rx_next;
        bool in_callback;
        int fatal_err;
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int session_pushbuf_flush(libmqtt_t *self);

/*==============================================================================
  Local objects
==============================================================================*/

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Read big-endian (network order) 16-bit value.
 *
 * @param  buf          source buffer (at least 2 bytes)
 *
 * @return Read value.
 */
//==============================================================================
static inline u16_t get_u16(const u8_t *buf)
{
        return cast(u16_t, (buf[0] << 8) | buf[1]);
}

//==============================================================================
/**
 * @brief  Write big-endian (network order) 16-bit value.
 *
 * @param  buf          destination buffer (at least 2 bytes)
 * @param  value        value to write
 */
//==============================================================================
static inline void put_u16(u8_t *buf, u16_t value)
{
        buf[0] = cast(u8_t, value >> 8);
        buf[1] = cast(u8_t, value & 0xFF);
}

//==============================================================================
/**
 * @brief  Store connection-breaking error. Every next operation returns it.
 *
 * @param  self         instance
 * @param  err          negative error code
 *
 * @return err
 */
//==============================================================================
static int set_fatal(libmqtt_t *self, int err)
{
        if (self->fatal_err == 0) {
                self->fatal_err = err;
        }

        return err;
}

//==============================================================================
/**
 * @brief  Check if instance can perform operation.
 *
 * @param  self         instance
 * @param  waits        operation waits for broker response (uses receiver)
 *
 * @return 0 if allowed, negative error otherwise.
 */
//==============================================================================
static int check_state(libmqtt_t *self, bool waits)
{
        if (self->fatal_err) {
                return self->fatal_err;
        }

        if (waits and self->in_callback) {
                return -EFAULT;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Reads data from a fetch buffer (refilled from socket when empty).
 *
 * @param  self         instance
 * @param  dst          destination buffer
 * @param  count        maximum number of bytes to read
 *
 * @return Number of bytes read, 0 on timeout, negative value on error
 *         (error is fatal for the connection).
 */
//==============================================================================
static int session_fetchbuf_read(libmqtt_t *self, void *dst, size_t count)
{
        buffer_t *fetch_buf = &self->rx_packet.fetch_buf;

        if (fetch_buf->seek >= fetch_buf->level) {
                fetch_buf->seek = 0;
                fetch_buf->level = 0;

                errno = 0;
                int l = self->conf.socket_recv(&self->conf, fetch_buf->buf, cast(u16_t, fetch_buf->size));
                if (l < 0) {
                        if ((errno == ETIME) or (errno == EAGAIN)) {
                                return 0;
                        }

                        int err = errno ? errno : EIO;
                        VERBOSE_SESSION_WARNING("fetching error: %s\n", strerror(err));
                        return set_fatal(self, -err);

                } else if (l == 0) {
                        VERBOSE_SESSION_WARNING("connection closed by peer\n");
                        return set_fatal(self, -ECONNRESET);
                }

                fetch_buf->level = cast(size_t, l);
        }

        size_t left = fetch_buf->level - fetch_buf->seek;
        size_t len = (count < left) ? count : left;

        memcpy(dst, &fetch_buf->buf[fetch_buf->seek], len);
        fetch_buf->seek += len;

        return cast(int, len);
}

//==============================================================================
/**
 * @brief  Receive MQTT packet (non-blocking state machine).
 *
 * Received packet is available in self->rx_packet (control byte, remaining
 * length) and self->rx_buf (variable header and payload).
 *
 * @param  self         instance
 *
 * @return Packet size when complete packet received, 0 when reception
 *         is pending, -EFBIG when oversized packet was dropped,
 *         other negative value on error (fatal).
 */
//==============================================================================
static int session_receive_packet(libmqtt_t *self)
{
        packet_t *packet = &self->rx_packet;

        /*
         * Drop rest of the packet that does not fit to the receive buffer.
         */
        while (packet->step == packet_step__discard) {
                u8_t tmp[32];
                size_t chunk = (packet->discard_left < sizeof(tmp)) ? packet->discard_left : sizeof(tmp);

                int n = session_fetchbuf_read(self, tmp, chunk);
                if (n <= 0) {
                        return n;
                }

                packet->discard_left -= cast(u32_t, n);
                if (packet->discard_left == 0) {
                        packet->step = packet_step__fixed_header;
                }
        }

        if (packet->step == packet_step__fixed_header) {
                u8_t byte = 0;
                int n = session_fetchbuf_read(self, &byte, 1);
                if (n <= 0) {
                        return n;
                }

                u8_t type = mqtt_get_type(byte);
                if ((type == mqtt_msg_type__reserved0) or (type == mqtt_msg_type__reserved15)) {
                        VERBOSE_SESSION_WARNING("invalid packet type %u\n", type);
                        return set_fatal(self, -EILSEQ);
                }

                packet->control_packet_type = byte;
                packet->remaining_length_part = 0;
                packet->remaining_length = 0;
                packet->received_length = 0;
                packet->step = packet_step__remaining_length;
        }

        while (packet->step == packet_step__remaining_length) {
                u8_t byte = 0;
                int n = session_fetchbuf_read(self, &byte, 1);
                if (n <= 0) {
                        return n;
                }

                if (packet->remaining_length_part >= 4) {
                        VERBOSE_SESSION_WARNING("malformed remaining length\n");
                        return set_fatal(self, -EILSEQ);
                }

                packet->remaining_length |= cast(u32_t, byte & 0x7F) << (packet->remaining_length_part * 7);
                packet->remaining_length_part++;

                if ((byte & 0x80) == 0) {
                        self->server_response_tref = self->conf.get_time_ms();

                        if (packet->remaining_length > self->rx_buf.size) {
                                VERBOSE_SESSION_WARNING("packet too large (%lu B), dropped\n",
                                        cast(unsigned long, packet->remaining_length));
                                packet->discard_left = packet->remaining_length;
                                packet->step = packet_step__discard;
                                return -EFBIG;
                        }

                        packet->variable_header_and_payload = self->rx_buf.buf;
                        packet->step = packet_step__variable_header_and_payload;
                }
        }

        while (packet->received_length < packet->remaining_length) {
                size_t left = packet->remaining_length - packet->received_length;
                int n = session_fetchbuf_read(self,
                        &packet->variable_header_and_payload[packet->received_length], left);
                if (n <= 0) {
                        return n;
                }

                packet->received_length += cast(u32_t, n);
        }

        packet->step = packet_step__fixed_header;

        self->server_response_tref = self->conf.get_time_ms();

        return cast(int, 1 + packet->remaining_length_part + packet->received_length);
}

//==============================================================================
/**
 * @brief  Writes data to a push buffer, sends buffer to socket when full.
 *
 * @param  self         instance
 * @param  src          source data
 * @param  count        number of bytes to write
 *
 * @return 0 on success, negative value on error (fatal).
 */
//==============================================================================
static int session_pushbuf_write(libmqtt_t *self, const void *src, size_t count)
{
        buffer_t *push_buf = &self->push_buf;
        const u8_t *byte = src;

        while (count > 0) {
                if (push_buf->seek >= push_buf->size) {
                        int err = session_pushbuf_flush(self);
                        if (err < 0) {
                                return err;
                        }
                }

                push_buf->buf[push_buf->seek++] = *byte++;
                count--;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Flushes the push buffer by sending its contents to the socket.
 *
 * @param  self         instance
 *
 * @return 0 on success, negative value on error (fatal).
 */
//==============================================================================
static int session_pushbuf_flush(libmqtt_t *self)
{
        buffer_t *push_buf = &self->push_buf;

        if (push_buf->seek == 0) {
                return 0;
        }

        size_t len = push_buf->seek;
        push_buf->seek = 0;

        errno = 0;
        int n = self->conf.socket_send(&self->conf, push_buf->buf, cast(u16_t, len));
        if (n != cast(int, len)) {
                int err = errno ? errno : EIO;
                VERBOSE_SESSION_WARNING("write error: %s\n", strerror(err));
                return set_fatal(self, -err);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Serializes and sends an MQTT control packet.
 *
 * If packet->payload is NULL only payload length is declared in the fixed
 * header and payload must be sent separately by the caller.
 *
 * @param  self         instance
 * @param  packet       control packet to send
 *
 * @return 0 on success, negative error code otherwise.
 */
//==============================================================================
static int session_serialize_packet(libmqtt_t *self, const mqtt_control_packet_t *packet)
{
        if (self->fatal_err) {
                return self->fatal_err;
        }

        if ((packet->variable_header_length > MQTT_MAX_REMAINING_LENGTH) or
                (packet->payload_length > MQTT_MAX_REMAINING_LENGTH - packet->variable_header_length)) {
                return -EFBIG;
        }

        u32_t remaining_length = packet->variable_header_length + packet->payload_length;

        u8_t header[5];
        size_t header_len = 0;

        header[header_len++] = cast(u8_t,
                (packet->type << mqtt_fixed_header_type_pos) |
                        (packet->fixed_header_flags & mqtt_fixed_header_flags_mask));

        do {
                u8_t byte = remaining_length & 0x7F;
                remaining_length >>= 7;
                if (remaining_length > 0) {
                        byte |= 0x80;
                }
                header[header_len++] = byte;
        } while (remaining_length > 0);

        int err = session_pushbuf_write(self, header, header_len);

        if (not err and packet->variable_header and packet->variable_header_length) {
                err = session_pushbuf_write(self, packet->variable_header, packet->variable_header_length);
        }

        if (not err and packet->payload and packet->payload_length) {
                err = session_pushbuf_write(self, packet->payload, packet->payload_length);
        }

        if (not err) {
                err = session_pushbuf_flush(self);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Send packet consisting only of a packet identifier
 *         (PUBACK, PUBREC, PUBREL, PUBCOMP).
 *
 * @param  self         instance
 * @param  type         packet type
 * @param  flags        fixed header flags (0x02 for PUBREL)
 * @param  packet_id    packet identifier
 *
 * @return 0 on success, negative error code otherwise.
 */
//==============================================================================
static int send_ack(libmqtt_t *self, mqtt_msg_type_t type, u8_t flags, u16_t packet_id)
{
        u8_t variable_header[2];
        put_u16(variable_header, packet_id);

        mqtt_control_packet_t packet = {0};
        packet.type = type;
        packet.fixed_header_flags = flags;
        packet.variable_header = variable_header;
        packet.variable_header_length = sizeof(variable_header);

        return session_serialize_packet(self, &packet);
}

//==============================================================================
/**
 * @brief  Copy string as MQTT string (2 B length + data) to the buffer.
 *         Zero-length strings are encoded as 0x00 0x00.
 *
 * @param  buf          destination buffer (at least len + 2 bytes)
 * @param  string       string to copy (can be NULL if len is 0)
 * @param  len          string length
 *
 * @return Number of bytes written.
 */
//==============================================================================
static size_t copy_mqtt_string(u8_t *buf, const char *string, u16_t len)
{
        put_u16(buf, len);

        if (len) {
                memcpy(buf + sizeof(u16_t), string, len);
        }

        return len + sizeof(u16_t);
}

//==============================================================================
/**
 * @brief  Get next packet identifier (never 0).
 *
 * @param  self         instance
 *
 * @return Packet identifier.
 */
//==============================================================================
static u16_t get_packet_id(libmqtt_t *self)
{
        u16_t packet_id = self->packet_id_ctr++;

        if (packet_id == 0) {
                packet_id = self->packet_id_ctr++;
        }

        return packet_id;
}

//==============================================================================
/**
 * @brief  Check topic filter syntax ('+' whole level, '#' last level only).
 *
 * @param  filter       topic filter (NUL-terminated)
 *
 * @return true if filter is valid, false otherwise.
 */
//==============================================================================
static bool topic_filter_is_valid(const char *filter)
{
        if (filter[0] == '\0') {
                return false;
        }

        for (size_t i = 0; filter[i] != '\0'; i++) {
                if (filter[i] == '+') {
                        if (((i > 0) and (filter[i - 1] != '/')) or
                                ((filter[i + 1] != '\0') and (filter[i + 1] != '/'))) {
                                return false;
                        }

                } else if (filter[i] == '#') {
                        if (((i > 0) and (filter[i - 1] != '/')) or (filter[i + 1] != '\0')) {
                                return false;
                        }
                }
        }

        return true;
}

//==============================================================================
/**
 * @brief  Check topic name used for publishing (no wildcards, not empty).
 *
 * @param  topic        topic name (NUL-terminated)
 *
 * @return true if topic name is valid, false otherwise.
 */
//==============================================================================
static bool topic_name_is_valid(const char *topic)
{
        return (topic[0] != '\0') and not strpbrk(topic, "+#");
}

//==============================================================================
/**
 * @brief  Match topic name against topic filter (MQTT 3.1.1, 4.7).
 *
 * @param  filter       NUL-terminated, valid topic filter
 * @param  topic        topic name (not NUL-terminated)
 * @param  topic_len    topic name length
 *
 * @return true if topic matches filter.
 */
//==============================================================================
static bool topic_matches(const char *filter, const char *topic, size_t topic_len)
{
        size_t t = 0;

        /* topics beginning with '$' are not matched by filters starting with wildcard */
        if ((topic_len > 0) and (topic[0] == '$') and ((filter[0] == '+') or (filter[0] == '#'))) {
                return false;
        }

        while (*filter != '\0') {
                if (*filter == '#') {
                        return true;
                }

                if (*filter == '+') {
                        while ((t < topic_len) and (topic[t] != '/')) {
                                t++;
                        }
                        filter++;
                        continue;
                }

                if (t == topic_len) {
                        /* "a/#" also matches parent level "a" */
                        return (filter[0] == '/') and (filter[1] == '#') and (filter[2] == '\0');
                }

                if (topic[t] != *filter) {
                        return false;
                }

                t++;
                filter++;
        }

        return t == topic_len;
}

//==============================================================================
/**
 * @brief  Find subscription by exact filter string.
 *
 * @param  self         instance
 * @param  filter       topic filter
 *
 * @return Subscription slot or NULL if not found.
 */
//==============================================================================
static subscription_t *find_subscription(libmqtt_t *self, const char *filter)
{
        for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                if (self->subfilter[i].topic and isstreq(self->subfilter[i].topic, filter)) {
                        return &self->subfilter[i];
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Find free subscription slot.
 *
 * @param  self         instance
 *
 * @return Free slot or NULL if all slots are used.
 */
//==============================================================================
static subscription_t *find_free_slot(libmqtt_t *self)
{
        for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                if (not self->subfilter[i].topic) {
                        return &self->subfilter[i];
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Count free subscription slots.
 *
 * @param  self         instance
 *
 * @return Number of free slots.
 */
//==============================================================================
static size_t count_free_slots(libmqtt_t *self)
{
        size_t free_slots = 0;

        for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                if (not self->subfilter[i].topic) {
                        free_slots++;
                }
        }

        return free_slots;
}

//==============================================================================
/**
 * @brief  Remove subscription (release topic and clear slot).
 *
 * @param  sub          subscription slot
 */
//==============================================================================
static void remove_subscription(subscription_t *sub)
{
        free(sub->topic);
        memset(sub, 0, sizeof(*sub));
}

//==============================================================================
/**
 * @brief  Remove subscriptions added by SUBSCRIBE with selected packet ID.
 *
 * @param  self         instance
 * @param  packet_id    SUBSCRIBE packet identifier
 */
//==============================================================================
static void rollback_subscriptions(libmqtt_t *self, u16_t packet_id)
{
        for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                if (self->subfilter[i].topic and (self->subfilter[i].pending_id == packet_id)) {
                        remove_subscription(&self->subfilter[i]);
                }
        }
}

//==============================================================================
/**
 * @brief  Register incoming QoS2 packet ID (duplicate detection).
 *
 * @param  self         instance
 * @param  packet_id    PUBLISH packet identifier
 *
 * @return true if message is new (should be delivered), false if duplicate.
 */
//==============================================================================
static bool qos2_rx_register(libmqtt_t *self, u16_t packet_id)
{
        for (size_t i = 0; i < LIBMQTT_QOS2_RX_SLOTS; i++) {
                if (self->qos2_rx_id[i] == packet_id) {
                        return false;
                }
        }

        self->qos2_rx_id[self->qos2_rx_next] = packet_id;
        self->qos2_rx_next = cast(u8_t, (self->qos2_rx_next + 1) % LIBMQTT_QOS2_RX_SLOTS);

        return true;
}

//==============================================================================
/**
 * @brief  Release incoming QoS2 packet ID (PUBREL received).
 *
 * @param  self         instance
 * @param  packet_id    PUBREL packet identifier
 */
//==============================================================================
static void qos2_rx_release(libmqtt_t *self, u16_t packet_id)
{
        for (size_t i = 0; i < LIBMQTT_QOS2_RX_SLOTS; i++) {
                if (self->qos2_rx_id[i] == packet_id) {
                        self->qos2_rx_id[i] = 0;
                }
        }
}

//==============================================================================
/**
 * @brief  Handle received PUBLISH packet: validate, dispatch to matching
 *         subscriptions and acknowledge (QoS1/QoS2).
 *
 * @param  self         instance
 * @param  flags        fixed header flags of received packet
 *
 * @return 0 on success, negative value on error.
 */
//==============================================================================
static int handle_publish(libmqtt_t *self, u8_t flags)
{
        const u8_t *vh = self->rx_packet.variable_header_and_payload;
        u32_t rem = self->rx_packet.remaining_length;
        u8_t qos = (flags & mqtt_publish_flag__qos_mask) >> mqtt_publish_flag__qos_pos;

        if ((qos > libmqtt_qos__QOS2) or (rem < sizeof(u16_t))) {
                VERBOSE_SESSION_WARNING("invalid publish packet!\n");
                return set_fatal(self, -EILSEQ);
        }

        u16_t topic_len = get_u16(vh);
        size_t header_len = sizeof(u16_t) + topic_len + (qos ? sizeof(u16_t) : 0);

        if ((topic_len == 0) or (header_len > rem)) {
                VERBOSE_SESSION_WARNING("invalid publish packet content!\n");
                return set_fatal(self, -EILSEQ);
        }

        u16_t packet_id = 0;
        if (qos > libmqtt_qos__QOS0) {
                packet_id = get_u16(&vh[sizeof(u16_t) + topic_len]);
                if (packet_id == 0) {
                        VERBOSE_SESSION_WARNING("invalid publish packet ID!\n");
                        return set_fatal(self, -EILSEQ);
                }
        }

        bool deliver = (qos == libmqtt_qos__QOS2) ? qos2_rx_register(self, packet_id) : true;

        if (deliver) {
                const char *topic = cast(const char *, &vh[sizeof(u16_t)]);

                self->in_callback = true;

                for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                        subscription_t *sub = &self->subfilter[i];

                        if (not sub->topic or not sub->callback) {
                                continue;
                        }

                        if (not topic_matches(sub->topic, topic, topic_len)) {
                                continue;
                        }

                        libmqtt_subscribe_msg_t msg = {
                                .self = self,
                                .context = sub->context,
                                .topic = topic,
                                .topic_len = topic_len,
                                .payload = &vh[header_len],
                                .payload_len = rem - header_len,
                        };

                        sub->callback(&msg);
                }

                self->in_callback = false;
        }

        /* acknowledge always, also when no subscription matched */
        if (qos == libmqtt_qos__QOS1) {
                return send_ack(self, mqtt_msg_type__puback, 0, packet_id);

        } else if (qos == libmqtt_qos__QOS2) {
                return send_ack(self, mqtt_msg_type__pubrec, 0, packet_id);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  MQTT packet reception cycle.
 *
 * @param  self         instance
 * @param  recv_type    received packet type
 *
 * @return On packet received positive value is returned (packet size).
 *         On busy (reception pending) 0 is returned.
 *         On error negative value is returned.
 */
//==============================================================================
static int loop(libmqtt_t *self, mqtt_msg_type_t *recv_type)
{
        int err = check_state(self, true);
        if (err) {
                return err;
        }

        i64_t now = self->conf.get_time_ms();

        /*
         * Keep alive ping
         */
        if (now - self->keep_alive_tref >= (self->keep_alive_ms / 2)) {
                self->keep_alive_tref = now;

                mqtt_control_packet_t packet = {0};
                packet.type = mqtt_msg_type__pingreq;

                err = session_serialize_packet(self, &packet);
                if (err) {
                        return err;
                }
        }

        /*
         * Receive packet
         */
        int packet_size = session_receive_packet(self);
        if (packet_size < 0) {
                return packet_size;

        } else if (packet_size == 0) {
                if (now - self->server_response_tref >= self->keep_alive_ms) {
                        VERBOSE_SESSION_WARNING("no server response during keep alive interval!\n");
                        return set_fatal(self, -ETIME);
                }

                return 0;
        }

        u8_t control = self->rx_packet.control_packet_type;
        mqtt_msg_type_t type = mqtt_get_type(control);
        u8_t flags = control & mqtt_fixed_header_flags_mask;

        if (recv_type) {
                *recv_type = type;
        }

        switch (type) {
        case mqtt_msg_type__publish:
                err = handle_publish(self, flags);
                break;

        case mqtt_msg_type__pubrec:
        case mqtt_msg_type__pubrel: {
                if (self->rx_packet.remaining_length < sizeof(u16_t)) {
                        err = set_fatal(self, -EILSEQ);
                        break;
                }

                u16_t packet_id = get_u16(self->rx_packet.variable_header_and_payload);

                if (type == mqtt_msg_type__pubrec) {
                        err = send_ack(self, mqtt_msg_type__pubrel, 0x02, packet_id);
                } else {
                        qos2_rx_release(self, packet_id);
                        err = send_ack(self, mqtt_msg_type__pubcomp, 0, packet_id);
                }
                break;
        }

        default:
                break;
        }

        return err ? err : packet_size;
}

//==============================================================================
/**
 * @brief  Wait for selected packet from broker.
 *
 * @param  self         instance
 * @param  expected     expected packet type
 * @param  check_id     compare packet identifier (first 2 bytes)
 * @param  packet_id    expected packet identifier
 *
 * @return 0 when packet received (available in rx_packet), negative on error.
 */
//==============================================================================
static int wait_for_packet(libmqtt_t *self, mqtt_msg_type_t expected, bool check_id, u16_t packet_id)
{
        i64_t tref = self->conf.get_time_ms();

        while (true) {
                if (self->conf.get_time_ms() - tref >= self->conf.timeout_ms) {
                        return -ETIME;
                }

                mqtt_msg_type_t type = mqtt_msg_type__reserved0;
                int n = loop(self, &type);
                if (n == -EFBIG) {
                        continue; // oversized packet dropped, connection still valid

                } else if (n < 0) {
                        return n;

                } else if ((n == 0) or (type != expected)) {
                        continue;
                }

                if (not check_id) {
                        return 0;
                }

                if ((self->rx_packet.remaining_length >= sizeof(u16_t)) and
                        (get_u16(self->rx_packet.variable_header_and_payload) == packet_id)) {
                        return 0;
                }
        }
}

//==============================================================================
/**
 * @brief  Prepare PUBLISH packet header in tx buffer.
 *
 * @param  self         instance
 * @param  topic        topic name
 * @param  qos          quality of service
 * @param  retain       retain flag
 * @param  payload_len  payload length
 * @param  packet       [out] prepared control packet (payload pointer not set)
 * @param  packet_id    [out] packet identifier (0 for QoS0)
 *
 * @return 0 on success, negative value on error.
 */
//==============================================================================
static int build_publish(libmqtt_t *self, const char *topic, libmqtt_qos_t qos, bool retain,
        size_t payload_len, mqtt_control_packet_t *packet, u16_t *packet_id)
{
        size_t topic_len = strlen(topic);

        if (not topic_name_is_valid(topic) or (topic_len > MQTT_MAX_STRING_LENGTH)) {
                return -EINVAL;
        }

        if (sizeof(u16_t) + topic_len + sizeof(u16_t) > self->tx_buf.size) {
                return -ENOSPC;
        }

        u8_t *variable_header = self->tx_buf.buf;
        size_t variable_header_len = copy_mqtt_string(variable_header, topic, cast(u16_t, topic_len));

        *packet_id = 0;

        if (qos > libmqtt_qos__QOS0) {
                *packet_id = get_packet_id(self);
                put_u16(&variable_header[variable_header_len], *packet_id);
                variable_header_len += sizeof(u16_t);
        }

        if (payload_len > MQTT_MAX_REMAINING_LENGTH - variable_header_len) {
                return -EFBIG;
        }

        memset(packet, 0, sizeof(*packet));
        packet->type = mqtt_msg_type__publish;
        packet->fixed_header_flags = cast(u8_t,
                (qos << mqtt_publish_flag__qos_pos) | (retain ? mqtt_publish_flag__retain : 0));
        packet->variable_header = variable_header;
        packet->variable_header_length = cast(u32_t, variable_header_len);
        packet->payload_length = cast(u32_t, payload_len);

        return 0;
}

//==============================================================================
/**
 * @brief  Create new library instance
 *
 * @param  conf         configuration
 *
 * @return On success library instance object, on error NULL.
 */
//==============================================================================
libmqtt_t *libmqtt__new(const libmqtt_conf_t *conf)
{
        if (not conf or not conf->socket_recv or not conf->socket_send or not conf->rx_buf_size or
                not conf->tx_buf_size or not conf->out_buf_size or not conf->get_time_ms or
                not conf->max_subscribe_topics or not conf->keep_alive or not conf->timeout_ms or
                (conf->out_buf_size > UINT16_MAX)) {
                return NULL;
        }

        libmqtt_t *self = calloc(1, sizeof(*self));
        if (not self) {
                return NULL;
        }

        self->conf = *conf;

        self->push_buf.size = conf->out_buf_size;
        self->push_buf.buf = malloc(conf->out_buf_size);
        if (not self->push_buf.buf) {
                return libmqtt__delete(self);
        }

        self->rx_buf.size = conf->rx_buf_size;
        self->rx_buf.buf = malloc(conf->rx_buf_size);
        if (not self->rx_buf.buf) {
                return libmqtt__delete(self);
        }

        self->tx_buf.size = conf->tx_buf_size;
        self->tx_buf.buf = malloc(conf->tx_buf_size);
        if (not self->tx_buf.buf) {
                return libmqtt__delete(self);
        }

        self->subfilter = calloc(conf->max_subscribe_topics, sizeof(*self->subfilter));
        if (not self->subfilter) {
                return libmqtt__delete(self);
        }

        self->rx_packet.fetch_buf.buf = self->rx_packet.buffer;
        self->rx_packet.fetch_buf.size = sizeof(self->rx_packet.buffer);
        self->rx_packet.variable_header_and_payload = self->rx_buf.buf;
        self->packet_id_ctr = 1;
        self->keep_alive_ms = (cast(u32_t, conf->keep_alive) * 1000);

        return self;
}

//==============================================================================
/**
 * @brief  Delete library instance.
 *
 * @param  self         instance
 *
 * @return NULL.
 */
//==============================================================================
libmqtt_t *libmqtt__delete(libmqtt_t *self)
{
        if (self) {
                free(self->push_buf.buf);
                free(self->rx_buf.buf);
                free(self->tx_buf.buf);

                if (self->subfilter) {
                        for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                                free(self->subfilter[i].topic);
                        }

                        free(self->subfilter);
                }

                free(self);
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Connect to broker.
 *
 * @param  self         instance
 * @param  conn_params  connection parameters
 *
 * @return CONNACK return code (0 = accepted) or negative error code.
 */
//==============================================================================
int libmqtt__connect(libmqtt_t *self, const libmqtt_connect_params_t *conn_params)
{
        if (not self or not conn_params or not conn_params->client_id) {
                return -EINVAL;
        }

        int err = check_state(self, true);
        if (err) {
                return err;
        }

        const libmqtt_connect_params_t *p = conn_params;

        if ((p->password and not p->user_name) or ((p->will_topic != NULL) != (p->will_msg != NULL)) or
                (p->will_qos > libmqtt_qos__QOS2)) {
                return -EINVAL;
        }

        bool last_will = (p->will_topic != NULL);

        if (last_will and not topic_name_is_valid(p->will_topic)) {
                return -EINVAL;
        }

        size_t client_id_len = strlen(p->client_id);
        size_t will_topic_len = last_will ? strlen(p->will_topic) : 0;
        size_t will_msg_len = last_will ? strlen(p->will_msg) : 0;
        size_t user_name_len = p->user_name ? strlen(p->user_name) : 0;
        size_t password_len = p->password ? strlen(p->password) : 0;

        if ((client_id_len > MQTT_MAX_STRING_LENGTH) or (will_topic_len > MQTT_MAX_STRING_LENGTH) or
                (will_msg_len > MQTT_MAX_STRING_LENGTH) or (user_name_len > MQTT_MAX_STRING_LENGTH) or
                (password_len > MQTT_MAX_STRING_LENGTH)) {
                return -EINVAL;
        }

        size_t required = sizeof(u16_t) + client_id_len;
        required += last_will ? (2 * sizeof(u16_t) + will_topic_len + will_msg_len) : 0;
        required += p->user_name ? (sizeof(u16_t) + user_name_len) : 0;
        required += p->password ? (sizeof(u16_t) + password_len) : 0;

        if (required > self->tx_buf.size) {
                return -ENOSPC;
        }

        size_t payload_len = 0;
        u8_t *payload = self->tx_buf.buf;

        payload_len += copy_mqtt_string(&payload[payload_len], p->client_id, cast(u16_t, client_id_len));

        if (last_will) {
                payload_len += copy_mqtt_string(&payload[payload_len], p->will_topic,
                        cast(u16_t, will_topic_len));
                payload_len += copy_mqtt_string(&payload[payload_len], p->will_msg,
                        cast(u16_t, will_msg_len));
        }

        if (p->user_name) {
                payload_len += copy_mqtt_string(&payload[payload_len], p->user_name,
                        cast(u16_t, user_name_len));
        }

        if (p->password) {
                payload_len += copy_mqtt_string(&payload[payload_len], p->password,
                        cast(u16_t, password_len));
        }

        u8_t connect_flags = mqtt_connect_flag__clean_start;

        if (p->user_name) {
                connect_flags |= mqtt_connect_flag__user_name;
        }

        if (p->password) {
                connect_flags |= mqtt_connect_flag__password;
        }

        if (last_will) {
                connect_flags |= mqtt_connect_flag__will_flag;
                connect_flags |= cast(u8_t, p->will_qos << mqtt_connect_flag__will_qos_pos);
                if (p->will_retain) {
                        connect_flags |= mqtt_connect_flag__will_retain;
                }
        }

        u8_t variable_header[10] = {
                0,
                4,
                'M',
                'Q',
                'T',
                'T',
                4, // protocol level: MQTT 3.1.1
                connect_flags,
                cast(u8_t, self->conf.keep_alive >> 8),
                cast(u8_t, self->conf.keep_alive & 0xFF),
        };

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__connect;
        packet.variable_header = variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = cast(u32_t, payload_len);

        err = session_serialize_packet(self, &packet);
        if (err) {
                return err;
        }

        i64_t tref = self->conf.get_time_ms();
        self->server_response_tref = tref;
        self->keep_alive_tref = tref;

        err = wait_for_packet(self, mqtt_msg_type__connack, false, 0);
        if (err) {
                return err;
        }

        if (self->rx_packet.remaining_length != 2) {
                VERBOSE_SESSION_WARNING("invalid CONNACK packet!\n");
                return set_fatal(self, -EILSEQ);
        }

        return self->rx_packet.variable_header_and_payload[1];
}

//==============================================================================
/**
 * @brief  Disconnect broker.
 *
 * @param  self         instance
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int libmqtt__disconnect(libmqtt_t *self)
{
        if (not self) {
                return -EINVAL;
        }

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__disconnect;

        return session_serialize_packet(self, &packet);
}

//==============================================================================
/**
 * @brief  Subscribe topics.
 *
 * @param  self         instance
 * @param  topic        topic array
 * @param  topics       topics in array
 *
 * @return On success 0 is returned. -EACCES if broker rejected any topic
 *         (rejected topics are not registered, accepted stay subscribed).
 */
//==============================================================================
int libmqtt__subscribe(libmqtt_t *self, const libmqtt_subscribe_t *topic, size_t topics)
{
        if (not self or not topic or not topics) {
                return -EINVAL;
        }

        int err = check_state(self, true);
        if (err) {
                return err;
        }

        size_t payload_len = 0;
        size_t new_slots = 0;

        for (size_t t = 0; t < topics; t++) {
                if (not topic[t].topic or not topic[t].callback or (topic[t].qos > libmqtt_qos__QOS2) or
                        not topic_filter_is_valid(topic[t].topic)) {
                        return -EINVAL;
                }

                size_t len = strlen(topic[t].topic);
                if (len > MQTT_MAX_STRING_LENGTH) {
                        return -EINVAL;
                }

                payload_len += sizeof(u16_t) + len + sizeof(u8_t /*qos*/);

                if (not find_subscription(self, topic[t].topic)) {
                        new_slots++;
                }
        }

        if (payload_len > self->tx_buf.size) {
                return -ENOSPC;
        }

        if (new_slots > count_free_slots(self)) {
                return -ENOSPC;
        }

        u16_t packet_id = get_packet_id(self);

        /*
         * New filters are registered before SUBSCRIBE is sent because broker
         * is allowed to send matching PUBLISH before SUBACK (MQTT 3.1.1, 3.8.4).
         */
        for (size_t t = 0; t < topics; t++) {
                if (find_subscription(self, topic[t].topic)) {
                        continue;
                }

                subscription_t *sub = find_free_slot(self);
                sub->topic = strdup(topic[t].topic);
                if (not sub->topic) {
                        rollback_subscriptions(self, packet_id);
                        return -ENOMEM;
                }

                sub->callback = topic[t].callback;
                sub->context = topic[t].context;
                sub->qos = topic[t].qos;
                sub->pending_id = packet_id;
        }

        u8_t *payload = self->tx_buf.buf;
        payload_len = 0;

        for (size_t t = 0; t < topics; t++) {
                payload_len += copy_mqtt_string(&payload[payload_len], topic[t].topic,
                        cast(u16_t, strlen(topic[t].topic)));
                payload[payload_len++] = cast(u8_t, topic[t].qos);
        }

        u8_t variable_header[2];
        put_u16(variable_header, packet_id);

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__subscribe;
        packet.fixed_header_flags = 0x02;
        packet.variable_header = variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = cast(u32_t, payload_len);

        err = session_serialize_packet(self, &packet);
        if (not err) {
                err = wait_for_packet(self, mqtt_msg_type__suback, true, packet_id);
        }

        if (err) {
                rollback_subscriptions(self, packet_id);
                return err;
        }

        if (self->rx_packet.remaining_length != sizeof(u16_t) + topics) {
                VERBOSE_SESSION_WARNING("invalid SUBACK packet!\n");
                rollback_subscriptions(self, packet_id);
                return set_fatal(self, -EILSEQ);
        }

        const u8_t *codes = &self->rx_packet.variable_header_and_payload[sizeof(u16_t)];
        bool rejected = false;

        for (size_t t = 0; t < topics; t++) {
                subscription_t *sub = find_subscription(self, topic[t].topic);

                if (codes[t] > libmqtt_qos__QOS2) { // 0x80: failure
                        VERBOSE_SESSION_WARNING("subscription of '%s' rejected\n", topic[t].topic);
                        rejected = true;

                        if (sub and (sub->pending_id == packet_id)) {
                                remove_subscription(sub);
                        }
                        continue;
                }

                if (sub) {
                        sub->callback = topic[t].callback;
                        sub->context = topic[t].context;
                        sub->qos = cast(libmqtt_qos_t, codes[t]); // granted QoS
                        sub->pending_id = 0;
                }
        }

        return rejected ? -EACCES : 0;
}

//==============================================================================
/**
 * @brief  Unsubscribe topics.
 *
 * @param  self         instance
 * @param  topic        topic to unsubscribe
 * @param  topics       topics to unsubscribe
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int libmqtt__unsubscribe(libmqtt_t *self, const char *topic[], size_t topics)
{
        if (not self or not topic or not topics) {
                return -EINVAL;
        }

        int err = check_state(self, true);
        if (err) {
                return err;
        }

        size_t payload_len = 0;

        for (size_t t = 0; t < topics; t++) {
                if (not topic[t] or not topic_filter_is_valid(topic[t])) {
                        return -EINVAL;
                }

                size_t len = strlen(topic[t]);
                if (len > MQTT_MAX_STRING_LENGTH) {
                        return -EINVAL;
                }

                payload_len += sizeof(u16_t) + len;
        }

        if (payload_len > self->tx_buf.size) {
                return -ENOSPC;
        }

        u16_t packet_id = get_packet_id(self);

        u8_t *payload = self->tx_buf.buf;
        payload_len = 0;

        for (size_t t = 0; t < topics; t++) {
                payload_len += copy_mqtt_string(&payload[payload_len], topic[t],
                        cast(u16_t, strlen(topic[t])));
        }

        u8_t variable_header[2];
        put_u16(variable_header, packet_id);

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__unsubscribe;
        packet.fixed_header_flags = 0x02;
        packet.variable_header = variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = cast(u32_t, payload_len);

        err = session_serialize_packet(self, &packet);
        if (err) {
                return err;
        }

        err = wait_for_packet(self, mqtt_msg_type__unsuback, true, packet_id);
        if (err) {
                return err;
        }

        for (size_t t = 0; t < topics; t++) {
                subscription_t *sub = find_subscription(self, topic[t]);
                if (sub) {
                        remove_subscription(sub);
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Publish message.
 *
 * @param  self         instance
 * @param  publish      publish
 *
 * @return On success 0 returned.
 */
//==============================================================================
int libmqtt__publish(libmqtt_t *self, const libmqtt_publish_t *publish)
{
        if (not self or not publish or not publish->topic or
                (publish->payload_len and not publish->payload) or (publish->qos > libmqtt_qos__QOS2)) {
                return -EINVAL;
        }

        int err = check_state(self, publish->qos > libmqtt_qos__QOS0);
        if (err) {
                return err;
        }

        mqtt_control_packet_t packet;
        u16_t packet_id;

        err = build_publish(self, publish->topic, publish->qos, publish->retain, publish->payload_len,
                &packet, &packet_id);
        if (err) {
                return err;
        }

        packet.payload = publish->payload;

        err = session_serialize_packet(self, &packet);
        if (err) {
                return err;
        }

        if (publish->qos == libmqtt_qos__QOS0) {
                return 0;
        }

        return wait_for_packet(self,
                (publish->qos == libmqtt_qos__QOS1) ? mqtt_msg_type__puback : mqtt_msg_type__pubcomp, true,
                packet_id);
}

//==============================================================================
/**
 * @brief  Publish message from file.
 *
 * @param  self         instance
 * @param  publish      publish
 *
 * @return On success 0 returned.
 */
//==============================================================================
int libmqtt__publish_file(libmqtt_t *self, const libmqtt_publish_file_t *publish)
{
        if (not self or not publish or not publish->topic or not publish->path or
                (publish->qos > libmqtt_qos__QOS2)) {
                return -EINVAL;
        }

        int err = check_state(self, publish->qos > libmqtt_qos__QOS0);
        if (err) {
                return err;
        }

        struct stat fp_stat;
        errno = 0;
        if (stat(publish->path, &fp_stat) != 0) {
                return errno ? -errno : -EIO;
        }

        if (fp_stat.st_size <= 0) {
                return -EAGAIN;
        }

        if (cast(unsigned long long, fp_stat.st_size) > MQTT_MAX_REMAINING_LENGTH) {
                return -EFBIG;
        }

        u32_t file_size = cast(u32_t, fp_stat.st_size);

        mqtt_control_packet_t packet;
        u16_t packet_id;

        err = build_publish(self, publish->topic, publish->qos, publish->retain, file_size, &packet,
                &packet_id);
        if (err) {
                return err;
        }

        errno = 0;
        FILE *fp = fopen(publish->path, "rb");
        if (not fp) {
                return errno ? -errno : -EIO;
        }

        packet.payload = NULL; // payload sent below directly from file

        err = session_serialize_packet(self, &packet);

        size_t chunk_max = (self->tx_buf.size < UINT16_MAX) ? self->tx_buf.size : UINT16_MAX;
        u32_t left = file_size;

        while (not err and (left > 0)) {
                size_t chunk = (left < chunk_max) ? left : chunk_max;

                errno = 0;
                size_t n = fread(self->tx_buf.buf, 1, chunk, fp);
                if (n == 0) {
                        int e = (ferror(fp) and errno) ? errno : EIO;
                        VERBOSE_SESSION_WARNING("file read error (file changed?): %s\n", strerror(e));
                        // packet length already declared: stream can not be recovered
                        err = set_fatal(self, -e);
                        break;
                }

                errno = 0;
                int l = self->conf.socket_send(&self->conf, self->tx_buf.buf, cast(u16_t, n));
                if (l != cast(int, n)) {
                        int e = errno ? errno : EIO;
                        VERBOSE_SESSION_WARNING("(file) write error: %s\n", strerror(e));
                        err = set_fatal(self, -e);
                        break;
                }

                left -= cast(u32_t, n);
        }

        fclose(fp);

        if (err) {
                return err;
        }

        // long transfer blocks keep alive handling: do not report false timeout
        self->server_response_tref = self->conf.get_time_ms();

        if (publish->qos == libmqtt_qos__QOS0) {
                return 0;
        }

        return wait_for_packet(self,
                (publish->qos == libmqtt_qos__QOS1) ? mqtt_msg_type__puback : mqtt_msg_type__pubcomp, true,
                packet_id);
}

//==============================================================================
/**
 * @brief  MQTT single loop.
 *
 * @param  self         instance
 *
 * @return On success 0 is returned. -EFBIG when oversized packet was
 *         dropped (connection still valid), other negative values on error.
 */
//==============================================================================
int libmqtt__loop_once(libmqtt_t *self)
{
        if (not self) {
                return -EINVAL;
        }

        int n = loop(self, NULL);
        return (n >= 0) ? 0 : n;
}

/*==============================================================================
  End of file
==============================================================================*/
