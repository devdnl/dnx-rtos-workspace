/*==============================================================================
@file   ftpd.c

@author Daniel Zorychta

@brief  The FTP server.

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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dnx/net.h>
#include <dnx/misc.h>
#include <dnx/os.h>
#include <dnx/thread.h>
#include <conf_parser.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define DEFAULT_PORT      21
#define DEFAULT_PASV_PORT 2121

#define CONFIG_PARAM "--config="

#define CONFIG_KEY_DEBUG     "debug"
#define CONFIG_KEY_PORT      "port"
#define CONFIG_KEY_PASV_PORT "pasv-port"
#define CONFIG_KEY_USER      "user"
#define CONFIG_KEY_PASSWORD  "password"
#define CONFIG_KEY_ROOT_DIR  "root-dir"

#define TIMEOUT_SEND   5000
#define TIMEOUT_RECV   5000
#define TIMEOUT_ACCEPT 10000
#define TIMEOUT_DRAIN  1000

#define PATH_LEN     256
#define CMD_LINE_LEN 512

#define VIRT_PATH(_self) ((_self)->path + global->root_len) // client view of self->path

#define VERBOSE(...)                 \
        if (global->verbose) {       \
                printf(__VA_ARGS__); \
        }

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
typedef enum {
        FTP_CMD__ABOR, //!< abort a file transfer
        FTP_CMD__CWD,  //!< change working directory
        FTP_CMD__DELE, //!< delete a remote file
        FTP_CMD__LIST, //!< list remote files
        FTP_CMD__MDTM, //!< return the modification time of a file
        FTP_CMD__MKD,  //!< make a remote directory
        FTP_CMD__NLST, //!< name list of remote directory
        FTP_CMD__PASS, //!< send password
        FTP_CMD__PASV, //!< enter passive mode
        FTP_CMD__PORT, //!< open a data port
        FTP_CMD__PWD,  //!< print working directory
        FTP_CMD__QUIT, //!< terminate the connection
        FTP_CMD__RETR, //!< retrieve a remote file
        FTP_CMD__RMD,  //!< remove a remote directory
        FTP_CMD__RNFR, //!< rename from
        FTP_CMD__RNTO, //!< rename to
        FTP_CMD__SITE, //!< site-specific commands
        FTP_CMD__SIZE, //!< return the size of a file
        FTP_CMD__STOR, //!< store a file on the remote host
        FTP_CMD__TYPE, //!< set transfer type
        FTP_CMD__USER, //!< send username
        // Less common commands
        FTP_CMD__ACCT, //!< *send account information
        FTP_CMD__APPE, //!< append to a remote file
        FTP_CMD__CDUP, //!< CWD to the parent of the current directory
        FTP_CMD__HELP, //!< return help on using the server
        FTP_CMD__MODE, //!< set transfer mode
        FTP_CMD__NOOP, //!< do nothing
        FTP_CMD__REIN, //!< *reinitialize the connection
        FTP_CMD__STAT, //!< return server status
        FTP_CMD__STOU, //!< store a file uniquely
        FTP_CMD__STRU, //!< set file transfer structure
        FTP_CMD__SYST, //!< return system type
        FTP_CMD__FEAT, //!< list supported features (RFC 2389)
        FTP_CMD__OPTS, //!< set options (RFC 2389)
        FTP_CMD__EPSV, //!< extended passive mode (RFC 2428)
        FTP_CMD__XCWD, //!< RFC 775 alias of CWD (Windows ftp.exe)
        FTP_CMD__XCUP, //!< RFC 775 alias of CDUP (Windows ftp.exe)
        FTP_CMD__XMKD, //!< RFC 775 alias of MKD (Windows ftp.exe)
        FTP_CMD__XPWD, //!< RFC 775 alias of PWD (Windows ftp.exe)
        FTP_CMD__XRMD, //!< RFC 775 alias of RMD (Windows ftp.exe)
        FTP_CMD__COUNT
} FTP_cmd_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        bool verbose;
        conf_parser_t *conf;
        const char *user;
        const char *password;
        char root_dir[PATH_LEN];
        size_t root_len;
        uint16_t port;
        uint16_t pasv_port;

        struct thread {
                tid_t id;
                bool busy;                     // slot in use
                uint8_t slot;                  // slot index (selects PASV port)
                SOCKET *socket;                // cmd channel
                SOCKET *sock_pasv;             // data channel (passive mode listener)
                NET_INET_sockaddr_t port_addr; // data channel address (active mode)
                bool active_mode;              // PORT command received
                bool pasv_pending;             // PASV/EPSV command received
                bool user_ok;                  // correct USER received
                bool logged_in;                // correct PASS received
                bool discard_line;             // too long command line is discarded
                bool quit;
                char *RNFR;
                char *cmd_args;
                uint16_t data_port;
                size_t cmd_len;
                char cmd[CMD_LINE_LEN];
                char CWD[PATH_LEN];
                char path[2 * PATH_LEN]; // real path: root-dir + client path
                char buf[8192];
        } thread[8];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(ftpd, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Send reply to the control connection.
 * @param  self         thread object
 * @param  msg          reply (with CRLF)
 */
//==============================================================================
static void reply(struct thread *self, const char *msg)
{
        socket_send(self->socket, msg, strlen(msg), NET_FLAGS__COPY);
        VERBOSE("%s", msg);
}

//==============================================================================
/**
 * @brief  Send formatted reply to the control connection.
 * @param  self         thread object
 * @param  fmt          format
 * @param  ...          arguments
 */
//==============================================================================
static void replyf(struct thread *self, const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(self->buf, sizeof(self->buf), fmt, args);
        va_end(args);

        if (n > 0) {
                n = min(n, cast(int, sizeof(self->buf)) - 1);
                socket_send(self->socket, self->buf, n, NET_FLAGS__COPY);
                VERBOSE("%s", self->buf);
        }
}

//==============================================================================
/**
 * @brief  Normalize absolute path in place (removes "//", "." and "..").
 * @param  path         absolute path
 */
