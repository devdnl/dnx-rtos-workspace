# Core of the workspace Makefile for GNU make (tools/workspace.mk)
####################################################################################################
#
# MIT License
# 
# Copyright (C) 2025 Daniel Zorychta <daniel.zorychta@gmail.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
####################################################################################################

#
# This file belongs to the workspace and is updated with it, do not edit it. The project Makefile
# sets its settings (variables of this file defined by ?=) and includes this file:
#
#     PROJECT = myproject
#     SIM_APPSET_DIR = my-appset
#     include tools/workspace.mk
#
# and then defines targets of its partitions (kernel, appsets, romfs) by the macros of this file
# (build_kernel, build_appset, build_romfs, install_hex, config_*). Help of the project is added by
# "help ::" rules, additional cleaning by "clean ::" rules (run after cleaning of this file).
####################################################################################################

#---------------------------------------------------------------------------------------------------
# SETTINGS (defaults, set in the project Makefile before include)
#---------------------------------------------------------------------------------------------------
PROJECT         ?= project
TOOLCHAIN       ?= arm-none-eabi-
TARGET_DIR_NAME ?= build

# POSIX simulator: appset and romfs directories of the simulator system (sources of the project),
# configuration names (config/<name>)
SIM_APPSET_DIR    ?= app-appset
SIM_ROMFS_DIR     ?= app-romfs
SIM_KERNEL_CONFIG ?= sim-kernel
SIM_APPSET_CONFIG ?= sim-appset
SIM_ROMFS_CONFIG  ?= sim-romfs

# POSIX simulator: board panel (run-sim PANEL=1, run-sim-example)
SIM_PANEL_SOCKET ?= $(TARGET_DIR_NAME)/dnx-panel.sock
SIM_PANEL_PORT   ?= 8080

# run-sim-example: options of simulator (example board, files of sim-env in /host, network tap0)
SIM_EXAMPLE_OPTS ?= --board sim-env/boards/example.lua --hostfs host=sim-env --tap tap0

# POSIX simulator: other appsets linked with the simulator kernel (siminit mounts them by appsetfs
# in /opt/<appset directory name>, programs are started by path): list of <appset directory>
# [:<configuration directory>] (relative to the workspace or absolute), default configuration is
# config/sim-appset, e.g. SIM_APPSETS="btl-appset /home/user/project/appset:config/sim-appset";
# appset is built in $(TARGET_DIR_NAME)/sim-<appset directory name>, so names must be unique
SIM_APPSETS ?=

# drvset partition of POSIX simulator (option --drvset of simulator, used by test-sim): example
# driver and file system dsramfs
SIM_DRVSET_DIR ?= tools/drvset/test/sim

# test-sim: the test appset (programs appsettest and echo) is always linked with the simulator
ifneq ($(filter test-sim,$(MAKECMDGOALS)),)
override SIM_APPSETS += sim-env/testappset
endif

#---------------------------------------------------------------------------------------------------
# BASIC PROGRAMS DEFINITIONS
#---------------------------------------------------------------------------------------------------
SHELL           = /bin/bash
ECHO           ?= /bin/echo -e
REMOVE         ?= /bin/rm -rf
COPY           ?= /bin/cp -r
RENAME         ?= /bin/mv
MOVE           ?= /bin/mv
MKDIR          ?= /bin/mkdir -p
CAT            ?= /bin/cat
SREC_CAT       ?= /bin/srec_cat
GREP           ?= /bin/grep
SIZEOF         ?= /usr/bin/stat -c %s
WC             ?= /usr/bin/wc
PYTHON3        ?= /usr/bin/python3
MD5SUM         ?= /usr/bin/md5sum
OBJCOPY        ?= $(TOOLCHAIN)objcopy
OBJDUMP        ?= $(TOOLCHAIN)objdump
SIZE           ?= $(TOOLCHAIN)size
CONFIGTOOL     ?= $(SHELL) ./kernel/tools/configtool.sh
FLASH_CPU      ?= $(SHELL) ./tools/flash.sh
MKFPKG         ?= $(PYTHON3) ./kernel/tools/mkfpkg.py
APPSET_BASE    ?= tools/appset
ROMFS_BASE     ?= tools/romfs
MKLINKERSCRIPT ?= $(SHELL) ./tools/create_linker_script.sh
SYNCDIR        ?= $(PYTHON3) ./kernel/tools/syncdir.py

