/*==============================================================================
File    auth.c

Author  Daniel Zorychta

Brief   User authentication: passwd and shadow files, PBKDF2-SHA256 hashes, limit of failed logins.

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
#include "auth.h"
#include "sha256.h"

#include <dnx/os.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define SCHEME       "$pbkdf2-sha256$"
#define SALT_LEN     16
#define SALT_MAX     32
#define HASH_LEN     32
#define LINE_LEN     256
#define SERVICES_MAX 64
#define GECOS_MAX    64
#define HASH_TEXT    112
#define ID_MAX       65535
#define FIRST_UID    1000
#define TMP_SUFFIX   ".tmp"
#define LOCK_TRIES   30 /* lock (temporary shadow file) is retried every 100 ms */

/*==============================================================================
  Local object types
==============================================================================*/
/* password line (shadow file) */
typedef struct {
        char user[AUTH_USER_MAX + 1];
        char services[SERVICES_MAX + 1];
        char hash_text[HASH_TEXT]; /* hash field of line */
        uint32_t iterations;
        uint8_t salt[SALT_MAX];
        size_t salt_len;
        uint8_t hash[HASH_LEN];
} shadow_t;

/* work memory (allocated: services have small stacks) */
typedef struct {
        char line[LINE_LEN];
        char copy[LINE_LEN];
        char gecos[GECOS_MAX + 1];
        shadow_t sh;
        shadow_t sh_copy;
        auth_user_t pw;
        auth_user_t pw_copy;
        SHA256_CTX ictx;
        SHA256_CTX octx;
        SHA256_CTX ctx;
        uint8_t pad[64];
        uint8_t u[HASH_LEN];
        uint8_t dk[HASH_LEN];
} work_t;

/* change of files */
typedef enum {
        CHANGE_PASSWORD,
        CHANGE_ACCOUNT,
        CHANGE_REMOVE,
} change_op_t;

typedef struct {
        change_op_t op;
        const char *user;
        const char *password;
        const char *services;
        uint32_t iterations;
        int uid;
        int gid;
        const char *home;
        const char *shell;
} change_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static bool use_rw_files(void);
static FILE *db_open(bool shadow);
static bool parse_passwd(char *line, auth_user_t *u, char *gecos);
static bool parse_shadow(char *line, shadow_t *e);
static int passwd_find(work_t *w, const char *name, uid_t uid);
static int shadow_find(work_t *w, const char *user, const char *service, const char *services);
static void pbkdf2(work_t *w, const char *password, const uint8_t *salt, size_t salt_len,
        uint32_t iterations);
static bool service_allowed(const char *list, const char *service);
static bool equal(const uint8_t *a, const uint8_t *b, size_t len);
static bool valid_name(const char *str, size_t max, const char *extra);
static bool valid_field(const char *str, size_t max);
static size_t b64_encode(const uint8_t *src, size_t len, char *dst);
static int b64_decode(const char *src, uint8_t *dst, size_t max);
static bool service_listed(const char *list, const char *service);
static void services_remove(char *list, const char *remove);
static int append(char **buf, size_t *size, const char *line);
static int modify(const change_t *c);
static int write_file(const char *path, int fd, const char *content, size_t size, mode_t mode);

/*==============================================================================
  Local objects
==============================================================================*/
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Check password of user and service.
 */
