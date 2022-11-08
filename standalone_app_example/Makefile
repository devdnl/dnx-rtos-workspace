# Makefile for GNU make
####################################################################################################
#
# AUTHOR: DANIEL ZORYCHTA
#
# Version: 20130516
#--------------------------------------------------------------------------------------------------
#
#    Copyright (C) 2011, 2012, 2013  Daniel Zorychta (daniel.zorychta@gmail.com)
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

####################################################################################################
# PROJECT CONFIGURATION
####################################################################################################
_NO_  = 0
_YES_ = 1

PROJECT = example
TOOLCHAIN = arm-none-eabi-
TARGET = stm32h7

ifeq ($(TARGET), stm32f1)
CPUCONFIG_AFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_CFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m3 -mthumb -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM3
CPUCONFIG_LDFLAGS=-mcpu=cortex-m3 -mthumb -mthumb-interwork -nostartfiles
endif

ifeq ($(TARGET), stm32f3)
CPUCONFIG_AFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_LDFLAGS=-mcpu=cortex-m4 -mthumb -mthumb-interwork -nostartfiles
endif

ifeq ($(TARGET), stm32f4)
CPUCONFIG_AFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM4
CPUCONFIG_LDFLAGS=-mcpu=cortex-m4 -mthumb -mthumb-interwork -nostartfiles
endif

ifeq ($(TARGET), stm32f7)
CPUCONFIG_AFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_LDFLAGS=-mcpu=cortex-m7 -mthumb -mthumb-interwork -nostartfiles
endif

ifeq ($(TARGET), stm32h7)
CPUCONFIG_AFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_CXXFLAGS=-mcpu=cortex-m7 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16 -ffast-math -fno-math-errno -mthumb-interwork -DGCC_ARMCM7
CPUCONFIG_LDFLAGS=-mcpu=cortex-m7 -mthumb -mthumb-interwork -nostartfiles
endif

# LIBC configuration
LIBC_FLAGS = -D__LIBC_COLOR_TERMINAL_ENABLE__=1 \
             -D__LIBC_SYSTEM_PROG__="\"dsh -e\"" \
             -D__LIBC_DNX_RTOS_ADDRESS__=0x08000000

#---------------------------------------------------------------------------------------------------
# DEFAULT COMPILER FLAGS
#---------------------------------------------------------------------------------------------------
AFLAGS   = -c \
           -g \
           -ggdb3 \
           -include ./config/config.h \
           -include ./build/defs.h \
           $(CPUCONFIG_AFLAGS)

CFLAGS   = -c \
           -g \
           -ggdb3  \
           -Os \
           -std=c18 \
           -ffunction-sections \
           -Wall \
           -Wextra \
           -Wparentheses \
           -mno-unaligned-access \
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
           -Wno-enum-conversion \
           -include ./build/defs.h \
           -I./build\
           -Wno-main\
           -fPIC -msingle-pic-base\
           -fno-plt\
           -DCOMPILE_EPOCH_TIME=$(shell $(DATE) "+%s") \
           $(CPUCONFIG_CFLAGS) \
           $(LIBC_FLAGS)

CXXFLAGS = -c \
           -g \
           -ggdb3 \
           -Os \
           -std=c++11 \
           -ffunction-sections \
           -fno-rtti \
           -fno-exceptions \
           -fno-unwind-tables \
           -mno-unaligned-access \
           -Wall \
           -Wextra \
           -Wparentheses \
           -Werror=parentheses \
           -Werror=type-limits \
           -Werror=return-type \
           -Werror=enum-compare \
           -Werror=misleading-indentation \
           -Werror=switch \
           -Werror=stringop-overflow \
           -Werror=overflow \
           -fPIC -msingle-pic-base\
           -fno-plt\
           -DCOMPILE_EPOCH_TIME=$(shell $(DATE) "+%s") \
           -include ./build/defs.h \
           -I./build\
           $(CPUCONFIG_CXXFLAGS) \
           $(LIBC_FLAGS)

LFLAGS   = -g \
           $(CPUCONFIG_LDFLAGS) \
           -T./tools/memory_layout.ld \
           -Wl,--gc-sections \
           -mno-unaligned-access \
           -Wl,-u,vectors \
           -fPIC -msingle-pic-base\
           -fno-plt\
           -Wl,-Map=$(TARGET_PATH)/$(PROJECT).map,--cref,--no-warn-mismatch \
           -Wall \
           -specs=nano.specs -specs=rdimon.specs \
           -lm

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
TARGET_DIR_NAME = build
GEN_PROG_DIR    = $(TARGET_DIR_NAME)/program