#---------------------------------------------------------------------------------------------------
# MACROS
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# define macro used to build kernel according to application name
define build_kernel
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS image..."
	$(MKLINKERSCRIPT) ./config/${$@_APP_NAME}/custom_linker_script.h ./config/${$@_APP_NAME}/custom_linker_script.ld
	$(REMOVE) kernel/build/ioctl
	$(MAKE) -C kernel -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) kernel/build/output/dnx.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) kernel/build/output/dnx.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) kernel/build/output/dnx.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
	$(MKDIR) $(TARGET_DIR_NAME)/ioctl
	$(SYNCDIR) kernel/build/ioctl/ $(TARGET_DIR_NAME)/ioctl/
	$(REMOVE) kernel/build/ioctl
endef

# define macro used to build appset: appset directory (programs, libs), configuration name
# (config/<name>), build directory is $(TARGET_DIR_NAME)/<configuration name>
define build_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(eval $@_BOARD_NAME = $(3))
	$(eval $@_PROJECT_OPTS = $(4))
	$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile all APPSET_DIR=${$@_APP_NAME} CONFIG_DIR=config/${$@_CONFIG_NAME} BUILD_DIR=$(TARGET_DIR_NAME)/${$@_CONFIG_NAME} BOARD_NAME=${$@_BOARD_NAME} PROJECT_OPTS=${$@_PROJECT_OPTS}
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.elf $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.elf
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
endef

# define macro used to build POSIX simulator kernel
define build_sim_kernel
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS simulator kernel..."
	$(REMOVE) kernel/build/ioctl
	$(MAKE) -C kernel -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) kernel/build/output/dnx.a $(TARGET_DIR_NAME)/${$@_APP_NAME}.a
	$(MKDIR) $(TARGET_DIR_NAME)/ioctl
	$(SYNCDIR) kernel/build/ioctl/ $(TARGET_DIR_NAME)/ioctl/
	$(REMOVE) kernel/build/ioctl
endef

# define macro used to build POSIX simulator appset: appset directory, configuration directory,
# build name ($(TARGET_DIR_NAME)/<name> and <name>.o), symbol of program table
define build_sim_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_DIR = $(2))
	$(eval $@_BUILD_NAME = $(3))
	$(eval $@_TABLE_SYMBOL = $(4))
	$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile all APPSET_DIR=${$@_APP_NAME} CONFIG_DIR=${$@_CONFIG_DIR} BUILD_DIR=$(TARGET_DIR_NAME)/${$@_BUILD_NAME} BOARD_NAME=posix PROJECT_OPTS=sim TABLE_SYMBOL=${$@_TABLE_SYMBOL}
	$(COPY) $(TARGET_DIR_NAME)/${$@_BUILD_NAME}/output/appset.o $(TARGET_DIR_NAME)/${$@_BUILD_NAME}.o
endef

# other appsets of the POSIX simulator (SIM_APPSETS): fields of entry, build name, table symbol
sim_appset_dir    = $(word 1,$(subst :, ,$(1)))
sim_appset_config = $(or $(word 2,$(subst :, ,$(1))),config/sim-appset)
sim_appset_name   = sim-$(notdir $(abspath $(call sim_appset_dir,$(1))))
sim_appset_symbol = _program_table_desc_$(subst .,_,$(subst -,_,$(notdir $(abspath $(call sim_appset_dir,$(1))))))
SIM_APPSET_OBJS   = $(foreach a,$(SIM_APPSETS),$(TARGET_DIR_NAME)/$(call sim_appset_name,$(a)).o)

# rule building one of other appsets of the POSIX simulator: entry of SIM_APPSETS
define sim_appset_rule
.PHONY : build-$(call sim_appset_name,$(1))
build-$(call sim_appset_name,$(1)) : _build_prepare
	@if [ ! -d "$(call sim_appset_dir,$(1))" ]; then $$(ECHO) "SIM_APPSETS: appset directory not found: $(call sim_appset_dir,$(1))"; exit 1; fi
	@if [ ! -f "$(call sim_appset_config,$(1))/config.h" ]; then $$(ECHO) "SIM_APPSETS: configuration not found: $(call sim_appset_config,$(1))"; exit 1; fi
	@if [ "$(call sim_appset_name,$(1))" = "sim-appset" ]; then $$(ECHO) "SIM_APPSETS: appset directory name 'appset' is used by the main appset: $(call sim_appset_dir,$(1))"; exit 1; fi
	@$$(ECHO) "Building simulator appset $(call sim_appset_dir,$(1)) ($(call sim_appset_config,$(1)))..."
	@$$(call build_sim_appset, $(call sim_appset_dir,$(1)), $(call sim_appset_config,$(1)), $(call sim_appset_name,$(1)), $(call sim_appset_symbol,$(1)))
