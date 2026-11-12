# Makefile for GNU make
####################################################################################################
#
# MIT License
#
# Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>
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
# Appset build base: builds the appset (programs and libraries) selected by the
# appset directory with the given configuration:
#
#   make -C tools/appset all APPSET_DIR=<appset> [CONFIG_DIR=<config>] [BUILD_DIR=<build>]
#
#   APPSET_DIR  appset directory: programs/ and libs/ (*.mk files of common
#               programs and libraries, directories of local ones)
#   CONFIG_DIR  appset configuration (default: tools/appset/config)
#   BUILD_DIR   build directory (default: build/<appset name>)
#   TABLE_SYMBOL  POSIX simulator: name of the global symbol of the program table
#               (default: _program_table_desc, the main appset), other appsets
#               linked with the same kernel need unique names
#
# Relative paths are relative to the workspace directory.
#
####################################################################################################

# workspace and base paths
BASE_DIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WORKSPACE   := $(abspath $(BASE_DIR)/../..)
COMMON_APPS := $(WORKSPACE)/common_apps
COMMON_LIBS := $(WORKSPACE)/common_libs

# path relative to workspace (or absolute)
ws_path = $(abspath $(if $(filter /%,$(1)),$(1),$(WORKSPACE)/$(1)))

ifeq ($(CONFIG_DIR),)
override CONFIG_DIR := $(BASE_DIR)/config
else
override CONFIG_DIR := $(call ws_path,$(CONFIG_DIR))
endif

ifneq ($(APPSET_DIR),)
override APPSET_DIR := $(call ws_path,$(APPSET_DIR))
endif

ifeq ($(BUILD_DIR),)
override BUILD_DIR := $(WORKSPACE)/build/$(if $(APPSET_DIR),$(notdir $(APPSET_DIR)),appset)
else
override BUILD_DIR := $(call ws_path,$(BUILD_DIR))
endif

CONFIG=$(CONFIG_DIR)/config.h
include $(CONFIG)

ifeq ($(__PROGRAMS_EXCLUDE_LIST__),"")
	__PROGRAMS_EXCLUDE_LIST__=" "
endif

####################################################################################################
# PROJECT CONFIGURATION
####################################################################################################
# project name
PROJECT = $(__PROJECT_NAME__)

#---------------------------------------------------------------------------------------------------
# DEFAULT COMPILER FLAGS
#---------------------------------------------------------------------------------------------------
TOOLCHAIN = $(__PROJECT_TOOLCHAIN__)
CC_WRAPPER = 

# microcontroller specific flags (not used by the POSIX simulator)
ifeq ($(__CPU_ARCH__), posix)
MCU_CFLAGS =
MCU_LFLAGS =
else
MCU_CFLAGS = -mno-unaligned-access
MCU_LFLAGS = -mno-unaligned-access -Wl,-u,vectors -specs=nano.specs -specs=rdimon.specs -Wl,--print-memory-usage
endif

AFLAGS   = -c -MMD -MP \
           -g \
           -ggdb3 \
           -include $(CONFIG) \
           $(CPUCONFIG_AFLAGS)

CFLAGS   = -c -MMD -MP \
           -g \
           -ggdb3  \
           -Os \
           -std=c18 \
           -ffunction-sections \
           -Wall \
           -Wextra \
           -Wparentheses \
           $(MCU_CFLAGS) \
           -Werror=implicit-function-declaration \
           -Werror=int-conversion \
           -Werror=incompatible-pointer-types \
           -Werror=type-limits \
           -Werror=return-type \
           -Werror=implicit-int \
           -Werror=parentheses \
           -Werror=enum-compare \
           -Werror=discarded-qualifiers \
           -Werror=override-init \
           -Werror=misleading-indentation \
           -Werror=switch \
           -Werror=stringop-overflow \
           -Werror=overflow \
           -Werror=array-bounds \
           -Wno-enum-conversion \
           -include $(CONFIG) \
           -ffile-prefix-map=$(WORKSPACE)/= \
           -I$(BUILD_DIR)\
           -I$(CONFIG_DIR)\
           -Wno-main\
           $(CPUCONFIG_CFLAGS)

