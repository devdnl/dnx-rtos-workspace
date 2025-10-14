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
#define DEBUG_CMD_ENABLE        false

#define CONFIG_KEY_USER         "user"
#define CONFIG_KEY_PASSWORD     "password"
#define CONFIG_KEY_PORT         "port"
#define CONFIG_KEY_DEBUG        "debug"
#define CONFIG_KEY_ALLOW_ENABLE "allow.enable"
#define CONFIG_KEY_ALLOW_BASE   "allow."

#define MAX_TOPIC_LENGTH        128
#define MAX_TOPICS              500
#define MAX_PAYLOAD_SIZE        4096
#define MAX_RETAIN_TIME_ms      (24 * 60 * 60 * 1000UL)
#define TOPICDB_MAINTENANCE_ms  (60 * 1000UL)

#define DEBUG_SESSION(...)                                                              \
        if (global->debug) {                                                            \
                time_t t = time(NULL);                                                  \
                struct tm lt;                                                           \
                localtime_r(&t, &lt);                                                   \
                char t_str[32];                                                         \
                strftime(t_str, sizeof(t_str), "%F %X", &lt);                           \
                printf("[mqtt_broker, debug, %s, %s] ", t_str, session->client_ip_str); \
                printf(__VA_ARGS__);                                                    \
        }

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

#define mqtt_max_topics_in_subscription             16

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
        uint8_t *msg;
        uint16_t msg_length;
        uint16_t msg_alloc_length;
        clock_t msg_update_tref;
        uint32_t subscribers;
        bool retain;
} topicdb_t;

typedef struct {
        topicdb_t *topicdb_ref;
        clock_t msg_sent_tref;
        uint8_t flags;
} topicsub_t;

typedef struct {
        SOCKET *socket;
        char *client_id;
        char *user_name;
        char *password;
        llist_t *subscribtion_list;
        char client_ip_str[16];
        uint8_t msg_buf[256];
        NET_INET_IPv4_t client_ip;
        clock_t alive_tref;
        packet_t packet;
        fetchbuf_t fetch_buf;
        pushbuf_t push_buf;
        mqtt_protocol_version_t protocol_version;
        uint32_t publish_ctr;
        int32_t session_expiry_interval;
        int16_t keep_alive;
        int8_t connect_flags;
        bool close_session;

        struct {
                char *topic;
                char *payload;
                int16_t payload_length;
        } last_will;
} session_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static uint16_t ntohu16(uint16_t value);
static uint32_t ntohu32(uint32_t value);

static session_t *session_new(SOCKET *socket);
static session_t *session_delete(session_t *session);
static void llist_session_dtor(void *object);
static bool session_poll(session_t *session);
static int session_receive_packet(session_t *session);
static int session_fetchbuf_read(session_t *session, void *dst, size_t count);
static int session_pushbuf_write(session_t *session, const void *src, size_t count);
static int session_pushbuf_flush(session_t *session);
static int session_fetch_variable_length(session_t *session, uint32_t *length);
static int session_fetch_skip(session_t *session, size_t skip_bytes);
static int session_fetch_buffer(session_t *session, void *dst, size_t count);
static int session_fetch_i8(session_t *session, int8_t *val);
static int session_fetch_i16(session_t *session, int16_t *val);
static int session_fetch_i32(session_t *session, int32_t *val);
static int session_fetch_string_alloc(session_t *session, size_t len, char **string);
static int session_push_i8(session_t *session, int8_t val);
static int session_push_variable_length(session_t *session, int32_t vlength);
static int session_push_buffer(session_t *session, const void *buf, size_t count);
static bool session_cmd_connect(session_t *session);
static bool session_resp_connack(session_t *session, mqtt_reason_t reason_code);
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
static int session_interpret_properties(session_t *session, size_t property_length);
static void session_send_subscribed_topics(session_t *session);
static bool session_resp_suback(session_t *session, const mqtt_reason_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier);
static bool session_resp_unsuback(session_t *session, const mqtt_reason_t *reason_code,
        size_t reason_code_count, int16_t packet_identifier);
static mqtt_reason_t session_publish_topic(session_t *session, uint8_t qos, const char *topic,
        const void *payload, size_t payload_length);

