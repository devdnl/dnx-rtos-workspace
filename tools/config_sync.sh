#!/usr/bin/env bash
# @file    config_sync.sh
#
# @brief   Synchronize all workspace configuration directories (config/*) and
#          BSP files (bsp/*) with the default configurations of the kernel,
#          appset and romfs projects:
#
#            1. config/*-kernel  is synchronized with kernel/config
#            2. config/*-appset  is synchronized with app-appset/config
#            3. config/*-romfs   is synchronized with app-romfs/config
#            4. every BSP file is imported (its configuration directories are
#               reset to the default configuration before import) and exported
#               again with the same configuration directories (each directory
#               is optimized for its CPU architecture)
#            5. bsp/stm32h747i-disc1.bsp is applied to set the default
#               configuration of the project, bsp/posix-sim.bsp is applied to
#               set the configuration of the simulator (config/sim-*)
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
    if [ ! -d app-appset/config ]; then
        echo "[error] source directory not found: app-appset/config" >&2
        exit 1
    fi
    echo "==> Sync $target with default from app-appset/config"
    bsp_copy_defaults app-appset/config "$target"
done

#--- Step 3: sync romfs configurations ----------------------------------------
for target in config/*-romfs; do
    if [ ! -d "$target" ]; then
        continue
    fi
    if [ ! -d app-romfs/config ]; then
        echo "[error] source directory not found: app-romfs/config" >&2
        exit 1
    fi
    echo "==> Sync $target with default from app-romfs/config"
    bsp_copy_defaults app-romfs/config "$target"
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

#--- Step 5: apply the default BSP of the project -----------------------------
echo "==> Apply default BSP bsp/stm32h747i-disc1.bsp"
bash tools/bsp_import.sh bsp/stm32h747i-disc1.bsp

echo "==> Apply simulator BSP bsp/posix-sim.bsp"
bash tools/bsp_import.sh bsp/posix-sim.bsp

echo "== Configuration synchronized."