CXXFLAGS = -c -MMD -MP \
           -g \
           -ggdb3 \
           -Os \
           -std=c++11 \
           -ffunction-sections \
           -fno-rtti \
           -fno-exceptions \
           -fno-unwind-tables \
           $(MCU_CFLAGS) \
           -Wall \
           -Wextra \
           -Wparentheses \
           -Wno-nonnull-compare \
           -Werror=parentheses \
           -Werror=type-limits \
           -Werror=return-type \
           -Werror=enum-compare \
           -Werror=misleading-indentation \
           -Werror=switch \
           -Werror=stringop-overflow \
           -Werror=overflow \
           -Werror=array-bounds \
           -include $(CONFIG) \
           -ffile-prefix-map=$(WORKSPACE)/= \
           -I$(BUILD_DIR)\
           -I$(CONFIG_DIR)\
           $(CPUCONFIG_CXXFLAGS)

LFLAGS   = -g \
           $(CPUCONFIG_LDFLAGS) \
           -Wl,--gc-sections \
           -Wl,-Map=$(TARGET_PATH)/$(PROJECT).map,--cref,--no-warn-mismatch \
           -Wall \
           $(MCU_LFLAGS) \
           -lm

# POSIX simulator: appset is a relocatable object linked with kernel. Only the
# program table is exported, all other symbols are local (no conflicts with
# kernel and host library symbols). The table of other appsets linked with the
# same kernel is renamed (TABLE_SYMBOL), siminit finds them in the section
# dnx_appset_tables (program_registration.c) and mounts them (appsetfs).
TABLE_SYMBOL ?= _program_table_desc

# name of other appset of the POSIX simulator (renamed table): reference of the table is put in
# the section dnx_appset_tables, siminit mounts the appset in /opt/<name> (appsetfs)
APPSET_TABLE_NAME = $(if $(filter-out _program_table_desc,$(TABLE_SYMBOL)),$(notdir $(abspath $(APPSET_DIR))))
POSIX_OBJCOPY_FLAGS = $(if $(filter-out _program_table_desc,$(TABLE_SYMBOL)),--redefine-sym _program_table_desc=$(TABLE_SYMBOL)) \
                      --keep-global-symbol=$(TABLE_SYMBOL) \
                      --redefine-sym __data_init_start=_appset_data_init_start \
                      --redefine-sym __data_start=_appset_data_start \
                      --redefine-sym __data_size=_appset_data_size \
                      --redefine-sym __bss_start=_appset_bss_start \
                      --redefine-sym __bss_size=_appset_bss_size

#---------------------------------------------------------------------------------------------------
# FILE EXTENSIONS CONFIGURATION
#---------------------------------------------------------------------------------------------------
OBJ_EXT = o
C_EXT   = c
CXX_EXT = cpp
AS_EXT  = s

#---------------------------------------------------------------------------------------------------
# FILE AND DIRECTORY NAMES
#---------------------------------------------------------------------------------------------------
# defines project path with binaries
TARGET_DIR_NAME = $(BUILD_DIR)
TARGET_PATH     = $(TARGET_DIR_NAME)/output
GEN_PROG_DIR    = $(TARGET_DIR_NAME)/program

# defines object folder name
OBJ_DIR_NAME    = obj

# dependencies file name
DEP_FILE_NAME   = $(PROJECT).d

# folder localizations
APP_LOC         = $(APPSET_DIR)
APP_PRG_LOC     = $(APP_LOC)/programs
APP_LIB_LOC     = $(APP_LOC)/libs

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
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
SREC_CAT   = /bin/srec_cat
CODECHECK  = cppcheck
CC         = $(CC_WRAPPER) $(TOOLCHAIN)gcc
CXX        = $(CC_WRAPPER) $(TOOLCHAIN)g++
LD         = $(TOOLCHAIN)g++
AS         = $(TOOLCHAIN)gcc -x assembler-with-cpp
AR         = $(TOOLCHAIN)ar
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
SYNCDIR    = $(PYTHON3) $(WORKSPACE)/kernel/tools/syncdir.py
CONFIGTOOL = $(SHELL) $(WORKSPACE)/kernel/tools/configtool.sh
ADDAPPS    = $(SHELL) $(BASE_DIR)/addapps.sh
RUNGENS    = $(SHELL) $(BASE_DIR)/rungens.sh
FINDGVAR   = NM=$(TOOLCHAIN)nm $(SHELL) $(BASE_DIR)/find_global_vars.sh

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# number of threads used in compilation (cpu count)
THREAD = $(shell $(ECHO) $$($(CAT) /proc/cpuinfo | $(GREP) processor | $(WC) -l))

