# dnx RTOS workspace

## Overview
The dnx RTOS workspace is a set of operating system (dnx RTOS), application sets (appset),
common libraries, configuration and tools. The purpose of this repository is an example of basic
system configuration and as a reference project. Repository constructed in that way
allows user to create many different projects in single repository. It is much easier
to keep system up-to-date and also is much easier to create multi-processor projects,
because libraries can be shared between particular sub projects.

### What is dnx RTOS
dnx RTOS is a general purpose operating system for small (mostly 32-bit)
microcontrollers. It is built on top of the FreeRTOS kernel, and the dnx layer
above it is modeled on the well-known Unix architecture: drivers are visible as
device files, file systems are mounted in a single directory tree (VFS) and
programs are started from a shell like on a PC. The program layer is mostly
compatible with the C standard, so many PC programs can be ported with little
effort. The system can be thought of as a FreeRTOS kernel distribution.
The same system can be also started on PC as a Linux program (POSIX simulator),
which is used for development and automatic tests without hardware.

The operating system itself lives in the separate
[dnx-rtos-kernel](https://github.com/devdnl/dnx-rtos-kernel) repository, which is
included here as the `kernel` git submodule.

### Main components
A complete firmware image built in this workspace consists of up to three
independent parts. Each part is compiled separately and placed in its own flash
partition:

- **kernel** – the operating system: FreeRTOS scheduler, dnx layer, drivers,
  file systems and network stack. The same kernel sources are built twice with
  different configurations: once for the application (`app-kernel`) and once for
  the boot loader (`btl-kernel`).
- **application set (appset)** – collection of programs and libraries that run on
  top of the kernel. The appset is linked as a separate image and the kernel finds
  programs through the appset table placed at a configured flash address. There are
  two appsets in the workspace:
  - `app-appset` – regular application set (shell, system tools, servers, etc.),
  - `btl-appset` – boot loader application set (`bootloader` program).
- **romfs** – read-only file system image (`app-romfs`) created from the
  `app-romfs` directory at build time. It is a simple way to deliver
  configuration files, scripts, web pages or graphics from the PC to the
  microcontroller.

Because the parts are independent, it is possible to rebuild and flash only the
part that was changed, e.g. only the appset during application development.

### Kernel features
- Unix-like architecture: device files in `/dev`, VFS, mount points, pipes,
  standard streams, multiple program instances,
- interrupts are not masked by the system (the FreeRTOS feature),
- dynamic memory allocation with simple garbage collector for programs,
- modular driver layer; drivers are initialized at runtime (`driver_init()`,
  `modprobe`),
- supported CPU families: STM32F1, STM32F3, STM32F4, STM32F7, STM32H7,
  Raspberry Pi RP2040 (Cortex-M0+) and PC (Linux POSIX simulator, x86),
- drivers: GPIO, UART/TTY, SPI, I2C, CAN, ADC, PWM, DMA, RTC, WDG, CRC, SDMMC,
  SD over SPI, QSPI, ETH, ENC28J60, USB device and host, LTDC, DCI, external
  memory controller, I2C/SPI EEPROM, NVM, partitions, memory block, loop, sound,
  DHT11, encoder timer, zero/urandom, host disk (simulator) and others,
- file systems: ramfs, procfs, romfs, FAT/exFAT (fatfs), EXT2/3/4 (ext4fs),
  eefs (EEPROM file system), flatfs, hostfs (host directory, simulator),
- network: TCP/IP (lwIP based inet), CAN network (cannet), simple IPC (sipc),
  Unix domain sockets (uds).

The minimal hardware requirements are about 64 KiB of flash and 20 KiB of RAM,
but the real needs depend on the configuration.

### Application set
The `app-appset` contains a reference set of programs, e.g.:

- shell and terminal: `dsh`, `login`, `tmux`, `more`, `ed` (line editor),
- full-screen (TUI) programs: `nano` (text editor), `mc` (file manager),
  `dialog` (dialog boxes from scripts), `tuidemo`,
- file tools: `ls`, `cat`, `cp`, `rm`, `mkdir`, `touch`, `chmod`, `dd`, `grep`,
  `hexdump`, `md5sum`, `echo`, `mount`, `umount`, `df`, `sync`, `mkfifo`,
  `mknod`,
- storage: `mkext`, `mkeefs`, `mkflatfs` (file system formatters), `storinit`
  (storage initialization),
- system tools: `initd`, `siminit`, `top`, `free`, `memmon`, `dmesg`, `lsmod`,
  `modprobe`, `uptime`, `uname`, `date`, `dstd` (daylight saving
  time), `time`, `perf`, `reboot`, `shutdown`,
- network: `ifconfig`, `netmgr`, `ping`, `ethstat`, `canstat`, `httpsrv` (HTTP
  server with Lua scripting), `ftpd`, `telnetd`, `sntpd`, `mqtt_broker`,
- interpreters: `lua`, `luac`, `tcl`,
- tests: `fstest` (file systems), `udstest` (Unix domain sockets), `inettest`
  (TCP/IP), `keytest` and `ttyread` (terminal),
- examples: `usbdevmouse`.

The `initd` program is started by the kernel as the first process and is
responsible for initializing drivers, mounting file systems and starting the
user programs (in the simulator the `siminit` program is used instead).

Sources of programs shared by appsets are placed in `common_apps/<name>/`. An
appset contains a program by the file `<appset>/programs/<name>.mk`:
```make
PROG_ROOT_CAT = $(COMMON_APPS)/
include $(COMMON_APPS)/cat/Makefile.in
```
Programs specific to a single appset are directories in `<appset>/programs/`
with `Makefile.in` (`initd`, `siminit`, `bootloader`, `PROG_ROOT_<NAME>` is
set by the build). All programs of the appset are built except programs listed
in `__PROGRAMS_EXCLUDE_LIST__` of the appset configuration
(`config/<appset>/programs/programs_flags.h`).

The program name (command) is the name of the `.mk` file or of the program
directory and can contain `.` and `-` (e.g. `mkfs.eefs`). The program source
declares it by `PROGRAM_PARAMS(<identifier>, ...)`, where the identifier is
the name with other characters than letters, digits and `_` replaced by `_`
(`PROGRAM_PARAMS(mkfs_eefs, STACK_DEPTH_LOW);`).

The appset directory contains only programs and libraries. All appsets are
built by the common build base `tools/appset` (Makefile, default
configuration, scripts) with the appset directory, the configuration and the
build directory given as arguments, e.g.:
```
make -C tools/appset all APPSET_DIR=app-appset CONFIG_DIR=config/app-appset BUILD_DIR=build/app-appset
```
The top-level targets (`build-app-appset`, `build-btl-appset`, ...) do this,
see `tools/appset/README.md`. A new appset is a new directory with
`programs/` and `libs/` and its configuration in `config/` (copy of the default
configuration `tools/appset/config`). The ROMFS image is built the same way by
`tools/romfs` from the directory with files of the image (`app-romfs`).

Programs of the main appset (`__PROJ_APPSET_TABLE_ADDRESS__`) are started by
name. More appsets can be programmed in other flash partitions (linked for
their address, executed in place) and mounted by the `appsetfs` file system
from a memory region device (`MEMBLOCK`); its programs are executable files of
the mount point:
```
modprobe -i MEMBLOCK 1 0 /dev/appset1 -c memory_address=0x08100000,block_size=524288,access=rdonly
mount /dev/appset1 appsetfs /opt/progs
ls /opt/progs
/opt/progs/heating
mount - appsetfs /bin        # programs of the main appset as files (done by initd)
```
A program is started by path, or by name when it is not found in the main
appset and the kernel finds it in a directory of the program search path
(`__OS_PROGRAM_PATH__`, e.g. `"/bin:/opt/progs"`; programs read it by
`confstr(_CS_PATH)`, `dsh` completes program names from these directories,
`/bin` is the main appset mounted by `initd`). Program files are
empty, read-only and executable. The appset can be unmounted when none of its
programs is running (`EBUSY` otherwise) and the same appset cannot be mounted
twice. `df` shows the partition size (memory region of the `MEMBLOCK` device,
`__PROJ_APPSET_TABLE_SIZE__` of the kernel configuration for the main appset)
and the space used by the appset image.

### Common libraries
The `common_libs` directory contains libraries shared by all appsets (and by
possible other sub projects): `libc` (dnx RTOS C library), `lua`, `zlib`,
`luadnx` (Lua bindings to dnx RTOS services), `fastlz`, `libmqtt`,
`http-parser`, `http-multipart-parser`, `fpkg`, `bs` (binary serialization),
`conf_parser`, `libtui` (text user interface), `llist`, `tinyexpr`, `utcl`,
`crc`, `chsum`, `md5`, `sha1` and `sha256`. Libraries are attached to particular appset by
makefiles placed in `<appset>/libs` (`<name>.mk`: `LIB_ROOT_<NAME> = $(COMMON_LIBS)/` and
`include $(COMMON_LIBS)/<name>/Makefile.in`), local libraries of the appset are
directories with `Makefile.in` (e.g. `app-appset/libs/liblocal`).

### Board Support Packages
Configuration of the whole workspace for a selected board is stored in a single
BSP file (`bsp/*.bsp`). Included BSPs:

- `stm32f746g-disco.bsp` – STM32F746G-DISCO board,
- `stm32h747i-disc1.bsp` – STM32H747I-DISCO board,
- `stm32h747i-disc1_with_btl.bsp` – STM32H747I-DISCO board with boot loader,
- `Raspberry-Pi-Pico.bsp` – Raspberry Pi Pico (RP2040),
- `posix-sim.bsp` – POSIX simulator (`config/sim-*`).

### Workspace structure
- `app-appset` – application set: programs (`programs/`) and libraries (`libs/`),
- `app-dist` – files added to the application FPKG update package,
- `app-romfs` – content of the romfs image,
- `bsp` – board support packages,
- `btl-appset` – boot loader application set (programs and libraries),
- `build` – output binaries and build directories of appsets and romfs
  (`build/<configuration name>/`, created after compilation),
- `common_apps` – programs shared between appsets,
- `common_libs` – libraries shared between appsets,
- `config` – current configuration of all parts (`app-kernel`, `app-appset`,
  `app-romfs`, `btl-kernel`, `btl-appset`) and of the POSIX simulator
  (`sim-kernel`, `sim-appset`, `sim-romfs`),
- `kernel` – dnx RTOS kernel (git submodule),
- `sim-env` – simulation environment of the POSIX simulator (board model:
  Lua description of hardware connected to the microcontroller, pages of the
  board panel, see `sim-env/README.md`),
- `tools` – build bases of appsets (`tools/appset`) and romfs (`tools/romfs`)
  with their default configurations, helper scripts (BSP import/export,
  flashing, OpenOCD, terminal, linker script generator, Docker image,
  simulator tests `simtest.py`, board panel `simpanel.py`).

## Getting started

### Clone repository
The kernel is a git submodule, so the repository should be cloned with
submodules:
```
git clone --recurse-submodules https://github.com/devdnl/dnx-rtos-workspace.git
```
or in already cloned repository:
```
git submodule update --init
```

### Build environment
The recommended way is to use the prepared Docker image that contains all needed
tools (ARM GCC toolchain, i686 musl toolchain for the simulator, OpenOCD,
srecord, Python, Lua, dialog, minicom, dosfstools, e2fsprogs, etc.):
```
cd tools
./build_docker_image.py
cd ..
./run_container.py              # interactive shell in the container
./run_container.py make build-app   # run single command in the container
```
The container is created once per workspace directory and the workspace is
mounted at the same path as on the host. After the image is rebuilt (e.g.
`tools/Dockerfile` was changed) the old container has to be removed
(`docker rm <name>`), then it is created again from the new image. On Linux the
container is started in privileged mode with the host network and `/dev`, so
the host serial ports and TAP interfaces are available also in the container.

### Select board
Import the BSP file of the selected board to the `config` directory:
```
./tools/bsp_import.sh bsp/stm32f746g-disco.bsp
```
Configuration directories included in the BSP file (e.g. `config/app-*` and
`config/btl-*` of a board) are reset to the default configuration before
import, so the result does not depend on the previous configuration; other
directories are not changed (`--no-reset` applies the BSP values on the current
configuration). The simulator configuration (`config/sim-*`) is stored in
`bsp/posix-sim.bsp`.

The current configuration can be saved as a BSP by `./tools/bsp_export.sh`:
```
./tools/bsp_export.sh ~/my-project.bsp               # all directories (default, same as "all")
./tools/bsp_export.sh bsp/my-board.bsp "app*|btl*"   # only application and bootloader
./tools/bsp_export.sh my-sim.bsp sim-                # only simulator
```
Directories are given as patterns separated by `|` (pattern with `*` is a shell
pattern, other patterns are prefixes, e.g. `app-|sim-`); patterns with `*` or
`|` must be quoted in the shell. Relative paths of BSP files are relative to the
current directory. Each directory is
optimized for its CPU architecture (`__CPU_ARCH__`): files of other
architectures are not exported, they have default values after import.

`./tools/config_sync.sh` synchronizes all configuration directories and BSP
files with the default configurations (after a new configuration flag is added
to the kernel, appset or romfs): BSP files are imported and exported again
with the same directories, then the default BSP (`bsp/stm32h747i-disc1.bsp`)
and the simulator BSP (`bsp/posix-sim.bsp`) are applied.

### Configuration
Each part has its own configuration (TUI configtool):
```
make config-app-kernel
make config-app-appset
make config-app-romfs
make config-btl-kernel
make config-btl-appset
```

### Build
```
make build-app      # application: kernel + appset + romfs
make build-btl      # boot loader: kernel + appset
make build-all      # boot loader and application in single hex file
```
Parts can be also built separately: `build-app-kernel`, `build-app-appset`,
`build-app-romfs`, `build-btl-kernel`, `build-btl-appset`. Type `make help` to
see all targets.

Results are placed in the `build` directory:
- `*.hex`, `*.bin`, `*.elf` files of particular parts,
- `project_app.hex`, `project_btl.hex`, `project_btl_app.hex` – merged images,
- `project-app.fpkg`, `project-btl.fpkg` – FPKG packages that can be used by
  the boot loader to update the firmware.

### Flash
Start OpenOCD (target is selected according to the kernel configuration) and
then install selected image:
```
./tools/runocd.sh       # in separate terminal
make install-app        # or: install, install-btl, install-app-kernel,
                        #     install-app-appset, install-app-romfs
```
The system terminal can be opened by `./tools/terminal.sh [/dev/ttyUSBx]`.

### POSIX simulator (Linux)
dnx RTOS can be started on PC as a Linux program (FreeRTOS POSIX port, 32-bit
x86 executable). The simulator uses its own configuration: `config/sim-kernel`,
`config/sim-appset`, `config/sim-romfs` (appset `app-appset` and ROMFS content
`app-romfs` are shared with the application). The init program of the simulator is `siminit`.
```
make build-sim      # creates build/dnx-sim and build/sim-romfs.bin
make run-sim        # builds and starts simulator in the current terminal
```
Other appsets can be linked with the simulator too (e.g. the bootloader appset
or an appset of another project): `SIM_APPSETS` is a list of `<appset
directory>[:<configuration directory>]` (relative to the workspace or absolute,
default configuration `config/sim-appset`), each appset is built in
`build/sim-<appset directory name>`:
```
make build-sim SIM_APPSETS="btl-appset /home/user/project/appset"
make run-sim SIM_APPSETS=btl-appset
```
`siminit` mounts each of them by `appsetfs` in `/opt/<appset directory name>`
(memory region device `/dev/appset<n>` of its program table, as an appset
partition of the microcontroller) and the main appset in `/bin`; programs are
started by path or found in the program search path (`__OS_PROGRAM_PATH__` of
`config/sim-kernel`, `"/bin:/opt/testappset"`). All symbols of an appset except its
program table are local, so appsets can contain the same libraries (libc) and
program names in the source code; the table of the appset is
`_program_table_desc_<appset directory name>` (debugger). `make test-sim`
always links the test appset `sim-env/testappset` (program `appsettest`: libc,
threads and child process in other appset, search order of programs, start by
path and program search path, files of `appsetfs`; mount and unmount from the
shell).
The simulator is built by the `i686-unknown-linux-musl-` toolchain (installed
in the Docker image on x86_64 hosts) as a static executable, so it can be
started also outside the container. Program options:
```
build/dnx-sim [--ram <KiB>] [--disk <n>=<file>[:ro]] [--hostfs <name>=<dir>]
              [--uart <n>=pty[:<link>]|<device>] [--tap <name>]
              [--board <script> [--panel <socket>]]
make run-sim SIM_OPTS="--hostfs host=."
```
- `--ram` sets size of RAM (in KiB),
- `--disk` connects host file to the `HOSTDISK` driver (major number `<n>`);
  disk 0 is ROMFS image mounted by `siminit` in `/etc`
  (`--disk 0=build/sim-romfs.bin:ro`, used by `make run-sim`),
  disks 1..3 are initialized by `siminit` as `/dev/sda`..`/dev/sdc`, MBR
  partitions as `/dev/sdX1`..`/dev/sdX4` (`PART` driver), FAT and ext4 file
  systems are mounted in `/mnt/sdXn` (or `/mnt/sdX` when disk has no
  partitions), e.g. `--disk 1=sd.img`; other file systems can be mounted
  manually, e.g. `mount /dev/sdb1 flatfs /mnt/flatfs`; disks 4..7 are not
  initialized by `siminit`, the driver can emulate hardware constraints of
  block devices, e.g. `modprobe -i HOSTDISK 4 0 /dev/blk -c
  block_size=512,buffer_align=32` (the kernel block layer splits unaligned
  requests and uses bounce buffers),
- `--hostfs` shares host directory as `<name>` (option can be used many times),
  the `hostfs` file system mounts directory selected by source
  `host://<name>[/subdir]`, e.g. `mount host://www hostfs /mnt/www`,
  `mount host://www/sub/dir hostfs /mnt ro`; `siminit` mounts directory `host`
  in `/host`,
- `--uart` connects `UART<n>` (`/dev/ttyS<n>`, n = 0..3) to the new pseudo
  terminal (its path is printed at start, optional `<link>` is a symbolic link
  to it, e.g. `--uart 1=pty:/tmp/ttyDNX1`, then `picocom /tmp/ttyDNX1`) or to
  the host serial port (e.g. `--uart 2=/dev/ttyUSB0`, baud rate, parity and
  stop bits are configured by UART `ioctl()`); not connected UART drops
  transmitted data (also when PTY is not opened by any program); the
  simulator terminal is connected to `UART0` (`/dev/ttyS0`) by default,
  `--uart 0=pty` moves the system console to PTY,
- `--tap` connects the `ETH` driver (`/dev/eth`) to the host TAP interface,
  `siminit` starts `netmgr` with `/etc/netmgr.conf` (static address
  192.168.77.2/24, gateway 192.168.77.1, file `app-romfs/netmgr.conf`),
- `--board` loads the board model – Lua script which describes hardware
  connected to the microcontroller (e.g. LEDs, buttons, connections between
  GPIO pins, I2C devices: EEPROM, I/O expander, temperature sensor, SPI
  devices: EEPROM, SD card, shift register), e.g.
  `--board sim-env/boards/example.lua`; API and device models are described
  in `sim-env/README.md`. I2C devices are created in dnx RTOS e.g. by
  `modprobe -i I2C 0 0 /dev/i2c-ee -c address=0x50,sub_addr_mode=2`,
- `--panel` creates socket of the board panel – visualization of the board
  (LEDs, buttons, values of models, LCD of the `LTDC` driver with touch panel)
  in a web browser by `tools/simpanel.py`,
  e.g. `make run-sim PANEL=1 SIM_OPTS="--board sim-env/boards/example.lua"`
  and `http://localhost:8080`; pages are defined in `sim-env/panel/`
  (see `sim-env/README.md`).

RTC (`/dev/rtc`) uses the host clock, time set in dnx is kept as offset to the
host time (until simulator restart). `Ctrl+\` terminates the simulator,
`reboot` restarts it with the same options.

#### Disk images
Disk image with FAT and ext4 partitions can be created e.g. by:
```
truncate -s 64M sd.img
printf 'label: dos\n2048,32M,6\n,,83\n' | sfdisk sd.img
sudo losetup -P -f --show sd.img      # e.g. /dev/loop0
sudo mkfs.vfat /dev/loop0p1 && sudo mkfs.ext4 /dev/loop0p2
sudo losetup -d /dev/loop0
build/dnx-sim --disk 0=build/sim-romfs.bin:ro --disk 1=sd.img
```
Images of flatfs and eefs are formatted in the simulator by `mkflatfs` and
`mkeefs` programs (e.g. on empty image connected by `--disk 2=raw.img`).

#### Network
TAP interface is created once (the simulator does not need root privileges):
```
sudo ip tuntap add tap0 mode tap user $USER
sudo ip addr add 192.168.77.1/24 dev tap0
sudo ip link set tap0 up
make run-sim SIM_OPTS="--tap tap0"
ping 192.168.77.2                     # e.g. also telnet, ftp, http servers
```
Internet access of the simulator requires NAT on the host (DNS servers are
listed in `/etc/dns.conf`), e.g.:
```
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -s 192.168.77.0/24 ! -o tap0 -j MASQUERADE
```

### Automatic tests
The system can be tested without hardware (e.g. in CI) by `make test-sim`
(script `tools/simtest.py`). The simulator is started in a pseudo terminal, the
test programs are executed in the shell and their `SUMMARY:` lines are checked:

| Test             | Description |
|------------------|-------------|
| boot             | system starts, shell responds |
| `devcfg: config strings` | driver configuration strings (`sim-env/tests/devcfg.lua`, `modprobe -c`): PART, FBRIDGE, UART, MEMBLOCK, invalid values |
| `devcfg: uart termios` | UART:1 configured in dnx (baud rate, stop bits) is checked on the host pseudo terminal |
| `board: gpio`    | GPIO test (`sim-env/tests/gpio.lua`) with board model `sim-env/boards/test.lua` |
| `board: i2c`     | I2C test (`sim-env/tests/i2c.lua`): EEPROM, PCF8574, LM75 models, not connected device |
| `board: i2cee`   | I2CEE driver and eefs on 24C256 model (`sim-env/tests/i2cee.lua`) |
| `board: spi`     | SPI test (`sim-env/tests/spi.lua`): CS selection, bit order, mode, 74HC595 |
| `board: spiee`   | SPIEE driver and eefs on 25LC256 model (`sim-env/tests/spiee.lua`) |
| `board: sdcard`  | SDSPI driver on SD card models (SDHC, SD v1), ext2 created by `mkext` (`sim-env/tests/sdcard.lua`) |
| `fstest: ext2 on sdspi` | `fstest` on ext2 file system of the SD card model (SDSPI driver) |
| `board: panel`   | board panel protocol: socket of `tools/simpanel.py`, output, input and control signals (`sim-env/tests/panel.lua`), LCD frame of `LTDC` driver and touch read from FT5336 model (`sim-env/tests/lcd.lua`) |
| `fstest: ramfs`  | `fstest` in `/tmp` |
| `hostfs: directories` | two host directories (`--hostfs host=... --hostfs aux=...`): mount by name, subdirectory read only, unknown name and `..` rejected (`sim-env/tests/hostfs.lua`) |
| `fstest: hostfs` | `fstest` in temporary host directory (`/host`) |
| `fstest: fatfs`  | `fstest` on FAT partition of temporary disk image, checked by `fsck.fat` after test |
| `fstest: ext4fs` | `fstest` on ext4 partition of temporary disk image, checked by `e2fsck` after test |
| `fstest: flatfs` | partition formatted by `mkflatfs`, then `fstest` |
| `fstest: eefs`   | partition formatted by `mkeefs`, then `fstest` |
| `fstest: ext2 (mkext)` | partition formatted by `mkext -e 2`, then `fstest`, checked by `e2fsck` after test |
| `fstest: ext3 (mkext)` | partition formatted by `mkext -e 3 -j` (journal), then `fstest`, checked by `e2fsck` after test |
| `blkdev: constraints` | block layer on host disk with emulated constraints (block size 512 B, buffer alignment 32 B): unaligned requests, bounce buffers (`sim-env/tests/blkdev.lua`) |
| `fstest: ext2 on blkdev` | `fstest` on ext2 file system of that disk, checked by `e2fsck` after test |
| `fstest: romfs`  | `fstest -r` (read tests and read-only checks) on romfs image with reference data (`fstest_ref.py`, `mkfpkg.py`) |
| `udstest`        | Unix domain sockets |
| `inettest`       | TCP/IP (only with `--tap`) |
| no crash/panic   | no APP CRASH or KERNEL PANIC during tests |

```
make test-sim
make test-sim SIMTEST_OPTS="--tap tap0"          # also network tests
make test-sim SIMTEST_OPTS="--tap tap0 --perf"   # also network measurements
make test-sim SIMTEST_OPTS="ext4"                # only tests which name contains text
make test-sim SIMTEST_OPTS="--iostat"            # block device statistics of file system tests
python3 tools/simtest.py -h                      # all options (e.g. -v, --keep)
```
Exit status is 0 when all executed tests passed, the simulator output is saved
in `build/simtest.log`. FAT and ext4 tests require `dosfstools` and
`e2fsprogs` (installed in the Docker image), otherwise are skipped (ext2 and
ext3 tests are executed, only `e2fsck` check is skipped). All tests
take about 7 minutes (most of the time take fstest on eefs, because of small
blocks of the file system, and fstest on the SD card model over SPI).

# Memory layout
There is many possibilities of memory layout configuration. The simplest is when
whole flash is used for operating system and application set (with or without
romfs file system). More complicated is when one want to use additionally custom
boot loader or boot loader based on dnx RTOS application set (that kind of boot loader
looks similarly to application).

Microcontrollers' flash memory is divided to several sectors mostly of the same size
(in some microcontrollers it is not a case). Groups of sectors (or at least single
sector) can be treated as partitions. Each partition has different purpose, e.g.
there can be partition for operating system, application set or read-only data.
Using dnx RTOS workspace there is possibility to install only selected partition
instead of install whole flash by programmer or boot loader. Also selected partition
can be compiled separately if dependencies are met.

In general there is possibility to configure partitions at any flash address, but
the problem could occur when user want to program partitions separately - flash
memory can be erased only by sectors. If partition borders are in the middle of
sector then single sector erase affects both partitions and application or
operating system could be erased and broken.

## Basic flash layout
In this layout there are three partitions:

- operating system
- application set
- romfs

There is possibility to reduce number of partitions to two but it is nice to have
romfs, because that file system is connection between build environment and
runtime environment, in general it is easy way to share files between PC and
microcontroller. That file system can be used to store configuration or display data
in easy way; it means that application can open file and use it as regular file.
There is not need to create special flash tables to store files.

```
            CPU Flash memory
            +---------------------------+ sector 0   ---+
            | dnx RTOS                  |               | dnx RTOS partition
            | (kernel)                  | sector 1   ---+
            +---------------------------+ sector 2   ---+
            |                           |               |
            |                           |               |
            | application set           |               |
            | (appset)                  |               | application set partition
            |                           |               |
            |                           |               |
            |                           | sector 8   ---+
            +---------------------------+ sector 9   ---+
            | romfs                     |               | romfs partition
            +---------------------------+            ---+
```

### Example: STM32H747I-DISCO (`stm32h747i-disc1.bsp`)
The STM32H747XI microcontroller has 2 MiB of internal flash divided into two
banks, 8 sectors of 128 KiB each (16 sectors in total, `0x08000000`–`0x081FFFFF`).
In this configuration there is no boot loader – the kernel is started directly
after reset. The romfs partition is placed between the kernel and the
application set, so the application set occupies the whole second bank.

```
 0x08000000 +---------------------------+ sector 0   ---+
            |                           |               |
            | app-kernel       512 KiB  |               |
            |                           | sector 3      |
 0x08080000 +---------------------------+ sector 4      | bank 1
            |                           |               |
            | app-romfs        512 KiB  |               |
            |                           | sector 7      |
 0x08100000 +---------------------------+ sector 8   ---+
            |                           |               |
            |                           |               |
            | app-appset      1024 KiB  |               | bank 2
            |                           |               |
            |                           | sector 15     |
 0x08200000 +---------------------------+            ---+
```

| Partition  | Address      | Size     | Sectors | Configuration key |
|------------|--------------|----------|---------|-------------------|
| app-kernel | `0x08000000` | 512 KiB  | 0–3     | `config/app-kernel/custom_linker_script.h` (`flash` region) |
| app-romfs  | `0x08080000` | 512 KiB  | 4–7     | `config/app-romfs/config.h` (`__ROMFS_ADDRESS__`) |
| app-appset | `0x08100000` | 1024 KiB | 8–15    | `config/app-appset/project/project_flags.h` (`__PROJ_APPSET_TABLE_ADDRESS__`, `__PROJ_APPSET_TABLE_SIZE__`) |

The romfs size is not configured explicitly; the space available for the image
is limited by the next partition (512 KiB).

The addresses have to be consistent between parts:

- `__PROJ_APPSET_TABLE_ADDRESS__` in `config/app-kernel/project/project_flags.h`
  and `config/app-appset/project/project_flags.h` must be equal (`0x08100000`),
  also `__PROJ_APPSET_TABLE_SIZE__` in bytes (`1048576`; the kernel shows it as
  size of `appsetfs` with the main appset),
- `__LIBC_DNX_RTOS_ADDRESS__` in `config/app-appset/project/project_flags.h`
  must be equal to the kernel address (`0x08000000`),
- the application mounts romfs from `__ROMFS_ADDRESS__` (`0x08080000`, see
  `app-romfs/README`).

The whole flash is programmed by:
```
make build-app
make install-app        # or install-app-kernel, install-app-appset, install-app-romfs
```

## Flash layout with boot loader and application
In this layout there are boot loader section and application section (set of partitions).
The boot loader section contains operating system and application set partitions.
The application section contains operating system, application set and romfs
partitions.

The boot loader is a regular dnx RTOS system (`btl-kernel`) with its own
application set (`btl-appset`) that contains only the `bootloader` program. It is
started after reset, can update the application from a package and then jumps to
the application.

### Example: STM32H747I-DISCO (`stm32h747i-disc1_with_btl.bsp`)
The STM32H747XI microcontroller has 2 MiB of internal flash divided into two
banks, 8 sectors of 128 KiB each (16 sectors in total, `0x08000000`–`0x081FFFFF`).
All partition borders are placed at sector borders, so each partition can be
erased and programmed independently.

```
 0x08000000 +---------------------------+ sector 0   ---+
            | btl-kernel       256 KiB  | sector 1      |
 0x08040000 +---------------------------+ sector 2      | boot loader section
            | btl-appset       256 KiB  | sector 3      | (bank 1)
 0x08080000 +---------------------------+ sector 4   ---+
            |                           |               |
            | app-kernel       512 KiB  |               |
            |                           | sector 7      |
 0x08100000 +---------------------------+ sector 8      | application section
            |                           |               | (bank 1 + bank 2)
            | app-appset       768 KiB  |               |
            |                           | sector 13     |
 0x081C0000 +---------------------------+ sector 14     |
            | app-romfs        256 KiB  | sector 15     |
 0x08200000 +---------------------------+            ---+
```

| Partition  | Address      | Size    | Sectors | Configuration key |
|------------|--------------|---------|---------|-------------------|
| btl-kernel | `0x08000000` | 256 KiB | 0–1     | `config/btl-kernel/custom_linker_script.h` (`flash` region) |
| btl-appset | `0x08040000` | 256 KiB | 2–3     | `config/btl-appset/project/project_flags.h` (`__PROJ_APPSET_TABLE_ADDRESS__`, `__PROJ_APPSET_TABLE_SIZE__`) |
| app-kernel | `0x08080000` | 512 KiB | 4–7     | `config/app-kernel/custom_linker_script.h` (`flash` region) |
| app-appset | `0x08100000` | 768 KiB | 8–13    | `config/app-appset/project/project_flags.h` (`__PROJ_APPSET_TABLE_ADDRESS__`, `__PROJ_APPSET_TABLE_SIZE__`) |
| app-romfs  | `0x081C0000` | 256 KiB | 14–15   | `config/app-romfs/config.h` (`__ROMFS_ADDRESS__`) |

The addresses have to be consistent between parts:

- the kernel must know where its appset is placed – `__PROJ_APPSET_TABLE_ADDRESS__`
  in `config/<x>-kernel/project/project_flags.h` must be equal to the address
  of the appset (`0x08040000` for the boot loader, `0x08100000` for the
  application), `__PROJ_APPSET_TABLE_SIZE__` to its size in bytes,
- the appset must know where its kernel is placed – `__LIBC_DNX_RTOS_ADDRESS__`
  in `config/<x>-appset/project/project_flags.h` (`0x08000000` for the boot
  loader, `0x08080000` for the application),
- the boot loader starts the application from the `app-kernel` address
  (`APP_BASE_ADDRESS` in the `bootloader` program),
- the application mounts romfs from `__ROMFS_ADDRESS__` (see
  `app-romfs/README`).

The RAM (AXI SRAM, SRAM1–4, ITCM, DTCM and backup SRAM) is not partitioned –
the boot loader and the application use the same regions, because only one of
them is running at a time.

### Boot sequence
1. After reset the CPU starts `btl-kernel` from `0x08000000`, which runs the
   `bootloader` program from `btl-appset`.
2. The boot loader initializes basic drivers and mounts the SD card
   (`/dev/sd0p1`, FAT) at `/sd`.
3. If the update package `/sd/vendor/project-app.fpkg` exists, the boot loader
   extracts the `manifest` file from it and executes it as a TCL script.
   The manifest (`app-dist/manifest`) describes which images from the package
   are written to which addresses:
   ```
   flash_erase 0x08080000 [get_file_size "/app-kernel.bin"]
   flash_write "/app-kernel.bin" 0x08080000

   flash_erase 0x08100000 [get_file_size "/app-appset.bin"]
   flash_write "/app-appset.bin" 0x08100000

   flash_erase 0x081C0000 [get_file_size "/app-romfs.bin"]
   flash_write "/app-romfs.bin" 0x081C0000

   fpkg_extract "/manifest" "/sd/manifest"

   puts "DONE"
   ```
   `flash_erase` erases only the sectors needed for the image. Available
   commands: `flash_erase`, `flash_write`, `qspi_flash_erase`,
   `qspi_flash_write`, `get_file_size`, `fpkg_extract`, `remove`, `mkdir` and
   `rename`. Internal flash commands can access only the application section
   (`0x08080000`–`0x081FFFFF`), so the boot loader cannot overwrite itself.
   After the update the package is renamed to `project-app.fpkg.bak`, so it is
   not installed again at next start.
4. The boot loader unmounts the SD card and jumps to the application reset
   vector stored at `0x08080000`. If the application partition is empty (erased
   flash) the boot loader stops and waits.

The update package is created by `make build-app` (`build/project-app.fpkg`;
copy it to `vendor/` on the SD card) from the content of the `app-dist` directory (manifest and other files) and the
application binaries (`app-kernel.bin`, `app-appset.bin`, `app-romfs.bin`).

### Programming
Whole flash (boot loader and application) can be programmed by:
```
make build-all
make install
```
Later, during application development, only the application section (or a
single partition of it) can be programmed, the boot loader stays untouched:
```
make build-app
make install-app        # or install-app-kernel, install-app-appset, install-app-romfs
```
