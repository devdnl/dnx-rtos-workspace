/*==============================================================================
@file   mqtt_broker.c

@author Daniel Zorychta

@brief  MQTT broker.

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

#pragma GCC optimize("Ofast")

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <llist/llist.h>
#include <dnx/misc.h>
#include <dnx/net.h>
#include <dnx/thread.h>
#include <sys/shm.h>
#include <unistd.h>
#include <ctype.h>
#include <netmgr/shared.h>
#include <dnx/os.h>
#include <time.h>
#include <llist.h>
#include <conf_parser.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define DEBUG_CMD_ENABLE         false

#define CONFIG_KEY_USER          "user"
#define CONFIG_KEY_PASSWORD      "password"
#define CONFIG_KEY_PORT          "port"
#define CONFIG_KEY_DEBUG         "debug"
#define CONFIG_KEY_ALLOW_ENABLE  "allow.enable"
#define CONFIG_KEY_ALLOW_BASE    "allow."
#define CONFIG_KEY_STATS_PATH    "runtime_stats_path"

#define MAX_TOPIC_LENGTH         128
#define MAX_TOPICS               500
#define MAX_PAYLOAD_SIZE         4096
#define MAX_RETAIN_TIME_ms       (24 * 60 * 60 * 1000UL)
#define TOPICDB_MAINTENANCE_ms   (60 * 1000UL)
#define CONNECT_TIMEOUT_ms       (10 * 1000UL)
#define MAX_QOS2_INFLIGHT        8
#define MAX_CLIENT_ID_LENGTH     64

#define SAFE_STR(s)              ((s) ? (s) : "")

#define DEBUG_SESSION(...)                                                                      \
        do {                                                                                    \
                if (global->debug) {                                                            \
                        time_t t = time(NULL);                                                  \
                        struct tm lt;                                                           \
                        localtime_r(&t, &lt);                                                   \
                        char t_str[32];                                                         \
                        strftime(t_str, sizeof(t_str), "%F %X", &lt);                           \
                        printf("[mqtt_broker, debug, %s, %s] ", t_str, session->client_ip_str); \
                        printf(__VA_ARGS__);                                                    \
                }                                                                               \
        } while (false)

#define DEBUG_SESSION_ERROR()                                                                            \
        do {                                                                                             \
                time_t t = time(NULL);                                                                   \
                struct tm lt;                                                                            \
                localtime_r(&t, &lt);                                                                    \
                char t_str[32];                                                                          \
                strftime(t_str, sizeof(t_str), "%F %X", &lt);                                            \
                printf("[mqtt_broker, error, %s, %s] %s: %u\n", t_str, session->client_ip_str, __func__, \
                        __LINE__);                                                                       \
        } while (false)

#define VERBOSE_SESSION(...)                                                           \
        do {                                                                           \
                time_t t = time(NULL);                                                 \
                struct tm lt;                                                          \
                localtime_r(&t, &lt);                                                  \
                char t_str[32];                                                        \
                strftime(t_str, sizeof(t_str), "%F %X", &lt);                          \
                printf("[mqtt_broker, info, %s, %s] ", t_str, session->client_ip_str); \
                printf(__VA_ARGS__);                                                   \
        } while (false)

#define VERBOSE_SESSION_WARNING(...)                                                      \
        do {                                                                              \
                time_t t = time(NULL);                                                    \
                struct tm lt;                                                             \
                localtime_r(&t, &lt);                                                     \
                char t_str[32];                                                           \
                strftime(t_str, sizeof(t_str), "%F %X", &lt);                             \
                printf("[mqtt_broker, warning, %s, %s] ", t_str, session->client_ip_str); \
                printf(__VA_ARGS__);                                                      \
        } while (false)

#define VERBOSE(...)                                          \
        do {                                                  \
                time_t t = time(NULL);                        \
                struct tm lt;                                 \
                localtime_r(&t, &lt);                         \
                char t_str[32];                               \
                strftime(t_str, sizeof(t_str), "%F %X", &lt); \
                printf("[mqtt_broker, info, %s] ", t_str);    \
                printf(__VA_ARGS__);                          \
        } while (false)

#if DEBUG_CMD_ENABLE > 0
#define DEBUG_CMD(...) VERBOSE(__VA_ARGS__)
#else
#define DEBUG_CMD(...)
#endif

#define mqtt_fixed_header_flags_mask                (0x0F)

#define mqtt_connect_flag__user_name                (1 << 7)
#define mqtt_connect_flag__password                 (1 << 6)
#define mqtt_connect_flag__will_retain              (1 << 5)
#define mqtt_connect_flag__will_qos_mask            (3 << 3)
#define mqtt_connect_flag__will_qos_0               (0 << 3)
#define mqtt_connect_flag__will_qos_1               (1 << 3)
#define mqtt_connect_flag__will_qos_2               (2 << 3)
#define mqtt_connect_flag__will_flag                (1 << 2)
#define mqtt_connect_flag__clean_start              (1 << 1)
#define mqtt_connect_flag__reserved                 (1 << 0)

#define mqtt_connack_session_present                (1 << 0)

#define mqtt_publish_flag__duplicated               (1 << 3)
#define mqtt_publish_flag__qos_mask                 (3 << 1)
#define mqtt_publish_flag__qos_pos                  1
#define mqtt_publish_flag__qos_0                    (0 << 1)
#define mqtt_publish_flag__qos_1                    (1 << 1)
#define mqtt_publish_flag__qos_2                    (2 << 1)
#define mqtt_publish_flag__retain                   (1 << 0)

#define mqtt_subscribe_flag__retain_handling_mask   (3 << 4)
#define mqtt_subscribe_flag__retain_handling_always (0 << 4)
#define mqtt_subscribe_flag__retain_handling_avoid  (1 << 4)
#define mqtt_subscribe_flag__retain_handling_new    (2 << 4)
#define mqtt_subscribe_flag__retain_as_published    (1 << 3)
#define mqtt_subscribe_flag__no_local               (1 << 2)
#define mqtt_subscribe_flag__qos_mask               (3 << 0)
#define mqtt_subscribe_flag__qos_0                  (0 << 0)
#define mqtt_subscribe_flag__qos_1                  (1 << 0)
#define mqtt_subscribe_flag__qos_2                  (2 << 0)
#define mqtt_subscribe_flag__v3_reserved_mask       (0xFC)
#define mqtt_subscribe_flag__v5_reserved_mask       (0xC0)

#define mqtt_v3_connack__accepted                   0x00
#define mqtt_v3_connack__unacceptable_protocol      0x01
#define mqtt_v3_connack__identifier_rejected        0x02
#define mqtt_v3_connack__server_unavailable         0x03
#define mqtt_v3_connack__bad_user_name_or_password  0x04
#define mqtt_v3_connack__not_authorized             0x05
#define mqtt_v3_suback__failure                     0x80

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

typedef enum {
        mqtt_reason__success = 0x00,
        mqtt_reason__normal_disconnection = 0x00,
        mqtt_reason__granted_qos_0 = 0x00,
        mqtt_reason__granted_qos_1 = 0x01,
        mqtt_reason__granted_qos_2 = 0x02,
        mqtt_reason__disconnect_with_will_message = 0x04,
        mqtt_reason__no_matching_subscribers = 0x10,
        mqtt_reason__no_subscription_existed = 0x11,
        mqtt_reason__continue_authentication = 0x18,
        mqtt_reason__re_authenticate = 0x19,
        mqtt_reason__unspecified_error = 0x80,
        mqtt_reason__malformed_packet = 0x81,
        mqtt_reason__protocol_error = 0x82,
        mqtt_reason__implementation_specific_error = 0x83,
        mqtt_reason__unsupported_protocol_version = 0x84,
        mqtt_reason__client_identifier_not_valid = 0x85,
        mqtt_reason__bad_user_name_or_password = 0x86,
        mqtt_reason__not_authorized = 0x87,
        mqtt_reason__server_unavailable = 0x88,
        mqtt_reason__server_busy = 0x89,
        mqtt_reason__banned = 0x8a,
        mqtt_reason__server_shutting_down = 0x8b,
        mqtt_reason__bad_authentication_method = 0x8c,
        mqtt_reason__keep_alive_timeout = 0x8d,
        mqtt_reason__session_taken_over = 0x8e,
        mqtt_reason__topic_filter_invalid = 0x8f,
        mqtt_reason__topic_name_invalid = 0x90,
        mqtt_reason__packet_identifier_in_use = 0x91,
        mqtt_reason__packet_identifier_not_found = 0x92,
        mqtt_reason__receive_maximum_exceeded = 0x93,
        mqtt_reason__topic_alias_invalid = 0x94,
        mqtt_reason__packet_too_large = 0x95,
        mqtt_reason__message_rate_too_high = 0x96,
        mqtt_reason__quota_exceeded = 0x97,
        mqtt_reason__administrative_action = 0x98,
        mqtt_reason__payload_format_invalid = 0x99,
        mqtt_reason__retain_not_supported = 0x9a,
        mqtt_reason__qos_not_supported = 0x9b,
        mqtt_reason__use_another_server = 0x9c,
        mqtt_reason__server_moved = 0x9d,
        mqtt_reason__shared_subscriptions_not_supported = 0x9e,
        mqtt_reason__connection_rate_exceeded = 0x9f,
        mqtt_reason__maximum_connect_time = 0xa0,
        mqtt_reason__subscription_identifiers_not_supported = 0xa1,
        mqtt_reason__wildcard_subscriptions_not_supported = 0xa2,
} mqtt_reason_t;

typedef enum {
        mqtt_property__payload_format_indicator = 0x01,
        mqtt_property__message_expiry_interval = 0x02,
        mqtt_property__content_type = 0x03,
        mqtt_property__response_topic = 0x08,
        mqtt_property__correlation_data = 0x09,
        mqtt_property__subscription_identifier = 0x0b,
        mqtt_property__session_expiry_interval = 0x11,
        mqtt_property__assigned_client_identifier = 0x12,
        mqtt_property__server_keep_alive = 0x13,
        mqtt_property__authentication_method = 0x15,
        mqtt_property__authentication_data = 0x16,
        mqtt_property__request_problem_information = 0x17,
        mqtt_property__will_delay_interval = 0x18,
        mqtt_property__request_response_information = 0x19,
        mqtt_property__response_information = 0x1a,
        mqtt_property__server_reference = 0x1c,
        mqtt_property__reason_string = 0x1f,
        mqtt_property__receive_maximum = 0x21,
        mqtt_property__topic_alias_maximum = 0x22,
        mqtt_property__topic_alias = 0x23,
        mqtt_property__maximum_qos = 0x24,
        mqtt_property__retain_available = 0x25,
        mqtt_property__user_property = 0x26,
        mqtt_property__maximum_packet_size = 0x27,
        mqtt_property__wildcard_subscription_available = 0x28,
        mqtt_property__subscription_identifier_available = 0x29,
        mqtt_property__shared_subscription_available = 0x2a,
} mqtt_property_t;

typedef enum {
        mqtt_protocol_version__3p1,
        mqtt_protocol_version__3p1p1,
        mqtt_protocol_version__5p0,
} mqtt_protocol_version_t;

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

typedef struct {
        uint8_t buf[256];
        size_t seek;
        size_t size;
} fetchbuf_t;

typedef struct {
        uint8_t buf[256];
        size_t seek;
} pushbuf_t;

typedef enum {
        packet_step__fixed_header,
        packet_step__remaining_length,
        packet_step__variable_header_and_payload,
} packet_step_t;

typedef struct {
        packet_step_t step;
        u8_t remaining_length_part;
        uint32_t remaining_length;
        uint32_t received_length;
        uint32_t payload_read_index;
        mqtt_fixed_header_t fixed_header;
        u8_t *variable_header_and_payload;
        bool variable_header_and_payload_allocated;
        u8_t buf[256];
} packet_t;

typedef struct {
        char *topic;

        /* last published message (delivered to current subscribers) */
        uint8_t *msg;
        uint16_t msg_length;
        uint16_t msg_alloc_length;
        uint32_t msg_seq;               // incremented on each publication
        uint32_t msg_publisher_id;      // session ID of publisher (No Local option)
        uint8_t msg_qos;
        bool msg_retain;

        /* retained message (delivered to new subscribers) */
        uint8_t *retained_msg;
        uint16_t retained_length;
        uint8_t retained_qos;
        bool has_retained;

        clock_t msg_update_tref;
        uint32_t subscribers;
} topicdb_t;