# sets header search path (adds -I flags to paths)
SEARCHPATH          = $(foreach var, $(HDRLOC),-I$(var))

# main target without defined prefixes
TARGET = $(__CPU_ARCH__)

# object path
OBJ_PATH = $(TARGET_DIR_NAME)/$(TARGET)/$(OBJ_DIR_NAME)

# list of sources to compile
-include $(GEN_PROG_DIR)/Makefile   # file is created in the addapps script

# defines objects localizations (sources and header paths are absolute: PROG_ROOT_<NAME>,
# LIB_ROOT_<NAME>, generated files)
HDRLOC           = $(APP_LOC) $(HDRLOC_PROGRAMS) $(COMMON_APPS) $(HDRLOC_LIB) $(COMMON_LIBS)

# defines all C and C++ sources (other entries, e.g. empty root paths, are ignored)
CSRC             = $(filter %.$(C_EXT),$(CSRC_PROGRAMS) $(CSRC_LIB))
CXXSRC           = $(filter %.$(CXX_EXT),$(CXXSRC_PROGRAMS) $(CXXSRC_LIB))

# object path of source file: workspace sources in $(OBJ_PATH)/<path in workspace>, other
# sources in $(OBJ_PATH)/_ext/<absolute path>
obj_of = $(foreach f,$(1),$(if $(filter $(WORKSPACE)/%,$(f)),$(OBJ_PATH)/$(patsubst $(WORKSPACE)/%,%,$(f)),$(OBJ_PATH)/_ext$(f)))

# defines objects paths
OBJECTS_CSRC           = $(call obj_of,$(CSRC:.$(C_EXT)=.$(OBJ_EXT)))
OBJECTS_CXXSRC         = $(call obj_of,$(CXXSRC:.$(CXX_EXT)=.$(OBJ_EXT)))
OBJECTS_ALL            = $(OBJECTS_CSRC) $(OBJECTS_CXXSRC)

# functions: programs and libraries shall not contain global variables
FIND_GLOBAL_VARS_LIBS_PROGS = $(FINDGVAR) $@ || ($(RM) $@; exit 1)

.DELETE_ON_ERROR:

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all : check_appset generate
	@$(MAKE) -s -j 1 -f$(THIS_MAKEFILE) rungens
	@$(MAKE) -s -j 1 -f$(THIS_MAKEFILE) build_start

.PHONY : build_start
build_start : buildobjects linkobjects hex status

.PHONY : check_appset
check_appset :
ifeq ($(APPSET_DIR),)
	$(error APPSET_DIR is not defined (appset directory, e.g. APPSET_DIR=app-appset))
endif
	@if [ ! -d "$(APPSET_DIR)" ]; then $(ECHO) "Appset directory not found: $(APPSET_DIR)"; exit 1; fi

####################################################################################################
# help
####################################################################################################
.PHONY : help
help :
	@$(ECHO) "This is help for the appset build base"
	@$(ECHO) "Usage: make -C tools/appset <target> APPSET_DIR=<appset> [CONFIG_DIR=<config>] [BUILD_DIR=<build>]"
	@$(ECHO) "Possible targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   all                 build appset"
	@$(ECHO) "   config              configuration (text mode), default configuration without CONFIG_DIR"
	@$(ECHO) "   clean               clean build directory"
	@$(ECHO) ""
	@$(ECHO) "Non-build targets:"
	@$(ECHO) "   check               static code analyze by using cppcheck"
	@$(ECHO) "   checkprog           static code analyze of programs by using cppcheck"
	@$(ECHO) "   quickcheck          quick static code analyze by using cppcheck"
	@$(ECHO) "   rungens             start generator scripts"