static mqtt_reason_t topicsub_subscribe(session_t *session, const char *topic, u8_t flags);
static mqtt_reason_t topicsub_unsubscribe(session_t *session, const char *topic);
static int topicsub_compare(const void *a, const void *b);
static void topicsub_destroy(void *a);

static int topicdb_compare(const void *a, const void *b);
static void topicdb_destroy(void *a);
static void topicdb_maintenance(void);
static mqtt_reason_t topicdb_update_topic(const char *topic, const uint8_t *msg_ref, size_t msg_len,
        bool retain, topicdb_t **topicdb_ref, bool create_only);

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        conf_parser_t *conf;
        bool debug;
        bool allow_list_enabled;

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

        int exit_code = EXIT_FAILURE;

        errno = 0;
        socket = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
        if (not socket) {
                printf("[%s, info] unable to create socket: %s\n", argv[0], strerror(errno));
                goto cleanup;
        }

        const NET_INET_sockaddr_t ADDR_ANY = {
                .addr = NET_INET_IPv4_ANY,
                .port = atoi(conf_parser__get_value(global->conf, CONFIG_KEY_PORT)),
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

        socket_set_recv_timeout(socket, 40);

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

                topicdb_maintenance();
        }

cleanup:
        socket_close(socket);
        llist_delete(global->topic_db.data);
        llist_delete(global->sessions);
        conf_parser__delete(global->conf);

        return exit_code;
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
        session->keep_alive = 5;

        socket_set_recv_timeout(session->socket, 0);
        socket_set_send_timeout(session->socket, 1000);

        NET_INET_sockaddr_t addr;
        socket_get_address(session->socket, &addr, sizeof(addr));
        session->client_ip = addr.addr;

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

        VERBOSE_SESSION("client '%s' session closed.\n", session->client_id);

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
                return false;
        }

        if ((now - session->alive_tref) >= ((session->keep_alive * 1000) + 60000UL)) {
                VERBOSE_SESSION_WARNING("closing session because of inactivity.\n");
                return false;
        }

        bool connected = true;

        session_send_subscribed_topics(session);

        int packet_size = session_receive_packet(session);
        if (packet_size < 0) {
                return false;

        } else if (packet_size == 0) {
                return true;
        }

        mqtt_msg_type_t type = (session->packet.fixed_header.control_packet_type >> 4);

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
                session_cmd_disconnect(session);
                VERBOSE_SESSION("client '%s' disconnected.\n", session->client_id);
                connected = false;
                break;

        case mqtt_msg_type__suback:
                VERBOSE_SESSION_WARNING("client command mqtt_msg_type__SUBACK!\n");
                break;

        case mqtt_msg_type__unsuback:
                VERBOSE_SESSION_WARNING("client command mqtt_msg_type__UNSUBACK!\n");
                break;

        case mqtt_msg_type__pingresp:
                VERBOSE_SESSION_WARNING("client command mqtt_msg_type__PINGRESP!\n");
                break;

        case mqtt_msg_type__connack:
                VERBOSE_SESSION_WARNING("client command mqtt_msg_type__CONNACK!\n");
                break;

        default:
        case mqtt_msg_type__reserved0:
        case mqtt_msg_type__reserved15:
                VERBOSE_SESSION_WARNING("received reserved command!\n");
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

                if (session->last_will.topic and session->last_will.payload) {
                        session_publish_topic(session, 0, session->last_will.topic,
                                session->last_will.payload, session->last_will.payload_length);
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

                } else if ((packet->fixed_header.control_packet_type >> 4) > mqtt_msg_type__reserved15) {
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
                int n = session_fetchbuf_read(session, &byte, 1);
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

        } else if (len >= (bufsz - 1)) {
                return -1;
        }

        memcpy(string, &packet->variable_header_and_payload[packet->payload_read_index], len);

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

                len += cast(int, (byte & 0x7F)) << (i * 7);

                if ((byte & 0x80) == 0) {
                        *length = len;
                        break;
                }
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
        int err = 0;

        int max_skip = min(skip_bytes, (packet->remaining_length - packet->payload_read_index));
        packet->payload_read_index += max_skip;

        if (max_skip == 0) {
                err = -EINVAL;
        }

        return err;
}

