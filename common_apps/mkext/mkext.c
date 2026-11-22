/*==============================================================================
@file   mkext.c

@author Daniel Zorychta

@brief  Create EXT2/3/4 file system.

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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <dnx/misc.h>

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
/*
 * Layout (no flex_bg, no meta_bg, no resize inode): every group starts with
 * its metadata: superblock and group descriptor table (groups 0, 1 and powers
 * of 3, 5, 7), block bitmap, inode bitmap, inode table. Root directory,
 * lost+found and journal are placed in group 0 after metadata.
 * EXT4: extents and metadata checksums, groups 1.. are not initialized (lazy
 * initialization by driver). EXT2/EXT3: all bitmaps and inode tables written.
 */
#define INODE_SIZE  256
#define EXTRA_ISIZE 32
#define FIRST_INO   11
#define ROOT_INO    2
#define JOURNAL_INO 8
#define LPF_INO     11
#define DESC_SIZE   32
#define SMALL_FS    (512UL * 1024 * 1024)

#define FCOM_HAS_JOURNAL  0x0004
#define FCOM_DIR_INDEX    0x0020
#define FINCOM_FILETYPE   0x0002
#define FINCOM_EXTENTS    0x0040
#define FRO_SPARSE_SUPER  0x0001
#define FRO_LARGE_FILE    0x0002
#define FRO_HUGE_FILE     0x0008
#define FRO_DIR_NLINK     0x0020
#define FRO_EXTRA_ISIZE   0x0040
#define FRO_METADATA_CSUM 0x0400

#define BG_INODE_UNINIT  0x0001
#define BG_BLOCK_UNINIT  0x0002
#define BG_ITABLE_ZEROED 0x0004

#define INODE_FLAG_EXTENTS 0x80000
#define FT_DIR             2

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        int fd;
        u8_t *buf;        /* block buffer */
        u32_t bs;         /* block size */
        u32_t bpg;        /* blocks per group */
        u32_t blocks;     /* number of blocks */
        u32_t first_data; /* first data block */
        u32_t groups;     /* number of groups */
        u32_t gdt_blocks; /* group descriptor table blocks */
        u32_t ipg;        /* inodes per group */
        u32_t itb;        /* inode table blocks per group */
        u32_t data;       /* first free block of group 0 */
        u32_t jblocks;    /* journal size */
        u32_t jmeta;      /* journal indirect blocks (EXT3) */
        u32_t free_blocks;
        u32_t now;
        u32_t seed; /* metadata checksum seed */
        u32_t fcom, fincom, fro;
        bool csum;
        bool extents;
        u8_t uuid[16];
        u8_t hash_seed[16];
        const char *label;
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(mkext, STACK_DEPTH_LOW);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Store little-endian numbers.
 */
//==============================================================================
static void st16(u8_t *p, u32_t v)
{
        p[0] = v;
        p[1] = v >> 8;
}

static void st32(u8_t *p, u32_t v)
{
        st16(p, v);
        st16(p + 2, v >> 16);
}

//==============================================================================
/**
 * @brief Store big-endian number (journal).
 */
//==============================================================================
static void stbe32(u8_t *p, u32_t v)
{
        p[0] = v >> 24;
        p[1] = v >> 16;
        p[2] = v >> 8;
        p[3] = v;
}

//==============================================================================
/**
 * @brief CRC32C (Castagnoli) without pre and post inversion (as ext4).
 */
//==============================================================================
static u32_t crc32c(u32_t crc, const void *buf, size_t len)
{
        const u8_t *p = buf;

        while (len--) {
                crc ^= *p++;
                for (int k = 0; k < 8; k++) {
                        crc = (crc >> 1) ^ (0x82F63B78 & -(crc & 1));
                }
        }

        return crc;
}

//==============================================================================
/**
 * @brief CRC32C of 32-bit little-endian number.
 */
//==============================================================================
static u32_t crc32c_u32(u32_t crc, u32_t v)
{
        u8_t le[4];
        st32(le, v);
        return crc32c(crc, le, 4);
}

//==============================================================================
/**
 * @brief Group contains superblock backup (sparse_super): 0, 1, 3^n, 5^n, 7^n.
 */
