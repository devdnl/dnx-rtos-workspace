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

#---------------------------------------------------------------------------------------------------
# FILE AND DIRECTORY NAMES
#---------------------------------------------------------------------------------------------------
PROJECT = project
TOOLCHAIN = arm-none-eabi-
TARGET_DIR_NAME = build

#---------------------------------------------------------------------------------------------------
# BASIC PROGRAMS DEFINITIONS
#---------------------------------------------------------------------------------------------------
SHELL          = /bin/bash
ECHO           = /bin/echo -e
REMOVE         = /bin/rm -rf
COPY           = /bin/cp -r
RENAME         = /bin/mv
MOVE           = /bin/mv
MKDIR          = /bin/mkdir -p
CAT            = /bin/cat
SREC_CAT       = /bin/srec_cat
GREP           = /bin/grep
SIZEOF         = /usr/bin/stat -c %s
WC             = /usr/bin/wc
PYTHON3        = /usr/bin/python3
MD5SUM         = /usr/bin/md5sum
OBJCOPY        = $(TOOLCHAIN)objcopy
OBJDUMP        = $(TOOLCHAIN)objdump
SIZE           = $(TOOLCHAIN)size
CONFIGTOOL     = $(SHELL) ./tools/configtool.sh
FLASH_CPU      = $(SHELL) ./tools/flash.sh
MKFPKG         = $(PYTHON3) ./system/tools/mkfpkg.py
MKLINKERSCRIPT = $(SHELL) ./tools/create_linker_script.sh
SYNCDIR        = $(PYTHON3) ./system/tools/syncdir.py

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# define macro used to build system according to application name
define make_system
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS image..."
	$(MKLINKERSCRIPT) ../../config/${$@_APP_NAME}/custom_linker_script.h ../config/${$@_APP_NAME}/custom_linker_script.ld
	$(REMOVE) system/build/ioctl
	$(MAKE) -C system -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) system/build/output/dnx.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
	$(MKDIR) $(TARGET_DIR_NAME)/ioctl
	$(SYNCDIR) system/build/ioctl/ $(TARGET_DIR_NAME)/ioctl/
	$(REMOVE) system/build/ioctl
endef

# define macro used to build appset according to name
define make_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(eval $@_BOARD_NAME = $(3))
	$(eval $@_PROJECT_OPTS = $(4))
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile all BOARD_NAME=${$@_BOARD_NAME} CONFIG_DIR=../config/${$@_CONFIG_NAME} PROJECT_OPTS=${$@_PROJECT_OPTS}
	$(COPY) ${$@_APP_NAME}/build/output/appset.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
	$(COPY) ${$@_APP_NAME}/build/output/appset.elf $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.elf
	$(COPY) ${$@_APP_NAME}/build/output/appset.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
endef

# define macro used to create ROMFS
define make_romfs
	$(eval $@_ROMFS_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(MAKE) -C ${$@_ROMFS_NAME} -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_CONFIG_NAME}
	$(COPY) ${$@_ROMFS_NAME}/build/romfs.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
	$(COPY) ${$@_ROMFS_NAME}/build/romfs.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
endef

# define macro to clean appset
define clean_appset
	$(eval $@_APP_NAME = $(1))
	$(ECHO) -n "Cleaning ${$@_APP_NAME} files: "
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile clean
endef

# define macro to clean system
define clean_system
	$(ECHO) -n "Cleaning system files: "
	$(MAKE) -C system -s -j 1 -fMakefile clean
endef

# define macro to clean romfs
define clean_romfs
	$(eval $@_ROMFS_NAME = $(1))
	$(ECHO) "Cleaning ${$@_ROMFS_NAME}..."
	$(MAKE) -C ${$@_ROMFS_NAME} -s -j 1 -fMakefile clean
endef

# define macro install
define install_hex
	$(eval $@_HEX_FILE = $(1))
	$(FLASH_CPU) ../$(TARGET_DIR_NAME)/${$@_HEX_FILE}
endef

# define macro used to configure system
define config_system
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ../../config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure appset
define config_appset
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ../../config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure romfs
define config_romfs
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ../../config/${$@_CONFIG_NAME}/config.h
endef

