#!/usr/bin/env python3
# ==================================================================================================
# @file   drvset_check.py
#
# @author Daniel Zorychta
#
# @brief  Offline check of drvset partition image against kernel image (no hardware needed).
#
#         The script makes the same checks as the kernel at start (kernel/src/drivers/drvset.c)
#         and additionally checks the binary interface: each stub of kernel function linked into
#         drvset (kernel/src/drvset/ksymstub.S) shall select the kernel symbol table entry of the
#         function with the same name. Images are read from ELF files, no other tools are used.
#
#         Usage:
#           drvset_check.py <kernel.elf> <drvset.elf> [--flash ADDR] [--size BYTES]
#                           [--ram BYTES] [--max-modules N] [-v]
#           drvset_check.py --kernel-info <kernel.elf>
#
#         --kernel-info       prints interrupts available for drvset (free_irqs=), interrupts
#                             handled by the kernel image (used_irqs=), built-in modules
#                             (modules=) and file systems (filesystems=), used by tests
#
#         --flash, --size     drvset partition (__OS_DRVSET_FLASH_ADDRESS__, __OS_DRVSET_FLASH_SIZE__),
#                             default: partition starts at the header, size is not checked
#         --ram               __OS_DRVSET_RAM_SIZE__ in bytes, needed for the POSIX simulator
#                             (kernel image i386: RAM mapped after the partition)
#         --max-modules       __OS_DRVSET_MAX_MODULES__, default: not checked
#
#         Exit status: 0 image is correct, 1 errors found, 2 incorrect files.
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

import argparse
import struct
import sys

# binary interface (kernel/src/include/drivers/drvset.h, kernel/ksymtab.h, kernel/kvectors.h)
DRVSET_MAGIC = 0x53565244
DRVSET_HEADER_VERSION = 1
DRVSET_HEADER = '<IHHIIIIIIIIIIIIIIII'  # 72 bytes
DRVSET_FIELDS = ('magic', 'header_version', 'ksymtab_version', 'ksymtab_count', 'image_size', 'data_load',
                 'data_start', 'data_end', 'bss_start', 'bss_end', 'modules', 'modules_count', 'irq_table',
                 'irq_count', 'ksymtab_vector', 'irq_shared', 'irq_shared_count', 'filesystems',
                 'filesystems_count')
KSYMTAB_MAGIC = 0x4D59534B
KSYMTAB_SYM_OFFSET = 8
KVECTOR_KSYMTAB = 254
KVECTOR_IRQ0 = 16
IRQ_MAX = 256 - KVECTOR_IRQ0
MODULE_ENTRY_SIZE = 44  # name, type, 9 interface functions
IRQ_SHARED_SIZE = 8
FS_MAGIC = 0xD9EFD24F  # _VFS_FILE_SYSTEM_MAGIC_NO, last member of file system interface

# kernel symbols exported under other kernel names (kernel/src/kernel/ksymtab.c)
KSYM_ALIASES = {'sys_strlcpy': '_strlcpy', 'sys_strlcat': '_strlcat'}


class ElfError(Exception):
    pass


