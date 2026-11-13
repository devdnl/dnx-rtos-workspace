#!/usr/bin/env python3
# ==================================================================================================
# @file   simtest.py
#
# @author Daniel Zorychta
#
# @brief  Automatic tests of dnx RTOS started in the POSIX simulator (no hardware needed).
#
#         The simulator is started in the pseudo terminal, test programs are executed in the dsh
#         shell and their SUMMARY lines are checked. Tests:
#           - boot (shell available, simulator platform),
#           - driver configuration strings (sim-env/tests/devcfg.lua, modprobe -c), UART:1 is
#             connected to the host pseudo terminal and its configuration is checked by termios,
#           - board model (sim-env/boards/test.lua): GPIO, I2C, I2CEE/eefs, DS2482/DS18B20 (1-Wire),
#             SPI, SPIEE/eefs, MCP23S17 (dnx.spi), UART (echo, NMEA GPS), Ethernet (netdev models: ping,
#             UDP, TCP by nc program) and SDSPI/ext2 tests (sim-env/tests/*.lua) executed by dnx Lua
#             interpreter, fstest on ext2
#             on SD card model (SDSPI driver), board panel protocol (socket of tools/simpanel.py),
#           - fstest on ramfs (/tmp) and hostfs (/host, temporary host directory),
#           - fstest on FAT and ext4 partitions of temporary disk image (/mnt/sda1, /mnt/sda2),
#             image is checked by fsck.fat and e2fsck after test (requires dosfstools and
#             e2fsprogs, otherwise tests are skipped),
#           - fstest on flatfs, eefs, ext2 and ext3 (journal) partitions of the second disk
#             (formatted by mkflatfs, mkeefs and mkext programs), ext2 and ext3 are checked by
#             e2fsck after test (requires e2fsprogs, otherwise check is skipped),
#           - other appset linked with simulator (sim-env/testappset mounted by siminit in
#             /opt/testappset by appsetfs, program appsettest: libc, threads, child process,
#             search order of programs, start by path and program search path, files of
#             appsetfs, umount of used appset), mount/umount of the appset from shell,
#           - drvset partition (drivers built separately from the kernel, option --drvset of
#             simulator, image of tools/drvset/test/sim built by make build-sim-drvset): module
#             registration, driver initialization by modprobe, read, release, fstest on file
#             system dsramfs of the partition, partition with incorrect header (started in
#             separate simulator), skipped without image,
#           - block layer (sim-env/tests/blkdev.lua): unaligned requests and bounce buffers on host disk
#             with emulated constraints (block size, buffer alignment), fstest on ext2 on that disk,
#             image is checked by e2fsck,
#           - fstest -r (read tests and read-only checks) on romfs image with reference data
#             created by fstest_ref.py and mkfpkg.py,
#           - udstest (UNIX domain sockets),
#           - perf stress tests (threads, heap, synchronization objects, processes), with --perf
#             option also performance measurements of perf program,
#           - inettest (TCP/IP, only with --tap option, TAP interface must be prepared),
#           - no application crash and kernel panic.
#
#         Option --iostat prints block device statistics (iostat) of file system tests.
#
#         Usage:
#           tools/simtest.py [--sim build/dnx-sim] [--rom build/sim-romfs.bin] [--tap tap0]
#                            [--drvset build/sim-drvset.bin|none]
#                            [--board sim-env/boards/test.lua|none] [--perf] [--iostat]
#                            [--log build/simtest.log] [-v] [filter]
#
#         Exit status: 0 if all executed tests passed, otherwise 1.
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
import base64
import json
import os
import pty
import re
import select
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import termios
import threading
import time

# --------------------------------------------------------------------------------------------------
# constants
# --------------------------------------------------------------------------------------------------
BOOT_TIMEOUT = 30           # seconds
NETWORK_TIMEOUT = 20        # seconds
DEFAULT_TIMEOUT = 60        # seconds
FSTEST_TIMEOUT = 300        # seconds
SDSPI_FSTEST_TIMEOUT = 600  # seconds (SD card model over SPI, about 2.5 minutes)
NETTEST_TIMEOUT = 600       # seconds
PERF_TIMEOUT = 300          # seconds
QUIET_TIME = 0.3            # seconds without output after prompt

SECTOR_SIZE = 512
DISK_SIZE = 64 * 1024 * 1024
PART1_START = 2048          # sectors
PART1_SIZE = 32 * 1024 * 1024 // SECTOR_SIZE
PART2_START = PART1_START + PART1_SIZE
PART2_SIZE = DISK_SIZE // SECTOR_SIZE - PART2_START

# second disk (formatted by simulator): partition 1 flatfs, partition 2 eefs, partition 3 ext2,
# partition 4 ext3 (mkext program)
RAW_DISK_SIZE = 50 * 1024 * 1024
FLATFS_START = 2048         # sectors
FLATFS_SIZE = 16 * 1024 * 1024 // SECTOR_SIZE
FLATFS_MAX_FILE_KIB = 64    # flatfs file slot size
EEFS_START = FLATFS_START + FLATFS_SIZE
EEFS_SIZE = 1024 * 1024 // SECTOR_SIZE
EXT2_START = EEFS_START + EEFS_SIZE
EXT2_SIZE = 16 * 1024 * 1024 // SECTOR_SIZE
EXT3_START = EXT2_START + EXT2_SIZE
EXT3_SIZE = RAW_DISK_SIZE // SECTOR_SIZE - EXT3_START

# block layer test disk (HOSTDISK with emulated constraints, ext2 created by mkext)
BLK_DISK = 4
BLK_DISK_SIZE = 16 * 1024 * 1024

