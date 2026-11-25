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
#     include tools/workspace.mk
#
# and then includes example targets (each file can be omitted, the core works without them):
#
#     include tools/target_app.mk       application (app-kernel, app-appset, app-romfs, drvset)
#     include tools/target_btl.mk       bootloader (btl-kernel, btl-appset), with application:
#                                       build-all, install
#     include tools/target_sim.mk       POSIX simulator (sim-kernel, sim-appset, sim-romfs)
#     include tools/target_test_sim.mk  tests in POSIX simulator (needs target_sim.mk)
#
# or defines own targets of its partitions (kernel, appsets, romfs) by the macros of this file
# (build_kernel, build_appset, build_sim_kernel, build_sim_appset, build_romfs, install_hex,
# config_*), the example targets show how to do it. Help of the project is added by "help ::"
# rules, additional cleaning by "clean ::" rules (run after cleaning of this file).
####################################################################################################

#---------------------------------------------------------------------------------------------------
# SETTINGS (defaults, set in the project Makefile before include)
#---------------------------------------------------------------------------------------------------
PROJECT         ?= project
TOOLCHAIN       ?= arm-none-eabi-
TARGET_DIR_NAME ?= build

# definitions of the project added to build/defs.h of each appset (board and simulator), e.g.
# APPSET_DEFS = STEROWNIK HW_REV=2 gives "#define STEROWNIK" and "#define HW_REV 2"; an appset can
# have own definitions by a target-specific variable: build-btl-appset : APPSET_DEFS += BOOTLOADER
APPSET_DEFS     ?=

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
LICENSES       ?= $(PYTHON3) ./kernel/tools/licenses.py

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
	$(COPY) kernel/build/output/dnx.files $(TARGET_DIR_NAME)/${$@_APP_NAME}.files
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
	$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile all APPSET_DIR=${$@_APP_NAME} CONFIG_DIR=config/${$@_CONFIG_NAME} BUILD_DIR=$(TARGET_DIR_NAME)/${$@_CONFIG_NAME} BOARD_NAME=${$@_BOARD_NAME} PROJECT_OPTS=${$@_PROJECT_OPTS} APPSET_DEFS="$(APPSET_DEFS)"
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.elf $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.elf
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
	$(COPY) $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}/output/appset.files $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.files
endef

# define macro used to build POSIX simulator kernel
define build_sim_kernel
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS simulator kernel..."
	$(REMOVE) kernel/build/ioctl
	$(MAKE) -C kernel -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) kernel/build/output/dnx.a $(TARGET_DIR_NAME)/${$@_APP_NAME}.a
	$(COPY) kernel/build/output/dnx.files $(TARGET_DIR_NAME)/${$@_APP_NAME}.files
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
	$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile all APPSET_DIR=${$@_APP_NAME} CONFIG_DIR=${$@_CONFIG_DIR} BUILD_DIR=$(TARGET_DIR_NAME)/${$@_BUILD_NAME} BOARD_NAME=posix PROJECT_OPTS=sim TABLE_SYMBOL=${$@_TABLE_SYMBOL} APPSET_DEFS="$(APPSET_DEFS)"
	$(COPY) $(TARGET_DIR_NAME)/${$@_BUILD_NAME}/output/appset.o $(TARGET_DIR_NAME)/${$@_BUILD_NAME}.o
	$(COPY) $(TARGET_DIR_NAME)/${$@_BUILD_NAME}/output/appset.files $(TARGET_DIR_NAME)/${$@_BUILD_NAME}.files
endef

# other appsets of the POSIX simulator (entry: <appset directory>[:<configuration directory>]):
# fields of entry, build name, table symbol
sim_appset_dir    = $(word 1,$(subst :, ,$(1)))
sim_appset_config = $(or $(word 2,$(subst :, ,$(1))),config/sim-appset)
sim_appset_name   = sim-$(notdir $(abspath $(call sim_appset_dir,$(1))))
sim_appset_symbol = _program_table_desc_$(subst .,_,$(subst -,_,$(notdir $(abspath $(call sim_appset_dir,$(1))))))

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

# define macro used to create licenses of build: output name ($(TARGET_DIR_NAME)/<name>.spdx and
# <name>-licenses.txt.gz), document name, lists of source files (*.files of kernel and appsets),
# directories of appsets outside the workspace (files without LICENSE.spdx are reported)
define licenses_report
	$(eval $@_NAME = $(strip $(1)))
	$(eval $@_TITLE = $(strip $(2)))
	$(eval $@_FILES = $(3))
	$(eval $@_ROOTS = $(4))
	$(ECHO) "Creating licenses of ${$@_TITLE}: $(TARGET_DIR_NAME)/${$@_NAME}.spdx, $(TARGET_DIR_NAME)/${$@_NAME}-licenses.txt.gz"
	$(LICENSES) report -o $(TARGET_DIR_NAME)/${$@_NAME} -n "${$@_TITLE}" -r $(CURDIR) $(addprefix -r ,${$@_ROOTS}) ${$@_FILES}
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
	@$(ECHO) "   test-drvset         build and check drvset partitions with test kernel (no hardware needed)"
	@$(ECHO) ""

.PHONY : _build_prepare
_build_prepare :
	@$(MKDIR) $(TARGET_DIR_NAME)

.PHONY : clean
clean ::
	@$(call clean_kernel)
	@$(REMOVE) -r $(TARGET_DIR_NAME)

.PHONY : test-drvset
test-drvset :
	@$(SHELL) tools/drvset/test/test.sh
