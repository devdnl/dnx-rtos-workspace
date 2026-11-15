# Makefile for GNU make
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
# Makefile of the project: settings, the core of the workspace (tools/workspace.mk: macros,
# simulator, tests, help and clean) and targets of the project partitions. The core is updated
# with the workspace, this file belongs to the project: partitions (names of kernels, appsets and
# romfs images, their configurations in config/<name>) and own targets are defined here. The
# targets below are the example layout of the workspace: application (app-kernel, app-appset,
# app-romfs) and bootloader (btl-kernel, btl-appset).
####################################################################################################

#---------------------------------------------------------------------------------------------------
# SETTINGS (other settings and their defaults: tools/workspace.mk)
#---------------------------------------------------------------------------------------------------
PROJECT = project
TOOLCHAIN = arm-none-eabi-
TARGET_DIR_NAME = build

# POSIX simulator: appset and romfs of the simulator system
SIM_APPSET_DIR = app-appset
SIM_ROMFS_DIR = app-romfs

# board name and options of the project passed to appsets (BOARD_NAME, PROJECT_OPTS)
APPSET_BOARD = boardname
APPSET_OPTS = proj_opts

# drvset partition (drivers built separately from the kernel, tools/drvset): directory with
# Makefile.in of drivers, built with the application kernel configuration (config/app-kernel)
DRVSET_DIR ?= tools/drvset/example

#---------------------------------------------------------------------------------------------------
# CORE OF THE WORKSPACE
#---------------------------------------------------------------------------------------------------
include tools/workspace.mk

####################################################################################################
# targets of the project
####################################################################################################
help ::
	@$(ECHO) "Project targets:"
	@$(ECHO) "   build-all           build application with bootloader in single hex file"
	@$(ECHO) "   build-app           build application (kernel + appset + romfs)"
	@$(ECHO) "   build-app-kernel    build application's kernel"
	@$(ECHO) "   build-app-appset    build application's appset"
	@$(ECHO) "   build-app-romfs     build application's romfs"
	@$(ECHO) "   build-app-drvset    build drvset partition of application kernel (DRVSET_DIR=...)"
	@$(ECHO) "   build-btl           build bootloader (kernel + appset)"
	@$(ECHO) "   build-btl-kernel    build bootloader's kernel"
	@$(ECHO) "   build-btl-appset    build bootloader's appset"
	@$(ECHO) "   clean-app-appset    clean application's appset"
	@$(ECHO) "   clean-btl-appset    clean bootloader's appset"
	@$(ECHO) "   install             install (flash) bootloader and application"
	@$(ECHO) "   install-btl         install (flash) bootloader"
	@$(ECHO) "   install-app         install (flash) application"
	@$(ECHO) "   install-app-kernel  install (flash) application kernel"
	@$(ECHO) "   install-app-appset  install (flash) application appset"
	@$(ECHO) "   install-app-romfs   install (flash) application romfs"
	@$(ECHO) "   config-app-kernel   application kernel configuration"
	@$(ECHO) "   config-app-appset   application appset configuration"
	@$(ECHO) "   config-app-romfs    application romfs configuration"
	@$(ECHO) "   config-btl-kernel   bootloader kernel configuration"
	@$(ECHO) "   config-btl-appset   bootloader appset configuration"
	@$(ECHO) "   appset-rungens      run generators of application's appset"
	@$(ECHO) ""

.PHONY : build-all
build-all : _build_prepare build-btl build-app
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex -Intel;\

.PHONY : build-app
build-app : _build_prepare build-app-romfs build-app-kernel build-app-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-kernel.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./app-dist $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-kernel.bin $(TARGET_DIR_NAME)/app-dist/app-kernel.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-romfs.bin $(TARGET_DIR_NAME)/app-dist/app-romfs.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT)-app.fpkg
	@$(ECHO) "Target app built successfully."

.PHONY : build-btl
build-btl : _build_prepare build-btl-kernel build-btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-kernel.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin
	@$(MKDIR) $(TARGET_DIR_NAME)/btl-dist
	@$(COPY) $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin $(TARGET_DIR_NAME)/btl-dist/btl.bin
	@$(MD5SUM) -b $(TARGET_DIR_NAME)/btl-dist/btl.bin | head -c 32 > $(TARGET_DIR_NAME)/btl-dist/btl.md5
	@$(MKFPKG) $(TARGET_DIR_NAME)/btl-dist $(TARGET_DIR_NAME)/$(PROJECT)-btl.fpkg

.PHONY : build-app-kernel
build-app-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, app-kernel)

.PHONY : build-app-appset
build-app-appset : _build_prepare
	@$(call build_appset, app-appset, app-appset, $(APPSET_BOARD), $(APPSET_OPTS))

.PHONY : build-app-drvset
build-app-drvset : _build_prepare
	@$(MAKE) -C tools/drvset -s -fMakefile all DRVSET_DIR=$(DRVSET_DIR) CONFIG_DIR=config/app-kernel BUILD_DIR=$(TARGET_DIR_NAME)/app-drvset
	@$(COPY) $(TARGET_DIR_NAME)/app-drvset/output/drvset.hex $(TARGET_DIR_NAME)/app-drvset.hex
	@$(COPY) $(TARGET_DIR_NAME)/app-drvset/output/drvset.elf $(TARGET_DIR_NAME)/app-drvset.elf
	@$(COPY) $(TARGET_DIR_NAME)/app-drvset/output/drvset.bin $(TARGET_DIR_NAME)/app-drvset.bin
	@if [ -f $(TARGET_DIR_NAME)/app-kernel.elf ]; then \
	    $(MAKE) -C tools/drvset -s -fMakefile check DRVSET_DIR=$(DRVSET_DIR) CONFIG_DIR=config/app-kernel \
	            BUILD_DIR=$(TARGET_DIR_NAME)/app-drvset KERNEL_ELF=$(TARGET_DIR_NAME)/app-kernel.elf; fi

.PHONY : build-app-romfs
build-app-romfs : _build_prepare
	@$(call build_romfs, app-romfs, app-romfs)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)

.PHONY : build-btl-kernel
build-btl-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, btl-kernel)

.PHONY : build-btl-appset
build-btl-appset : _build_prepare
	@$(call build_appset, btl-appset, btl-appset, $(APPSET_BOARD), $(APPSET_OPTS))

.PHONY : clean-btl-appset
clean-btl-appset : _build_prepare
	@$(call clean_appset, btl-appset)

.PHONY : install
install :
	@$(call install_hex, $(PROJECT)_btl_app.hex)

.PHONY : install-app
install-app :
	@$(call install_hex, $(PROJECT)_app.hex)

.PHONY : install-btl
install-btl :
	@$(call install_hex, $(PROJECT)_btl.hex)

.PHONY : install-app-kernel
install-app-kernel :
	@$(call install_hex, app-kernel.hex)

.PHONY : install-app-appset
install-app-appset :
	@$(call install_hex, app-appset.hex)

.PHONY : install-app-romfs
install-app-romfs :
	@$(call install_hex, app-romfs.hex)

.PHONY : config-app-kernel
config-app-kernel :
	@$(call config_kernel, app-kernel)

.PHONY : config-app-appset
config-app-appset :
	@$(call config_appset, app-appset)

.PHONY : config-app-romfs
config-app-romfs :
	@$(call config_romfs, app-romfs)

.PHONY : config-btl-kernel
config-btl-kernel :
	@$(call config_kernel, btl-kernel)

.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)

.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile rungens APPSET_DIR=app-appset CONFIG_DIR=config/app-appset