class Elf32:
    """Minimal reader of 32-bit little-endian ELF: symbols and memory of loadable segments."""

    def __init__(self, path):
        self.path = path
        with open(path, 'rb') as f:
            self.data = f.read()

        d = self.data
        if d[:4] != b'\x7fELF' or d[4] != 1 or d[5] != 1:
            raise ElfError(f'{path}: not a 32-bit little-endian ELF file')

        (self.e_type, self.e_machine, _, _, e_phoff, e_shoff, _, _, e_phentsize, e_phnum, e_shentsize,
         e_shnum, e_shstrndx) = struct.unpack_from('<HHIIIIIHHHHHH', d, 16)

        self.segments = []
        for i in range(e_phnum):
            p_type, p_offset, p_vaddr, p_paddr, p_filesz, _, _, _ = struct.unpack_from('<IIIIIIII', d,
                                                                                       e_phoff + i * e_phentsize)
            if p_type == 1 and p_filesz:
                self.segments.append((p_vaddr, p_paddr, p_offset, p_filesz))

        sections = [struct.unpack_from('<IIIIIIIIII', d, e_shoff + i * e_shentsize) for i in range(e_shnum)]
        self.symbols = {}
        self.functions = {}
        for sh in sections:
            if sh[1] != 2:  # SHT_SYMTAB
                continue
            strtab = sections[sh[6]]
            for off in range(sh[4], sh[4] + sh[5], 16):
                st_name, st_value, st_size, st_info, _, st_shndx = struct.unpack_from('<IIIBBH', d, off)
                if st_name == 0 or st_shndx == 0:
                    continue
                end = d.index(b'\0', strtab[4] + st_name)
                name = d[strtab[4] + st_name:end].decode()
                if (st_info & 0xF) == 2:  # STT_FUNC
                    self.functions[name] = st_value
                if (st_info >> 4) != 0 or name not in self.symbols:  # global symbols first
                    self.symbols[name] = (st_value, st_size)

        if not self.segments:
            raise ElfError(f'{path}: no loadable segments')

    def sym(self, name):
        if name not in self.symbols:
            raise ElfError(f'{self.path}: symbol {name} not found')
        return self.symbols[name][0]

    def has(self, name):
        return name in self.symbols

    def read(self, addr, size):
        """Reads memory of image (load addresses, flash)."""
        for vaddr, paddr, offset, filesz in self.segments:
            for base in (paddr, vaddr):
                if base <= addr and addr + size <= base + filesz:
                    pos = offset + addr - base
                    return self.data[pos:pos + size]
        return None

    def u32(self, addr):
        b = self.read(addr, 4)
        return None if b is None else struct.unpack('<I', b)[0]

    def u16(self, addr):
        b = self.read(addr, 2)
        return None if b is None else struct.unpack('<H', b)[0]

    def string(self, addr, limit=64):
        out = b''
        while len(out) < limit:
            b = self.read(addr + len(out), 1)
            if b is None:
                return None
            if b == b'\0':
                return out.decode(errors='replace')
            out += b
        return None