endef

# define macro used to create ROMFS: directory with files of the image, configuration name
# (config/<name>), build directory is $(TARGET_DIR_NAME)/<configuration name>
define build_romfs
	$(eval $@_ROMFS_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(MAKE) -C $(ROMFS_BASE) -s -j 1 -fMakefile all ROMFS_DIR=${$@_ROMFS_NAME} CONFIG_DIR=config/${$@_CONFIG_NAME} BUILD_DIR=$(TARGET_DIR_NAME)/${$@_CONFIG_NAME}
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/romfs.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/romfs.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
endef

# define macro to clean appset: configuration name (build directory)
define clean_appset
	$(eval $@_CONFIG_NAME = $(1))
	$(ECHO) -n "Cleaning ${$@_CONFIG_NAME} files: "
	$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile clean BUILD_DIR=$(TARGET_DIR_NAME)/${$@_CONFIG_NAME}
endef

# define macro to clean kernel
define clean_kernel
	$(ECHO) -n "Cleaning kernel files: "
	$(MAKE) -C kernel -s -j 1 -fMakefile clean
endef

# define macro to clean romfs: configuration name (build directory)
define clean_romfs
	$(eval $@_CONFIG_NAME = $(1))
	$(ECHO) "Cleaning ${$@_CONFIG_NAME}..."
	$(MAKE) -C $(ROMFS_BASE) -s -j 1 -fMakefile clean BUILD_DIR=$(TARGET_DIR_NAME)/${$@_CONFIG_NAME}
endef

# define macro install
define install_hex
	$(eval $@_HEX_FILE = $(1))
	$(FLASH_CPU) ../$(TARGET_DIR_NAME)/${$@_HEX_FILE}
endef

# define macro used to configure kernel
define config_kernel
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure appset
define config_appset
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure romfs
define config_romfs
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
endef

####################################################################################################
# targets of the workspace
####################################################################################################
# help is the default target (unless the project Makefile defines a target before include)
ifeq ($(.DEFAULT_GOAL),)
.DEFAULT_GOAL := help
endif

.PHONY : help
help ::
	@$(ECHO) "This is help for this $(THIS_MAKEFILE)"
	@$(ECHO) "Workspace targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   clean               clean project (kernel and $(TARGET_DIR_NAME) directory)"
	@$(ECHO) "   build-sim           build POSIX simulator (Linux): $(TARGET_DIR_NAME)/dnx-sim"
	@$(ECHO) "   build-sim-drvset    build drvset partition of simulator: $(TARGET_DIR_NAME)/sim-drvset.bin (SIM_DRVSET_DIR=...,"
	@$(ECHO) "                       run-sim SIM_OPTS=\"--drvset $(TARGET_DIR_NAME)/sim-drvset.bin\")"
	@$(ECHO) "   run-sim             build and start POSIX simulator (options: SIM_OPTS=..., PANEL=1"
	@$(ECHO) "                       board panel in web browser, needs --board, PANEL_OPTS=...)"
	@$(ECHO) "   run-sim-example     build and start POSIX simulator with example board and panel"
	@$(ECHO) "                       (http://localhost:$(SIM_PANEL_PORT), /host = sim-env, network tap0,"
	@$(ECHO) "                       tools/create_tap.sh, options: SIM_OPTS=...)"
	@$(ECHO) "   test-sim            build and test system in POSIX simulator (options: SIMTEST_OPTS=...)"
	@$(ECHO) "                       other appsets in simulator: SIM_APPSETS=\"<appset>[:<config>] ...\""
	@$(ECHO) "                       (test-sim adds sim-env/testappset)"
	@$(ECHO) "   test-drvset         build and check drvset partitions with test kernel (no hardware needed)"
	@$(ECHO) "   config-sim-kernel   simulator kernel configuration (also config-sim-appset, config-sim-romfs)"
	@$(ECHO) ""

.PHONY : _build_prepare
_build_prepare :
	@$(MKDIR) $(TARGET_DIR_NAME)

.PHONY : clean
clean ::
	@$(call clean_kernel)
	@$(REMOVE) -r $(TARGET_DIR_NAME)

.PHONY : build-sim
build-sim : _build_prepare build-sim-romfs build-sim-kernel build-sim-appset
	@$(MAKE) -C kernel -s -j 1 -fMakefile link-sim CONFIG_DIR=../config/$(SIM_KERNEL_CONFIG) \
	         APPSET_OBJ="../$(TARGET_DIR_NAME)/sim-appset.o $(addprefix ../,$(SIM_APPSET_OBJS))" \
	         OUTPUT=../$(TARGET_DIR_NAME)/dnx-sim
	@$(ECHO) "Target sim built successfully: $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro"

.PHONY : build-sim-kernel
build-sim-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_sim_kernel, $(SIM_KERNEL_CONFIG))

