# AGENTS.md — dnx RTOS workspace

Guidance for AI coding agents working in this repository. Read it before
changing code, configuration or build scripts.

## What this repository is

`dnx-rtos-workspace` is a reference/multi-project workspace for **dnx RTOS** —
a Unix-like OS layer on top of FreeRTOS for 32-bit microcontrollers (STM32
F1/F3/F4/F7/H7, RP2040 `rpcm0`). One workspace builds several flash partitions
that can be built and flashed separately:

| Partition     | Sources                         | Config               |
|---------------|---------------------------------|----------------------|
| app kernel    | `kernel/` (git submodule)       | `config/app-kernel/` |
| app appset    | `app-appset/`                   | `config/app-appset/` |
| app romfs     | `app-romfs/`                    | `config/app-romfs/`  |
| bootloader kernel | `kernel/` (same submodule)  | `config/btl-kernel/` |
| bootloader appset | `btl-appset/`               | `config/btl-appset/` |

The default configuration targets **stm32h7** (`__CPU_ARCH__` in
`config/*/project/project_flags.h`).

## Directory map

- `kernel/` — **git submodule** (`dnx-rtos-kernel`). Kernel, drivers, file
  systems, network, CPU ports, FreeRTOS. Changes here are committed in the
  submodule repository, not in the workspace.
  - `src/kernel/` core (process, syscalls, kworker, FreeRTOS + ports)
  - `src/drivers/<name>/` drivers: `<name>_ioctl.h`, `Makefile`, and per-arch
    subdirs (`noarch/`, `stm32f4/`, …)
  - `src/fs/<name>/` file systems (ramfs, romfs, fatfs, ext4fs, eefs, flatfs,
    procfs, appsetfs), `vfs.c`, `pipe.c`
  - `src/net/` (inet, cannet, sipc, uds), `src/mm/`, `src/cpu/<arch>/`
  - `src/include/` internal kernel headers (`drivers/driver.h`, `fs/fs.h`, …)
  - `config/` default kernel config (the workspace overrides it via `config/app-kernel`)
  - `tools/` configtool-cli (Lua), `mkfpkg.py`, `syncdir.py`, helper scripts
- `app-appset/`, `btl-appset/` — user-space application sets (only programs
  and libraries, built by `tools/appset`; the simulator appset is built from
  `app-appset` with `config/sim-appset`)
  - `programs/<prog>.mk` pulls in a program from `common_apps/`;
    `programs/<prog>/` program specific to this appset (`initd`, `siminit`,
    `bootloader`)
  - `libs/<lib>.mk` pulls in libraries from `common_libs/`; `libs/liblocal/` local lib
- `tools/appset/` — appset build base: `Makefile` (`APPSET_DIR`, `CONFIG_DIR`,
  `BUILD_DIR`), default appset configuration `config/`, `addapps.sh` (generates
  program registration), `find_global_vars.sh`, `memory_layout.ld`.
- `tools/romfs/` — ROMFS build base: `Makefile` (`ROMFS_DIR`, `CONFIG_DIR`,
  `BUILD_DIR`) and default ROMFS configuration `config/`.
- `tools/drvset/` — drvset build base (drivers built separately from the
  kernel, flash partition): `Makefile` (`DRVSET_DIR`, `CONFIG_DIR` = kernel
  configuration, `BUILD_DIR`, `check` target; modules `DRVSET_MODULES`,
  file systems `DRVSET_FILESYSTEMS`), `ldregion.awk`,
  `drvset_check.py` (offline check against kernel image), example driver
  `example/`, tests `test/`; ABI parts are in `kernel/src/drvset/`. Top-level
  targets `build-app-drvset`, `test-drvset`.
- `common_apps/` — programs shared by appsets, one directory per program
  (`Makefile.in` + sources).
- `common_libs/` — libraries shared by appsets: `libc` (the dnx user-space
  libc: `stdio.h`, `dnx/misc.h`, `dnx/os.h`, `dnx/thread.h`, …), `bs`, `lua`,
  `zlib`, `llist`, `fpkg`, `crc`, `md5`, `sha*`, `http-parser`, `libmqtt`, …