class Checker:

    def __init__(self, kernel, drvset, flash, size, max_modules, verbose, ram=None):
        self.k = kernel
        self.d = drvset
        self.flash = flash
        self.size = size
        self.ram = ram
        self.max_modules = max_modules
        self.verbose = verbose
        self.errors = []

    def error(self, msg):
        self.errors.append(msg)

    def info(self, msg):
        if self.verbose:
            print('  ' + msg)

    def in_flash(self, addr, size):
        return self.flash <= addr and addr + size <= self.flash_end

    def in_ram(self, start, end):
        return start <= end and self.ram_start <= start and end <= self.ram_end

    def kernel_info(self):
        k = self.k
        # POSIX simulator (i386): no vector table, RAM of partition is mapped after the partition
        self.posix = k.e_machine == 3
        if self.posix:
            self.kernel_info_posix()
            return

        self.vectors_addr = k.sym('vectors')
        self.vectors = [k.u32(self.vectors_addr + 4 * i) for i in range(256)]
        if None in self.vectors:
            raise ElfError(f'{k.path}: vector table is not in image')

        self.default_handler = k.sym('__Default_Handler') & ~1

        self.ksymtab = self.vectors[KVECTOR_KSYMTAB]
        if not self.ksymtab or k.u32(self.ksymtab) != KSYMTAB_MAGIC:
            raise ElfError(f'{k.path}: kernel symbol table not found (drvset disabled in kernel?)')
        self.ksymtab_version = k.u16(self.ksymtab + 4)
        self.ksymtab_count = k.u16(self.ksymtab + 6)

        self.ram_start = k.sym('__drvset_ram_start')
        self.ram_end = k.sym('__drvset_ram_end')

        # names of kernel functions by address
        self.kfunc = {}
        for name, addr in k.functions.items():
            self.kfunc.setdefault(addr & ~1, set()).add(name)

        # modules built into the kernel
        self.builtin = set()
        if k.has('_drvreg_module_table'):
            addr, size = k.symbols['_drvreg_module_table']
            for i in range(size // MODULE_ENTRY_SIZE):
                name = k.string(k.u32(addr + i * MODULE_ENTRY_SIZE) or 0)
                if name:
                    self.builtin.add(name)

        self.info(f'kernel: vectors 0x{self.vectors_addr:08X}, symbol table 0x{self.ksymtab:08X} '
                  f'(version {self.ksymtab_version}, {self.ksymtab_count} symbols), drvset RAM '
                  f'0x{self.ram_start:08X}-0x{self.ram_end:08X}, {len(self.builtin)} built-in modules')

    def kernel_info_posix(self):
        k = self.k
        self.vectors = []
        self.default_handler = None
        self.ksymtab = k.sym('_ksymtab')
        if k.u32(self.ksymtab) != KSYMTAB_MAGIC:
            raise ElfError(f'{k.path}: kernel symbol table not found (drvset disabled in kernel?)')
        self.ksymtab_version = k.u16(self.ksymtab + 4)
        self.ksymtab_count = k.u16(self.ksymtab + 6)
        self.kfunc = {}
        for name, addr in k.functions.items():
            self.kfunc.setdefault(addr, set()).add(name)
        self.builtin = set()
        if k.has('_drvreg_module_table'):
            addr, size = k.symbols['_drvreg_module_table']
            for i in range(size // MODULE_ENTRY_SIZE):
                name = k.string(k.u32(addr + i * MODULE_ENTRY_SIZE) or 0)
                if name:
                    self.builtin.add(name)
        self.ram_start = self.ram_end = 0  # set by check_header() (partition address)
        self.info(f'kernel (POSIX simulator): symbol table 0x{self.ksymtab:08X} (version {self.ksymtab_version}, '
                  f'{self.ksymtab_count} symbols), {len(self.builtin)} built-in modules')

    def kernel_filesystems(self):
        """File systems built into the kernel and size of file system entry (configuration dependent)."""
        k = self.k
        self.builtin_fs = set()
        self.fs_entry_size = None
        if k.has('_FS_table') and k.has('_FS_table_size'):
            addr, size = k.symbols['_FS_table']
            count = k.u32(k.sym('_FS_table_size')) or 0
            if count:
                self.fs_entry_size = size // count
                for i in range(count):
                    name = k.string(k.u32(addr + i * self.fs_entry_size) or 0)
                    if name:
                        self.builtin_fs.add(name)

    def check_filesystems(self, h):
        d = self.d
        count = h['filesystems_count']
        if not count:
            return
        size = self.fs_entry_size
        if size is None:
            self.error('file system entry size unknown (kernel without file systems?)')
            return
        if not self.in_flash(h['filesystems'], count * size):
            self.error('incorrect file system table')
            return
        names = []
        for i in range(count):
            entry = h['filesystems'] + i * size
            naddr = d.u32(entry)
            name = d.string(naddr) if naddr is not None and self.in_flash(naddr, 1) else None
            if name is None or d.u32(entry + size - 4) != FS_MAGIC:
                self.error(f'incorrect file system {i} (interface of other kernel configuration?)')
                continue
            if name in self.builtin_fs or name in names:
                self.error(f'file system {name} already exists')
            names.append(name)
            for j in range((size - 8) // 4):
                fn = d.u32(entry + 4 + 4 * j)
                if fn and not self.in_flash(fn & ~1, 2):
                    self.error(f'file system {name}: incorrect interface function {j}')
        self.info('file systems: ' + ', '.join(names))

    def check_header(self):
        d = self.d
        haddr = d.sym('_drvset_header')
        if self.flash is None:
            self.flash = haddr
        self.flash_end = self.flash + self.size if self.size else 0x100000000

        if self.posix:
            if not self.size or not self.ram:
                raise ElfError('POSIX simulator: partition size and RAM size are needed (--size, --ram)')
            self.ksymtab_slot = self.flash_end  # first word of RAM: symbol table address
            self.ram_start = self.flash_end + 8
            self.ram_end = self.flash_end + self.ram

        raw = d.read(self.flash, struct.calcsize(DRVSET_HEADER))
        if raw is None:
            self.error(f'no partition header at 0x{self.flash:08X}')
            return None
        h = dict(zip(DRVSET_FIELDS, struct.unpack(DRVSET_HEADER, raw)))
        if haddr != self.flash:
            self.error(f'partition header at 0x{haddr:08X}, partition starts at 0x{self.flash:08X}')

        if h['magic'] != DRVSET_MAGIC:
            self.error('incorrect magic number')
            return None
        if h['header_version'] != DRVSET_HEADER_VERSION:
            self.error(f'unsupported header version {h["header_version"]}')
            return None
        if h['ksymtab_version'] != self.ksymtab_version or h['ksymtab_count'] > self.ksymtab_count:
            self.error(f'incompatible kernel symbol table (drvset {h["ksymtab_version"]}/{h["ksymtab_count"]}, '
                       f'kernel {self.ksymtab_version}/{self.ksymtab_count})')
        if not self.in_flash(self.flash, h['image_size']):
            self.error(f'image size {h["image_size"]} exceeds partition')

        data_size = (h['data_end'] - h['data_start']) & 0xFFFFFFFF
        if (not self.in_ram(h['data_start'], h['data_end']) or not self.in_ram(h['bss_start'], h['bss_end'])
                or not self.in_flash(h['data_load'], data_size)):
            self.error(f'data outside of reserved memory (0x{self.ram_start:08X}-0x{self.ram_end:08X})')

        if self.posix:
            if h['ksymtab_vector'] != self.ksymtab_slot:
                self.error(f'linked for kernel at another address (symbol table slot 0x{h["ksymtab_vector"]:08X}, '
                           f'kernel 0x{self.ksymtab_slot:08X})')
        elif h['ksymtab_vector'] != self.vectors_addr + 4 * KVECTOR_KSYMTAB:
            self.error(f'linked for kernel at another address (vector 0x{h["ksymtab_vector"]:08X}, '
                       f'kernel 0x{self.vectors_addr + 4 * KVECTOR_KSYMTAB:08X})')

        self.info(f'drvset: header 0x{self.flash:08X}, image {h["image_size"]} B, RAM '
                  f'{data_size + h["bss_end"] - h["bss_start"]} B, {h["modules_count"]} modules, '
                  f'{h["irq_shared_count"]} shared interrupt handlers')
        return h

    def check_modules(self, h):
        d = self.d
        count = h['modules_count']
        if (self.max_modules is not None and count > self.max_modules) or \
                not self.in_flash(h['modules'], count * MODULE_ENTRY_SIZE):
            self.error(f'incorrect module table ({count} modules, max {self.max_modules})')
            return

        names = []
        for i in range(count):
            entry = h['modules'] + i * MODULE_ENTRY_SIZE
            naddr = d.u32(entry)
            name = d.string(naddr) if naddr is not None and self.in_flash(naddr, 1) else None
            if name is None:
                self.error(f'incorrect name of module {i}')
                continue
            if name in self.builtin or name in names:
                self.error(f'module {name} already exists')
            names.append(name)

            for j in range(9):
                fn = d.u32(entry + 8 + 4 * j)
                if not fn or not self.in_flash(fn & ~1, 2):
                    self.error(f'module {name}: incorrect interface function {j}')
        self.info('modules: ' + ', '.join(names))

    def check_irq(self, irq, handler, what):
        if not handler:
            return
        if not self.in_flash(handler & ~1, 2):
            self.error(f'incorrect {what} handler of IRQ {irq}')
        elif self.vectors[KVECTOR_IRQ0 + irq] & ~1 != self.default_handler:
            owner = self.kfunc.get(self.vectors[KVECTOR_IRQ0 + irq] & ~1, {'not existing in CPU'})
            self.error(f'IRQ {irq} is not available ({what} handler, kernel: {", ".join(sorted(owner))})')
        else:
            self.info(f'IRQ {irq}: {what} handler 0x{handler:08X}')

    def check_interrupts(self, h):
        d = self.d
        if self.posix:
            if h['irq_count'] or h['irq_shared_count']:
                self.error('interrupts are not supported by simulator')
            return
        if h['irq_count'] > IRQ_MAX or (h['irq_count'] and not self.in_flash(h['irq_table'], 4 * h['irq_count'])):
            self.error('incorrect interrupt table')
        else:
            for irq in range(h['irq_count']):
                self.check_irq(irq, d.u32(h['irq_table'] + 4 * irq), 'exclusive')

        count = h['irq_shared_count']
        if count and not self.in_flash(h['irq_shared'], count * IRQ_SHARED_SIZE):
            self.error('incorrect shared interrupt table')
            return
        for i in range(count):
            irq = d.u32(h['irq_shared'] + i * IRQ_SHARED_SIZE)
            handler = d.u32(h['irq_shared'] + i * IRQ_SHARED_SIZE + 4)
            if irq >= IRQ_MAX or not handler:
                self.error(f'incorrect shared handler of IRQ {irq}')
            else:
                self.check_irq(irq, handler, 'shared')

    def stub_index(self, addr):
        """Returns symbol table index used by stub at addr (ksymstub.S) or None."""
        d = self.d
        if self.posix:
            # i386: movl __ksymtab_vector, %eax; jmp *off(%eax) (disp8 or disp32)
            code = d.read(addr, 11)
            if not code or code[0] != 0xA1 or code[5] != 0xFF:
                return None
            if code[6] == 0x60:
                off = code[7]
            elif code[6] == 0xA0:
                off = struct.unpack_from('<I', code, 7)[0]
            else:
                return None
            return (off - KSYMTAB_SYM_OFFSET) // 4
        hw = [d.u16(addr + 2 * i) for i in range(4)]
        if None in hw:
            return None
        # Cortex-M3/M4/M7: ldr.w r12, [pc, #x]; ldr.w r12, [r12]; ldr.w r12, [r12, #off]; bx r12
        if hw[0] == 0xF8DF and (hw[1] & 0xF000) == 0xC000 and d.u16(addr + 8) == 0xF8DC:
            off = d.u16(addr + 10) & 0xFFF
            return (off - KSYMTAB_SYM_OFFSET) // 4
        # Cortex-M0+: push {r0, r1, r2}; ... offset in second literal word
        if hw[0] == 0xB407:
            off = d.u32(addr + 20)
            return None if off is None else (off - KSYMTAB_SYM_OFFSET) // 4
        return None

    def check_stubs(self, h):
        d = self.d
        stubs = 0
        for name, addr in sorted(d.functions.items()):
            idx = self.stub_index(addr & ~1)
            if idx is None:
                continue
            stubs += 1
            if idx >= h['ksymtab_count'] or idx >= self.ksymtab_count:
                self.error(f'stub {name}: symbol index {idx} out of kernel symbol table')
                continue
            entry = self.k.u32(self.ksymtab + KSYMTAB_SYM_OFFSET + 4 * idx)
            knames = self.kfunc.get((entry or 0) & (~0 if self.posix else ~1), set())
            if KSYM_ALIASES.get(name, name) not in knames and name not in knames:
                self.error(f'stub {name}: symbol {idx} is {", ".join(sorted(knames)) or f"0x{entry:08X}"} in kernel')
            else:
                self.info(f'stub {name}: symbol {idx}')
        self.info(f'{stubs} kernel functions used')

    def run(self):
        self.kernel_info()
        h = self.check_header()
        if h:
            self.check_modules(h)
            self.kernel_filesystems()
            self.check_filesystems(h)
            self.check_interrupts(h)
            self.check_stubs(h)
        return not self.errors


def kernel_info(path):
    c = Checker(Elf32(path), None, None, None, None, False)
    c.kernel_info()
    irqs = [i - KVECTOR_IRQ0 for i in range(KVECTOR_IRQ0, len(c.vectors)) if c.vectors[i]]
    print('free_irqs=' + ','.join(str(i) for i in irqs if c.vectors[KVECTOR_IRQ0 + i] & ~1 == c.default_handler))
    print('used_irqs=' + ','.join(str(i) for i in irqs if c.vectors[KVECTOR_IRQ0 + i] & ~1 != c.default_handler
                                  and i < KVECTOR_KSYMTAB - KVECTOR_IRQ0))
    print('modules=' + ','.join(sorted(c.builtin)))
    c.kernel_filesystems()
    print('filesystems=' + ','.join(sorted(c.builtin_fs)))


def main():
    if len(sys.argv) == 3 and sys.argv[1] == '--kernel-info':
        try:
            kernel_info(sys.argv[2])
        except (ElfError, OSError) as e:
            print(f'drvset check: {e}')
            return 2
        return 0

    ap = argparse.ArgumentParser(description='Offline check of drvset partition image against kernel image.')
    ap.add_argument('kernel', help='kernel image (ELF)')
    ap.add_argument('drvset', help='drvset partition image (ELF)')
    ap.add_argument('--flash', type=lambda x: int(x, 0), help='partition address')
    ap.add_argument('--size', type=lambda x: int(x, 0), help='partition size in bytes')
    ap.add_argument('--ram', type=lambda x: int(x, 0), help='partition RAM size in bytes (POSIX simulator)')
    ap.add_argument('--max-modules', type=int, help='maximum number of modules')
    ap.add_argument('-v', '--verbose', action='store_true', help='print details')
    args = ap.parse_args()

    try:
        checker = Checker(Elf32(args.kernel), Elf32(args.drvset), args.flash, args.size, args.max_modules,
                          args.verbose, args.ram)
        ok = checker.run()
    except (ElfError, OSError) as e:
        print(f'drvset check: {e}')
        return 2

    for e in checker.errors:
        print(f'drvset check: ERROR: {e}')
    print(f'drvset check: {"OK" if ok else "FAILED"} ({args.drvset})')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
