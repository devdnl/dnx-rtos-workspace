# Makefile for GNU make
####################################################################################################
#
# AUTHOR: DANIEL ZORYCHTA
#
# Version: 20220905
#--------------------------------------------------------------------------------------------------
#
#    Copyright (C) 2022  Daniel Zorychta (daniel.zorychta@gmail.com)
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
SHELL      = /bin/bash
ECHO       = /bin/echo -e
REMOVE     = /bin/rm -rf
COPY       = /bin/cp -r
RENAME     = /bin/mv
MKDIR      = /bin/mkdir -p
CAT        = /bin/cat
SREC_CAT   = /bin/srec_cat
GREP       = /bin/grep
SIZEOF     = /usr/bin/stat -c %s
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
CONFIGTOOL = ./tools/configtool.sh
FLASH_CPU  = ./tools/flash.sh
MKFPKG     = $(PYTHON3) ./system/tools/mkfpkg.py

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
	$(eval $@_BOARD_NAME = $(2))
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile all BOARD_NAME=${$@_BOARD_NAME}
	$(COPY) ${$@_APP_NAME}/build/output/appset.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) ${$@_APP_NAME}/build/output/appset.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) ${$@_APP_NAME}/build/output/appset.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
endef

# define macro used to create ROMFS
define make_romfs
	$(eval $@_ROMFS_NAME = $(1))
	$(eval $@_ROMFS_OFFSET = $(2))
	$(ECHO) "Creating ROMFS (${$@_ROMFS_NAME}) image..."
	$(MKFPKG) ${$@_ROMFS_NAME} $(TARGET_DIR_NAME)/${$@_ROMFS_NAME}.bin
	$(SREC_CAT) $(TARGET_DIR_NAME)/${$@_ROMFS_NAME}.bin -binary -offset ${$@_ROMFS_OFFSET} -output $(TARGET_DIR_NAME)/${$@_ROMFS_NAME}.hex -Intel
endef

# define macro to clean appset
define clean_appset
	$(eval $@_APP_NAME = $(1))
	$(ECHO) -n "Cleaning ${$@_APP_NAME} files: "
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile clean
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
	$(CONFIGTOOL) ../../${$@_APP_NAME}/config/config.h
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
	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : app
app : _build_prepare app-system app-appset app-romfs
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-system.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./app-dist $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-system.bin $(TARGET_DIR_NAME)/app-dist/app-system.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT).fpkg


.PHONY : btl
btl : _build_prepare btl-system btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-system.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin


.PHONY : app-system
app-system : _build_prepare
	@$(call make_system, app-system)

.PHONY : app-appset
app-appset : _build_prepare
	@$(call make_appset, app-appset, default)

.PHONY : app-romfs
app-romfs : _build_prepare
	@$(call make_romfs, app-romfs, 0x08070000)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)


.PHONY : btl-system
btl-system : _build_prepare
	@$(call make_system, btl-system)

.PHONY : btl-appset
btl-appset : _build_prepare
	@$(call make_appset, btl-appset, default)

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


.PHONY : config-btl-system
config-btl-system :
	@$(call config_system, btl-system)


.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

