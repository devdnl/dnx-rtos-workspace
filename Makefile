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

#---------------------------------------------------------------------------------------------------
# BASIC PROGRAMS DEFINITIONS
#---------------------------------------------------------------------------------------------------
SHELL      = /bin/bash
ECHO       = /bin/echo -e
RM         = /bin/rm -f
MKDIR      = /bin/mkdir -p
DATE       = /bin/date
CAT        = /bin/cat
GREP       = /bin/grep
UNAME      = /bin/uname -s
SIZEOF     = /usr/bin/stat -c %s
MKDEP      = /usr/bin/makedepend
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
CC         = $(TOOLCHAIN)gcc
CXX        = $(TOOLCHAIN)g++
LD         = $(TOOLCHAIN)g++
AS         = $(TOOLCHAIN)gcc -x assembler-with-cpp
AR         = $(TOOLCHAIN)ar
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
CONFIGTOOL = ./tools/configtool.sh
FLASH_CPU  = ./tools/flash.sh
RESET_CPU  = ./tools/reset.sh
GIT_HOOKS  = ./tools/apply_git_hooks.sh
DOXYGEN    = ./tools/doxygen.sh
RELEASEPKG = ./tools/releasepkg.sh
RUNGENS    = ./tools/rungens.sh

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

.PHONY : clean
clean :
	@$(MAKE) -C system -s -j 1 -fMakefile clean
	@$(MAKE) -C appset -s -j 1 -fMakefile clean

.PHONY : system
system :
	@$(MAKE) -C system -s -j 1 -fMakefile all

.PHONY : appset
appset :
	@$(MAKE) -C appset -s -j 1 -fMakefile all

.PHONY : install
install : install-system install-appset

.PHONY : install-system
install-system :
	@$(MAKE) -C system -s -j 1 -fMakefile install

.PHONY : install-appset
install-appset :
	@$(MAKE) -C appset -s -j 1 -fMakefile install
	
.PHONY : config-system
config-system :
	@$(CONFIGTOOL) ../../system/config/config.h
	
.PHONY : config-appset
config-appset :
	@$(CONFIGTOOL) ../../appset/config/config.h
	
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
	@$(ECHO) "   config-system       system configuration"
	@$(ECHO) "   config-appset       appset configuration"
	@$(ECHO) ""