//==============================================================================
/**
 * @brief Fetches a buffer of data from the session's fetch buffer.
 *
 * This function reads a specified number of bytes from the session's fetch buffer
 * and stores them in the provided destination buffer.
 *
 * @param session  Pointer to the session structure.
 * @param dst      Pointer to the destination buffer where data will be stored.
 * @param count    Number of bytes to fetch.
 *
 * @return Returns 0 on success. Returns -1 if the remaining length is insufficient
 *         or if reading from the buffer fails.
 */
//==============================================================================
static int session_fetch_buffer(session_t *session, void *dst, size_t count)
{
        packet_t *packet = &session->packet;

        if ((packet->remaining_length - packet->payload_read_index) < count) {
                return -1;
        }

        memcpy(dst, &packet->variable_header_and_payload[packet->payload_read_index], count);

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

        session->alive_tref = get_uptime_ms();

        packet_t *packet = &session->packet;

        /*
         * Fixed header
         */
        if (packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length <= 0) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        /*
         * Variable header
         */
        int16_t protocol_name_len;
        char protocol_name[32];
        int8_t protocol_version;

        if (session_fetch_i16(session, &protocol_name_len) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (protocol_name_len >= cast(int16_t, sizeof(protocol_name))) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_string(session, protocol_name_len, protocol_name, sizeof(protocol_name)) !=
                0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i8(session, &protocol_version) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i8(session, &session->connect_flags) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &session->keep_alive) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (isstreqn(protocol_name, "MQIsdp", protocol_name_len) and (protocol_version == 3)) {
                session->protocol_version = mqtt_protocol_version__3p1;
                DEBUG_SESSION("client is using MQTT 3.1\n");

        } else if (isstreqn(protocol_name, "MQTT", protocol_name_len) and (protocol_version == 4)) {
                session->protocol_version = mqtt_protocol_version__3p1p1;
                DEBUG_SESSION("client is using MQTT 3.1.1\n");

        } else if (isstreqn(protocol_name, "MQTT", protocol_name_len) and (protocol_version == 5)) {
                session->protocol_version = mqtt_protocol_version__5p0;
                DEBUG_SESSION("client is using MQTT 5.0\n");

        } else {
                DEBUG_SESSION_ERROR();
                return false;
        }

        /*
         * Properties
         */
        if (session->protocol_version == mqtt_protocol_version__5p0) {
                uint32_t property_length = 0;
                if (session_fetch_variable_length(session, &property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_interpret_properties(session, property_length) < 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        /*
         * Payload
         */
        int16_t length;

        /*client ID */ {
                if (session_fetch_i16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->client_id) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (session->connect_flags & mqtt_connect_flag__will_flag) {

                if (session->protocol_version == mqtt_protocol_version__5p0) {
                        uint32_t property_length = 0;
                        if (session_fetch_variable_length(session, &property_length) != 0) {
                                DEBUG_SESSION_ERROR();
                                return false;

                        } else if (session_fetch_skip(session, property_length) < 0) {
                                DEBUG_SESSION_ERROR();
                                return false;
                        }
                }

                if (session_fetch_i16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->last_will.topic) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_i16(session, &session->last_will.payload_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, session->last_will.payload_length,
                                   &session->last_will.payload) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (session->connect_flags & mqtt_connect_flag__user_name) {
                if (session_fetch_i16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->user_name) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (session->connect_flags & mqtt_connect_flag__password) {
                if (session_fetch_i16(session, &length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string_alloc(session, length, &session->password) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        bool client_allowed = true;
        if (global->allow_list_enabled) {
                client_allowed = false;

                size_t i = 0;
                const char *key = NULL;
                const char *val = NULL;

                const char *base = CONFIG_KEY_ALLOW_BASE;
                const size_t base_len = strlen(base);

                while (conf_parser__get_pair(global->conf, i, &key, &val) == 0) {

                        if (isstreqn(key, base, base_len)) {
                                const char *client_name = key + base_len;

                                if (isstreq(client_name, session->client_id) and
                                        (conf_parser__is_value_true(val))) {
                                        client_allowed = true;
                                        break;
                                }
                        }

                        i++;
                }
        }

        bool client_already_connected = false;

        if (isstrempty(session->client_id)) {
                client_allowed = false;
        }

        if (client_allowed) {
                llist_foreach (session_t *, s, global->sessions) {
                        if ((s != session) and isstreq(s->client_id, session->client_id)) {
                                s->close_session = true;
                        }
                }
        }

        mqtt_reason_t reason_code;

        if (not client_allowed) {
                VERBOSE_SESSION_WARNING("client '%s' not allowed!\n", session->client_id);
                reason_code = mqtt_reason__banned;

        } else if (client_already_connected) {
                VERBOSE_SESSION_WARNING("client '%s' is already connected!\n", session->client_id);
                reason_code = mqtt_reason__client_identifier_not_valid;

        } else if (not isstreq(session->user_name, conf_parser__get_value(global->conf, CONFIG_KEY_USER)) or
                not isstreq(session->password, conf_parser__get_value(global->conf, CONFIG_KEY_PASSWORD))) {

                VERBOSE_SESSION_WARNING(
                        "credetials of '%s' are invalid. User: '%s' and/or password: '%s' are incorrect!\n",
                        session->client_id, session->user_name, session->password);

                reason_code = mqtt_reason__bad_user_name_or_password;

        } else {
                VERBOSE_SESSION("connected client ID: '%s'\n", session->client_id);
                reason_code = mqtt_reason__success;
        }

        return session_resp_connack(session, reason_code);
}

//==============================================================================
/**
 * @brief Sends a CONNACK response to the client.
 *
 * This function constructs and sends an MQTT CONNACK (connection acknowledgment)
 * packet to the client, indicating whether the connection was accepted or rejected.
 *
 * @param session      Pointer to the session structure.
 * @param reason_code  Reason code indicating the result of the connection attempt.
 *
 * @return Returns true if the CONNACK packet was successfully sent. Returns false if
 *         the packet serialization or transmission fails.
 */
//==============================================================================
static bool session_resp_connack(session_t *session, mqtt_reason_t reason_code)
{
        DEBUG_CMD("%s\n", __func__);

        uint8_t variable_header[3];
        variable_header[0] = (session->connect_flags & mqtt_connect_flag__clean_start)
                ? 0
                : mqtt_connack_session_present;
        variable_header[1] = reason_code;
        variable_header[2] = 0;

        u8_t variable_header_len = sizeof(variable_header);
        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = 2;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__connack;
        packet.fixed_header_flags = 0;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
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
 * @return True if the DISCONNECT message was processed successfully, false otherwise.
 */
//==============================================================================
static bool session_cmd_disconnect(session_t *session)
{
        DEBUG_CMD("%s\n", __func__);

        packet_t *packet = &session->packet;

        session->alive_tref = get_uptime_ms();

        int8_t reason_code = 0;
        uint32_t property_length = 0;

        if (packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version != mqtt_protocol_version__5p0) {
                return true;

        } else if (packet->remaining_length <= 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {

                if (session_fetch_i8(session, &reason_code) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_variable_length(session, &property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_skip(session, property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        } else {
                DEBUG_SESSION_ERROR();
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

        session->alive_tref = get_uptime_ms();

        uint8_t publish_flags = packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask;
        uint8_t qos = (publish_flags & mqtt_publish_flag__qos_mask);
        int16_t topic_length = 0;
        int16_t packet_identifier = 0;
        char topic_buf[MAX_TOPIC_LENGTH] = {0};

        if (packet->remaining_length <= 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &topic_length) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        if (qos != mqtt_publish_flag__qos_0) {
                if (session_fetch_i16(session, &packet_identifier) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (session->protocol_version == mqtt_protocol_version__5p0) {
                uint32_t property_length = 0;

                if (session_fetch_variable_length(session, &property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_skip(session, property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        /*
         * Payload
         */
        bool retain = publish_flags & mqtt_publish_flag__retain;
        size_t msg_len = (packet->remaining_length - packet->payload_read_index);
        if (msg_len >= MAX_PAYLOAD_SIZE) {
                DEBUG_SESSION_ERROR();
                return false;
        }

        bool allocated = (msg_len > sizeof(session->msg_buf));
        uint8_t *msg = session->msg_buf;

        if (allocated) {
                msg = malloc(msg_len);
                if (not msg) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        if (session_fetch_buffer(session, msg, msg_len) != 0) {
                DEBUG_SESSION_ERROR();

                if (allocated) {
                        free(msg);
                }

                return false;
        }

        mqtt_reason_t reason_code = topicdb_update_topic(topic_buf, msg, msg_len, retain, NULL, false);

        if (allocated) {
                free(msg);
        }

        /*
         * Response
         */
        switch (qos) {
        default:
        case mqtt_publish_flag__qos_0:
                return true;

        case mqtt_publish_flag__qos_1:
                return session_resp_puback(session, reason_code, packet_identifier);

        case mqtt_publish_flag__qos_2:
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

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length <= 0) {
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

        /*
         * Response
         */
        return session_resp_pubcomp(session, mqtt_reason__success, packet_identifier);
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

        /*
         * Response
         */
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
        packet.fixed_header_flags = 0;
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

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length <= 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {
                uint32_t property_length = 0;

                if (session_fetch_variable_length(session, &property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_skip(session, property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        mqtt_reason_t reason_code[mqtt_max_topics_in_subscription] = {mqtt_reason__unspecified_error};
        size_t reason_code_count = 0;

        while ((packet->remaining_length - packet->payload_read_index) > 0) {

                char topic_buf[MAX_TOPIC_LENGTH] = {0};
                int16_t topic_length = 0;
                int8_t subscribe_flags = 0;

                if (session_fetch_i16(session, &topic_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_i8(session, &subscribe_flags) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }

                // DEBUG("subscribe: '%s'\n", session->topic_buf);
                mqtt_reason_t reason = topicsub_subscribe(session, topic_buf, subscribe_flags);

                int qos = subscribe_flags & mqtt_subscribe_flag__qos_mask;

                if (reason_code_count >= sizeof(reason_code)) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (qos == mqtt_subscribe_flag__qos_0) {
                        reason_code[reason_code_count] = (reason == mqtt_reason__success)
                                ? mqtt_reason__granted_qos_0
                                : reason;

                } else if (qos == mqtt_subscribe_flag__qos_1) {
                        reason_code[reason_code_count] = (reason == mqtt_reason__success)
                                ? mqtt_reason__granted_qos_1
                                : reason;

                } else if (qos == mqtt_subscribe_flag__qos_2) {
                        reason_code[reason_code_count] = (reason == mqtt_reason__success)
                                ? mqtt_reason__granted_qos_2
                                : reason;

                } else {
                        reason_code[reason_code_count] = mqtt_reason__unspecified_error;
                }

                reason_code_count++;
        }

        return session_resp_suback(session, reason_code, reason_code_count, packet_identifier);
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
static bool session_resp_suback(session_t *session, const mqtt_reason_t *reason_code,
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

        session->alive_tref = get_uptime_ms();

        int16_t packet_identifier = 0;

        if ((packet->fixed_header.control_packet_type & mqtt_fixed_header_flags_mask) != 2) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (packet->remaining_length <= 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session_fetch_i16(session, &packet_identifier) != 0) {
                DEBUG_SESSION_ERROR();
                return false;

        } else if (session->protocol_version == mqtt_protocol_version__5p0) {
                uint32_t property_length = 0;

                if (session_fetch_variable_length(session, &property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_skip(session, property_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }
        }

        mqtt_reason_t reason_code[mqtt_max_topics_in_subscription] = {mqtt_reason__unspecified_error};
        size_t reason_code_count = 0;

        while ((packet->remaining_length - packet->payload_read_index) > 0) {

                char topic_buf[MAX_TOPIC_LENGTH] = {0};
                int16_t topic_length = 0;

                if (session_fetch_i16(session, &topic_length) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;

                } else if (session_fetch_string(session, topic_length, topic_buf, sizeof(topic_buf)) != 0) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }

                DEBUG_SESSION("unsubscribe: '%s'\n", topic_buf);
                mqtt_reason_t reason = topicsub_unsubscribe(session, topic_buf);

                if (reason_code_count >= sizeof(reason_code)) {
                        DEBUG_SESSION_ERROR();
                        return false;
                }

                reason_code[reason_code_count] = reason;

                reason_code_count++;
        }

        return session_resp_unsuback(session, reason_code, reason_code_count, packet_identifier);
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
static bool session_resp_unsuback(session_t *session, const mqtt_reason_t *reason_code,
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
 *
 * @return Returns 0 on success or -1 if an error occurs while fetching or processing properties.
 */
//==============================================================================
static int session_interpret_properties(session_t *session, size_t property_length)
{
        int n = 0;

        while (property_length > 0) {
                uint8_t id;
                if (session_fetch_i8(session, cast(int8_t *, &id)) != 0) {
                        return -1;
                }

                property_length--;

                switch (id) {
                case mqtt_property__payload_format_indicator:
                        break;
                case mqtt_property__message_expiry_interval:
                        break;
                case mqtt_property__content_type:
                        break;
                case mqtt_property__response_topic:
                        break;
                case mqtt_property__correlation_data:
                        break;
                case mqtt_property__subscription_identifier:
                        break;
                case mqtt_property__session_expiry_interval:
                        if (session_fetch_i32(session, &session->session_expiry_interval) != 0) {
                                return false;
                        } else {
                                property_length -= 4;
                        }
                        break;
                case mqtt_property__assigned_client_identifier:
                        break;
                case mqtt_property__server_keep_alive:
                        break;
                case mqtt_property__authentication_method:
                        break;
                case mqtt_property__authentication_data:
                        break;
                case mqtt_property__request_problem_information:
                        break;
                case mqtt_property__will_delay_interval:
                        break;
                case mqtt_property__request_response_information:
                        break;
                case mqtt_property__response_information:
                        break;
                case mqtt_property__server_reference:
                        break;
                case mqtt_property__reason_string:
                        break;
                case mqtt_property__receive_maximum:
                        break;
                case mqtt_property__topic_alias_maximum:
                        break;
                case mqtt_property__topic_alias:
                        break;
                case mqtt_property__maximum_qos:
                        break;
                case mqtt_property__retain_available:
                        break;
                case mqtt_property__user_property:
                        break;
                case mqtt_property__maximum_packet_size:
                        break;
                case mqtt_property__wildcard_subscription_available:
                        break;
                case mqtt_property__subscription_identifier_available:
                        break;
                case mqtt_property__shared_subscription_available:
                        break;
                }
        }

        return n;
}

//==============================================================================
/**
 * @brief Sends a message to subscriber of a specific topic, taking into account QoS and retain settings.
 *
 * @param  session              Pointer to the session structure containing connection data.
 * @param  qos                  QOS
 * @param  topic                topic to send
 * @param  payload              payload to send or NULL if send from topicdb.
 * @param  payload_length       payload length
 *
 * @return Returns MQTT reason code indicating success or failure in sending the message.
 */
//==============================================================================
static mqtt_reason_t session_publish_topic(session_t *session, uint8_t qos, const char *topic,
        const void *payload, size_t payload_length)
{
        if (payload and not payload_length) {
                return mqtt_reason__implementation_specific_error;
        }

        if (not payload and payload_length) {
                return mqtt_reason__implementation_specific_error;
        }

        if (qos >= 3) {
                VERBOSE_SESSION_WARNING("publish: wrong QOS value: %u!\n", qos);
                qos = 0;
        }

        mqtt_reason_t reason = 0;
        uint8_t *variable_header = NULL;
        uint8_t *msg = NULL;
        size_t msg_len = 0;
        bool msg_allocated = false;

        if (not payload) {
                int pos = llist_find_begin(global->topic_db.data, &(topicdb_t) {.topic = const_cast(topic)});

                if (pos >= 0) {
                        topicdb_t *topicdb = llist_at(global->topic_db.data, pos);
                        if (topicdb) {
                                msg_len = topicdb->msg_length;

                                if (topicdb->msg_length <= sizeof(session->msg_buf)) {
                                        msg = session->msg_buf;
                                        msg_allocated = false;
                                } else {
                                        msg = malloc(msg_len);
                                        msg_allocated = true;
                                }

                                if (msg and msg_len) {
                                        // message is copied to avoid change during send
                                        memcpy(msg, topicdb->msg, msg_len);
                                }
                        }
                }
        } else {
                msg = const_cast(payload);
                msg_len = payload_length;
                msg_allocated = false;
        }

        if (not msg or not msg_len) {
                reason = mqtt_reason__success;
                goto cleanup;
        }

        size_t topic_length_size = sizeof(uint16_t);
        size_t topic_length = strlen(topic);
        size_t identifier_length = (qos > 0) ? sizeof(uint16_t) : 0;
        size_t property_length_size = sizeof(uint8_t);
        size_t variable_header_len = topic_length_size + topic_length + identifier_length +
                property_length_size;

        variable_header = malloc(variable_header_len);
        if (not variable_header) {
                reason = mqtt_reason__implementation_specific_error;
                goto cleanup;
        }

        variable_header[0] = topic_length >> 8;
        variable_header[1] = topic_length;
        memcpy(&variable_header[2], topic, topic_length);

        if (qos > 0) {
                uint16_t identifier = ++global->topic_db.msg_id_ctr;
                if (identifier == 0) {
                        identifier = ++global->topic_db.msg_id_ctr;
                }

                variable_header[topic_length_size + topic_length + 0] = identifier >> 8;
                variable_header[topic_length_size + topic_length + 1] = identifier >> 0;
        }

        variable_header[topic_length_size + topic_length + identifier_length] = 0;

        if (session->protocol_version != mqtt_protocol_version__5p0) {
                variable_header_len = variable_header_len - 1;
        }

        mqtt_control_packet_t packet;
        packet.type = mqtt_msg_type__publish;
        packet.fixed_header_flags = qos << mqtt_publish_flag__qos_pos;
        packet.variable_header = variable_header;
        packet.variable_header_length = variable_header_len;
        packet.payload = msg;
        packet.payload_length = msg_len;

        reason = session_serialize_packet(session, &packet) == 0 ? mqtt_reason__success
                                                                 : mqtt_reason__implementation_specific_error;

        // printf("broker -> %s: %s:%.*s\n", session->client_id, topic, msg_len, msg);

cleanup:
        if (msg_allocated) {
                free(msg);
        }

        free(variable_header);

        return reason;
}

//==============================================================================
/**
 * @brief  Send subscribed topic if refreshed.
 *
 * @param  session              Pointer to the session structure containing connection data.
 */
//==============================================================================
static void session_send_subscribed_topics(session_t *session)
{
        if (session->publish_ctr == global->topic_db.publish_ctr) {
                return;
        }

        llist_foreach (topicsub_t *, sub, session->subscribtion_list) {
                if (sub->topicdb_ref->msg_update_tref > sub->msg_sent_tref) {

                        u8_t qos = sub->flags & mqtt_subscribe_flag__qos_mask;

                        mqtt_reason_t reason = session_publish_topic(session, qos, sub->topicdb_ref->topic,
                                sub->topicdb_ref->msg, sub->topicdb_ref->msg_length);

                        if (reason != mqtt_reason__success) {
                                VERBOSE_SESSION("unable to publish topic.");
                                break;
                        }

                        sub->msg_sent_tref = get_uptime_ms();
                }
        }

        session->publish_ctr = global->topic_db.publish_ctr;
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

        mqtt_reason_t reason = mqtt_reason__implementation_specific_error;
        topicsub_t *topicsub = NULL;

        int pos = -1;
        int i = 0;
        llist_foreach (topicsub_t *, sub, session->subscribtion_list) {
                if (sub->topicdb_ref and isstreq(sub->topicdb_ref->topic, topic)) {
                        pos = i;
                        break;
                }
                i++;
        }

        if (pos < 0) {
                if (llist_size(session->subscribtion_list) >= MAX_TOPICS) {
                        VERBOSE_SESSION_WARNING("server reached maximal amount of topics (%lu)!\n",
                                MAX_TOPICS);

                } else {
                        topicsub_t t = {
                                .topicdb_ref = NULL,
                                .flags = flags,
                                .msg_sent_tref = 0,
                        };

                        topicdb_update_topic(topic, NULL, 0, false, &t.topicdb_ref, true);

                        if (t.topicdb_ref) {
                                topicsub = llist_push_emplace_back(session->subscribtion_list, sizeof(t), &t);
                        }
                }

        } else {
                topicsub = llist_at(session->subscribtion_list, pos);
        }

        /*
         * Send existing data
         */
        // #define mqtt_subscribe_flag__retain_handling_mask   (3 << 4)
        // #define mqtt_subscribe_flag__retain_handling_always (0 << 4)
        // #define mqtt_subscribe_flag__retain_handling_avoid  (1 << 4)
        // #define mqtt_subscribe_flag__retain_handling_new    (2 << 4)
        // #define mqtt_subscribe_flag__retain_as_published    (1 << 3)
        if (topicsub) {
                topicsub->topicdb_ref->subscribers++;

                topicsub->flags = flags;

                if (flags & mqtt_subscribe_flag__no_local) {
                        reason = mqtt_reason__success;
                } else {
                        reason = session_publish_topic(session, flags & mqtt_subscribe_flag__qos_mask, topic,
                                NULL, 0);
                }

                topicsub->msg_sent_tref = get_uptime_ms();
        }

        return reason;
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
        mqtt_reason_t reason = 0;

        if (isstreq(topic, "#")) {
                llist_clear(session->subscribtion_list);

        } else {
                llist_foreach (topicsub_t *, sub, session->subscribtion_list) {
                        if (sub->topicdb_ref and isstreq(sub->topicdb_ref->topic, topic)) {
                                llist_erase_by_iterator(&_iterator);
                                break;
                        }
                }
        }

        return reason;
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
 * @param  retain        Whether the message should be retained by the broker for future subscribers.
 *
 * @return Returns MQTT reason code indicating success or failure in publishing the message.
 */
//==============================================================================
static mqtt_reason_t topicdb_update_topic(const char *topic, const uint8_t *msg_ref, size_t msg_len,
        bool retain, topicdb_t **topicdb_ref, bool create_only)
{
        mqtt_reason_t reason = mqtt_reason__implementation_specific_error;

        if (strchr(topic, '#') or strchr(topic, '+')) {
                return mqtt_reason__wildcard_subscriptions_not_supported;
        }

        topicdb_t *topicdb = NULL;

        int pos = llist_find_begin(global->topic_db.data, &(topicdb_t) {.topic = const_cast(topic)});
        if (pos < 0) {
                if (llist_size(global->topic_db.data) >= MAX_TOPICS) {
                        VERBOSE("server reached maximal amount of topics (%lu)!\n", MAX_TOPICS);

                } else {
                        topicdb_t t = {0};
                        t.topic = strdup(topic);
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

        if (topicdb and not create_only) {
                if (msg_len == 0) {
                        free(topicdb->msg);
                        topicdb->msg = NULL;
                        topicdb->msg_length = 0;
                        topicdb->msg_alloc_length = 0;

                } else if (topicdb->msg_alloc_length < msg_len) {
                        free(topicdb->msg);
                        topicdb->msg = malloc(msg_len);
                        if (topicdb->msg) {
                                topicdb->msg_length = msg_len;
                                topicdb->msg_alloc_length = msg_len;
                        } else {
                                topicdb->msg_length = 0;
                                topicdb->msg_alloc_length = 0;
                        }

                } else if (topicdb->msg_alloc_length >= msg_len) {
                        topicdb->msg_length = msg_len;
                }

                if (topicdb->msg) {
                        memcpy(topicdb->msg, msg_ref, topicdb->msg_length);
                }

                topicdb->retain = retain;
                topicdb->msg_update_tref = get_uptime_ms();
                reason = 0;

                // printf("publish: %s:%.*s\n", topic, msg_len, msg_ref);

                global->topic_db.publish_ctr++;
        }

        if (topicdb_ref) {
                *topicdb_ref = topicdb;
        }

        return reason;
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
        sa->msg = NULL;
        sa->msg_length = 0;
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
