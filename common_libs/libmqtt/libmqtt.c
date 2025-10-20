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
#include <time.h>
#include <errno.h>
#include <dnx/vt100.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define VERBOSE_SESSION_WARNING(...)                          \
        do {                                                  \
                time_t t = time(NULL);                        \
                struct tm lt;                                 \
                localtime_r(&t, &lt);                         \
                char t_str[32];                               \
                strftime(t_str, sizeof(t_str), "%F %X", &lt); \
                printf("[libmqtt, warning, %s] ", t_str);     \
                printf(__VA_ARGS__);                          \
        } while (false)

#define mqtt_fixed_header_flags_mask     (0x0F)
#define mqtt_fixed_header_type_pos       4

#define mqtt_connect_flag__user_name     (1 << 7)
#define mqtt_connect_flag__password      (1 << 6)
#define mqtt_connect_flag__will_retain   (1 << 5)
#define mqtt_connect_flag__will_qos_mask (3 << 3)
#define mqtt_connect_flag__will_qos_0    (0 << 3)
#define mqtt_connect_flag__will_qos_1    (1 << 3)
#define mqtt_connect_flag__will_qos_2    (2 << 3)
#define mqtt_connect_flag__will_flag     (1 << 2)
#define mqtt_connect_flag__clean_start   (1 << 1)
#define mqtt_connect_flag__reserved      (1 << 0)

#define mqtt_connack_session_present     (1 << 0)

#define mqtt_publish_flag__duplicated    (1 << 3)
#define mqtt_publish_flag__qos_mask      (3 << 1)
#define mqtt_publish_flag__qos_pos       1
#define mqtt_publish_flag__qos_0         (0 << 1)
#define mqtt_publish_flag__qos_1         (1 << 1)
#define mqtt_publish_flag__qos_2         (2 << 1)
#define mqtt_publish_flag__retain        (1 << 0)

#define mqtt_get_type(fixed_header)      ((fixed_header >> 4) & 0x0F)

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

typedef struct PACKED {
        uint8_t control_packet_type;
} mqtt_fixed_header_t;

typedef struct {
        mqtt_msg_type_t type;
        uint8_t fixed_header_flags;
        int32_t variable_header_length;
        int32_t payload_length;
        const void *variable_header;
        const void *payload;
} mqtt_control_packet_t;

typedef enum {
        packet_step__fixed_header,
        packet_step__remaining_length,
        packet_step__variable_header_and_payload,
} packet_step_t;

typedef struct {
        uint8_t *buf;
        size_t seek;
        size_t size;
        size_t level;
} buffer_t;

typedef struct {
        u8_t buffer[128];
        buffer_t fetch_buf;
        packet_step_t step;
        u8_t remaining_length_part;
        uint32_t remaining_length;
        uint32_t received_length;
        uint32_t payload_read_index;
        mqtt_fixed_header_t fixed_header;
        u8_t *variable_header_and_payload;
} packet_t;

typedef struct PACKED {
        u8_t protocol_name[6];
        u8_t protocol_level;
        u8_t connect_flags;
        u16_t keep_alive;
} variable_packet_connect_t;

