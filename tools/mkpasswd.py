#!/usr/bin/env python3
"""
Create account of dnx RTOS account files (common_libs/auth): line of passwd file
(public account) and line of shadow file (PBKDF2-HMAC-SHA256 hash of password with
random salt). Lines can be added to the default files (e.g. /etc/passwd and
/etc/shadow of ROMFS image). Do not keep shadow files in public repositories: hash
of a weak password can be cracked offline.

passwd line: name:x:uid:gid:description:home:shell
shadow line: name:hash:services

Usage: tools/mkpasswd.py [options] <user> [services]
  user          user name (letters, digits, '.', '_', '-')
  services      services of password separated by commas: shell (login on
                console, su), telnet, ftp, mqtt, sudo
                ('*': all services except sudo, default); several shadow lines of
                one user with different services are separate passwords of services
  -i <n>        PBKDF2 iterations (default 10000, 1..1000000)
  -g            generate random password (printed to stderr), 1 iteration is
                enough for random password (e.g. accounts of MQTT devices)
  -u <uid>      user ID (default: 0 for root, otherwise 1000)
  -G <gid>      group ID (default: user ID)
  -c <text>     description of account
  -H <dir>      home directory (default: empty, "/")
  -S <shell>    shell command (default: empty, program of service)
  -o <dir>      append lines to <dir>/passwd (only new user) and <dir>/shadow
                instead of printing them

Example: tools/mkpasswd.py -o app-romfs root
"""

import argparse
import base64
import getpass
import hashlib
import os
import re
import secrets
import sys

ITERATIONS_DEFAULT = 10000
ITERATIONS_MAX = 1000000


def b64(data):
    return base64.b64encode(data).decode().rstrip("=")


def field(value, name, max_len):
    if len(value) > max_len or re.search(r"[:\r\n]", value):
        sys.exit("invalid " + name)
    return value


def user_exists(path, user):
    if not os.path.exists(path):
        return False
    with open(path) as f:
        return any(line.split(":", 1)[0] == user for line in f if not line.startswith("#"))


def main():
    ap = argparse.ArgumentParser(description="Create account of dnx RTOS account files.")
    ap.add_argument("-i", type=int, default=None, help="PBKDF2 iterations")
    ap.add_argument("-g", action="store_true", help="generate random password")
    ap.add_argument("-u", type=int, default=None, help="user ID")
    ap.add_argument("-G", type=int, default=None, help="group ID")
    ap.add_argument("-c", default="", help="description")
    ap.add_argument("-H", default="", help="home directory")
    ap.add_argument("-S", default="", help="shell command")
    ap.add_argument("-o", default=None, help="directory of passwd and shadow files")
    ap.add_argument("user")
    ap.add_argument("services", nargs="?", default="*")
    args = ap.parse_args()

    if not re.fullmatch(r"[A-Za-z0-9._-]{1,32}", args.user):
        sys.exit("invalid user name")

    if not re.fullmatch(r"[A-Za-z0-9,*_-]{1,64}", args.services):
        sys.exit("invalid services")

    uid = args.u if args.u is not None else (0 if args.user == "root" else 1000)
    gid = args.G if args.G is not None else uid
    if not (0 <= uid <= 65535 and 0 <= gid <= 65535):
        sys.exit("invalid user or group ID")

    gecos = field(args.c, "description", 64)
    home = field(args.H, "home directory", 63)
    shell = field(args.S, "shell", 63)

    iterations = args.i if args.i is not None else (1 if args.g else ITERATIONS_DEFAULT)
    if not 1 <= iterations <= ITERATIONS_MAX:
        sys.exit("invalid number of iterations")

    if args.g:
        password = b64(secrets.token_bytes(16))
        print("password: " + password, file=sys.stderr)
    else:
        password = getpass.getpass("Password: ")
        if password != getpass.getpass("Repeat password: "):
            sys.exit("passwords differ")

    if not password or len(password) > 128 or re.search(r"[\r\n]", password):
        sys.exit("invalid password")

    salt = os.urandom(16)
    hash_ = hashlib.pbkdf2_hmac("sha256", password.encode(), salt, iterations, 32)

    passwd_line = "%s:x:%d:%d:%s:%s:%s\n" % (args.user, uid, gid, gecos, home, shell)
    shadow_line = "%s:$pbkdf2-sha256$%d$%s$%s:%s\n" % (args.user, iterations, b64(salt), b64(hash_), args.services)

    if args.o:
        passwd_path = os.path.join(args.o, "passwd")
        shadow_path = os.path.join(args.o, "shadow")
        if not user_exists(passwd_path, args.user):
            with open(passwd_path, "a") as f:
                f.write(passwd_line)
        with open(shadow_path, "a") as f:
            f.write(shadow_line)
    else:
        sys.stdout.write("# passwd\n" + passwd_line + "# shadow\n" + shadow_line)


if __name__ == "__main__":
    main()