typedef struct {
        topicdb_t *topicdb_ref;
        uint32_t msg_seq_sent;          // last delivered msg_seq
        uint8_t flags;
        bool send_retained;             // retained message waits for delivery
} topicsub_t;

typedef struct {
        bool topic_alias;
        bool authentication_method;
} mqtt_properties_t;

typedef struct {
        SOCKET *socket;
        char *client_id;
        char *user_name;
        char *password;
        llist_t *subscribtion_list;
        char client_ip_str[16];
        NET_INET_IPv4_t client_ip;
        clock_t alive_tref;
        packet_t packet;
        fetchbuf_t fetch_buf;
        pushbuf_t push_buf;
        mqtt_protocol_version_t protocol_version;
        uint32_t id;
        uint32_t publish_ctr;
        int32_t session_expiry_interval;
        uint16_t keep_alive;
        int8_t connect_flags;
        bool connected;
        bool close_session;
        clock_t connection_tref;

        struct {
                uint16_t id[MAX_QOS2_INFLIGHT];
                uint8_t count;
        } qos2_rx;

        struct {
                char *topic;
                char *payload;
                uint16_t payload_length;
                uint8_t qos;
                bool retain;
        } last_will;
} session_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static uint16_t ntohu16(uint16_t value);
static uint32_t ntohu32(uint32_t value);
static void runtime_stats(void);
static session_t *session_new(SOCKET *socket);
static session_t *session_delete(session_t *session);
static void llist_session_dtor(void *object);
static bool session_poll(session_t *session);
static int session_receive_packet(session_t *session);
static void session_send_last_will(session_t *session);
static int session_fetchbuf_read(session_t *session, void *dst, size_t count);
static int session_pushbuf_write(session_t *session, const void *src, size_t count);
static int session_pushbuf_flush(session_t *session);
static int session_fetch_variable_length(session_t *session, uint32_t *length);
static int session_fetch_skip(session_t *session, size_t skip_bytes);
static int session_fetch_i8(session_t *session, int8_t *val);
static int session_fetch_i16(session_t *session, int16_t *val);
static int session_fetch_i32(session_t *session, int32_t *val);
static int session_fetch_u16(session_t *session, uint16_t *val);
static int session_fetch_string_alloc(session_t *session, size_t len, char **string);
static bool session_is_v5(const session_t *session);
static bool str_equal(const char *a, const char *b);
static bool topic_name_valid(const char *topic);
static bool session_send_disconnect(session_t *session, mqtt_reason_t reason_code);
static int session_push_i8(session_t *session, int8_t val);
static int session_push_variable_length(session_t *session, int32_t vlength);
static int session_push_buffer(session_t *session, const void *buf, size_t count);
static bool session_cmd_connect(session_t *session);
static bool session_resp_connack(session_t *session, mqtt_reason_t reason_code, bool client_id_assigned);
static bool session_cmd_pingreq(session_t *session);
static bool session_resp_pingresp(session_t *session);
static bool session_cmd_disconnect(session_t *session);
static bool session_cmd_publish(session_t *session);
static bool session_resp_puback(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier);
static bool session_resp_pubrec(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier);
static bool session_cmd_pubrel(session_t *session);
static bool session_cmd_puback(session_t *session);
static bool session_resp_pubcomp(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier);
static bool session_cmd_pubrec(session_t *session);
static bool session_cmd_pubcomp(session_t *session);
static bool session_resp_pubrel(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier);
static bool session_cmd_subscribe(session_t *session);
static bool session_cmd_unsubscribe(session_t *session);
static int session_serialize_packet(session_t *session, const mqtt_control_packet_t *packet);
static int session_interpret_properties(session_t *session, size_t property_length, mqtt_properties_t *props);
static int session_fetch_properties(session_t *session, mqtt_properties_t *props);
static bool session_send_subscribed_topics(session_t *session);
static bool session_resp_suback(session_t *session, const uint8_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier);
static bool session_resp_unsuback(session_t *session, const uint8_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier);
static mqtt_reason_t session_publish_topic(session_t *session, uint8_t qos, bool retain, const char *topic,
        const void *payload, size_t payload_length);

static mqtt_reason_t topicsub_subscribe(session_t *session, const char *topic, u8_t flags);
static mqtt_reason_t topicsub_unsubscribe(session_t *session, const char *topic);
static int topicsub_compare(const void *a, const void *b);
static void topicsub_destroy(void *a);

static int topicdb_compare(const void *a, const void *b);
static void topicdb_destroy(void *a);
static void topicdb_maintenance(void);
static mqtt_reason_t topicdb_update_topic(const char *topic, const void *msg_ref, size_t msg_len, uint8_t qos,
        bool retain, uint32_t publisher_id, topicdb_t **topicdb_ref, bool create_only);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        conf_parser_t *conf;
        bool debug;
        bool allow_list_enabled;
        uint32_t session_id_ctr;

        struct {
                clock_t refresh;
                const char *path;
        } runtime_stats;

        struct {
                llist_t *data;
                uint32_t publish_ctr;
                uint16_t msg_id_ctr;
                clock_t last_maintenance_tref;
        } topic_db;

        llist_t *sessions;
};

static const char *config_param = "--config=";

/*==============================================================================
  Exported objects
==============================================================================*/
PROGRAM_PARAMS(mqtt_broker, STACK_DEPTH_MEDIUM);

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Help message.
 *
 * @param  argv         arguemnts
 */
//==============================================================================
static void print_help(char *argv[])
{
        printf("Usage: %s args\n", argv[0]);
        printf("  -h, --help            this help\n");
        printf("  --config=<file>       configuration file\n");
        printf("  --wait-for-network    wait for network to be available\n");
        printf("\n");

        printf("Example configuration:\n");
        printf("user = \"user\"\n");
        printf("password = \"password\"\n");
        printf("port = 1883\n");
        printf("debug = false\n");
        printf("\n");
        printf("[allow]\n");
        printf("enable = false\n");
        printf("client_id1 = false\n");
        printf("client_id2 = false\n");
}

//==============================================================================
/**
 * Main program function.
 *
 * @param argc      argument count
 * @param argv      arguments
 *
 * @return On succes 0.
 */
//==============================================================================
int main(int argc, char *argv[])
{
        SOCKET *socket = NULL;
        char *conf_path = NULL;
        bool wait_for_network = false;
        int exit_code = EXIT_FAILURE;

        if (argc < 2) {
                print_help(argv);
                goto cleanup;
        }

        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "-h") or isstreq(argv[i], "--help")) {
                        print_help(argv);
                        goto cleanup;

                } else if (isstreq(argv[i], "--wait-for-network")) {
                        wait_for_network = true;

                } else if (isstreqn(argv[i], config_param, strlen(config_param))) {
                        conf_path = argv[i] + strlen(config_param);
                }
        }

        if (not conf_path) {
                printf("[%s, info] no configuration file given!\n", argv[0]);
                goto cleanup;
        }

        global->conf = conf_parser__new(conf_path);
        if (not global->conf) {
                printf("[%s, info] no such file '%s'!\n", argv[0], conf_path);
                goto cleanup;
        }

        global->debug = conf_parser__is_value_true(conf_parser__get_value(global->conf, CONFIG_KEY_DEBUG));
        if (global->debug) {
                printf("[%s, info] debug enabled.\n", argv[0]);
        }

        global->allow_list_enabled = conf_parser__is_value_true(
                conf_parser__get_value(global->conf, CONFIG_KEY_ALLOW_ENABLE));
        if (global->allow_list_enabled) {
                printf("[%s, info] allowing connections only from allow list.\n", argv[0]);
        }

        global->runtime_stats.path = conf_parser__get_value(global->conf, CONFIG_KEY_STATS_PATH);
        if (global->runtime_stats.path) {
                printf("[%s, info] enabled runtime stats.\n", argv[0]);
        }

        global->topic_db.data = llist_new(topicdb_compare, topicdb_destroy);
        if (not global->topic_db.data) {
                printf("[%s, info] unable to create topic database list!\n", argv[0]);
                goto cleanup;
        }

        global->sessions = llist_new(NULL, llist_session_dtor);
        if (not global->sessions) {
                printf("[%s, info] unable to create session list!\n", argv[0]);
                goto cleanup;
        }

        if (wait_for_network) {
                printf("[%s, info] waiting for network to be available...\n", argv[0]);
        }

        while (wait_for_network) {
                sleep(1);

                NET_family_t family;
                NET_INET_status_t status;
                if (ifstatus("inet", &family, &status, sizeof(status)) == 0) {

                        if ((status.state == NET_INET_STATE__DHCP_CONFIGURED) or
                                (status.state == NET_INET_STATE__STATIC_IP)) {

                                break;
                        }
                }
        }

        const char *port_str = conf_parser__get_value(global->conf, CONFIG_KEY_PORT);
        int port = port_str ? atoi(port_str) : 1883;
        if ((port <= 0) or (port > 65535)) {
                printf("[%s, info] invalid port number!\n", argv[0]);
                goto cleanup;
        }

        if (not conf_parser__get_value(global->conf, CONFIG_KEY_USER) and
                not conf_parser__get_value(global->conf, CONFIG_KEY_PASSWORD)) {
                printf("[%s, info] no user and password configured, authentication disabled!\n", argv[0]);
        }

        errno = 0;
        socket = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
        if (not socket) {
                printf("[%s, info] unable to create socket: %s\n", argv[0], strerror(errno));
                goto cleanup;
        }

        const NET_INET_sockaddr_t ADDR_ANY = {
                .addr = NET_INET_IPv4_ANY,
                .port = port,
        };

        errno = 0;
        if (socket_bind(socket, &ADDR_ANY, sizeof(ADDR_ANY)) != 0) {
                printf("[%s, info] unable to bind socket: %s\n", argv[0], strerror(errno));
                goto cleanup;
        }

        errno = 0;
        if (socket_listen(socket) != 0) {
                printf("[%s, info] unable to listen socket: %s\n", argv[0], strerror(errno));
                goto cleanup;
        }

        printf("[%s, info] listening connections at port %u.\n", argv[0], ADDR_ANY.port);

        socket_set_recv_timeout(socket, 10);

        while (true) {
                size_t last_num_clients = llist_size(global->sessions);

                SOCKET *new_socket;
                if (socket_accept(socket, &new_socket) == 0) {
                        session_t *session = session_new(new_socket);
                        if (session) {
                                llist_push_back(global->sessions, session);
                        } else {
                                socket_close(new_socket);
                        }
                }

                llist_foreach (session_t *, session, global->sessions) {
                        if (not session_poll(session)) {
                                llist_erase_by_iterator(&_iterator);
                        }
                }

                size_t num_clients = llist_size(global->sessions);
                if (num_clients != last_num_clients) {
                        last_num_clients = num_clients;
                        VERBOSE("connected clients: %u.\n", llist_size(global->sessions));
                }

                runtime_stats();

                topicdb_maintenance();
        }

cleanup:
        if (socket) {
                socket_close(socket);
        }

        llist_delete(global->sessions);         // sessions first: subscriptions refer to topic DB
        llist_delete(global->topic_db.data);
        conf_parser__delete(global->conf);

        return exit_code;
}

//==============================================================================
/**
 * @brief  Create runtime stats file.
 */
