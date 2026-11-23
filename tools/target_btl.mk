# Example targets of the bootloader for GNU make (tools/target_btl.mk)
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
# Targets of the bootloader (example): kernel config/btl-kernel, appset $(BTL_APPSET_DIR) with
# configuration config/btl-appset. When targets of the application are included before this file
# (tools/target_app.mk), targets of both are defined (build-all, install). Included by the project
# Makefile after tools/workspace.mk. The project which does not need these targets removes the
# include and defines own targets with the macros of tools/workspace.mk (this file is an example
# how to do it). This file belongs to the workspace and is updated with it, do not edit it.
####################################################################################################
TARGET_BTL_MK := 1

#---------------------------------------------------------------------------------------------------
# SETTINGS (defaults, set in the project Makefile before include)
#---------------------------------------------------------------------------------------------------
# directory of the bootloader appset
BTL_APPSET_DIR ?= btl-appset

####################################################################################################
# targets
####################################################################################################
help ::
	@$(ECHO) "Bootloader targets:"
	@$(ECHO) "   build-btl           build bootloader (kernel + appset)"
	@$(ECHO) "   build-btl-kernel    build bootloader's kernel"
	@$(ECHO) "   build-btl-appset    build bootloader's appset"
	@$(ECHO) "   clean-btl-appset    clean bootloader's appset"
	@$(ECHO) "   install-btl         install (flash) bootloader"
	@$(ECHO) "   config-btl-kernel   bootloader kernel configuration"
	@$(ECHO) "   config-btl-appset   bootloader appset configuration"
ifdef TARGET_APP_MK
	@$(ECHO) "   build-all           build application with bootloader in single hex file"
	@$(ECHO) "   install             install (flash) bootloader and application"
endif
	@$(ECHO) ""

.PHONY : build-btl
build-btl : _build_prepare build-btl-kernel build-btl-appset
	@$(call licenses_report, $(PROJECT)_btl, $(PROJECT) bootloader, \
	        $(TARGET_DIR_NAME)/btl-kernel.files $(TARGET_DIR_NAME)/btl-appset.files, $(BTL_APPSET_DIR))
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-kernel.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin
	@$(MKDIR) $(TARGET_DIR_NAME)/btl-dist
	@$(COPY) $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin $(TARGET_DIR_NAME)/btl-dist/btl.bin
	@$(MD5SUM) -b $(TARGET_DIR_NAME)/btl-dist/btl.bin | head -c 32 > $(TARGET_DIR_NAME)/btl-dist/btl.md5
	@$(MKFPKG) $(TARGET_DIR_NAME)/btl-dist $(TARGET_DIR_NAME)/$(PROJECT)-btl.fpkg

.PHONY : build-btl-kernel
build-btl-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, btl-kernel)

.PHONY : build-btl-appset
build-btl-appset : _build_prepare
	@$(call build_appset, $(BTL_APPSET_DIR), btl-appset, $(APPSET_BOARD), $(APPSET_OPTS))

.PHONY : clean-btl-appset
clean-btl-appset : _build_prepare
	@$(call clean_appset, btl-appset)

.PHONY : install-btl
install-btl :
	@$(call install_hex, $(PROJECT)_btl.hex)

.PHONY : config-btl-kernel
config-btl-kernel :
	@$(call config_kernel, btl-kernel)

.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)

# application with bootloader (targets of the application: tools/target_app.mk)
ifdef TARGET_APP_MK
.PHONY : build-all
build-all : _build_prepare build-btl build-app
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex -Intel

.PHONY : install
install :
	@$(call install_hex, $(PROJECT)_btl_app.hex)
endif
