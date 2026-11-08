#!/usr/bin/env bash
# ==================================================================================================
# @file   test.sh
#
# @author Daniel Zorychta
#
# @brief  Tests of drvset partition (drivers built separately from the kernel), no hardware needed.
#
#         The script builds the test kernel (configuration config/app-kernel with drvset and IRQ
#         module enabled by kernel.bsp), drvset partitions and checks them against the kernel
#         image by drvset_check.py (the same checks as the kernel at start and binary interface
#         of kernel functions):
#           - example partition (tools/drvset/example),
#           - test drivers (drivers/): kernel functions, driver interfaces of kernel drivers,
#             exclusive and shared interrupt handlers, character and block devices, file system
#             (ramfs of the kernel built into the partition as dsramfs),
#           - negative tests: not exported kernel function (link error), interrupt handled by
#             the kernel image, module name of the kernel, file system name of the kernel,
#             partition linked for other kernel address (check errors).
#
#         Usage: tools/drvset/test/test.sh   (or: make test-drvset)
#         Output: build/drvset-test (logs of each test), SUMMARY line.
#
#         MIT License
#
#         Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
#
#         Permission is hereby granted, free of charge, to any person obtaining
#         a copy of this software and associated documentation files (the
#         "Software"), to deal in the Software without restriction, including
#         without limitation the rights to use, copy, modify, merge, publish,
#         distribute, sublicense, and/or sell copies of the Software, and to
#         permit persons to whom the Software is furnished to do so, subject to
#         the following conditions:
#
#         The above copyright notice and this permission notice shall be included
#         in all copies or substantial portions of the Software.
#
#         THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#         OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#         FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#         THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#         OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#         ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#         OTHER DEALINGS IN THE SOFTWARE.
#
# ==================================================================================================

set -u

WORKSPACE=$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)
cd "$WORKSPACE"

TEST_DIR=tools/drvset/test
OUT=build/drvset-test
KCONF=$OUT/kconf/config
KERNEL_ELF=$OUT/dnx.elf
CHECK="python3 tools/drvset/drvset_check.py"

passed=0
failed=0

result()
{
    if [ "$1" -eq 0 ]; then
        echo "PASS: $2"
        passed=$((passed + 1))
    else
        echo "FAIL: $2 (log: $3)"
        failed=$((failed + 1))
    fi
}

# build drvset partition: <name> <drvset dir> [make variables...]
build_drvset()
{
    local name=$1 dir=$2
    shift 2
    make -C tools/drvset -s -fMakefile all DRVSET_DIR="$dir" CONFIG_DIR="$KCONF" BUILD_DIR="$OUT/$name" "$@" \
         > "$OUT/$name.log" 2>&1
}

# check drvset partition against test kernel: <name> [make variables...]
check_drvset()
{
    local name=$1
    shift
    make -C tools/drvset -s -fMakefile check DRVSET_DIR="$WORKSPACE/tools/drvset/example" CONFIG_DIR="$KCONF" \
         BUILD_DIR="$OUT/$name" KERNEL_ELF="$KERNEL_ELF" CHECK_OPTS=-v "$@" >> "$OUT/$name.log" 2>&1
}

# expected check error: <name> <message>
expect_check_error()
{
    local name=$1 msg=$2
    if check_drvset "$name"; then
        return 1
    fi
    grep -q "ERROR: .*$msg" "$OUT/$name.log"
}

echo "Preparing test kernel configuration..."
rm -rf "$OUT"
mkdir -p "$OUT/kconf"
cp -r config/app-kernel "$KCONF"
if ! bash kernel/tools/configtool.sh import=$TEST_DIR/kernel.bsp:$OUT/kconf > "$OUT/config.log" 2>&1 ||
   ! bash tools/create_linker_script.sh "$KCONF/custom_linker_script.h" "$KCONF/custom_linker_script.ld" \
          >> "$OUT/config.log" 2>&1; then
    echo "FAIL: test kernel configuration (log: $OUT/config.log)"
    exit 1
fi

