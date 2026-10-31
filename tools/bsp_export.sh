#!/usr/bin/env bash
# @file    bsp_export.sh
# @brief   Export workspace configuration (config/*) to BSP file. Each exported
#          configuration directory is optimized for its CPU architecture
#          (__CPU_ARCH__ of the directory): files of other architectures are
#          not exported (they have default values after import).
#
#          Usage: tools/bsp_export.sh <bsp-file> [dirs]
#            dirs   configuration directories (names of config/*): patterns
#                   separated by '|', pattern with '*' is a shell pattern,
#                   other patterns are prefixes, e.g. "*" (default: all),
#                   "app*|btl*", "app-|sim-", "sim-"
#
#          MIT License, Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
#          (license text: tools/bsp_common.sh)

set -euo pipefail

source "$(dirname -- "${BASH_SOURCE[0]}")/bsp_common.sh"

if [ "$#" == "0" ]; then
    echo "Usage: $0 <bsp-file> [dirs]"
    exit 1
fi

bsp="$(bsp_abspath "$1")"
spec="${2:-*}"

dirs=""
for dir in "$BSP_ROOT_DIR"/config/*/; do
    name="$(basename "$dir")"
    if bsp_match "$name" "$spec"; then
        dirs="$dirs,config/$name"
    fi
done

if [ -z "$dirs" ]; then
    echo "[error] no configuration directory matches '$spec'" >&2
    exit 1
fi

bash "$BSP_CONFIGTOOL" export="$bsp":"$BSP_ROOT_DIR/$dirs"
lua "$BSP_ROOT_DIR/tools/optimize_bsp.lua" "$bsp"