//==============================================================================
static bool has_super(u32_t g)
{
        if (g <= 1) {
                return true;
        }

        for (u32_t b = 3; b <= 7; b += 2) {
                u32_t n = b;
                while (n < g) {
                        n *= b;
                }
                if (n == g) {
                        return true;
                }
        }

        return false;
}

//==============================================================================
/**
 * @brief Group layout.
 */
//==============================================================================
static u32_t group_start(u32_t g)
{
        return global->first_data + g * global->bpg;
}

static u32_t group_blocks(u32_t g)
{
        return (g < global->groups - 1) ? global->bpg : global->blocks - group_start(g);
}

static u32_t block_bitmap(u32_t g)
{
        return group_start(g) + (has_super(g) ? 1 + global->gdt_blocks : 0);
}

static u32_t used_blocks(u32_t g)
{
        u32_t used = block_bitmap(g) - group_start(g) + 2 + global->itb;

        if (g == 0) {
                used += 2 + global->jmeta + global->jblocks; /* root, lost+found, journal */
        }

        return used;
}

//==============================================================================
/**
 * @brief Number of indirect blocks of EXT3 journal.
 */
//==============================================================================
static u32_t journal_meta(u32_t n)
{
        u32_t p = global->bs / 4;

        if (global->extents || (n <= 12)) {
                return 0;
        }

        n -= 12;
        if (n <= p) {
                return 1;
        }

        return 2 + (n - p + p - 1) / p;
}

//==============================================================================
/**
 * @brief Byte of bitmap: bits below used and from limit are set.
 */
//==============================================================================
static u8_t bitmap_byte(u32_t used, u32_t limit, u32_t i)
{
        u8_t v = 0;

        for (int k = 0; k < 8; k++) {
                u32_t bit = i * 8 + k;
                if ((bit < used) || (bit >= limit)) {
                        v |= 1 << k;
                }
        }

        return v;
}

static u16_t bitmap_csum(u32_t used, u32_t limit, u32_t len)
{
        u32_t crc = global->seed;

        for (u32_t i = 0; i < len; i++) {
                u8_t b = bitmap_byte(used, limit, i);
                crc = crc32c(crc, &b, 1);
        }

        return crc;
}

//==============================================================================
/**
 * @brief Write buffer at byte offset.
 */
//==============================================================================
static int write_at(u64_t offset, const void *buf, size_t len)
{
        if ((lseek64(global->fd, offset, SEEK_SET) < 0) || (write(global->fd, buf, len) != (ssize_t)len)) {
                perror("write");
                return -1;
        }

        return 0;
}

static int write_block(u32_t blk)
{
        return write_at((u64_t)blk * global->bs, global->buf, global->bs);
}

//==============================================================================
/**
 * @brief Create block map of inode (i_block): extent or direct and indirect
 *        blocks (EXT3 journal: indirect blocks before data).
 *
 * @param blk           i_block (60 bytes, zeroed)
 * @param start         first block
 * @param len           number of blocks
 */
//==============================================================================
static void block_map(u8_t *blk, u32_t start, u32_t len)
{
        if (global->extents) {
                st16(blk + 0, 0xF30A); /* extent header: magic, entries, max, depth */
                st16(blk + 2, 1);
                st16(blk + 4, 4);
                st16(blk + 12 + 4, len); /* extent: logical 0, length, start */
                st32(blk + 12 + 8, start);
        } else {
                for (u32_t k = 0; (k < 12) && (k < len); k++) {
                        st32(blk + 4 * k, start + k);
                }
                if (len > 12) {
                        st32(blk + 48, global->data + 2); /* indirect */
                }
                if (len > 12 + global->bs / 4) {
                        st32(blk + 52, global->data + 3); /* double indirect */
                }
        }
}

//==============================================================================
/**
 * @brief Create inode (root, lost+found, journal).
 *
 * @param ino           inode number
 * @param p             inode buffer (zeroed)
 */
