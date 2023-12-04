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
compressed = False
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


def make_file_descriptor(original_file_size, container_file_size, orignal_file_crc32, container_file_crc32, prng_seed, file_name, file_data, fout):

    flags = 0
    if encrypted:  flags = flags | (1 << 1)
    if compressed: flags = falgs | (1 << 2)

    struct_format = '<IIIIIII' + str(len(file_name)) + 's'

    desc_data = pack(struct_format, len(file_name), original_file_size, container_file_size,
                           orignal_file_crc32, container_file_crc32,
                           flags, prng_seed, bytes(file_name.encode('utf-8')))

    desc_hdr = pack('<II', FILE_DESCRIPTOR_MAGIC, binascii.crc32(desc_data))

    desc = desc_hdr + desc_data

    if encrypted:
        desc = encdec(desc, desc_seed)
        file_data = encdec(file_data, prng_seed)

    fout.write(desc)
    fout.write(file_data)


def make_file_entry(file_absolute_address):

    entry_data = pack('<II', file_absolute_address, 0)

    entry_hdr = pack('<II', FILE_ENTRY_MAGIC, binascii.crc32(entry_data))

    entry = entry_hdr + entry_data

    if encrypted:
        entry = encdec(entry, desc_seed)

    return entry;


def make_container_header(number_of_files):
    flags = 0
    if encrypted:  flags = flags | (1 << 0)

    desc = pack('<IHH', desc_seed,
                (number_of_files ^ ((desc_seed >> 16)  & 0xFFFF)),
                (flags ^ (desc_seed & 0xFFFF)) )

    desc_hdr = pack('<II', CONTAINER_HEADER_MAGIC, binascii.crc32(desc))

    return desc_hdr + desc;


if __name__ == "__main__":

    for opt in sys.argv[1:]:
        if opt == '-e':
            encrypted = True
        elif opt == '-c':
            compressed = True
        else:
            file_list.append(opt)

    if len(file_list) < 1:
        print('Usage: ' + sys.argv[0] + ' [-e] [-c] <file 1> ... <file n> <out file>')
        print('  -e         encrypted file')
        print('  -c         compressed file')
        sys.exit(1)

    else:
        output = file_list[-1]
        file_list = file_list[0:-1]

        with open(output, "wb") as fout:

            entry_list = []

            # foma header
            fout.write(b"ROMFS\n")

            # skip header (max header size 64 bytes)
            fout.seek(64)

            for file_name in file_list:

                with open(file_name, "rb") as fin:

                    data           = fin.read()
                    orig_file_crc  = binascii.crc32(data)
                    orig_file_size = fin.tell()
                    prng_seed      = random.randint(0, (1<<32))

                    write_pointer = fout.tell()
                    if write_pointer % 4 != 0:
                        write_pointer = (write_pointer + 4) & 0xFFFFFFFC
                        fout.seek(write_pointer)

                    if compressed:
                        cont_file_crc  = orig_file_crc  # TODO
                        cont_file_size = orig_file_size # TODO
                    else:
                        cont_file_crc  = orig_file_crc
                        cont_file_size = orig_file_size
                    print(write_pointer)
                    make_file_descriptor(orig_file_size, cont_file_size,
                                         orig_file_crc, cont_file_crc,
                                         prng_seed, file_name, data, fout)

                    entry = make_file_entry(write_pointer)
                    entry_list.append(entry)


            write_pointer = fout.tell()
            if write_pointer % 4 != 0:
                write_pointer = (write_pointer + 4) & 0xFFFFFFFC
                fout.seek(write_pointer)

            for entry in entry_list:
                fout.write(entry)

            hdr = make_container_header(len(file_list))
            fout.write(hdr)
