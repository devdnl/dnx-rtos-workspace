/*==============================================================================
@file   fdisk.c

@author Daniel Zorychta

@brief  Partition table editor (GPT and MBR)

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define SECTOR_SIZE    512
#define ALIGN_SECTORS  2048 /* 1 MiB */
#define GPT_ENTRIES    128
#define GPT_ENTRY_SIZE 128
#define GPT_HDR_SIZE   92
#define MBR_TABLE      0x1BE
#define MBR_TYPE_GPT   0xEE
#define NAME_CHARS     36

/*==============================================================================
  Local types, enums definitions
==============================================================================*/
/* on-disk structures (little-endian CPU) */
typedef struct __attribute__((packed)) {
        u8_t type[16];
        u8_t guid[16];
        u64_t first;
        u64_t last;
        u64_t attr;
        u16_t name[NAME_CHARS];
} gpt_entry_t;

typedef struct __attribute__((packed)) {
        char signature[8];
        u32_t revision;
        u32_t header_size;
        u32_t header_crc;
        u32_t reserved;
        u64_t my_lba;
        u64_t alt_lba;
        u64_t first_usable;
        u64_t last_usable;
        u8_t disk_guid[16];
        u64_t entries_lba;
        u32_t entries;
        u32_t entry_size;
        u32_t entries_crc;
} gpt_header_t;

typedef struct __attribute__((packed)) {
        u8_t status;
        u8_t chs_first[3];
        u8_t type;
        u8_t chs_last[3];
        u32_t first;
        u32_t sectors;
} mbr_entry_t;

typedef enum {
        SCHEME_NONE,
        SCHEME_MBR,
        SCHEME_GPT,
} scheme_t;

typedef struct {
        const char *name;
        u8_t mbr;
        const char *guid;
} ptype_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/
static void usage(const char *name);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        int fd;
        u64_t sectors;
        scheme_t scheme;
        bool modified;
        u8_t mbr[SECTOR_SIZE];
        gpt_header_t hdr;
        gpt_entry_t *entry;
};

