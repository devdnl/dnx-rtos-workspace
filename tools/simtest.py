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
#           - fstest on ramfs (/tmp) and hostfs (/host, temporary host directory),
#           - fstest on FAT and ext4 partitions of temporary disk image (/mnt/sda1, /mnt/sda2),
#             image is checked by fsck.fat and e2fsck after test (requires dosfstools and
#             e2fsprogs, otherwise tests are skipped),
#           - udstest (UNIX domain sockets),
#           - inettest (TCP/IP, only with --tap option, TAP interface must be prepared),
#           - no application crash and kernel panic.
#
#         Usage:
#           tools/simtest.py [--sim build/dnx-sim] [--rom build/sim-romfs.bin] [--tap tap0]
#                            [--perf] [--log build/simtest.log] [-v] [filter]
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
import os
import pty
import re
import select
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time

# --------------------------------------------------------------------------------------------------
# constants
# --------------------------------------------------------------------------------------------------
BOOT_TIMEOUT = 30           # seconds
NETWORK_TIMEOUT = 20        # seconds
DEFAULT_TIMEOUT = 60        # seconds
FSTEST_TIMEOUT = 300        # seconds
NETTEST_TIMEOUT = 600       # seconds
QUIET_TIME = 0.3            # seconds without output after prompt

SECTOR_SIZE = 512
DISK_SIZE = 64 * 1024 * 1024
PART1_START = 2048          # sectors
PART1_SIZE = 32 * 1024 * 1024 // SECTOR_SIZE
PART2_START = PART1_START + PART1_SIZE
PART2_SIZE = DISK_SIZE // SECTOR_SIZE - PART2_START

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
        return self.out[start:].endswith("\n$ ")

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

    # run program which prints SUMMARY line
    def summary_test(self, sim, name, cwd, cmd, timeout):
        if not self.selected(name):
            return

        if cwd:
            ok, out = sim.run("cd " + cwd)
            if not ok or ("No such" in out) or ("rror" in out):
                self.result(name, FAIL, "cannot enter %s" % cwd)
                return

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

        # performance measurements (results of the PERFORMANCE section)
        if self.args.perf and ok:
            for line in out.splitlines():
                if line.startswith("  ") and ((" KiB/s" in line) or (" us " in line) or ("/s" in line)):
                    print("    " + line.strip())

    def execute(self):
        args = self.args

        mkfs_fat = find_tool("mkfs.fat", "mkfs.vfat")
        mkfs_ext4 = find_tool("mkfs.ext4")
        fsck_fat = find_tool("fsck.fat", "fsck.vfat")
        e2fsck = find_tool("e2fsck")

        tmpdir = tempfile.mkdtemp(prefix="dnx-simtest-")
        hostfs_dir = os.path.join(tmpdir, "hostfs")
        os.mkdir(hostfs_dir)

        disk = None
        if mkfs_fat and mkfs_ext4:
            disk = os.path.join(tmpdir, "sd.img")
            try:
                create_disk_image(disk, mkfs_fat, mkfs_ext4)
            except subprocess.CalledProcessError as e:
                print("Disk image error: %s" % e.stderr.decode(errors="replace").strip())
                disk = None

        argv = [args.sim, "--disk", "0=%s:ro" % args.rom, "--hostfs", hostfs_dir]
        if disk:
            argv += ["--disk", "1=%s" % disk]
        if args.tap:
            argv += ["--tap", args.tap]

        os.makedirs(os.path.dirname(os.path.abspath(args.log)), exist_ok=True)
        with open(args.log, "w", buffering=1) as log:
            log.write("$ %s\n" % " ".join(argv))

            sim = Simulator(argv, log, args.verbose)
            try:
                self.tests(sim, disk)
            finally:
                if sim.is_alive():
                    sim.run("cd /", 5)
                    if disk:
                        sim.run("umount /mnt/sda1", 10)
                        sim.run("umount /mnt/sda2", 10)
                    sim.run("sync", 10)
                sim.stop()

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

        if args.keep:
            print("Temporary files: %s" % tmpdir)
        else:
            shutil.rmtree(tmpdir, ignore_errors=True)

    def tests(self, sim, disk):
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

        # file systems
        self.summary_test(sim, "fstest: ramfs", "/tmp", "fstest", FSTEST_TIMEOUT)
        self.summary_test(sim, "fstest: hostfs", "/host", "fstest", FSTEST_TIMEOUT)

        if disk:
            self.summary_test(sim, "fstest: fatfs", "/mnt/sda1", "fstest", FSTEST_TIMEOUT)
            self.summary_test(sim, "fstest: ext4fs", "/mnt/sda2", "fstest", FSTEST_TIMEOUT)
        else:
            for name in ("fstest: fatfs", "fstest: ext4fs"):
                if self.selected(name):
                    self.result(name, SKIP, "mkfs.fat or mkfs.ext4 not found (dosfstools, e2fsprogs)")

        # network
        opt = "" if args.perf else "-t"
        self.summary_test(sim, "udstest", None, ("udstest " + opt).strip(), NETTEST_TIMEOUT)

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
    parser.add_argument("--perf", action="store_true", help="also performance measurements of network tests")
    parser.add_argument("--log", default=os.path.join(root, "build", "simtest.log"), help="log file")
    parser.add_argument("--keep", action="store_true", help="keep temporary files (disk image, hostfs)")
    parser.add_argument("-v", "--verbose", action="store_true", help="show simulator output")
    parser.add_argument("filter", nargs="?", default=None, help="run only tests which name contains text")
    args = parser.parse_args()

    for path in (args.sim, args.rom):
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