- `config/` — the actual configuration of each partition (see below).
- `bsp/` — exported board configurations (`*.bsp`), applied with `tools/bsp_import.sh`
  (directories of the BSP are reset to defaults before import); `bsp/posix-sim.bsp`
  holds the simulator configuration (`config/sim-*`). `tools/bsp_export.sh <bsp>
  ["app*|btl*"]` exports selected directories, each optimized for its own
  `__CPU_ARCH__`; `tools/config_sync.sh` re-syncs all configs and BSPs with the
  defaults (run it after adding a configuration flag).
- `app-romfs/` — files packed into the ROMFS image.
- `sim-env/` — POSIX simulator environment: board models (`boards/`), device
  models (`models/`), tests and demos executed in dnx (`tests/`, `demos/`),
  pages of the board panel (`panel/`); API in `sim-env/README.md`.
- `app-dist/` — manifest for the `.fpkg` update package.
- `tools/` — Dockerfile, flashing (`flash.sh`, `runocd.sh`), linker-script
  generator, BSP import/export, simulator tests (`simtest.py`) and board panel
  (`simpanel.py`).
- `build/` — output (git-ignored): images and build directories of appsets and
  romfs (`build/<config name>/`). Never edit or commit.

## Build environment

Builds require `arm-none-eabi-gcc` 10.3-2021.10, GNU make, `srec_cat`,
Python 3, Lua 5.4+ with LuaFileSystem and `dialog` (configtool). The supported environment is the
Docker image defined in `tools/Dockerfile`:

```sh
cd tools && ./build_docker_image.py      # once: build the image
./run_container.py                        # interactive shell in the container
./run_container.py make build-app         # run one command in the container
```

`run_container.py` creates the container on its first call and then always
opens an interactive shell, ignoring any arguments. A command passed as
arguments runs only once the container already exists. After running it, the
script stops the container. The image needs network access to Docker Hub,
developer.arm.com and sourceforge.net.

If the toolchain is not available in your environment, do not try to install a
different GCC version or "fix" the Makefiles to work around it — say that the
build could not be verified.

## Build commands (top-level `Makefile`)

```sh
make help               # list targets
make build-app          # app kernel + appset + romfs -> build/project_app.hex, project-app.fpkg
make build-btl          # bootloader kernel + appset
make build-all          # bootloader + application in one hex
make build-app-kernel   # single partition (also build-app-appset, build-app-romfs, build-btl-*)
make clean
make install[-app|-btl|-app-kernel|-app-appset|-app-romfs]   # flash via OpenOCD/GDB
make config-app-kernel  # interactive configtool (also config-app-appset, config-btl-*)
```

Notes:
- The kernel is shared by app and bootloader; `build-*-kernel` always cleans
  the kernel first and builds it with `CONFIG_DIR=../config/<name>`.
- Kernel build exports generated ioctl headers to `build/ioctl/`.
- Appset build (`make -C tools/appset all APPSET_DIR=<appset>
  CONFIG_DIR=config/<name> BUILD_DIR=build/<name>`, relative paths are relative
  to the workspace) runs `addapps.sh`, which generates `program_registration.c`,
  the program Makefile and the linker script in `build/<name>/program/`. Do not
  hand-edit generated files. Do not add Makefiles, configuration or scripts to
  appset directories: everything common belongs to `tools/appset`.
- `install*` and `config-*` targets need hardware / an interactive terminal —
  do not run them unless the user asks.
- Static analysis: `make check` inside `kernel/`, `make -C tools/appset check`
  / `checkprog` with `APPSET_DIR=app-appset CONFIG_DIR=config/app-appset`
  (cppcheck).
- `kernel/tools/tests.sh` builds the kernel for all test configurations in
  `kernel/tools/test_bsp/` and restores the kernel's default `config/`.
