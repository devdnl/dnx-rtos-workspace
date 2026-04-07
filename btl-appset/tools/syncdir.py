#!/usr/bin/env python3

import os
import hashlib
import shutil
import sys


def md5sum(file_path, chunk_size=65536):
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(chunk_size), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def sync_dirs(src, dst):
    for root, dirs, files in os.walk(src):
        rel_path = os.path.relpath(root, src)
        dst_root = os.path.join(dst, rel_path)

        if not os.path.exists(dst_root):
            os.makedirs(dst_root)

        for file in files:
            src_file = os.path.join(root, file)
            dst_file = os.path.join(dst_root, file)

            if not os.path.exists(dst_file):
                shutil.copy2(src_file, dst_file)
            else:
                if md5sum(src_file) != md5sum(dst_file):
                    shutil.copy2(src_file, dst_file)
                else:
                    pass

    for root, dirs, files in os.walk(dst, topdown=False):
        rel_path = os.path.relpath(root, dst)
        src_root = os.path.join(src, rel_path)

        for file in files:
            dst_file = os.path.join(root, file)
            src_file = os.path.join(src_root, file)

            if not os.path.exists(src_file):
                os.remove(dst_file)

        if not os.path.exists(src_root):
            shutil.rmtree(root)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 syncdir.py <src> <dst>")
        sys.exit(1)

    src_dir = sys.argv[1]
    dst_dir = sys.argv[2]

    if not os.path.isdir(src_dir):
        print("Source path is not a directory!")
        sys.exit(1)

    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)

    sync_dirs(src_dir, dst_dir)
