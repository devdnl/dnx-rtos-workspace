# Makefile for GNU make
####################################################################################################
#
# AUTHOR: DANIEL ZORYCHTA
#
# Version: 20250626
#--------------------------------------------------------------------------------------------------
#
#    Copyright (C) 2025  Daniel Zorychta (daniel.zorychta@gmail.com)
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the  Free Software  Foundation;  either version 2 of the License, or
#    any later version.
#
#    This  program  is  distributed  in the hope that  it will be useful,
#    but  WITHOUT  ANY  WARRANTY;  without  even  the implied warranty of
#    MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the
#    GNU General Public License for more details.
#
#    You  should  have received a copy  of the GNU General Public License
#    along  with  this  program;  if not,  write  to  the  Free  Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
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
MKDIR          = /bin/mkdir -p
CAT            = /bin/cat
SREC_CAT       = /bin/srec_cat
GREP           = /bin/grep
SIZEOF         = /usr/bin/stat -c %s
WC             = /usr/bin/wc
PYTHON3        = /usr/bin/python3
OBJCOPY        = $(TOOLCHAIN)objcopy
OBJDUMP        = $(TOOLCHAIN)objdump
SIZE           = $(TOOLCHAIN)size
CONFIGTOOL     = ./tools/configtool.sh
FLASH_CPU      = ./tools/flash.sh
MKFPKG         = $(PYTHON3) ./system/tools/mkfpkg.py
MKLINKERSCRIPT = ./tools/create_linker_script.sh
MD5SUM         = /usr/bin/md5sum

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY =

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# define macro used to build system according to application name
define make_system
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS image..."
	$(MKLINKERSCRIPT) ../../config/${$@_APP_NAME}/custom_linker_script.h ../config/${$@_APP_NAME}/custom_linker_script.ld
	$(REMOVE) system/build/ioctl
	$(MAKE) -C system -s -j 1 -fMakefile clean all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) system/build/output/dnx.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
	$(REMOVE) $(TARGET_DIR_NAME)/ioctl
	$(COPY) system/build/ioctl $(TARGET_DIR_NAME)
	$(REMOVE) system/build/ioctl
endef

# define macro used to build appset according to name
define make_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(eval $@_BOARD_NAME = $(3))
	$(eval $@_PROJECT_OPTS = $(4))
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile all BOARD_NAME=${$@_BOARD_NAME} CONFIG_DIR=../config/${$@_CONFIG_NAME} PROJECT_OPTS=${$@_PROJECT_OPTS}
	$(COPY) ${$@_APP_NAME}/build/output/appset.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) ${$@_APP_NAME}/build/output/appset.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) ${$@_APP_NAME}/build/output/appset.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
endef

# define macro used to create ROMFS
define make_romfs
	$(eval $@_ROMFS_NAME = $(1))
	$(MAKE) -C ${$@_ROMFS_NAME} -s -j 1 -fMakefile all
	$(COPY) ${$@_ROMFS_NAME}/build/romfs.bin $(TARGET_DIR_NAME)/${$@_ROMFS_NAME}.bin
	$(COPY) ${$@_ROMFS_NAME}/build/romfs.hex $(TARGET_DIR_NAME)/${$@_ROMFS_NAME}.hex
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
	$(eval $@_APP_NAME = $(1))
	$(CONFIGTOOL) ../../config/${$@_APP_NAME}/config.h
endef

# define macro used to configure appset
define config_appset
	$(eval $@_APP_NAME = $(1))
	$(CONFIGTOOL) ../../config/${$@_APP_NAME}/config.h
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
	@$(ECHO) "   all                 build application with bootloader in single hex file"
	@$(ECHO) "   btl                 build bootloader (system + appset)"
	@$(ECHO) "   app                 build application (system + appset)"
	@$(ECHO) "   app-system          build application's system"
	@$(ECHO) "   app-appset          build application's appset"
	@$(ECHO) "   btl-system          build bootloader's system"
	@$(ECHO) "   btl-appset          build bootloader's appset"
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


.PHONY : all
all : _build_prepare btl app
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex -Intel;\


.PHONY : clean
clean :
	@$(call clean_appset, system)
	@$(call clean_appset, app-appset)
	@$(call clean_appset, btl-appset)
	@$(call clean_romfs, app-romfs)
	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : app
app : _build_prepare app-romfs app-system app-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-system.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./app-dist $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-system.bin $(TARGET_DIR_NAME)/app-dist/app-system.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-romfs.bin $(TARGET_DIR_NAME)/app-dist/app-romfs.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT)-app.fpkg
	@$(ECHO) "Target app built successfully."


.PHONY : btl
btl : _build_prepare btl-system btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-system.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin
	@$(MKDIR) $(TARGET_DIR_NAME)/btl-dist
	@$(COPY) $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin $(TARGET_DIR_NAME)/btl-dist/btl.bin
	@$(MD5SUM) -b $(TARGET_DIR_NAME)/btl-dist/btl.bin | head -c 32 > $(TARGET_DIR_NAME)/btl-dist/btl.md5
	@$(MKFPKG) $(TARGET_DIR_NAME)/btl-dist $(TARGET_DIR_NAME)/$(PROJECT)-btl.fpkg


.PHONY : app-system
app-system : _build_prepare
	@$(call clean_system)
	@$(call make_system, app-system)

.PHONY : app-appset
app-appset : _build_prepare
	@$(call make_appset, app-appset, app-appset, boardname, proj_opts)

.PHONY : app-romfs
app-romfs : _build_prepare
	@$(call make_romfs, app-romfs)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)


.PHONY : btl-system
btl-system : _build_prepare
	@$(call make_system, btl-system)

.PHONY : btl-appset
btl-appset : _build_prepare
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
	@$(call config_appset, app-romfs)


.PHONY : config-btl-system
config-btl-system :
	@$(call config_system, btl-system)


.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

