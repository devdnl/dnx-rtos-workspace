/*==============================================================================
File    fpkg.c

Author  Daniel Zorychta

Brief   File package container.

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
  FPKG container layout (all values little endian):

        0x00            "FPKG\n" + padding to 64 bytes
        0x40            u32 total file size
        0x44            file_descriptor_t + name (no NUL) + content  (4 B aligned)
        ...             next descriptors
        entries_offset  file_entry_t[number_of_files]                (4 B aligned)
        EOF-24          container_header_t (last field = CRC32 of entire file)

  Names are absolute paths ("/dir/file"). When FLAG_SORTED is set, entries are
  sorted by name (byte order, as strcmp()), which allows binary search and
  early termination of directory listing. Images without the flag (created by
  older mkfpkg.py) are handled by linear search.
==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include "fpkg.h"
#include <dnx/misc.h>
#include <iso646.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define ALLOC_BUF_SIZE         8192

#define FPKG_CRC32_INIT        0xFFFFFFFF

#define FILE_DESCRIPTOR_MAGIC  0x43534446
#define FILE_ENTRY_MAGIC       0x544E4546
#define CONTAINER_HEADER_MAGIC 0x474B5046

#define FLAG_ENCRYPTED         (1 << 0)
#define FLAG_SORTED            (1 << 1)

#define is_encrypted(self)     ((self)->flags & FLAG_ENCRYPTED)
#define is_sorted(self)        ((self)->flags & FLAG_SORTED)

#define NAME_BUF_LEN           512
#define MAX_PATH_LEN           (NAME_BUF_LEN - 2)      /* place for '/' and '\0' */

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
        /* followed by name (file_name_len bytes) and content (file_size bytes) */
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

typedef struct {
        uint32_t index;
        file_entry_t entry;
        file_descriptor_t desc;
        bool is_dir;
} item_t;

struct fpkg {
        FILE *file;
        uint32_t file_size;
        uint32_t entries_offset;
        uint32_t prng_seed;
        uint32_t number_of_files;
        uint32_t flags;
        char tmp[NAME_BUF_LEN];
};

struct fpkg_file {
        fpkg_t *self;
        uint32_t size;
        uint32_t seek;
        uint32_t content_prng_seed;
        uint32_t current_seed;
        uint32_t content_offset;
};

struct fpkg_dir {
        fpkg_t *self;
        char *root;             /* directory path with trailing '/' */
        char *item_name;        /* last returned item */
        uint32_t root_len;
        uint32_t start;         /* first entry that can belong to directory */
        uint32_t seek;
};

/*==============================================================================
  Local function prototypes
==============================================================================*/
static uint32_t prng(uint32_t *seed);
static void xcrypt(void *buf, uint32_t buflen, uint32_t *seed);
static uint32_t calc_CRC32(const void *buf, uint32_t buflen, uint32_t init);
static char *str_dup(const char *str);
static int read_at(fpkg_t *self, uint32_t offset, void *buf, size_t len);
static int read_image_info(fpkg_t *self);
static int read_container_header(fpkg_t *self, container_header_t *hdr);
static int read_file_entry(fpkg_t *self, uint32_t n, file_entry_t *entry);
static int read_file_descriptor(fpkg_t *self, const file_entry_t *entry, file_descriptor_t *desc);
static int read_file_descriptor_name(fpkg_t *self, const file_entry_t *entry, const file_descriptor_t *desc,
        char *buf, size_t buf_len, bool *truncated);
static int read_item(fpkg_t *self, uint32_t n, file_entry_t *entry, file_descriptor_t *desc, bool *truncated);
static int normalize_path(const char *path, size_t *key_len);
static int lookup(fpkg_t *self, const char *key, size_t key_len, item_t *item);
static int dir_next(fpkg_dir_t *dir, fpkg_dirent_t *dirent);
static bool is_file_valid(fpkg_t *self);
static bool is_container_header_valid(const container_header_t *hdr);
static bool is_file_entry_valid(const fpkg_t *self, const file_entry_t *file_entry);
static bool is_file_descriptor_valid(const fpkg_t *self, const file_entry_t *file_entry,
        const file_descriptor_t *file_desc);

