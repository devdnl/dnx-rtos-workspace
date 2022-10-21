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
# defines project path with binaries
TARGET_DIR_NAME = build
TOOLCHAIN = arm-none-eabi-
PROJNAME = project

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
.PHONY : all
all : system appset
	@$(MKDIR) $(TARGET_DIR_NAME)
	@$(CAT) system/build/output/dnx.elf > $(TARGET_DIR_NAME)/$(PROJNAME).elf
	@$(OBJCOPY) --dump-section .text=$(TARGET_DIR_NAME)/appset.text appset/build/output/appset.elf
	@$(OBJCOPY) --add-section .text.appset=$(TARGET_DIR_NAME)/appset.text build/$(PROJNAME).elf
	@$(REMOVE) $(TARGET_DIR_NAME)/appset.text
	@$(OBJCOPY) --change-section-address .text.appset=0x8010000 $(TARGET_DIR_NAME)/$(PROJNAME).elf

.PHONY : clean
clean :
	@if [ -d system/config-default ]; then $(REMOVE) -rf system/config; $(RENAME) -f system/config-default system/config; fi
	@if [ -d appset/config-default ]; then $(REMOVE) -rf appset/config; $(RENAME) -f appset/config-default appset/config; fi
	@$(MAKE) -C system -s -j 1 -fMakefile clean
	@$(MAKE) -C appset -s -j 1 -fMakefile clean
	@$(REMOVE) -r $(TARGET_DIR_NAME)

.PHONY : system
system :
	@if [ -d system/config-default ]; then $(REMOVE) -rf system/config; $(RENAME) -f system/config-default system/config; fi
	@$(RENAME) system/config system/config-default
	@$(COPY) config/system system/config
	@$(MAKE) -C system -s -j 1 -fMakefile all
	@$(REMOVE) -rf system/config
	@$(RENAME) system/config-default system/config

.PHONY : appset
appset :
	@if [ -d appset/config-default ]; then $(REMOVE) -rf appset/config; $(RENAME) -f appset/config-default appset/config; fi
	@$(RENAME) appset/config appset/config-default
	@$(COPY) config/appset appset/config
	@$(MAKE) -C appset -s -j 1 -fMakefile all
	@$(REMOVE) -rf appset/config
	@$(RENAME) appset/config-default appset/config

.PHONY : install
install :
	@$(FLASH_CPU)

.PHONY : install-system
install-system :
	@$(MAKE) -C system -s -j 1 -fMakefile install

.PHONY : install-appset
install-appset :
	@$(MAKE) -C appset -s -j 1 -fMakefile install

.PHONY : config
config :
	@$(CONFIGTOOL)

.PHONY : config-system
config-system :
	@$(CONFIGTOOL) ../../system/config/config.h
	
.PHONY : config-appset
config-appset :
	@$(CONFIGTOOL) ../../appset/config/config.h
	
.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

.PHONY : system-rungens
system-rungens :
	@$(MAKE) -C system -s -j 1 -fMakefile rungens

####################################################################################################
# help
####################################################################################################
.PHONY : help
help :
	@$(ECHO) "This is help for this $(THIS_MAKEFILE)"
	@$(ECHO) "Possible targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   all                 build all sources in respository"
	@$(ECHO) "   clean               clean project"
	@$(ECHO) "   system              build system"
	@$(ECHO) "   appset              build application set"
	@$(ECHO) "   install             install (flash) all build files"
	@$(ECHO) "   install-system      install (flash) system files"
	@$(ECHO) "   install-appset      install (flash) appset files"
	@$(ECHO) "   config              start configurator"
	@$(ECHO) "   config-system       system configuration"
	@$(ECHO) "   config-appset       appset configuration"
	@$(ECHO) ""

