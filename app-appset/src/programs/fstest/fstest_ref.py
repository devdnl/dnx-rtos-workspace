#!/usr/bin/env python3
"""
Generate fstest reference data set (fst_ref/) on host machine.

Content is identical to the one created by 'fstest -g' and is used to build
images of read-only file systems, e.g.:

    ./fstest_ref.py image_root/
    <make read-only image from image_root/>
    (on target) cd /mnt/rofs; fstest -r

Options must match fstest configuration (FSTEST_BIG_FILE_SIZE,
FSTEST_DIR_FILES, FSTEST_LONG_NAME_MAX).

MIT License, Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>
"""

import argparse
import os

REF_DIR = "fst_ref"
CHARS = "abcdefghijklmnopqrstuvwxyz0123456789"
NAME_LEN = [1, 8, 12, 13, 16, 31, 32, 64, 128]  # + long_name_max
BLOCK_SIZES = [1, 511, 512, 513, 1023, 1024, 1025, 4095, 4096, 4097]
SPECIAL_NAME = [
    "with space.txt",
    "multi.dot.name.ext",
    "UPPER.TXT",
    "MiXeD.CaSe",
    "noext",
    ".hidden",
    "dash-under_score~1",
    "plus+comma,eq=semi;",
    "brackets[(1)]",
    "hash#excl!at@",
]


def pattern(size, seed):
    """The same as pattern() in fstest.c."""
    return bytes(((pos * 7) ^ (pos >> 7) ^ (pos >> 13) ^ (seed * 29)) & 0xFF for pos in range(size))


def ref_files(big_size, dir_files, long_name_max):
    """Yield (path, size, seed) in the same order as ref_file_get()."""
    yield "empty.file", 0, 0
    yield "small.file", 100, 0x01
    yield "big.file", big_size + 3, 0x02
    yield "l1/l2/l3/deep.file", 1000, 0x5A

    for i, size in enumerate(BLOCK_SIZES):
        yield f"sizes.dir/{size}", size, (0x10 + i) & 0xFF

    for i in range(dir_files):
        yield f"list.dir/f{i:03d}", i, i & 0xFF

    for length in NAME_LEN + [long_name_max]:
        if length <= long_name_max:
            name = "".join(CHARS[k % len(CHARS)] for k in range(length))
            yield f"names.dir/{name}", length, length & 0xFF

    for i, name in enumerate(SPECIAL_NAME):
        yield f"names.dir/{name}", 20, i


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("root", help="directory where fst_ref/ is created")
    parser.add_argument("--big-size", type=int, default=32 * 1024, help="FSTEST_BIG_FILE_SIZE (default: 32768)")
    parser.add_argument("--dir-files", type=int, default=40, help="FSTEST_DIR_FILES (default: 40)")
    parser.add_argument("--long-name-max", type=int, default=200, help="FSTEST_LONG_NAME_MAX (default: 200)")
    args = parser.parse_args()

    if not 0 < args.dir_files <= 63:
        parser.error("--dir-files must be in range 1..63")

    base = os.path.join(args.root, REF_DIR)

    for d in ["", "empty.dir", "list.dir", "names.dir", "sizes.dir", "l1", "l1/l2", "l1/l2/l3"]:
        os.makedirs(os.path.join(base, d), exist_ok=True)

    count = 0
    for path, size, seed in ref_files(args.big_size, args.dir_files, args.long_name_max):
        with open(os.path.join(base, path), "wb") as f:
            f.write(pattern(size, seed))
        count += 1

    print(f"Reference data created in '{base}/' ({count} files).")


if __name__ == "__main__":
    main()
