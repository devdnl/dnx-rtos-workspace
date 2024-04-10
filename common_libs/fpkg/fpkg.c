/*==============================================================================
File    fpkg.c

Author  Daniel Zorychta

Brief   File package container.

        Copyright (C) 2023 Daniel Zorychta <daniel.zorychta@gmail.com>

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

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include "fpkg.h"
#include <dnx/misc.h>
#include <stdlib.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define ALLOC_BUF_SIZE         8192

#define FPKG_CRC32_INIT        0xFFFFFFFF

#define FILE_DESCRIPTOR_MAGIC  0x43534446
#define FILE_ENTRY_MAGIC       0x544E4546
#define CONTAINER_HEADER_MAGIC 0x474B5046

#define FLAG_ENCRYPTED         (1 << 0)

#define is_encrypted(self)     (self->flags & FLAG_ENCRYPTED)

/*==============================================================================
  Local object types
==============================================================================*/
typedef struct {
        uint32_t magic;
        uint32_t crc32;
        uint32_t file_name_len;
        uint32_t file_name_crc32;
        uint32_t file_size;
        uint32_t file_crc32;
        uint32_t content_prng_seed;
        uint32_t flags;
        uint8_t  file_name_file_data[];
} file_descriptor_t;

typedef struct {
        uint32_t magic;
        uint32_t crc32;
        uint32_t file_descriptor_address;
        uint32_t reserved;
} file_entry_t;

typedef struct {
        uint32_t magic;
        uint32_t crc32;
        uint32_t prng_seed;
        uint32_t number_of_files;
        uint32_t flags;
        uint32_t file_crc32;
} container_header_t;

struct fpkg {
        FILE    *file;
        uint32_t prng_seed;
        uint32_t number_of_files;
        uint32_t flags;
        char     tmp[256];
};

struct fpkg_file {
        fpkg_t  *self;
        uint32_t size;
        uint32_t seek;
        uint32_t content_prng_seed;
        uint32_t current_seed;
        uint32_t content_offset;
};