- `make test-drvset` (`tools/drvset/test/test.sh`) builds the test kernel
  (`config/app-kernel` + drvset and IRQ), drvset partitions (example, test
  drivers, negative tests) and checks them by `tools/drvset/drvset_check.py`
  (no hardware); output in `build/drvset-test`.

## POSIX simulator

- `make build-sim` builds dnx RTOS as a Linux program `build/dnx-sim`
  (kernel `__CPU_ARCH__ posix`, configs `config/sim-kernel`, `config/sim-appset`,
  `config/sim-romfs`, init program `siminit`). `make run-sim` starts it.
- Toolchain: `i686-unknown-linux-musl-` (static 32-bit executable). The kernel
  and appset assume 32-bit pointers, so the simulator is always built as i386.
  dnx code is compiled with `-funsigned-char` (the same `char` as on ARM).
- Kernel parts: `src/cpu/posix` (`cpuctl.c`, `host/host.c`), FreeRTOS port
  `src/kernel/dnx-ports/GCC/Posix`, drivers `uart/posix`, `gpio/posix`,
  `i2c/posix`, `spi/posix`, `ltdc/posix`, `eth/posix`, `hostdisk/posix`
  (optional emulated block size and buffer alignment constraints for tests of
  the block layer), `rtc/posix` (host clock + offset), file system `fs/hostfs` (host directory selected by
  `--hostfs`, mounted by `siminit` in `/host`; paths cannot leave this
  directory). Files using host headers are listed as `CSRC_HOST` and
  compiled without dnx `lib/` headers (dnx redefines `pid_t`, `time_t`,
  `struct timeval`, ...). The only interface between both sides is
  `src/cpu/posix/host/host.h` (basic C types only). Host calls are short,
  non-blocking and executed in the critical section.
- `--uart <n>=pty[:<link>]|<device>|board` connects UART0..3 to PTY, host
  serial port or board model (UART0 is the simulator terminal by default).
  `uart/posix` sends data in chunks (writer or service thread), host writes
  are non-blocking except the simulator terminal. `siminit` initializes
  `/dev/ttyS1..3` (raw, no shell). UARTs not selected by `--uart` can be
  attached by board scripts (`uart.attach("UART:1", dev)`, `"UART:<n>"` =
  `/dev/ttyS<n>`): `board.c` keeps RX/TX FIFOs with reception times
  (transmission time from baud rate and frame configured by the system, 10 ms
  transmitter window for the 5 ms polling of `uart/posix`, receiver overrun
  after 200 ms), the configuration is passed to the model
  (`dev:configure(cfg)`), data to the model by `dev:receive(data)`, from the
  model by `uart.send()`. Model of GPS: `sim-env/models/nmea_gps.lua`.
- `--tap <name>` connects `eth/posix` (`/dev/eth`, frames via
  `/dev/net/tun`, software MAC filter) to the host TAP interface; `siminit`
  starts `netmgr --daemon /etc/netmgr.conf` (ROMFS content `app-romfs`
  is shared with the application, `config/sim-romfs` sets image parameters).
- Drivers are configured by structures (`IOCTL_<DRV>__CONFIGURE`) or by
  configuration strings (`IOCTL_<DRV>__CONFIGURE_STR` = `IOCTL_DEVICE__CONFIGURE_STR`,
  "key=value,key=value", parsed by `sys_opt_*()`, enumerations by
  `sys_opt_get_enum()`); from shell: `modprobe -i <drv> <major> <minor> <path>
  -c <config>` (new device) or `modprobe -c <path> <config>` (existing device).
