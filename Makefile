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


.PHONY : all
all : _build_prepare btl app
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex -Intel;\


.PHONY : _build_prepare
_build_prepare :
	@$(MKDIR) $(TARGET_DIR_NAME)


.PHONY : btl bootloader
bootloader : btl
btl : _build_prepare btlsystem btlset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btlsystem.hex -Intel $(TARGET_DIR_NAME)/btlset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin


.PHONY : app application
application : app
app : _build_prepare appsystem appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/appsystem.hex -Intel $(TARGET_DIR_NAME)/appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel


.PHONY : clean
clean :
	@$(ECHO) -n "Cleaning system files: "
	@if [ -d system/config-default ]; then $(REMOVE) -rf system/config; $(RENAME) -f system/config-default system/config; fi
	@$(MAKE) -C system -s -j 1 -fMakefile clean

	@$(ECHO) -n "Cleaning appset files: "
	@if [ -d appset/config-default ]; then $(REMOVE) -rf appset/config; $(RENAME) -f appset/config-default appset/config; fi
	@$(MAKE) -C appset -s -j 1 -fMakefile clean

	@$(ECHO) -n "Cleaning btlset files: "
	@if [ -d btlset/config-default ]; then $(REMOVE) -rf btlset/config; $(RENAME) -f btlset/config-default btlset/config; fi
	@if [ -d btlset ]; then $(MAKE) -C btlset -s -j 1 -fMakefile clean; else $(ECHO) "No bootloader project."; fi


	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : appsystem
appsystem : _build_prepare
	@if [ -d system/config-default ]; then $(REMOVE) -rf system/config; $(RENAME) -f system/config-default system/config; fi
	@$(RENAME) system/config system/config-default
	@$(COPY) config/app/system system/config
	@$(MAKE) -C system -s -j 1 -fMakefile clean all
	@$(REMOVE) -rf system/config
	@$(RENAME) system/config-default system/config
	@$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/appsystem.hex
	@$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/appsystem.elf
	@$(COPY) system/build/output/dnx.bin $(TARGET_DIR_NAME)/appsystem.bin


.PHONY : appset
appset : _build_prepare
	@if [ -d appset/config-default ]; then $(REMOVE) -rf appset/config; $(RENAME) -f appset/config-default appset/config; fi
	@$(RENAME) appset/config appset/config-default
	@$(COPY) config/app/appset appset/config
	@$(MAKE) -C appset -s -j 1 -fMakefile all
	@$(REMOVE) -rf appset/config
	@$(RENAME) appset/config-default appset/config
	@$(COPY) appset/build/output/appset.hex $(TARGET_DIR_NAME)/appset.hex
	@$(COPY) appset/build/output/appset.elf $(TARGET_DIR_NAME)/appset.elf
	@$(COPY) appset/build/output/appset.bin $(TARGET_DIR_NAME)/appset.bin


.PHONY : btlsystem
btlsystem : _build_prepare
	@if [ -d system/config-default ]; then $(REMOVE) -rf system/config; $(RENAME) -f system/config-default system/config; fi
	@$(RENAME) system/config system/config-default
	@$(COPY) config/btl/system system/config
	@$(MAKE) -C system -s -j 1 -fMakefile clean all
	@$(REMOVE) -rf system/config
	@$(RENAME) system/config-default system/config
	@$(COPY) system/build/output/dnx.hex $(TARGET_DIR_NAME)/btlsystem.hex
	@$(COPY) system/build/output/dnx.elf $(TARGET_DIR_NAME)/btlsystem.elf


.PHONY : btlset
btlset : _build_prepare
	@if [ -d btlset/config-default ]; then $(REMOVE) -rf btlset/config; $(RENAME) -f btlset/config-default btlset/config; fi
	@$(RENAME) btlset/config btlset/config-default
	@$(COPY) config/btl/appset btlset/config
	@$(MAKE) -C btlset -s -j 1 -fMakefile all
	@$(REMOVE) -rf btlset/config
	@$(RENAME) btlset/config-default btlset/config
	@$(COPY) btlset/build/output/btlset.hex $(TARGET_DIR_NAME)/btlset.hex
	@$(COPY) btlset/build/output/btlset.elf $(TARGET_DIR_NAME)/btlset.elf


.PHONY : install
install :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_btl_app.hex


.PHONY : install-app
install-app :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_app.hex


.PHONY : install-btl
install-btl :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/$(PROJECT)_btl.hex


.PHONY : install-appsystem
install-appsystem :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/appsystem.hex


.PHONY : install-appset
install-appset :
	@$(FLASH_CPU) ../$(TARGET_DIR_NAME)/appset.hex


.PHONY : config
config :
	@$(CONFIGTOOL)


.PHONY : config-appsystem
config-appsystem :
	@$(CONFIGTOOL) ../../config/app/system/config.h


.PHONY : config-appset
config-appset :
	@$(CONFIGTOOL) ../../config/app/appset/config.h


.PHONY : config-btlsystem
config-btlsystem :
	@$(CONFIGTOOL) ../../config/btl/system/config.h


.PHONY : config-btlset
config-btlset :
	@$(CONFIGTOOL) ../../config/btl/appset/config.h


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

