# Example targets of the application for GNU make (tools/target_app.mk)
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
# Targets of the application (example): kernel config/app-kernel, appset $(APP_APPSET_DIR) with
# configuration config/app-appset, romfs $(APP_ROMFS_DIR) with configuration config/app-romfs,
# update package from $(APP_DIST_DIR) and drvset partition $(DRVSET_DIR). Included by the project
# Makefile after tools/workspace.mk. The project which does not need these targets removes the
# include and defines own targets with the macros of tools/workspace.mk (this file is an example
# how to do it). This file belongs to the workspace and is updated with it, do not edit it.
####################################################################################################
TARGET_APP_MK := 1

#---------------------------------------------------------------------------------------------------
# SETTINGS (defaults, set in the project Makefile before include)
#---------------------------------------------------------------------------------------------------
# directories of the application appset, romfs image and files of the update package
APP_APPSET_DIR ?= app-appset
APP_ROMFS_DIR  ?= app-romfs
APP_DIST_DIR   ?= app-dist

# drvset partition (drivers built separately from the kernel, tools/drvset): directory with
# Makefile.in of drivers, built with the application kernel configuration (config/app-kernel)
DRVSET_DIR     ?= tools/drvset/example

####################################################################################################
# targets
####################################################################################################
help ::
	@$(ECHO) "Application targets:"
	@$(ECHO) "   build-app           build application (kernel + appset + romfs)"
	@$(ECHO) "   build-app-kernel    build application's kernel"
	@$(ECHO) "   build-app-appset    build application's appset"
	@$(ECHO) "   build-app-romfs     build application's romfs"
	@$(ECHO) "   build-app-drvset    build drvset partition of application kernel (DRVSET_DIR=...)"
	@$(ECHO) "   clean-app-appset    clean application's appset"
	@$(ECHO) "   install-app         install (flash) application"
	@$(ECHO) "   install-app-kernel  install (flash) application kernel"
	@$(ECHO) "   install-app-appset  install (flash) application appset"
	@$(ECHO) "   install-app-romfs   install (flash) application romfs"
	@$(ECHO) "   config-app-kernel   application kernel configuration"
	@$(ECHO) "   config-app-appset   application appset configuration"
	@$(ECHO) "   config-app-romfs    application romfs configuration"
	@$(ECHO) "   appset-rungens      run generators of application's appset"
	@$(ECHO) ""

.PHONY : build-app
build-app : _build_prepare build-app-romfs build-app-kernel build-app-appset
	@$(call licenses_report, $(PROJECT)_app, $(PROJECT) application, \
	        $(TARGET_DIR_NAME)/app-kernel.files $(TARGET_DIR_NAME)/app-appset.files, $(APP_APPSET_DIR))
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-kernel.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./$(APP_DIST_DIR) $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-kernel.bin $(TARGET_DIR_NAME)/app-dist/app-kernel.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-romfs.bin $(TARGET_DIR_NAME)/app-dist/app-romfs.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT)-app.fpkg
	@$(ECHO) "Target app built successfully."

.PHONY : build-app-kernel
build-app-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, app-kernel)

.PHONY : build-app-appset
build-app-appset : _build_prepare
	@$(call build_appset, $(APP_APPSET_DIR), app-appset, $(APPSET_BOARD), $(APPSET_OPTS))

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
	@$(call build_romfs, $(APP_ROMFS_DIR), app-romfs)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)

.PHONY : install-app
install-app :
	@$(call install_hex, $(PROJECT)_app.hex)

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

.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C $(APPSET_BASE) -s -j 1 -fMakefile rungens APPSET_DIR=$(APP_APPSET_DIR) CONFIG_DIR=config/app-appset
