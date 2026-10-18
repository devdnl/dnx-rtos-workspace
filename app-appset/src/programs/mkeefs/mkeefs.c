/*==============================================================================
File    mkeefs.c

Author  Daniel Zorychta

Brief   Format selected file by EEFS.

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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dnx/misc.h>
#include <sys/stat.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define BLOCK_SIZE         128
#define BLOCKS_IN_BYTE     8
#define ENTRY_TYPE_NONE    0xFF

#define MIN_BLOCKS         8     // the same limit as in the driver
#define MAX_BLOCKS         65535 // 16-bit block numbers

#define BLOCK_MAIN_MAGIC   0x53464545
#define BLOCK_MAGIC_DIR    0x30524944
#define BLOCK_MAGIC_BITMAP 0x20504D42

#define BMP_MAIN_BITS (sizeof(((block_main_t *)0)->bitmap) * BLOCKS_IN_BYTE)
#define BMP_BLK_BITS  (sizeof(((block_bitmap_t *)0)->map) * BLOCKS_IN_BYTE)

/*==============================================================================
  Local object types
==============================================================================*/
/**
 * FS structure:
 * Block        Structure
 * 0            block_main (defines number of additional bitmap blocks)
 * 1            block_bitmap 1 (if needed)
 * 2            block_bitmap 2 (if needed)
 * n            block_bitmap n (if needed)
 * n+1          dir_entry root (position calculated by using block_main definitions)
 *
 * Next blocks according to block position address defined in root dir entry.
 */

/**
 * Generic checksum block (calculation purposes).
 */
typedef struct PACKED {
        uint8_t buf[126];
        uint16_t checksum;
} block_chsum_t;

/**
 * Main block definition (block 0).
 */
typedef struct PACKED {
        uint32_t magic;        //!< FS identifier ('EEFS')
        uint16_t blocks;       //!< number of memory 128B blocks
        uint8_t bitmap_blocks; //!< number of next bitmap blocks
        uint8_t bitmap[119];   //!< empty block bitmap
        uint16_t checksum;     //!< checksum
} block_main_t;

/**
 * Bitmap block definition.
 */
typedef struct PACKED {
        uint32_t magic;    //!< block magic number
        uint8_t map[122];  //!< bitmap
        uint16_t checksum; //!< checksum
} block_bitmap_t;

/**
 * Single directory entry.
 */
typedef struct PACKED {
        uint16_t block_addr; //!< block address with object
        uint8_t type;        //!< entry type
        char name[21];       //!< entry name
} dir_entry_t;

/**
 * Directory block.
 */
typedef struct PACKED {
        uint32_t magic;       //!< block magic number
        uint32_t ctime;       //!< create timestamp
        uint32_t mtime;       //!< modification timestamp
        uint16_t gid;         //!< group ID
        uint16_t uid;         //!< user ID
        uint16_t mode;        //!< mode
        uint16_t parent;      //!< parent entry block number
        uint16_t next;        //!< next directory entries
        uint16_t all_items;   //!< number of items in all chains
        uint8_t _[6];         //!< reserved
        dir_entry_t entry[4]; //!< directory entry
        uint16_t checksum;    //!< checksum
} block_dir_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        FILE *file;
        size_t mem_size;

        union {
                block_main_t main;
                block_bitmap_t bitmap;
                block_dir_t dir;
                block_chsum_t chsum;
        } block;
};

PROGRAM_PARAMS(mkeefs, STACK_DEPTH_LOW);

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
 * @brief  Function calculate fletcher 16 checksum.
 *
 * @param  data         buffer
 * @param  bytes        buffer size
 *
 * @return Checksum.
 */
//==============================================================================
static uint16_t fletcher16(uint8_t const *data, size_t bytes)
{
        uint16_t sum1 = 0xff, sum2 = 0xff;
        size_t tlen;

        while (bytes) {
                tlen = ((bytes >= 20) ? 20 : bytes);
                bytes -= tlen;
                do {
                        sum2 += sum1 += *data++;
                        tlen--;
                } while (tlen);

                sum1 = (sum1 & 0xff) + (sum1 >> 8);
                sum2 = (sum2 & 0xff) + (sum2 >> 8);
        }

        /* Second reduction step to reduce sums to 8 bits */
        sum1 = (sum1 & 0xff) + (sum1 >> 8);
        sum2 = (sum2 & 0xff) + (sum2 >> 8);

        return (sum2 << 8) | sum1;
}

//==============================================================================
/**
 * Show help message.
 *
 * @param  pname        program name.
 */
//==============================================================================
static void show_help(char *pname)
{
        printf("Usage: %s <file>\n", pname);
        puts("  file\tfile to format (device)");
}

//==============================================================================
/**
 * Function write block by filling pages.
 *
 * @param block_pos     block position in memory.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
//==============================================================================
static int write_block(uint16_t block_pos)
{
        global->block.chsum.checksum = fletcher16(global->block.chsum.buf, sizeof(global->block.chsum.buf));

        global->block.chsum.checksum ^= block_pos;

        if (fseek(global->file, block_pos * cast(fpos_t, BLOCK_SIZE), SEEK_SET) != 0) {
                perror("Block seek");
                return EXIT_FAILURE;
        }

        if (fwrite(&global->block, 1, BLOCK_SIZE, global->file) != BLOCK_SIZE) {
                perror("Block write");
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}

//==============================================================================
/**
 * Function mark block as used in bitmap.
 *
 * @param  bitmap       bitmap array.
 * @param  block        block to allocate (relative to bitmap start).
 */