####################################################################################################
# project configuration wizard
####################################################################################################
.PHONY : config
config :
	@$(ECHO) "Starting configtool..."
	@$(CONFIGTOOL) $(CONFIG)

####################################################################################################
# analisis
####################################################################################################
.PHONY : check checkprog quickcheck appsyntaxcheck
check : check_appset generate
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 \
	              --enable=warning,style,performance,portability,information,missingInclude \
	              --force --inconclusive \
	              --include=$(CONFIG_DIR)/project/project_flags.h \
	              $(SEARCHPATH) $(CSRC) $(CXXSRC)

checkprog : check_appset generate
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 --enable=warning,style,missingInclude \
	              --inconclusive \
	              -DCOMPILE_EPOCH_TIME=0 \
	              $(SEARCHPATH)\
	              $(CSRC_PROGRAMS)

quickcheck : check_appset generate
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 \
	              --enable=warning,style,performance,portability,missingInclude \
	              --force --inconclusive \
	              --include=$(CONFIG_DIR)/project/project_flags.h \
	              -I $(COMMON_LIBS)/libc/include/dnx \
	              $(CSRC) $(CXXSRC)

####################################################################################################
# create basic output files like hex, bin, lst etc.
####################################################################################################
.PHONY : hex
hex :
ifeq ($(TARGET), posix)
	@$(ECHO) 'Appset object is ready to link with kernel (POSIX simulator).'
else
	@$(ECHO) 'Creating IHEX image...'
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -R .bss -O ihex $(TARGET_PATH)/$(PROJECT).hex

	@$(ECHO) 'Creating binary image...'
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -R .bss -O binary $(TARGET_PATH)/$(PROJECT).bin
endif

	#@$(ECHO) 'Creating memory dump...'
	#@$(OBJDUMP) -x --syms $(TARGET_PATH)/$(PROJECT).elf > $(TARGET_PATH)/$(PROJECT).dmp

	#@$(ECHO) 'Creating extended listing....'
	#@$(OBJDUMP) -S $(TARGET_PATH)/$(PROJECT).elf > $(TARGET_PATH)/$(PROJECT).lst

	#@$(ECHO) 'Creating objects size list...'
	#@$(SIZE) -B -t --common $(TARGET_PATH)/$(PROJECT).a > $(TARGET_PATH)/$(PROJECT).size

####################################################################################################
# show compile status
####################################################################################################
.PHONY : status
status :
	@$(ECHO) "-----------------------------------"
	@$(ECHO) "| `$(DATE) "+Compilation completed: %H:%M:%S"` |"
	@$(ECHO) "-----------------------------------"

####################################################################################################
####################################################################################################
# Adds programs and libraries to the project
# This target is used to generate $(GEN_PROG_DIR)/program_registration.c,
# $(GEN_PROG_DIR)/Makefile and $(GEN_PROG_DIR)/layout.ld files required in the
# build process
####################################################################################################
.PHONY : generate
generate :
	@$(MKDIR) $(TARGET_PATH)

	@$(MKDIR) $(GEN_PROG_DIR)
	@$(MKDIR) $(GEN_PROG_DIR)-tmp
	@$(ECHO) "Adding user's programs and libraries to the project..."
	@APPSET_TABLE_NAME="$(APPSET_TABLE_NAME)" $(ADDAPPS) $(APP_LOC) $(GEN_PROG_DIR)-tmp "*" $(__PROGRAMS_EXCLUDE_LIST__) $(CONFIG_DIR)
	@$(SYNCDIR) $(GEN_PROG_DIR)-tmp/ $(GEN_PROG_DIR)/

	@$(ECHO) "Obtaining git hash..."
	@$(MKDIR) $(TARGET_DIR_NAME)/build
	@$(MKDIR) $(TARGET_DIR_NAME)/build-tmp
	@$(ECHO) "#define COMMIT_HASH \"$(shell git rev-parse --short HEAD 2>/dev/null)"\" > $(TARGET_DIR_NAME)/build-tmp/defs.h
	@$(ECHO) "#define BRANCH_NAME \"$(shell git symbolic-ref --short HEAD 2>/dev/null)"\" >> $(TARGET_DIR_NAME)/build-tmp/defs.h
	@$(ECHO) "#define BOARD_NAME \"$(BOARD_NAME)\"" >> $(TARGET_DIR_NAME)/build-tmp/defs.h
	@$(ECHO) "#define board_name_$(BOARD_NAME)" >> $(TARGET_DIR_NAME)/build-tmp/defs.h
	@$(ECHO) "#define PROJECT_OPTS \"$(PROJECT_OPTS)\"" >> $(TARGET_DIR_NAME)/build-tmp/defs.h
	@$(ECHO) "#define COMPILE_EPOCH_TIME $(shell $(DATE) "+%s")" > $(TARGET_DIR_NAME)/build-tmp/vars.h
	@$(SYNCDIR) $(TARGET_DIR_NAME)/build-tmp/ $(TARGET_DIR_NAME)/build/