# asynchronous messages: board engine (models) and netmgr daemon (network configuration)
ASYNC_MESSAGE = re.compile(r"(\[board\] |NM: )[^\n]*\n+")
ANSI_ESCAPE = re.compile(rb"\x1b\[[0-9;?]*[a-zA-Z]|\x1b[78]")
ANSI_INCOMPLETE = re.compile(rb"\x1b(\[[0-9;?]*)?$")
SUMMARY = re.compile(r"SUMMARY:\s*executed:\s*(\d+),\s*passed:\s*(\d+),\s*failed:\s*(\d+),\s*skipped:\s*(\d+)")
CRASH = re.compile(r"APP CRASH|KERNEL PANIC")

PASS = "PASS"
FAIL = "FAIL"
SKIP = "SKIP"


# --------------------------------------------------------------------------------------------------
# simulator connected to the pseudo terminal
# --------------------------------------------------------------------------------------------------
class Simulator:
    def __init__(self, argv, log, verbose):
        self.log = log
        self.verbose = verbose
        self.out = ""
        self.pending = b""
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            try:
                os.execv(argv[0], argv)
            finally:
                os._exit(127)

    # read output by selected time or until condition is met
    def read(self, timeout, until=None):
        end = time.time() + timeout
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    data = os.read(self.fd, 65536)
                except OSError:
                    return False

                # escape sequence can be split between reads
                data = self.pending + data
                m = ANSI_INCOMPLETE.search(data)
                self.pending = data[m.start():] if m else b""
                data = data[:m.start()] if m else data

                text = ANSI_ESCAPE.sub(b"", data).replace(b"\r", b"").decode(errors="replace")
                self.out += text
                self.log.write(text)
                self.log.flush()
                if self.verbose:
                    sys.stdout.write(text)
                    sys.stdout.flush()

            elif until and until():
                return True

        return bool(until and until())

    def is_alive(self):
        try:
            pid, _ = os.waitpid(self.pid, os.WNOHANG)
            return pid == 0
        except ChildProcessError:
            return False

    def _prompt_ready(self, start):
        # asynchronous messages of board engine (models) and netmgr can follow the prompt
        # (the terminal shows the prompt again before message of other process)
        out = re.sub(r"(\$ )+$", "$ ", ASYNC_MESSAGE.sub("", self.out[start:]))
        return out.endswith("\n$ ")

    # wait for shell prompt (prompt is the last output)
    def wait_prompt(self, timeout, start=0):
        end = time.time() + timeout
        while time.time() < end:
            if not self.read(min(1.0, max(0.0, end - time.time())), lambda: self._prompt_ready(start)):
                if not self.is_alive():
                    return False
                continue

            # prompt must be the last output (no asynchronous messages)
            n = len(self.out)
            self.read(QUIET_TIME)
            if (n == len(self.out)) or self._prompt_ready(start):
                return True

        return False

    # execute command and return its output
    def run(self, cmd, timeout=DEFAULT_TIMEOUT):
        start = len(self.out)
        os.write(self.fd, cmd.encode() + b"\r")
        ok = self.wait_prompt(timeout, start)
        return ok, self.out[start:]

    def stop(self):
        if self.is_alive():
            try:
                os.write(self.fd, b"\x1c")  # Ctrl+\ terminates simulator
            except OSError:
                pass
            self.read(2, lambda: not self.is_alive())

        if self.is_alive():
            os.kill(self.pid, signal.SIGKILL)

        try:
            os.waitpid(self.pid, 0)
        except ChildProcessError:
            pass

        try:
            os.close(self.fd)
        except OSError:
            pass


# --------------------------------------------------------------------------------------------------
# disk image with MBR: partition 1 FAT (32 MiB), partition 2 ext4 (rest of disk)
# --------------------------------------------------------------------------------------------------
def find_tool(*names):
    for name in names:
        path = shutil.which(name) or shutil.which(name, path="/usr/sbin:/sbin")
        if path:
            return path
    return None


def mbr_entry(ptype, start, size):
    # CHS fields are not used (LBA addressing)
    return struct.pack("<B3sB3sII", 0x00, b"\xfe\xff\xff", ptype, b"\xfe\xff\xff", start, size)