//==============================================================================
int auth_check(const char *user, const char *password, const char *service, auth_account_t *account)
{
        if (!user || !password || !service) {
                errno = EINVAL;
                return -1;
        }

        if (!auth_is_configured()) {
                errno = ENOENT;
                return -1;
        }

        work_t *w = calloc(1, sizeof(*w));
        if (!w) {
                errno = ENOMEM;
                return -1;
        }

        bool found = (passwd_find(w, user, 0) == 0) && (shadow_find(w, user, service, NULL) == 0);

        if (!found) {
                /* the same work for unknown user: time does not reveal accounts */
                memset(&w->sh, 0, sizeof(w->sh));
                w->sh.iterations = AUTH_ITERATIONS_DEFAULT;
                w->sh.salt_len = SALT_LEN;
        }

        pbkdf2(w, password, w->sh.salt, w->sh.salt_len, w->sh.iterations);

        bool ok = equal(w->dk, w->sh.hash, HASH_LEN) && found;

        if (ok && account) {
                account->uid = w->pw.uid;
                account->gid = w->pw.gid;
        }

        auth_wipe(w, sizeof(*w));
        free(w);

        if (!ok) {
                errno = EACCES;
                return -1;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Check password of line of user with selected services.
 */
//==============================================================================
int auth_check_line(const char *user, const char *password, const char *services)
{
        if (!user || !password) {
                errno = EINVAL;
                return -1;
        }

        work_t *w = calloc(1, sizeof(*w));
        if (!w) {
                errno = ENOMEM;
                return -1;
        }

        int err = shadow_find(w, user, NULL, services ? services : "");
        if (!err) {
                pbkdf2(w, password, w->sh.salt, w->sh.salt_len, w->sh.iterations);
                err = equal(w->dk, w->sh.hash, HASH_LEN) ? 0 : EACCES;
        }

        auth_wipe(w, sizeof(*w));
        free(w);

        if (err) {
                errno = err;
                return -1;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Check if accounts are configured.
 */
//==============================================================================
bool auth_is_configured(void)
{
        struct stat st;
        return (stat(AUTH_PASSWD_RW_FILE, &st) == 0) || (stat(AUTH_PASSWD_FILE, &st) == 0);
}

//==============================================================================
/**
 * @brief  Get account of user.
 */
//==============================================================================
int auth_get_user(const char *name, uid_t uid, auth_user_t *user)
{
        if (!user) {
                errno = EINVAL;
                return -1;
        }

        work_t *w = calloc(1, sizeof(*w));
        if (!w) {
                errno = ENOMEM;
                return -1;
        }

        int err = passwd_find(w, name, uid);
        if (!err) {
                *user = w->pw;
        }

        free(w);

        if (err) {
                errno = ENOENT;
                return -1;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Set password of user.
 */
//==============================================================================
int auth_set_password(const char *user, const char *password, const char *services, uint32_t iterations)
{
        if (!password || (password[0] == '\0') || (strlen(password) > AUTH_PASSWORD_MAX) ||
                strpbrk(password, "\r\n") || (services && !valid_name(services, SERVICES_MAX, ",*_-")) ||
                (iterations > AUTH_ITERATIONS_MAX)) {
                errno = EINVAL;
                return -1;
        }

        change_t c = {
                .op = CHANGE_PASSWORD,
                .user = user,
                .password = password,
                .services = services,
                .iterations = iterations ? iterations : AUTH_ITERATIONS_DEFAULT,
                .uid = -1,
                .gid = -1,
        };

        return modify(&c);
}

//==============================================================================
/**
 * @brief  Set account of user.
 */
//==============================================================================
int auth_set_user(const char *user, int uid, int gid, const char *home, const char *shell)
{
        if ((uid > ID_MAX) || (gid > ID_MAX) || (home && !valid_field(home, AUTH_HOME_MAX)) ||
                (shell && !valid_field(shell, AUTH_SHELL_MAX))) {
                errno = EINVAL;
                return -1;
        }

        change_t c = {
                .op = CHANGE_ACCOUNT,
                .user = user,
                .uid = uid,
                .gid = gid,
                .home = home,
                .shell = shell,
        };

        return modify(&c);
}

//==============================================================================
/**
 * @brief  Remove user or services of user.
 */
//==============================================================================
int auth_remove_user(const char *user, const char *services)
{
        if (services && !valid_name(services, SERVICES_MAX, ",*_-")) {
                errno = EINVAL;
                return -1;
        }

        change_t c = {
                .op = CHANGE_REMOVE,
                .user = user,
                .services = services,
                .uid = -1,
                .gid = -1,
        };

        return modify(&c);
}

//==============================================================================
/**
 * @brief  Call function for each password line of each user.
 */
//==============================================================================
int auth_foreach_user(
        void (*fn)(const auth_user_t *user, const char *services, uint32_t iterations, void *arg), void *arg)
{
        if (!fn) {
                errno = EINVAL;
                return -1;
        }

        work_t *w = calloc(1, sizeof(*w));
        if (!w) {
                errno = ENOMEM;
                return -1;
        }

        FILE *pf = db_open(false);
        if (!pf) {
                free(w);
                errno = ENOENT;
                return -1;
        }

        while (fgets(w->line, sizeof(w->line), pf)) {
                if (!parse_passwd(w->line, &w->pw, NULL)) {
                        continue;
                }

                bool listed = false;

                FILE *sf = db_open(true);
                if (sf) {
                        while (fgets(w->copy, sizeof(w->copy), sf)) {
                                if (parse_shadow(w->copy, &w->sh) && (strcmp(w->sh.user, w->pw.name) == 0)) {
                                        fn(&w->pw, w->sh.services, w->sh.iterations, arg);
                                        listed = true;
                                }
                        }
                        fclose(sf);
                }

                if (!listed) {
                        fn(&w->pw, NULL, 0, arg);
                }
        }

        fclose(pf);
        auth_wipe(w, sizeof(*w));
        free(w);

        return 0;
}

//==============================================================================
/**
 * @brief  Check password with cache of successful logins.
 */
//==============================================================================
int auth_check_cached(auth_cache_t *cache, const char *user, const char *password, const char *service,
        auth_account_t *account)
{
        if (!cache || !user || !password || !service) {
                errno = EINVAL;
                return -1;
        }

        SHA256_CTX ctx;
        uint8_t digest[HASH_LEN];

        sha256_init(&ctx);
        sha256_update(&ctx, (const uint8_t *)user, strlen(user) + 1);
        sha256_update(&ctx, (const uint8_t *)password, strlen(password) + 1);
        sha256_update(&ctx, (const uint8_t *)service, strlen(service));
        sha256_final(&ctx, digest);
        auth_wipe(&ctx, sizeof(ctx));

        uint32_t now = (uint32_t)get_uptime_ms();
        int oldest = 0;

        for (int i = 0; i < AUTH_CACHE_ENTRIES; i++) {
                if (cache->entry[i].valid && ((now - cache->entry[i].time) >= AUTH_CACHE_TIMEOUT_MS)) {
                        cache->entry[i].valid = false;
                }

                if (cache->entry[i].valid && equal(cache->entry[i].digest, digest, HASH_LEN)) {
                        if (account) {
                                *account = cache->entry[i].account;
                        }
                        auth_wipe(digest, sizeof(digest));
                        return 0;
                }

                if (!cache->entry[i].valid ||
                        (cache->entry[oldest].valid && (cache->entry[i].time < cache->entry[oldest].time))) {
                        oldest = i;
                }
        }

        auth_account_t acc;
        int err = auth_check(user, password, service, &acc);
        if (err == 0) {
                memcpy(cache->entry[oldest].digest, digest, HASH_LEN);
                cache->entry[oldest].time = now;
                cache->entry[oldest].account = acc;
                cache->entry[oldest].valid = true;

                if (account) {
                        *account = acc;
                }
        }

        auth_wipe(digest, sizeof(digest));

        return err;
}

//==============================================================================
/**
 * @brief  Initialize limit of failed logins.
 */
//==============================================================================
int auth_ratelimit_init(auth_ratelimit_t *rl)
{
        memset(rl, 0, sizeof(*rl));
        rl->mtx = mutex_new(MUTEX_TYPE_NORMAL);
        return rl->mtx ? 0 : -1;
}

//==============================================================================
/**
 * @brief  Release limit of failed logins.
 */
//==============================================================================
void auth_ratelimit_deinit(auth_ratelimit_t *rl)
{
        if (rl->mtx) {
                mutex_delete(rl->mtx);
                rl->mtx = NULL;
        }
}

//==============================================================================
/**
 * @brief  Check if login from address is allowed.
 */
//==============================================================================
bool auth_ratelimit_allow(auth_ratelimit_t *rl, uint32_t addr)
{
        bool allow = true;

        if (mutex_lock(rl->mtx, MAX_DELAY_MS)) {
                uint32_t now = (uint32_t)get_uptime_ms();

                for (int i = 0; i < AUTH_RATELIMIT_ENTRIES; i++) {
                        if (rl->entry[i].fails && (rl->entry[i].addr == addr)) {
                                if (rl->entry[i].fails >= AUTH_RATELIMIT_FAILS) {
                                        if ((now - rl->entry[i].time) < AUTH_RATELIMIT_BLOCK_MS) {
                                                allow = false;
                                        } else {
                                                rl->entry[i].fails = 0;
                                        }
                                }
                                break;
                        }
                }

                mutex_unlock(rl->mtx);
        }

        return allow;
}

//==============================================================================
/**
 * @brief  Report result of login.
 */
//==============================================================================
void auth_ratelimit_report(auth_ratelimit_t *rl, uint32_t addr, bool success)
{
        if (mutex_lock(rl->mtx, MAX_DELAY_MS)) {
                uint32_t now = (uint32_t)get_uptime_ms();
                int slot = -1;

                for (int i = 0; i < AUTH_RATELIMIT_ENTRIES; i++) {
                        if (rl->entry[i].fails && (rl->entry[i].addr == addr)) {
                                slot = i;
                                break;
                        }
                }

                if (success) {
                        if (slot >= 0) {
                                rl->entry[slot].fails = 0;
                        }

                } else {
                        if (slot < 0) {
                                /* free entry or the oldest one */
                                slot = 0;
                                for (int i = 0; i < AUTH_RATELIMIT_ENTRIES; i++) {
                                        if (!rl->entry[i].fails) {
                                                slot = i;
                                                break;
                                        }

                                        if (rl->entry[i].time < rl->entry[slot].time) {
                                                slot = i;
                                        }
                                }

                                rl->entry[slot].addr = addr;
                                rl->entry[slot].fails = 0;
                        }

                        if (rl->entry[slot].fails < UINT8_MAX) {
                                rl->entry[slot].fails++;
                        }

                        rl->entry[slot].time = now;
                }

                mutex_unlock(rl->mtx);
        }
}

//==============================================================================
/**
 * @brief  Clear memory with secret data.
 */
//==============================================================================
void auth_wipe(void *mem, size_t len)
{
        volatile uint8_t *p = mem;

        while (len--) {
                *p++ = 0;
        }
}

//==============================================================================
/**
 * @brief  Check which files are used: writable files when writable passwd file
 *         exists, otherwise default files.
 *
 * @return true: writable files.
 */
//==============================================================================
static bool use_rw_files(void)
{
        struct stat st;
        return stat(AUTH_PASSWD_RW_FILE, &st) == 0;
}

//==============================================================================
/**
 * @brief  Open passwd or shadow file (writable or default files).
 *
 * @param  shadow       true: shadow file, false: passwd file
 *
 * @return File or NULL.
 */
//==============================================================================
static FILE *db_open(bool shadow)
{
        if (use_rw_files()) {
                return fopen(shadow ? AUTH_SHADOW_RW_FILE : AUTH_PASSWD_RW_FILE, "r");
        } else {
                return fopen(shadow ? AUTH_SHADOW_FILE : AUTH_PASSWD_FILE, "r");
        }
}

//==============================================================================
/**
 * @brief  Split line to fields separated by ':' (line is modified).
 *
 * @param  line         line
 * @param  field        fields
 * @param  count        number of fields
 *
 * @return true if line has exactly count fields.
 */
//==============================================================================
static bool split(char *line, char **field, int count)
{
        line[strcspn(line, "\r\n")] = '\0';

        if ((line[0] == '#') || (line[0] == '\0')) {
                return false;
        }

        field[0] = line;
        for (int i = 1; i < count; i++) {
                field[i] = strchr(field[i - 1], ':');
                if (!field[i]) {
                        return false;
                }
                *field[i]++ = '\0';
        }

        return strchr(field[count - 1], ':') == NULL;
}

//==============================================================================
/**
 * @brief  Parse ID field.
 *
 * @param  str          string
 * @param  id           ID
 *
 * @return true if ID is valid.
 */
//==============================================================================
static bool parse_id(const char *str, u16_t *id)
{
        char *end = NULL;
        unsigned long n = strtoul(str, &end, 10);
        if ((end == str) || (*end != '\0') || (n > ID_MAX)) {
                return false;
        }

        *id = n;
        return true;
}

//==============================================================================
/**
 * @brief  Parse line of passwd file (line is modified).
 *         Format: name:x:uid:gid:description:home:shell
 *
 * @param  line         line
 * @param  u            account
 * @param  gecos        description (buffer of GECOS_MAX + 1, can be NULL)
 *
 * @return true if line contains valid account.
 */
//==============================================================================
static bool parse_passwd(char *line, auth_user_t *u, char *gecos)
{
        char *field[7];

        if (!split(line, field, 7) || !valid_name(field[0], AUTH_USER_MAX, "._-") ||
                !parse_id(field[2], &u->uid) || !parse_id(field[3], &u->gid) ||
                (strlen(field[4]) > GECOS_MAX) || (strlen(field[5]) > AUTH_HOME_MAX) ||
                (strlen(field[6]) > AUTH_SHELL_MAX)) {
                return false;
        }

        strcpy(u->name, field[0]);
        strcpy(u->home, field[5]);
        strcpy(u->shell, field[6]);

        if (gecos) {
                strcpy(gecos, field[4]);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Parse line of shadow file (line is modified).
 *         Format: name:hash:services
 *
 * @param  line         line
 * @param  e            entry
 *
 * @return true if line contains valid entry.
 */
//==============================================================================
static bool parse_shadow(char *line, shadow_t *e)
{
        char *field[3];

        if (!split(line, field, 3)) {
                return false;
        }

        char *hash = field[1];

        if (!valid_name(field[0], AUTH_USER_MAX, "._-") || !valid_name(field[2], SERVICES_MAX, ",*_-") ||
                (strncmp(hash, SCHEME, strlen(SCHEME)) != 0) || (strlen(hash) >= HASH_TEXT)) {
                return false;
        }

        strcpy(e->hash_text, hash);

        char *iter = hash + strlen(SCHEME);
        char *salt = strchr(iter, '$');
        if (!salt) {
                return false;
        }
        *salt++ = '\0';

        char *dk = strchr(salt, '$');
        if (!dk) {
                return false;
        }
        *dk++ = '\0';

        char *end = NULL;
        unsigned long n = strtoul(iter, &end, 10);
        if ((end == iter) || (*end != '\0') || (n < 1) || (n > AUTH_ITERATIONS_MAX)) {
                return false;
        }

        int salt_len = b64_decode(salt, e->salt, sizeof(e->salt));
        if ((salt_len < 1) || (b64_decode(dk, e->hash, sizeof(e->hash)) != HASH_LEN)) {
                return false;
        }

        strcpy(e->user, field[0]);
        strcpy(e->services, field[2]);
        e->iterations = n;
        e->salt_len = salt_len;

        return true;
}

//==============================================================================
/**
 * @brief  Find account in passwd file (result in w->pw).
 *
 * @param  w            work memory
 * @param  name         user name (NULL: find by uid)
 * @param  uid          user ID (name NULL)
 *
 * @return 0: found, ENOENT: not found or no file.
 */
//==============================================================================
static int passwd_find(work_t *w, const char *name, uid_t uid)
{
        FILE *f = db_open(false);
        if (!f) {
                return ENOENT;
        }

        int err = ENOENT;

        while (err && fgets(w->line, sizeof(w->line), f)) {
                if (parse_passwd(w->line, &w->pw_copy, NULL) &&
                        (name ? (strcmp(w->pw_copy.name, name) == 0) : (w->pw_copy.uid == uid))) {
                        w->pw = w->pw_copy;
                        err = 0;
                }
        }

        fclose(f);

        return err;
}

//==============================================================================
/**
 * @brief  Find password line of user (result in w->sh): line that names the
 *         service, otherwise line with "*"; or line with exactly given
 *         services (empty string: the only line of user).
 *
 * @param  w            work memory
 * @param  user         user name
 * @param  service      service (NULL: search by services)
 * @param  services     services of line (service NULL)
 *
 * @return 0: found, EACCES: not found, EINVAL: several lines (services "").
 */
//==============================================================================
static int shadow_find(work_t *w, const char *user, const char *service, const char *services)
{
        FILE *f = db_open(true);
        if (!f) {
                return EACCES;
        }

        bool found = false;
        bool candidate = false;
        int lines = 0;

        while (!found && fgets(w->line, sizeof(w->line), f)) {
                shadow_t *e = &w->sh_copy;

                if (!parse_shadow(w->line, e) || (strcmp(e->user, user) != 0)) {
                        continue;
                }

                lines++;

                if (service) {
                        if (service_listed(e->services, service)) {
                                w->sh = *e;
                                found = true;

                        } else if (!candidate && service_allowed(e->services, service)) {
                                w->sh = *e; // line with "*"
                                candidate = true;
                        }

                } else if ((services[0] == '\0') || (strcmp(e->services, services) == 0)) {
                        w->sh = *e;
                        candidate = true;
                        found = (services[0] != '\0');
                }
        }

        fclose(f);

        if (!service && (services[0] == '\0') && (lines > 1)) {
                return EINVAL;
        }

        return (found || candidate) ? 0 : EACCES;
}

//==============================================================================
/**
 * @brief  PBKDF2-HMAC-SHA256 with one output block (32 bytes in w->dk).
 *
 * @param  w            work memory
 * @param  password     password
 * @param  salt         salt
 * @param  salt_len     salt length
 * @param  iterations   number of iterations
 */
//==============================================================================
static void pbkdf2(work_t *w, const char *password, const uint8_t *salt, size_t salt_len, uint32_t iterations)
{
        static const uint8_t BLOCK_INDEX[4] = {0, 0, 0, 1};

        /* HMAC key: password or its hash (longer than block) */
        uint8_t *key = w->pad;
        size_t len = strlen(password);
        memset(key, 0, 64);

        if (len > 64) {
                sha256_init(&w->ctx);
                sha256_update(&w->ctx, (const uint8_t *)password, len);
                sha256_final(&w->ctx, key);
        } else {
                memcpy(key, password, len);
        }

        for (int i = 0; i < 64; i++) {
                key[i] ^= 0x36;
        }
        sha256_init(&w->ictx);
        sha256_update(&w->ictx, key, 64);

        for (int i = 0; i < 64; i++) {
                key[i] ^= 0x36 ^ 0x5C;
        }
        sha256_init(&w->octx);
        sha256_update(&w->octx, key, 64);

        /* U1 = HMAC(salt || INT(1)) */
        w->ctx = w->ictx;
        sha256_update(&w->ctx, salt, salt_len);
        sha256_update(&w->ctx, BLOCK_INDEX, sizeof(BLOCK_INDEX));
        sha256_final(&w->ctx, w->u);
        w->ctx = w->octx;
        sha256_update(&w->ctx, w->u, HASH_LEN);
        sha256_final(&w->ctx, w->u);
        memcpy(w->dk, w->u, HASH_LEN);

        /* Un = HMAC(Un-1), DK = U1 ^ U2 ^ ... */
        for (uint32_t n = 1; n < iterations; n++) {
                w->ctx = w->ictx;
                sha256_update(&w->ctx, w->u, HASH_LEN);
                sha256_final(&w->ctx, w->u);
                w->ctx = w->octx;
                sha256_update(&w->ctx, w->u, HASH_LEN);
                sha256_final(&w->ctx, w->u);

                for (int i = 0; i < HASH_LEN; i++) {
                        w->dk[i] ^= w->u[i];
                }
        }
}

//==============================================================================
/**
 * @brief  Compare buffers in constant time.
 *
 * @param  a            buffer a
 * @param  b            buffer b
 * @param  len          length
 *
 * @return true if buffers are equal.
 */
//==============================================================================
static bool equal(const uint8_t *a, const uint8_t *b, size_t len)
{
        uint8_t diff = 0;

        for (size_t i = 0; i < len; i++) {
                diff |= a[i] ^ b[i];
        }

        return diff == 0;
}

//==============================================================================
/**
 * @brief  Check if service is in list of services of account.
 *
 * @param  list         services separated by commas ("*": all)
 * @param  service      service
 *
 * @return true if service is allowed.
 */
//==============================================================================
static bool service_allowed(const char *list, const char *service)
{
        size_t slen = strlen(service);

        while (*list) {
                size_t len = strcspn(list, ",");

                if (((len == 1) && (list[0] == '*')) ||
                        ((len == slen) && (strncmp(list, service, len) == 0))) {
                        return true;
                }

                list += len;
                if (*list == ',') {
                        list++;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief  Check name (letters, digits and extra characters).
 *
 * @param  str          string
 * @param  max          maximal length
 * @param  extra        allowed extra characters
 *
 * @return true if name is valid.
 */
//==============================================================================
static bool valid_name(const char *str, size_t max, const char *extra)
{
        size_t len = 0;

        for (; str[len]; len++) {
                char c = str[len];
                bool alnum = ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
                        ((c >= '0') && (c <= '9'));

                if (!alnum && !strchr(extra, c)) {
                        return false;
                }
        }

        return (len > 0) && (len <= max);
}

//==============================================================================
/**
 * @brief  Base64 encoding without padding.
 *
 * @param  src          data
 * @param  len          data length
 * @param  dst          destination (min. 4 * len / 3 + 2 bytes)
 *
 * @return Length of string.
 */
//==============================================================================
static size_t b64_encode(const uint8_t *src, size_t len, char *dst)
{
        size_t n = 0;
        uint32_t acc = 0;
        int bits = 0;

        for (size_t i = 0; i < len; i++) {
                acc = (acc << 8) | src[i];
                bits += 8;

                while (bits >= 6) {
                        bits -= 6;
                        dst[n++] = B64[(acc >> bits) & 0x3F];
                }
        }

        if (bits > 0) {
                dst[n++] = B64[(acc << (6 - bits)) & 0x3F];
        }

        dst[n] = '\0';

        return n;
}

//==============================================================================
/**
 * @brief  Base64 decoding (padding is optional).
 *
 * @param  src          string
 * @param  dst          destination
 * @param  max          size of destination
 *
 * @return Number of decoded bytes, -1 on error.
 */
//==============================================================================
static int b64_decode(const char *src, uint8_t *dst, size_t max)
{
        size_t n = 0;
        uint32_t acc = 0;
        int bits = 0;

        for (; *src && (*src != '='); src++) {
                const char *p = strchr(B64, *src);
                if (!p) {
                        return -1;
                }

                acc = (acc << 6) | (uint32_t)(p - B64);
                bits += 6;

                if (bits >= 8) {
                        bits -= 8;
                        if (n >= max) {
                                return -1;
                        }
                        dst[n++] = (uint8_t)(acc >> bits);
                }
        }

        return (int)n;
}

//==============================================================================
/**
 * @brief  Check if service is named in list of services ("*" is not checked).
 *
 * @param  list         services separated by commas
 * @param  service      service
 *
 * @return true if service is in the list.
 */
//==============================================================================
static bool service_listed(const char *list, const char *service)
{
        size_t slen = strlen(service);

        while (*list) {
                size_t len = strcspn(list, ",");

                if ((len == slen) && (strncmp(list, service, len) == 0)) {
                        return true;
                }

                list += len;
                if (*list == ',') {
                        list++;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief  Remove services from list ("*" stays in the list).
 *
 * @param  list         services separated by commas (modified)
 * @param  remove       services to remove
 */
//==============================================================================
static void services_remove(char *list, const char *remove)
{
        char *dst = list;
        const char *src = list;

        while (*src) {
                size_t len = strcspn(src, ",");
                char name[SERVICES_MAX + 1];
                memcpy(name, src, len);
                name[len] = '\0';

                if (strcmp(name, "*") == 0 || !service_listed(remove, name)) {
                        if (dst != list) {
                                *dst++ = ',';
                        }
                        memmove(dst, name, len);
                        dst += len;
                }

                src += len;
                if (*src == ',') {
                        src++;
                }
        }

        *dst = '\0';
}

//==============================================================================
/**
 * @brief  Append line to buffer.
 *
 * @param  buf          buffer (reallocated)
 * @param  size         size of data in buffer
 * @param  line         line
 *
 * @return One of errno value.
 */
//==============================================================================
static int append(char **buf, size_t *size, const char *line)
{
        size_t len = strlen(line);
        char *p = realloc(*buf, *size + len + 2);
        if (!p) {
                return ENOMEM;
        }

        *buf = p;
        memcpy(p + *size, line, len);
        *size += len;

        if ((len == 0) || (p[*size - 1] != '\n')) {
                p[(*size)++] = '\n';
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Check field of passwd file (no ':' and new line).
 *
 * @param  str          string
 * @param  max          maximal length
 *
 * @return true if field is valid.
 */
//==============================================================================
static bool valid_field(const char *str, size_t max)
{
        return (strlen(str) <= max) && !strpbrk(str, ":\r\n");
}

//==============================================================================
/**
 * @brief  Write content to file by temporary file and rename.
 *
 * @param  path         file path
 * @param  fd           opened temporary file (path + TMP_SUFFIX, closed by function)
 * @param  content      content
 * @param  size         size of content
 * @param  mode         file mode
 *
 * @return One of errno value.
 */
//==============================================================================
static int write_file(const char *path, int fd, const char *content, size_t size, mode_t mode)
{
        int err = 0;
        char *tmp = malloc(strlen(path) + sizeof(TMP_SUFFIX));

        if (!tmp) {
                err = ENOMEM;
        } else {
                strcpy(tmp, path);
                strcat(tmp, TMP_SUFFIX);
        }

        if (!err && size && (write(fd, content, size) != (ssize_t)size)) {
                err = errno ? errno : EIO;
        }

        if ((close(fd) != 0) && !err) {
                err = errno ? errno : EIO;
        }

        if (!err) {
                chmod(tmp, mode); // creation mode is limited by umask
        }

        if (!err && (rename(tmp, path) != 0)) {
                /* file system that does not replace existing file by rename() */
                if ((errno != EEXIST) || (remove(path) != 0) || (rename(tmp, path) != 0)) {
                        err = errno ? errno : EIO;
                }
        }

        if (err && tmp) {
                remove(tmp);
        }

        free(tmp);

        return err;
}

//==============================================================================
/**
 * @brief  Change passwd and shadow files. Files are written by temporary files
 *         (temporary shadow file created exclusively is lock of changes), the
 *         writable files are created from default files at the first change.
 *
 * @param  c            change
 *
 * @return 0 on success, otherwise -1 and errno is set.
 */
//==============================================================================
static int modify(const change_t *c)
{
        if (!c->user || !valid_name(c->user, AUTH_USER_MAX, "._-")) {
                errno = EINVAL;
                return -1;
        }

        work_t *w = calloc(1, sizeof(*w));
        char *path = malloc(sizeof(AUTH_SHADOW_RW_FILE) + sizeof(TMP_SUFFIX) + sizeof(AUTH_PASSWD_RW_FILE));
        if (!w || !path) {
                free(w);
                free(path);
                errno = ENOMEM;
                return -1;
        }

        int err = 0;

        /* directories of files */
        const char *const dirs[] = {AUTH_PASSWD_RW_FILE, AUTH_SHADOW_RW_FILE};
        for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
                strcpy(path, dirs[i]);
                char *slash = strrchr(path, '/');
                if (slash && (slash != path)) {
                        *slash = '\0';
                        mkdir(path, 0755);
                }
        }

        /* lock: temporary shadow file created exclusively */
        int sfd = -1;
        strcpy(path, AUTH_SHADOW_RW_FILE TMP_SUFFIX);
        for (int i = 0; (sfd < 0) && (i < LOCK_TRIES); i++) {
                sfd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
                if ((sfd < 0) && (errno == EEXIST)) {
                        msleep(100);
                } else if (sfd < 0) {
                        break;
                }
        }

        if (sfd < 0) {
                err = (errno == EEXIST) ? EBUSY : (errno ? errno : EIO);
                free(w);
                free(path);
                errno = err;
                return -1;
        }

        char *passwd = NULL, *shadow = NULL, *hash_text = NULL;
        size_t passwd_size = 0, shadow_size = 0;
        bool changed = false;

        /* passwd file: account of user, the first free user ID */
        int free_uid = FIRST_UID;
        bool exists = false;

        FILE *f = db_open(false);
        if (f) {
                while (fgets(w->line, sizeof(w->line), f)) {
                        strcpy(w->copy, w->line);
                        if (parse_passwd(w->copy, &w->pw_copy, NULL)) {
                                if (strcmp(w->pw_copy.name, c->user) == 0) {
                                        exists = true;
                                } else if ((int)w->pw_copy.uid >= free_uid) {
                                        free_uid = w->pw_copy.uid + 1;
                                }
                        }
                }
                fclose(f);
        }

        if (!exists && (c->op == CHANGE_REMOVE)) {
                err = ENOENT;
        }

        f = err ? NULL : db_open(false);
        if (f) {
                while (!err && fgets(w->line, sizeof(w->line), f)) {
                        strcpy(w->copy, w->line);

                        if (!parse_passwd(w->copy, &w->pw, w->gecos) || (strcmp(w->pw.name, c->user) != 0)) {
                                err = append(&passwd, &passwd_size, w->line);
                                continue;
                        }

                        if ((c->op == CHANGE_REMOVE) && !c->services) {
                                changed = true; // user removed
                                continue;
                        }

                        if (c->op == CHANGE_ACCOUNT) {
                                w->pw.uid = (c->uid >= 0) ? c->uid : w->pw.uid;
                                w->pw.gid = (c->gid >= 0) ? c->gid : w->pw.gid;
                                strcpy(w->pw.home, c->home ? c->home : w->pw.home);
                                strcpy(w->pw.shell, c->shell ? c->shell : w->pw.shell);
                                changed = true;
                        }

                        snprintf(w->line, sizeof(w->line), "%s:x:%u:%u:%s:%s:%s\n", c->user, (uint)w->pw.uid,
                                (uint)w->pw.gid, w->gecos, w->pw.home, w->pw.shell);
                        err = append(&passwd, &passwd_size, w->line);
                }
                fclose(f);
        }

        if (!err && !exists && (c->op != CHANGE_REMOVE)) {
                int uid = (c->uid >= 0) ? c->uid : (strcmp(c->user, "root") == 0) ? 0 : free_uid;
                int gid = (c->gid >= 0) ? c->gid : uid;

                if (uid > ID_MAX) {
                        err = ENOSPC;
                } else {
                        snprintf(w->line, sizeof(w->line), "%s:x:%d:%d::%s:%s\n", c->user, uid, gid,
                                c->home ? c->home : "", c->shell ? c->shell : "");
                        err = append(&passwd, &passwd_size, w->line);
                        changed = true;
                }
        }

        /* shadow file: the first pass counts lines of user */
        int lines = 0;
        f = err ? NULL : db_open(true);
        if (f) {
                while (fgets(w->line, sizeof(w->line), f)) {
                        if (parse_shadow(w->line, &w->sh) && (strcmp(w->sh.user, c->user) == 0)) {
                                lines++;
                        }
                }
                fclose(f);
        }

        const char *services = c->services;

        if (!err && (c->op == CHANGE_PASSWORD) && !services) {
                if (lines > 1) {
                        err = EINVAL; // user has passwords of several services
                } else if (lines == 0) {
                        services = "*";
                }
        }

        /* new hash */
        if (!err && (c->op == CHANGE_PASSWORD)) {
                uint8_t salt[SALT_LEN];
                char salt_b64[(SALT_LEN * 4) / 3 + 2];
                char hash_b64[(HASH_LEN * 4) / 3 + 2];

                hash_text = malloc(HASH_TEXT);
                if (!hash_text) {
                        err = ENOMEM;
                } else if (getentropy(salt, sizeof(salt)) != 0) {
                        err = errno ? errno : EIO;
                } else {
                        pbkdf2(w, c->password, salt, sizeof(salt), c->iterations);
                        b64_encode(salt, sizeof(salt), salt_b64);
                        b64_encode(w->dk, HASH_LEN, hash_b64);
                        snprintf(hash_text, HASH_TEXT, SCHEME "%u$%s$%s", (unsigned)c->iterations, salt_b64,
                                hash_b64);
                }
        }

        /* shadow file: the second pass creates new content */
        bool done = false;

        f = err ? NULL : db_open(true);
        if (f) {
                while (!err && fgets(w->line, sizeof(w->line), f)) {
                        strcpy(w->copy, w->line);

                        if (!parse_shadow(w->copy, &w->sh) || (strcmp(w->sh.user, c->user) != 0) ||
                                (c->op == CHANGE_ACCOUNT)) {
                                err = append(&shadow, &shadow_size, w->line);
                                continue;
                        }

                        shadow_t *e = &w->sh;
                        const char *hash = e->hash_text;

                        if ((c->op == CHANGE_PASSWORD) &&
                                (!services || (strcmp(e->services, services) == 0))) {
                                hash = hash_text; // line of the same services
                                done = true;

                        } else if (services) {
                                services_remove(e->services, services);
                        } else {
                                e->services[0] = '\0'; // remove user
                        }

                        changed = true;

                        if (e->services[0] != '\0') {
                                snprintf(w->line, sizeof(w->line), "%s:%s:%s\n", c->user, hash, e->services);
                                err = append(&shadow, &shadow_size, w->line);
                        }
                }

                fclose(f);
        }

        if (!err && (c->op == CHANGE_PASSWORD) && !done) {
                snprintf(w->line, sizeof(w->line), "%s:%s:%s\n", c->user, hash_text, services);
                err = append(&shadow, &shadow_size, w->line);
                changed = true;
        }

        if (!err && !changed) {
                err = ENOENT;
        }

        /* shadow file first: writable passwd file selects writable files */
        if (!err) {
                err = write_file(AUTH_SHADOW_RW_FILE, sfd, shadow, shadow_size, 0600);
                sfd = -1;
        }

        if (!err) {
                strcpy(path, AUTH_PASSWD_RW_FILE TMP_SUFFIX);
                int pfd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                err = (pfd < 0) ? (errno ? errno : EIO)
                                : write_file(AUTH_PASSWD_RW_FILE, pfd, passwd, passwd_size, 0644);
        }

        if (sfd >= 0) {
                close(sfd);
                strcpy(path, AUTH_SHADOW_RW_FILE TMP_SUFFIX);
                remove(path);
        }

        if (shadow) {
                auth_wipe(shadow, shadow_size);
                free(shadow);
        }

        free(passwd);

        if (hash_text) {
                auth_wipe(hash_text, HASH_TEXT);
                free(hash_text);
        }

        auth_wipe(w, sizeof(*w));
        free(w);
        free(path);

        if (err) {
                errno = err;
                return -1;
        }

        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