####################################################################################################
# targets
####################################################################################################
.PHONY : help
help :
	@$(ECHO) "This is help for this $(THIS_MAKEFILE)"
	@$(ECHO) "Possible targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   clean               clean project"
	@$(ECHO) "   clean-app-appset    clean application's appset"
	@$(ECHO) "   clean-btl-appset    clean bootloader's appset"
	@$(ECHO) "   build-all           build application with bootloader in single hex file"
	@$(ECHO) "   build-btl           build bootloader (system + appset)"
	@$(ECHO) "   build-app           build application (system + appset)"
	@$(ECHO) "   build-app-system    build application's system"
	@$(ECHO) "   build-app-appset    build application's appset"
	@$(ECHO) "   build-btl-system    build bootloader's system"
	@$(ECHO) "   build-btl-appset    build bootloader's appset"
	@$(ECHO) "   install             install (flash) all build files"
	@$(ECHO) "   install-app         install (flash) application"
	@$(ECHO) "   install-btl         install (flash) bootloader"
	@$(ECHO) "   install-app-system  install (flash) application system"
	@$(ECHO) "   install-app-appset  install (flash) application appset"
	@$(ECHO) "   config              start empty configurator"
	@$(ECHO) "   config-app-system   application system configuration"
	@$(ECHO) "   config-app-appset   application appset configuration"
	@$(ECHO) "   config-btl-system   bootloader system configuration"
	@$(ECHO) "   config-btl-appset   bootloader appset configuration"
	@$(ECHO) ""


.PHONY : _build_prepare
_build_prepare :
	@$(MKDIR) $(TARGET_DIR_NAME)


.PHONY : build-all
build-all : _build_prepare build-btl build-app
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex -Intel;\


.PHONY : clean
clean :
	@$(call clean_appset, system)
	@$(call clean_appset, app-appset)
	@$(call clean_appset, btl-appset)
	@$(call clean_romfs, app-romfs)
	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : build-app
build-app : _build_prepare build-app-romfs build-app-system build-app-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-system.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./app-dist $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-system.bin $(TARGET_DIR_NAME)/app-dist/app-system.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-romfs.bin $(TARGET_DIR_NAME)/app-dist/app-romfs.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT)-app.fpkg
	@$(ECHO) "Target app built successfully."


.PHONY : build-btl
build-btl : _build_prepare build-btl-system build-btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-system.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin
	@$(MKDIR) $(TARGET_DIR_NAME)/btl-dist
	@$(COPY) $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin $(TARGET_DIR_NAME)/btl-dist/btl.bin
	@$(MD5SUM) -b $(TARGET_DIR_NAME)/btl-dist/btl.bin | head -c 32 > $(TARGET_DIR_NAME)/btl-dist/btl.md5
	@$(MKFPKG) $(TARGET_DIR_NAME)/btl-dist $(TARGET_DIR_NAME)/$(PROJECT)-btl.fpkg


.PHONY : build-app-system
build-app-system : _build_prepare
	@$(call clean_system)
	@$(call make_system, app-system)

.PHONY : build-app-appset
build-app-appset : _build_prepare
	@$(call make_appset, app-appset, app-appset, boardname, proj_opts)

.PHONY : build-app-romfs
build-app-romfs : _build_prepare
	@$(call make_romfs, app-romfs, app-romfs)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)


.PHONY : build-btl-system
build-btl-system : _build_prepare
	@$(call clean_system)
	@$(call make_system, btl-system)

.PHONY : build-btl-appset
build-btl-appset : _build_prepare
	@$(call make_appset, btl-appset, btl-appset, boardname, proj_opts)

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


.PHONY : install-app-system
install-app-system :
	@$(call install_hex, app-system.hex)


.PHONY : install-app-appset
install-app-appset :
	@$(call install_hex, app-appset.hex)


.PHONY : install-app-romfs
install-app-romfs :
	@$(call install_hex, app-romfs.hex)


.PHONY : config
config :
	@$(CONFIGTOOL)


.PHONY : config-app-system
config-app-system :
	@$(call config_system, app-system)


.PHONY : config-app-appset
config-app-appset :
	@$(call config_appset, app-appset)


.PHONY : config-app-romfs
config-app-romfs :
	@$(call config_romfs, app-romfs)


.PHONY : config-btl-system
config-btl-system :
	@$(call config_system, btl-system)


.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

