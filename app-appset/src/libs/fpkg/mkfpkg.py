#!/usr/bin/env python3

# This script create fpkg container by using selected files.

import sys, os
from struct import *
import binascii
import time
import os.path
import random

random.seed(time.time())

encrypted  = False
desc_seed  = random.randint(0, (1<<32))
file_list  = []


CRC32_INIT             = 0xFFFFFFFF
FILE_DESCRIPTOR_MAGIC  = 0x43534446
FILE_ENTRY_MAGIC       = 0x544E4546
CONTAINER_HEADER_MAGIC = 0x474B5046


def rand(seed):
    a = 1103515245
    c = 12345
    m = (1 << 32)
    seed = ((a * seed + c) % m) & 0xFFFFFFFF
    return seed


def encdec(buf, seed):
    out = bytearray()

    for byte in buf:
        seed = rand(seed)
        out.append(byte ^ (seed & 0xFF))

    return out


def make_file_descriptor(file_size, file_crc32, prng_seed, file_name, file_data, fout):

    flags = 0
    if encrypted:  flags = flags | (1 << 0)

    desc_data = pack('<IIIIII', len(file_name),  binascii.crc32(bytes(file_name.encode('utf-8'))), file_size,
         file_crc32, prng_seed, flags)

    struct_format = '<' + str(len(file_name)) + 's'
    desc_file_name = pack(struct_format, bytes(file_name.encode('utf-8')))

    desc_hdr = pack('<II', FILE_DESCRIPTOR_MAGIC, binascii.crc32(desc_data))

    if encrypted:
        desc = encdec(desc_hdr + desc_data, desc_seed)
        desc_file_name = encdec(desc_file_name, prng_seed)
        file_data = encdec(file_data, prng_seed)
    else:
        desc = desc_hdr + desc_data

    fout.write(desc)
    fout.write(desc_file_name)
    fout.write(file_data)


def make_file_entry(file_absolute_address, n_entry):

    entry_data = pack('<II', file_absolute_address, 0)

    entry_hdr = pack('<II', FILE_ENTRY_MAGIC, binascii.crc32(entry_data))

    entry = entry_hdr + entry_data

    if encrypted:
        entry = encdec(entry, desc_seed + n_entry)

    return entry;


def make_container_header(number_of_files):
    global desc_seed

    desc = bytearray()

    flags = 0;

    if encrypted:
        flags = (1 << 0)

    desc = pack('<III', desc_seed, number_of_files, flags)
    crc  = binascii.crc32(desc)

    if encrypted:
        desc = pack('<III', desc_seed,
            number_of_files ^ rand(desc_seed + 1),
            flags ^ rand(desc_seed + 2) )

    desc_hdr = pack('<II', CONTAINER_HEADER_MAGIC, crc)

    return desc_hdr + desc;


if __name__ == "__main__":

    if len(sys.argv) < 3:
        print('Usage: ' + sys.argv[0] + ' [-e] <search-path> <output-file>')
        print('  -e         encrypted')
        sys.exit(1)

    for opt in sys.argv[1:]:
        if opt == '-e':
            encrypted = True

    # collect files in selected directory
    walk_dir = sys.argv[-2]
    output = sys.argv[-1]
    file_list = []

    for root, subdirs, files in os.walk(walk_dir):
        for filename in files:
            file_path = os.path.join(root, filename)
            file_list.append(file_path)

    file_list.sort()

    for file_name in file_list:
        print(file_name)

    # create output file
    with open(output, "wb") as fout:

        entry_list = []

        # foma header
        fout.write(b"FPKG\n")

        # skip header (max header size 64 bytes)
        fout.seek(64)

        n_entry = 0

        for file_name in file_list:

            with open(file_name, "rb") as fin:

                data           = fin.read()
                file_crc  = binascii.crc32(data)
                file_size = fin.tell()
                prng_seed = random.randint(0, (1<<32))

                write_pointer = fout.tell()
                if write_pointer % 4 != 0:
                    write_pointer = (write_pointer + 4) & 0xFFFFFFFC
                    fout.seek(write_pointer)

                make_file_descriptor(file_size, file_crc, prng_seed,
                    file_name.replace(walk_dir, ""), data, fout)

                entry = make_file_entry(write_pointer, n_entry)
                entry_list.append(entry)

                n_entry = n_entry + 1

        write_pointer = fout.tell()
        if write_pointer % 4 != 0:
            write_pointer = (write_pointer + 4) & 0xFFFFFFFC
            fout.seek(write_pointer)

        for entry in entry_list:
            fout.write(entry)

        hdr = make_container_header(len(file_list))
        fout.write(hdr)

    # calculate CRC32 from entire file and put at the end of file
    with open(output, "r+b") as fout:
        data = fout.read()
        file_crc = pack('<I', binascii.crc32(data))
        fout.seek(len(data))
        fout.write(file_crc)