static const ptype_t TYPES[] = {
        {"fat", 0x0C, "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"},
        {"linux", 0x83, "0FC63DAF-8483-4772-8E79-3D69D8477DE4"},
        {"swap", 0x82, "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F"},
        {"efi", 0xEF, "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"},
        {"dnx", 0xDA, "EDD0027B-CCD9-46D3-A941-BA3FB88153E2"},
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(fdisk, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Function calculates CRC32 (IEEE 802.3).
 */
//==============================================================================
static u32_t crc32(const void *buf, size_t len)
{
        const u8_t *p = buf;
        u32_t crc = 0xFFFFFFFF;

        while (len--) {
                crc ^= *p++;
                for (int i = 0; i < 8; i++) {
                        crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
                }
        }

        return ~crc;
}

//==============================================================================
/**
 * @brief Function reads or writes sectors.
 */
//==============================================================================
static int disk_io(u64_t lba, void *buf, size_t sectors, bool wr)
{
        size_t size = sectors * SECTOR_SIZE;

        if (lseek64(global->fd, (int64_t)(lba * SECTOR_SIZE), SEEK_SET) < 0) {
                return -1;
        }

        ssize_t n = wr ? write(global->fd, buf, size) : read(global->fd, buf, size);

        if (n != (ssize_t)size) {
                if (n >= 0) {
                        errno = EIO;
                }
                return -1;
        }

        return 0;
}

//==============================================================================
/**
 * @brief Function converts GUID string to on-disk format (mixed endian).
 */
//==============================================================================
static bool parse_guid(const char *str, u8_t *guid)
{
        static const u8_t ORDER[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};

        if (strlen(str) != 36) {
                return false;
        }

        for (int i = 0; i < 16; i++) {
                if (*str == '-') {
                        str++;
                }

                unsigned int b;
                char hex[3] = {str[0], str[1], 0};
                char *end;
                b = strtoul(hex, &end, 16);
                if (*end) {
                        return false;
                }

                guid[ORDER[i]] = b;
                str += 2;
        }

        return true;
}

//==============================================================================
/**
 * @brief Function prints GUID.
 */
//==============================================================================
static void print_guid(const u8_t *g)
{
        printf("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X", g[3], g[2], g[1], g[0],
                g[5], g[4], g[7], g[6], g[8], g[9], g[10], g[11], g[12], g[13], g[14], g[15]);
}

//==============================================================================
/**
 * @brief Function creates random GUID (version 4).
 */
//==============================================================================
static void random_guid(u8_t *guid)
{
        if (getentropy(guid, 16) != 0) {
                for (int i = 0; i < 16; i++) {
                        guid[i] = rand();
                }
        }

        guid[7] = (guid[7] & 0x0F) | 0x40;
        guid[8] = (guid[8] & 0x3F) | 0x80;
}

//==============================================================================
/**
 * @brief Function checks that GPT header and entries are valid.
 */
//==============================================================================
static bool read_gpt(u64_t lba)
{
        u8_t sector[SECTOR_SIZE];
        gpt_header_t *hdr = (gpt_header_t *)sector;

        if (disk_io(lba, sector, 1, false) != 0) {
                return false;
        }

        u32_t crc = hdr->header_crc;
        hdr->header_crc = 0;

        if ((memcmp(hdr->signature, "EFI PART", 8) != 0) || (hdr->header_size < GPT_HDR_SIZE) ||
                (hdr->header_size > SECTOR_SIZE) || (crc32(sector, hdr->header_size) != crc) ||
                (hdr->my_lba != lba) || (hdr->entries != GPT_ENTRIES) ||
                (hdr->entry_size != GPT_ENTRY_SIZE)) {
                return false;
        }

        if (disk_io(hdr->entries_lba, global->entry, GPT_ENTRIES * GPT_ENTRY_SIZE / SECTOR_SIZE, false) !=
                0) {
                return false;
        }

        if (crc32(global->entry, GPT_ENTRIES * GPT_ENTRY_SIZE) != hdr->entries_crc) {
                return false;
        }

        memcpy(&global->hdr, hdr, sizeof(gpt_header_t));

        return true;
}

//==============================================================================
/**
 * @brief Function creates new empty GPT.
 */
//==============================================================================
static void new_gpt(void)
{
        memset(global->entry, 0, GPT_ENTRIES * GPT_ENTRY_SIZE);
        memset(&global->hdr, 0, sizeof(gpt_header_t));
        memcpy(global->hdr.signature, "EFI PART", 8);
        global->hdr.revision = 0x10000;
        global->hdr.header_size = GPT_HDR_SIZE;
        global->hdr.first_usable = 2 + GPT_ENTRIES * GPT_ENTRY_SIZE / SECTOR_SIZE;
        global->hdr.last_usable = global->sectors - 2 - GPT_ENTRIES * GPT_ENTRY_SIZE / SECTOR_SIZE;
        global->hdr.entries = GPT_ENTRIES;
        global->hdr.entry_size = GPT_ENTRY_SIZE;
        random_guid(global->hdr.disk_guid);
        global->scheme = SCHEME_GPT;
        global->modified = true;
}

//==============================================================================
/**
 * @brief Function creates new empty MBR.
 */
//==============================================================================
static void new_mbr(void)
{
        memset(global->mbr + MBR_TABLE, 0, 4 * sizeof(mbr_entry_t));
        getentropy(global->mbr + 0x1B8, 4); /* disk signature */
        global->mbr[510] = 0x55;
        global->mbr[511] = 0xAA;
        global->scheme = SCHEME_MBR;
        global->modified = true;
}

//==============================================================================
/**
 * @brief Function reads partition table.
 */
//==============================================================================
static int read_table(void)
{
        if (disk_io(0, global->mbr, 1, false) != 0) {
                return -1;
        }

        /*
         * GPT: protective MBR (type 0xEE). Damaged protective MBR (no signature
         * or no partitions): GPT is used when its header is valid.
         */
        bool sig = (global->mbr[510] == 0x55) && (global->mbr[511] == 0xAA);
        bool pmbr = false;
        bool empty = true;

        const mbr_entry_t *e = (const mbr_entry_t *)(global->mbr + MBR_TABLE);
        for (int i = 0; sig && (i < 4); i++) {
                pmbr |= (e[i].type == MBR_TYPE_GPT);
                empty &= (e[i].sectors == 0);
        }

        global->scheme = sig ? SCHEME_MBR : SCHEME_NONE;

        if (pmbr || empty) {
                bool primary = read_gpt(1);
                bool backup = !primary && read_gpt(global->sectors - 1);

                if (primary || backup) {
                        global->scheme = SCHEME_GPT;

                        if (!pmbr) {
                                printf("Protective MBR damaged (-r: repair)\n");
                                if (!sig) {
                                        memset(global->mbr, 0, SECTOR_SIZE);
                                }
                        }

                        if (backup) {
                                printf("Primary GPT damaged, backup GPT used (-r: repair)\n");
                        }

                } else if (pmbr) {
                        printf("GPT damaged\n");
                        global->scheme = SCHEME_NONE;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief Function writes partition table.
 */
//==============================================================================
static int write_table(void)
{
        if (global->scheme == SCHEME_MBR) {
                return disk_io(0, global->mbr, 1, true);
        }

        const u32_t entry_sectors = GPT_ENTRIES * GPT_ENTRY_SIZE / SECTOR_SIZE;
        const u64_t last = global->sectors - 1;

        /* protective MBR (boot code is not changed) */
        memset(global->mbr + MBR_TABLE, 0, 4 * sizeof(mbr_entry_t));
        mbr_entry_t *pe = (mbr_entry_t *)(global->mbr + MBR_TABLE);
        pe->type = MBR_TYPE_GPT;
        pe->first = 1;
        pe->sectors = (last > UINT32_MAX) ? UINT32_MAX : last;
        memset(pe->chs_first, 0xFF, 3);
        memset(pe->chs_last, 0xFF, 3);
        pe->chs_first[0] = 0;
        pe->chs_first[1] = 2;
        global->mbr[510] = 0x55;
        global->mbr[511] = 0xAA;

        u8_t sector[SECTOR_SIZE];
        gpt_header_t *hdr = (gpt_header_t *)sector;

        global->hdr.entries_crc = crc32(global->entry, GPT_ENTRIES * GPT_ENTRY_SIZE);
        global->hdr.header_size = GPT_HDR_SIZE;

        /* backup first: primary table is valid until it is written */
        for (int backup = 1; backup >= 0; backup--) {
                memset(sector, 0, sizeof(sector));
                memcpy(hdr, &global->hdr, sizeof(gpt_header_t));
                hdr->my_lba = backup ? last : 1;
                hdr->alt_lba = backup ? 1 : last;
                hdr->entries_lba = backup ? (last - entry_sectors) : 2;
                hdr->header_crc = 0;
                hdr->header_crc = crc32(sector, GPT_HDR_SIZE);

                if ((disk_io(hdr->entries_lba, global->entry, entry_sectors, true) != 0) ||
                        (disk_io(hdr->my_lba, sector, 1, true) != 0)) {
                        return -1;
                }
        }

        return disk_io(0, global->mbr, 1, true);
}

//==============================================================================
/**
 * @brief Function returns partition geometry and type.
 *
 * @return true if partition exists.
 */
//==============================================================================
static bool get_part(int n, u64_t *first, u64_t *last)
{
        if (global->scheme == SCHEME_GPT) {
                static const u8_t UNUSED[16];
                const gpt_entry_t *e = &global->entry[n - 1];
                if ((n < 1) || (n > GPT_ENTRIES) || (memcmp(e->type, UNUSED, 16) == 0)) {
                        return false;
                }
                *first = e->first;
                *last = e->last;

        } else if (global->scheme == SCHEME_MBR) {
                const mbr_entry_t *e = (const mbr_entry_t *)(global->mbr + MBR_TABLE) + (n - 1);
                if ((n < 1) || (n > 4) || (e->type == 0) || (e->sectors == 0)) {
                        return false;
                }
                *first = e->first;
                *last = (u64_t)e->first + e->sectors - 1;

        } else {
                return false;
        }

        return true;
}

//==============================================================================
/**
 * @brief Function returns number of partition entries.
 */
//==============================================================================
static int max_parts(void)
{
        return (global->scheme == SCHEME_GPT) ? GPT_ENTRIES : (global->scheme == SCHEME_MBR) ? 4 : 0;
}

//==============================================================================
/**
 * @brief Function returns usable area of disk.
 */
//==============================================================================
static void usable(u64_t *first, u64_t *last)
{
        if (global->scheme == SCHEME_GPT) {
                *first = global->hdr.first_usable;
                *last = global->hdr.last_usable;
        } else {
                *first = 1;
                *last = global->sectors - 1;
                if (*last > UINT32_MAX) {
                        *last = UINT32_MAX;
                }
        }
}

//==============================================================================
/**
 * @brief Function returns partition which contains selected sector.
 */
//==============================================================================
static int find_part(u64_t sector, int skip)
{
        for (int n = 1; n <= max_parts(); n++) {
                u64_t first, last;
                if ((n != skip) && get_part(n, &first, &last) && (sector >= first) && (sector <= last)) {
                        return n;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief Function prints size in human readable format.
 */
//==============================================================================
static const char *format_size(u64_t sectors, char *buf, size_t len)
{
        static const char UNIT[] = "KMGT";
        u64_t size = sectors / 2;
        int u = 0;

        while ((size >= 10240) && (u < 3)) {
                size /= 1024;
                u++;
        }

        snprintf(buf, len, "%llu %ciB", (unsigned long long)size, UNIT[u]);
        return buf;
}

//==============================================================================
/**
 * @brief Function prints partition table.
 */
//==============================================================================
static void list(const char *disk)
{
        static const char *SCHEME[] = {"no partition table", "MBR", "GPT"};

        char size[16];

        printf("Disk %s: %s, %llu sectors, %s\n", disk, format_size(global->sectors, size, sizeof(size)),
                (unsigned long long)global->sectors, SCHEME[global->scheme]);

        if (global->scheme == SCHEME_NONE) {
                return;
        }

        printf("%2s %10s %10s %10s  %-8s  %s\n", "Nr", "Start", "End", "Size", "Type", "Name");

        for (int n = 1; n <= max_parts(); n++) {
                u64_t first, last;
                if (!get_part(n, &first, &last)) {
                        continue;
                }

                printf("%2d %10llu %10llu %10s  ", n, (unsigned long long)first, (unsigned long long)last,
                        format_size(last - first + 1, size, sizeof(size)));

                const char *tname = NULL;
                if (global->scheme == SCHEME_GPT) {
                        const gpt_entry_t *e = &global->entry[n - 1];
                        for (size_t t = 0; t < ARRAY_SIZE(TYPES) && !tname; t++) {
                                u8_t guid[16];
                                if (parse_guid(TYPES[t].guid, guid) && (memcmp(guid, e->type, 16) == 0)) {
                                        tname = TYPES[t].name;
                                }
                        }

                        if (tname) {
                                printf("%-8s  ", tname);
                        } else {
                                print_guid(e->type);
                                printf("  ");
                        }

                        for (int i = 0; (i < NAME_CHARS) && e->name[i]; i++) {
                                u16_t c = e->name[i];
                                if (c < 0x80) {
                                        putchar(c);
                                } else if (c < 0x800) {
                                        putchar(0xC0 | (c >> 6));
                                        putchar(0x80 | (c & 0x3F));
                                } else {
                                        putchar(0xE0 | (c >> 12));
                                        putchar(0x80 | ((c >> 6) & 0x3F));
                                        putchar(0x80 | (c & 0x3F));
                                }
                        }
                } else {
                        u8_t type = ((const mbr_entry_t *)(global->mbr + MBR_TABLE))[n - 1].type;
                        for (size_t t = 0; t < ARRAY_SIZE(TYPES) && !tname; t++) {
                                if (TYPES[t].mbr == type) {
                                        tname = TYPES[t].name;
                                }
                        }

                        if (tname) {
                                printf("%s", tname);
                        } else {
                                printf("0x%02X", type);
                        }
                }

                putchar('\n');
        }
}

//==============================================================================
/**
 * @brief Function parses number with optional K, M, G suffix (bytes), number
 *        without suffix is a number of sectors.
 */
//==============================================================================
static bool parse_sectors(const char *str, u64_t *sectors)
{
        char *end;
        unsigned long long v = strtoull(str, &end, 10);

        switch (*end) {
        case 'K':
        case 'k':
                v = v * 1024 / SECTOR_SIZE;
                end++;
                break;
        case 'M':
        case 'm':
                v = v * 1024 * 1024 / SECTOR_SIZE;
                end++;
                break;
        case 'G':
        case 'g':
                v = v * 1024 * 1024 * 1024 / SECTOR_SIZE;
                end++;
                break;
        default:
                break;
        }

        *sectors = v;

        return (end != str) && (*end == '\0' || *end == ':');
}

//==============================================================================
/**
 * @brief Function sets partition type.
 */
//==============================================================================
static bool set_type(int n, const char *type)
{
        const ptype_t *t = NULL;
        for (size_t i = 0; i < ARRAY_SIZE(TYPES) && !t; i++) {
                if (strcmp(TYPES[i].name, type) == 0) {
                        t = &TYPES[i];
                }
        }

        if (global->scheme == SCHEME_GPT) {
                return parse_guid(t ? t->guid : type, global->entry[n - 1].type);
        }

        char *end;
        unsigned long v = t ? t->mbr : strtoul(type, &end, 16);
        if ((!t && *end) || (v == 0) || (v > 0xFF)) {
                return false;
        }

        ((mbr_entry_t *)(global->mbr + MBR_TABLE))[n - 1].type = v;
        return true;
}

//==============================================================================
/**
 * @brief Function sets partition name (UTF-8 to UTF-16).
 */
//==============================================================================
static bool set_name(int n, const char *name)
{
        if (global->scheme != SCHEME_GPT) {
                return false;
        }

        u16_t dst[NAME_CHARS] = {0};
        const u8_t *s = (const u8_t *)name;
        int i = 0;

        while (*s) {
                if (i >= NAME_CHARS) {
                        return false;
                }

                u16_t c = *s++;
                if ((c >= 0xE0) && (s[0] & s[1] & 0x80)) {
                        c = ((c & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F);
                        s += 2;
                } else if ((c >= 0xC0) && (s[0] & 0x80)) {
                        c = ((c & 0x1F) << 6) | (s[0] & 0x3F);
                        s += 1;
                }

                dst[i++] = c;
        }

        memcpy(global->entry[n - 1].name, dst, sizeof(dst));

        return true;
}

//==============================================================================
/**
 * @brief Function returns partition number from argument (N or N:...).
 */
//==============================================================================
static int part_number(const char *arg, const char **rest)
{
        char *end;
        long n = strtol(arg, &end, 10);

        if ((end == arg) || (n < 0) || (n > max_parts()) || ((*end != ':') && (*end != '\0'))) {
                return -1;
        }

        *rest = (*end == ':') ? end + 1 : end;
        return n;
}

//==============================================================================
/**
 * @brief Function adds new partition: N:START:END.
 */
//==============================================================================
static bool add_part(const char *arg)
{
        const char *p;
        int n = part_number(arg, &p);
        u64_t ufirst, ulast, first = 0, last = 0, tmp;

        if (n < 0) {
                return false;
        }

        if (n == 0) {
                for (n = 1; (n <= max_parts()) && get_part(n, &tmp, &tmp); n++)
                        ;
        }

        if ((n > max_parts()) || get_part(n, &tmp, &tmp)) {
                printf("Partition %d already exists or no free entry\n", n);
                return false;
        }

        usable(&ufirst, &ulast);

        /* start: 0 or empty is first free aligned sector */
        if (*p && (*p != ':') && !parse_sectors(p, &first)) {
                return false;
        }

        if (first == 0) {
                first = ufirst;
                for (int k = 0; k <= max_parts(); k++) {
                        first = (first + ALIGN_SECTORS - 1) / ALIGN_SECTORS * ALIGN_SECTORS;
                        int c = find_part(first, 0);
                        if (c == 0) {
                                break;
                        }
                        get_part(c, &tmp, &first);
                        first++;
                }
        }

        /* end: 0 or empty is end of free area, +SIZE is size */
        p = strchr(p, ':');
        p = p ? p + 1 : "";

        if (*p == '+') {
                if (!parse_sectors(p + 1, &last) || (last == 0)) {
                        return false;
                }
                last = first + last - 1;

        } else if (*p && !parse_sectors(p, &last)) {
                return false;
        }

        if (last == 0) {
                last = ulast;
                for (int k = 1; k <= max_parts(); k++) {
                        u64_t f, l;
                        if (get_part(k, &f, &l) && (f > first) && (f <= last)) {
                                last = f - 1;
                        }
                }
        }

        if ((first < ufirst) || (last > ulast) || (first > last)) {
                printf("Partition %d out of range (%llu-%llu)\n", n, (unsigned long long)ufirst,
                        (unsigned long long)ulast);
                return false;
        }

        for (int k = 1; k <= max_parts(); k++) {
                u64_t f, l;
                if (get_part(k, &f, &l) && (first <= l) && (last >= f)) {
                        printf("Partition %d overlaps partition %d\n", n, k);
                        return false;
                }
        }

        if (global->scheme == SCHEME_GPT) {
                gpt_entry_t *e = &global->entry[n - 1];
                memset(e, 0, sizeof(*e));
                e->first = first;
                e->last = last;
                random_guid(e->guid);
        } else {
                mbr_entry_t *e = (mbr_entry_t *)(global->mbr + MBR_TABLE) + (n - 1);
                memset(e, 0, sizeof(*e));
                memset(e->chs_first, 0xFF, 3);
                memset(e->chs_last, 0xFF, 3);
                e->first = first;
                e->sectors = last - first + 1;
        }

        set_type(n, "linux");

        return true;
}

//==============================================================================
/**
 * @brief Function checks that partition of disk is not mounted.
 */
//==============================================================================
static bool is_mounted(const char *disk)
{
        size_t len = strlen(disk);
        struct mntent mnt;

        for (int i = 0; getmntentry(i, &mnt) == 0; i++) {
                if (mnt.mnt_src && (strncmp(mnt.mnt_src, disk, len) == 0)) {
                        char c = mnt.mnt_src[len];
                        if ((c == '\0') || (c == 'p') || (c == '-') || ((c >= '0') && (c <= '9'))) {
                                printf("%s is mounted in %s\n", mnt.mnt_src, mnt.mnt_dir);
                                return true;
                        }
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief Main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *disk = NULL;
        bool force = false;

        for (int i = 1; i < argc; i++) {
                if (argv[i][0] != '-') {
                        disk = argv[i];
                } else if (strcmp(argv[i], "-f") == 0) {
                        force = true;
                } else if (strchr("gmr", argv[i][1]) && (argv[i][2] == '\0')) {
                        continue;
                } else if (strchr("ntcx", argv[i][1]) && (argv[i][2] == '\0') && (i + 1 < argc)) {
                        i++;
                } else {
                        usage(argv[0]);
                        return EXIT_FAILURE;
                }
        }

        if (!disk) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }

        bool edit = (argc > 2);

        global->fd = open(disk, edit ? O_RDWR : O_RDONLY);
        if (global->fd < 0) {
                perror(disk);
                return EXIT_FAILURE;
        }

        int status = EXIT_FAILURE;
        struct stat st;

        global->entry = malloc(GPT_ENTRIES * GPT_ENTRY_SIZE);

        if (!global->entry || (fstat(global->fd, &st) != 0)) {
                perror(disk);

        } else if ((global->sectors = st.st_size / SECTOR_SIZE) < 64) {
                printf("%s: disk too small\n", disk);

        } else if (read_table() != 0) {
                perror(disk);

        } else {
                status = EXIT_SUCCESS;

                for (int i = 1; (i < argc) && (status == EXIT_SUCCESS); i++) {
                        const char *opt = argv[i];
                        const char *arg = (i + 1 < argc) ? argv[i + 1] : "";
                        const char *rest = "";
                        bool ok = true;

                        if (opt[0] != '-' || opt[1] == 'f') {
                                continue;
                        }

                        if (opt[1] == 'r') {
                                global->modified = true;
                                continue;
                        }

                        if ((opt[1] == 'g') || (opt[1] == 'm')) {
                                u64_t f, l;
                                bool has_parts = false;
                                for (int k = 1; k <= max_parts(); k++) {
                                        has_parts |= get_part(k, &f, &l);
                                }

                                if (has_parts && !force) {
                                        printf("%s: partition table exists (use -f)\n", disk);
                                        status = EXIT_FAILURE;
                                } else if (opt[1] == 'g') {
                                        new_gpt();
                                } else {
                                        new_mbr();
                                }
                                continue;
                        }

                        i++;

                        if (global->scheme == SCHEME_NONE) {
                                printf("%s: no partition table (use -g or -m)\n", disk);
                                status = EXIT_FAILURE;
                                break;
                        }

                        int n = part_number(arg, &rest);

                        switch (opt[1]) {
                        case 'n':
                                ok = add_part(arg);
                                break;

                        case 't':
                        case 'c':
                        case 'x': {
                                u64_t f, l;
                                if ((n < 1) || !get_part(n, &f, &l)) {
                                        printf("Partition %s not exist\n", arg);
                                        ok = false;
                                } else if (opt[1] == 't') {
                                        ok = set_type(n, rest);
                                } else if (opt[1] == 'c') {
                                        ok = set_name(n, rest);
                                } else if (global->scheme == SCHEME_GPT) {
                                        memset(&global->entry[n - 1], 0, GPT_ENTRY_SIZE);
                                } else {
                                        memset((mbr_entry_t *)(global->mbr + MBR_TABLE) + (n - 1), 0,
                                                sizeof(mbr_entry_t));
                                }
                                break;
                        }
                        }

                        if (!ok) {
                                printf("%s: invalid argument: %s %s\n", disk, opt, arg);
                                status = EXIT_FAILURE;
                        }

                        global->modified = true;
                }

                if ((status == EXIT_SUCCESS) && global->modified && (global->scheme != SCHEME_NONE)) {
                        if (is_mounted(disk)) {
                                status = EXIT_FAILURE;

                        } else if (write_table() != 0) {
                                perror(disk);
                                status = EXIT_FAILURE;

                        } else {
                                /* partitions of PART driver are updated (other devices: not supported) */
                                if ((ioctl(global->fd, IOCTL_STORAGE__READ_PARTITION_TABLE) != 0) &&
                                        (errno != EBADRQC)) {
                                        perror(disk);
                                }
                        }
                }

                if (status == EXIT_SUCCESS) {
                        list(disk);
                }
        }

        free(global->entry);
        close(global->fd);

        return status;
}

//==============================================================================
/**
 * @brief Function prints help.
 */
//==============================================================================
static void usage(const char *name)
{
        printf("Usage: %s [options] <disk>\n", name);
        printf("Without options partition table is printed. Options are executed in order,\n");
        printf("partition table is written once at the end.\n");
        printf("  -g              create new GPT\n");
        printf("  -m              create new MBR\n");
        printf("  -f              replace existing partition table (-g, -m)\n");
        printf("  -r              write partition table (repair damaged GPT)\n");
        printf("  -n N:START:END  add partition N (0: first free entry), START: sector or\n");
        printf("                  offset with K/M/G suffix (0 or empty: first free, 1 MiB\n");
        printf("                  aligned), END: sector, +SIZE (sectors or K/M/G), 0 or empty:\n");
        printf("                  end of free space\n");
        printf("  -t N:TYPE       partition type: fat, linux, swap, efi, dnx, GUID (GPT) or\n");
        printf("                  hex number (MBR)\n");
        printf("  -c N:NAME       partition name (GPT)\n");
        printf("  -x N            delete partition\n");
        printf("Example: %s -g -n 0::+64M -t 1:fat -c 1:boot -n 0:: -c 2:data /dev/sda\n", name);
}

/*==============================================================================
  End of file
==============================================================================*/