//==============================================================================
static void normalize_path(char *path)
{
        char *r = path;
        char *w = path;

        while (*r) {
                while (*r == '/') {
                        r++;
                }

                if (*r == '\0') {
                        break;
                }

                const char *seg = r;
                while (*r and (*r != '/')) {
                        r++;
                }

                size_t seglen = r - seg;

                if ((seglen == 1) and (seg[0] == '.')) {
                        continue;
                }

                if ((seglen == 2) and (seg[0] == '.') and (seg[1] == '.')) {
                        while ((w > path) and (*--w != '/')) {
                        }
                        continue;
                }

                *w++ = '/';
                memmove(w, seg, seglen);
                w += seglen;
        }

        if (w == path) {
                *w++ = '/';
        }

        *w = '\0';
}

//==============================================================================
/**
 * @brief  Create real path from command argument and current directory.
 *         Client path (absolute or relative to CWD) is normalized first, so
 *         ".." cannot leave the client root, then root-dir is prepended.
 *         Client part of the result is available by VIRT_PATH().
 * @param  self         thread object
 * @param  arg          path argument (absolute or relative, may be empty)
 * @param  out          output buffer
 * @param  outsz        output buffer size
 * @return On success true is returned, otherwise false (path too long).
 */
//==============================================================================
static bool resolve_path(struct thread *self, const char *arg, char *out, size_t outsz)
{
        size_t vsz = min(outsz, cast(size_t, PATH_LEN));
        int len;

        if ((arg == NULL) or (*arg == '\0')) {
                len = snprintf(out, vsz, "%s", self->CWD);
        } else if (arg[0] == '/') {
                len = snprintf(out, vsz, "%s", arg);
        } else {
                len = snprintf(out, vsz, "%s/%s", self->CWD, arg);
        }

        if ((len < 0) or (cast(size_t, len) >= vsz)) {
                return false;
        }

        normalize_path(out);

        if (global->root_len > 0) {
                size_t vlen = strlen(out);

                if ((global->root_len + vlen) >= outsz) {
                        return false;
                }

                memmove(out + global->root_len, out, vlen + 1);
                memcpy(out, global->root_dir, global->root_len);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Resolve command argument to self->path. Send error reply on failure.
 * @param  self         thread object
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool resolve_arg(struct thread *self)
{
        if (resolve_path(self, self->cmd_args, self->path, sizeof(self->path))) {
                return true;
        } else {
                reply(self, "553 Path too long\r\n");
                return false;
        }
}

//==============================================================================
/**
 * @brief  Check if path is a directory.
 * @param  path         path
 * @param  st           stat object (filled if stat() succeeded)
 * @param  exists       set to true if file/dir exists
 * @return True if path is directory.
 */
//==============================================================================
static bool is_dir(const char *path, struct stat *st, bool *exists)
{
        memset(st, 0, sizeof(*st));

        if (stat(path, st) == 0) {
                *exists = true;
                return S_ISDIR(st->st_mode);
        }

        // some paths (e.g. mount points) cannot be stated but can be opened
        DIR *dir = opendir(path);
        if (dir) {
                closedir(dir);
                *exists = true;
                return true;
        }

        *exists = false;
        return false;
}

//==============================================================================
/**
 * @brief  Open data connection (active or passive mode).
 * @param  self         thread object
 * @return On success socket is returned, otherwise NULL.
 */
//==============================================================================
static SOCKET *data_open(struct thread *self)
{
        SOCKET *dsock = NULL;

        if (self->active_mode) {
                dsock = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
                if (dsock) {
                        if (socket_connect(dsock, &self->port_addr, sizeof(self->port_addr)) != 0) {
                                VERBOSE("Data connect error: %s\n", strerror(errno));
                                socket_close(dsock);
                                dsock = NULL;
                        }
                }

        } else if (self->sock_pasv) {
                socket_set_recv_timeout(self->sock_pasv, TIMEOUT_ACCEPT);
                if (socket_accept(self->sock_pasv, &dsock) != 0) {
                        VERBOSE("Data accept error: %s\n", strerror(errno));
                        dsock = NULL;
                }
        }

        self->active_mode = false;
        self->pasv_pending = false;

        return dsock;
}

//==============================================================================
/**
 * @brief  Drop data connection prepared for transfer that will not happen
 *         (command failed). Client usually connects to the passive port
 *         directly after PASV reply, so not accepted connection would be
 *         taken by the next transfer.
 * @param  self         thread object
 */
//==============================================================================
static void data_drain(struct thread *self)
{
        if (self->pasv_pending and self->sock_pasv) {
                SOCKET *dsock = NULL;
                socket_set_recv_timeout(self->sock_pasv, TIMEOUT_DRAIN);
                if (socket_accept(self->sock_pasv, &dsock) == 0) {
                        socket_close(dsock);
                }
        }

        self->active_mode = false;
        self->pasv_pending = false;
}

//==============================================================================
/**
 * @brief  Send one list entry.
 * @param  self         thread object
 * @param  socket       data socket
 * @param  name         file name
 * @param  mode         file mode
 * @param  size         file size
 * @param  mtime        modification time
 * @param  names_only   NLST format
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool send_list_entry(struct thread *self, SOCKET *socket, const char *name, mode_t mode, u64_t size,
        time_t mtime, bool names_only)
{
        int sz;

        if (names_only) {
                sz = snprintf(self->buf, sizeof(self->buf), "%s\r\n", name);

        } else {
                struct tm tm;
                localtime_r(&mtime, &tm);

                sz = snprintf(self->buf, sizeof(self->buf), "%s 1 root root %10u ",
                        S_ISDIR(mode) ? "drwxr-xr-x" : "-rw-r--r--", cast(uint, size));

                sz += strftime(self->buf + sz, sizeof(self->buf) - sz, "%b %d  %Y ", &tm);

                sz += snprintf(self->buf + sz, sizeof(self->buf) - sz, "%s\r\n", name);
        }

        sz = min(sz, cast(int, sizeof(self->buf)) - 1);

        if (socket_send(socket, self->buf, sz, NET_FLAGS__COPY) == sz) {
                VERBOSE("%s", self->buf);
                return true;
        } else {
                return false;
        }
}

//==============================================================================
/**
 * @brief  Send file list of self->path.
 * @param  self         thread object
 * @param  socket       data socket where list to be sent
 * @param  dir          path is a directory
 * @param  st           stat of path (used when path is a file)
 * @param  names_only   NLST format
 * @return On success 0 is returned, otherwise other value.
 */
//==============================================================================
static int send_file_list(struct thread *self, SOCKET *socket, bool dir, const struct stat *st,
        bool names_only)
{
        socket_set_send_timeout(socket, TIMEOUT_SEND);

        if (not dir) {
                const char *name = strrchr(self->path, '/');
                name = name ? name + 1 : self->path;

                return send_list_entry(self, socket, name, st->st_mode, st->st_size, st->st_mtime, names_only)
                        ? 0
                        : EIO;
        }

        int err = 0;

        char *fpath = malloc(3 * PATH_LEN);
        DIR *dirp = opendir(self->path);
        if (dirp and fpath) {
                dirent_t *dirent = NULL;
                while ((dirent = readdir(dirp))) {

                        struct stat fsta;
                        memset(&fsta, 0, sizeof(struct stat));

                        if (not names_only) {
                                snprintf(fpath, 3 * PATH_LEN, "%s/%s",
                                        isstreq(self->path, "/") ? "" : self->path, dirent->d_name);
                                stat(fpath, &fsta);
                        }

                        if (not send_list_entry(self, socket, dirent->d_name, dirent->mode, dirent->size,
                                    fsta.st_mtime, names_only)) {
                                err = EIO;
                                break;
                        }
                }

        } else {
                err = errno ? errno : ENOMEM;
        }

        if (dirp) {
                closedir(dirp);
        }

        if (fpath) {
                free(fpath);
        }

        return err;
}

//==============================================================================
/**
 * @brief  Receive file data.
 * @param  self         thread object
 * @param  file         destination file
 * @param  socket       socket from data to be received
 * @return On success 0 is returned, otherwise other value.
 */
//==============================================================================
static int receive_file(struct thread *self, FILE *file, SOCKET *socket)
{
        socket_set_recv_timeout(socket, TIMEOUT_RECV);

        int err = 0;
        bool xfer_end = false;

        while (not err and not xfer_end) {
                size_t buflen = sizeof(self->buf);
                size_t blksz = 0;
                char *buf = self->buf;

                while (buflen > 0) {
                        int sz = socket_read(socket, buf, buflen);
                        if (sz > 0) {
                                buflen -= sz;
                                buf += sz;
                                blksz += sz;

                        } else if (sz == 0) {
                                break;

                        } else {
                                if (errno == ECONNABORTED) {
                                        VERBOSE("Connection closed - OK\n");
                                } else {
                                        VERBOSE("Connection error %d\n", errno);
                                        err = errno ? errno : EIO;
                                }

                                xfer_end = true;
                                break;
                        }
                }

                if (not err and (blksz > 0)) {
                        if (fwrite(self->buf, 1, blksz, file) != blksz) {
                                VERBOSE("File write error %d\n", errno);
                                err = errno ? errno : EIO;
                        }
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Transmit file to via selected socket.
 * @param  self         thread object
 * @param  file         source file
 * @param  socket       socket by which data to be transferred
 * @return On success 0 is returned, otherwise other value.
 */
//==============================================================================
static int transmit_file(struct thread *self, FILE *file, SOCKET *socket)
{
        socket_set_send_timeout(socket, TIMEOUT_SEND);

        struct stat stat;
        int err = fstat(fileno(file), &stat);

        while (not err and (stat.st_size > 0)) {

                size_t len = fread(self->buf, 1, sizeof(self->buf), file);
                if (len == 0) {
                        VERBOSE("File end reached\n");
                        break;
                }

                stat.st_size -= min(cast(u64_t, len), cast(u64_t, stat.st_size));

                NET_flags_t flags = NET_FLAGS__COPY;

                if (stat.st_size) {
                        flags |= NET_FLAGS__MORE;
                }

                int n = socket_send(socket, self->buf, len, flags);

                if (n < 0) {
                        VERBOSE("Socket write error %d\n", errno);
                }

                err = (n != cast(int, len));
        }

        return err;
}

//==============================================================================
/**
 * @brief  Handle USER command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_USER(struct thread *self)
{
        self->logged_in = false;
        self->user_ok = isstreq(self->cmd_args, global->user);

        if (self->user_ok) {
                reply(self, "331 Enter user password\r\n");
        } else {
                reply(self, "530 Unknown user\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle PASS command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_PASS(struct thread *self)
{
        if (not self->user_ok) {
                reply(self, "503 Login with USER first\r\n");

        } else if (isstreq(self->cmd_args, global->password)) {
                self->logged_in = true;
                reply(self, "230 Hello!\r\n");

        } else {
                self->user_ok = false;
                reply(self, "530 Incorrect password!\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle QUIT command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_QUIT(struct thread *self)
{
        reply(self, "221 Bye\r\n");
        self->quit = true;
}

//==============================================================================
/**
 * @brief  Handle SYST command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_SYST(struct thread *self)
{
        reply(self, "215 UNIX Type: L8\r\n");
}

//==============================================================================
/**
 * @brief  Handle FEAT command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_FEAT(struct thread *self)
{
        reply(self,
                "211-Features:\r\n"
                " EPSV\r\n"
                " MDTM\r\n"
                " PASV\r\n"
                " SIZE\r\n"
                " UTF8\r\n"
                "211 End\r\n");
}

//==============================================================================
/**
 * @brief  Handle OPTS command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_OPTS(struct thread *self)
{
        if (strcasecmp(self->cmd_args, "UTF8 ON") == 0) {
                reply(self, "200 UTF8 mode enabled\r\n");
        } else {
                reply(self, "501 Option not supported\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle PWD command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_PWD(struct thread *self)
{
        replyf(self, "257 \"%s\" is current directory\r\n", self->CWD);
}

//==============================================================================
/**
 * @brief  Handle TYPE command.
 *         ASCII type is accepted for compatibility (most clients request it
 *         for directory listings), but data are always transferred as is.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_TYPE(struct thread *self)
{
        if ((strcasecmp(self->cmd_args, "I") == 0) or (strcasecmp(self->cmd_args, "L 8") == 0)) {
                reply(self, "200 Type set to I\r\n");

        } else if ((strcasecmp(self->cmd_args, "A") == 0) or (strcasecmp(self->cmd_args, "A N") == 0)) {
                reply(self, "200 Type set to A\r\n");

        } else {
                reply(self, "504 Type not supported\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle PASV and EPSV commands.
 * @param  self         thread object
 * @param  extended     true: EPSV; false: PASV
 */
//==============================================================================
static void handle_cmd_PASV(struct thread *self, bool extended)
{
        if (not self->sock_pasv) {
                reply(self, "425 Can't open data connection\r\n");
                return;
        }

        if (extended) {
                if (strcasecmp(self->cmd_args, "ALL") == 0) {
                        reply(self, "200 EPSV ALL accepted\r\n");
                        return;
                }

                replyf(self, "229 Entering Extended Passive Mode (|||%u|)\r\n", cast(uint, self->data_port));
                self->active_mode = false;
                self->pasv_pending = true;
                return;
        }

        NET_INET_status_t status;
        memset(&status, 0, sizeof(status));
        NET_family_t family;

        if (ifstatus("inet", &family, &status, sizeof(status)) == 0) {

                replyf(self, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n",
                        NET_INET_IPv4_a(status.address), NET_INET_IPv4_b(status.address),
                        NET_INET_IPv4_c(status.address), NET_INET_IPv4_d(status.address),
                        (self->data_port >> 8) & 0xFF, (self->data_port & 0xFF));

                self->active_mode = false;
                self->pasv_pending = true;

        } else {
                reply(self, "425 Can't open data connection\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle PORT command. The IP address from the argument is ignored
 *         and the address of the control connection peer is used instead
 *         (prevents FTP bounce attack).
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_PORT(struct thread *self)
{
        uint32_t val[6];
        const char *p = self->cmd_args;

        for (int i = 0; i < 6; i++) {
                if (not isdigit(cast(unsigned char, *p))) {
                        reply(self, "501 Syntax error in parameters\r\n");
                        return;
                }

                val[i] = 0;
                while (isdigit(cast(unsigned char, *p))) {
                        val[i] = (val[i] * 10) + (*p++ - '0');
                        if (val[i] > 255) {
                                reply(self, "501 Syntax error in parameters\r\n");
                                return;
                        }
                }

                if ((i < 5) and (*p++ != ',')) {
                        reply(self, "501 Syntax error in parameters\r\n");
                        return;
                }
        }

        NET_INET_sockaddr_t peer;
        memset(&peer, 0, sizeof(peer));

        if (socket_get_address(self->socket, &peer, sizeof(peer)) != 0) {
                reply(self, "425 Can't open data connection\r\n");
                return;
        }

        self->port_addr.addr = peer.addr;
        self->port_addr.port = (val[4] << 8) | val[5];
        self->active_mode = true;
        self->pasv_pending = false;

        reply(self, "200 PORT command successful\r\n");
}

//==============================================================================
/**
 * @brief  Handle LIST and NLST commands.
 * @param  self         thread object
 * @param  names_only   true: NLST; false: LIST
 */
//==============================================================================
static void handle_cmd_LIST(struct thread *self, bool names_only)
{
        // skip ls-like options (e.g. "LIST -a", "LIST -la /dir")
        const char *arg = self->cmd_args;
        while (*arg == '-') {
                while (*arg and (*arg != ' ')) {
                        arg++;
                }

                while (*arg == ' ') {
                        arg++;
                }
        }

        if (not resolve_path(self, arg, self->path, sizeof(self->path))) {
                data_drain(self);
                reply(self, "553 Path too long\r\n");
                return;
        }

        struct stat st;
        bool exists;
        bool dir = is_dir(self->path, &st, &exists);

        if (not exists) {
                data_drain(self);
                reply(self, "550 No such file or directory\r\n");
                return;
        }

        reply(self, "150 Opening data connection\r\n");

        SOCKET *dsock = data_open(self);
        if (dsock) {
                int err = send_file_list(self, dsock, dir, &st, names_only);
                socket_close(dsock);

                reply(self, err ? "426 Transfer aborted\r\n" : "226 Done\r\n");
        } else {
                reply(self, "425 Can't open data connection\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle CWD and CDUP commands.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_CWD(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        struct stat st;
        bool exists;

        if (is_dir(self->path, &st, &exists)) {
                strlcpy(self->CWD, VIRT_PATH(self), sizeof(self->CWD));
                reply(self, "250 Directory changed\r\n");
        } else {
                reply(self, "550 No such directory\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle MKD command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_MKD(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        if (mkdir(self->path, 0777) == 0) {
                replyf(self, "257 \"%s\" created\r\n", VIRT_PATH(self));
        } else {
                reply(self, "550 Failed\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle DELE and RMD commands.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_DELE_RMD(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        if (isstreq(VIRT_PATH(self), "/")) {
                reply(self, "550 Permission denied\r\n");

        } else if (remove(self->path) == 0) {
                reply(self, "250 File removed\r\n");
        } else {
                reply(self, "550 Failed\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle SIZE command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_SIZE(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        struct stat st;
        if ((stat(self->path, &st) == 0) and not S_ISDIR(st.st_mode)) {
                replyf(self, "213 %u\r\n", cast(uint, st.st_size));
        } else {
                reply(self, "550 No such file\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle MDTM command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_MDTM(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        struct stat st;
        if ((stat(self->path, &st) == 0) and not S_ISDIR(st.st_mode)) {
                time_t t = st.st_mtime;
                struct tm tm;
                gmtime_r(&t, &tm);

                replyf(self, "213 %04d%02d%02d%02d%02d%02d\r\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
        } else {
                reply(self, "550 No such file\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle STOR and APPE commands.
 * @param  self         thread object
 * @param  stor         true: STOR; false: APPE
 */
//==============================================================================
static void handle_cmd_STOR_APPE(struct thread *self, bool stor)
{
        if (not resolve_path(self, self->cmd_args, self->path, sizeof(self->path))) {
                data_drain(self);
                reply(self, "553 Path too long\r\n");
                return;
        }

        FILE *file = fopen(self->path, stor ? "w" : "a");
        if (not file) {
                data_drain(self);
                reply(self, "550 Can't create file\r\n");
                return;
        }

        reply(self, "150 Receiving data\r\n");

        SOCKET *dsock = data_open(self);
        if (dsock) {
                int err = receive_file(self, file, dsock);
                socket_close(dsock);

                reply(self, err ? "451 IO error\r\n" : "226 Done\r\n");
        } else {
                reply(self, "425 Can't open data connection\r\n");
        }

        fclose(file);
}

//==============================================================================
/**
 * @brief  Handle RETR command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_RETR(struct thread *self)
{
        if (not resolve_path(self, self->cmd_args, self->path, sizeof(self->path))) {
                data_drain(self);
                reply(self, "553 Path too long\r\n");
                return;
        }

        struct stat st;
        bool exists;
        FILE *file = NULL;

        if (not is_dir(self->path, &st, &exists) and exists) {
                file = fopen(self->path, "r");
        }

        if (not file) {
                data_drain(self);
                reply(self, "550 No such file\r\n");
                return;
        }

        reply(self, "150 Transmitting data\r\n");

        SOCKET *dsock = data_open(self);
        if (dsock) {
                int err = transmit_file(self, file, dsock);
                socket_close(dsock);

                reply(self, err ? "426 Connection error\r\n" : "226 Done\r\n");
        } else {
                reply(self, "425 Can't open data connection\r\n");
        }

        fclose(file);
}

//==============================================================================
/**
 * @brief  Handle RNFR command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_RNFR(struct thread *self)
{
        if (not resolve_arg(self)) {
                return;
        }

        struct stat st;
        bool exists;
        is_dir(self->path, &st, &exists);

        if (isstreq(VIRT_PATH(self), "/")) {
                reply(self, "550 Permission denied\r\n");

        } else if (exists) {
                if (self->RNFR) {
                        free(self->RNFR);
                }

                self->RNFR = malloc(strsize(self->path));
                if (self->RNFR) {
                        strcpy(self->RNFR, self->path);
                        reply(self, "350 Ready for RNTO\r\n");
                } else {
                        reply(self, "451 Not enough memory\r\n");
                }
        } else {
                reply(self, "550 No such file\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle RNTO command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_RNTO(struct thread *self)
{
        if (not self->RNFR) {
                reply(self, "503 Use RNFR first\r\n");
                return;
        }

        if (resolve_arg(self)) {
                if (isstreq(VIRT_PATH(self), "/")) {
                        reply(self, "550 Permission denied\r\n");
                } else if (rename(self->RNFR, self->path) == 0) {
                        reply(self, "250 File renamed\r\n");
                } else {
                        reply(self, "550 Rename error\r\n");
                }
        }

        free(self->RNFR);
        self->RNFR = NULL;
}

//==============================================================================
/**
 * @brief  Handle ABOR command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_ABOR(struct thread *self)
{
        reply(self, "226 ABOR command successful\r\n");
}

//==============================================================================
/**
 * @brief  Handle MODE command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_MODE(struct thread *self)
{
        if (strcasecmp(self->cmd_args, "s") == 0) {
                reply(self, "200 OK\r\n");
        } else {
                reply(self, "504 Rejected\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle NOOP command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_NOOP(struct thread *self)
{
        reply(self, "200 OK\r\n");
}

//==============================================================================
/**
 * @brief  Handle REIN command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_REIN(struct thread *self)
{
        self->user_ok = false;
        self->logged_in = false;
        strlcpy(self->CWD, "/", sizeof(self->CWD));

        if (self->RNFR) {
                free(self->RNFR);
                self->RNFR = NULL;
        }

        reply(self, "220 Service ready\r\n");
}

//==============================================================================
/**
 * @brief  Handle STAT command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_STAT(struct thread *self)
{
        reply(self, "502 Command not implemented\r\n");
}

//==============================================================================
/**
 * @brief  Handle STRU command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_STRU(struct thread *self)
{
        if (strcasecmp(self->cmd_args, "f") == 0) {
                reply(self, "200 OK\r\n");
        } else {
                reply(self, "504 Rejected\r\n");
        }
}

//==============================================================================
/**
 * @brief  Handle not implemented command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_not_implemented(struct thread *self)
{
        reply(self, "502 Command not implemented\r\n");
}

//==============================================================================
/**
 * @brief  Handle unknown command.
 * @param  self         thread object
 */
//==============================================================================
static void handle_cmd_unknown(struct thread *self)
{
        reply(self, "500 Unknown command\r\n");
}

//==============================================================================
/**
 * @brief  Receive one command line (terminated by LF) from control connection.
 *         The line is stored in self->cmd and terminated by '\0' (CR and LF
 *         are removed). Bytes received after the line are kept for the next
 *         call, so commands sent in one segment are not lost.
 * @param  self         thread object
 * @return Number of bytes to consume after command handling, 0 if the line
 *         was too long, or -1 on connection error.
 */
//==============================================================================
static int receive_cmd_line(struct thread *self)
{
        for (;;) {
                char *eol = memchr(self->cmd, '\n', self->cmd_len);
                if (eol) {
                        int consumed = (eol - self->cmd) + 1;

                        if (self->discard_line) {
                                self->discard_line = false;
                                memmove(self->cmd, eol + 1, self->cmd_len - consumed);
                                self->cmd_len -= consumed;
                                return 0;
                        }

                        *eol = '\0';
                        if ((eol > self->cmd) and (eol[-1] == '\r')) {
                                eol[-1] = '\0';
                        }

                        return consumed;
                }

                if (self->cmd_len >= sizeof(self->cmd) - 1) {
                        self->cmd_len = 0;
                        self->discard_line = true;
                }

                errno = 0;
                int n = socket_recv(self->socket, self->cmd + self->cmd_len,
                        sizeof(self->cmd) - 1 - self->cmd_len, NET_FLAGS__NONE);

                if (n < 0) {
                        return -1;
                }

                self->cmd_len += n;
        }
}

//==============================================================================
/**
 * @brief  Return number of command.
 * @param  self         thread object
 * @return Command ID.
 */
//==============================================================================
static FTP_cmd_t get_cmd(struct thread *self)
{
        static const char *CMD_STR[] = {
                [FTP_CMD__ABOR] = "ABOR",
                [FTP_CMD__CWD] = "CWD",
                [FTP_CMD__DELE] = "DELE",
                [FTP_CMD__LIST] = "LIST",
                [FTP_CMD__MDTM] = "MDTM",
                [FTP_CMD__MKD] = "MKD",
                [FTP_CMD__NLST] = "NLST",
                [FTP_CMD__PASS] = "PASS",
                [FTP_CMD__PASV] = "PASV",
                [FTP_CMD__PORT] = "PORT",
                [FTP_CMD__PWD] = "PWD",
                [FTP_CMD__QUIT] = "QUIT",
                [FTP_CMD__RETR] = "RETR",
                [FTP_CMD__RMD] = "RMD",
                [FTP_CMD__RNFR] = "RNFR",
                [FTP_CMD__RNTO] = "RNTO",
                [FTP_CMD__SITE] = "SITE",
                [FTP_CMD__SIZE] = "SIZE",
                [FTP_CMD__STOR] = "STOR",
                [FTP_CMD__TYPE] = "TYPE",
                [FTP_CMD__USER] = "USER",
                [FTP_CMD__ACCT] = "ACCT",
                [FTP_CMD__APPE] = "APPE",
                [FTP_CMD__CDUP] = "CDUP",
                [FTP_CMD__HELP] = "HELP",
                [FTP_CMD__MODE] = "MODE",
                [FTP_CMD__NOOP] = "NOOP",
                [FTP_CMD__REIN] = "REIN",
                [FTP_CMD__STAT] = "STAT",
                [FTP_CMD__STOU] = "STOU",
                [FTP_CMD__STRU] = "STRU",
                [FTP_CMD__SYST] = "SYST",
                [FTP_CMD__FEAT] = "FEAT",
                [FTP_CMD__OPTS] = "OPTS",
                [FTP_CMD__EPSV] = "EPSV",
                [FTP_CMD__XCWD] = "XCWD",
                [FTP_CMD__XCUP] = "XCUP",
                [FTP_CMD__XMKD] = "XMKD",
                [FTP_CMD__XPWD] = "XPWD",
                [FTP_CMD__XRMD] = "XRMD",
        };

        // skip Telnet IAC sequences sent by some clients before ABOR
        char *line = self->cmd;
        while (cast(unsigned char, *line) >= 0xF0) {
                line++;
        }

        VERBOSE("> %s\n", strncasecmp(line, "PASS", 4) == 0 ? "PASS ****" : line);

        size_t cmdlen = 0;
        while (line[cmdlen] and (line[cmdlen] != ' ')) {
                cmdlen++;
        }

        self->cmd_args = line + cmdlen;
        if (*self->cmd_args == ' ') {
                self->cmd_args++;
        }

        FTP_cmd_t cmd;
        for (cmd = FTP_CMD__ABOR; cmd < FTP_CMD__COUNT; cmd++) {
                if ((strlen(CMD_STR[cmd]) == cmdlen) and (strncasecmp(CMD_STR[cmd], line, cmdlen) == 0)) {
                        break;
                }
        }

        return cmd;
}

//==============================================================================
/**
 * @brief  Check if command can be executed without login.
 * @param  cmd          command
 * @return True if command is allowed.
 */
//==============================================================================
static bool is_cmd_allowed_without_login(FTP_cmd_t cmd)
{
        switch (cmd) {
        case FTP_CMD__USER:
        case FTP_CMD__PASS:
        case FTP_CMD__QUIT:
        case FTP_CMD__SYST:
        case FTP_CMD__FEAT:
        case FTP_CMD__OPTS:
        case FTP_CMD__NOOP:
        case FTP_CMD__HELP:
        case FTP_CMD__REIN:
        case FTP_CMD__COUNT:
                return true;
        default:
                return false;
        }
}

//==============================================================================
/**
 * @brief  FTP server.
 * @param  arg          thread self pointer
 */
//==============================================================================
static int serve(void *arg)
{
        struct thread *self = arg;

        VERBOSE("Thread-%u\n", self->slot);

        strlcpy(self->CWD, "/", sizeof(self->CWD));
        memset(self->buf, 0, sizeof(self->buf));
        self->data_port = global->pasv_port + self->slot;
        self->cmd_args = NULL;
        self->cmd_len = 0;
        self->discard_line = false;
        self->active_mode = false;
        self->pasv_pending = false;
        self->user_ok = false;
        self->logged_in = false;
        self->quit = false;

        if (!self->sock_pasv) {
                self->sock_pasv = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
                if (self->sock_pasv) {
                        NET_INET_sockaddr_t ADDR_ANY = {.addr = NET_INET_IPv4_ANY, .port = self->data_port};

                        int err = socket_bind(self->sock_pasv, &ADDR_ANY, sizeof(ADDR_ANY));
                        if (!err) {
                                err = socket_listen(self->sock_pasv);
                        }

                        if (err) {
                                VERBOSE("PASV socket error: %s\n", strerror(errno));
                                socket_close(self->sock_pasv);
                                self->sock_pasv = NULL;
                                goto finish;
                        }
                }
        }

        NET_INET_sockaddr_t addr;
        socket_get_address(self->socket, &addr, sizeof(addr));

        VERBOSE("New connection from: %d.%d.%d.%d\n", NET_INET_IPv4_a(addr.addr), NET_INET_IPv4_b(addr.addr),
                NET_INET_IPv4_c(addr.addr), NET_INET_IPv4_d(addr.addr));

        reply(self, "220 Service ready\r\n");

        while (not self->quit) {
                int consumed = receive_cmd_line(self);

                if (consumed < 0) {
                        VERBOSE("CMD stream: %s\n", strerror(errno));
                        break;

                } else if (consumed == 0) {
                        reply(self, "500 Line too long\r\n");
                        continue;
                }

                FTP_cmd_t cmd = get_cmd(self);

                if (not self->logged_in and not is_cmd_allowed_without_login(cmd)) {
                        reply(self, "530 Not logged in\r\n");

                } else {
                        switch (cmd) {
                        case FTP_CMD__ABOR:
                                handle_cmd_ABOR(self);
                                break;
                        case FTP_CMD__CWD:
                        case FTP_CMD__XCWD:
                                handle_cmd_CWD(self);
                                break;
                        case FTP_CMD__CDUP:
                        case FTP_CMD__XCUP:
                                self->cmd_args = "..";
                                handle_cmd_CWD(self);
                                break;
                        case FTP_CMD__DELE:
                        case FTP_CMD__RMD:
                        case FTP_CMD__XRMD:
                                handle_cmd_DELE_RMD(self);
                                break;
                        case FTP_CMD__LIST:
                                handle_cmd_LIST(self, false);
                                break;
                        case FTP_CMD__NLST:
                                handle_cmd_LIST(self, true);
                                break;
                        case FTP_CMD__MDTM:
                                handle_cmd_MDTM(self);
                                break;
                        case FTP_CMD__MKD:
                        case FTP_CMD__XMKD:
                                handle_cmd_MKD(self);
                                break;
                        case FTP_CMD__PASS:
                                handle_cmd_PASS(self);
                                break;
                        case FTP_CMD__PASV:
                                handle_cmd_PASV(self, false);
                                break;
                        case FTP_CMD__EPSV:
                                handle_cmd_PASV(self, true);
                                break;
                        case FTP_CMD__PORT:
                                handle_cmd_PORT(self);
                                break;
                        case FTP_CMD__PWD:
                        case FTP_CMD__XPWD:
                                handle_cmd_PWD(self);
                                break;
                        case FTP_CMD__QUIT:
                                handle_cmd_QUIT(self);
                                break;
                        case FTP_CMD__RETR:
                                handle_cmd_RETR(self);
                                break;
                        case FTP_CMD__RNFR:
                                handle_cmd_RNFR(self);
                                break;
                        case FTP_CMD__RNTO:
                                handle_cmd_RNTO(self);
                                break;
                        case FTP_CMD__SIZE:
                                handle_cmd_SIZE(self);
                                break;
                        case FTP_CMD__STOR:
                                handle_cmd_STOR_APPE(self, true);
                                break;
                        case FTP_CMD__APPE:
                                handle_cmd_STOR_APPE(self, false);
                                break;
                        case FTP_CMD__TYPE:
                                handle_cmd_TYPE(self);
                                break;
                        case FTP_CMD__USER:
                                handle_cmd_USER(self);
                                break;
                        case FTP_CMD__MODE:
                                handle_cmd_MODE(self);
                                break;
                        case FTP_CMD__NOOP:
                                handle_cmd_NOOP(self);
                                break;
                        case FTP_CMD__REIN:
                                handle_cmd_REIN(self);
                                break;
                        case FTP_CMD__STAT:
                                handle_cmd_STAT(self);
                                break;
                        case FTP_CMD__STRU:
                                handle_cmd_STRU(self);
                                break;
                        case FTP_CMD__SYST:
                                handle_cmd_SYST(self);
                                break;
                        case FTP_CMD__FEAT:
                                handle_cmd_FEAT(self);
                                break;
                        case FTP_CMD__OPTS:
                                handle_cmd_OPTS(self);
                                break;
                        case FTP_CMD__SITE:
                        case FTP_CMD__STOU:
                        case FTP_CMD__ACCT:
                        case FTP_CMD__HELP:
                                handle_cmd_not_implemented(self);
                                break;
                        default:
                                handle_cmd_unknown(self);
                                break;
                        }
                }

                // remove handled line from command buffer
                memmove(self->cmd, self->cmd + consumed, self->cmd_len - consumed);
                self->cmd_len -= consumed;
        }

        if (self->RNFR) {
                free(self->RNFR);
                self->RNFR = NULL;
        }

finish:
        VERBOSE("Connection closed.\n");
        socket_close(self->socket);
        self->socket = NULL;
        self->busy = false;

        return 0;
}

//==============================================================================
/**
 * @brief  Help message.
 *
 * @param  argv         arguments
 */
//==============================================================================
static void print_help(char *argv[])
{
        printf("Usage: %s args\n", argv[0]);
        printf("  -h, --help            this help\n");
        printf("  --config=<file>       configuration file\n");
        printf("  --wait-for-network    wait for network to be available\n");
        printf("  -v                    verbose mode (same as debug = true)\n");
        printf("\n");

        printf("Example configuration:\n");
        printf("user = \"user\"\n");
        printf("password = \"password\"\n");
        printf("port = %u\n", DEFAULT_PORT);
        printf("pasv-port = %u\n", DEFAULT_PASV_PORT);
        printf("root-dir = \"/sd/ftp\"\n");
        printf("debug = false\n");
        printf("\n");
        printf("Passive mode uses ports pasv-port .. pasv-port+%u.\n",
                cast(uint, ARRAY_SIZE(global->thread) - 1));
}

//==============================================================================
/**
 * @brief  Read port number from configuration.
 *
 * @param  key          configuration key
 * @param  def          default value (key not set)
 * @param  max          maximum allowed value
 * @param  port         read port
 *
 * @return On success true is returned, otherwise false.
 */
//==============================================================================
static bool get_conf_port(const char *key, uint16_t def, uint32_t max, uint16_t *port)
{
        const char *str = conf_parser__get_value(global->conf, key);
        if (not str) {
                *port = def;
                return true;
        }

        char *end = NULL;
        unsigned long val = strtoul(str, &end, 0);
        if ((end == str) or (*end != '\0') or (val == 0) or (val > max)) {
                return false;
        }

        *port = val;
        return true;
}

//==============================================================================
/**
 * @brief Program main function
 *
 * @param  argc         count of arguments
 * @param *argv[]       argument table
 *
 * @return program status
 */
//==============================================================================
int main(int argc, char *argv[])
{
        bool wait_for_network = false;
        const char *conf_path = NULL;

        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "-h") or isstreq(argv[i], "--help")) {
                        print_help(argv);
                        return EXIT_SUCCESS;
                } else if (isstreq(argv[i], "-v")) {
                        global->verbose = true;
                } else if (isstreq(argv[i], "--wait-for-network")) {
                        wait_for_network = true;
                } else if (isstreqn(argv[i], CONFIG_PARAM, strlen(CONFIG_PARAM))) {
                        conf_path = argv[i] + strlen(CONFIG_PARAM);
                }
        }

        if (not conf_path) {
                printf("[%s, info] no configuration file given!\n", argv[0]);
                print_help(argv);
                return EXIT_FAILURE;
        }

        global->conf = conf_parser__new(conf_path);
        if (not global->conf) {
                printf("[%s, info] no such file '%s'!\n", argv[0], conf_path);
                return EXIT_FAILURE;
        }

        int exit_code = EXIT_FAILURE;

        if (conf_parser__is_value_true(conf_parser__get_value(global->conf, CONFIG_KEY_DEBUG))) {
                global->verbose = true;
        }

        global->user = conf_parser__get_value(global->conf, CONFIG_KEY_USER);
        global->password = conf_parser__get_value(global->conf, CONFIG_KEY_PASSWORD);

        if (not global->user or not global->password or (*global->user == '\0') or
                (*global->password == '\0')) {
                printf("[%s, info] user and password must be configured!\n", argv[0]);
                goto cleanup;
        }

        const char *root_dir = conf_parser__get_value(global->conf, CONFIG_KEY_ROOT_DIR);
        if (not root_dir or (root_dir[0] != '/') or (strlen(root_dir) >= sizeof(global->root_dir))) {
                printf("[%s, info] root-dir must be set to an absolute path!\n", argv[0]);
                goto cleanup;
        }

        strlcpy(global->root_dir, root_dir, sizeof(global->root_dir));
        normalize_path(global->root_dir);

        DIR *dir = opendir(global->root_dir);
        if (not dir) {
                printf("[%s, info] root-dir '%s' is not accessible!\n", argv[0], global->root_dir);
                goto cleanup;
        }
        closedir(dir);

        // "/" is not prepended to client paths
        global->root_len = isstreq(global->root_dir, "/") ? 0 : strlen(global->root_dir);

        if (not get_conf_port(CONFIG_KEY_PORT, DEFAULT_PORT, UINT16_MAX, &global->port)) {
                printf("[%s, info] invalid port value!\n", argv[0]);
                goto cleanup;
        }

        if (not get_conf_port(CONFIG_KEY_PASV_PORT, DEFAULT_PASV_PORT,
                    UINT16_MAX - (ARRAY_SIZE(global->thread) - 1), &global->pasv_port)) {
                printf("[%s, info] invalid pasv-port value!\n", argv[0]);
                goto cleanup;
        }

        if ((global->port >= global->pasv_port) and
                (global->port < global->pasv_port + ARRAY_SIZE(global->thread))) {
                printf("[%s, info] port overlaps with passive ports range!\n", argv[0]);
                goto cleanup;
        }

        exit_code = EXIT_SUCCESS;

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

        SOCKET *socket = socket_open("inet", NET_SOCKET_TYPE__STREAM, 0);
        if (socket) {
                const NET_INET_sockaddr_t ADDR_ANY = {.addr = NET_INET_IPv4_ANY, .port = global->port};

                if (socket_bind(socket, &ADDR_ANY, sizeof(ADDR_ANY)) == 0) {

                        if (socket_listen(socket) == 0) {
                                int err;
                                do {
                                        SOCKET *new_socket;
                                        err = socket_accept(socket, &new_socket);
                                        if (!err) {

                                                bool started = false;

                                                for (size_t i = 0; i < ARRAY_SIZE(global->thread); i++) {

                                                        struct thread *t = &global->thread[i];

                                                        if (not t->busy) {

                                                                thread_attr_t attr;
                                                                attr.detached = false;
                                                                attr.priority = 0;
                                                                attr.stack_depth = STACK_DEPTH_MEDIUM;

                                                                // prepare slot before thread start
                                                                t->socket = new_socket;
                                                                t->slot = i;
                                                                t->busy = true;

                                                                t->id = thread_create(serve, &attr, t);
                                                                if (t->id) {
                                                                        started = true;
                                                                } else {
                                                                        t->socket = NULL;
                                                                        t->busy = false;
                                                                }

                                                                break;
                                                        }
                                                }

                                                if (!started) {
                                                        VERBOSE("No empty connection slots\n");
                                                        socket_close(new_socket);
                                                }
                                        }
                                } while (!err);
                        }
                }

                socket_close(socket);
        }

        for (size_t i = 0; i < ARRAY_SIZE(global->thread); i++) {
                if (global->thread[i].busy) {
                        thread_cancel(global->thread[i].id);
                }
        }

        if (errno != 0) {
                perror("Socket error");
                exit_code = EXIT_FAILURE;
        }

cleanup:
        conf_parser__delete(global->conf);

        return exit_code;
}

/*==============================================================================
  End of file
==============================================================================*/