/*==============================================================================
  Function definitions
==============================================================================*/
//==============================================================================
/**
 * @brief  Open container. Container header is always checked.
 *
 * @param  path         container path
 * @param  validate     full check (CRC of image, entries, names, content, order)
 *
 * @return Container instance or NULL.
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

        self->file = fopen(path, "rb");
        if (not self->file) {
                return fpkg__close(self);
        }

        if (read_image_info(self) != 0) {
                return fpkg__close(self);
        }

        if (validate and not is_file_valid(self)) {
                return fpkg__close(self);
        }

        return self;
}

//==============================================================================
/**
 * @brief  Close container.
 *
 * @param  self         container instance
 *
 * @return NULL.
 */
//==============================================================================
fpkg_t *fpkg__close(fpkg_t *self)
{
        if (not self) {
                return NULL;
        }

        if (self->file) {
                fclose(self->file);
        }

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
        if (not self) {
                return -1;
        }

        return cast(int, self->number_of_files);
}

//==============================================================================
/**
 * @brief  Check that container is consistent (full check).
 *
 * @param  self         container instance
 *
 * @return True if file is valid, otherwise false.
 */
//==============================================================================
bool fpkg__is_valid(fpkg_t *self)
{
        return self and is_file_valid(self);
}

//==============================================================================
/**
 * @brief  Open directory.
 *
 * @param  self         container instance
 * @param  dirname      directory path ("/" is root)
 *
 * @return Dir representation object or NULL (not exists, not a directory).
 */
//==============================================================================
fpkg_dir_t *fpkg__diropen(fpkg_t *self, const char *dirname)
{
        if (not self or not dirname) {
                return NULL;
        }

        size_t key_len;
        if (normalize_path(dirname, &key_len) != 0) {
                return NULL;
        }

        item_t item;
        if ((lookup(self, dirname, key_len, &item) != 0) or not item.is_dir) {
                return NULL;
        }

        fpkg_dir_t *dir = calloc(1, sizeof(*dir));
        if (not dir) {
                return NULL;
        }

        dir->self = self;
        dir->root = malloc(key_len + 2);
        dir->item_name = str_dup("");

        if (not dir->root or not dir->item_name) {
                return fpkg__dirclose(dir);
        }

        memcpy(dir->root, dirname, key_len);
        dir->root[key_len] = '/';
        dir->root[key_len + 1] = '\0';
        dir->root_len = key_len + 1;
        dir->start = item.index;
        dir->seek = item.index;

        return dir;
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
        if (not dir) {
                return NULL;
        }

        free(dir->root);
        free(dir->item_name);
        free(dir);

        return NULL;
}

//==============================================================================
/**
 * @brief  Read next directory item.
 *
 * @param  dir          Dir representation object.
 * @param  dirent       item
 *
 * @return FPKG_OK, FPKG_END_OF_DIR or one of FPKG_ERR_*.
 */
//==============================================================================
int fpkg__readdir(fpkg_dir_t *dir, fpkg_dirent_t *dirent)
{
        if (not dir or not dir->self or not dirent) {
                return FPKG_ERR_ARG;
        }

        return dir_next(dir, dirent);
}

//==============================================================================
/**
 * @brief  Start directory listing from beginning.
 *
 * @param  dir          Dir representation object.
 */
//==============================================================================
void fpkg__rewinddir(fpkg_dir_t *dir)
{
        if (dir) {
                dir->seek = dir->start;
                dir->item_name[0] = '\0';
        }
}

//==============================================================================
/**
 * @brief  Open file.
 *
 * @param  self         container instance
 * @param  filename     filename (full path)
 *
 * @return File representation object or NULL (not exists, directory).
 */
//==============================================================================
fpkg_file_t *fpkg__fopen(fpkg_t *self, const char *filename)
{
        if (not self or not filename) {
                return NULL;
        }

        size_t key_len;
        if (normalize_path(filename, &key_len) != 0) {
                return NULL;
        }

        item_t item;
        if ((lookup(self, filename, key_len, &item) != 0) or item.is_dir) {
                return NULL;
        }

        fpkg_file_t *file = malloc(sizeof(*file));
        if (file) {
                file->content_offset = item.entry.file_descriptor_address + sizeof(file_descriptor_t)
                                     + item.desc.file_name_len;
                file->content_prng_seed = item.desc.content_prng_seed;
                file->current_seed = item.desc.content_prng_seed;
                file->size = item.desc.file_size;
                file->seek = 0;
                file->self = self;
        }

        return file;
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
        free(file);

        return NULL;
}

