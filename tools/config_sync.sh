#!/usr/bin/env bash
# @file    config_sync.sh
#
# @brief   Synchronize all workspace configuration directories (config/*) and
#          BSP files (bsp/*) with the default configurations of the kernel,
#          appset and romfs build bases:
#
#            1. config/*-kernel  is synchronized with kernel/config
#            2. config/*-appset  is synchronized with tools/appset/config
#            3. config/*-romfs   is synchronized with tools/romfs/config
#            4. every BSP file is imported (its configuration directories are
#               reset to the default configuration before import) and exported
#               again with the same configuration directories (each directory
#               is optimized for its CPU architecture)
#            5. BSP files given as arguments are applied in the given order
#               to restore the configuration of the project; without
#               arguments bsp/stm32h747i-disc1.bsp (default configuration of
#               the workspace) and bsp/posix-sim.bsp (configuration of the
#               simulator, config/sim-*) are applied
#
#          Usage: tools/config_sync.sh [BSP file...]
#
# @note    All paths are relative to the workspace root (the parent
#          directory of this script).
#
#          MIT License
#
#          Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#          Permission is hereby granted, free of charge, to any person obtaining
#          a copy of this software and associated documentation files (the
#          "Software"), to deal in the Software without restriction, including
#          without limitation the rights to use, copy, modify, merge, publish,
#          distribute, sublicense, and/or sell copies of the Software, and to
#          permit persons to whom the Software is furnished to do so, subject to
#          the following conditions:
#
#          The above copyright notice and this permission notice shall be included
#          in all copies or substantial portions of the Software.
#
#          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#          OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#          MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#          IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#          CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#          TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#          SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

set -euo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

source tools/bsp_common.sh

for bsp in "$@"; do
    if [ ! -f "$bsp" ]; then
        echo "[error] BSP file not found: $bsp" >&2
        exit 1
    fi
done


#--- Step 1: sync kernel configurations ---------------------------------------
for target in config/*-kernel; do
    if [ ! -d "$target" ]; then
        continue
    fi
    if [ ! -d kernel/config ]; then
        echo "[error] source directory not found: kernel/config" >&2
        exit 1
    fi
    echo "==> Sync $target with default from kernel/config"
    bsp_copy_defaults kernel/config "$target"
done

#--- Step 2: sync appset configurations ---------------------------------------
for target in config/*-appset; do
    if [ ! -d "$target" ]; then
        continue
    fi
    if [ ! -d tools/appset/config ]; then
        echo "[error] source directory not found: tools/appset/config" >&2
        exit 1
    fi
    echo "==> Sync $target with default from tools/appset/config"
    bsp_copy_defaults tools/appset/config "$target"
done

#--- Step 3: sync romfs configurations ----------------------------------------
for target in config/*-romfs; do
    if [ ! -d "$target" ]; then
        continue
    fi
    if [ ! -d tools/romfs/config ]; then
        echo "[error] source directory not found: tools/romfs/config" >&2
        exit 1
    fi
    echo "==> Sync $target with default from tools/romfs/config"
    bsp_copy_defaults tools/romfs/config "$target"
done

#--- Step 4: re-import and re-export every BSP file ---------------------------
for bsp in bsp/*.bsp; do
    if [ ! -f "$bsp" ]; then
        continue
    fi

    # configuration directories included in the BSP file
    dirs=$(bsp_dirs "$bsp" | paste -sd'|')
    if [ -z "$dirs" ]; then
        echo "[warning] $bsp: no configuration directories" >&2
        continue
    fi

    echo "==> Import $bsp"
    bash tools/bsp_import.sh "$bsp"

    echo "==> Export $bsp ($dirs)"
    bash tools/bsp_export.sh "$bsp" "$dirs"
done

#--- Step 5: apply BSP files of the project -----------------------------------
if [ $# -eq 0 ]; then
    set -- bsp/stm32h747i-disc1.bsp bsp/posix-sim.bsp
fi

for bsp in "$@"; do
    echo "==> Apply BSP $bsp"
    bash tools/bsp_import.sh "$bsp"
done

echo "== Configuration synchronized."