####################################################################################################
# Start all generators
####################################################################################################
.PHONY : rungens
rungens :
	@$(RUNGENS) $(GENERATOR)

####################################################################################################
# linking rules
####################################################################################################
$(TARGET_PATH)/objects.list : $(OBJECTS_ALL)
	$(file >$@)
	$(foreach f,$(OBJECTS_ALL),$(file >>$@,$(f)))

.PHONY : linkobjects
linkobjects : $(TARGET_PATH)/objects.list
ifeq ($(TARGET), posix)
	@$(ECHO) "Linking (relocatable object)..."
	@$(LD) -r -nostdlib $(CPUCONFIG_LDFLAGS) @$(TARGET_PATH)/objects.list -o $(TARGET_PATH)/$(PROJECT)_r.o
	@$(OBJCOPY) $(POSIX_OBJCOPY_FLAGS) $(TARGET_PATH)/$(PROJECT)_r.o $(TARGET_PATH)/$(PROJECT).o
	@$(RM) $(TARGET_PATH)/$(PROJECT)_r.o
else
	@$(ECHO) "Linking..."
	@$(LD) @$(TARGET_PATH)/objects.list $(LFLAGS) -o $(TARGET_PATH)/$(PROJECT).elf
endif

####################################################################################################
# build objects
####################################################################################################
.PHONY : buildobjects buildobjects_$(TARGET)
buildobjects :
	@$(ECHO) "Starting building objects up to $(THREAD) threads..."
	@$(MAKE) -s -j$(THREAD) -f$(THIS_MAKEFILE) buildobjects_$(TARGET)

buildobjects_$(TARGET) : $(OBJECTS_ALL)

####################################################################################################
# rule used to compile object files from c sources
####################################################################################################
define compile_rules
$(OBJ_PATH)/$(1)%.$(OBJ_EXT) : $(2)%.$(C_EXT) $(THIS_MAKEFILE)
	@$$(ECHO) "Compiling: $$(patsubst $(WORKSPACE)/%,%,$$<)"
	@$$(MKDIR) $$(dir $$@)
	@$$(CC) $$(CFLAGS) $$(SEARCHPATH) $$< -o $$@
	@$$(FIND_GLOBAL_VARS_LIBS_PROGS)

$(OBJ_PATH)/$(1)%.$(OBJ_EXT) : $(2)%.$(CXX_EXT) $(THIS_MAKEFILE)
	@$$(ECHO) "Compiling: $$(patsubst $(WORKSPACE)/%,%,$$<)"
	@$$(MKDIR) $$(dir $$@)
	@$$(CXX) $$(CXXFLAGS) $$(SEARCHPATH) $$< -o $$@
	@$$(FIND_GLOBAL_VARS_LIBS_PROGS)
endef

# sources of workspace and other sources (absolute paths)
$(eval $(call compile_rules,,$(WORKSPACE)/))
$(eval $(call compile_rules,_ext/,/))

####################################################################################################
# clean all targets
####################################################################################################
.PHONY : clean
clean :
	@$(ECHO) "Deleting all build files..."
	-@$(RM) -r $(TARGET_DIR_NAME)

####################################################################################################
# include all dependencies
####################################################################################################
-include $(OBJECTS_ALL:.o=.d)
