/*==============================================================================
File    auth.h

Author  Daniel Zorychta

Brief   User authentication: password file, PBKDF2-SHA256 hashes, limit of failed logins.

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

/**
@defgroup AUTH_H_ AUTH_H_

User authentication library used by network services (telnetd, ftpd,
mqtt_broker), login and passwd programs.

Accounts are kept in two files like in Linux ('#' starts a comment):

- passwd (public, mode 0644), one line per user:
@code
name:x:<uid>:<gid>:<description>:<home directory>:<shell>
root:x:0:0:Administrator:/:
sensor:x:1001:1001::/home/sensor:dsh
@endcode
  User ID and group ID (0..65535, 0: root) are credentials of programs started
  for the user (shell of login and telnetd, session of ftpd). Home directory
  is the working directory of the shell (empty: "/"), shell is the program
  started by login, telnetd and su (empty: program of the service, e.g. dsh).

- shadow (secret, mode 0600), one line per password:
@code
name:$pbkdf2-sha256$<iterations>$<salt>$<hash>:<services>
root:$pbkdf2-sha256$10000$Gq3kQ1m1rZK8wE0QXfQh6A$x9Lm...:*
root:$pbkdf2-sha256$1$ZRT39TIXWogL5VrXFNIyDw$KpFF...:mqtt
@endcode
  Salt (16 bytes) and hash (32 bytes) are base64 encoded without padding, hash
  is PBKDF2-HMAC-SHA256 of password, so lines can be created on PC by
  tools/mkpasswd.py. Services are names of services of the line separated by
  commas ("*": all services): "shell" (login on console, su), "telnet"
  (telnetd), "ftp" (ftpd), "mqtt" (mqtt_broker), "sudo" (sudo, only given
  explicitly, "*" does not allow sudo). Account with "shell" only (e.g. root)
  logs in only on console. A user can have several lines with different services and passwords
  (separate password of service), the line that names the service has
  priority over the line with "*".

Accounts are read from @ref AUTH_PASSWD_RW_FILE and @ref AUTH_SHADOW_RW_FILE
(writable files changed by passwd program) or, when the writable passwd file
does not exist, from @ref AUTH_PASSWD_FILE and @ref AUTH_SHADOW_FILE (default
accounts, e.g. files of ROMFS image). Paths are set by libc flags
__LIBC_PASSWD_FILE__, __LIBC_SHADOW_FILE__, __LIBC_PASSWD_RW_FILE__ and
__LIBC_SHADOW_RW_FILE__ (configuration of the appset). Keep the writable files
on a file system with file permissions (FAT does not store modes of files).
*/
/**@{*/

#pragma once

/*==============================================================================
  Include files
==============================================================================*/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dnx/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  Exported macros
==============================================================================*/
/** Public account file with default accounts (e.g. ROMFS image). */
#ifdef __LIBC_PASSWD_FILE__
#define AUTH_PASSWD_FILE __LIBC_PASSWD_FILE__
#else
#define AUTH_PASSWD_FILE "/etc/passwd"
#endif

/** Password file with default passwords (e.g. ROMFS image). */
#ifdef __LIBC_SHADOW_FILE__
#define AUTH_SHADOW_FILE __LIBC_SHADOW_FILE__
#else
#define AUTH_SHADOW_FILE "/etc/shadow"
#endif

/** Writable public account file (mode 0644). */
#ifdef __LIBC_PASSWD_RW_FILE__
#define AUTH_PASSWD_RW_FILE __LIBC_PASSWD_RW_FILE__
#else
#define AUTH_PASSWD_RW_FILE "/mnt/etc/passwd"
#endif

/** Writable password file (mode 0600). */
#ifdef __LIBC_SHADOW_RW_FILE__
#define AUTH_SHADOW_RW_FILE __LIBC_SHADOW_RW_FILE__
#else
#define AUTH_SHADOW_RW_FILE "/mnt/etc/shadow"
#endif

