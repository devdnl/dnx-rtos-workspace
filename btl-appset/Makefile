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

ifeq ($(CONFIG_DIR),)
	CONFIG_DIR=./config
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
COPTS = 

#---------------------------------------------------------------------------------------------------
# DEFAULT COMPILER FLAGS
#---------------------------------------------------------------------------------------------------
TOOLCHAIN = $(__PROJECT_TOOLCHAIN__)

AFLAGS   = -c \
           -g \
           -ggdb3 \
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
           -Werror=array-bounds \
           -Wno-enum-conversion \
           -include $(CONFIG) \
           -include ./build/defs.h \
           -I./build\
           -I$(CONFIG_DIR)\
           -Wno-main\
           $(CPUCONFIG_CFLAGS)

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
           -Wno-nonnull-compare  \
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
           -include ./build/defs.h \
           -I./build\
           -I$(CONFIG_DIR)\
           $(CPUCONFIG_CXXFLAGS)

LFLAGS   = -g \
           $(CPUCONFIG_LDFLAGS) \
           -Wl,--gc-sections \
           -mno-unaligned-access \
           -Wl,-u,vectors \
           -Wl,-Map=$(TARGET_PATH)/$(PROJECT).map,--cref,--no-warn-mismatch \
           -Wall \
           -specs=nano.specs -specs=rdimon.specs \
           -Wl,--print-memory-usage \
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
TARGET_PATH     = $(TARGET_DIR_NAME)/output
GEN_PROG_DIR    = $(TARGET_DIR_NAME)/program

# defines object folder name
OBJ_DIR_NAME    = obj

# dependencies file name
DEP_FILE_NAME   = $(PROJECT).d

# folder localizations
APP_LOC         = src
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
MKDEP      = /usr/bin/makedepend
WC         = /usr/bin/wc
PYTHON3    = /usr/bin/python3
CC         = $(COPTS) $(TOOLCHAIN)gcc
CXX        = $(COPTS) $(TOOLCHAIN)g++
LD         = $(TOOLCHAIN)g++
AS         = $(TOOLCHAIN)gcc -x assembler-with-cpp
AR         = $(TOOLCHAIN)ar
OBJCOPY    = $(TOOLCHAIN)objcopy
OBJDUMP    = $(TOOLCHAIN)objdump
SIZE       = $(TOOLCHAIN)size
CONFIGTOOL = ./tools/configtool.sh
CODECHECK  = cppcheck
ADDAPPS    = ./tools/addapps.sh
FLASH_CPU  = ./tools/flash.sh
RESET_CPU  = ./tools/reset.sh
GIT_HOOKS  = ./tools/apply_git_hooks.sh
DOXYGEN    = ./tools/doxygen.sh
RELEASEPKG = ./tools/releasepkg.sh
RUNGENS    = ./tools/rungens.sh
FINDGVAR   = ./tools/find_global_vars.sh
SREC_CAT   = /bin/srec_cat
MKFPKG     = $(PYTHON3) ../system/tools/mkfpkg.py

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
SEARCHPATH_TARGET   = $(foreach var, $(HDRLOC_$(TARGET)),-I$(var)) -Isrc/
SEARCHPATH_PROGRAMS = $(foreach var, $(_HDRLOC_PROGRAMS),-I$(var))
SEARCHPATH_LIB      = $(foreach var, $(_HDRLOC_LIB),-I$(var))
SEARCHPATH_CORE     = $(foreach var, $(_HDRLOC_CORE),-I$(var))
SEARCHPATH_NOARCH   = $(foreach var, $(_HDRLOC_NOARCH),-I$(var))
SEARCHPATH_ARCH     = $(foreach var, $(_HDRLOC_ARCH),-I$(var))

# main target without defined prefixes
TARGET = $(__CPU_ARCH__)

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

# functions
FIND_GLOBAL_VARS_LIBS_PROGS = if [[ "$@" =~ $(APP_PRG_LOC) ]] || [[ "$@" =~ $(APP_LIB_LOC) ]]; then $(FINDGVAR) $@ || ($(RM) $@; exit 1); fi

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all : generate apply_git_hooks
	@$(MAKE) -s -j 1 -f$(THIS_MAKEFILE) rungens
	@$(MAKE) -s -j 1 -f$(THIS_MAKEFILE) build_start

