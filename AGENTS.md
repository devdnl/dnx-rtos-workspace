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
| app romfs     | `app-romfs/content/`            | `config/app-romfs/`  |
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
    procfs), `vfs.c`, `pipe.c`
  - `src/net/` (inet, cannet, sipc, uds), `src/mm/`, `src/cpu/<arch>/`
  - `src/include/` internal kernel headers (`drivers/driver.h`, `fs/fs.h`, …)
  - `config/` default kernel config (the workspace overrides it via `config/app-kernel`)
  - `tools/` configtool-cli (Lua), `mkfpkg.py`, `syncdir.py`, helper scripts
- `app-appset/`, `btl-appset/` — user-space application sets
  - `src/programs/<prog>/` one directory per program (`Makefile.in` + sources)
  - `src/libs/<lib>.mk` pulls in libraries from `common_libs/`; `src/libs/liblocal/` local lib
  - `tools/` `addapps.sh` (generates program registration), `find_global_vars.sh`
- `common_libs/` — libraries shared by appsets: `libc` (the dnx user-space
  libc: `stdio.h`, `dnx/misc.h`, `dnx/os.h`, `dnx/thread.h`, …), `bs`, `lua`,
  `zlib`, `llist`, `fpkg`, `crc`, `md5`, `sha*`, `http-parser`, `libmqtt`, …
- `config/` — the actual configuration of each partition (see below).
- `bsp/` — exported board configurations (`*.bsp`), applied with `tools/bsp_import.sh`.
- `app-romfs/content/` — files packed into the ROMFS image.
- `app-dist/` — manifest for the `.fpkg` update package.
- `tools/` — Dockerfile, flashing (`flash.sh`, `runocd.sh`), linker-script
  generator, BSP import/export.
- `build/` — output (git-ignored). Never edit or commit.

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
- Appset build runs `addapps.sh`, which generates `program_registration.c`
  and the program Makefile in `app-appset/build/program/`. Do not hand-edit
  generated files.
- `install*` and `config-*` targets need hardware / an interactive terminal —
  do not run them unless the user asks.
- Static analysis: `make check` inside `kernel/`, `make check` / `make checkprog`
  inside `app-appset/` (cppcheck).
- `kernel/tools/tests.sh` builds the kernel for all test configurations in
  `kernel/tools/test_bsp/` and restores the kernel's default `config/`.

## POSIX simulator

- `make build-sim` builds dnx RTOS as a Linux program `build/dnx-sim`
  (kernel `__CPU_ARCH__ posix`, configs `config/sim-kernel`, `config/sim-appset`,
  `config/sim-romfs`, init program `siminit`). `make run-sim` starts it.
- Toolchain: `i686-unknown-linux-musl-` (static 32-bit executable). The kernel
  and appset assume 32-bit pointers, so the simulator is always built as i386.
  dnx code is compiled with `-funsigned-char` (the same `char` as on ARM).
- Kernel parts: `src/cpu/posix` (`cpuctl.c`, `host/host.c`), FreeRTOS port
  `src/kernel/dnx-ports/GCC/Posix`, drivers `uart/posix`, `gpio/posix`,
  `hostdisk/posix`, file system `fs/hostfs` (host directory selected by
  `--hostfs`, mounted by `siminit` in `/host`; paths cannot leave this
  directory). Files using host headers are listed as `CSRC_HOST` and
  compiled without dnx `lib/` headers (dnx redefines `pid_t`, `time_t`,
  `struct timeval`, ...). The only interface between both sides is
  `src/cpu/posix/host/host.h` (basic C types only). Host calls are short,
  non-blocking and executed in the critical section.
- Appset is linked as a relocatable object (`ld -r`), only
  `_program_table_desc` stays global, so appset symbols do not conflict with
  kernel and host library symbols. libc calls kernel by `_dnx_syscall` when
  `__LIBC_DNX_RTOS_ADDRESS__` is 0.
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
- Program selection: `__PROGRAMS_LIST__` / `__PROGRAMS_EXCLUDE_LIST__` in
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

- A program is a directory in `src/programs/<name>/` with a `Makefile.in`
  listing sources:
  ```make
  CSRC_PROGRAMS   += $(PROG_ROOT_<NAME>)<name>/<name>.c
  CXXSRC_PROGRAMS +=
  HDRLOC_PROGRAMS +=
  ```
- Declare the program with `PROGRAM_PARAMS(<name>, STACK_DEPTH_…);` and an
  ordinary `int main(int argc, char *argv[])`.
- **No global or static variables in programs and libraries.** Each program
  may run in many instances, so all state goes into
  `GLOBAL_VARIABLES_SECTION { … };` (accessed through `global->…`) or into
  dynamically allocated memory. The build enforces this with
  `tools/find_global_vars.sh` (any `.bss`/`.data` symbol fails the build).
  `const` data in flash is fine.
- Programs use the dnx libc from `common_libs/libc` (POSIX-like API,
  `<dnx/os.h>`, `<dnx/misc.h>`, `<dnx/thread.h>`). Mind the small stack and
  heap — avoid large local arrays; prefer `malloc` or `global->` buffers.
- A shared library lives in `common_libs/<lib>/` with a `Makefile.in`
  (`CSRC_LIB`, `CXXSRC_LIB`, `HDRLOC_LIB`) and is linked into an appset by
  `src/libs/<lib>.mk`. Changes in `common_libs` affect both appsets.
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
  sleep) and `dnx.proc` (spawn/run/wait/kill, process and thread statistics).
  Functions return `nil, message, errno` on error. Do not duplicate Lua
  standard library functionality there. Test script:
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