//==============================================================================
/**
 * @brief  Set file position. Position beyond end of file is limited to file
 *         size, negative position is an error (position is not changed).
 *
 * Keystream byte depends only on 8 LSB of LCG state (modulus 2^32), and these
 * bits have period 256, so seed is advanced by (distance mod 256) steps.
 *
 * @param  file         File representation object.
 * @param  seek         offset
 * @param  mode         mode
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
int fpkg__fseek(fpkg_file_t *file, int32_t seek, fpkg_seek_mode_t mode)
{
        if (not file or not file->self) {
                return -1;
        }

        int64_t pos;

        switch (mode) {
        case fpkg_seek_mode__set: pos = seek; break;
        case fpkg_seek_mode__cur: pos = cast(int64_t, file->seek) + seek; break;
        case fpkg_seek_mode__end: pos = cast(int64_t, file->size) + seek; break;
        default: return -1;
        }

        if (pos < 0) {
                return -1;
        }

        uint32_t new_seek = (pos > file->size) ? file->size : cast(uint32_t, pos);

        if (is_encrypted(file->self)) {
                uint32_t n;

                if (new_seek >= file->seek) {
                        n = new_seek - file->seek;
                } else {
                        file->current_seed = file->content_prng_seed;
                        n = new_seek;
                }

                for (n &= 0xFF; n > 0; n--) {
                        prng(&file->current_seed);
                }
        }

        file->seek = new_seek;

        return 0;
}

//==============================================================================
/**
 * @brief  Return file position.
 *
 * @param  file         File representation object.
 *
 * @return Position or -1 on error.
 */