def create_disk_image(path, mkfs_fat, mkfs_ext4):
    with open(path, "wb") as f:
        f.truncate(DISK_SIZE)
        mbr = bytearray(SECTOR_SIZE)
        mbr[446:462] = mbr_entry(0x06, PART1_START, PART1_SIZE)  # FAT16
        mbr[462:478] = mbr_entry(0x83, PART2_START, PART2_SIZE)  # Linux
        mbr[510:512] = b"\x55\xaa"
        f.write(mbr)

    run = lambda args: subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, check=True)
    run([mkfs_fat, "-F", "16", "-n", "DNXTEST", "--offset", str(PART1_START), path, str(PART1_SIZE // 2)])
    # metadata_csum_seed feature is tested if it is known by e2fsprogs (old versions)
    ext4 = [mkfs_ext4, "-q", "-F", "-L", "dnxtest", "-E", "offset=%u" % (PART2_START * SECTOR_SIZE)]
    size = [path, "%uk" % (PART2_SIZE // 2)]
    try:
        run(ext4 + ["-O", "metadata_csum_seed"] + size)
    except subprocess.CalledProcessError:
        run(ext4 + size)


def create_raw_disk_image(path):
    with open(path, "wb") as f:
        f.truncate(RAW_DISK_SIZE)
        mbr = bytearray(SECTOR_SIZE)
        mbr[446:462] = mbr_entry(0xda, FLATFS_START, FLATFS_SIZE)  # non-FS data
        mbr[462:478] = mbr_entry(0xda, EEFS_START, EEFS_SIZE)
        mbr[478:494] = mbr_entry(0x83, EXT2_START, EXT2_SIZE)     # Linux
        mbr[494:510] = mbr_entry(0x83, EXT3_START, EXT3_SIZE)
        mbr[510:512] = b"\x55\xaa"
        f.write(mbr)


def create_romfs_image(path, root, tmpdir):
    content = os.path.join(tmpdir, "romfs")
    os.mkdir(content)
    run = lambda args: subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, check=True)
    run([sys.executable, os.path.join(root, "common_apps", "fstest", "fstest_ref.py"), content])
    run([sys.executable, os.path.join(root, "kernel", "tools", "mkfpkg.py"), content, path])


def extract_partition(image, start, size, path):
    with open(image, "rb") as src, open(path, "wb") as dst:
        src.seek(start * SECTOR_SIZE)
        dst.write(src.read(size * SECTOR_SIZE))


def check_partition(image, start, size, tmpdir, cmd):
    part = os.path.join(tmpdir, "part.img")
    extract_partition(image, start, size, part)
    r = subprocess.run(cmd + [part], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    os.remove(part)
    return r.returncode == 0, r.stdout


# --------------------------------------------------------------------------------------------------
# tests
# --------------------------------------------------------------------------------------------------
class TestRunner:
    def __init__(self, args):
        self.args = args
        self.results = []

    def result(self, name, status, info=""):
        self.results.append((name, status, info))
        color = {PASS: "\x1b[32m", FAIL: "\x1b[31m", SKIP: "\x1b[33m"}[status] if sys.stdout.isatty() else ""
        reset = "\x1b[0m" if color else ""
        print("%-28s %s%s%s %s" % (name, color, status, reset, info))
        sys.stdout.flush()

    def selected(self, name):
        return (not self.args.filter) or (self.args.filter.lower() in name.lower())

    # run program which prints SUMMARY line, setup commands are executed before (e.g. format, mount)
    def appset_test(self, sim):
        name = "appsets: testappset"
        if not self.selected(name):
            return

        ok, out = sim.run("appsettest", DEFAULT_TIMEOUT)
        if ok and ("is unknown command" in out):
            self.result(name, SKIP, "simulator built without sim-env/testappset (make test-sim or SIM_APPSETS)")
            return

        self.summary_test(sim, name, None, "appsettest", DEFAULT_TIMEOUT)
        self.appsetfs_mount_test(sim)

    # appsetfs: unmount of appset that is not used, start of its program fails, mount again,
    # second mount of the same appset is refused (EBUSY)
    def appsetfs_mount_test(self, sim):
        name = "appsetfs: mount/umount"
        if not self.selected(name):
            return

        steps = (
            ("umount /opt/testappset", lambda out: ("rror" not in out) and ("No such" not in out)),
            ("/opt/testappset/echo", lambda out: "is unknown command" in out),
            ("mount /dev/appset0 appsetfs /opt/testappset", lambda out: "rror" not in out),
            ("mkdir /tmp/appset2", lambda out: True),
            ("mount /dev/appset0 appsetfs /tmp/appset2", lambda out: "busy" in out),
            ("/opt/testappset/echo", lambda out: "testappset" in out.replace("/opt/testappset/echo", "")),
        )

        for cmd, check in steps:
            ok, out = sim.run(cmd, DEFAULT_TIMEOUT)
            if not ok or not check(out):
                lines = [l for l in out.strip().splitlines() if l.strip()]
                self.result(name, FAIL, "'%s': %s" % (cmd, lines[-2] if len(lines) > 1 else "no output"))
                return

        self.result(name, PASS, "")

    # drvset partition (example driver): registration at boot, initialization, read and release
    def drvset_test(self, sim):
        name = "drvset: example driver"
        if not self.selected(name):
            return

        if not self.args.drvset:
            self.result(name, SKIP, "drvset image not found (make build-sim-drvset, --drvset)")
            return

        if "drvset: 1 modules, 1 file systems registered" not in sim.out:
            self.result(name, FAIL, "partition not registered at boot (see log: %s)" % self.args.log)
            return

        steps = (("modprobe -i EXAMPLE 0 0 /dev/example", "initialized"),
                 ("cat /dev/example", "irq: 0"),
                 ("modprobe -R /dev/example", "released"))
        for cmd, expected in steps:
            ok, out = sim.run(cmd)
            if not ok or expected not in out:
                self.result(name, FAIL, "'%s': '%s' expected" % (cmd, expected))
                return

        self.result(name, PASS, "registered, initialized, read, released")

    # file system of drvset partition (ramfs of the kernel built into the partition as dsramfs)
    def drvset_fs_test(self, sim):
        name = "fstest: dsramfs (drvset)"
        if not self.selected(name):
            return

        if not self.args.drvset:
            self.result(name, SKIP, "drvset image not found (make build-sim-drvset, --drvset)")
            return

        self.summary_test(sim, name, "/mnt/dsfs", "fstest", FSTEST_TIMEOUT,
                          ("mkdir /mnt/dsfs", "mount - dsramfs /mnt/dsfs"))

    # partition with incorrect header is rejected and system starts (separate simulator)
    def drvset_bad_image_test(self, log, tmpdir):
        name = "drvset: incorrect image"
        if not self.selected(name):
            return

        if not self.args.drvset:
            self.result(name, SKIP, "drvset image not found (make build-sim-drvset, --drvset)")
            return

        with open(self.args.drvset, "rb") as f:
            image = bytearray(f.read())
        struct.pack_into("<H", image, 4, 0xBAD)  # header version
        bad = os.path.join(tmpdir, "drvset-bad.bin")
        with open(bad, "wb") as f:
            f.write(image)

        argv = [self.args.sim, "--disk", "0=%s:ro" % self.args.rom, "--drvset", bad]
        log.write("\n$ %s\n" % " ".join(argv))
        sim = Simulator(argv, log, self.args.verbose)
        try:
            booted = sim.read(BOOT_TIMEOUT, lambda: "\n$ " in sim.out)
            sim.read(1)
            ok, out = sim.run("modprobe -i EXAMPLE 0 0 /dev/example") if booted else (False, "")
            if not booted:
                self.result(name, FAIL, "system does not start (see log: %s)" % self.args.log)
            elif "drvset: unsupported header version" not in sim.out:
                self.result(name, FAIL, "partition error not reported")
            elif not ok or "initialized" in out:
                self.result(name, FAIL, "module of rejected partition initialized")
            else:
                self.result(name, PASS, "partition rejected, system started")

            if CRASH.search(sim.out):
                self.result(name + " (crash)", FAIL, "see log: %s" % self.args.log)
        finally:
            sim.stop()

    def summary_test(self, sim, name, cwd, cmd, timeout, setup=(), device=None):
        if not self.selected(name):
            return

        for c in setup:
            ok, out = sim.run(c, timeout)
            if not ok or ("rror" in out) or ("Usage" in out) or ("No such" in out):
                lines = out.strip().splitlines()
                self.result(name, FAIL, "'%s' failed: %s" % (c, lines[-2] if len(lines) > 1 else ""))
                return

        if cwd:
            ok, out = sim.run("cd " + cwd)
            if not ok or ("No such" in out) or ("rror" in out):
                self.result(name, FAIL, "cannot enter %s" % cwd)
                return

        # block device statistics of the test only (after setup)
        iostat = self.args.iostat and device
        if iostat:
            sim.run("iostat -e " + device)

        t = time.time()
        ok, out = sim.run(cmd, timeout)
        dt = time.time() - t

        if cwd:
            sim.run("cd /")

        m = SUMMARY.search(out)
        if not ok:
            self.result(name, FAIL, "timeout or simulator terminated")
        elif CRASH.search(out):
            self.result(name, FAIL, "program crashed")
        elif not m:
            self.result(name, FAIL, "no SUMMARY line")
        else:
            executed, passed, failed, skipped = (int(x) for x in m.groups())
            info = "executed: %u, passed: %u, failed: %u, skipped: %u (%.1f s)" % (
                executed, passed, failed, skipped, dt)
            self.result(name, PASS if (failed == 0 and executed > 0) else FAIL, info)

        if iostat:
            _, stat = sim.run("iostat " + device)
            sim.run("iostat -d " + device)
            for line in stat.splitlines()[1:]:
                if line.startswith(device) or line.startswith("  "):
                    print("    " + line.rstrip())

        # performance measurements (results of the PERFORMANCE section)
        if self.args.perf and ok:
            for line in out.splitlines():
                if line.startswith("  ") and ((" KiB/s" in line) or (" us " in line) or ("/s" in line)):
                    print("    " + line.strip())

    # board panel: the test connects to the panel socket like tools/simpanel.py, signals of board
    # boards/test.lua: P_OUT shows PA18, P_IN drives PA19, P_CTL sets P_VAL = 2 * P_CTL; LCD of LTDC
    # driver (frame sent to the panel) and touch of LCD read from FT5336 model by I2C driver; LCD "tft"
    # of ST7789 model (SPI:3) and its touch (P_TOUCH)
    def panel_test(self, sim):
        name = "board: panel"
        if not self.selected(name):
            return

        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            s.settimeout(5)
            s.connect(self.args.panel_socket)
        except OSError as e:
            self.result(name, FAIL, "cannot connect to panel socket: %s" % e)
            return

        state, signals, lcds = {}, [], {}
        lock = threading.Lock()

        # messages are read all the time (the simulator closes connection of panel that does not read)
        def reader():
            buf = bytearray()
            s.settimeout(None)
            while True:
                try:
                    data = s.recv(262144)
                except OSError:
                    return
                if not data:
                    return
                buf += data
                while b"\n" in buf:
                    end = buf.index(b"\n")
                    msg = json.loads(bytes(buf[:end]))
                    del buf[:end + 1]
                    with lock:
                        if msg.get("t") == "lcd":
                            lcd_update(msg)
                        else:
                            if msg.get("t") == "init":
                                signals[:] = [x["id"] for x in msg.get("signals", [])]
                                state.clear()
                            state.update(msg.get("state", {}))

        def wait(cond, timeout=3):
            end = time.time() + timeout
            while time.time() < end:
                with lock:
                    if cond():
                        return True
                time.sleep(0.02)
            with lock:
                return cond()

        threading.Thread(target=reader, daemon=True).start()

        # whole LCD frames (id -> {geometry, frame}) assembled from changed lines
        def lcd_update(msg):
            lcd_id = msg.get("id")
            if not msg.get("w"):
                lcds.pop(lcd_id, None)
                return
            lcd = lcds.setdefault(lcd_id, {})
            line = msg["w"] * 2 if msg.get("fmt") == "rgb565" else 0
            if lcd.get("geometry") != (msg["w"], msg["h"], msg.get("fmt")):
                lcd["geometry"] = (msg["w"], msg["h"], msg.get("fmt"))
                lcd["frame"] = bytearray(line * msg["h"])
            data = base64.b64decode(msg.get("data", ""))
            lcd["frame"][msg["y"] * line:msg["y"] * line + len(data)] = data

        def lcd_pixel(x, y, lcd_id="ltdc"):
            lcd = lcds.get(lcd_id, {})
            frame = lcd.get("frame")
            w = lcd["geometry"][0] if frame else 0
            return frame[(y * w + x) * 2] | (frame[(y * w + x) * 2 + 1] << 8) if frame else None

        def lcd_geometry(lcd_id="ltdc"):
            return lcds.get(lcd_id, {}).get("geometry")

        def touch_state():
            ok, out = sim.run("lua lcd.lua touch")
            m = re.search(r"TOUCH n=(\d+) x=(\d+) y=(\d+) ev=(\d) int=(\d) id=0x51", out)
            return tuple(int(v) for v in m.groups()) if (ok and m) else None

        def dnx(level):
            ok, out = sim.run("lua panel.lua %s" % level)
            m = re.search(r"PA19=(\d)", out)
            return m.group(1) if (ok and m) else None

        errors = []
        try:
            if not wait(lambda: {"P_OUT", "P_IN", "P_VAL", "P_CTL"} <= set(signals)):
                errors.append("init message without signals of test board")
            else:
                sim.run("cd /host/sim-env")

                if dnx(1) != "0" or not wait(lambda: state.get("P_OUT") is True):
                    errors.append("P_OUT does not show PA18 = 1")

                s.sendall(b"set P_IN b1\n")
                if not wait(lambda: state.get("P_IN") is True) or dnx(0) != "1":
                    errors.append("P_IN does not drive PA19")
                if not wait(lambda: state.get("P_OUT") is False):
                    errors.append("P_OUT does not show PA18 = 0")

                s.sendall(b"set P_CTL n21\n")
                if not wait(lambda: state.get("P_VAL") == 42):
                    errors.append("P_CTL handler not called (P_VAL = %r)" % state.get("P_VAL"))

                s.sendall(b"set P_IN b0\n")
                if not wait(lambda: state.get("P_IN") is False) or dnx("") != "0":
                    errors.append("P_IN does not release PA19")

                # LCD (LTDC driver): line 10 is red, touch is read from FT5336 by I2C driver
                ok, out = sim.run("lua lcd.lua init")
                if "LCD=261120" not in out:
                    errors.append("LTDC device not created")
                elif not wait(lambda: lcd_pixel(5, 10) == 0xF800 and lcd_pixel(5, 11) == 0, 5):
                    errors.append("LCD frame not received (geometry %r)" % (lcd_geometry(),))
                elif lcd_geometry() != (480, 272, "rgb565"):
                    errors.append("LCD geometry %r" % (lcd_geometry(),))
                else:
                    s.sendall(b"touch 123 45 1 ltdc\n")
                    time.sleep(0.1)
                    st = touch_state()
                    if not st or st[:3] != (1, 123, 45) or st[4] != 0:
                        errors.append("touch not reported by FT5336: %r" % (st,))
                    s.sendall(b"touch 123 45 0 ltdc\n")
                    time.sleep(0.1)
                    st = touch_state()
                    if not st or st[0] != 0 or st[3] != 1 or st[4] != 1:
                        errors.append("release not reported by FT5336: %r" % (st,))

                # framebuffer memory is needed by next tests
                ok, out = sim.run("lua lcd.lua release")
                if "RELEASED" not in out:
                    errors.append("LTDC device not released")
                elif not wait(lambda: "ltdc" not in lcds, 3):
                    errors.append("LCD not detached from panel after release")

                # LCD "tft" of ST7789 model (SPI:3), shown after SLPOUT and DISPON
                if "tft" in lcds:
                    errors.append("LCD tft shown before initialization")
                ok, out = sim.run("lua spilcd.lua init")
                if "ID=858552 PM=0x9C" not in out:
                    errors.append("ST7789 init: %r" % out.strip())
                elif not wait(lambda: lcd_geometry("tft") == (240, 320, "rgb565"), 5):
                    errors.append("LCD tft not shown (geometry %r)" % (lcd_geometry("tft"),))
                else:
                    ok, out = sim.run("lua spilcd.lua draw")
                    px = lambda: (lcd_pixel(5, 10, "tft"), lcd_pixel(5, 11, "tft"), lcd_pixel(5, 20, "tft"),
                                  lcd_pixel(6, 21, "tft"), lcd_pixel(7, 20, "tft"), lcd_pixel(7, 30, "tft"))
                    if "DRAWN" not in out:
                        errors.append("ST7789 draw: %r" % out.strip())
                    elif not wait(lambda: px() == (0xF800, 0, 0x001F, 0x001F, 0, 0x07E0), 5):
                        errors.append("LCD tft pixels %r" % (px(),))

                    # touch of LCD tft is not passed to FT5336 (handler of LCD "ltdc")
                    s.sendall(b"touch 12 300 1 tft\n")
                    if not wait(lambda: state.get("P_TOUCH") == "12 300 1 tft"):
                        errors.append("touch of LCD tft: %r" % state.get("P_TOUCH"))
                    st = touch_state()
                    if not st or st[0] != 0:
                        errors.append("touch of LCD tft reported by FT5336: %r" % (st,))
                    s.sendall(b"touch 12 300 0 tft\n")
                    if not wait(lambda: state.get("P_TOUCH") == "12 300 0 tft"):
                        errors.append("release of LCD tft: %r" % state.get("P_TOUCH"))

                    ok, out = sim.run("lua spilcd.lua off")
                    if "OFF" not in out or not wait(lambda: "tft" not in lcds, 3):
                        errors.append("LCD tft shown after DISPOFF")

                sim.run("cd /")
        except (OSError, ValueError) as e:
            errors.append(str(e))
        finally:
            s.close()

        self.result(name, FAIL if errors else PASS, ", ".join(errors))

    # UART:1 configured by devcfg.lua: baud=9600,parity=even,stop_bits=2 (parity is not checked:
    # Linux pseudo terminal does not keep the PARENB flag)
    def check_uart_config(self):
        attr = termios.tcgetattr(self.uart_slave)
        cflag, ispeed = attr[2], attr[4]
        errors = []
        if ispeed != termios.B9600:
            errors.append("baud rate is not 9600")
        if not (cflag & termios.CSTOPB):
            errors.append("stop bits are not 2")
        self.result("devcfg: uart termios", FAIL if errors else PASS, ", ".join(errors))

    def execute(self):
        args = self.args

        mkfs_fat = find_tool("mkfs.fat", "mkfs.vfat")
        mkfs_ext4 = find_tool("mkfs.ext4")
        fsck_fat = find_tool("fsck.fat", "fsck.vfat")
        e2fsck = find_tool("e2fsck")

        tmpdir = tempfile.mkdtemp(prefix="dnx-simtest-")
        hostfs_dir = os.path.join(tmpdir, "hostfs")
        os.mkdir(hostfs_dir)
        args.hostfs_dir = hostfs_dir

        disk = None
        if mkfs_fat and mkfs_ext4:
            disk = os.path.join(tmpdir, "sd.img")
            try:
                create_disk_image(disk, mkfs_fat, mkfs_ext4)
            except subprocess.CalledProcessError as e:
                print("Disk image error: %s" % e.stderr.decode(errors="replace").strip())
                disk = None

        raw_disk = os.path.join(tmpdir, "raw.img")
        create_raw_disk_image(raw_disk)

        blk_disk = os.path.join(tmpdir, "blk.img")
        with open(blk_disk, "wb") as f:
            f.truncate(BLK_DISK_SIZE)

        romfs = os.path.join(tmpdir, "romfs.img")
        try:
            create_romfs_image(romfs, args.root, tmpdir)
        except (subprocess.CalledProcessError, OSError) as e:
            print("ROMFS image error: %s" % e)
            romfs = None

        argv = [args.sim, "--disk", "0=%s:ro" % args.rom, "--hostfs", hostfs_dir]
        if args.drvset:
            argv += ["--drvset", args.drvset]
        if disk:
            argv += ["--disk", "1=%s" % disk]
        argv += ["--disk", "2=%s" % raw_disk]
        argv += ["--disk", "%u=%s" % (BLK_DISK, blk_disk)]
        if romfs:
            argv += ["--disk", "3=%s:ro" % romfs]
        if args.tap:
            argv += ["--tap", args.tap]
        # test programs (Lua) are available in the /host/sim-env directory
        shutil.copytree(os.path.join(args.root, "sim-env", "tests"), os.path.join(hostfs_dir, "sim-env"))
        if args.board:
            args.panel_socket = os.path.join(tmpdir, "panel.sock")
            argv += ["--board", args.board, "--panel", args.panel_socket]

        # UART:1 is connected to the host pseudo terminal as serial device (configuration test)
        self.uart_master, self.uart_slave = os.openpty()
        argv += ["--uart", "1=%s" % os.ttyname(self.uart_slave)]

        os.makedirs(os.path.dirname(os.path.abspath(args.log)), exist_ok=True)
        with open(args.log, "w", buffering=1) as log:
            log.write("$ %s\n" % " ".join(argv))

            sim = Simulator(argv, log, args.verbose)
            try:
                self.tests(sim, disk, romfs)
            finally:
                if sim.is_alive():
                    sim.run("cd /", 5)
                    if disk:
                        sim.run("umount /mnt/sda1", 10)
                        sim.run("umount /mnt/sda2", 10)
                    for mnt in ("/mnt/i2cee", "/mnt/spiee", "/mnt/sdcard", "/mnt/flatfs", "/mnt/eefs", "/mnt/ext2",
                                "/mnt/ext3", "/mnt/romfs", "/mnt/blk", "/mnt/dsfs"):
                        sim.run("umount " + mnt, 10)
                    sim.run("sync", 10)
                sim.stop()

            # partition with incorrect header: system starts without partition
            self.drvset_bad_image_test(log, tmpdir)

            if self.selected("crash") or not args.filter:
                if CRASH.search(sim.out):
                    self.result("no crash/panic", FAIL, "see log: %s" % args.log)
                else:
                    self.result("no crash/panic", PASS)

        # file system consistency checked by the host tools
        if disk and self.selected("fatfs"):
            if fsck_fat:
                ok, out = check_partition(disk, PART1_START, PART1_SIZE, tmpdir, [fsck_fat, "-n"])
                self.result("fatfs: fsck.fat", PASS if ok else FAIL, "" if ok else out.strip().splitlines()[-1])
            else:
                self.result("fatfs: fsck.fat", SKIP, "fsck.fat not found (dosfstools)")

        if disk and self.selected("ext4fs"):
            if e2fsck:
                ok, out = check_partition(disk, PART2_START, PART2_SIZE, tmpdir, [e2fsck, "-f", "-n"])
                self.result("ext4fs: e2fsck", PASS if ok else FAIL, "" if ok else out.strip().splitlines()[-1])
            else:
                self.result("ext4fs: e2fsck", SKIP, "e2fsck not found (e2fsprogs)")

        # ext2/ext3 created by mkext and modified by fstest
        for name, start, size in (("ext2", EXT2_START, EXT2_SIZE), ("ext3", EXT3_START, EXT3_SIZE)):
            if self.selected("fstest: %s (mkext)" % name):
                if e2fsck:
                    ok, out = check_partition(raw_disk, start, size, tmpdir, [e2fsck, "-f", "-n"])
                    self.result("%s: e2fsck" % name, PASS if ok else FAIL,
                                "" if ok else out.strip().splitlines()[-1])
                else:
                    self.result("%s: e2fsck" % name, SKIP, "e2fsck not found (e2fsprogs)")

        if self.selected("fstest: ext2 on blkdev"):
            if e2fsck:
                ok, out = check_partition(blk_disk, 0, BLK_DISK_SIZE // SECTOR_SIZE, tmpdir, [e2fsck, "-f", "-n"])
                self.result("blkdev: e2fsck", PASS if ok else FAIL, "" if ok else out.strip().splitlines()[-1])
            else:
                self.result("blkdev: e2fsck", SKIP, "e2fsck not found (e2fsprogs)")

        os.close(self.uart_master)
        os.close(self.uart_slave)

        if args.keep:
            print("Temporary files: %s" % tmpdir)
        else:
            shutil.rmtree(tmpdir, ignore_errors=True)

    def tests(self, sim, disk, romfs):
        args = self.args

        # boot: the first prompt can be followed by messages of daemons (netmgr), so empty command is
        # used to synchronize with shell
        if not sim.read(BOOT_TIMEOUT, lambda: "\n$ " in sim.out):
            self.result("boot", FAIL, "shell prompt not found (see log: %s)" % args.log)
            return

        if args.tap:
            sim.read(NETWORK_TIMEOUT, lambda: "NM: Gateway" in sim.out)

        sim.read(1)
        ok, _ = sim.run("")
        if not ok:
            self.result("boot", FAIL, "shell does not respond (see log: %s)" % args.log)
            return

        ok, out = sim.run("uname -a")
        if ok and "POSIX simulator" in out:
            self.result("boot", PASS, out.strip().splitlines()[1] if len(out.strip().splitlines()) > 1 else "")
        else:
            self.result("boot", FAIL, "unexpected uname output")
            return

        # driver configuration strings (modprobe -c), UART configuration is checked on host side
        self.summary_test(sim, "devcfg: config strings", "/host/sim-env", "lua devcfg.lua", DEFAULT_TIMEOUT)
        if self.selected("devcfg: config strings"):
            self.check_uart_config()

        # board model (before fstest on hostfs, test files are removed after test)
        board_tests = ("board: gpio", "board: i2c", "board: i2cee", "board: ds2482", "board: spi", "board: spiee",
                       "board: mcp23s17", "board: uart", "board: net",
                       "board: sdcard", "fstest: ext2 on sdspi", "board: panel")
        if args.board:
            # Ethernet models of the board: netmgr configures the network in background, its
            # messages are printed to the terminal (wait, so they do not interrupt tests)
            if any(self.selected(n) for n in board_tests):
                end = time.time() + NETWORK_TIMEOUT
                while time.time() < end:
                    ok, out = sim.run("ifconfig")
                    if (not ok) or ("name: inet" not in out) or ("STATIC IP" in out) or ("DHCP CONFIGURED" in out):
                        break
                    sim.read(1)
                sim.read(0.5)

            self.summary_test(sim, "board: gpio", "/host/sim-env", "lua gpio.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: i2c", "/host/sim-env", "lua i2c.lua", DEFAULT_TIMEOUT)

            self.summary_test(sim, "board: i2cee", "/host/sim-env", "lua i2cee.lua", FSTEST_TIMEOUT)
            self.summary_test(sim, "board: ds2482", "/host/sim-env", "lua ds2482.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: mcp23s17", "/host/sim-env", "lua mcp23s17.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: uart", "/host/sim-env", "lua uart.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: net", "/host/sim-env", "lua net.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: spi", "/host/sim-env", "lua spi.lua", DEFAULT_TIMEOUT)
            self.summary_test(sim, "board: spiee", "/host/sim-env", "lua spiee.lua", FSTEST_TIMEOUT)
            self.summary_test(sim, "board: sdcard", "/host/sim-env", "lua sdcard.lua", FSTEST_TIMEOUT)

            # SD card (SDSPI driver) formatted by sdcard.lua
            if self.selected("board: sdcard"):
                self.summary_test(sim, "fstest: ext2 on sdspi", "/mnt/sdcard", "fstest", SDSPI_FSTEST_TIMEOUT,
                                  ("mount /dev/sdcard ext4fs /mnt/sdcard",), "/dev/sdcard")
            elif self.selected("fstest: ext2 on sdspi"):
                self.result("fstest: ext2 on sdspi", SKIP, "requires 'board: sdcard' test")

            self.panel_test(sim)
        else:
            for name in board_tests:
                if self.selected(name):
                    self.result(name, SKIP, "board model not selected (--board none)")

        # other appset linked with simulator (make test-sim links sim-env/testappset)
        self.appset_test(sim)

        # drvset partition (option --drvset)
        self.drvset_test(sim)
        self.drvset_fs_test(sim)

        # block layer: HOSTDISK with emulated constraints (block size, buffer alignment), formatted by blkdev.lua
        self.summary_test(sim, "blkdev: constraints", "/host/sim-env", "lua blkdev.lua", FSTEST_TIMEOUT)
        if self.selected("blkdev: constraints"):
            self.summary_test(sim, "fstest: ext2 on blkdev", "/mnt/blk", "fstest", FSTEST_TIMEOUT,
                              ("mount /dev/blk ext4fs /mnt/blk",), "/dev/blk")
        elif self.selected("fstest: ext2 on blkdev"):
            self.result("fstest: ext2 on blkdev", SKIP, "requires 'blkdev: constraints' test")

        shutil.rmtree(os.path.join(args.hostfs_dir, "sim-env"), ignore_errors=True)

        # file systems
        self.summary_test(sim, "fstest: ramfs", "/tmp", "fstest", FSTEST_TIMEOUT)
        self.summary_test(sim, "fstest: hostfs", "/host", "fstest", FSTEST_TIMEOUT)

        if disk:
            self.summary_test(sim, "fstest: fatfs", "/mnt/sda1", "fstest", FSTEST_TIMEOUT, device="/dev/sda1")
            self.summary_test(sim, "fstest: ext4fs", "/mnt/sda2", "fstest", FSTEST_TIMEOUT, device="/dev/sda2")
        else:
            for name in ("fstest: fatfs", "fstest: ext4fs"):
                if self.selected(name):
                    self.result(name, SKIP, "mkfs.fat or mkfs.ext4 not found (dosfstools, e2fsprogs)")

        self.summary_test(sim, "fstest: flatfs", "/mnt/flatfs", "fstest", FSTEST_TIMEOUT,
                          ("mkflatfs /dev/sdb1 %u" % FLATFS_MAX_FILE_KIB, "mkdir /mnt/flatfs",
                           "mount /dev/sdb1 flatfs /mnt/flatfs"), "/dev/sdb1")
        self.summary_test(sim, "fstest: eefs", "/mnt/eefs", "fstest", FSTEST_TIMEOUT,
                          ("mkeefs /dev/sdb2", "mkdir /mnt/eefs", "mount /dev/sdb2 eefs /mnt/eefs"),
                          "/dev/sdb2")
        self.summary_test(sim, "fstest: ext2 (mkext)", "/mnt/ext2", "fstest", FSTEST_TIMEOUT,
                          ("mkext -e 2 -l dnxext2 -i /dev/sdb3", "mkdir /mnt/ext2",
                           "mount /dev/sdb3 ext4fs /mnt/ext2"), "/dev/sdb3")
        self.summary_test(sim, "fstest: ext3 (mkext)", "/mnt/ext3", "fstest", FSTEST_TIMEOUT,
                          ("mkext -e 3 -j -l dnxext3 -i /dev/sdb4", "mkdir /mnt/ext3",
                           "mount /dev/sdb4 ext4fs /mnt/ext3"), "/dev/sdb4")

        if romfs:
            self.summary_test(sim, "fstest: romfs", "/mnt/romfs", "fstest -r", FSTEST_TIMEOUT,
                              ("mkdir /mnt/romfs", "mount /dev/sdc romfs /mnt/romfs"), "/dev/sdc")
        elif self.selected("fstest: romfs"):
            self.result("fstest: romfs", SKIP, "ROMFS image not created")

        # network
        opt = "" if args.perf else "-t"
        self.summary_test(sim, "udstest", None, ("udstest " + opt).strip(), NETTEST_TIMEOUT)

        # system stress tests (threads, heap, synchronization objects, processes)
        self.summary_test(sim, "perf: stress", None, "perf" if args.perf else "perf -s", PERF_TIMEOUT)

        if not self.selected("inettest"):
            return

        if not args.tap:
            self.result("inettest", SKIP, "TAP interface not selected (--tap <name>)")
            return

        # network is configured by netmgr in background
        end = time.time() + NETWORK_TIMEOUT
        configured = False
        while (time.time() < end) and not configured:
            ok, out = sim.run("ifconfig")
            configured = ok and ("STATIC IP" in out or "DHCP CONFIGURED" in out)
            if not configured:
                sim.read(1)

        if configured:
            self.summary_test(sim, "inettest", None, ("inettest " + opt).strip(), NETTEST_TIMEOUT)
        elif "LINK DISCONNECTED" in out:
            self.result("inettest", FAIL, "no link: host TAP interface '%s' is down or not available" % args.tap)
        else:
            self.result("inettest", FAIL, "network interface not configured")


# --------------------------------------------------------------------------------------------------
# main
# --------------------------------------------------------------------------------------------------
def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    parser = argparse.ArgumentParser(description="dnx RTOS POSIX simulator tests")
    parser.add_argument("--sim", default=os.path.join(root, "build", "dnx-sim"), help="simulator program")
    parser.add_argument("--rom", default=os.path.join(root, "build", "sim-romfs.bin"), help="ROMFS image")
    parser.add_argument("--tap", default=None, help="TAP interface (enables inettest)")
    parser.add_argument("--drvset", default=os.path.join(root, "build", "sim-drvset.bin"),
                        help="drvset partition image (tools/drvset/example), 'none' disables drvset tests")
    parser.add_argument("--board", default=os.path.join(root, "sim-env", "boards", "test.lua"),
                        help="board model script, 'none' disables board tests")
    parser.add_argument("--perf", action="store_true", help="also performance measurements of network tests and perf")
    parser.add_argument("--iostat", action="store_true", help="print block device statistics of file system tests")
    parser.add_argument("--log", default=os.path.join(root, "build", "simtest.log"), help="log file")
    parser.add_argument("--keep", action="store_true", help="keep temporary files (disk image, hostfs)")
    parser.add_argument("-v", "--verbose", action="store_true", help="show simulator output")
    parser.add_argument("filter", nargs="?", default=None, help="run only tests which name contains text")
    args = parser.parse_args()
    args.root = root
    if args.board == "none":
        args.board = None
    if args.drvset == "none" or not os.path.isfile(args.drvset):
        args.drvset = None

    for path in [args.sim, args.rom] + ([args.board] if args.board else []):
        if not os.path.isfile(path):
            print("File not found: %s (run: make build-sim)" % path)
            return 1

    runner = TestRunner(args)
    runner.execute()

    failed = sum(1 for r in runner.results if r[1] == FAIL)
    passed = sum(1 for r in runner.results if r[1] == PASS)
    skipped = sum(1 for r in runner.results if r[1] == SKIP)

    print("\nSIMTEST: passed: %u, failed: %u, skipped: %u (log: %s)" % (passed, failed, skipped, args.log))

    return 1 if (failed or not passed) else 0


if __name__ == "__main__":
    sys.exit(main())