//==============================================================================
static void make_inode(u32_t ino, u8_t *p)
{
        u32_t mode, links = 1, start, len = 1, meta = 0;

        if (ino == ROOT_INO) {
                mode = 040777;
                links = 3;
                start = global->data;
        } else if (ino == LPF_INO) {
                mode = 040700;
                links = 2;
                start = global->data + 1;
        } else if ((ino == JOURNAL_INO) && global->jblocks) {
                mode = 0100600;
                meta = global->jmeta;
                start = global->data + 2 + meta;
                len = global->jblocks;
        } else {
                return;
        }

        st16(p + 0x00, mode);
        st32(p + 0x04, len * global->bs);
        st32(p + 0x08, global->now);
        st32(p + 0x0C, global->now);
        st32(p + 0x10, global->now);
        st16(p + 0x1A, links);
        st32(p + 0x1C, (len + meta) * (global->bs / 512));

        if (global->extents) {
                st32(p + 0x20, INODE_FLAG_EXTENTS);
        }

        block_map(p + 0x28, start, len);

        st16(p + 0x80, EXTRA_ISIZE);

        if (global->csum) {
                u32_t crc = crc32c_u32(crc32c_u32(global->seed, ino), 0);
                crc = crc32c(crc, p, INODE_SIZE);
                st16(p + 0x7C, crc);
                st16(p + 0x82, crc >> 16);
        }
}

//==============================================================================
/**
 * @brief Create superblock.
 *
 * @param g             group number
 * @param p             superblock buffer (1024 bytes, zeroed)
 */
//==============================================================================
static void make_super(u32_t g, u8_t *p)
{
        u32_t log = 0;
        while ((1024U << log) < global->bs) {
                log++;
        }

        st32(p + 0x00, global->groups * global->ipg);
        st32(p + 0x04, global->blocks);
        st32(p + 0x0C, global->free_blocks);
        st32(p + 0x10, global->groups * global->ipg - FIRST_INO);
        st32(p + 0x14, global->first_data);
        st32(p + 0x18, log);
        st32(p + 0x1C, log);
        st32(p + 0x20, global->bpg);
        st32(p + 0x24, global->bpg);
        st32(p + 0x28, global->ipg);
        st32(p + 0x30, global->now);
        st16(p + 0x36, 0xFFFF);
        st16(p + 0x38, 0xEF53);
        st16(p + 0x3A, 1); /* clean */
        st16(p + 0x3C, 1); /* errors: continue */
        st32(p + 0x40, global->now);
        st32(p + 0x4C, 1); /* dynamic revision */
        st32(p + 0x54, FIRST_INO);
        st16(p + 0x58, INODE_SIZE);
        st16(p + 0x5A, g);
        st32(p + 0x5C, global->fcom);
        st32(p + 0x60, global->fincom);
        st32(p + 0x64, global->fro);
        memcpy(p + 0x68, global->uuid, 16);
        strncpy((char *)p + 0x78, global->label, 16);

        if (global->jblocks) {
                st32(p + 0xE0, JOURNAL_INO);
                block_map(p + 0x10C, global->data + 2 + global->jmeta,
                        global->jblocks); /* journal inode backup */
                st32(p + 0x10C + 64, global->jblocks * global->bs);
                p[0xFD] = 1;
        }

        memcpy(p + 0xEC, global->hash_seed, 16);
        p[0xFC] = 1; /* half MD4 */
        st32(p + 0x108, global->now);
        st16(p + 0x15C, EXTRA_ISIZE);
        st16(p + 0x15E, EXTRA_ISIZE);
        st32(p + 0x160, 1); /* signed directory hash */

        if (global->csum) {
                p[0x175] = 1; /* crc32c */
                st32(p + 0x3FC, crc32c(~0U, p, 0x3FC));
        }
}

//==============================================================================
/**
 * @brief Create group descriptor.
 *
 * @param g             group number
 * @param p             descriptor buffer (zeroed)
 */
