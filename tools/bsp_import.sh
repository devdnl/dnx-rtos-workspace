#!/usr/bin/env bash
# @file    bsp_import.sh
# @brief   Import BSP file to the workspace configuration (config/*).
#          Configuration directories included in the BSP file are reset to the
#          default configuration before import (result: default configuration
#          + BSP values, independent of the previous configuration), other
#          directories are not changed.
#
#          Usage: tools/bsp_import.sh <bsp-file> [--no-reset]
#            --no-reset   BSP values are applied on the current configuration
#
#          MIT License, Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
#          (license text: tools/bsp_common.sh)

set -euo pipefail

source "$(dirname -- "${BASH_SOURCE[0]}")/bsp_common.sh"

if [ "$#" == "0" ]; then
    echo "Usage: $0 <bsp-file> [--no-reset]"
    exit 1
fi

bsp="$(bsp_abspath "$1")"
reset=true

if [ "${2:-}" == "--no-reset" ]; then
    reset=false
fi

if [ ! -f "$bsp" ]; then
    echo "[error] BSP file not found: $1" >&2
    exit 1
fi

if $reset; then
    for name in $(bsp_dirs "$bsp"); do
        bsp_reset_dir "$name"
    done
fi

bash "$BSP_CONFIGTOOL" import="$bsp":"$BSP_ROOT_DIR/"