# defines object folder name
OBJ_DIR_NAME    = obj

# dependencies file name
DEP_FILE_NAME   = $(PROJECT).d

# folder localizations
APP_LOC         = src
APP_PRG_LOC     = $(APP_LOC)
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
MKDEP      = /usr/bin/makedepend
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
PYTHON2    = /usr/bin/python2
CC         = $(TOOLCHAIN)gcc
CXX        = $(TOOLCHAIN)g++
LD         = $(TOOLCHAIN)g++
AS         = $(TOOLCHAIN)gcc -x assembler-with-cpp
AR         = $(TOOLCHAIN)ar
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
ADDAPPS    = ./tools/addapps.sh
MKEXE      = ./tools/mkexe.sh

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY =

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# number of threads used in compilation (cpu count)
THREAD = $(shell $(ECHO) $$($(CAT) /proc/cpuinfo | $(GREP) processor | $(WC) -l))

# sets header search path (adds -I flags to paths)
SEARCHPATH          = $(foreach var, $(HDRLOC),-I$(var)) $(foreach var, $(HDRLOC_$(TARGET)),-I$(var))
SEARCHPATH_PROGRAMS = $(foreach var, $(_HDRLOC_PROGRAMS),-I$(var))
SEARCHPATH_LIB      = $(foreach var, $(_HDRLOC_LIB),-I$(var))

# target path
TARGET_PATH = $(TARGET_DIR_NAME)/output

# object path
OBJ_PATH = $(TARGET_DIR_NAME)/$(TARGET)/$(OBJ_DIR_NAME)

# list of sources to compile
-include $(GEN_PROG_DIR)/Makefile   # file is created in the addapps script

# defines objects localizations
_HDRLOC_PROGRAMS = $(foreach file, $(HDRLOC_PROGRAMS),$(APP_PRG_LOC)/$(file))
_HDRLOC_LIB      = $(foreach file, $(HDRLOC_LIB),$(APP_LIB_LOC)/$(file))
HDRLOC           = src/ $(_HDRLOC_PROGRAMS) $(_HDRLOC_LIB)

# defines all C sources
_CSRC_PROGRAMS   = $(foreach file, $(CSRC_PROGRAMS),$(APP_PRG_LOC)/$(file))
_CSRC_LIB        = $(foreach file, $(CSRC_LIB),$(APP_LIB_LOC)/$(file))
CSRC             = $(_CSRC_PROGRAMS) $(_CSRC_LIB)

# defines all C++ sources
_CXXSRC_PROGRAMS = $(foreach file, $(CXXSRC_PROGRAMS),$(APP_PRG_LOC)/$(file))
_CXXSRC_LIB      = $(foreach file, $(CXXSRC_LIB),$(APP_LIB_LOC)/$(file))
CXXSRC           = $(_CXXSRC_PROGRAMS) $(_CXXSRC_LIB)

# defines all assembler sources
ASRC             = $(foreach file, $(ASRC_ARCH),$(SYS_LOC)/$(file))

# convert source files names to objects
CSRCPROGRAMSOBJ        = $(_CSRC_PROGRAMS:.$(C_EXT)=.$(OBJ_EXT))
CSRCLIBOBJ             = $(_CSRC_LIB:.$(C_EXT)=.$(OBJ_EXT))
CSRCOBJ                = $(CSRC:.$(C_EXT)=.$(OBJ_EXT))
CXXSRCPROGRAMSOBJ      = $(_CXXSRC_PROGRAMS:.$(CXX_EXT)=.$(OBJ_EXT))
CXXSRCLIBOBJ           = $(_CXXSRC_LIB:.$(CXX_EXT)=.$(OBJ_EXT))
CXXSRCOBJ              = $(CXXSRC:.$(CXX_EXT)=.$(OBJ_EXT))
ASRCOBJ                = $(ASRC:.$(AS_EXT)=.$(OBJ_EXT))