//==============================================================================
static void alloc_block(uint8_t bitmap[], uint32_t block)
{
        bitmap[block / 8] &= ~(1 << (block % 8));
}

//==============================================================================
/**
 * Function mark as used blocks of bitmap that are behind medium end.
 * Prevents allocation of not existing blocks.
 *
 * @param  bitmap       bitmap array.
 * @param  bits         number of bitmap bits.
 * @param  base         number of first block described by bitmap.
 * @param  total        total number of blocks.
 */
//==============================================================================
static void alloc_beyond_end(uint8_t bitmap[], uint32_t bits, uint32_t base, uint32_t total)
{
        for (uint32_t bit = 0; bit < bits; bit++) {
                if ((base + bit) >= total) {
                        alloc_block(bitmap, bit);
                }
        }
}

//==============================================================================
/**
 * Function format selected file according to given parameters.
 *
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
//==============================================================================
static int mkfs(void)
{
        // calculate file system parameters
        uint32_t total_blocks = cast(uint32_t, global->mem_size / BLOCK_SIZE);

        if (total_blocks > MAX_BLOCKS) {
                total_blocks = MAX_BLOCKS;
        }

        uint32_t extra_bmp_blks = 0;
        if (total_blocks > BMP_MAIN_BITS) {
                extra_bmp_blks = CEILING(total_blocks - BMP_MAIN_BITS, BMP_BLK_BITS);
        }

        uint16_t root_dir_block_addr = 1 + extra_bmp_blks;

        uint16_t used_blocks = 1 /*main*/ + extra_bmp_blks + 1 /*root dir*/;

        // create main block
        memset(&global->block, 0xFF, sizeof(global->block));

        global->block.main.magic = BLOCK_MAIN_MAGIC;
        global->block.main.blocks = total_blocks;
        global->block.main.bitmap_blocks = extra_bmp_blks;

        for (u16_t block = 0; block < used_blocks; block++) {
                alloc_block(global->block.main.bitmap, block);
        }

        alloc_beyond_end(global->block.main.bitmap, BMP_MAIN_BITS, 0, total_blocks);

        if (write_block(0) != EXIT_SUCCESS) {
                return EXIT_FAILURE;
        }

        // create extra bitmap blocks
        for (u16_t addr = 1; addr <= extra_bmp_blks; addr++) {
                memset(&global->block, 0xFF, sizeof(global->block));
                global->block.bitmap.magic = BLOCK_MAGIC_BITMAP;

                uint32_t base = BMP_MAIN_BITS + (addr - 1) * BMP_BLK_BITS;
                alloc_beyond_end(global->block.bitmap.map, BMP_BLK_BITS, base, total_blocks);

                if (write_block(addr) != EXIT_SUCCESS) {
                        return EXIT_FAILURE;
                }
        }

        // create root
        time_t t = time(NULL);

        memset(&global->block, 0xFF, sizeof(global->block));
        global->block.dir.magic = BLOCK_MAGIC_DIR;
        global->block.dir.ctime = t;
        global->block.dir.mtime = t;
        global->block.dir.gid = getgid();
        global->block.dir.uid = getuid();
        global->block.dir.mode = 0666;
        global->block.dir.parent = 0;
        global->block.dir.all_items = 0;
        global->block.dir.next = 0;

        return write_block(root_dir_block_addr);
}

//==============================================================================
/**
 * Main program function.
 *
 * @param argc      argument count
 * @param argv      arguments
 */
//==============================================================================
int main(int argc, char *argv[])
{
        if (argc < 2) {
                show_help(argv[0]);
                return EXIT_FAILURE;
        }

        int err = EXIT_FAILURE;

        global->file = fopen(argv[1], "r+");
        if (!global->file) {
                perror(argv[1]);
                return EXIT_FAILURE;
        }

        struct stat buf;
        memset(&buf, 0, sizeof(struct stat));

        if (fstat(fileno(global->file), &buf) != 0) {
                perror(argv[1]);

        } else {
                global->mem_size = buf.st_size;

                if (global->mem_size < (MIN_BLOCKS * BLOCK_SIZE)) {
                        printf("Too small disc for this file system (minimum %u bytes)!\n",
                                cast(uint, MIN_BLOCKS * BLOCK_SIZE));

                } else {
                        if (global->mem_size > (cast(size_t, MAX_BLOCKS) * BLOCK_SIZE)) {
                                printf("Disc is bigger than file system limit, only %u bytes are used.\n",
                                        cast(uint, MAX_BLOCKS * BLOCK_SIZE));
                        } else {
                                printf("Memory size set automatically to %u bytes.\n",
                                        cast(uint, global->mem_size));
                        }

                        err = mkfs();
                }
        }

        if (fclose(global->file) != 0) {
                perror(argv[1]);
                err = EXIT_FAILURE;
        }

        return err;
}

/*==============================================================================
  End of file
==============================================================================*/
