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
REMOVE     = /bin/rm -f
COPY       = /bin/cp -r
RENAME     = /bin/mv
MKDIR      = /bin/mkdir -p
CAT        = /bin/cat
SREC_CAT   = /bin/srec_cat
GREP       = /bin/grep
SIZEOF     = /usr/bin/stat -c %s
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
PYTHON2    = /usr/bin/python2
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
CONFIGTOOL = ./tools/configtool.sh
FLASH_CPU  = ./tools/flash.sh

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY =

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

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
	@$(ECHO) -n "Cleaning system files: "
	@$(MAKE) -C system -s -j 1 -fMakefile clean

	@$(ECHO) -n "Cleaning app-appset files: "
	@$(MAKE) -C app-appset -s -j 1 -fMakefile clean

	@$(ECHO) -n "Cleaning btl-appset files: "
	@if [ -d btl-appset ]; then $(MAKE) -C btl-appset -s -j 1 -fMakefile clean; else $(ECHO) "No bootloader project."; fi

	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : app
app : _build_prepare app-system app-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-system.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel


.PHONY : btl
btl : _build_prepare btl-system btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-system.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin


.PHONY : app-system
app-system : _build_prepare
	@$(MAKE) -C system -s -j 1 -fMakefile clean all CONFIG_DIR=../config/app/system
	@$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/app-system.hex
	@$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/app-system.elf
	@$(COPY) system/build/output/dnx.bin $(TARGET_DIR_NAME)/app-system.bin
	@$(COPY) system/build/ioctl $(TARGET_DIR_NAME)/ioctl


.PHONY : app-appset
app-appset : _build_prepare
	@$(MAKE) -C app-appset -s -j 1 -fMakefile all
	@$(COPY) app-appset/build/output/appset.hex $(TARGET_DIR_NAME)/app-appset.hex
	@$(COPY) app-appset/build/output/appset.elf $(TARGET_DIR_NAME)/app-appset.elf
	@$(COPY) app-appset/build/output/appset.bin $(TARGET_DIR_NAME)/app-appset.bin


.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(MAKE) -C app-appset -s -j 1 -fMakefile clean


.PHONY : btl-system
btl-system : _build_prepare
	@$(MAKE) -C system -s -j 1 -fMakefile clean all CONFIG_DIR=../config/btl/system
	@$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/btl-system.hex
	@$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/btl-system.elf
	@$(COPY) system/build/output/dnx.bin $(TARGET_DIR_NAME)/btl-system.bin
	@$(COPY) system/build/ioctl $(TARGET_DIR_NAME)/ioctl


.PHONY : btl-appset
btl-appset : _build_prepare
	@$(MAKE) -C btl-appset -s -j 1 -fMakefile all
	@$(COPY) btl-appset/build/output/appset.hex $(TARGET_DIR_NAME)/btl-appset.hex
	@$(COPY) btl-appset/build/output/appset.elf $(TARGET_DIR_NAME)/btl-appset.elf
	@$(COPY) btl-appset/build/output/appset.bin $(TARGET_DIR_NAME)/btl-appset.bin

.PHONY : clean-btl-appset
clean-btl-appset : _build_prepare
	@$(MAKE) -C btl-appset -s -j 1 -fMakefile clean


.PHONY : install
install :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex


.PHONY : install-app
install-app :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_app.hex


.PHONY : install-btl
install-btl :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_btl.hex


.PHONY : install-app-system
install-app-system :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/app-system.hex


.PHONY : install-app-appset
install-app-appset :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/app-appset.hex


.PHONY : config
config :
	@$(CONFIGTOOL)


.PHONY : config-app-system
config-app-system :
	@$(CONFIGTOOL) ../../config/app/system/config.h


.PHONY : config-app-appset
config-app-appset :
	@$(CONFIGTOOL) ../../app-appset/config/config.h


.PHONY : config-btl-system
config-btl-system :
	@$(CONFIGTOOL) ../../config/btl/system/config.h


.PHONY : config-btl-appset
config-btl-appset :
	@$(CONFIGTOOL) ../../btl-appset/config/config.h


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