# defines objects paths
OBJECTS_CSRC_PROGRAMS  = $(foreach var,$(CSRCPROGRAMSOBJ),$(OBJ_PATH)/$(var))
OBJECTS_CSRC_LIB       = $(foreach var,$(CSRCLIBOBJ),$(OBJ_PATH)/$(var))
OBJECTS_CXXSRC_PROGRAMS= $(foreach var,$(CXXSRCPROGRAMSOBJ),$(OBJ_PATH)/$(var))
OBJECTS_CXXSRC_LIB     = $(foreach var,$(CXXSRCLIBOBJ),$(OBJ_PATH)/$(var))
OBJECTS_ASRC           = $(foreach var,$(ASRCOBJ),$(OBJ_PATH)/$(var))
OBJECTS_CSRC           = $(OBJECTS_CSRC_PROGRAMS) $(OBJECTS_CSRC_LIB)
OBJECTS_CXXSRC         = $(OBJECTS_CXXSRC_PROGRAMS) $(OBJECTS_CXXSRC_LIB)
OBJECTS_ALL            = $(OBJECTS_ASRC) $(OBJECTS_CSRC) $(OBJECTS_CXXSRC)

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all : generate
	@$(MAKE) -s -j 1 -f$(THIS_MAKEFILE) build_start

.PHONY : build_start
build_start : dependencies buildobjects linkobjects hex status

####################################################################################################
# help
####################################################################################################
.PHONY : help
help :
	@$(ECHO) "This is help for this $(THIS_MAKEFILE)"
	@$(ECHO) "Possible targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   config              project configuration (text mode)"
	@$(ECHO) "   clean               clean project"
	@$(ECHO) ""
	@$(ECHO) "Non-build targets:"
	@$(ECHO) "   check               static code analyze by using cppcheck"
	@$(ECHO) "   quickcheck          quick static code analyze by using cppcheck"

####################################################################################################
# project configuration wizard
####################################################################################################
.PHONY : config
config : clean apply_git_hooks
	@$(ECHO) "Starting configtool..."
	@$(CONFIGTOOL)

####################################################################################################
# analisis
####################################################################################################
.PHONY : check checkprog quickcheck appsyntaxcheck
check :
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 \
	              --enable=warning,style,performance,portability,information,missingInclude \
	              --force --inconclusive \
	              --include=./config/project/flags.h \
	              $(SEARCHPATH) $(CSRC) $(CXXSRC)

checkprog :
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 --enable=warning,style,missingInclude \
	              --inconclusive \
	              -DCOMPILE_EPOCH_TIME=0 \
	              $(SEARCHPATH)\
	              $(foreach file, $(CSRC_PROGRAMS),$(APP_PRG_LOC)/$(file))

quickcheck :
	@$(CODECHECK) -j $(THREAD) -q --std=c99 --std=c++11 \
	              --enable=warning,style,performance,portability,missingInclude \
	              --force --inconclusive \
	              --include=./config/project/flags.h \
	              -I src/application/libs/libc/include/dnx \
	              $(CSRC) $(CXXSRC)

####################################################################################################
# create basic output files like hex, bin, lst etc.
####################################################################################################
.PHONY : hex
hex :
	@$(ECHO) 'Creating IHEX image...'
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -O ihex $(TARGET_PATH)/$(PROJECT).hex

	@$(ECHO) 'Creating binary image...'
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -O binary $(TARGET_PATH)/$(PROJECT).bin

	@$(ECHO) 'Creating memory dump...'
	@$(OBJDUMP) -x --syms $(TARGET_PATH)/$(PROJECT).elf > $(TARGET_PATH)/$(PROJECT).dmp

	@$(ECHO) 'Creating extended listing....'
	@$(OBJDUMP) -S $(TARGET_PATH)/$(PROJECT).elf > $(TARGET_PATH)/$(PROJECT).lst

	@$(ECHO) 'Creating objects size list...'
	@$(SIZE) -B -t --common $(TARGET_PATH)/$(PROJECT).elf > $(TARGET_PATH)/$(PROJECT).size

	@$(ECHO) 'Creating executable file...'
	@$(SHELL) $(MKEXE) $(TARGET_PATH)/$(PROJECT) 1 $(TARGET_PATH)/$(PROJECT)

	@$(ECHO) "Application size: $$($(SIZEOF) $(TARGET_PATH)/$(PROJECT)) bytes\n"

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
# This target is used to generate ./src/programs/program_registration.c,
# ./src/programs/Makefile, and ./src/lib/Makefile files required in the
# build process
####################################################################################################
.PHONY : generate
generate :
	@$(MKDIR) $(TARGET_PATH)

	@$(MKDIR) $(GEN_PROG_DIR)
	@$(ECHO) "Adding user's programs and libraries to the project..."
	@$(SHELL) $(ADDAPPS) ./$(APP_LOC) $(GEN_PROG_DIR)

	@$(ECHO) "Obtaining git hash..."
	@$(ECHO) "#define COMMIT_HASH \"$(shell git rev-parse --short HEAD 2>/dev/null)"\" > build/defs.h
	@$(ECHO) "#define BRANCH_NAME \"$(shell git symbolic-ref --short HEAD 2>/dev/null)"\" >> build/defs.h