struct fpkg_dir {
        fpkg_t  *self;
        char    *dir_root;
        char    *item_name;
        uint32_t dir_root_len;
        uint32_t seek;
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static uint32_t prng(uint32_t *seed);

static void xcrypt(void *buf, uint32_t buflen, uint32_t *seed);

static bool is_file_valid(fpkg_t *self, bool full_check);

static bool is_container_header_valid(const container_header_t *hdr);

static bool is_file_entry_valid(const file_entry_t *file_entry);

static bool is_file_descriptor_valid(const file_descriptor_t *file_desc);

static bool is_file_descriptor_name_valid(const file_descriptor_t *file_desc,
        const char                                                *name);

static int read_container_header(fpkg_t *self, container_header_t *hdr);

static int read_file_entry(fpkg_t *self, uint16_t n, file_entry_t *entry);

static int read_file_descriptor(fpkg_t *self, file_entry_t *entry,
        file_descriptor_t *desc);

static int read_file_descriptor_name(fpkg_t *self, file_entry_t *entry,
        file_descriptor_t *desc, char *buf, size_t buf_len);

static uint32_t calc_CRC32(const void *buf, uint32_t buflen, uint32_t init);

/*==============================================================================
  Local objects
==============================================================================*/

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
 * @brief  Open container.
 *
 * @param  path         container path
 *
 * @return Container instance.
 */
//==============================================================================
fpkg_t *fpkg__open(const char *path, bool validate)
{
        if (not path) {
                return NULL;
        }

        fpkg_t *self = calloc(1, sizeof(*self));
        if (not self) {
                return NULL;
        }

        self->file = fopen(path, "r");
        if (not self->file) {
                return fpkg__close(self);
        }

        if (validate) {
                if (not is_file_valid(self, true)) {
                        return fpkg__close(self);
                }
        }

        container_header_t header;
        if (read_container_header(self, &header) != 0) {
                return fpkg__close(self);
        }

        self->prng_seed       = header.prng_seed;
        self->number_of_files = header.number_of_files;
        self->flags           = header.flags;

        return self;
}

//==============================================================================
/**
 * @brief  Close container.
 *
 * @param  self         container instance
 */
//==============================================================================
fpkg_t *fpkg__close(fpkg_t *self)
{
        if (not self) {
                return NULL;
        }

        fclose(self->file);

        free(self);

        return NULL;
}

//==============================================================================
/**
 * @brief  Return number of files in container.
 *
 * @param  self                 container instance
 *
 * @return Number of files or -1 on error.
 */
//==============================================================================
int fpkg__get_number_of_files(fpkg_t *self)
{
        int err = -1;

        if (not self) {
                return err;
        }

        return self->number_of_files;
}

//==============================================================================
/**
 * @brief  Open directory.
 *
 * @param  self         container instance
 * @param  dirname      dirname
 *
 * @return Dir representation object.
 */
//==============================================================================
fpkg_dir_t *fpkg__diropen(fpkg_t *self, const char *dirname)
{
        if (not self or not dirname) {
                return NULL;
        }

        fpkg_dir_t *dir = calloc(1, sizeof(*dir));
        if (not dir) {
                return NULL;
        }

        size_t dirname_len = strlen(dirname);

        dir->dir_root = strndup(dirname, dirname_len + 2);
        if (not dir->dir_root) {
                goto error;
        }

        dir->item_name = strdup("");
        if (not dir->item_name) {
                goto error;
        }

        strcpy(dir->dir_root, dirname);

        if (dirname[dirname_len - 1] != '/') {
                strcat(dir->dir_root, "/");
        }

        dir->dir_root_len = strlen(dir->dir_root);
        dir->self         = self;

        for (size_t i = 0; i < self->number_of_files; i++) {

                file_entry_t entry;
                if (read_file_entry(self, i, &entry) != 0) {
                        goto error;
                }

                file_descriptor_t desc;
                if (read_file_descriptor(self, &entry, &desc) != 0) {
                        goto error;
                }

                if (read_file_descriptor_name(self, &entry, &desc, self->tmp,
                            sizeof(self->tmp)) != cast(int, desc.file_name_len)) {
                        goto error;
                }

                if (strncmp(self->tmp, dir->dir_root, dir->dir_root_len) == 0) {
                        return dir;
                }
        }

error:
        return fpkg__dirclose(dir);
}

//==============================================================================
/**
 * @brief  Close directory.
 *
 * @param  dir          Dir representation object.
 *
 * @return NULL.
 */
//==============================================================================
fpkg_dir_t *fpkg__dirclose(fpkg_dir_t *dir)
{
        if (not dir or not dir->self) {
                return NULL;
        }

        free(dir->dir_root);
        free(dir->item_name);
        free(dir);

        return NULL;
}

//==============================================================================
/**
 * @brief  Read container entries.
 *
 * @param  dir          Dir representation object.
 * @param  entry        entry
 *
 * @return On success 0 is returned.
 */
//==============================================================================
int fpkg__readdir(fpkg_dir_t *dir, fpkg_dirent_t *dirent)
{
        if (not dir or not dir->self or not dirent) {
                return -1;
        }

        while (dir->seek < dir->self->number_of_files) {

                file_entry_t entry;
                if (read_file_entry(dir->self, dir->seek, &entry) != 0) {
                        return -2;
                }

                file_descriptor_t desc;
                if (read_file_descriptor(dir->self, &entry, &desc) != 0) {
                        return -3;
                }

                if (read_file_descriptor_name(dir->self, &entry, &desc, dir->self->tmp,
                            sizeof(dir->self->tmp)) != cast(int, desc.file_name_len)) {
                        return -4;
                }

                if (strncmp(dir->self->tmp, dir->dir_root, strlen(dir->dir_root)) == 0) {

                        char *item_name = dir->self->tmp + dir->dir_root_len;

                        char *slash = strchr(item_name, '/');
                        if (slash) {
                                *slash       = '\0';
                                dirent->dir  = true;
                                dirent->size = 0;
                        } else {
                                dirent->dir  = false;
                                dirent->size = desc.file_size;
                        }

                        if (strcmp(dir->item_name, item_name) != 0) {
                                free(dir->item_name);
                                dir->item_name = strdup(item_name);
                                dirent->name   = dir->item_name;
                                return 0;
                        }
                }

                dir->seek++;
        }

        return -5;
}

//==============================================================================
/**
 * @brief  Open file.
 *
 * @param  self         container instance
 * @param  filename     filename (full path)
 *
 * @return File representation object.
 */
//==============================================================================
fpkg_file_t *fpkg__fopen(fpkg_t *self, const char *filename)
{
        if (not self or not filename) {
                return NULL;
        }

        for (size_t i = 0; i < self->number_of_files; i++) {

                file_entry_t entry;
                if (read_file_entry(self, i, &entry) != 0) {
                        return NULL;
                }

                file_descriptor_t desc;
                if (read_file_descriptor(self, &entry, &desc) != 0) {
                        return NULL;
                }

                if (read_file_descriptor_name(self, &entry, &desc, self->tmp,
                            sizeof(self->tmp)) != cast(int, desc.file_name_len)) {
                        return NULL;
                }

                if (strcmp(self->tmp, filename) == 0) {

                        fpkg_file_t *file = malloc(sizeof(*file));
                        if (file) {
                                file->content_offset = entry.file_descriptor_address +
                                        sizeof(desc) + desc.file_name_len;

                                file->content_prng_seed = desc.content_prng_seed;
                                file->current_seed      = desc.content_prng_seed;
                                file->size              = desc.file_size;
                                file->seek              = 0;
                                file->self              = self;

                                return file;
                        }
                }
        }

        return NULL;
}

//==============================================================================
/**
 * @brief  Close file.
 *
 * @param  file         File representation object.
 *
 * @return NULL.
 */
//==============================================================================
fpkg_file_t *fpkg__fclose(fpkg_file_t *file)
{
        if (not file or not file->self) {
                return NULL;
        }

        file->self = NULL;
        free(file);

        return NULL;
}

//==============================================================================
/**
 * @brief  Set file position.
 *
 * @param  file         File representation object.
 * @param  seek         position
 * @param  mode         mode
 *
 * @return On succes 0 is returned.
 */
//==============================================================================
int fpkg__fseek(fpkg_file_t *file, int32_t seek, fpkg_seek_mode_t mode)
{
        if (not file or not file->self) {
                return -1;
        }

        uint32_t old_seek = file->seek;

        switch (mode) {
        case fpkg_seek_mode__set:
                file->seek = seek;
                break;

        case fpkg_seek_mode__cur:
                file->seek += seek;
                break;

        case fpkg_seek_mode__end:
                file->seek = file->size + seek;
                break;

        default:
                break;
        }

        if (seek > cast(typeof(seek), file->size)) {
                file->seek = file->size;
        }

        if (is_encrypted(file->self)) {
                size_t n = 0;

                if (file->seek >= old_seek) {
                        n = file->seek - old_seek;

                } else {
                        file->current_seed = file->content_prng_seed;
                        n                  = file->seek;
                }

                while (n > 0) {
                        prng(&file->current_seed);
                        n--;
                }
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Read file.
 *
 * @param  file         File representation object.
 * @param  dst          destination
 * @param  count        number of bytes
 *
 * @return Number of read bytes.
 */
//==============================================================================
size_t fpkg__fread(fpkg_file_t *file, uint8_t *dst, uint32_t count)
{
        if (not file or not file->self or not dst or not count) {
                return 0;
        }

        size_t n = 0;

        if (fseek(file->self->file, file->content_offset + file->seek, SEEK_SET) != 0) {
                return n;
        }

        size_t len = min((file->size - file->seek), count);
        n          = fread(dst, 1, len, file->self->file);
        file->seek += n;

        if (n > 0 and is_encrypted(file->self)) {
                xcrypt(dst, len, &file->current_seed);
        }

        return n;
}

//==============================================================================
/**
 * @brief  File statistics.
 *
 * @param  self         container instance
 * @param  filename     file path
 * @param  entry        file info
 *
 * @return On succes 0 is returned.
 */
//==============================================================================
int fpkg__stat(fpkg_t *self, const char *filename, fpkg_stat_t *stat)
{
        int err = -1;

        fpkg_file_t *file = fpkg__fopen(self, filename);
        if (file) {
                err = fpkg__fstat(file, stat);
        }

        return err;
}

//==============================================================================
/**
 * @brief  File statistics.
 *
 * @param  file         File representation object.
 * @param  entry        file info
 *
 * @return On succes 0 is returned.
 */
//==============================================================================
int fpkg__fstat(fpkg_file_t *file, fpkg_stat_t *stat)
{
        if (not file or not file->self or not stat) {
                return -1;
        }

        stat->size = file->size;

        return 0;
}

//==============================================================================
/**
 * @brief  Check that file is consistent.
 *
 * @param  self         container instance
 *
 * @return True if file is valid, otherwise false.
 */
//==============================================================================
bool fpkg__is_valid(fpkg_t *self)
{
        bool valid = false;

        if (self) {
                valid = is_file_valid(self, true);
        }

        return valid;
}

//==============================================================================
/**
 * @brief  Calculate CRC32.
 *
 * @param  buf          buffer
 * @param  buflen       buffer length
 * @param  init         initial value
 *
 * @return CRC32 result.
 */
//==============================================================================
static uint32_t calc_CRC32(const void *buf, uint32_t buflen, uint32_t init)
{
        const uint8_t *b = buf;

        uint32_t crc = init;

        while (buflen--) {

                crc = crc ^ *b++;

                for (int shift = 7; shift >= 0; shift--) {
                        uint32_t mask = -(crc & 1);
                        crc           = (crc >> 1) ^ (0xEDB88320 & mask);
                }
        }

        return ~crc;
}

//==============================================================================
/**
 * @brief Function returns a pseudo-random integer.
 *
 * @param  seed         seed value
 *
 * @return The function return a pseudo-random value.
 */
//==============================================================================
static uint32_t prng(uint32_t *seed)
{
        const uint64_t a = 1103515245;
        const uint32_t c = 12345;
        const uint64_t m = (1ULL << 32);

        *seed = (a * *seed + c) % m;

        return *seed;
}

//==============================================================================
/**
 * @brief  Function encode/decode selected block.
 *
 * @param  buf          buffer
 * @param  buflen       buffer length
 * @param  seed         seed value
 */
//==============================================================================
static void xcrypt(void *buf, uint32_t buflen, uint32_t *seed)
{
        uint8_t *blk = buf;

        while (buflen > 0) {
                *blk = *blk ^ prng(seed);
                blk++;
                buflen--;
        }
}

//==============================================================================
/**
 * @brief  Read container header.
 *
 * @param  self         container instance
 * @param  hdr          header destination buffer
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int read_container_header(fpkg_t *self, container_header_t *hdr)
{
        int err = -1;

        if (fseek(self->file, -sizeof(container_header_t), SEEK_END) == 0) {

                if ((fread(hdr, sizeof(*hdr), 1, self->file) == 1) and
                        (!ferror(self->file))) {

                        if (hdr->prng_seed != 0) {
                                uint32_t seed = hdr->prng_seed + 1;
                                hdr->number_of_files ^= prng(&seed);

                                seed = hdr->prng_seed + 2;
                                hdr->flags ^= prng(&seed);
                        }

                        err = 0;
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Read file entry.
 *
 * @param  self         container instance
 * @param  n            n-th entry
 * @param  entry        file entry buffer
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int read_file_entry(fpkg_t *self, uint16_t n, file_entry_t *entry)
{
        int err = -1;

        int32_t entry_address = sizeof(container_header_t) +
                (sizeof(file_entry_t) * self->number_of_files) -
                (sizeof(file_entry_t) * n);

        if (fseek(self->file, -entry_address, SEEK_END) == 0) {

                clearerr(self->file);

                if ((fread(entry, sizeof(*entry), 1, self->file) == 1) and
                        (!ferror(self->file))) {

                        if (is_encrypted(self)) {
                                uint32_t seed = self->prng_seed + n;
                                xcrypt(entry, sizeof(file_entry_t), &seed);
                        }

                        err = 0;
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Read file descriptor.
 *
 * @param  self         container instance
 * @param  entry        file entry
 * @param  desc         descriptor buffer
 *
 * @return On success 0 is returned.
 */
//==============================================================================
static int read_file_descriptor(fpkg_t *self, file_entry_t *entry,
        file_descriptor_t *desc)
{
        int err = -1;

        if (fseek(self->file, entry->file_descriptor_address, SEEK_SET) == 0) {

                if ((fread(desc, sizeof(*desc), 1, self->file) == 1) and
                        (!ferror(self->file))) {

                        if (is_encrypted(self)) {
                                uint32_t seed = self->prng_seed;
                                xcrypt(desc, sizeof(file_descriptor_t), &seed);
                        }

                        err = 0;
                }
        }

        return err;
}

//==============================================================================
/**
 * @brief  Read file descriptor.
 *
 * @param  self         container instance
 * @param  entry        file entry
 * @param  desc         descriptor buffer
 *
 * @return File name length.
 */
//==============================================================================
static int read_file_descriptor_name(fpkg_t *self, file_entry_t *entry,
        file_descriptor_t *desc, char *buf, size_t buf_len)
{
        size_t name_len = 0;

        if (fseek(self->file, entry->file_descriptor_address + sizeof(file_descriptor_t),
                    SEEK_SET) == 0) {

                name_len = min(desc->file_name_len, buf_len - 1);

                if ((fread(buf, 1, name_len, self->file) == name_len) and
                        (!ferror(self->file))) {

                        if (is_encrypted(self)) {
                                uint32_t seed = desc->content_prng_seed;
                                xcrypt(buf, name_len, &seed);
                        }

                        buf[name_len] = '\0';
                }
        }

        return name_len;
}

//==============================================================================
/**
 * @brief  Function check if file is valid.
 *
 * @param  self         container instance
 * @param  full_check   full file check
 *
 * @return True if file is valid, false if not.
 */
//==============================================================================
static bool is_file_valid(fpkg_t *self, bool full_check)
{
        bool valid = false;

        uint32_t buflen = ALLOC_BUF_SIZE;

        uint8_t *buf = malloc(buflen);
        if (!buf) {
                return false;
        }

        if (full_check) {
                uint32_t size = 0;
                uint32_t crc  = ~FPKG_CRC32_INIT;

                if (fseek(self->file, 0, SEEK_END) == 0) {
                        size = ftell(self->file) - sizeof(uint32_t);
                }

                if (fseek(self->file, 0, SEEK_SET) != 0) {
                        size = 0;
                }

                while (size > 0) {
                        size_t len = min(size, buflen);

                        if (fread(buf, 1, len, self->file) == len) {
                                crc = calc_CRC32(buf, len, ~crc);
                        } else {
                                break;
                        }

                        size -= len;
                }

                uint32_t file_crc;
                if (fread(&file_crc, sizeof(file_crc), 1, self->file) == 1) {

                        valid = (file_crc == crc);
                }
        }

        if (valid) {
                container_header_t header;
                if (read_container_header(self, &header) != 0) {
                        return false;
                }

                if (not is_container_header_valid(&header)) {
                        return false;
                }

                self->prng_seed       = header.prng_seed;
                self->number_of_files = header.number_of_files;
                self->flags           = header.flags;

                for (uint16_t i = 0; i < header.number_of_files; i++) {

                        file_entry_t file_entry;
                        if (read_file_entry(self, i, &file_entry) != 0) {
                                valid = false;
                                break;
                        }

                        if (not is_file_entry_valid(&file_entry)) {
                                valid = false;
                                break;
                        }

                        file_descriptor_t file_desc;
                        if (read_file_descriptor(self, &file_entry, &file_desc) != 0) {
                                valid = false;
                                break;
                        }

                        if (not is_file_descriptor_valid(&file_desc)) {
                                valid = false;
                                break;
                        }

                        char *buf_name = cast(char *, buf);
                        if (read_file_descriptor_name(self, &file_entry, &file_desc,
                                    buf_name, buflen) != cast(int, file_desc.file_name_len)) {
                                valid = false;
                                break;
                        }

                        if (not is_file_descriptor_name_valid(&file_desc, buf_name)) {
                                valid = false;
                                break;
                        }
                }
        }

        free(buf);

        return valid;
}

//==============================================================================
/**
 * @brief  Function check if container header is coherent.
 *
 * @param  header               header to check
 *
 * @return True if valid, otherwise false.
 */
//==============================================================================
static bool is_container_header_valid(const container_header_t *hdr)
{
        bool valid = false;

        uint32_t crc = calc_CRC32(&hdr->prng_seed,
                sizeof(*hdr) - offsetof(container_header_t, prng_seed) -
                        sizeof(hdr->file_crc32),
                FPKG_CRC32_INIT);

        if ((hdr->magic == CONTAINER_HEADER_MAGIC) && (hdr->crc32 == crc)) {

                if (hdr->prng_seed != 0) {
                        valid = (hdr->flags & FLAG_ENCRYPTED);
                } else {
                        valid = true;
                }
        }

        return valid;
}

//==============================================================================
/**
 * @brief  Check file entry coherency.
 *
 * @param  file_entry           file entry
 *
 * @return True if valid, otherwise false.
 */
//==============================================================================
static bool is_file_entry_valid(const file_entry_t *file_entry)
{
        uint32_t crc = calc_CRC32(&file_entry->file_descriptor_address,
                sizeof(*file_entry) - offsetof(file_entry_t, file_descriptor_address),
                FPKG_CRC32_INIT);

        return ((file_entry->magic == FILE_ENTRY_MAGIC) and (file_entry->crc32 == crc) and
                (file_entry->file_descriptor_address < cast(uintptr_t, file_entry)));
}

//==============================================================================
/**
 * @brief  Check file descriptor coherency.
 *
 * @param  file_desc_address    file descriptor address
 * @param  buf                  operational buffer for file CRC32 calculation (can be
 * NULL)
 * @param  buflen               buffer length
 *
 * @return True if valid, otherwise false.
 */
//==============================================================================
static bool is_file_descriptor_valid(const file_descriptor_t *file_desc)
{
        uint32_t crc = calc_CRC32(&file_desc->file_name_len,
                sizeof(*file_desc) - offsetof(file_descriptor_t, file_name_len),
                FPKG_CRC32_INIT);

        return ((file_desc->magic == FILE_DESCRIPTOR_MAGIC) and
                (file_desc->crc32 == crc));
}

//==============================================================================
/**
 * @brief  Check file descriptor coherency.
 *
 * @param  file_desc_address    file descriptor address
 * @param  buf                  operational buffer for file CRC32 calculation (can be
 * NULL)
 * @param  buflen               buffer length
 *
 * @return True if valid, otherwise false.
 */
//==============================================================================
static bool is_file_descriptor_name_valid(const file_descriptor_t *file_desc,
        const char                                                *name)
{
        uint32_t crc = calc_CRC32(name, file_desc->file_name_len, FPKG_CRC32_INIT);

        return file_desc->file_name_crc32 == crc;
}

/*==============================================================================
  End of file
==============================================================================*/
