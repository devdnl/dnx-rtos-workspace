/*=========================================================================*//**
@file    mkext.c

@author  Daniel Zorychta

@brief   Create EXT2/3/5 file system.

@note    Copyright (C) 2022 Daniel Zorychta <daniel.zorychta@gmail.com>

         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation and modified by the dnx RTOS exception.

         NOTE: The modification  to the GPL is  included to allow you to
               distribute a combined work that includes dnx RTOS without
               being obliged to provide the source  code for proprietary
               components outside of the dnx RTOS.

         The dnx RTOS  is  distributed  in the hope  that  it will be useful,
         but WITHOUT  ANY  WARRANTY;  without  even  the implied  warranty of
         MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
         GNU General Public License for more details.

         Full license text is available on the following file: doc/license.txt.


*//*==========================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dnx/misc.h>
#include "ext4.h"
#include "ext4_mkfs.h"

/*==============================================================================
  Local symbolic constants/macros
==============================================================================*/
#define SECTOR_SIZE             512

/*==============================================================================
  Local types, enums definitions
==============================================================================*/

/*==============================================================================
  Local function prototypes
==============================================================================*/
static int bopen(struct ext4_blockdev *bdev);
static int bclose(struct ext4_blockdev *bdev);
static int bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt);
static int bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt);
static int bdev_lock(struct ext4_blockdev *bdev);
static int bdev_unlock(struct ext4_blockdev *bdev);

/*==============================================================================
  Local object definitions
==============================================================================*/
GLOBAL_VARIABLES_SECTION {
        struct ext4_blockdev_iface bdif;
        struct ext4_blockdev       bd;
        struct ext4_fs             fs;
        struct ext4_mkfs_info      info;
        FILE                      *dev;
        u8_t                       buf[SECTOR_SIZE];
};

/*==============================================================================
  Exported object definitions
==============================================================================*/
PROGRAM_PARAMS(mkext, STACK_DEPTH_MEDIUM);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief Program main function
 */
//==============================================================================
int main(int argc, char *argv[])
{
        const char *input_file = NULL;
        const char *label = "";
        int ext_version = 2;
        size_t block_size = 1024;
        bool journaling = false;

        if (argc < 2) {
                printf("Usage: %s [options]\n", argv[0]);
                printf(" -b     block size: 1024 (default), 2048, 4096\n");
                printf(" -e     ext version: 2 (default), 3, 4\n");
                printf(" -l     label\n");
                printf(" -j     enable journaling (ext3 and ext4)\n");
                printf(" -i     input file\n");
                return EXIT_FAILURE;
        }

        for (int i = 1; i < argc; i++) {
                if (isstreq(argv[i], "-b")) {
                        block_size = strtol(argv[++i], NULL, 0);

                } else if (isstreq(argv[i], "-e")) {
                        ext_version = strtol(argv[++i], NULL, 10);

                } else if (isstreq(argv[i], "-l")) {
                        label = argv[++i];

                } else if (isstreq(argv[i], "-i")) {
                        input_file = argv[++i];

                } else if (isstreq(argv[i], "-j")) {
                        journaling = true;

                } else {
                        printf("%s: unknown option '%s'\n", argv[0], argv[i]);
                        return EXIT_FAILURE;
                }
        }

        global->dev = fopen(input_file, "r+");
        if (not global->dev) {
                perror(input_file);
                return EXIT_FAILURE;
        }

        struct stat dev_stat;
        if (fstat(fileno(global->dev), &dev_stat) != 0) {
                fclose(global->dev);
                perror(input_file);
                return EXIT_FAILURE;
        }

        global->bdif.open     = bopen;
        global->bdif.bread    = bread;
        global->bdif.bwrite   = bwrite;
        global->bdif.close    = bclose;
        global->bdif.lock     = bdev_lock;
        global->bdif.unlock   = bdev_unlock;
        global->bdif.ph_bsize = SECTOR_SIZE;
        global->bdif.ph_bbuf  = global->buf;
        global->bdif.ph_bcnt  = dev_stat.st_size / SECTOR_SIZE;
        global->bdif.p_user   = NULL;

        global->bd.part_size   = dev_stat.st_size;
        global->bd.bdif        = &global->bdif;

        global->info.block_size = block_size;
        global->info.journal    = journaling;
        global->info.label      = label;

        int err = ext4_mkfs(&global->fs, &global->bd, &global->info, ext_version);
        if (not err) {
                memset(&global->info, 0, sizeof(global->info));
                err = ext4_mkfs_read_info(&global->bd, &global->info);
                if (not err) {
                        printf("Created filesystem with parameters:\n");
                        printf("Size: %llu\n", global->info.len);
                        printf("Block size: %lu\n", global->info.block_size);
                        printf("Blocks per group: %lu\n", global->info.blocks_per_group);
                        printf("Inodes per group: %lu\n", global->info.inodes_per_group);
                        printf("Inode size: %lu\n", global->info.inode_size);
                        printf("Inodes: %lu\n", global->info.inodes);
                        printf("Journal blocks: %lu\n", global->info.journal_blocks);
                        printf("Features ro_compat: 0x%x\n", global->info.feat_ro_compat);
                        printf("Features compat: 0x%x\n", global->info.feat_compat);
                        printf("Features incompat: 0x%x\n", global->info.feat_incompat);
                        printf("BG desc reserve: %lu\n", global->info.bg_desc_reserve_blocks);
                        printf("Descriptor size: %lu\n",global->info.dsc_size);
                        printf("Label: %s\n", global->info.label);

                } else {
                        printf("%s: error %d\n", argv[0], err);
                }
        } else {
                printf("%s: error %d\n", argv[0], err);
        }

        fclose(global->dev);

        return EXIT_SUCCESS;
}