//==============================================================================
static void runtime_stats(void)
{
        clock_t now = get_uptime_ms();

        if (not global->runtime_stats.path) {
                return;
        }

        if (now - global->runtime_stats.refresh < 5000) {
                return;
        }

        global->runtime_stats.refresh = now;

        size_t buf_len = 2048;
        char *buf = malloc(buf_len);
        if (not buf) {
                return;
        }

        scope (FILE *fp = fopen(global->runtime_stats.path, "w"), fp, fclose(fp)) {

                size_t n = 0;
                size_t i = 1;

                n += snprintf(&buf[n], buf_len - n, "Currently connected clients:\n");

                llist_foreach (session_t *, s, global->sessions) {

                        if (n >= (buf_len - 1)) {
                                n = buf_len - 1;
                                break;
                        }

                        static const char *mqtt_ver_str[] = {
                                [mqtt_protocol_version__3p1] = "v3.1  ",
                                [mqtt_protocol_version__3p1p1] = "v3.1.1",
                                [mqtt_protocol_version__5p0] = "v5.0  ",
                        };

                        u32_t uptime = (now - s->connection_tref) / 1000;

                        char ip_str_aligned[16];
                        strlcpy(ip_str_aligned, "               ", sizeof(ip_str_aligned));
                        memcpy(ip_str_aligned, s->client_ip_str, min(strlen(s->client_ip_str), sizeof(ip_str_aligned) - 1));

                        n += snprintf(&buf[n], buf_len - n, "%u: %4u:%02u:%02u %s %s '%s'\n", i++,
                                uptime / 3600, (uptime / 60) % 60, uptime % 60,
                                mqtt_ver_str[s->protocol_version], ip_str_aligned, SAFE_STR(s->client_id));
                }

                n = min(n, buf_len - 1);

                fwrite(buf, 1, n, fp);
        }

        free(buf);
}

//==============================================================================
/**
 * @brief  Create new session.
 *
 * @param  socket       socket to use by session
 *
 * @return Session object, or NULl on error.
 */
//==============================================================================
static session_t *session_new(SOCKET *socket)
{
        session_t *session = calloc(1, sizeof(*session));
        if (not session) {
                return NULL;
        }

        session->subscribtion_list = llist_new(topicsub_compare, topicsub_destroy);
        if (not session->subscribtion_list) {
                return session_delete(session);
        }

        session->socket = socket;
        session->alive_tref = get_uptime_ms();
        session->keep_alive = 0;
        session->id = ++global->session_id_ctr;

        socket_set_recv_timeout(session->socket, 0);
        socket_set_send_timeout(session->socket, 1000);

        NET_INET_sockaddr_t addr;
        socket_get_address(session->socket, &addr, sizeof(addr));
        session->client_ip = addr.addr;

        session->connection_tref = get_uptime_ms();

        snprintf(session->client_ip_str, sizeof(session->client_ip_str), "%u.%u.%u.%u",
                NET_INET_IPv4_a(addr.addr), NET_INET_IPv4_b(addr.addr), NET_INET_IPv4_c(addr.addr),
                NET_INET_IPv4_d(addr.addr));

        VERBOSE_SESSION("connecting new client...\n");

        return session;
}

//==============================================================================
/**
 * @brief  Delete session.
 *
 * @param  session      session object
 *
 * @return NULL.
 */
//==============================================================================
static session_t *session_delete(session_t *session)
{
        if (not session) {
                return NULL;
        }

        if (session->socket) {
                socket_disconnect(session->socket);
                socket_close(session->socket);
        }

        llist_delete(session->subscribtion_list);

        VERBOSE_SESSION("client '%s' session closed.\n", SAFE_STR(session->client_id));

        if (session->packet.variable_header_and_payload_allocated) {
                free(session->packet.variable_header_and_payload);
        }

        free(session->client_id);
        free(session->user_name);
        free(session->password);
        free(session->last_will.topic);
        free(session->last_will.payload);

        memset(session, 0, sizeof(*session));
        free(session);

        return NULL;
}

//==============================================================================
/**
 * @brief  Session descructor called from linked list.
 *
 * @param  object       session object
 */
//==============================================================================
static void llist_session_dtor(void *object)
{
        session_delete(object);
}

//==============================================================================
/**
 * @brief  Session polling function.
 *
 * @param  session      session
 *
 * @return True on success, otherwise false.
 */