- `make test-sim` (`tools/simtest.py`) runs driver configuration string tests
  (`devcfg`), board tests (`sim-env/tests/*.lua`: GPIO, I2C, I2CEE, DS2482, SPI,
  SPIEE, MCP23S17, UART (echo, NMEA GPS), SDSPI, board panel protocol with LCD and touch), the block layer test
  (`blkdev`: host disk with emulated constraints), `fstest` (ramfs, hostfs,
  FAT/ext4 image, flatfs/eefs/ext2/ext3 partitions formatted in dnx, ext2 on
  the SD card model and on the constrained disk, read-only romfs image),
  `udstest`, drvset partition (`--drvset`: example driver, fstest on its file
  system `dsramfs`, incorrect image) and
  `inettest` (`--tap`) in the simulator and checks their
  `SUMMARY:` lines (ext2/ext3/ext4/FAT images are checked by e2fsck/fsck.fat);
  use it to verify kernel/appset changes on PC. A test name filter runs a
  subset, e.g. `make test-sim SIMTEST_OPTS=board`.
- `--board <script>` loads the board model: kernel
  `src/cpu/posix/host/board.c` (interface `board.h`) with unmodified Lua 5.5
  sources in `src/cpu/posix/host/lua` (`CSRC_HOST_LIB`: compiled for the
  host without warnings, do not edit them). Board scripts, device models and
  tests are in the workspace `sim-env`. Lua runs in its own host thread,
  system events are queued. `gpio/posix` reports output changes and
  reads pins driven by the board. `i2c/posix` (master only) forwards bus
  primitives (start, address, write, read, stop) as synchronous requests to
  the board thread (Lua device models `i2c.attach()`, `sim-env/models`);
  without the board buses are empty (address NACK -> ENXIO). Devices are
  created by `modprobe -i I2C <bus> <n> <path> -c <config string>`. I2C
  device addresses are 7-bit (as in datasheets, e.g. EEPROM `address=0x50`)
  in the driver configuration and in board models; 10-bit addresses use
  `addr_10bit`.
  `spi/posix` sends each transceive as one synchronous request (mode and bit
  order in flags); CS is a GPIO pin set by the generic SPI driver, the board
  thread handles queued GPIO events before a request, so models see CS
  changes in order (`spi.attach(bus, cs, dev)`). Device models: `sim-env/models`. Boards in
  `sim-env/boards`, tests run in dnx in `sim-env/tests`, API in
  `sim-env/README.md`.