//==============================================================================
/**
 * @brief  Function open block device. Not used, block opened at FS init.
 *
 * @param  bdev         block device.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bopen(struct ext4_blockdev *bdev)
{
        UNUSED_ARG1(bdev);
        return ESUCC;
}

//==============================================================================
/**
 * @brief  Function close block device. Not used, block closed at FS release.
 *
 * @param  bdev         block device.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bclose(struct ext4_blockdev *bdev)
{
        UNUSED_ARG1(bdev);
        return ESUCC;
}

//==============================================================================
/**
 * @brief  Function read data from selected block (by cache).
 *
 * @param  bdev         block device.
 * @param  buf          buffer to read.
 * @param  blk_id       block ID (start block).
 * @param  blk_cnt      number of blocks to read.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
        fseek(global->dev, blk_id * bdev->bdif->ph_bsize, SEEK_SET);
        return fread(buf, bdev->bdif->ph_bsize, blk_cnt, global->dev) == blk_cnt ? EOK : errno;
}

//==============================================================================
/**
 * @brief  Function write data to selected block (by cache).
 *
 * @param  bdev         block device.
 * @param  buf          buffer to write.
 * @param  blk_id       block ID (start block).
 * @param  blk_cnt      number of blocks to read.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt)
{
        fseek(global->dev, blk_id * bdev->bdif->ph_bsize, SEEK_SET);
        return fwrite(buf, bdev->bdif->ph_bsize, blk_cnt, global->dev) == blk_cnt ? EOK : errno;
}

//==============================================================================
/**
 * @brief  Function lock access to disc.
 *
 * @param  bdev         block device.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bdev_lock(struct ext4_blockdev *bdev)
{
        UNUSED_ARG1(bdev);
        return 0;
}

//==============================================================================
/**
 * @brief  Function unlock access to disc.
 *
 * @param  bdev         block device.
 *
 * @return One of errno value (errno.h).
 */
//==============================================================================
static int bdev_unlock(struct ext4_blockdev *bdev)
{
        UNUSED_ARG1(bdev);
        return 0;
}

/*==============================================================================
  End of file
==============================================================================*/