/** Default number of PBKDF2 iterations. */
#define AUTH_ITERATIONS_DEFAULT 10000

/** Maximal number of PBKDF2 iterations accepted in password file. */
#define AUTH_ITERATIONS_MAX 1000000

/** Maximal length of user name. */
#define AUTH_USER_MAX 32

/** Maximal length of home directory path. */
#define AUTH_HOME_MAX 63

/** Maximal length of shell command. */
#define AUTH_SHELL_MAX 63

/** Maximal length of password. */
#define AUTH_PASSWORD_MAX 128

/** Delay of answer after failed login [ms] (used by services). */
#define AUTH_FAIL_DELAY_MS 1000

/** Number of addresses remembered by limit of failed logins. */
#define AUTH_RATELIMIT_ENTRIES 8

/** Number of failed logins that blocks address. */
#define AUTH_RATELIMIT_FAILS 5

/** Time of address block [ms]. */
#define AUTH_RATELIMIT_BLOCK_MS 60000

/** Number of entries of cache of successful logins. */
#define AUTH_CACHE_ENTRIES 4

/** Validity of cache entry [ms]. */
#define AUTH_CACHE_TIMEOUT_MS 600000

/*==============================================================================
  Exported object types
==============================================================================*/
/**
 * Credentials of account.
 */
typedef struct {
        uid_t uid; /*!< user ID */
        gid_t gid; /*!< group ID */
} auth_account_t;

/**
 * Account (line of passwd file).
 */
typedef struct {
        char name[AUTH_USER_MAX + 1];   /*!< user name */
        uid_t uid;                      /*!< user ID */
        gid_t gid;                      /*!< group ID */
        char home[AUTH_HOME_MAX + 1];   /*!< home directory (empty: "/") */
        char shell[AUTH_SHELL_MAX + 1]; /*!< shell command (empty: default of service) */
} auth_user_t;

/**
 * Limit of failed logins (address is blocked after @ref AUTH_RATELIMIT_FAILS
 * failures for @ref AUTH_RATELIMIT_BLOCK_MS). Object is thread safe.
 */
typedef struct {
        mutex_t *mtx;
        struct {
                uint32_t addr;
                uint32_t time;
                uint8_t fails;
        } entry[AUTH_RATELIMIT_ENTRIES];
} auth_ratelimit_t;

/**
 * Cache of successful logins (fast check of repeated logins, e.g. MQTT
 * devices). Object is not thread safe.
 */
typedef struct {
        struct {
                uint8_t digest[32];
                uint32_t time;
                auth_account_t account;
                bool valid;
        } entry[AUTH_CACHE_ENTRIES];
} auth_cache_t;

/*==============================================================================
  Exported functions
==============================================================================*/
/**
 * @brief  Check password of user and service. Time of check does not depend on
 *         existence of user.
 * @param  user         user name
 * @param  password     password
 * @param  service      service name (e.g. "ftp")
 * @param  account      credentials of account (can be NULL)
 * @return 0 on success, otherwise -1 and errno is set: EACCES (wrong user,
 *         password or service), ENOENT (no password file), ENOMEM.
 */
extern int auth_check(const char *user, const char *password, const char *service, auth_account_t *account);

/**
 * @brief  Check password with cache of successful logins.
 * @param  cache        cache (zero initialized)
 * @param  user         user name
 * @param  password     password
 * @param  service      service name
 * @param  account      credentials of account (can be NULL)
 * @return The same as auth_check().
 */
extern int auth_check_cached(auth_cache_t *cache, const char *user, const char *password, const char *service,
        auth_account_t *account);

/**
 * @brief  Check password of line of user with selected services (e.g. old
 *         password before change by user).
 * @param  user         user name
 * @param  password     password
 * @param  services     services of line (NULL: the only line of user)
 * @return 0 on success, otherwise -1 and errno is set: EACCES (wrong password
 *         or no line), EINVAL (user has several lines and services is NULL).
 */