.PHONY : build-sim-appset
build-sim-appset : _build_prepare $(foreach a,$(SIM_APPSETS),build-$(call sim_appset_name,$(a)))
	@$(call build_sim_appset, $(SIM_APPSET_DIR), config/$(SIM_APPSET_CONFIG), sim-appset, _program_table_desc)

$(foreach a,$(SIM_APPSETS),$(eval $(call sim_appset_rule,$(a))))

.PHONY : build-sim-drvset
build-sim-drvset : _build_prepare
	@$(MAKE) -C tools/drvset -s -fMakefile all DRVSET_DIR=$(SIM_DRVSET_DIR) CONFIG_DIR=config/$(SIM_KERNEL_CONFIG) \
	         BUILD_DIR=$(TARGET_DIR_NAME)/sim-drvset
	@$(COPY) $(TARGET_DIR_NAME)/sim-drvset/output/drvset.bin $(TARGET_DIR_NAME)/sim-drvset.bin

.PHONY : build-sim-romfs
build-sim-romfs : _build_prepare
	@$(call build_romfs, $(SIM_ROMFS_DIR), $(SIM_ROMFS_CONFIG))

.PHONY : run-sim-example
run-sim-example : build-sim
	@$(PYTHON3) tools/simpanel.py --exit-with-parent --socket $(SIM_PANEL_SOCKET) --port $(SIM_PANEL_PORT)\
	 $(PANEL_OPTS) >$(TARGET_DIR_NAME)/simpanel.log 2>&1 & pid=$$!;\
	 sleep 0.5; if ! kill -0 $$pid 2>/dev/null; then $(CAT) $(TARGET_DIR_NAME)/simpanel.log; exit 1; fi;\
	 $(ECHO) "Board panel: http://localhost:$(SIM_PANEL_PORT) (log: $(TARGET_DIR_NAME)/simpanel.log)";\
	 $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro --panel $(SIM_PANEL_SOCKET) \
	 $(SIM_EXAMPLE_OPTS) $(SIM_OPTS);\
	 st=$$?; kill $$pid 2>/dev/null; exit $$st

.PHONY : run-sim
run-sim : build-sim
ifeq ($(PANEL),1)
	@$(PYTHON3) tools/simpanel.py --exit-with-parent --socket $(SIM_PANEL_SOCKET) --port $(SIM_PANEL_PORT)\
	 $(PANEL_OPTS) >$(TARGET_DIR_NAME)/simpanel.log 2>&1 & pid=$$!;\
	 sleep 0.5; if ! kill -0 $$pid 2>/dev/null; then $(CAT) $(TARGET_DIR_NAME)/simpanel.log; exit 1; fi;\
	 $(ECHO) "Board panel: http://localhost:$(SIM_PANEL_PORT) (log: $(TARGET_DIR_NAME)/simpanel.log)";\
	 $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro --panel $(SIM_PANEL_SOCKET) $(SIM_OPTS);\
	 st=$$?; kill $$pid 2>/dev/null; exit $$st
else
	@$(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro $(SIM_OPTS)
endif

.PHONY : test-sim
test-sim : build-sim build-sim-drvset
	@$(MAKE) -C tools/drvset -s -fMakefile check DRVSET_DIR=$(SIM_DRVSET_DIR) CONFIG_DIR=config/$(SIM_KERNEL_CONFIG) \
	         BUILD_DIR=$(TARGET_DIR_NAME)/sim-drvset KERNEL_ELF=$(TARGET_DIR_NAME)/dnx-sim
	@$(PYTHON3) tools/simtest.py --sim $(TARGET_DIR_NAME)/dnx-sim --rom $(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin\
	 --drvset $(TARGET_DIR_NAME)/sim-drvset.bin --log $(TARGET_DIR_NAME)/simtest.log $(SIMTEST_OPTS)

.PHONY : test-drvset
test-drvset :
	@$(SHELL) tools/drvset/test/test.sh

.PHONY : config-sim-kernel
config-sim-kernel :
	@$(call config_kernel, $(SIM_KERNEL_CONFIG))

.PHONY : config-sim-appset
config-sim-appset :
	@$(call config_appset, $(SIM_APPSET_CONFIG))

.PHONY : config-sim-romfs
config-sim-romfs :
	@$(call config_romfs, $(SIM_ROMFS_CONFIG))
