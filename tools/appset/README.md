# Appset build base

The directory contains everything that is common to all application sets
(appsets) of the workspace: the Makefile, the default appset configuration
and the build scripts. An appset directory (e.g. `app-appset`, `btl-appset`)
contains only its programs and libraries:
```
<appset>/
  programs/
    <name>.mk           program from common_apps/<name>
    <name>/Makefile.in  local program of the appset
  libs/
    <name>.mk           library from common_libs/<name>
    <name>/Makefile.in  local library of the appset
```

Program from `common_apps` (the same for libraries, `LIB_ROOT_<NAME>` and
`$(COMMON_LIBS)`):
```make
# Makefile for GNU make

PROG_ROOT_CAT = $(COMMON_APPS)/
include $(COMMON_APPS)/cat/Makefile.in
```

Local program (`PROG_ROOT_<NAME>` is set by the build to `<appset>/programs/`,
the name is upper case, `-` and `.` are replaced by `_`):
```make
CSRC_PROGRAMS   += $(PROG_ROOT_INITD)initd/initd.c
CXXSRC_PROGRAMS +=
HDRLOC_PROGRAMS +=
```

All programs of the appset are built except programs listed in
`__PROGRAMS_EXCLUDE_LIST__` of the configuration
(`<config>/programs/programs_flags.h`).

## Usage
The appset is built by the top-level targets of the workspace
(`make build-app-appset`, `make build-btl-appset`, `make build-sim-appset`),
which call:
```
make -C tools/appset all APPSET_DIR=<appset> [CONFIG_DIR=<config>] [BUILD_DIR=<build>] [BOARD_NAME=...] [PROJECT_OPTS=...]
```

| Variable | Description |
|----------|-------------|
| `APPSET_DIR` | appset directory (`programs/`, `libs/`), required to build |
| `CONFIG_DIR` | appset configuration, default: `tools/appset/config` |
| `BUILD_DIR` | build directory (objects, generated files, `output/appset.{elf,hex,bin,o}`), default: `build/<appset name>` |
| `TABLE_SYMBOL` | POSIX simulator: global symbol of the program table in `output/appset.o`, default: `_program_table_desc` (main appset), other appsets linked with the same kernel use unique names (`SIM_APPSETS` of the workspace) |

Relative paths are relative to the workspace directory. The workspace build
uses `config/<name>` and `build/<name>` (e.g. `config/sim-appset` and
`build/sim-appset` for the simulator appset built from `app-appset`), so
appsets built from the same directory with different configurations do not
share objects.

Targets:

| Target | Description |
|--------|-------------|
| `all` | build appset |
| `clean` | remove build directory |
| `config` | configuration (configtool), without `CONFIG_DIR`: default configuration |
| `check`, `checkprog`, `quickcheck` | static code analysis (cppcheck) |
| `rungens` | start generator scripts |
| `help` | list of targets |

## Files
- `Makefile` – build of the appset,
- `config/` – default appset configuration (template of `config/*-appset`,
  used by `tools/config_sync.sh` and BSP import),
- `addapps.sh` – generates the program registration table
  (`program_registration.c`), the makefile of programs and libraries and the
  linker script (`<build>/program/`),
- `memory_layout.ld` – memory layout of the appset image,
- `find_global_vars.sh` – checks that programs and libraries do not contain
  global variables (each program can run in many instances),
- `rungens.sh` – starts generator scripts.

Paths of sources are absolute, objects are placed in
`<build>/<arch>/obj/<path in workspace>`, file names (`__FILE__`, debug
information) are relative to the workspace directory.

## ROMFS
The ROMFS image is built by `tools/romfs` in the same way:
```
make -C tools/romfs all ROMFS_DIR=app-romfs [CONFIG_DIR=config/app-romfs] [BUILD_DIR=build/app-romfs]
```
`ROMFS_DIR` is the directory with files of the image, the default
configuration is `tools/romfs/config`.