//==============================================================================
int32_t fpkg__ftell(fpkg_file_t *file)
{
        if (not file) {
                return -1;
        }

        return cast(int32_t, file->seek);
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
size_t fpkg__fread(fpkg_file_t *file, void *dst, uint32_t count)
{
        if (not file or not file->self or not dst or not count) {
                return 0;
        }

        uint32_t len = min(file->size - file->seek, count);
        if (len == 0) {
                return 0;
        }

        FILE *f = file->self->file;

        if (fseek(f, cast(long, file->content_offset + file->seek), SEEK_SET) != 0) {
                return 0;
        }

        size_t n = fread(dst, 1, len, f);

        if (is_encrypted(file->self)) {
                xcrypt(dst, n, &file->current_seed);
        }

        file->seek += n;

        return n;
}

//==============================================================================
/**
 * @brief  File or directory statistics.
 *
 * @param  self         container instance
 * @param  filename     path
 * @param  stat         file info
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
int fpkg__stat(fpkg_t *self, const char *filename, fpkg_stat_t *stat)
{
        if (not self or not filename or not stat) {
                return -1;
        }

        size_t key_len;
        if (normalize_path(filename, &key_len) != 0) {
                return -1;
        }

        item_t item;
        if (lookup(self, filename, key_len, &item) != 0) {
                return -1;
        }

        stat->dir = item.is_dir;
        stat->size = item.is_dir ? 0 : item.desc.file_size;

        return 0;
}

//==============================================================================
/**
 * @brief  File statistics.
 *
 * @param  file         File representation object.
 * @param  stat         file info
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
int fpkg__fstat(fpkg_file_t *file, fpkg_stat_t *stat)
{
        if (not file or not file->self or not stat) {
                return -1;
        }

        stat->size = file->size;
        stat->dir = false;

        return 0;
}

//==============================================================================
/**
 * @brief  Calculate CRC32.
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
                        crc = (crc >> 1) ^ (0xEDB88320 & mask);
                }
        }

        return ~crc;
}

//==============================================================================
/**
 * @brief Function returns a pseudo-random integer (LCG, modulus 2^32).
 */
//==============================================================================
static uint32_t prng(uint32_t *seed)
{
        *seed = (UINT32_C(1103515245) * *seed) + UINT32_C(12345);

        return *seed;
}

//==============================================================================
/**
 * @brief  Function encode/decode selected block.
 */
//==============================================================================
static void xcrypt(void *buf, uint32_t buflen, uint32_t *seed)
{
        uint8_t *blk = buf;

        while (buflen > 0) {
                *blk = *blk ^ cast(uint8_t, prng(seed));
                blk++;
                buflen--;
        }
}

//==============================================================================
/**
 * @brief  Duplicate string (portable strdup()).
 */
//==============================================================================
static char *str_dup(const char *str)
{
        size_t len = strlen(str) + 1;

        char *dup = malloc(len);
        if (dup) {
                memcpy(dup, str, len);
        }

        return dup;
}

//==============================================================================
/**
 * @brief  Read exact number of bytes from selected offset.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_at(fpkg_t *self, uint32_t offset, void *buf, size_t len)
{
        clearerr(self->file);

        if (fseek(self->file, cast(long, offset), SEEK_SET) != 0) {
                return -1;
        }

        if (len == 0) {
                return 0;
        }

        if ((fread(buf, 1, len, self->file) != len) or ferror(self->file)) {
                return -1;
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Read and check container header, compute image geometry.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_image_info(fpkg_t *self)
{
        if (fseek(self->file, 0, SEEK_END) != 0) {
                return -1;
        }

        long size = ftell(self->file);

        if ((size < cast(long, sizeof(container_header_t))) or (cast(uint64_t, size) > UINT32_MAX)) {
                return -1;
        }

        self->file_size = cast(uint32_t, size);

        container_header_t header;
        if (read_container_header(self, &header) != 0) {
                return -1;
        }

        if (not is_container_header_valid(&header)) {
                return -1;
        }

        uint64_t tables_size = sizeof(container_header_t)
                             + (cast(uint64_t, header.number_of_files) * sizeof(file_entry_t));

        if ((tables_size > self->file_size) or (header.number_of_files > INT32_MAX)) {
                return -1;
        }

        self->entries_offset = self->file_size - cast(uint32_t, tables_size);
        self->prng_seed = header.prng_seed;
        self->number_of_files = header.number_of_files;
        self->flags = header.flags;

        return 0;
}

//==============================================================================
/**
 * @brief  Read container header.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_container_header(fpkg_t *self, container_header_t *hdr)
{
        if (read_at(self, self->file_size - sizeof(container_header_t), hdr, sizeof(*hdr)) != 0) {
                return -1;
        }

        if (hdr->prng_seed != 0) {
                uint32_t seed = hdr->prng_seed + 1;
                hdr->number_of_files ^= prng(&seed);

                seed = hdr->prng_seed + 2;
                hdr->flags ^= prng(&seed);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Read n-th file entry.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_file_entry(fpkg_t *self, uint32_t n, file_entry_t *entry)
{
        if (read_at(self, self->entries_offset + (sizeof(file_entry_t) * n), entry, sizeof(*entry)) != 0) {
                return -1;
        }

        if (is_encrypted(self)) {
                uint32_t seed = self->prng_seed + n;
                xcrypt(entry, sizeof(*entry), &seed);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Read file descriptor.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_file_descriptor(fpkg_t *self, const file_entry_t *entry, file_descriptor_t *desc)
{
        if (read_at(self, entry->file_descriptor_address, desc, sizeof(*desc)) != 0) {
                return -1;
        }

        if (is_encrypted(self)) {
                uint32_t seed = self->prng_seed;
                xcrypt(desc, sizeof(*desc), &seed);
        }

        return 0;
}

//==============================================================================
/**
 * @brief  Read file name (NUL terminated). Name longer than buffer is
 *         truncated and reported by truncated flag.
 *
 * @return On success 0 is returned, otherwise -1 (buffer content undefined).
 */
//==============================================================================
static int read_file_descriptor_name(fpkg_t *self, const file_entry_t *entry, const file_descriptor_t *desc,
        char *buf, size_t buf_len, bool *truncated)
{
        size_t name_len = desc->file_name_len;
        *truncated = false;

        if (name_len > (buf_len - 1)) {
                name_len = buf_len - 1;
                *truncated = true;
        }

        if (read_at(self, entry->file_descriptor_address + sizeof(file_descriptor_t), buf, name_len) != 0) {
                buf[0] = '\0';
                return -1;
        }

        if (is_encrypted(self)) {
                uint32_t seed = desc->content_prng_seed;
                xcrypt(buf, name_len, &seed);
        }

        buf[name_len] = '\0';

        return 0;
}

//==============================================================================
/**
 * @brief  Read entry, descriptor and name (to self->tmp) of n-th item.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int read_item(fpkg_t *self, uint32_t n, file_entry_t *entry, file_descriptor_t *desc, bool *truncated)
{
        if (read_file_entry(self, n, entry) != 0) {
                return -1;
        }

        if (read_file_descriptor(self, entry, desc) != 0) {
                return -1;
        }

        return read_file_descriptor_name(self, entry, desc, self->tmp, sizeof(self->tmp), truncated);
}

//==============================================================================
/**
 * @brief  Compute search key length: trailing slashes are ignored, so "/dir/"
 *         and "/dir" are the same object, and "/" is the root (length 0).
 *
 * @return On success 0 is returned, otherwise -1 (path too long).
 */
//==============================================================================
static int normalize_path(const char *path, size_t *key_len)
{
        size_t len = strlen(path);

        while ((len > 0) and (path[len - 1] == '/')) {
                len--;
        }

        if (len > MAX_PATH_LEN) {
                return -1;
        }

        *key_len = len;

        return 0;
}

//==============================================================================
/**
 * @brief  Compare entry name with key (+ optional trailing '/'). Bytes are
 *         compared as unsigned (as strcmp()).
 *
 * @return <0, 0, >0 as strcmp().
 */
//==============================================================================
static int name_cmp(const char *name, bool truncated, const char *key, size_t key_len, bool slash)
{
        size_t total = key_len + (slash ? 1 : 0);

        for (size_t i = 0; i < total; i++) {
                uint8_t k = (i < key_len) ? cast(uint8_t, key[i]) : '/';
                uint8_t n = cast(uint8_t, name[i]);

                if (n != k) {
                        return (n < k) ? -1 : 1;
                }
        }

        return (name[total] or truncated) ? 1 : 0;
}

//==============================================================================
/**
 * @brief  Check if name is located in directory key ("key/...").
 */
//==============================================================================
static bool is_in_dir(const char *name, const char *key, size_t key_len)
{
        return (strncmp(name, key, key_len) == 0) and (name[key_len] == '/');
}

//==============================================================================
/**
 * @brief  Find first entry that is not less than key (lower bound).
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int lower_bound(fpkg_t *self, uint32_t lo, const char *key, size_t key_len, bool slash, uint32_t *idx)
{
        uint32_t hi = self->number_of_files;

        while (lo < hi) {
                uint32_t mid = lo + (hi - lo) / 2;

                file_entry_t entry;
                file_descriptor_t desc;
                bool trunc;
                if (read_item(self, mid, &entry, &desc, &trunc) != 0) {
                        return -1;
                }

                if (name_cmp(self->tmp, trunc, key, key_len, slash) < 0) {
                        lo = mid + 1;
                } else {
                        hi = mid;
                }
        }

        *idx = lo;

        return 0;
}

//==============================================================================
/**
 * @brief  Binary search of file or directory (sorted images only).
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int lookup_sorted(fpkg_t *self, const char *key, size_t key_len, item_t *item)
{
        bool trunc;
        uint32_t i;

        if (lower_bound(self, 0, key, key_len, false, &i) != 0) {
                return -1;
        }

        if (i < self->number_of_files) {
                if (read_item(self, i, &item->entry, &item->desc, &trunc) != 0) {
                        return -1;
                }

                if (name_cmp(self->tmp, trunc, key, key_len, false) == 0) {
                        item->index = i;
                        item->is_dir = false;
                        return 0;
                }
        }

        // "key/" > "key", so directory content cannot be located before i
        if (lower_bound(self, i, key, key_len, true, &i) != 0) {
                return -1;
        }

        if (i < self->number_of_files) {
                if (read_item(self, i, &item->entry, &item->desc, &trunc) != 0) {
                        return -1;
                }

                if (is_in_dir(self->tmp, key, key_len)) {
                        item->index = i;
                        item->is_dir = true;
                        return 0;
                }
        }

        return -1;
}

//==============================================================================
/**
 * @brief  Linear search of file or directory (unsorted images). Name is read
 *         only if length (and CRC for files) match.
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int lookup_linear(fpkg_t *self, const char *key, size_t key_len, item_t *item)
{
        uint32_t key_crc = calc_CRC32(key, cast(uint32_t, key_len), FPKG_CRC32_INIT);

        for (uint32_t i = 0; i < self->number_of_files; i++) {

                if (read_file_entry(self, i, &item->entry) != 0) {
                        return -1;
                }

                if (read_file_descriptor(self, &item->entry, &item->desc) != 0) {
                        return -1;
                }

                bool exact = (item->desc.file_name_len == key_len);
                bool longer = (item->desc.file_name_len > (key_len + 1));

                if (exact and (item->desc.file_name_crc32 != key_crc)) {
                        continue;
                }

                if (not exact and not longer) {
                        continue;
                }

                bool trunc;
                if (read_file_descriptor_name(self, &item->entry, &item->desc, self->tmp, sizeof(self->tmp),
                                &trunc) != 0) {
                        return -1;
                }

                if (exact and (memcmp(self->tmp, key, key_len) == 0)) {
                        item->index = i;
                        item->is_dir = false;
                        return 0;
                }

                if (longer and is_in_dir(self->tmp, key, key_len)) {
                        item->index = i;
                        item->is_dir = true;
                        return 0;
                }
        }

        return -1;
}

//==============================================================================
/**
 * @brief  Find file or directory. Root (key_len == 0) always exists.
 *
 * @param  self         container instance
 * @param  key          path (first key_len characters are used)
 * @param  key_len      key length
 * @param  item         found item (for directory: index of first entry)
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int lookup(fpkg_t *self, const char *key, size_t key_len, item_t *item)
{
        if (key_len == 0) {
                item->index = 0;
                item->is_dir = true;
                return 0;
        }

        if (is_sorted(self)) {
                return lookup_sorted(self, key, key_len, item);
        } else {
                return lookup_linear(self, key, key_len, item);
        }
}

//==============================================================================
/**
 * @brief  Find next directory item. Duplicated subdirectory names are skipped
 *         only if they are adjacent (always true for sorted images).
 *
 * @return FPKG_OK, FPKG_END_OF_DIR or one of FPKG_ERR_*.
 */
//==============================================================================
static int dir_next(fpkg_dir_t *dir, fpkg_dirent_t *dirent)
{
        fpkg_t *self = dir->self;

        while (dir->seek < self->number_of_files) {

                file_entry_t entry;
                file_descriptor_t desc;
                bool trunc;
                if (read_item(self, dir->seek, &entry, &desc, &trunc) != 0) {
                        return FPKG_ERR_IO;
                }

                dir->seek++;

                if (strncmp(self->tmp, dir->root, dir->root_len) != 0) {
                        if (is_sorted(self)) {
                                // sorted: directory content is contiguous
                                dir->seek = self->number_of_files;
                                break;
                        }
                        continue;
                }

                char *item_name = self->tmp + dir->root_len;

                char *slash = strchr(item_name, '/');
                if (slash) {
                        *slash = '\0';
                } else if (trunc) {
                        continue;       // name cannot be represented
                }

                if ((item_name[0] == '\0') or (strcmp(item_name, dir->item_name) == 0)) {
                        continue;
                }

                char *name = str_dup(item_name);
                if (not name) {
                        dir->seek--;
                        return FPKG_ERR_NOMEM;
                }

                free(dir->item_name);
                dir->item_name = name;

                dirent->name = dir->item_name;
                dirent->dir = (slash != NULL);
                dirent->size = slash ? 0 : desc.file_size;

                return FPKG_OK;
        }

        return FPKG_END_OF_DIR;
}

//==============================================================================
/**
 * @brief  Calculate CRC32 of image region (optionally decoded).
 *
 * @return On success 0 is returned, otherwise -1.
 */
//==============================================================================
static int region_crc32(fpkg_t *self, uint32_t offset, uint32_t len, uint32_t seed, bool decrypt,
        uint8_t *buf, uint32_t buflen, uint32_t *crc_out)
{
        uint32_t crc = ~FPKG_CRC32_INIT;

        if (read_at(self, offset, buf, 0) != 0) {
                return -1;
        }

        while (len > 0) {
                uint32_t n = min(len, buflen);

                if ((fread(buf, 1, n, self->file) != n) or ferror(self->file)) {
                        return -1;
                }

                if (decrypt) {
                        xcrypt(buf, n, &seed);
                }

                crc = calc_CRC32(buf, n, ~crc);
                len -= n;
        }

        *crc_out = crc;

        return 0;
}

//==============================================================================
/**
 * @brief  Full image check: CRC of entire image, entries, descriptors, names,
 *         content and (for sorted images) order of names.
 *
 * @param  self         container instance (geometry already read)
 *
 * @return True if file is valid, false if not.
 */
//==============================================================================
static bool is_file_valid(fpkg_t *self)
{
        uint8_t *buf = malloc(ALLOC_BUF_SIZE);
        char *prev = malloc(NAME_BUF_LEN);

        bool valid = (buf and prev);
        bool prev_trunc = false;
        uint32_t crc;

        if (valid) {
                valid = (region_crc32(self, 0, self->file_size - sizeof(uint32_t), 0, false,
                                buf, ALLOC_BUF_SIZE, &crc) == 0);
        }

        if (valid) {
                uint32_t file_crc;
                valid = (fread(&file_crc, sizeof(file_crc), 1, self->file) == 1) and (file_crc == crc);
        }

        for (uint32_t i = 0; valid and (i < self->number_of_files); i++) {

                file_entry_t entry;
                file_descriptor_t desc;

                valid = (read_file_entry(self, i, &entry) == 0)
                    and is_file_entry_valid(self, &entry)
                    and (read_file_descriptor(self, &entry, &desc) == 0)
                    and is_file_descriptor_valid(self, &entry, &desc);

                if (not valid) {
                        break;
                }

                uint32_t name_offset = entry.file_descriptor_address + sizeof(file_descriptor_t);

                valid = (region_crc32(self, name_offset, desc.file_name_len, desc.content_prng_seed,
                                is_encrypted(self), buf, ALLOC_BUF_SIZE, &crc) == 0)
                    and (crc == desc.file_name_crc32)
                    and (region_crc32(self, name_offset + desc.file_name_len, desc.file_size,
                                desc.content_prng_seed, is_encrypted(self), buf, ALLOC_BUF_SIZE, &crc) == 0)
                    and (crc == desc.file_crc32);

                if (valid and is_sorted(self)) {
                        bool trunc;
                        valid = (read_file_descriptor_name(self, &entry, &desc, self->tmp, sizeof(self->tmp),
                                        &trunc) == 0);

                        if (valid and (i > 0)) {
                                int cmp = strcmp(prev, self->tmp);

                                // wrong order or duplicated name
                                valid = (cmp < 0) or ((cmp == 0) and (prev_trunc or trunc));
                        }

                        if (valid) {
                                strcpy(prev, self->tmp);
                                prev_trunc = trunc;
                        }
                }
        }

        free(prev);
        free(buf);

        return valid;
}

//==============================================================================
/**
 * @brief  Function check if container header is coherent.
 */
//==============================================================================
static bool is_container_header_valid(const container_header_t *hdr)
{
        bool valid = false;

        uint32_t crc = calc_CRC32(&hdr->prng_seed,
                sizeof(*hdr) - offsetof(container_header_t, prng_seed) - sizeof(hdr->file_crc32),
                FPKG_CRC32_INIT);

        if ((hdr->magic == CONTAINER_HEADER_MAGIC) and (hdr->crc32 == crc)) {

                if (hdr->prng_seed != 0) {
                        valid = (hdr->flags & FLAG_ENCRYPTED);
                } else {
                        valid = not (hdr->flags & FLAG_ENCRYPTED);
                }
        }

        return valid;
}

//==============================================================================
/**
 * @brief  Check file entry coherency (magic, CRC, descriptor bounds).
 */
//==============================================================================
static bool is_file_entry_valid(const fpkg_t *self, const file_entry_t *file_entry)
{
        uint32_t crc = calc_CRC32(&file_entry->file_descriptor_address,
                sizeof(*file_entry) - offsetof(file_entry_t, file_descriptor_address), FPKG_CRC32_INIT);

        uint64_t desc_end = cast(uint64_t, file_entry->file_descriptor_address) + sizeof(file_descriptor_t);

        return ((file_entry->magic == FILE_ENTRY_MAGIC) and (file_entry->crc32 == crc) and
                (desc_end <= self->entries_offset));
}

//==============================================================================
/**
 * @brief  Check file descriptor coherency (magic, CRC, name/content bounds).
 */
//==============================================================================
static bool is_file_descriptor_valid(const fpkg_t *self, const file_entry_t *file_entry,
        const file_descriptor_t *file_desc)
{
        uint32_t crc = calc_CRC32(&file_desc->file_name_len,
                sizeof(*file_desc) - offsetof(file_descriptor_t, file_name_len), FPKG_CRC32_INIT);

        uint64_t end = cast(uint64_t, file_entry->file_descriptor_address)
                     + sizeof(file_descriptor_t)
                     + file_desc->file_name_len
                     + file_desc->file_size;

        return ((file_desc->magic == FILE_DESCRIPTOR_MAGIC) and (file_desc->crc32 == crc) and
                (file_desc->file_name_len > 0) and (end <= self->entries_offset));
}

/*==============================================================================
  End of file
==============================================================================*/