echo "Building test kernel..."
make -C kernel -s clean > /dev/null 2>&1
make -C kernel -s -fMakefile all CONFIG_DIR="../$KCONF" > "$OUT/kernel.log" 2>&1
rc=$?
[ $rc -eq 0 ] && cp kernel/build/output/dnx.elf "$KERNEL_ELF"
make -C kernel -s clean > /dev/null 2>&1
result $rc "kernel with drvset support" "$OUT/kernel.log"
if [ $rc -ne 0 ]; then
    echo "SUMMARY: drvset tests: $passed passed, $failed failed"
    exit 1
fi

# interrupts of test kernel: free (default handler) and handled by kernel image
info=$($CHECK --kernel-info "$KERNEL_ELF")
free_irqs=$(echo "$info" | sed -n 's/^free_irqs=//p' | tr ',' ' ')
used_irqs=$(echo "$info" | sed -n 's/^used_irqs=//p' | tr ',' ' ')
modules=$(echo "$info" | sed -n 's/^modules=//p')
filesystems=$(echo "$info" | sed -n 's/^filesystems=//p')
read -r irq1 irq2 irq3 _ <<< "$free_irqs"
read -r irq_used _ <<< "$used_irqs"
echo "Test interrupts: shared $irq1, $irq2, exclusive $irq3, handled by kernel ${irq_used:-none}"

# example partition
build_drvset example tools/drvset/example && check_drvset example
result $? "example partition" "$OUT/example.log"

# test drivers: exclusive handler name of free interrupt from generated interrupt list
handler=$(sed -n "s/^ *X($irq3, \([A-Za-z0-9_]*\)).*/\1/p" "$OUT/example/gen/drvset_irq.h")
build_drvset drivers $TEST_DIR/drivers \
             CFLAGS_DRVSET="-DTEST_IRQ=$irq1 -DTEST_IRQ2=$irq2 -DTEST_IRQ_HANDLER=$handler" &&
    check_drvset drivers &&
    grep -q "modules: DSTEST, DSTESTBLK" "$OUT/drivers.log" &&
    grep -q "file systems: dsramfs" "$OUT/drivers.log" &&
    grep -q "IRQ $irq3: exclusive handler" "$OUT/drivers.log" &&
    [ "$(grep -c "IRQ $irq1: shared handler" "$OUT/drivers.log")" -eq 2 ] &&
    grep -q "IRQ $irq2: shared handler" "$OUT/drivers.log" &&
    grep -q "stub _IRQ_DDI_attach" "$OUT/drivers.log" &&
    grep -q "stub sys_strlcat" "$OUT/drivers.log"
result $? "test drivers (kernel functions, driver interfaces, interrupts, block device, file system)" "$OUT/drivers.log"

# negative: kernel function that is not exported
! build_drvset neg-undefined $TEST_DIR/neg-undefined && grep -q "undefined reference to .*_vfs_init" \
    "$OUT/neg-undefined.log"
result $? "not exported kernel function rejected by linker" "$OUT/neg-undefined.log"

# negative: interrupt handled by kernel image
if [ -n "${irq_used:-}" ]; then
    build_drvset neg-irq $TEST_DIR/neg-irq CFLAGS_DRVSET="-DTEST_IRQ_USED=$irq_used" &&
        expect_check_error neg-irq "IRQ $irq_used is not available"
    result $? "interrupt of kernel image rejected" "$OUT/neg-irq.log"
fi

# negative: module name of kernel
if [[ ",$modules," == *",GPIO,"* ]]; then
    build_drvset neg-module $TEST_DIR/neg-module && expect_check_error neg-module "module GPIO already exists"
    result $? "module name of kernel rejected" "$OUT/neg-module.log"
fi

# negative: file system name of kernel
if [[ ",$filesystems," == *",ramfs,"* ]]; then
    build_drvset neg-fs $TEST_DIR/neg-fs && expect_check_error neg-fs "file system ramfs already exists"
    result $? "file system name of kernel rejected" "$OUT/neg-fs.log"
fi

# negative: partition linked for other kernel address
build_drvset neg-address tools/drvset/example KERNEL_VECTORS=0x08100000 &&
    expect_check_error neg-address "linked for kernel at another address"
result $? "partition of other kernel address rejected" "$OUT/neg-address.log"

echo "SUMMARY: drvset tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