struct libmqtt {
        i64_t keep_alive_tref;
        i64_t server_response_tref;
        libmqtt_subscribe_t *subfilter;
        libmqtt_conf_t conf;
        packet_t rx_packet;
        buffer_t rx_buf;
        buffer_t tx_buf;
        buffer_t push_buf;
        uint32_t keep_alive_ms;
        uint16_t packet_id_ctr;
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int session_fetchbuf_read(libmqtt_t *self, void *dst, size_t count);

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
 * @brief  Function convert value for host/network purpose.
 *
 * @param  value        value to convert
 *
 * @return Converted value.
 */
//==============================================================================
static u16_t ntohu16(u16_t value)
{
        return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

//==============================================================================
/**
 * @brief  Receive MQTT packet.
 *
 * @param  session      session instance
 *
 * @return Number of received bytes, negative values on error.
 */
//==============================================================================
static int session_receive_packet(libmqtt_t *self)
{
        packet_t *packet = &self->rx_packet;

        if (packet->step == packet_step__fixed_header) {
                errno = 0;
                int n = session_fetchbuf_read(self, &packet->fixed_header, sizeof(packet->fixed_header));
                if (n < 0) {
                        return n;

                } else if (n == 0) {
                        return 0;

                } else if ((packet->fixed_header.control_packet_type >> 4) == mqtt_msg_type__reserved0) {
                        packet->step = packet_step__fixed_header;
                        return -EILSEQ;

                } else if ((packet->fixed_header.control_packet_type >> 4) >= mqtt_msg_type__reserved15) {
                        packet->step = packet_step__fixed_header;
                        return -EILSEQ;

                } else {
                        packet->remaining_length_part = 0;
                        packet->remaining_length = 0;
                        packet->received_length = 0;
                        packet->step = packet_step__remaining_length;
                }
        }

        while (packet->step == packet_step__remaining_length) {
                errno = 0;
                u8_t byte = 0;
                int n = session_fetchbuf_read(self, &byte, 1);
                if (n < 0) {
                        packet->step = packet_step__fixed_header;
                        return n;

                } else if (n == 0) {
                        return 0;

                } else if (packet->remaining_length_part >= 4) {
                        packet->step = packet_step__fixed_header;
                        return 0;

                } else {
                        packet->remaining_length += cast(int, (byte & 0x7F))
                                << (packet->remaining_length_part * 7);

                        packet->remaining_length_part++;

                        if ((byte & 0x80) == 0) {

                                if (packet->remaining_length > self->rx_buf.size) {
                                        packet->step = packet_step__fixed_header;
                                        return -ENOMEM;

                                } else if (packet->remaining_length <= self->rx_buf.size) {
                                        packet->variable_header_and_payload = self->rx_buf.buf;
                                }

                                packet->step = packet_step__variable_header_and_payload;
                                break;
                        }
                }
        }

        while ((packet->step == packet_step__variable_header_and_payload) and
                (packet->received_length < packet->remaining_length)) {

                size_t left = packet->remaining_length - packet->received_length;
                int n = session_fetchbuf_read(self,
                        &packet->variable_header_and_payload[packet->received_length], left);
                if (n < 0) {
                        packet->step = packet_step__fixed_header;
                        return n;

                } else if (n == 0) {
                        return 0;

                } else {
                        packet->received_length += n;
                }
        }

        packet->payload_read_index = 0;
        packet->step = packet_step__fixed_header;

        self->server_response_tref = self->conf.get_time_ms();

        return sizeof(packet->fixed_header) + packet->remaining_length_part + packet->received_length;
}

//==============================================================================
/**
 * @brief Reads data from a fetch buffer and stores it in the destination buffer.
 *
 * This function reads up to `count` bytes of data from a `buffer_t` buffer.
 * If the buffer does not contain enough data, it attempts to fetch more data
 * from the specified socket. The read data is copied into the provided `dst` buffer.
 *
 * @param session       Pointer to the session structure.
 * @param dst           Pointer to the destination buffer where the read data will be stored.
 * @param count         Number of bytes to read.
 *
 * @return On success, the number of bytes read is returned. On error, a negative
 *         value corresponding to the error code (stored in `errno`) is returned.
 */
//==============================================================================
static int session_fetchbuf_read(libmqtt_t *self, void *dst, size_t count)
{
        int n = 0;

        buffer_t *fetch_buf = &self->rx_packet.fetch_buf;

        if (fetch_buf->seek >= fetch_buf->level) {
                fetch_buf->seek = 0;
                fetch_buf->level = 0;

                errno = 0;
                int l = self->conf.socket_recv(&self->conf, fetch_buf->buf, fetch_buf->size);
                if (l < 0) {
                        if (errno == ETIME) {
                                return 0;

                        } else {
                                int err = errno;
                                VERBOSE_SESSION_WARNING("fetching error: %s\n", strerror(err));
                                n = -err;
                        }
                } else {
                        fetch_buf->level = l;
                }
        }

        if (fetch_buf->seek < fetch_buf->level) {
                size_t left = cast(size_t, (fetch_buf->level - fetch_buf->seek));
                size_t len = min(count, left);

                if (len) {
                        memcpy(dst, &fetch_buf->buf[fetch_buf->seek], len);
                        count -= len;
                        n += len;
                        dst += len;
                        fetch_buf->seek += len;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief Writes data to a push buffer and sends it to a socket when the buffer is full.
 *
 * This function writes up to `count` bytes of data from the `src` buffer to a `push_buf_t` buffer.
 * When the buffer becomes full, its contents are sent to the specified socket.
 *
 * @param session   Pointer to the session structure.
 * @param src       Pointer to the source buffer containing data to write.
 * @param count     Number of bytes to write.
 *
 * @return On success, the number of bytes written is returned. On error, the return value is
 *         negative and represents the error code.
 */
//==============================================================================
static int session_pushbuf_write(libmqtt_t *self, const void *src, size_t count)
{
        int n = 0;

        buffer_t *push_buf = &self->push_buf;

        const uint8_t *byte = src;

        while (count > 0) {
                if (push_buf->seek >= push_buf->size) {
                        errno = 0;
                        int l = self->conf.socket_send(&self->conf, push_buf->buf, push_buf->seek);
                        if (l != cast(int, push_buf->seek)) {
                                int err = errno;
                                VERBOSE_SESSION_WARNING("write error: %s\n", strerror(err));
                                push_buf->seek = 0;
                                return -err;
                        } else {
                                push_buf->seek = 0;
                        }
                }

                push_buf->buf[push_buf->seek++] = *byte++;
                count--;
                n++;
        }

        return n;
}

//==============================================================================
/**
 * @brief Flushes the push buffer by sending its contents to the socket.
 *
 * This function sends any remaining data in the `push_buf_t` buffer to the socket
 * and resets the buffer's position.
 *
 * @param session   Pointer to the session structure.
 *
 * @return On success, the number of bytes sent is returned. If no data is pending,
 *         returns 0. On error, a negative value is returned.
 */
//==============================================================================
static int session_pushbuf_flush(libmqtt_t *self)
{
        int n = 0;

        buffer_t *push_buf = &self->push_buf;

        if (push_buf->seek > 0) {
                errno = 0;
                n = self->conf.socket_send(&self->conf, push_buf->buf, push_buf->seek);
                if (n != cast(int, push_buf->seek)) {
                        int err = errno;
                        VERBOSE_SESSION_WARNING("flush error: %s\n", strerror(err));
                        n = -err;
                }

                push_buf->seek = 0;
        }

        return n;
}

//==============================================================================
/**
 * @brief Pushes an 8-bit integer to the session's push buffer.
 *
 * This function writes an 8-bit integer to the session's push buffer to be sent
 * through the socket.
 *
 * @param session  Pointer to the session structure.
 * @param val      Value to push.
 *
 * @return Returns 0 on success. Returns -1 if writing to the buffer fails.
 */
//==============================================================================
static int session_push_i8(libmqtt_t *self, int8_t val)
{
        if (session_pushbuf_write(self, &val, sizeof(val)) == sizeof(val)) {
                return 0;
        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Encodes and pushes a variable-length integer to the session's push buffer.
 *
 * This function encodes a variable-length integer (1 to 4 bytes) and writes it to
 * the session's push buffer for transmission.
 *
 * @param session   Pointer to the session structure.
 * @param vlength   Variable-length integer to encode and push.
 *
 * @return Returns 0 on success. Returns -1 if writing to the buffer fails.
 */
//==============================================================================
static int session_push_variable_length(libmqtt_t *self, int32_t vlength)
{
        uint8_t buf[4];
        int n = 0;

        do {
                uint8_t byte = vlength & 0x7F;
                vlength >>= 7;

                if (vlength > 0) {
                        byte |= 0x80;
                }

                buf[n++] = byte;

        } while ((vlength > 0) and (n < 4));

        if (session_pushbuf_write(self, buf, n) == n) {
                return 0;
        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Pushes a buffer of data to the session's push buffer.
 *
 * This function writes a specified buffer of data to the session's push buffer
 * for transmission through the socket.
 *
 * @param session  Pointer to the session structure.
 * @param buf      Pointer to the data buffer to push.
 * @param count    Number of bytes to push.
 *
 * @return Returns 0 on success. Returns -1 if writing to the buffer fails.
 */
//==============================================================================
static int session_push_buffer(libmqtt_t *self, const void *buf, size_t count)
{
        if (not buf or not count) {
                return 0;

        } else if (session_pushbuf_write(self, buf, count) == cast(int, count)) {
                return 0;

        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Serializes and sends an MQTT control packet.
 *
 * @param  session  Pointer to the MQTT session structure.
 * @param  packet   Pointer to the control packet structure to be serialized and sent.
 *
 * @return 0 if the packet was sent successfully, -1 otherwise.
 */
//==============================================================================
static int session_serialize_packet(libmqtt_t *self, const mqtt_control_packet_t *packet)
{
        int32_t remainder_length = packet->variable_header_length + packet->payload_length;

        if (session_push_i8(self,
                    (packet->type << 4) | (packet->fixed_header_flags & mqtt_fixed_header_flags_mask)) != 0) {
                return -1;

        } else if (session_push_variable_length(self, remainder_length) != 0) {
                return -2;

        } else if (session_push_buffer(self, packet->variable_header, packet->variable_header_length) != 0) {
                return -3;

        } else if (session_push_buffer(self, packet->payload, packet->payload_length) != 0) {
                return -4;
        }

        if (session_pushbuf_flush(self) < 0) {
                return -5;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Copy string as MQTT string to the buffer.
 *
 * @param  buf          destination buffer
 * @param  string       string to copy
 * @param  len          string length
 *
 * @return String size.
 */
//==============================================================================
static size_t copy_mqtt_string(u8_t *buf, const char *string, u16_t len)
{
        if (not buf or not string or not len) {
                return 0;
        }

        u16_t l = ntohu16(len);
        memcpy(buf, &l, sizeof(l));
        memcpy(buf + sizeof(u16_t), string, len);

        return len + sizeof(u16_t);
}

//==============================================================================
/**
 * @brief  Get packet ID.
 *
 * @param  self         instance
 *
 * @return Packet ID.
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
        if (not self) {
                return -EINVAL;
        }

        i64_t now = self->conf.get_time_ms();

        /*
         * Keep alive ping
         */
        if (now - self->keep_alive_tref >= (self->keep_alive_ms / 2)) {
                self->keep_alive_tref = now;

                mqtt_control_packet_t packet = {0};
                packet.type = mqtt_msg_type__pingreq;

                if (session_serialize_packet(self, &packet) != 0) {
                        return -EIO;
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
                        return -ETIME;
                } else {
                        return 0;
                }
        }

        mqtt_msg_type_t type = mqtt_get_type(self->rx_packet.fixed_header.control_packet_type);
        u8_t flags = self->rx_packet.fixed_header.control_packet_type & mqtt_fixed_header_flags_mask;

        if (recv_type) {
                *recv_type = type;
        }

        if (type == mqtt_msg_type__publish) {
                u16_t topic_len = ((self->rx_packet.variable_header_and_payload[0] << 8) |
                        self->rx_packet.variable_header_and_payload[1]);

                u16_t packet_id = 0;
                u16_t packet_id_len = 0;

                if ((flags & mqtt_publish_flag__qos_mask) > mqtt_publish_flag__qos_0) {
                        packet_id_len = sizeof(packet_id);
                        packet_id = ((self->rx_packet.variable_header_and_payload[topic_len + 0] << 8) |
                                self->rx_packet.variable_header_and_payload[topic_len + 1]);
                }

                char *topic = cast(char *, &self->rx_packet.variable_header_and_payload[2]);

                void *payload = &self->rx_packet.variable_header_and_payload[sizeof(topic_len) + topic_len +
                        packet_id_len];

                int payload_len = self->rx_packet.remaining_length - topic_len - sizeof(topic_len) -
                        packet_id_len;

                if ((sizeof(topic_len) + topic_len + packet_id_len + payload_len) !=
                        self->rx_packet.remaining_length) {
                        VERBOSE_SESSION_WARNING("invalid publish packet content!\n");
                        return 0;
                }

                for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                        if (not self->subfilter[i].topic) {
                                continue;
                        }

                        size_t cb_topic_len = strlen(self->subfilter[i].topic);

                        if ((topic_len == cb_topic_len) and
                                isstreqn(self->subfilter[i].topic, topic, topic_len)) {

                                libmqtt_subscribe_msg_t msg = {
                                        .self = self,
                                        .context = self->subfilter[i].context,
                                        .topic = topic,
                                        .topic_len = topic_len,
                                        .payload = payload,
                                        .payload_len = payload_len,
                                };

                                self->subfilter[i].callback(&msg);

                                if (self->subfilter[i].qos == libmqtt_qos__QOS1) {
                                        u16_t variable_header = ntohu16(packet_id);

                                        mqtt_control_packet_t packet = {0};
                                        packet.type = mqtt_msg_type__puback;
                                        packet.variable_header = &variable_header;
                                        packet.variable_header_length = sizeof(variable_header);
                                        if (session_serialize_packet(self, &packet) != 0) {
                                                return -EIO;
                                        }

                                } else if (self->subfilter[i].qos == libmqtt_qos__QOS2) {
                                        u16_t variable_header = ntohu16(packet_id);

                                        mqtt_control_packet_t packet = {0};
                                        packet.type = mqtt_msg_type__pubrec;
                                        packet.variable_header = &variable_header;
                                        packet.variable_header_length = sizeof(variable_header);
                                        if (session_serialize_packet(self, &packet) != 0) {
                                                return -EIO;
                                        }
                                }

                                break;
                        }
                }

        } else if (type == mqtt_msg_type__pubrec) {
                mqtt_control_packet_t packet = {0};
                packet.type = mqtt_msg_type__pubrel;
                packet.fixed_header_flags = 0x02;
                packet.variable_header = self->rx_packet.variable_header_and_payload;
                packet.variable_header_length = sizeof(u16_t);
                if (session_serialize_packet(self, &packet) != 0) {
                        return -ECONNABORTED;
                }

        } else if (type == mqtt_msg_type__pubrel) {
                mqtt_control_packet_t packet = {0};
                packet.type = mqtt_msg_type__pubcomp;
                packet.variable_header = self->rx_packet.variable_header_and_payload;
                packet.variable_header_length = sizeof(u16_t);
                if (session_serialize_packet(self, &packet) != 0) {
                        return -ECONNABORTED;
                }
        }

        return packet_size;
}

//==============================================================================
/**
 * @brief  Create new library instance
 *
 * @param  self         instance
 * @param  conf         IO functions.
 *
 * @return On success library instance object, on error NULL.
 */
//==============================================================================
libmqtt_t *libmqtt__new(const libmqtt_conf_t *conf)
{
        if (not conf or not conf->socket_recv or not conf->socket_send or not conf->rx_buf_size or
                not conf->tx_buf_size or not conf->out_buf_size or not conf->get_time_ms or
                not conf->max_subscribe_topics or not conf->keep_alive) {
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
        self->packet_id_ctr = 1;
        self->keep_alive_ms = (conf->keep_alive * 1000);

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
 * @return On success 0 is returned.
 */
//==============================================================================
int libmqtt__connect(libmqtt_t *self, const libmqtt_connect_params_t *conn_params)
{
        if (not self or not conn_params or not conn_params->client_id) {
                return -EINVAL;
        }

        bool last_will = (conn_params->will_topic and conn_params->will_msg);

        u16_t client_id_len = strlen(conn_params->client_id);
        u16_t will_topic_len = last_will ? strlen(conn_params->will_topic) : 0;
        u16_t will_msg_len = last_will ? strlen(conn_params->will_msg) : 0;
        u16_t user_name_len = conn_params->user_name ? strlen(conn_params->user_name) : 0;
        u16_t password_len = conn_params->password ? strlen(conn_params->password) : 0;

        if ((sizeof(u16_t) + client_id_len) + (sizeof(u16_t) + will_topic_len) +
                        (sizeof(u16_t) + will_msg_len) + (sizeof(u16_t) + user_name_len) +
                        (sizeof(u16_t) + password_len) >
                self->tx_buf.size) {
                return -ENOSPC;
        }

        u16_t payload_len = 0;
        u8_t *payload = self->tx_buf.buf;

        payload_len += copy_mqtt_string(&payload[payload_len], conn_params->client_id, client_id_len);
        payload_len += copy_mqtt_string(&payload[payload_len], conn_params->will_topic, will_topic_len);
        payload_len += copy_mqtt_string(&payload[payload_len], conn_params->will_msg, will_msg_len);
        payload_len += copy_mqtt_string(&payload[payload_len], conn_params->user_name, user_name_len);
        payload_len += copy_mqtt_string(&payload[payload_len], conn_params->password, password_len);

        variable_packet_connect_t variable_header = {
                .protocol_name = {0, 4, 'M', 'Q', 'T', 'T'},
                .protocol_level = 4, // MQTT 3.1.1
                .connect_flags = (conn_params->user_name ? mqtt_connect_flag__user_name : 0) |
                        (conn_params->password ? mqtt_connect_flag__password : 0) |
                        (conn_params->will_topic ? mqtt_connect_flag__will_flag : 0) |
                        (conn_params->will_topic ? mqtt_connect_flag__will_retain : 0) |
                        (conn_params->will_topic ? mqtt_connect_flag__will_qos_0 : 0) |
                        mqtt_connect_flag__clean_start,
                .keep_alive = ntohu16(self->keep_alive_ms / 1000),
        };

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__connect;
        packet.fixed_header_flags = 0;
        packet.variable_header = &variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = payload_len;

        if (session_serialize_packet(self, &packet) != 0) {
                return -EIO;
        }

        i64_t tref = self->conf.get_time_ms();

        self->server_response_tref = tref;

        while (true) {
                if (self->conf.get_time_ms() - tref >= self->conf.timeout_ms) {
                        return -ETIME;
                }

                mqtt_msg_type_t packet_type;
                int n = loop(self, &packet_type);
                if (n < 0) {
                        return n;

                } else if (n == 0) {
                        continue;

                } else if (n > 0) {
                        if (packet_type == mqtt_msg_type__connack) {
                                return self->rx_packet.variable_header_and_payload[1];
                        }
                }
        }

        return 0;
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

        if (session_serialize_packet(self, &packet) != 0) {
                return -EIO;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Subscribe topics.
 *
 * @param  self         instance
 * @param  topic        topic array
 * @param  topics       topics in array
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int libmqtt__subscribe(libmqtt_t *self, const libmqtt_subscribe_t *topic, size_t topics)
{
        if (not self or not topic or not topics) {
                return -EINVAL;
        }

        u16_t payload_len = 0;
        for (size_t i = 0; i < topics; i++) {
                if (topic[i].topic and topic[i].callback and (topic[i].qos <= libmqtt_qos__QOS2)) {
                        payload_len += sizeof(u16_t) + strlen(topic[i].topic) + sizeof(u8_t /*qos*/);
                } else {
                        return -EINVAL;
                }
        }

        if (payload_len > self->tx_buf.size) {
                return -ENOSPC;
        }

        u16_t packet_id = get_packet_id(self);
        u16_t variable_header = ntohu16(packet_id);

        payload_len = 0;
        u8_t *payload = self->tx_buf.buf;

        for (size_t i = 0; i < topics; i++) {
                if (topic[i].topic and topic[i].callback and (topic[i].qos <= libmqtt_qos__QOS2)) {
                        payload_len += copy_mqtt_string(&payload[payload_len], topic[i].topic,
                                strlen(topic[i].topic));
                        payload[payload_len++] = topic[i].qos;
                }
        }

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__subscribe;
        packet.fixed_header_flags = 0x02;
        packet.variable_header = &variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = payload_len;

        if (session_serialize_packet(self, &packet) != 0) {
                return -ECONNABORTED;
        }

        i64_t tref = self->conf.get_time_ms();

        while (true) {
                if (self->conf.get_time_ms() - tref >= self->conf.timeout_ms) {
                        return -ETIME;
                }

                mqtt_msg_type_t packet_type;
                int n = loop(self, &packet_type);
                if (n < 0) {
                        return n;

                } else if (n == 0) {
                        continue;

                } else if (n > 0) {
                        if (packet_type == mqtt_msg_type__suback) {
                                u16_t packet_id_ans = (self->rx_packet.variable_header_and_payload[0] << 8) |
                                        self->rx_packet.variable_header_and_payload[1];

                                if (packet_id_ans == packet_id) {
                                        size_t i = 0;
                                        for (size_t t = 0; t < topics; t++) {
                                                for (; i < self->conf.max_subscribe_topics; i++) {
                                                        if (self->subfilter[i].topic) {
                                                                continue;
                                                        }

                                                        self->subfilter[i].topic = strdup(topic[t].topic);
                                                        if (not self->subfilter[i].topic) {
                                                                VERBOSE_SESSION_WARNING(
                                                                        "string allocation error.");
                                                                continue;
                                                        }

                                                        self->subfilter[i].qos = topic[t].qos;
                                                        self->subfilter[i].callback = topic[t].callback;
                                                        self->subfilter[i].context = topic[t].context;

                                                        break;
                                                }
                                        }
                                        break;
                                }
                        }
                }
        }

        return 0;
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

        u16_t payload_len = 0;
        for (size_t i = 0; i < topics; i++) {
                if (topic[i]) {
                        payload_len += sizeof(u16_t) + strlen(topic[i]);
                } else {
                        return -EINVAL;
                }
        }

        if (payload_len > self->tx_buf.size) {
                return -ENOSPC;
        }

        u16_t packet_id = get_packet_id(self);
        u16_t variable_header = ntohu16(packet_id);

        payload_len = 0;
        u8_t *payload = self->tx_buf.buf;

        for (size_t i = 0; i < topics; i++) {
                if (topic[i]) {
                        payload_len += copy_mqtt_string(&payload[payload_len], topic[i], strlen(topic[i]));
                }
        }

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__unsubscribe;
        packet.fixed_header_flags = 0x02;
        packet.variable_header = &variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = payload;
        packet.payload_length = payload_len;

        if (session_serialize_packet(self, &packet) != 0) {
                return -ECONNABORTED;
        }

        i64_t tref = self->conf.get_time_ms();

        while (true) {
                if (self->conf.get_time_ms() - tref >= self->conf.timeout_ms) {
                        return -ETIME;
                }

                mqtt_msg_type_t packet_type;
                int n = loop(self, &packet_type);
                if (n < 0) {
                        return n;

                } else if (n == 0) {
                        continue;

                } else if (n > 0) {
                        if (packet_type == mqtt_msg_type__unsuback) {
                                u16_t packet_id_ans = (self->rx_packet.variable_header_and_payload[0] << 8) |
                                        self->rx_packet.variable_header_and_payload[1];

                                if (packet_id_ans == packet_id) {
                                        for (size_t t = 0; t < topics; t++) {
                                                for (size_t i = 0; i < self->conf.max_subscribe_topics; i++) {
                                                        if (not self->subfilter[i].topic) {
                                                                continue;
                                                        }

                                                        if (not isstreq(self->subfilter[i].topic, topic[t])) {
                                                                continue;
                                                        }

                                                        self->subfilter[i].qos = 0;
                                                        self->subfilter[i].callback = NULL;
                                                        free(self->subfilter[i].topic);
                                                        self->subfilter[i].topic = NULL;

                                                        break;
                                                }
                                        }
                                        break;
                                }
                        }
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
int libmqtt__publish(libmqtt_t *self, libmqtt_publish_t *publish)
{
        if (not self or not publish or not publish->topic or not publish->payload or
                not publish->payload_len or (publish->qos > libmqtt_qos__QOS2)) {
                return -EINVAL;
        }

        if (sizeof(u16_t) + strlen(publish->topic) + sizeof(u16_t) > self->tx_buf.size) {
                return -ENOSPC;
        }

        static u8_t QOS[] = {
                [libmqtt_qos__QOS0] = mqtt_publish_flag__qos_0,
                [libmqtt_qos__QOS1] = mqtt_publish_flag__qos_1,
                [libmqtt_qos__QOS2] = mqtt_publish_flag__qos_2,
        };

        size_t variable_header_len = 0;
        u8_t *variable_header = self->tx_buf.buf;
        variable_header_len += copy_mqtt_string(&variable_header[variable_header_len], publish->topic,
                strlen(publish->topic));

        u16_t packet_id = get_packet_id(self);

        if (publish->qos >= libmqtt_qos__QOS1) {
                variable_header[variable_header_len++] = (packet_id >> 8);
                variable_header[variable_header_len++] = (packet_id >> 0);
        }

        mqtt_control_packet_t packet = {0};
        packet.type = mqtt_msg_type__publish;
        packet.fixed_header_flags = QOS[publish->qos] | (publish->retain ? mqtt_publish_flag__retain : 0);
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = publish->payload;
        packet.payload_length = publish->payload_len;

        if (session_serialize_packet(self, &packet) != 0) {
                return -EIO;
        }

        i64_t tref = self->conf.get_time_ms();

        if (publish->qos > libmqtt_qos__QOS0) {
                while (true) {
                        if (self->conf.get_time_ms() - tref >= self->conf.timeout_ms) {
                                return -ETIME;
                        }

                        mqtt_msg_type_t packet_type;
                        int n = loop(self, &packet_type);
                        if (n < 0) {
                                return -EIO;

                        } else if (n == 0) {
                                continue;

                        } else if (n > 0) {
                                u8_t *variable_header = self->rx_packet.variable_header_and_payload;

                                if ((publish->qos == libmqtt_qos__QOS1) and
                                        (packet_type == mqtt_msg_type__puback)) {
                                        u16_t ack_packet_id = (variable_header[0] << 8) | variable_header[1];
                                        if (ack_packet_id == packet_id) {
                                                break;
                                        }

                                } else if ((publish->qos == libmqtt_qos__QOS2) and
                                        (packet_type == mqtt_msg_type__pubcomp)) {
                                        u16_t ack_packet_id = (variable_header[0] << 8) | variable_header[1];
                                        if (ack_packet_id == packet_id) {
                                                break;
                                        }
                                }
                        }
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  MQTT single loop.
 *
 * @param  self         instance
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int libmqtt__loop_once(libmqtt_t *self)
{
        int n = loop(self, NULL);
        return (n >= 0) ? 0 : n;
}

/*==============================================================================
  End of file
==============================================================================*/