extern int auth_check_line(const char *user, const char *password, const char *services);

/**
 * @brief  Check if accounts are configured (passwd file exists).
 * @return true if accounts are configured.
 */
extern bool auth_is_configured(void);

/**
 * @brief  Get account of user (passwd file, readable by all users).
 * @param  name         user name (NULL: find by uid)
 * @param  uid          user ID (name == NULL)
 * @param  user         account
 * @return 0 on success, otherwise -1 and errno is set (ENOENT: no user).
 */
extern int auth_get_user(const char *name, uid_t uid, auth_user_t *user);

/**
 * @brief  Set password of user (account is created when does not exist: user
 *         ID 0 for "root", otherwise the first free ID from 1000, group ID the
 *         same as user ID). Changes are written to the writable files.
 *
 *         Without services the only password line of user is changed (new
 *         line: "*", error EINVAL when user has passwords of several
 *         services). With services the line with the same services is changed
 *         or created and these services are removed from other lines of user
 *         (separate password of service).
 * @param  user         user name (letters, digits, '.', '_', '-')
 * @param  password     password
 * @param  services     services of line (NULL: see above)
 * @param  iterations   PBKDF2 iterations (0: @ref AUTH_ITERATIONS_DEFAULT)
 * @return 0 on success, otherwise -1 and errno is set (EBUSY: files are
 *         changed by other program).
 */
extern int auth_set_password(const char *user, const char *password, const char *services,
        uint32_t iterations);

/**
 * @brief  Set account of user (created without password when does not exist).
 * @param  user         user name
 * @param  uid          user ID (-1: not changed, new user: see auth_set_password())
 * @param  gid          group ID (-1: not changed, new user: user ID)
 * @param  home         home directory (NULL: not changed)
 * @param  shell        shell command (NULL: not changed)
 * @return 0 on success, otherwise -1 and errno is set.
 */
extern int auth_set_user(const char *user, int uid, int gid, const char *home, const char *shell);

/**
 * @brief  Remove user or services of user.
 * @param  user         user name
 * @param  services     services to remove (NULL: account and all passwords)
 * @return 0 on success, otherwise -1 and errno is set (ENOENT: nothing to
 *         remove, EBUSY: files are changed by other program).
 */
extern int auth_remove_user(const char *user, const char *services);

/**
 * @brief  Call function for each password line of each user (user without
 *         password or without access to shadow file: one call with services
 *         NULL).
 * @param  fn           function (user, services, iterations, arg)
 * @param  arg          argument of function
 * @return 0 on success, otherwise -1 and errno is set (ENOENT: no passwd file).
 */
extern int auth_foreach_user(
        void (*fn)(const auth_user_t *user, const char *services, uint32_t iterations, void *arg), void *arg);

/**
 * @brief  Initialize limit of failed logins.
 * @param  rl           object
 * @return 0 on success, otherwise -1.
 */
extern int auth_ratelimit_init(auth_ratelimit_t *rl);

/**
 * @brief  Release limit of failed logins.
 * @param  rl           object
 */
extern void auth_ratelimit_deinit(auth_ratelimit_t *rl);

/**
 * @brief  Check if login from address is allowed.
 * @param  rl           object
 * @param  addr         address of client
 * @return false when address is blocked.
 */
extern bool auth_ratelimit_allow(auth_ratelimit_t *rl, uint32_t addr);

/**
 * @brief  Report result of login.
 * @param  rl           object
 * @param  addr         address of client
 * @param  success      true: successful login (counter of address is cleared)
 */
extern void auth_ratelimit_report(auth_ratelimit_t *rl, uint32_t addr, bool success);

/**
 * @brief  Clear memory with secret data (not removed by compiler).
 * @param  mem          memory
 * @param  len          length
 */
extern void auth_wipe(void *mem, size_t len);

#ifdef __cplusplus
}
#endif

/**@}*/
/*==============================================================================
  End of file
==============================================================================*/