- `--panel <socket>` (needs `--board`) creates a Unix socket of the board
  panel; `tools/simpanel.py` (Python standard library only) connects to it
  and serves the panel in a web browser (`make run-sim PANEL=1
  SIM_OPTS="--board ..."`, http://localhost:8080). Protocol: JSON lines
  (signal definitions and changes, LCD frames) from the simulator, `set
  <id> <value>` and `touch <x> <y> <0|1>` lines to the simulator. Board
  scripts define signals (`panel.output/input/value/control`,
  `panel.on_touch`), pages in `sim-env/panel/<page>/` define the look
  (`/panel.js`: `data-signal`, `data-input`, `<canvas data-lcd>`).
  `ltdc/posix` registers its framebuffer by `_host_lcd_attach()`; an LCD
  thread in `board.c` sends changed lines (base64) every 50 ms. Touch goes to
  the board script, usually to the FT5336 model (`sim-env/models/ft5336.lua`)
  read by the system through the `I2C` driver.
- `--drvset <file>` maps a drvset partition image (`make build-sim-drvset`,
  `tools/drvset` for `config/sim-kernel`, i386 stubs of `ksymstub.S`) at
  `__OS_DRVSET_FLASH_ADDRESS__` (0x50000000) and its RAM after it
  (`_host_drvset_map()`); the kernel stores the symbol table address in the
  first word of that RAM. Interrupts are not emulated (partitions with
  interrupt handlers are rejected). Modules are initialized by `modprobe -i`.
- `siminit` connects `--disk 1..3` as `/dev/sda..sdc` with MBR partitions
  (`PART`) and mounts FAT/ext4 in `/mnt/sdXn` (or whole disk in `/mnt/sdX`).
- Appset is linked as a relocatable object (`ld -r`), only
  `_program_table_desc` stays global, so appset symbols do not conflict with
  kernel and host library symbols. libc calls kernel by `_dnx_syscall` when
  `__LIBC_DNX_RTOS_ADDRESS__` is 0.
- More appsets in the simulator: `make build-sim SIM_APPSETS="<appset>[:<config>] ..."`
  (default config `config/sim-appset`, build `build/sim-<appset name>`). Other
  appsets get the table symbol `_program_table_desc_<appset name>`
  (`TABLE_SYMBOL` of `tools/appset`); when `__PROJ_APPSET_TABLE_ADDRESS__` is 0
  the generated `program_registration.c` puts a reference of the table (table,
  size, appset name) in the `dnx_appset_tables` section, `siminit` creates a
  `MEMBLOCK` device of each table (`/dev/appset<n>`) and mounts it by
  `appsetfs` in `/opt/<appset name>` (the main appset in `/bin`), as appset
  partitions are mounted on the microcontroller. Programs started by name are
  searched in the main appset, then in `__OS_PROGRAM_PATH__` of
  `config/sim-kernel` (`/bin:/opt/testappset`). `make test-sim` adds the test
  appset `sim-env/testappset` (`appsettest`, tests `appsets: testappset` and
  `appsetfs: mount/umount`; they are skipped when the simulator was built
  without it).
- A program fault (SIGSEGV, ...) is reported as APP CRASH like a CPU exception
  on the microcontroller; the simulator keeps running.

## Configuration files (important)

Files under `config/**` (`config.h`, `*_flags.h`) are **hybrid files read by
both C preprocessor and make**. Rules:

- Each flag is named `__FLAG_NAME__` and is defined twice: as a C `#define`
  and as a make assignment inside a `#/* … #*/` block. Keep both in sync.
- Boolean flags use `_YES_` / `_NO_`.
- Comments starting with `#` contain configtool (Lua) metadata, e.g.
  `# uC.ARCH = this:GetFlagValue(...)`. Do not reformat or remove them.
- Prefer changing configuration through configtool (`make config-…`); when
  editing by hand, edit only flag values and preserve the file layout.
- Do not run `clang-format` on configuration files.
- Workspace configuration lives in `config/`; `kernel/config/` is only the
  kernel's default and is not used by the workspace build.
- Program selection: `__PROGRAMS_EXCLUDE_LIST__` in
  `config/*-appset/programs/programs_flags.h`.

## Coding style

- Formatting is defined by `.clang-format` (WebKit base, **8-space indent**,
  110 columns, braces always inserted, `PointerAlignment: Right`, includes not
  sorted). Run `clang-format -i <file>` on C/C++ files you change; do not
  reformat files you did not otherwise touch, and never format third-party
  code (FreeRTOS, FatFs, lwext4, Lua, zlib, …).
- C18 for C, C++11 for C++ (no RTTI, no exceptions). Builds use `-Os`,
  `-Wall -Wextra` and many `-Werror=…` flags — code must compile warning-free.
- Follow the existing file template: header block (`File/Author/Brief` +
  license), then sections `Include files`, `Local symbolic constants/macros`,
  `Local types, enums definitions`, `Local function prototypes`,
  `Local object definitions`, `Exported object definitions`,
  `Function definitions`.
- Every function has a Doxygen comment framed with `//====…` lines
  (`@brief`, `@param[in]/[out]`, `@return`).
- Licenses: kernel files use GPL with the dnx RTOS exception; appset programs
  and newer files use MIT. Keep the license of the file you edit; ask before
  choosing a license for a new file if unclear.
- Code, comments and commit messages are in English. Commit messages are short
  sentences ending with a period, e.g. `Fix in the VFS.`, `Improvements in df.`

## Kernel-side conventions (`kernel/src`)

- Drivers implement the `API_MOD_*` interface from `drivers/driver.h`
  (`API_MOD_INIT/RELEASE/OPEN/CLOSE/WRITE/READ/IOCTL/FLUSH/STAT`); file
  systems implement `API_FS_*` from `fs/fs.h`.
- Functions return an errno value (`ESUCC` on success), not -1/0.
- Use kernel services `sys_*` (`sys_malloc/sys_zalloc/sys_free`,
  `sys_mutex_*`, `sys_semaphore_*`, `sys_queue_*`, `sys_sleep_ms`,
  `sys_critical_section_*`, …) — never the user-space libc or raw FreeRTOS
  calls from drivers/fs. Mark unused args with `UNUSED_ARG1/2/…`.
- ioctl requests are defined in `<driver>_ioctl.h` with `_IO/_IOR/_IOW/_IOWR`
  as `IOCTL_<DRV>__<NAME>`.
- Drivers and file systems are discovered from directory structure by
  `drivers/adddriver.sh` and `fs/addfs.sh`; each module needs its `Makefile`
  with `__ENABLE_<NAME>__` guards and matching config flags in
  `config/arch/...` or `config/filesystems/...`. Use existing modules (e.g.
  `drivers/zero`, `fs/ramfs`) as templates.

## Appset-side conventions (programs and libraries)

- A program shared by appsets is a directory `common_apps/<name>/` with a
  `Makefile.in` listing sources:
  ```make
  CSRC_PROGRAMS   += $(PROG_ROOT_<NAME>)<name>/<name>.c
  CXXSRC_PROGRAMS +=
  HDRLOC_PROGRAMS +=
  ```
  An appset contains it by `programs/<name>.mk` (add the file to each
  appset that should have the program):
  ```make
  PROG_ROOT_<NAME> = $(COMMON_APPS)/
  include $(COMMON_APPS)/<name>/Makefile.in
  ```
  A program specific to one appset is a directory `programs/<name>/` with
  the same `Makefile.in` (`PROG_ROOT_<NAME>` is set to `<appset>/programs/` by
  the generated Makefile). `addapps.sh` registers
  all programs found in `programs/` except `__PROGRAMS_EXCLUDE_LIST__`.
  Sources are compiled with absolute paths, `__FILE__` is relative to the
  workspace (`-ffile-prefix-map`).
- Declare the program with `PROGRAM_PARAMS(<name>, STACK_DEPTH_…);` and an
  ordinary `int main(int argc, char *argv[])`. The program name (command) is
  the `.mk`/directory name and may contain `.` and `-` (`mkfs.eefs`); the
  `PROGRAM_PARAMS` identifier is the name with other characters than letters,
  digits and `_` replaced by `_` (`mkfs_eefs`, `addapps.sh`).
- **No global or static variables in programs and libraries.** Each program
  may run in many instances, so all state goes into
  `GLOBAL_VARIABLES_SECTION { … };` (accessed through `global->…`) or into
  dynamically allocated memory. The build enforces this with
  `tools/appset/find_global_vars.sh` (any `.bss`/`.data` symbol fails the build).
  `const` data in flash is fine.
- Programs use the dnx libc from `common_libs/libc` (POSIX-like API,
  `<dnx/os.h>`, `<dnx/misc.h>`, `<dnx/thread.h>`). Mind the small stack and
  heap — avoid large local arrays; prefer `malloc` or `global->` buffers.
  Big I/O buffers of block device or file transfers should be allocated by
  `posix_memalign()` with alignment `sysconf(_SC_LEVEL1_DCACHE_LINESIZE)`
  (at least `sizeof(void *)`), so DMA transfers on CPUs with data cache
  (STM32F7/H7) do not need kernel bounce buffers (see `dd`, `cp`).
- Atomic test and set of libc (futex, heap lock: `libc/source/atomic.h`):
  C11 `atomic_flag` (ldrex/strex) on CPUs with `__ARM_FEATURE_LDREX`, kernel
  `SYSCALL_ATOMICTAS` on Cortex-M0+ (compiler generates not atomic code). A
  multicore port whose ldrex/strex do not work between cores sets
  `-D__DNX_ATOMIC_KERNEL__` in `CPUCONFIG_CFLAGS` of both the kernel config
  (`cpu_flags.h`) and the appset projects (`project_flags.h`); libc checks
  the kernel at heap creation (mismatch: `malloc()` fails, `ENOTSUP`).
- A thread created with `detached = false` (or `attr == NULL`) must be joined
  by `thread_join()`/`thread_join2()`, also after `thread_cancel()`: its thread
  ID and slot stay reserved until join. Use `detached = true` for threads that
  are never joined. A killed process (`process_kill()`) that is not detached
  must be waited for by `process_wait()`, like a process that finished itself;
  its exit status is `PROCESS_STATUS_KILLED` (137).
- A shared library lives in `common_libs/<lib>/` with a `Makefile.in`
  (`CSRC_LIB`, `CXXSRC_LIB`, `HDRLOC_LIB`) and is linked into an appset by
  `libs/<lib>.mk` (`LIB_ROOT_<NAME> = $(COMMON_LIBS)/`,
  `include $(COMMON_LIBS)/<lib>/Makefile.in`). Changes in `common_libs` affect
  both appsets.
- Full-screen terminal programs (`nano`, `mc`, `dialog`, `tuidemo`) use
  `common_libs/libtui` (`tui.h`): tty raw mode (`IOCTL_TTY__RAW_MODE_ON/OFF`),
  screen buffers with differential refresh (slow serial links), key decoder
  (xterm/VT100/minicom sequences, Esc+digit as F key), modal boxes
  (`tui_msgbox`, `tui_question`, `tui_inputbox`, …) and `tui_suspend`/
  `tui_resume` for starting other programs, `tui_scroll` (full-width scroll
  hint: terminal scrolls rows instead of sending them). Blank areas are
  sent as screen clear / erase to end of line with background color (BCE
  terminals; `tui_set_bce(tui, false)` limits it to default background).
  `keytest` shows raw key codes.
  libtui and its programs can be built and tested on Linux with
  `-DTUI_PORT_POSIX` (termios port); dnx program macros need a small shim.
- The kernel splits a command line into arguments with shell quoting rules
  (single/double quotes, backslash; quoted parts are joined). `system()` passes
  the command to `__LIBC_SYSTEM_PROG__` (`dsh -e`) as one single-quoted
  argument.
- Lua (third-party, keep changes minimal): `common_libs/lua/ldnxlib.c` adds
  `io.popen()` (program started without shell; output/input through a
  temporary file in `/run`, so it is not streamed). configtool runs on the
  target with `lua configtool-cli.lua <config.h>` and the `dialog` program.
- `common_libs/luadnx` (MIT) contains Lua bindings to dnx services, preloaded
  by the `lua` program with `luadnx_openlibs()`: `lfs` (LuaFileSystem 1.8
  compatible), `dnx.sys` (system info, memory, CPU load, syslog, reboot,
  sleep), `dnx.proc` (spawn/run/wait/kill, process and thread statistics) and
  `dnx.spi` (`transceive(path, frame, ...)`: full duplex SPI transfer by
  `IOCTL_SPI__TRANSCEIVE`, all frames in one CS activation; `ENOTSUP` when
  the kernel is built without the SPI driver). Functions return
  `nil, message, errno` on error. Do not duplicate Lua standard library
  functionality there. Test script:
  `common_libs/luadnx/test/luadnx_test.lua` (run on target).

## Working rules for agents

- Keep changes minimal and focused; do not refactor or reformat unrelated code.
- Never modify `build/`, generated files, or third-party sources unless the
  task explicitly requires it (third-party forks such as FatFs or lwext4 carry
  local patches — read before editing).
- Changes in `kernel/` belong to the submodule — point that out to the user and
  do not update the submodule pointer in the workspace unless asked.
- Do not change memory layout (`custom_linker_script.*`, partition addresses
  in `project_flags.h`) without being asked — wrong values brick the flash
  layout (partitions must be sector-aligned).
- After changes, build the affected partition (`make build-app-appset`,
  `make build-app-kernel`, …) if the toolchain is available, and report the
  result. If you cannot build, state it explicitly.