.PHONY : build_start
build_start : dependencies buildobjects buildarchive linkobjects hex status

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
	@$(ECHO) "   flash, install      flash target CPU by using ./tools/flash.sh script"
	@$(ECHO) "   reset               reset target CPU by using ./tools/reset.sh script"
	@$(ECHO) "   release             create Release package"
	@$(ECHO) "   doc                 create documentation (Doxygen)"
	@$(ECHO) "   rungens             start generator scripts"

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
	@$(SIZE) -B -t --common $(TARGET_PATH)/$(PROJECT).a > $(TARGET_PATH)/$(PROJECT).size

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
	@$(RM) $(TARGET_PATH)/*.*

	@$(MKDIR) $(GEN_PROG_DIR)
	@$(ECHO) "Adding user's programs and libraries to the project..."
	@$(SHELL) $(ADDAPPS) ./$(APP_LOC) $(GEN_PROG_DIR) $(__PROGRAMS_LIST__) $(__PROGRAMS_EXCLUDE_LIST__) $(CONFIG_DIR)

	@$(ECHO) "Obtaining git hash..."
	@$(ECHO) "#define COMMIT_HASH \"$(shell git rev-parse --short HEAD 2>/dev/null)"\" > build/defs.h
	@$(ECHO) "#define BRANCH_NAME \"$(shell git symbolic-ref --short HEAD 2>/dev/null)"\" >> build/defs.h
	@$(ECHO) "#define BOARD_NAME \"$(BOARD_NAME)\"" >> build/defs.h
	@$(ECHO) "#define board_name_$(BOARD_NAME)" >> build/defs.h
	@$(ECHO) "#define PROJECT_OPTS \"$(PROJECT_OPTS)\"" >> build/defs.h

####################################################################################################
# Start all generators
####################################################################################################
.PHONY : rungens
rungens :
	@$(RUNGENS) $(GENERATOR)

####################################################################################################
# Copy git hooks to git repository
####################################################################################################
.PHONY : apply_git_hooks
apply_git_hooks :
	@$(GIT_HOOKS)

####################################################################################################
# makes dependences
####################################################################################################
.PHONY : dependencies
dependencies :
	@$(ECHO) "Creating dependencies for '$(TARGET)' target..."
	@$(MKDIR) $(TARGET_PATH)
	@$(ECHO) "" > $(TARGET_PATH)/$(DEP_FILE_NAME)
	@$(MKDEP)    -f $(TARGET_PATH)/$(DEP_FILE_NAME) -p $(OBJ_PATH)/ -o .$(OBJ_EXT) $(SEARCHPATH_TARGET) -Y -- $(CFLAGS_$(TARGET)) -- $(CSRC) $(CXXSRC) >/dev/null 2>&1
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
	@$(LD) $(TARGET_PATH)/$(PROJECT).a $(LFLAGS) -o $(TARGET_PATH)/$(PROJECT).elf
	@#$(LD) $(OBJECTS_ALL) $(LFLAGS) -o $(TARGET_PATH)/$(PROJECT).elf

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
	$(FIND_GLOBAL_VARS_LIBS_PROGS)

####################################################################################################
# rule used to compile object files from C++ sources
####################################################################################################
$(OBJ_PATH)/%.$(OBJ_EXT) : %.$(CXX_EXT) $(THIS_MAKEFILE)
	@$(ECHO) "Compiling: $<"
	@$(MKDIR) $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SEARCHPATH) $< -o $@
	$(FIND_GLOBAL_VARS_LIBS_PROGS)

####################################################################################################
# rule used to compile object files from assembler sources
####################################################################################################
$(OBJ_PATH)/%.$(OBJ_EXT) : %.$(AS_EXT) $(THIS_MAKEFILE)
	@$(ECHO) "Compiling: $<"
	@$(MKDIR) $(dir $@)
	@$(AS) $(AFLAGS) $(SEARCHPATH) $< -o $@

####################################################################################################
# clean target
####################################################################################################
.PHONY : cleantarget
cleantarget :
	@$(ECHO) "Cleaning target..."
	-@$(RM) -r $(OBJ_PATH) $(LST_PATH)
	-@$(RM) $(TARGET_DIR_NAME)/*.*

####################################################################################################
# clean all targets
####################################################################################################
.PHONY : clean
clean :
	@$(ECHO) "Deleting all build files..."
	-@$(RM) -r $(TARGET_DIR_NAME) ./doc/doxygen/html/ ./doc/doxygen/latex ./$(APP_LOC)/Makefile ./$(APP_LOC)/program_registration.c

####################################################################################################
# flash target CPU by using ./tools/flash.sh script
####################################################################################################
.PHONY : flash
flash:
	@$(FLASH_CPU) ../$(TARGET_PATH)/appset.hex

.PHONY : install
install : flash

####################################################################################################
# reset target CPU by using ./tools/flash.sh script
####################################################################################################
.PHONY : reset
reset:
	@$(RESET_CPU)

####################################################################################################
# target used to create Release archive
####################################################################################################
.PHONY : release
release: clean
	@$(SHELL) $(RELEASEPKG)

####################################################################################################
# target used to Doxygen documentation
####################################################################################################
.PHONY : doc
doc:
	$(DOXYGEN)

####################################################################################################
# include all dependencies
####################################################################################################
-include $(TARGET_PATH)/$(DEP_FILE_NAME)
