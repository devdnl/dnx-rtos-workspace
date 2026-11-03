#!/usr/bin/env bash
# @file    bsp_common.sh
# @brief   Common functions of BSP scripts (bsp_import.sh, bsp_export.sh,
#          config_sync.sh). The file is sourced, paths are relative to the
#          workspace root (BSP_ROOT_DIR).
#
#          MIT License
#
#          Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
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

BSP_ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BSP_CONFIGTOOL="$BSP_ROOT_DIR/kernel/tools/configtool.sh"

#-------------------------------------------------------------------------------
# @brief  Absolute path of file given relative to the current directory (or absolute).
#-------------------------------------------------------------------------------
bsp_abspath()
{
    case "$1" in
        /*) echo "$1" ;;
        *)  echo "$PWD/$1" ;;
    esac
}

#-------------------------------------------------------------------------------
# @brief  Default configuration of configuration directory (name of config/*).
#-------------------------------------------------------------------------------
bsp_default_dir()
{
    case "$1" in
        *-kernel) echo "$BSP_ROOT_DIR/kernel/config" ;;
        *-appset) echo "$BSP_ROOT_DIR/tools/appset/config" ;;
        *-romfs)  echo "$BSP_ROOT_DIR/tools/romfs/config" ;;
        *)        echo "" ;;
    esac
}

#-------------------------------------------------------------------------------
# @brief  List of configuration directories (names of config/*) in BSP file.
#-------------------------------------------------------------------------------
bsp_dirs()
{
    grep -oE '^[[:space:]]*\["config/[^/"]+/' "$1" | sed -E 's/.*"config\/([^/]+)\//\1/' | sort -u
}

#-------------------------------------------------------------------------------
# @brief  Check if configuration directory name matches the specification.
#         Specification: patterns separated by '|', pattern with '*' or '?' is
#         a shell pattern, other patterns are prefixes, e.g. "*" or "all",
#         "app*|btl*", "app-|sim-", "sim-kernel".
#-------------------------------------------------------------------------------
bsp_match()
{
    local name="$1" spec="$2" pat
    local -a pats

    # "all" is the same as "*" (no shell quoting needed)
    if [ "$spec" == "all" ]; then
        spec="*"
    fi

    # split without pathname expansion ("*" must not be expanded to file names)
    IFS='|' read -r -a pats <<< "$spec"

    for pat in "${pats[@]}"; do
        case "$pat" in
            *[\*\?]*) [[ "$name" == $pat ]] && return 0 ;;
            *)        [[ "$name" == "$pat"* ]] && return 0 ;;
        esac
    done
    return 1
}

#-------------------------------------------------------------------------------
# @brief  Copy default configuration to configuration directory. Existing linker
#         scripts (*.ld) are not overwritten: they are generated from
#         custom_linker_script.h by the build.
#-------------------------------------------------------------------------------
bsp_copy_defaults()
{
    local src="$1" dst="$2" f rel
    mkdir -p "$dst"
    (cd "$src" && find . -type f) | while read -r rel; do
        rel="${rel#./}"
        if [[ "$rel" == *.ld ]] && [ -f "$dst/$rel" ]; then
            continue
        fi
        mkdir -p "$dst/$(dirname "$rel")"
        cp "$src/$rel" "$dst/$rel"
    done
}

#-------------------------------------------------------------------------------
# @brief  Reset configuration directory to the default configuration.
#-------------------------------------------------------------------------------
bsp_reset_dir()
{
    local name="$1"
    local def
    def="$(bsp_default_dir "$name")"

    if [ -z "$def" ] || [ ! -d "$def" ]; then
        echo "[warning] no default configuration of config/$name" >&2
        return 0
    fi

    bsp_copy_defaults "$def" "$BSP_ROOT_DIR/config/$name"
}