//==============================================================================
static void make_desc(u32_t g, u8_t *p)
{
        u32_t bb = block_bitmap(g);

        st32(p + 0x00, bb);
        st32(p + 0x04, bb + 1);
        st32(p + 0x08, bb + 2);
        st16(p + 0x0C, group_blocks(g) - used_blocks(g));
        st16(p + 0x0E, global->ipg - (g ? 0 : FIRST_INO));
        st16(p + 0x10, g ? 0 : 2);

        if (global->csum) {
                u16_t flags = BG_ITABLE_ZEROED;

                if (g > 0) {
                        flags = BG_INODE_UNINIT | ((g < global->groups - 1) ? BG_BLOCK_UNINIT : 0);
                }

                st16(p + 0x12, flags);

                if (!(flags & BG_BLOCK_UNINIT)) {
                        st16(p + 0x18, bitmap_csum(used_blocks(g), group_blocks(g), global->bs));
                }

                if (g == 0) {
                        st16(p + 0x1A, bitmap_csum(FIRST_INO, global->ipg, global->ipg / 8));
                }

                st16(p + 0x1C, global->ipg - (g ? 0 : FIRST_INO));
                st16(p + 0x1E, crc32c(crc32c_u32(global->seed, g), p, DESC_SIZE));
        }
}

//==============================================================================
/**
 * @brief Create directory block.
 */
//==============================================================================
static void dirent(u8_t *p, u32_t ino, u32_t rec_len, const char *name)
{
        st32(p, ino);
        st16(p + 4, rec_len);
        p[6] = strlen(name);
        p[7] = FT_DIR;
        memcpy(p + 8, name, p[6]);
}

static void make_dir(u32_t ino)
{
        u8_t *p = global->buf;
        u32_t end = global->bs - (global->csum ? 12 : 0);

        memset(p, 0, global->bs);
        dirent(p, ino, 12, ".");

        if (ino == ROOT_INO) {
                dirent(p + 12, ROOT_INO, 12, "..");
                dirent(p + 24, LPF_INO, end - 24, "lost+found");
        } else {
                dirent(p + 12, ROOT_INO, end - 12, "..");
        }

        if (global->csum) {
                u8_t *tail = p + end;
                st16(tail + 4, 12);
                tail[7] = 0xDE;
                st32(tail + 8, crc32c(crc32c_u32(crc32c_u32(global->seed, ino), 0), p, end));
        }
}

//==============================================================================
/**
 * @brief Calculate file system geometry.
 *
 * @param size          device size in bytes
 *
 * @return 0 on success.
 */
//==============================================================================
static int geometry(u64_t size)
{
        u64_t blocks = size / global->bs;
        u32_t ratio = (size < SMALL_FS) ? 4096 : 16384;
        u32_t ipb = global->bs / INODE_SIZE;
        u32_t align = (ipb > 8) ? ipb : 8;

        if (blocks > UINT32_MAX) {
                printf("Device too big for block size %lu\n", (unsigned long)global->bs);
                return -1;
        }

        global->bpg = global->bs * 8;
        global->first_data = (global->bs == 1024) ? 1 : 0;
        global->blocks = blocks;

        while (global->blocks > global->first_data) {
                global->groups = (global->blocks - global->first_data + global->bpg - 1) / global->bpg;
                global->gdt_blocks = (global->groups * DESC_SIZE + global->bs - 1) / global->bs;

                u64_t inodes = ((u64_t)global->blocks * global->bs) / ratio;
                u32_t ipg = (inodes + global->groups - 1) / global->groups;
                ipg = (ipg + align - 1) / align * align;
                if (ipg < 16) {
                        ipg = 16;
                }
                if (ipg > global->bpg) {
                        ipg = global->bpg;
                }
                global->ipg = ipg;
                global->itb = ipg * INODE_SIZE / global->bs;

                /* last group too small for its metadata: removed */
                u32_t last = global->groups - 1;
                if ((last > 0) && (group_blocks(last) < used_blocks(last) + 50)) {
                        global->blocks -= group_blocks(last);
                        continue;
                }

                break;
        }

        global->data = block_bitmap(0) + 2 + global->itb;

        if ((global->blocks <= global->first_data) || (group_blocks(0) < used_blocks(0) + 16)) {
                printf("Device too small\n");
                return -1;
        }

        if (global->jblocks) {
                u32_t avail = group_blocks(0) - used_blocks(0);
                u32_t n = (global->blocks < 32768) ? 1024 : (global->blocks < 262144) ? 4096 : 8192;

                if (n > avail) {
                        n = avail;
                }
                while ((n > 0) && (n + journal_meta(n) > avail)) {
                        n--;
                }

                if (n < 1024) {
                        printf("Device too small for journal\n");
                        return -1;
                }

                global->jblocks = n;
                global->jmeta = journal_meta(n);
        }

        global->free_blocks = 0;
        for (u32_t g = 0; g < global->groups; g++) {
                global->free_blocks += group_blocks(g) - used_blocks(g);
        }

        return 0;
}