//==============================================================================
static bool session_poll(session_t *session)
{
        clock_t now = get_uptime_ms();

        if (session->close_session) {
                VERBOSE_SESSION_WARNING("closing session because of double connection.\n");
                session_send_disconnect(session, mqtt_reason__session_taken_over);
                session_send_last_will(session);
                return false;
        }

        if (not session->connected) {
                if ((now - session->alive_tref) >= CONNECT_TIMEOUT_ms) {
                        VERBOSE_SESSION_WARNING("closing session because of CONNECT timeout.\n");
                        return false;
                }

        } else if (session->keep_alive > 0) {
                // MQTT-3.1.2-24: disconnect after 1.5 x Keep Alive without any packet
                clock_t timeout = (cast(clock_t, session->keep_alive) * 1500UL);

                if ((now - session->alive_tref) >= timeout) {
                        VERBOSE_SESSION_WARNING("closing session because of inactivity.\n");
                        session_send_disconnect(session, mqtt_reason__keep_alive_timeout);
                        session_send_last_will(session);
                        return false;
                }
        }

        if (session->connected) {
                if (not session_send_subscribed_topics(session)) {
                        VERBOSE_SESSION_WARNING("unable to send subscribed topics.\n");
                        session_send_last_will(session);
                        return false;
                }
        }

        int packet_size = session_receive_packet(session);
        if (packet_size < 0) {
                if (packet_size == -EILSEQ) {
                        session_send_disconnect(session, mqtt_reason__malformed_packet);
                } else if (packet_size == -ENOMEM) {
                        session_send_disconnect(session, mqtt_reason__packet_too_large);
                }

                session_send_last_will(session);
                return false;

        } else if (packet_size == 0) {
                return true;
        }

        // any received control packet resets keep alive timer
        session->alive_tref = get_uptime_ms();

        bool connected = true;

        mqtt_msg_type_t type = (session->packet.fixed_header.control_packet_type >> 4);

        if (not session->connected and (type != mqtt_msg_type__connect)) {
                // MQTT-3.1.0-1: first packet sent from client must be CONNECT
                VERBOSE_SESSION_WARNING("packet received before CONNECT!\n");
                type = mqtt_msg_type__reserved0;

        } else if (session->connected and (type == mqtt_msg_type__connect)) {
                // MQTT-3.1.0-2: second CONNECT is a protocol violation
                VERBOSE_SESSION_WARNING("second CONNECT received!\n");
                session_send_disconnect(session, mqtt_reason__protocol_error);
                type = mqtt_msg_type__reserved0;
        }

        switch (type) {
        case mqtt_msg_type__connect:
                connected = session_cmd_connect(session);
                break;

        case mqtt_msg_type__publish:
                connected = session_cmd_publish(session);
                break;

        case mqtt_msg_type__pubrel:
                connected = session_cmd_pubrel(session);
                break;

        case mqtt_msg_type__puback:
                connected = session_cmd_puback(session);
                break;

        case mqtt_msg_type__pubrec:
                connected = session_cmd_pubrec(session);
                break;

        case mqtt_msg_type__pubcomp:
                connected = session_cmd_pubcomp(session);
                break;

        case mqtt_msg_type__subscribe:
                connected = session_cmd_subscribe(session);
                break;

        case mqtt_msg_type__unsubscribe:
                connected = session_cmd_unsubscribe(session);
                break;

        case mqtt_msg_type__pingreq:
                connected = session_cmd_pingreq(session);
                break;

        case mqtt_msg_type__disconnect:
                // will message is published only on DISCONNECT with reason 0x04 (MQTT 5)
                if (not session_cmd_disconnect(session)) {
                        session_send_last_will(session);
                }
                VERBOSE_SESSION("client '%s' disconnected.\n", SAFE_STR(session->client_id));
                connected = false;
                break;

        case mqtt_msg_type__suback:
        case mqtt_msg_type__unsuback:
        case mqtt_msg_type__pingresp:
        case mqtt_msg_type__connack:
                // packets allowed only in server->client direction (protocol error)
                VERBOSE_SESSION_WARNING("client sent server-only packet type %u!\n", type);
                session_send_disconnect(session, mqtt_reason__protocol_error);
                connected = false;
                break;

        default:
        case mqtt_msg_type__reserved0:
        case mqtt_msg_type__reserved15:
                VERBOSE_SESSION_WARNING("received reserved or unexpected command!\n");
                connected = false;
                break;
        }

        if (session->packet.variable_header_and_payload_allocated) {
                free(session->packet.variable_header_and_payload);
                session->packet.variable_header_and_payload = NULL;
                session->packet.variable_header_and_payload_allocated = false;
        }

        if (not connected) {
                VERBOSE_SESSION("closing session...\n");

                if (type != mqtt_msg_type__disconnect) {
                        session_send_last_will(session);
                }
        }

        return connected;
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
static int session_receive_packet(session_t *session)
{
        packet_t *packet = &session->packet;

        if (packet->step == packet_step__fixed_header) {
                errno = 0;
                int n = session_fetchbuf_read(session, &packet->fixed_header, sizeof(packet->fixed_header));
                if (n < 0) {
                        return n;

                } else if (n == 0) {
                        return 0;

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
                int n = session_fetchbuf_read(session, &byte, 1);
                if (n < 0) {
                        packet->step = packet_step__fixed_header;
                        return n;

                } else if (n == 0) {
                        return 0;

                } else if ((packet->remaining_length_part >= 3) and (byte & 0x80)) {
                        // remaining length is encoded on max 4 bytes (malformed packet)
                        packet->step = packet_step__fixed_header;
                        return -EILSEQ;

                } else {
                        packet->remaining_length += cast(int, (byte & 0x7F))
                                << (packet->remaining_length_part * 7);

                        packet->remaining_length_part++;

                        if ((byte & 0x80) == 0) {

                                if (packet->remaining_length >= MAX_PAYLOAD_SIZE) {
                                        packet->step = packet_step__fixed_header;
                                        return -ENOMEM;

                                } else if (packet->remaining_length < sizeof(packet->buf)) {
                                        packet->variable_header_and_payload = packet->buf;
                                        packet->variable_header_and_payload_allocated = false;

                                } else {
                                        packet->variable_header_and_payload = malloc(
                                                packet->remaining_length);
                                        if (packet->variable_header_and_payload) {
                                                packet->variable_header_and_payload_allocated = true;
                                        } else {
                                                packet->step = packet_step__fixed_header;
                                                return -ENOMEM;
                                        }
                                }

                                packet->step = packet_step__variable_header_and_payload;
                                break;
                        }
                }
        }

        while ((packet->step == packet_step__variable_header_and_payload) and
                (packet->received_length < packet->remaining_length)) {

                size_t left = packet->remaining_length - packet->received_length;
                int n = session_fetchbuf_read(session,
                        &packet->variable_header_and_payload[packet->received_length], left);
                if (n < 0) {
                        if (packet->variable_header_and_payload_allocated) {
                                free(packet->variable_header_and_payload);
                                packet->variable_header_and_payload = NULL;
                                packet->variable_header_and_payload_allocated = false;
                        }
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

        return sizeof(packet->fixed_header) + packet->remaining_length_part + packet->received_length;
}

//==============================================================================
/**
 * @brief  Send session last will if set.
 *
 * @param  session      session instance
 */
//==============================================================================
static void session_send_last_will(session_t *session)
{
        if (session->connected and session->last_will.topic) {
                VERBOSE_SESSION("sending last will...\n");

                topicdb_update_topic(session->last_will.topic, session->last_will.payload,
                        session->last_will.payload_length, session->last_will.qos,
                        session->last_will.retain, session->id, NULL, false);
        }

        // will message is published only once
        free(session->last_will.topic);
        free(session->last_will.payload);
        session->last_will.topic = NULL;
        session->last_will.payload = NULL;
        session->last_will.payload_length = 0;
}

//==============================================================================
/**
 * @brief  Check if session use MQTT 5.
 *
 * @param  session      session instance
 *
 * @return True if MQTT 5.0 is used.
 */
//==============================================================================
static bool session_is_v5(const session_t *session)
{
        return session->protocol_version == mqtt_protocol_version__5p0;
}

//==============================================================================
/**
 * @brief  NULL safe string comparison.
 *
 * @param  a            string a (can be NULL)
 * @param  b            string b (can be NULL)
 *
 * @return True if both strings are NULL or both are equal.
 */
//==============================================================================
static bool str_equal(const char *a, const char *b)
{
        if (not a or not b) {
                return a == b;
        }

        return strcmp(a, b) == 0;
}

//==============================================================================
/**
 * @brief  Check if topic name can be used in PUBLISH (no wildcards, not empty).
 *
 * @param  topic        topic name
 *
 * @return True if topic name is valid.
 */
//==============================================================================
static bool topic_name_valid(const char *topic)
{
        return topic and (topic[0] != '\0') and not strchr(topic, '#') and not strchr(topic, '+');
}

//==============================================================================
/**
 * @brief  Send DISCONNECT packet to client (MQTT 5 only, ignored for older versions).
 *
 * @param  session      session instance
 * @param  reason_code  disconnect reason
 *
 * @return True if packet was sent or not required.
 */
//==============================================================================
static bool session_send_disconnect(session_t *session, mqtt_reason_t reason_code)
{
        if (not session->connected or not session_is_v5(session)) {
                return true;
        }

        uint8_t variable_header[2];
        variable_header[0] = reason_code;
        variable_header[1] = 0;         // property length

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__disconnect;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = sizeof(variable_header);
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Reads data from a fetch buffer and stores it in the destination buffer.
 *
 * This function reads up to `count` bytes of data from a `fetchbuf_t` buffer.
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
static int session_fetchbuf_read(session_t *session, void *dst, size_t count)
{
        int n = 0;

        fetchbuf_t *fetch_buf = &session->fetch_buf;

        if (fetch_buf->seek >= fetch_buf->size) {
                fetch_buf->seek = 0;
                fetch_buf->size = 0;

                errno = 0;
                int l = socket_recv(session->socket, fetch_buf->buf, sizeof(fetch_buf->buf), 0);
                if (l < 0) {
                        if (errno == ETIME) {
                                return 0;

                        } else {
                                int err = errno;
                                VERBOSE_SESSION_WARNING("fetching error: %s\n", strerror(err));
                                n = -err;
                        }
                } else {
                        fetch_buf->size = l;
                }
        }

        if (fetch_buf->seek < fetch_buf->size) {
                size_t left = cast(size_t, (fetch_buf->size - fetch_buf->seek));
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
static int session_pushbuf_write(session_t *session, const void *src, size_t count)
{
        int n = 0;

        pushbuf_t *push_buf = &session->push_buf;

        const uint8_t *byte = src;

        while (count > 0) {
                if (push_buf->seek >= sizeof(push_buf->buf)) {
                        errno = 0;
                        int l = socket_write(session->socket, push_buf->buf, push_buf->seek);
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
static int session_pushbuf_flush(session_t *session)
{
        int n = 0;

        pushbuf_t *push_buf = &session->push_buf;

        if (push_buf->seek > 0) {
                errno = 0;
                n = socket_write(session->socket, push_buf->buf, push_buf->seek);
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
 * @brief Fetches an 8-bit integer from the session's fetch buffer.
 *
 * This function reads an 8-bit integer from the session's fetch buffer
 * and decrements the remaining length of the session accordingly.
 *
 * @param session   Pointer to the session structure.
 * @param val       Pointer to the destination variable to store the fetched value.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -2 if reading from the buffer fails.
 */
//==============================================================================
static int session_fetch_i8(session_t *session, int8_t *val)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < cast(int32_t, sizeof(*val))) {
                return -1;
        }

        memcpy(val, &packet->variable_header_and_payload[packet->payload_read_index], sizeof(*val));

        packet->payload_read_index += sizeof(*val);

        return 0;
}

//==============================================================================
/**
 * @brief Fetches a 16-bit integer from the session's fetch buffer.
 *
 * This function reads a 16-bit integer from the session's fetch buffer, converts it
 * from network byte order to host byte order, and decrements the remaining length
 * of the session accordingly.
 *
 * @param session   Pointer to the session structure.
 * @param val       Pointer to the destination variable to store the fetched value.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -2 if reading from the buffer fails.
 */
//==============================================================================
static int session_fetch_i16(session_t *session, int16_t *val)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < cast(int32_t, sizeof(*val))) {
                return -1;
        }

        memcpy(val, &packet->variable_header_and_payload[packet->payload_read_index], sizeof(*val));

        packet->payload_read_index += sizeof(*val);

        *val = ntohu16(*val);

        return 0;
}

//==============================================================================
/**
 * @brief Fetches an unsigned 16-bit integer from the session's fetch buffer.
 *
 * @param session   Pointer to the session structure.
 * @param val       Pointer to the destination variable to store the fetched value.
 *
 * @return Returns 0 on success, -1 if the remaining length is insufficient.
 */
//==============================================================================
static int session_fetch_u16(session_t *session, uint16_t *val)
{
        int16_t v = 0;
        int err = session_fetch_i16(session, &v);
        *val = cast(uint16_t, v);
        return err;
}

//==============================================================================
/**
 * @brief Fetches a 32-bit integer from the session's fetch buffer.
 *
 * This function reads a 32-bit integer from the session's fetch buffer, converts it
 * from network byte order to host byte order, and decrements the remaining length
 * of the session accordingly.
 *
 * @param session   Pointer to the session structure.
 * @param val       Pointer to the destination variable to store the fetched value.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -2 if reading from the buffer fails.
 */
//==============================================================================
static int session_fetch_i32(session_t *session, int32_t *val)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < cast(int32_t, sizeof(*val))) {
                return -1;
        }

        memcpy(val, &packet->variable_header_and_payload[packet->payload_read_index], sizeof(*val));

        packet->payload_read_index += sizeof(*val);

        *val = ntohu32(*val);

        return 0;
}

//==============================================================================
/**
 * @brief Fetches a string from the session's fetch buffer.
 *
 * This function reads a string of the specified length from the session's fetch buffer,
 * null-terminates it, and stores it in the provided buffer.
 *
 * @param session   Pointer to the session structure.
 * @param len       Length of the string to fetch.
 * @param string    Pointer to the destination buffer where the string will be stored.
 * @param bufsz     Size of the destination buffer.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or if the string length exceeds the buffer size. On error, a negative
 *         error code is returned.
 */
//==============================================================================
static int session_fetch_string(session_t *session, size_t len, char *string, size_t bufsz)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < len) {
                return -1;

        } else if (len >= bufsz) {
                return -1;
        }

        const void *src = &packet->variable_header_and_payload[packet->payload_read_index];

        // MQTT-1.5.4-2: UTF-8 string must not include null character
        if (memchr(src, '\0', len)) {
                return -1;
        }

        memcpy(string, src, len);

        string[len] = '\0';
        packet->payload_read_index += len;

        return 0;
}

//==============================================================================
/**
 * @brief Allocates memory and fetches a string from the session's fetch buffer.
 *
 * This function dynamically allocates memory to store the fetched string of the
 * specified length. The fetched string is null-terminated and stored in the provided pointer.
 *
 * @param session   Pointer to the session structure.
 * @param len       Length of the string to fetch.
 * @param string    Pointer to a pointer where the allocated string will be stored.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -ENOMEM if memory allocation fails. On other errors, a negative error
 *         code is returned.
 */
//==============================================================================
static int session_fetch_string_alloc(session_t *session, size_t len, char **string)
{
        packet_t *packet = &session->packet;

        *string = NULL;

        if ((packet->remaining_length - packet->payload_read_index) < len) {
                return -1;
        }

        char *s = malloc(len + 1);
        if (not s) {
                return -ENOMEM;
        }

        memcpy(s, &packet->variable_header_and_payload[packet->payload_read_index], len);

        packet->payload_read_index += len;

        s[len] = 0;
        *string = s;

        return 0;
}

//==============================================================================
/**
 * @brief Decodes a variable-length field from the session's fetch buffer.
 *
 * This function decodes a variable-length field (up to 4 bytes) from the session's fetch buffer
 * and stores the result in the provided `length` variable. It optionally checks
 * the remaining length of the session before fetching each byte.
 *
 * @param session                 Pointer to the session structure.
 * @param length                  Pointer to the variable where the decoded length will be stored.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -EINVAL on other errors.
 */
//==============================================================================
static int session_fetch_variable_length(session_t *session, uint32_t *length)
{
        packet_t *packet = &session->packet;
        int err = 0;
        uint32_t len = 0;
        *length = 0;

        for (int i = 0; i < 4; i++) {

                if ((packet->remaining_length - packet->payload_read_index) < 1) {
                        err = -1;
                        break;
                }

                uint8_t byte = packet->variable_header_and_payload[packet->payload_read_index];

                packet->payload_read_index++;

                len += cast(uint32_t, (byte & 0x7F)) << (i * 7);

                if ((byte & 0x80) == 0) {
                        *length = len;
                        return 0;
                }

                // continuation bit in 4th byte (malformed variable byte integer)
                err = -1;
        }

        return err;
}

//==============================================================================
/**
 * @brief Skips a specified number of bytes in the session's fetch buffer.
 *
 * This function skips over a given number of bytes in the session's fetch buffer
 * and decrements the remaining length of the session accordingly.
 *
 * @param session     Pointer to the session structure.
 * @param skip_bytes  Number of bytes to skip.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or -EINVAL if reading from the buffer fails.
 */
//==============================================================================
static int session_fetch_skip(session_t *session, size_t skip_bytes)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < skip_bytes) {
                return -EINVAL;
        }

        packet->payload_read_index += skip_bytes;

        return 0;
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
static int session_push_i8(session_t *session, int8_t val)
{
        if (session_pushbuf_write(session, &val, sizeof(val)) == sizeof(val)) {
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
static int session_push_variable_length(session_t *session, int32_t vlength)
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

        if (session_pushbuf_write(session, buf, n) == n) {
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
static int session_push_buffer(session_t *session, const void *buf, size_t count)
{
        if (not buf or not count) {
                return 0;

        } else if (session_pushbuf_write(session, buf, count) == cast(int, count)) {
                return 0;

        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Handles an MQTT CONNECT command from the client.
 *
 * This function processes an MQTT CONNECT command, validates the client's protocol
 * version, credentials, and other parameters, and interprets any properties
 * or payload in the message. If successful, it prepares the session for the client.
 *
 * @param session  Pointer to the session structure.
 *
 * @return Returns true if the CONNECT command was processed successfully. Returns false
 *         if any validation or processing step fails.
 */
//==============================================================================
static bool session_cmd_connect(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        /*
         * Fixed header
         */
        if (packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length == 0) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        /*
         * Variable header
         */
        uint16_t protocol_name_len = 0;
        char protocol_name[8];
        int8_t protocol_version = 0;
        uint16_t keep_alive = 0;

        if (session_fetch_u16(session, &protocol_name_len) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_string(session, protocol_name_len, protocol_name, sizeof(protocol_name)) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i8(session, &protocol_version) != 0) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        bool protocol_supported = true;

        if (str_equal(protocol_name, "MQIsdp")) {
                session->protocol_version = mqtt_protocol_version__3p1;
                protocol_supported = (protocol_version == 3);

        } else if (str_equal(protocol_name, "MQTT")) {
                if (protocol_version == 4) {
                        session->protocol_version = mqtt_protocol_version__3p1p1;

                } else if (protocol_version == 5) {
                        session->protocol_version = mqtt_protocol_version__5p0;

                } else {
                        session->protocol_version = mqtt_protocol_version__3p1p1;
                        protocol_supported = false;
                }

        } else {
                // MQTT-3.1.2-1: invalid protocol name, close connection without CONNACK
                DEBUG_SESSION_ERROR();
                return false;
        }

        if (not protocol_supported) {
                // MQTT-3.1.2-2: respond with CONNACK 0x01 and close connection
                VERBOSE_SESSION_WARNING("unsupported protocol version %d!\n", protocol_version);
                session_resp_connack(session, mqtt_reason__unsupported_protocol_version, false);
                return false;
        }

        DEBUG_SESSION("client is using MQTT %s\n",
                (session->protocol_version == mqtt_protocol_version__3p1)   ? "3.1"
                : (session->protocol_version == mqtt_protocol_version__3p1p1) ? "3.1.1"
                                                                              : "5.0");

        if (session_fetch_i8(session, &session->connect_flags) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_u16(session, &keep_alive) != 0) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        session->keep_alive = keep_alive;

        /*
         * Connect flags validation
         */
        uint8_t flags = cast(uint8_t, session->connect_flags);
        uint8_t will_qos = (flags & mqtt_connect_flag__will_qos_mask) >> 3;

        if (flags & mqtt_connect_flag__reserved) {
                // MQTT-3.1.2-3
                DEBUG_SESSION_ERROR();
                return false;

        } else if (flags & mqtt_connect_flag__will_flag) {
                if (will_qos > 2) {
                        // MQTT-3.1.2-14
                        DEBUG_SESSION_ERROR();
                        return false;
                }

        } else if ((will_qos != 0) or (flags & mqtt_connect_flag__will_retain)) {
                // MQTT-3.1.2-13, MQTT-3.1.2-15
                DEBUG_SESSION_ERROR();
                return false;
        }

        if (not session_is_v5(session) and (flags & mqtt_connect_flag__password) and
                not (flags & mqtt_connect_flag__user_name)) {
                // MQTT-3.1.2-22
                DEBUG_SESSION_ERROR();
                return false;
        }

        /*
         * Properties
         */
        mqtt_properties_t props = {0};

        if (session_is_v5(session)) {
                if (session_fetch_properties(session, &props) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        /*
         * Payload
         */
        uint16_t length = 0;

        /* client ID */ {
                if (session_fetch_u16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->client_id) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (strlen(session->client_id) != length) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (flags & mqtt_connect_flag__will_flag) {

                if (session_is_v5(session)) {
                        mqtt_properties_t will_props = {0};

                        if (session_fetch_properties(session, &will_props) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if (session_fetch_u16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->last_will.topic) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if ((strlen(session->last_will.topic) != length) or
                        not topic_name_valid(session->last_will.topic)) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_u16(session, &session->last_will.payload_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, session->last_will.payload_length,
                                   &session->last_will.payload) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }

                session->last_will.qos = will_qos;
                session->last_will.retain = (flags & mqtt_connect_flag__will_retain);
        }

        if (flags & mqtt_connect_flag__user_name) {
                if (session_fetch_u16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->user_name) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (strlen(session->user_name) != length) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (flags & mqtt_connect_flag__password) {
                if (session_fetch_u16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->password) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (packet->payload_read_index != packet->remaining_length) {
                // unexpected data at the end of packet
                DEBUG_SESSION_ERROR();
                return false;
        }

        /*
         * Validation
         */
        mqtt_reason_t reason_code = mqtt_reason__success;
        bool client_id_assigned = false;

        if (props.authentication_method) {
                // enhanced authentication is not supported
                VERBOSE_SESSION_WARNING("authentication method not supported!\n");
                reason_code = mqtt_reason__bad_authentication_method;

        } else if (isstrempty(session->client_id)) {
                // MQTT-3.1.3-6, MQTT-3.1.3-8: server can assign client ID if clean session is set
                if ((session->protocol_version == mqtt_protocol_version__3p1) or
                        ((session->protocol_version == mqtt_protocol_version__3p1p1) and
                                not (flags & mqtt_connect_flag__clean_start))) {

                        VERBOSE_SESSION_WARNING("empty client ID not allowed!\n");
                        reason_code = mqtt_reason__client_identifier_not_valid;

                } else {
                        char *id = malloc(24);
                        if (not id) {
                                reason_code = mqtt_reason__server_unavailable;
                        } else {
                                snprintf(id, 24, "dnx-%08x-%u", cast(unsigned, get_uptime_ms()),
                                        cast(unsigned, session->id));
                                free(session->client_id);
                                session->client_id = id;
                                client_id_assigned = true;
                        }
                }

        } else if (strlen(session->client_id) > MAX_CLIENT_ID_LENGTH) {
                VERBOSE_SESSION_WARNING("client ID too long!\n");
                reason_code = mqtt_reason__client_identifier_not_valid;
        }

        if ((reason_code == mqtt_reason__success) and global->allow_list_enabled) {
                bool client_allowed = false;

                size_t i = 0;
                const char *key = NULL;
                const char *val = NULL;

                const char *base = CONFIG_KEY_ALLOW_BASE;
                const size_t base_len = strlen(base);

                while (conf_parser__get_pair(global->conf, i, &key, &val) == 0) {

                        if (key and isstreqn(key, base, base_len)) {
                                const char *client_name = key + base_len;

                                if (str_equal(client_name, session->client_id) and
                                        (conf_parser__is_value_true(val))) {
                                        client_allowed = true;
                                        break;
                                }
                        }

                        i++;
                }

                if (not client_allowed) {
                        VERBOSE_SESSION_WARNING("client '%s' not allowed!\n", session->client_id);
                        reason_code = mqtt_reason__not_authorized;
                }
        }

        if (reason_code == mqtt_reason__success) {
                const char *cfg_user = conf_parser__get_value(global->conf, CONFIG_KEY_USER);
                const char *cfg_password = conf_parser__get_value(global->conf, CONFIG_KEY_PASSWORD);

                if (cfg_user or cfg_password) {
                        if (not str_equal(session->user_name, cfg_user) or
                                not str_equal(session->password, cfg_password)) {

                                VERBOSE_SESSION_WARNING("invalid credentials of client '%s' (user: '%s')!\n",
                                        session->client_id, SAFE_STR(session->user_name));

                                reason_code = mqtt_reason__bad_user_name_or_password;
                        }
                }
        }

        if (reason_code == mqtt_reason__success) {
                // MQTT-3.1.4-2: existing session with the same client ID is closed
                // (only after successful authentication)
                llist_foreach (session_t *, s, global->sessions) {
                        if ((s != session) and s->connected and str_equal(s->client_id, session->client_id)) {
                                s->close_session = true;
                        }
                }

                VERBOSE_SESSION("connected client ID: '%s'\n", session->client_id);
        }

        bool sent = session_resp_connack(session, reason_code, client_id_assigned);

        if (sent and (reason_code == mqtt_reason__success)) {
                session->connected = true;
                return true;

        } else {
                // MQTT-3.2.2-5: close connection if CONNACK reason is not success
                return false;
        }
}

//==============================================================================
/**
 * @brief  Convert MQTT 5 reason code to MQTT 3.1/3.1.1 CONNACK return code.
 *
 * @param  reason_code  MQTT 5 reason code
 *
 * @return MQTT 3.x return code.
 */
//==============================================================================
static uint8_t connack_v3_return_code(mqtt_reason_t reason_code)
{
        switch (reason_code) {
        case mqtt_reason__success:
                return mqtt_v3_connack__accepted;

        case mqtt_reason__unsupported_protocol_version:
                return mqtt_v3_connack__unacceptable_protocol;

        case mqtt_reason__client_identifier_not_valid:
                return mqtt_v3_connack__identifier_rejected;

        case mqtt_reason__server_unavailable:
        case mqtt_reason__server_busy:
                return mqtt_v3_connack__server_unavailable;

        case mqtt_reason__bad_user_name_or_password:
                return mqtt_v3_connack__bad_user_name_or_password;

        default:
                return mqtt_v3_connack__not_authorized;
        }
}

//==============================================================================
/**
 * @brief Sends a CONNACK response to the client.
 *
 * This function constructs and sends an MQTT CONNACK (connection acknowledgment)
 * packet to the client, indicating whether the connection was accepted or rejected.
 *
 * @param session             Pointer to the session structure.
 * @param reason_code         Reason code indicating the result of the connection attempt.
 * @param client_id_assigned  Client ID was assigned by server (MQTT 5 property is sent).
 *
 * @return Returns true if the CONNACK packet was successfully sent. Returns false if
 *         the packet serialization or transmission fails.
 */
//==============================================================================
static bool session_resp_connack(session_t *session, mqtt_reason_t reason_code, bool client_id_assigned)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[32 + MAX_CLIENT_ID_LENGTH];
        size_t n = 0;

        // MQTT-3.2.2-1, MQTT-3.2.2-4: session state is not stored by the broker,
        // thus session present flag is always 0.
        variable_header[n++] = 0;

        if (not session_is_v5(session)) {
                variable_header[n++] = connack_v3_return_code(reason_code);

        } else {
                variable_header[n++] = reason_code;

                uint8_t props[sizeof(variable_header) - 3];
                size_t p = 0;

                props[p++] = mqtt_property__receive_maximum;
                props[p++] = MAX_QOS2_INFLIGHT >> 8;
                props[p++] = MAX_QOS2_INFLIGHT;

                props[p++] = mqtt_property__maximum_packet_size;
                props[p++] = cast(uint32_t, MAX_PAYLOAD_SIZE) >> 24;
                props[p++] = cast(uint32_t, MAX_PAYLOAD_SIZE) >> 16;
                props[p++] = cast(uint32_t, MAX_PAYLOAD_SIZE) >> 8;
                props[p++] = cast(uint32_t, MAX_PAYLOAD_SIZE) & 0xFF;

                props[p++] = mqtt_property__wildcard_subscription_available;
                props[p++] = 0;

                props[p++] = mqtt_property__subscription_identifier_available;
                props[p++] = 0;

                props[p++] = mqtt_property__shared_subscription_available;
                props[p++] = 0;

                if (client_id_assigned and session->client_id) {
                        size_t len = min(strlen(session->client_id), cast(size_t, MAX_CLIENT_ID_LENGTH));
                        props[p++] = mqtt_property__assigned_client_identifier;
                        props[p++] = len >> 8;
                        props[p++] = len;
                        memcpy(&props[p], session->client_id, len);
                        p += len;
                }

                variable_header[n++] = p;       // property length (always < 128)
                memcpy(&variable_header[n], props, p);
                n += p;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__connack;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = n;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Handles an MQTT PINGREQ message from the client and responds with a PINGRESP message.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the message was processed successfully, false otherwise.
 */
//==============================================================================
static bool session_cmd_pingreq(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        session->alive_tref = get_uptime_ms();

        if (packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) {
                DEBUG_SESSION_ERROR();
                return false;

        } else {
                return session_resp_pingresp(session);
        }
}

//==============================================================================
/**
 * @brief Sends an MQTT PINGRESP message to the client in response to a PINGREQ.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PINGRESP message was serialized and sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_pingresp(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__pingresp;
        packet.fixed_header_flags = 0;
        packet.variable_header = NULL;
        packet.variable_header_length = 0;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Handles an MQTT DISCONNECT message from the client and validates its content.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if will message shall be discarded (normal disconnection), false if
 *         will message shall be published (malformed packet or reason code 0x04).
 */
//==============================================================================
static bool session_cmd_disconnect(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        if (packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (not session_is_v5(session)) {
                // MQTT-3.14.4-3: will message is discarded
                return (packet->remaining_length == 0);
        }

        // MQTT 5: reason code and properties are optional
        int8_t reason_code = mqtt_reason__normal_disconnection;

        if (packet->remaining_length >= 1) {
                if (session_fetch_i8(session, &reason_code) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (packet->remaining_length >= 2) {
                mqtt_properties_t props = {0};

                if (session_fetch_properties(session, &props) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (cast(uint8_t, reason_code) == mqtt_reason__disconnect_with_will_message) {
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief Handles an MQTT PUBLISH message from the client and processes its payload.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PUBLISH message was processed successfully, false otherwise.
 */
//==============================================================================
static bool session_cmd_publish(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        uint8_t publish_flags = packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask;
        uint8_t qos = (publish_flags & mqtt_publish_flag__qos_mask) >> mqtt_publish_flag__qos_pos;
        bool retain = publish_flags & mqtt_publish_flag__retain;
        bool dup = publish_flags & mqtt_publish_flag__duplicated;
        uint16_t topic_length = 0;
        uint16_t packet_identifier = 0;
        char topic_buf[MAX_TOPIC_LENGTH] = {0};

        if (qos > 2) {
                // MQTT-3.3.1-4: QoS 3 is malformed packet
                DEBUG_SESSION_ERROR();
                session_send_disconnect(session, mqtt_reason__malformed_packet);
                return false;

        } else if (dup and (qos == 0)) {
                // MQTT-3.3.1-2
                DEBUG_SESSION_ERROR();
                session_send_disconnect(session, mqtt_reason__malformed_packet);
                return false;

        } else if (packet->remaining_length == 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_u16(session, &topic_length) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (topic_length >= sizeof(topic_buf)) {
                VERBOSE_SESSION_WARNING("publish: topic too long (%u)!\n", topic_length);
                session_send_disconnect(session, mqtt_reason__topic_name_invalid);
                return false;

        } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (not topic_name_valid(topic_buf)) {
                // MQTT-3.3.2-2: topic name must not contain wildcards (and must not be empty
                // because topic aliases are not supported)
                VERBOSE_SESSION_WARNING("publish: invalid topic name '%s'!\n", topic_buf);
                session_send_disconnect(session, mqtt_reason__topic_name_invalid);
                return false;
        }

        if (qos != 0) {
                if (session_fetch_u16(session, &packet_identifier) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (packet_identifier == 0) {
                        // MQTT-2.3.1-1: packet identifier must be non-zero
                        DEBUG_SESSION_ERROR();
                        session_send_disconnect(session, mqtt_reason__protocol_error);
                        return false;
                }
        }

        if (session_is_v5(session)) {
                mqtt_properties_t props = {0};

                if (session_fetch_properties(session, &props) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (props.topic_alias) {
                        // Topic Alias Maximum is not sent in CONNACK (0), so alias is not allowed
                        DEBUG_SESSION_ERROR();
                        session_send_disconnect(session, mqtt_reason__topic_alias_invalid);
                        return false;
                }
        }

        /*
         * QoS 2 duplicate detection (exactly once delivery)
         */
        bool deliver = true;

        if (qos == 2) {
                for (size_t i = 0; i < session->qos2_rx.count; i++) {
                        if (session->qos2_rx.id[i] == packet_identifier) {
                                deliver = false;        // retransmission, already delivered
                                break;
                        }
                }

                if (deliver) {
                        if (session->qos2_rx.count >= MAX_QOS2_INFLIGHT) {
                                if (session_is_v5(session)) {
                                        VERBOSE_SESSION_WARNING("receive maximum exceeded!\n");
                                        session_send_disconnect(session, mqtt_reason__receive_maximum_exceeded);
                                        return false;
                                }

                                // MQTT 3.x: forget the oldest flow
                                memmove(&session->qos2_rx.id[0], &session->qos2_rx.id[1],
                                        sizeof(session->qos2_rx.id[0]) * (MAX_QOS2_INFLIGHT - 1));
                                session->qos2_rx.count--;
                        }
                }
        }

        /*
         * Payload (zero length payload is allowed)
         */
        size_t msg_len = (packet->remaining_length - packet->payload_read_index);
        const uint8_t *msg = &packet->variable_header_and_payload[packet->payload_read_index];

        mqtt_reason_t reason_code = mqtt_reason__success;

        if (deliver) {
                reason_code = topicdb_update_topic(topic_buf, msg, msg_len, qos, retain, session->id, NULL, false);

                if ((qos == 2) and (reason_code == mqtt_reason__success)) {
                        session->qos2_rx.id[session->qos2_rx.count++] = packet_identifier;
                }
        }

        /*
         * Response
         */
        switch (qos) {
        default:
        case 0:
                return true;

        case 1:
                return session_resp_puback(session, reason_code, packet_identifier);

        case 2:
                return session_resp_pubrec(session, reason_code, packet_identifier);
        }
}

//==============================================================================
/**
 * @brief Sends an MQTT PUBACK message to acknowledge a PUBLISH message with QoS 1.
 *
 * @param  session            Pointer to the session structure.
 * @param  reason_code        Reason code indicating the result of the acknowledgment.
 * @param  packet_identifier  Identifier of the packet being acknowledged.
 *
 * @return True if the PUBACK message was serialized and sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_puback(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[4];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = reason_code;
        variable_header[3] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__puback;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Sends an MQTT PUBREC message to acknowledge receipt of a PUBLISH message with QoS 2.
 *
 * @param  session            Pointer to the session structure.
 * @param  reason_code        Reason code indicating the result of the acknowledgment.
 * @param  packet_identifier  Identifier of the packet being acknowledged.
 *
 * @return True if the PUBREC message was serialized and sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_pubrec(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[4];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = reason_code;
        variable_header[3] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__pubrec;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Handles an MQTT PUBREL message from the client and sends a PUBCOMP message in response.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PUBREL message was processed and the PUBCOMP message was sent successfully, false
 * otherwise.
 */
//==============================================================================
static bool session_cmd_pubrel(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        uint16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                // MQTT-3.6.1-1
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length < 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_u16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_is_v5(session)) {
                // if reason code and property length is 0 then both can be ommitted.
                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        int8_t reason_code = 0;

                        if (session_fetch_i8(session, &reason_code) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        mqtt_properties_t props = {0};

                        if (session_fetch_properties(session, &props) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }
        }

        /*
         * Release QoS 2 flow
         */
        mqtt_reason_t reason_code = mqtt_reason__packet_identifier_not_found;

        for (size_t i = 0; i < session->qos2_rx.count; i++) {
                if (session->qos2_rx.id[i] == packet_identifier) {
                        memmove(&session->qos2_rx.id[i], &session->qos2_rx.id[i + 1],
                                sizeof(session->qos2_rx.id[0]) * (session->qos2_rx.count - i - 1));
                        session->qos2_rx.count--;
                        reason_code = mqtt_reason__success;
                        break;
                }
        }

        /*
         * Response
         */
        return session_resp_pubcomp(session, reason_code, packet_identifier);
}

//==============================================================================
/**
 * @brief Handles an MQTT PUBACK message from the client for a PUBLISH message with QoS 1.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PUBACK message was processed successfully, false otherwise.
 */
//==============================================================================
static bool session_cmd_puback(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length < 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {
                // if reason code and property length is 0 then both can be ommitted.

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        int8_t reason_code = 0;

                        if (session_fetch_i8(session, &reason_code) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        uint32_t property_length = 0;
                        if (session_fetch_variable_length(session, &property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;

                        } else if (session_fetch_skip(session, property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }
        }

        return true;
}

//==============================================================================
/**
 * @brief Handles an MQTT PUBREC message from the client for a PUBLISH message with QoS 2.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PUBREC message was processed and a PUBREL message was sent successfully, false
 * otherwise.
 */
//==============================================================================
static bool session_cmd_pubrec(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;
        int8_t reason_code = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length < 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {
                // if reason code and property length is 0 then both can be ommitted.

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        if (session_fetch_i8(session, &reason_code) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        uint32_t property_length = 0;
                        if (session_fetch_variable_length(session, &property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;

                        } else if (session_fetch_skip(session, property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }
        }

        /*
         * Response (MQTT 5: reason code >= 0x80 ends the QoS 2 flow, no PUBREL is sent)
         */
        if (cast(uint8_t, reason_code) >= 0x80) {
                return true;
        }

        return session_resp_pubrel(session, 0, packet_identifier);
}

//==============================================================================
/**
 * @brief Handles an MQTT PUBCOMP message from the client for a PUBREL message with QoS 2.
 *
 * @param  session  Pointer to the session structure.
 *
 * @return True if the PUBCOMP message was processed successfully, false otherwise.
 */
//==============================================================================
static bool session_cmd_pubcomp(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length < 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {
                // if reason code and property length is 0 then both can be ommitted.

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        int8_t reason_code = 0;

                        if (session_fetch_i8(session, &reason_code) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if ((packet->remaining_length - packet->payload_read_index) > 0) {
                        uint32_t property_length = 0;
                        if (session_fetch_variable_length(session, &property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;

                        } else if (session_fetch_skip(session, property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }
        }

        return true;
}

//==============================================================================
/**
 * @brief Sends an MQTT PUBREL message in response to a PUBREC message with QoS 2.
 *
 * @param  session            Pointer to the session structure.
 * @param  reason_code        Reason code indicating the result of the acknowledgment.
 * @param  packet_identifier  Identifier of the packet being acknowledged.
 *
 * @return True if the PUBREL message was serialized and sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_pubrel(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[4];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = reason_code;
        variable_header[3] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__pubrel;
        packet.fixed_header_flags = 0x02;       // MQTT-3.6.1-1: reserved flags must be 0010
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Sends a PUBCOMP response for a previously received PUBREL packet.
 *
 * @param  session             Pointer to the MQTT session structure.
 * @param  reason_code         MQTT reason code for the PUBCOMP response.
 * @param  packet_identifier   Identifier of the packet being acknowledged.
 *
 * @return True if the packet was sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_pubcomp(session_t *session, mqtt_reason_t reason_code, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[4];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = reason_code;
        variable_header[3] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__pubcomp;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = NULL;
        packet.payload_length = 0;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Handles a SUBSCRIBE command received from a client.
 *
 * @param  session  Pointer to the MQTT session structure.
 *
 * @return True if the subscription was successfully processed, false otherwise.
 */
//==============================================================================
static bool session_cmd_subscribe(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        uint16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                // MQTT-3.8.1-1
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length == 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_u16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet_identifier == 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_is_v5(session)) {
                mqtt_properties_t props = {0};

                if (session_fetch_properties(session, &props) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        // smallest topic filter entry: length (2) + 1 character + options (1)
        size_t max_count = ((packet->remaining_length - packet->payload_read_index) / 4) + 1;
        uint8_t *reason_code = malloc(max_count);
        if (not reason_code) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        size_t reason_code_count = 0;
        bool ok = true;

        while (ok and ((packet->remaining_length - packet->payload_read_index) > 0)) {

                char topic_buf[MAX_TOPIC_LENGTH] = {0};
                uint16_t topic_length = 0;
                int8_t subscribe_flags = 0;
                bool topic_too_long = false;

                if (session_fetch_u16(session, &topic_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;

                } else if (topic_length == 0) {
                        // MQTT-4.7.3-1: topic filter must be at least one character long
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;

                } else if (topic_length >= sizeof(topic_buf)) {
                        topic_too_long = true;

                        if (session_fetch_skip(session, topic_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                ok = false;
                                break;
                        }

                } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;
                }

                if (session_fetch_i8(session, &subscribe_flags) != 0) {
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;
                }

                uint8_t opts = cast(uint8_t, subscribe_flags);
                uint8_t qos = opts & mqtt_subscribe_flag__qos_mask;
                uint8_t retain_handling = (opts & mqtt_subscribe_flag__retain_handling_mask) >> 4;

                if (session_is_v5(session)) {
                        if ((opts & mqtt_subscribe_flag__v5_reserved_mask) or (retain_handling == 3) or (qos == 3)) {
                                // MQTT-3.8.3-5
                                DEBUG_SESSION_ERROR();
                                session_send_disconnect(session, mqtt_reason__malformed_packet);
                                ok = false;
                                break;
                        }

                } else if ((opts & mqtt_subscribe_flag__v3_reserved_mask) or (qos == 3)) {
                        // MQTT-3.8.3-4
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;
                }

                mqtt_reason_t reason;

                if (topic_too_long) {
                        VERBOSE_SESSION_WARNING("subscribe: topic filter too long (%u)!\n", topic_length);
                        reason = mqtt_reason__topic_filter_invalid;

                } else if (strchr(topic_buf, '#') or strchr(topic_buf, '+')) {
                        reason = mqtt_reason__wildcard_subscriptions_not_supported;

                } else if (session_is_v5(session) and isstreqn(topic_buf, "$share/", 7)) {
                        reason = mqtt_reason__shared_subscriptions_not_supported;

                } else {
                        DEBUG_SESSION("subscribe: '%s'\n", topic_buf);
                        reason = topicsub_subscribe(session, topic_buf, opts);
                }

                if (reason == mqtt_reason__success) {
                        reason_code[reason_code_count] = qos;   // granted QoS 0, 1 or 2

                } else if (session_is_v5(session)) {
                        reason_code[reason_code_count] = reason;

                } else {
                        reason_code[reason_code_count] = mqtt_v3_suback__failure;
                }

                reason_code_count++;
        }

        if (ok and (reason_code_count == 0)) {
                // MQTT-3.8.3-3: SUBSCRIBE must contain at least one topic filter
                DEBUG_SESSION_ERROR();
                session_send_disconnect(session, mqtt_reason__protocol_error);
                ok = false;
        }

        if (ok) {
                ok = session_resp_suback(session, reason_code, reason_code_count, packet_identifier);
        }

        free(reason_code);

        return ok;
}

//==============================================================================
/**
 * @brief Sends a SUBACK response to a previously received SUBSCRIBE packet.
 *
 * @param  session             Pointer to the MQTT session structure.
 * @param  reason_code         Array of reason codes for each subscribed topic.
 * @param  reason_code_count   Number of reason codes in the array.
 * @param  packet_identifier   Identifier of the SUBSCRIBE packet being acknowledged.
 *
 * @return True if the packet was sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_suback(session_t *session, const uint8_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[3];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__suback;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = reason_code;
        packet.payload_length = reason_code_count;

        return session_serialize_packet(session, &packet) == 0;
}

//==============================================================================
/**
 * @brief Handles an UNSUBSCRIBE command received from a client.
 *
 * @param  session  Pointer to the MQTT session structure.
 *
 * @return True if the unsubscription was successfully processed, false otherwise.
 */
//==============================================================================
static bool session_cmd_unsubscribe(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        uint16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                // MQTT-3.10.1-1
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length == 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_u16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet_identifier == 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_is_v5(session)) {
                mqtt_properties_t props = {0};

                if (session_fetch_properties(session, &props) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        // smallest topic filter entry: length (2) + 1 character
        size_t max_count = ((packet->remaining_length - packet->payload_read_index) / 3) + 1;
        uint8_t *reason_code = malloc(max_count);
        if (not reason_code) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        size_t reason_code_count = 0;
        bool ok = true;

        while ((packet->remaining_length - packet->payload_read_index) > 0) {

                char topic_buf[MAX_TOPIC_LENGTH] = {0};
                uint16_t topic_length = 0;
                mqtt_reason_t reason;

                if (session_fetch_u16(session, &topic_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;

                } else if (topic_length == 0) {
                        // MQTT-4.7.3-1
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;

                } else if (topic_length >= sizeof(topic_buf)) {
                        // such subscription cannot exist
                        if (session_fetch_skip(session, topic_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                ok = false;
                                break;
                        }

                        reason = mqtt_reason__no_subscription_existed;

                } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                        DEBUG_SESSION_ERROR();
                        ok = false;
                        break;

                } else {
                        DEBUG_SESSION("unsubscribe: '%s'\n", topic_buf);
                        reason = topicsub_unsubscribe(session, topic_buf);
                }

                reason_code[reason_code_count++] = reason;
        }

        if (ok and (reason_code_count == 0)) {
                // MQTT-3.10.3-2: UNSUBSCRIBE must contain at least one topic filter
                DEBUG_SESSION_ERROR();
                session_send_disconnect(session, mqtt_reason__protocol_error);
                ok = false;
        }

        if (ok) {
                ok = session_resp_unsuback(session, reason_code, reason_code_count, packet_identifier);
        }

        free(reason_code);

        return ok;
}

//==============================================================================
/**
 * @brief Sends an UNSUBACK response to a previously received UNSUBSCRIBE packet.
 *
 * @param  session             Pointer to the MQTT session structure.
 * @param  reason_code         Array of reason codes for each unsubscribed topic.
 * @param  reason_code_count   Number of reason codes in the array.
 * @param  packet_identifier   Identifier of the UNSUBSCRIBE packet being acknowledged.
 *
 * @return True if the packet was sent successfully, false otherwise.
 */
//==============================================================================
static bool session_resp_unsuback(session_t *session, const uint8_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[3];
        variable_header[0] = packet_identifier >> 8;
        variable_header[1] = packet_identifier;
        variable_header[2] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                // MQTT 3.x UNSUBACK contains only packet identifier
                variable_header_len = 2;
                reason_code = NULL;
                reason_code_count = 0;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__unsuback;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = reason_code;
        packet.payload_length = reason_code_count;

        return session_serialize_packet(session, &packet) == 0;
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
static int session_serialize_packet(session_t *session, const mqtt_control_packet_t *packet)
{
        int32_t remainder_length = packet->variable_header_length + packet->payload_length;

        if (session_push_i8(session,
                    (packet->type << 4) | (packet->fixed_header_flags & mqtt_fixed_header_flags_mask)) != 0) {
                return -1;

        } else if (session_push_variable_length(session, remainder_length) != 0) {
                return -2;

        } else if (session_push_buffer(session, packet->variable_header, packet->variable_header_length) !=
                0) {
                return -3;

        } else if (session_push_buffer(session, packet->payload, packet->payload_length) != 0) {
                return -4;
        }

        if (session_pushbuf_flush(session) < 0) {
                return -5;
        }

        return 0;
}

//==============================================================================
/**
 * @brief Interprets MQTT properties from the session data and updates session state accordingly.
 *
 * @param  session          Pointer to the session structure containing connection data.
 * @param  property_length  The length of the properties to be interpreted.
 * @param  props            Collected properties that require further handling.
 *
 * @return Returns 0 on success or -1 if an error occurs while fetching or processing properties.
 */
//==============================================================================
static int session_interpret_properties(session_t *session, size_t property_length, mqtt_properties_t *props)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < property_length) {
                return -1;
        }

        uint32_t end = packet->payload_read_index + property_length;

        while (packet->payload_read_index < end) {
                uint8_t id;
                if (session_fetch_i8(session, cast(int8_t *, &id)) != 0) {
                        return -1;
                }

                int err = 0;
                int8_t  v8;
                int16_t v16;
                int32_t v32;
                uint32_t vlen;
                uint16_t len;

                switch (id) {
                /* byte */
                case mqtt_property__payload_format_indicator:
                case mqtt_property__request_problem_information:
                case mqtt_property__request_response_information:
                case mqtt_property__maximum_qos:
                case mqtt_property__retain_available:
                case mqtt_property__wildcard_subscription_available:
                case mqtt_property__subscription_identifier_available:
                case mqtt_property__shared_subscription_available:
                        err = session_fetch_i8(session, &v8);
                        break;

                /* two byte integer */
                case mqtt_property__topic_alias:
                        props->topic_alias = true;
                        err = session_fetch_i16(session, &v16);
                        break;

                case mqtt_property__server_keep_alive:
                case mqtt_property__receive_maximum:
                case mqtt_property__topic_alias_maximum:
                        err = session_fetch_i16(session, &v16);
                        break;

                /* four byte integer */
                case mqtt_property__session_expiry_interval:
                        err = session_fetch_i32(session, &session->session_expiry_interval);
                        break;

                case mqtt_property__message_expiry_interval:
                case mqtt_property__will_delay_interval:
                case mqtt_property__maximum_packet_size:
                        err = session_fetch_i32(session, &v32);
                        break;

                /* variable byte integer */
                case mqtt_property__subscription_identifier:
                        err = session_fetch_variable_length(session, &vlen);
                        break;

                /* UTF-8 string or binary data */
                case mqtt_property__authentication_method:
                        props->authentication_method = true;
                        /* fall through */
                case mqtt_property__content_type:
                case mqtt_property__response_topic:
                case mqtt_property__assigned_client_identifier:
                case mqtt_property__response_information:
                case mqtt_property__server_reference:
                case mqtt_property__reason_string:
                case mqtt_property__correlation_data:
                case mqtt_property__authentication_data:
                        err = session_fetch_u16(session, &len);
                        if (err == 0) {
                                err = session_fetch_skip(session, len);
                        }
                        break;

                /* UTF-8 string pair */
                case mqtt_property__user_property:
                        for (int i = 0; (i < 2) and (err == 0); i++) {
                                err = session_fetch_u16(session, &len);
                                if (err == 0) {
                                        err = session_fetch_skip(session, len);
                                }
                        }
                        break;

                default:
                        // unknown property identifier (malformed packet)
                        return -1;
                }

                if (err != 0) {
                        return -1;
                }
        }

        // property values must fit exactly in property length
        return (packet->payload_read_index == end) ? 0 : -1;
}

//==============================================================================
/**
 * @brief Fetch property length and interpret MQTT 5 properties.
 *
 * @param  session          Pointer to the session structure containing connection data.
 * @param  props            Collected properties (can be NULL).
 *
 * @return Returns 0 on success or -1 on malformed properties.
 */
//==============================================================================
static int session_fetch_properties(session_t *session, mqtt_properties_t *props)
{
        mqtt_properties_t dummy = {0};
        uint32_t property_length = 0;

        if (session_fetch_variable_length(session, &property_length) != 0) {
                return -1;
        }

        return session_interpret_properties(session, property_length, props ? props : &dummy);
}

//==============================================================================
/**
 * @brief Sends a message to subscriber of a specific topic, taking into account QoS and retain settings.
 *
 * @param  session              Pointer to the session structure containing connection data.
 * @param  qos                  QOS
 * @param  retain               RETAIN flag of sent PUBLISH packet
 * @param  topic                topic to send
 * @param  payload              payload to send (can be NULL if payload_length is 0)
 * @param  payload_length       payload length (zero length payload is allowed)
 *
 * @return Returns MQTT reason code indicating success or failure in sending the message.
 */
//==============================================================================
static mqtt_reason_t session_publish_topic(session_t *session, uint8_t qos, bool retain, const char *topic,
        const void *payload, size_t payload_length)
{
        if (not payload and payload_length) {
                return mqtt_reason__implementation_specific_error;
        }

        if (qos >= 3) {
                VERBOSE_SESSION_WARNING("publish: wrong QOS value: %u!\n", qos);
                qos = 0;
        }

        size_t topic_length = strlen(topic);
        size_t identifier_length = (qos > 0) ? sizeof(uint16_t) : 0;
        size_t property_length_size = session_is_v5(session) ? sizeof(uint8_t) : 0;
        size_t variable_header_len = sizeof(uint16_t) + topic_length + identifier_length + property_length_size;

        uint8_t *variable_header = malloc(variable_header_len);
        if (not variable_header) {
                return mqtt_reason__implementation_specific_error;
        }

        size_t n = 0;
        variable_header[n++] = topic_length >> 8;
        variable_header[n++] = topic_length;
        memcpy(&variable_header[n], topic, topic_length);
        n += topic_length;

        if (qos > 0) {
                uint16_t identifier = ++global->topic_db.msg_id_ctr;
                if (identifier == 0) {
                        identifier = ++global->topic_db.msg_id_ctr;
                }

                variable_header[n++] = identifier >> 8;
                variable_header[n++] = identifier >> 0;
        }

        if (property_length_size) {
                variable_header[n++] = 0;       // property length
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__publish;
        packet.fixed_header_flags = (qos << mqtt_publish_flag__qos_pos) | (retain ? mqtt_publish_flag__retain : 0);
        packet.variable_header = variable_header;
        packet.variable_header_length = n;
        packet.payload = payload;
        packet.payload_length = payload_length;

        mqtt_reason_t reason = (session_serialize_packet(session, &packet) == 0)
                ? mqtt_reason__success
                : mqtt_reason__implementation_specific_error;

        free(variable_header);

        return reason;
}

//==============================================================================
/**
 * @brief  Send subscribed topic if refreshed.
 *
 * Retained message is sent after (re)subscription, last published message is sent
 * if was published after subscription. Note: broker stores only the last message
 * per topic, so messages published faster than session poll period are coalesced.
 *
 * @param  session              Pointer to the session structure containing connection data.
 *
 * @return True on success, false if sending failed (connection shall be closed).
 */
//==============================================================================
static bool session_send_subscribed_topics(session_t *session)
{
        if (session->publish_ctr == global->topic_db.publish_ctr) {
                return true;
        }

        // counter is read before sending; publications during send are handled in next poll
        uint32_t publish_ctr = global->topic_db.publish_ctr;

        llist_foreach (topicsub_t *, sub, session->subscribtion_list) {

                topicdb_t *topicdb = sub->topicdb_ref;
                if (not topicdb) {
                        continue;
                }

                uint8_t sub_qos = sub->flags & mqtt_subscribe_flag__qos_mask;
                bool no_local = session_is_v5(session) and (sub->flags & mqtt_subscribe_flag__no_local);
                bool retain_as_published = session_is_v5(session) and
                        (sub->flags & mqtt_subscribe_flag__retain_as_published);

                if (sub->send_retained) {
                        sub->send_retained = false;

                        if (topicdb->has_retained) {
                                // MQTT-3.3.1-8: retained message sent due to subscription has RETAIN=1
                                mqtt_reason_t reason = session_publish_topic(session,
                                        min(sub_qos, topicdb->retained_qos), true, topicdb->topic,
                                        topicdb->retained_msg, topicdb->retained_length);

                                if (reason != mqtt_reason__success) {
                                        return false;
                                }
                        }
                }

                if (topicdb->msg_seq != sub->msg_seq_sent) {
                        sub->msg_seq_sent = topicdb->msg_seq;

                        if (no_local and (topicdb->msg_publisher_id == session->id)) {
                                continue;
                        }

                        // MQTT-3.8.4-8: QoS of delivered message is min(published QoS, granted QoS)
                        mqtt_reason_t reason = session_publish_topic(session,
                                min(sub_qos, topicdb->msg_qos), retain_as_published and topicdb->msg_retain,
                                topicdb->topic, topicdb->msg, topicdb->msg_length);

                        if (reason != mqtt_reason__success) {
                                return false;
                        }
                }
        }

        session->publish_ctr = publish_ctr;

        return true;
}

//==============================================================================
/**
 * @brief Subscribes to a specific topic in the topic database and manages subscription state.
 *
 * @param  connection    The connection identifier.
 * @param  topic         The topic string to subscribe to.
 * @param  flags         Subscription flags such as QoS level and retain settings.
 *
 * @return Returns MQTT reason code indicating success or failure of the subscription process.
 */
//==============================================================================
static mqtt_reason_t topicsub_subscribe(session_t *session, const char *topic, u8_t flags)
{
        if (strchr(topic, '#') or strchr(topic, '+')) {
                return mqtt_reason__wildcard_subscriptions_not_supported;
        }

        // retain handling option exists only in MQTT 5 (0: send retained messages)
        uint8_t retain_handling = session_is_v5(session)
                ? ((flags & mqtt_subscribe_flag__retain_handling_mask) >> 4)
                : 0;

        topicsub_t *topicsub = NULL;

        llist_foreach (topicsub_t *, sub, session->subscribtion_list) {
                if (sub->topicdb_ref and str_equal(sub->topicdb_ref->topic, topic)) {
                        topicsub = sub;
                        break;
                }
        }

        if (topicsub) {
                // MQTT-3.8.4-3: existing subscription is replaced, retained message is sent again
                topicsub->flags = flags;
                topicsub->send_retained = (retain_handling == 0);

        } else {
                if (llist_size(session->subscribtion_list) >= MAX_TOPICS) {
                        VERBOSE_SESSION_WARNING("client reached maximal amount of subscriptions (%d)!\n",
                                MAX_TOPICS);
                        return mqtt_reason__quota_exceeded;
                }

                topicdb_t *topicdb = NULL;
                topicdb_update_topic(topic, NULL, 0, 0, false, 0, &topicdb, true);
                if (not topicdb) {
                        return mqtt_reason__quota_exceeded;
                }

                topicsub_t t = {
                        .topicdb_ref = topicdb,
                        .msg_seq_sent = topicdb->msg_seq,       // do not send messages published before
                        .flags = flags,
                        .send_retained = (retain_handling != 2),
                };

                topicsub = llist_push_emplace_back(session->subscribtion_list, sizeof(t), &t);
                if (not topicsub) {
                        return mqtt_reason__implementation_specific_error;
                }

                topicdb->subscribers++;
        }

        // force session_send_subscribed_topics() to check subscriptions
        global->topic_db.publish_ctr++;

        return mqtt_reason__success;
}

//==============================================================================
/**
 * @brief Unsubscribes from a specific topic or wildcard topic and updates the topic database.
 *
 * @param  connection    The connection identifier.
 * @param  topic         The topic string to unsubscribe from.
 *
 * @return Returns MQTT reason code indicating success or failure of the unsubscription process.
 */
//==============================================================================
static mqtt_reason_t topicsub_unsubscribe(session_t *session, const char *topic)
{
        llist_foreach (topicsub_t *, sub, session->subscribtion_list) {
                if (sub->topicdb_ref and str_equal(sub->topicdb_ref->topic, topic)) {
                        llist_erase_by_iterator(&_iterator);
                        return mqtt_reason__success;
                }
        }

        return mqtt_reason__no_subscription_existed;
}

//==============================================================================
/**
 * @brief Compare topic string.
 *
 * @param  a            topic a
 * @param  b            topic b
 *
 * @return Return 0 if strings are equal.
 */
//==============================================================================
static int topicsub_compare(const void *a, const void *b)
{
        const topicsub_t *sa = a;
        const topicsub_t *sb = b;

        if (sa->topicdb_ref and sb->topicdb_ref) {
                return strcmp(sa->topicdb_ref->topic, sb->topicdb_ref->topic);
        } else {
                return -1;
        }
}

//==============================================================================
/**
 * @brief Destroys topic data.
 *
 * @param  a            topic data
 */
//==============================================================================
static void topicsub_destroy(void *a)
{
        topicsub_t *sub = a;

        if (sub->topicdb_ref) {
                if (sub->topicdb_ref->subscribers) {
                        sub->topicdb_ref->subscribers--;
                }
                sub->topicdb_ref = NULL;
        }

        free(sub);
}

//==============================================================================
/**
 * @brief Create new or update existing topic.
 *
 * @param  topic         The topic string where the message will be published.
 * @param  msg_ref       Pointer to the message payload to be sent.
 * @param  msg_len       The length of the message payload.
 * @param  qos           QoS of published message.
 * @param  retain        Whether the message should be retained by the broker for future subscribers.
 * @param  publisher_id  Session ID of publisher (used by No Local subscription option).
 * @param  topicdb_ref   Created/found topic (can be NULL).
 * @param  create_only   Only create topic, do not update message.
 *
 * @return Returns MQTT reason code indicating success or failure in publishing the message.
 */
//==============================================================================
static mqtt_reason_t topicdb_update_topic(const char *topic, const void *msg_ref, size_t msg_len, uint8_t qos,
        bool retain, uint32_t publisher_id, topicdb_t **topicdb_ref, bool create_only)
{
        mqtt_reason_t reason = mqtt_reason__implementation_specific_error;

        if (topicdb_ref) {
                *topicdb_ref = NULL;
        }

        if (strchr(topic, '#') or strchr(topic, '+')) {
                return mqtt_reason__topic_name_invalid;
        }

        if (msg_len > UINT16_MAX) {
                return mqtt_reason__packet_too_large;
        }

        topicdb_t *topicdb = NULL;

        int pos = llist_find_begin(global->topic_db.data, &(topicdb_t) {.topic = const_cast(topic)});
        if (pos < 0) {
                if (llist_size(global->topic_db.data) >= MAX_TOPICS) {
                        VERBOSE("server reached maximal amount of topics (%d)!\n", MAX_TOPICS);
                        return mqtt_reason__quota_exceeded;

                } else {
                        topicdb_t t = {0};
                        t.topic = strdup(topic);
                        t.msg_update_tref = get_uptime_ms();
                        if (t.topic) {
                                topicdb = llist_push_emplace_back(global->topic_db.data, sizeof(t), &t);
                                if (not topicdb) {
                                        free(t.topic);
                                }
                        }
                }

        } else {
                topicdb = llist_at(global->topic_db.data, pos);
        }

        if (not topicdb) {
                return reason;
        }

        if (not create_only) {
                /*
                 * Message delivered to current subscribers
                 */
                if (topicdb->msg_alloc_length < msg_len) {
                        uint8_t *buf = malloc(msg_len);
                        if (not buf) {
                                return mqtt_reason__implementation_specific_error;
                        }

                        free(topicdb->msg);
                        topicdb->msg = buf;
                        topicdb->msg_alloc_length = msg_len;
                }

                if (msg_len) {
                        memcpy(topicdb->msg, msg_ref, msg_len);
                }

                topicdb->msg_length = msg_len;
                topicdb->msg_qos = qos;
                topicdb->msg_retain = retain;
                topicdb->msg_publisher_id = publisher_id;
                topicdb->msg_seq++;
                topicdb->msg_update_tref = get_uptime_ms();

                /*
                 * Retained message: changed only by PUBLISH with RETAIN=1 (MQTT-3.3.1-5),
                 * zero length payload removes retained message (MQTT-3.3.1-6, MQTT-3.3.1-7)
                 */
                if (retain) {
                        free(topicdb->retained_msg);
                        topicdb->retained_msg = NULL;
                        topicdb->retained_length = 0;
                        topicdb->has_retained = false;

                        if (msg_len) {
                                topicdb->retained_msg = malloc(msg_len);
                                if (topicdb->retained_msg) {
                                        memcpy(topicdb->retained_msg, msg_ref, msg_len);
                                        topicdb->retained_length = msg_len;
                                        topicdb->retained_qos = qos;
                                        topicdb->has_retained = true;
                                } else {
                                        VERBOSE("unable to store retained message of '%s'!\n", topic);
                                }
                        }
                }

                global->topic_db.publish_ctr++;
        }

        if (topicdb_ref) {
                *topicdb_ref = topicdb;
        }

        return mqtt_reason__success;
}

//==============================================================================
/**
 * @brief  Topic database maintenance.
 */
//==============================================================================
static void topicdb_maintenance(void)
{
        clock_t now = get_uptime_ms();

        if (now - global->topic_db.last_maintenance_tref >= TOPICDB_MAINTENANCE_ms) {

                llist_foreach (topicdb_t *, topicdb, global->topic_db.data) {

                        if ((now - topicdb->msg_update_tref >= MAX_RETAIN_TIME_ms) and
                                (topicdb->subscribers == 0)) {

                                VERBOSE("'%s' topic erased.\n", topicdb->topic);
                                llist_erase_by_iterator(&_iterator);
                        }
                }

                global->topic_db.last_maintenance_tref = now;
        }
}

//==============================================================================
/**
 * @brief Compare topic string.
 *
 * @param  a            topic a
 * @param  b            topic b
 *
 * @return Return 0 if strings are equal.
 */
//==============================================================================
static int topicdb_compare(const void *a, const void *b)
{
        const topicdb_t *sa = a;
        const topicdb_t *sb = b;
        return strcmp(sa->topic, sb->topic);
}

//==============================================================================
/**
 * @brief Destroys topic data.
 *
 * @param  a            topic data
 */
//==============================================================================
static void topicdb_destroy(void *a)
{
        topicdb_t *sa = a;
        free(sa->topic);
        free(sa->msg);
        free(sa->retained_msg);
        sa->msg = NULL;
        sa->retained_msg = NULL;
        sa->msg_length = 0;
        sa->retained_length = 0;
        sa->topic = NULL;
        free(a);
}

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
 * @brief  Function convert value for host/network purpose.
 *
 * @param  value        value to convert
 *
 * @return Converted value.
 */
//==============================================================================
static u32_t ntohu32(u32_t value)
{
        return ((value & 0x000000FFUL) << 24) | ((value & 0x0000FF00UL) << 8) |
                ((value & 0x00FF0000UL) >> 8) | ((value & 0xFF000000UL) >> 24);
}

/*==============================================================================
  End of file
==============================================================================*/
