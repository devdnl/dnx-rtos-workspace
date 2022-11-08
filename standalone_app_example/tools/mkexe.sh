#!/usr/bin/env bash

source=$1   # filename without extention
arch=$2     # architecture id (0:ARM, 1:ARM_THUMB)
output=$3   # output file name

version=1
type=0
flags=0

echo -ne '#!exec\n\0' > $output     # shebang
echo -ne '\x'$version >> $output    # version
echo -ne '\x'$arch >> $output       # arch (1:ARCH_CORTEX_M_THUMB)
echo -ne '\x'$type >> $output       # type (0:executable)
echo -ne '\x'$flags >> $output      # flags

# BSS section size
bss_size=$(($(readelf "${source}.elf" -S | grep bss | sed 's/.*NOBITS\s*[0-9a-fA-F]*\s*[0-9a-fA-F]*\s*\([0-9a-fA-F]*\).*/0x\1/')))

bss_size_str=$(printf '%08X' $bss_size)

bss_size_0="\x$(printf '%02X' $((($bss_size >>  0) & 0xFF)))"
bss_size_1="\x$(printf '%02X' $((($bss_size >>  8) & 0xFF)))"
bss_size_2="\x$(printf '%02X' $((($bss_size >> 16) & 0xFF)))"
bss_size_3="\x$(printf '%02X' $((($bss_size >> 24) & 0xFF)))"

echo -ne $bss_size_0$bss_size_1$bss_size_2$bss_size_3 >> $output

# code
dd if=${source}.bin of=$output obs=1 seek=128 >/dev/null 2>&1