//==============================================================================
/**
 * @brief Write bitmaps and inode tables.
 */
//==============================================================================
static int write_groups(void)
{
        u8_t *buf = global->buf;

        for (u32_t g = 0; g < global->groups; g++) {
                u32_t bb = block_bitmap(g);

                if (!global->csum || (g == 0) || (g == global->groups - 1)) {
                        for (u32_t i = 0; i < global->bs; i++) {
                                buf[i] = bitmap_byte(used_blocks(g), group_blocks(g), i);
                        }
                        if (write_block(bb) != 0) {
                                return -1;
                        }
                }

                if (global->csum && (g > 0)) {
                        continue; /* inode bitmap and table not initialized */
                }

                for (u32_t i = 0; i < global->bs; i++) {
                        buf[i] = bitmap_byte(g ? 0 : FIRST_INO, global->ipg, i);
                }
                if (write_block(bb + 1) != 0) {
                        return -1;
                }

                u32_t ipb = global->bs / INODE_SIZE;
                for (u32_t k = 0; k < global->itb; k++) {
                        memset(buf, 0, global->bs);
                        for (u32_t j = 0; (g == 0) && (j < ipb); j++) {
                                make_inode(k * ipb + j + 1, buf + j * INODE_SIZE);
                        }
                        if (write_block(bb + 2 + k) != 0) {
                                return -1;
                        }
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief Write root directory, lost+found and journal.
 */
//==============================================================================
static int write_data(void)
{
        u8_t *buf = global->buf;

        make_dir(ROOT_INO);
        if (write_block(global->data) != 0) {
                return -1;
        }

        make_dir(LPF_INO);
        if (write_block(global->data + 1) != 0) {
                return -1;
        }

        if (!global->jblocks) {
                return 0;
        }

        /* EXT3 journal indirect blocks: single, double, indirect of double */
        u32_t p = global->bs / 4;
        u32_t meta0 = global->data + 2;
        u32_t jstart = meta0 + global->jmeta;

        for (u32_t m = 0; m < global->jmeta; m++) {
                memset(buf, 0, global->bs);
                for (u32_t j = 0; j < p; j++) {
                        u32_t l = (m == 0) ? 12 + j : 12 + p + (m - 2) * p + j;
                        if (m == 1) {
                                if (j < global->jmeta - 2) {
                                        st32(buf + 4 * j, meta0 + 2 + j);
                                }
                        } else if (l < global->jblocks) {
                                st32(buf + 4 * j, jstart + l);
                        }
                }
                if (write_block(meta0 + m) != 0) {
                        return -1;
                }
        }

        /* journal superblock (v2, empty), other blocks zeroed */
        memset(buf, 0, global->bs);
        stbe32(buf + 0x00, 0xC03B3998);
        stbe32(buf + 0x04, 4);
        stbe32(buf + 0x0C, global->bs);
        stbe32(buf + 0x10, global->jblocks);
        stbe32(buf + 0x14, 1);
        stbe32(buf + 0x18, 1);
        memcpy(buf + 0x30, global->uuid, 16);
        stbe32(buf + 0x40, 1);
        if (write_block(jstart) != 0) {
                return -1;
        }

        memset(buf, 0, global->bs);
        for (u32_t l = 1; l < global->jblocks; l++) {
                if (write_block(jstart + l) != 0) {
                        return -1;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief Write superblocks and group descriptor tables (last: device is
 *        recognized only when formatting is finished).
 */
//==============================================================================
static int write_super(void)
{
        u8_t *buf = global->buf;
        u32_t dpb = global->bs / DESC_SIZE;

        for (u32_t g = 0; g < global->groups; g++) {
                if (!has_super(g)) {
                        continue;
                }

                memset(buf, 0, global->bs);
                make_super(g, buf);

                int err = (g == 0) ? write_at(1024, buf, 1024) : write_block(group_start(g));
                if (err) {
                        return -1;
                }

                for (u32_t k = 0; k < global->gdt_blocks; k++) {
                        memset(buf, 0, global->bs);
                        for (u32_t d = 0; (d < dpb) && (k * dpb + d < global->groups); d++) {
                                make_desc(k * dpb + d, buf + d * DESC_SIZE);
                        }
                        if (write_block(group_start(g) + 1 + k) != 0) {
                                return -1;
                        }
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *input_file = NULL;
        int ext_version = 2;
        bool journal = false;

        global->label = "";

        for (int i = 1; i < argc; i++) {
                bool has_arg = (i + 1 < argc);

                if (isstreq(argv[i], "-b") && has_arg) {
                        global->bs = strtol(argv[++i], NULL, 0);

                } else if (isstreq(argv[i], "-e") && has_arg) {
                        ext_version = strtol(argv[++i], NULL, 10);

                } else if (isstreq(argv[i], "-l") && has_arg) {
                        global->label = argv[++i];

                } else if (isstreq(argv[i], "-i") && has_arg) {
                        input_file = argv[++i];

                } else if (isstreq(argv[i], "-j")) {
                        journal = true;

                } else {
                        input_file = NULL;
                        break;
                }
        }

        if (!input_file || (ext_version < 2) || (ext_version > 4) || (journal && (ext_version == 2)) ||
                (global->bs && (global->bs != 1024) && (global->bs != 2048) && (global->bs != 4096))) {
                printf("Usage: %s [options] -i <device>\n", argv[0]);
                printf(" -b     block size: 1024, 2048, 4096 (default: 1024, 4096 from 512 MiB)\n");
                printf(" -e     ext version: 2 (default), 3, 4\n");
                printf(" -l     label (up to 16 characters)\n");
                printf(" -j     journal (ext3 and ext4)\n");
                printf(" -i     device\n");
                return EXIT_FAILURE;
        }

        global->fd = open(input_file, O_RDWR);
        if (global->fd < 0) {
                perror(input_file);
                return EXIT_FAILURE;
        }

        int status = EXIT_FAILURE;
        struct stat st;

        if (fstat(global->fd, &st) != 0) {
                perror(input_file);
                goto finish;
        }

        if (!global->bs) {
                global->bs = (st.st_size < SMALL_FS) ? 1024 : 4096;
        }

        global->now = time(NULL);
        global->jblocks = journal;
        global->fincom = FINCOM_FILETYPE;
        global->fro = FRO_SPARSE_SUPER | FRO_LARGE_FILE;
        global->fcom = (ext_version >= 3) ? FCOM_DIR_INDEX : 0;
        global->fcom |= journal ? FCOM_HAS_JOURNAL : 0;

        if (ext_version == 4) {
                global->extents = true;
                global->csum = true;
                global->fincom |= FINCOM_EXTENTS;
                global->fro |= FRO_HUGE_FILE | FRO_DIR_NLINK | FRO_EXTRA_ISIZE | FRO_METADATA_CSUM;
        }

        getentropy(global->uuid, sizeof(global->uuid));
        global->uuid[6] = (global->uuid[6] & 0x0F) | 0x40;
        global->uuid[8] = (global->uuid[8] & 0x3F) | 0x80;
        getentropy(global->hash_seed, sizeof(global->hash_seed));
        global->seed = crc32c(~0U, global->uuid, sizeof(global->uuid));

        global->buf = malloc(global->bs);

        if (!global->buf) {
                perror(NULL);

        } else if ((geometry(st.st_size) == 0) && (write_groups() == 0) && (write_data() == 0) &&
                (write_super() == 0)) {

                printf("Created ext%d file system:\n", ext_version);
                printf("Block size: %lu\n", (unsigned long)global->bs);
                printf("Blocks: %lu (groups: %lu)\n", (unsigned long)global->blocks,
                        (unsigned long)global->groups);
                printf("Inodes: %lu (per group: %lu)\n", (unsigned long)(global->ipg * global->groups),
                        (unsigned long)global->ipg);
                printf("Journal blocks: %lu\n", (unsigned long)global->jblocks);
                printf("Label: %s\n", global->label);
                status = EXIT_SUCCESS;
        }

finish:
        free(global->buf);
        close(global->fd);

        return status;
}

/*==============================================================================
  End of file
==============================================================================*/