####################################################################################################
# makes dependences
####################################################################################################
.PHONY : dependencies
dependencies :
	@$(ECHO) "Creating dependencies for '$(TARGET)' target..."
	@$(MKDIR) $(TARGET_PATH)
	@$(RM) $(TARGET_PATH)/*.*
	@$(ECHO) "" > $(TARGET_PATH)/$(DEP_FILE_NAME)
	@$(MKDEP) -a -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_PROGRAMS) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
	@$(MKDEP) -a -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_LIB) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
	@$(MKDEP) -a -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_CORE) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
	@$(MKDEP) -a -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_NOARCH) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
	@$(MKDEP) -a -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_ARCH) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
	@$(ECHO) "$(foreach var,$(CSRC),\n$(OBJ_PATH)/$(var:.$(C_EXT)=.$(OBJ_EXT)) : $(var))" >> $(TARGET_PATH)/$(DEP_FILE_NAME)
	@$(ECHO) "$(foreach var,$(CXXSRC),\n$(OBJ_PATH)/$(var:.$(CXX_EXT)=.$(OBJ_EXT)) : $(var))" >> $(TARGET_PATH)/$(DEP_FILE_NAME)
	@$(ECHO) "$(foreach var,$(ASRC),\n$(OBJ_PATH)/$(var:.$(AS_EXT)=.$(OBJ_EXT)) : $(var))" >> $(TARGET_PATH)/$(DEP_FILE_NAME)

####################################################################################################
# linking rules
####################################################################################################
.PHONY : linkobjects
linkobjects :
	@$(ECHO) "Linking..."
	@#$(LD) $(TARGET_PATH)/$(PROJECT).a $(LFLAGS) -o $(TARGET_PATH)/$(PROJECT).elf
	@$(LD) $(OBJECTS_ALL) $(LFLAGS) -o $(TARGET_PATH)/$(PROJECT).elf

####################################################################################################
# create objects archive
####################################################################################################
buildarchive :
	@$(ECHO) "Archiving..."
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_ASRC)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CSRC_PROGRAMS)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CSRC_LIB)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CSRC_CORE)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CSRC_NOARCH)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CSRC_ARCH)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CXXSRC_PROGRAMS)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CXXSRC_LIB)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CXXSRC_CORE)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CXXSRC_NOARCH)
	@$(AR) rTPcs $(TARGET_PATH)/$(PROJECT).a $(OBJECTS_CXXSRC_ARCH)

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
$(OBJ_PATH)/%.$(OBJ_EXT) : %.$(C_EXT) $(THIS_MAKEFILE)
	@$(ECHO) "Compiling: $<"
	@$(MKDIR) $(dir $@)
	@$(CC) $(CFLAGS) $(SEARCHPATH) $< -o $@

####################################################################################################
# rule used to compile object files from C++ sources
####################################################################################################
$(OBJ_PATH)/%.$(OBJ_EXT) : %.$(CXX_EXT) $(THIS_MAKEFILE)
	@$(ECHO) "Compiling: $<"
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SEARCHPATH) $< -o $@

####################################################################################################
# rule used to compile object files from assembler sources
####################################################################################################
$(OBJ_PATH)/%.$(OBJ_EXT) : %.$(AS_EXT) $(THIS_MAKEFILE)
	@$(ECHO) "Compiling: $<"
	@$(MKDIR) $(dir $@)
	@$(AS) $(AFLAGS) $(SEARCHPATH) $< -o $@

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
-include $(TARGET_PATH)/$(DEP_FILE_NAME)
