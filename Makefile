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
CONFIGTOOL     = $(SHELL) ./kernel/tools/configtool.sh
FLASH_CPU      = $(SHELL) ./tools/flash.sh
MKFPKG         = $(PYTHON3) ./kernel/tools/mkfpkg.py
MKLINKERSCRIPT = $(SHELL) ./tools/create_linker_script.sh
SYNCDIR        = $(PYTHON3) ./kernel/tools/syncdir.py

#---------------------------------------------------------------------------------------------------
# MAKEFILE CORE (do not edit)
#---------------------------------------------------------------------------------------------------
# defines VALUES
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

# defines this makefile name
THIS_MAKEFILE = $(firstword $(MAKEFILE_LIST))

# define macro used to build kernel according to application name
define build_kernel
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS image..."
	$(MKLINKERSCRIPT) ./config/${$@_APP_NAME}/custom_linker_script.h ./config/${$@_APP_NAME}/custom_linker_script.ld
	$(REMOVE) kernel/build/ioctl
	$(MAKE) -C kernel -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) kernel/build/output/dnx.hex $(TARGET_DIR_NAME)/${$@_APP_NAME}.hex
	$(COPY) kernel/build/output/dnx.elf $(TARGET_DIR_NAME)/${$@_APP_NAME}.elf
	$(COPY) kernel/build/output/dnx.bin $(TARGET_DIR_NAME)/${$@_APP_NAME}.bin
	$(MKDIR) $(TARGET_DIR_NAME)/ioctl
	$(SYNCDIR) kernel/build/ioctl/ $(TARGET_DIR_NAME)/ioctl/
	$(REMOVE) kernel/build/ioctl
endef

# define macro used to build appset according to name
define build_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(eval $@_BOARD_NAME = $(3))
	$(eval $@_PROJECT_OPTS = $(4))
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile all BOARD_NAME=${$@_BOARD_NAME} CONFIG_DIR=../config/${$@_CONFIG_NAME} PROJECT_OPTS=${$@_PROJECT_OPTS}
	$(COPY) ${$@_APP_NAME}/build/output/appset.hex $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.hex
	$(COPY) ${$@_APP_NAME}/build/output/appset.elf $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.elf
	$(COPY) ${$@_APP_NAME}/build/output/appset.bin $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.bin
endef

# define macro used to build POSIX simulator kernel
define build_sim_kernel
	$(eval $@_APP_NAME = $(1))
	$(ECHO) "Start building dnx RTOS simulator kernel..."
	$(REMOVE) kernel/build/ioctl
	$(MAKE) -C kernel -s -j 1 -fMakefile all CONFIG_DIR=../config/${$@_APP_NAME}
	$(COPY) kernel/build/output/dnx.a $(TARGET_DIR_NAME)/${$@_APP_NAME}.a
	$(MKDIR) $(TARGET_DIR_NAME)/ioctl
	$(SYNCDIR) kernel/build/ioctl/ $(TARGET_DIR_NAME)/ioctl/
	$(REMOVE) kernel/build/ioctl
endef

# define macro used to build POSIX simulator appset
define build_sim_appset
	$(eval $@_APP_NAME = $(1))
	$(eval $@_CONFIG_NAME = $(2))
	$(MAKE) -C ${$@_APP_NAME} -s -j 1 -fMakefile all BOARD_NAME=posix CONFIG_DIR=../config/${$@_CONFIG_NAME} PROJECT_OPTS=sim
	$(COPY) ${$@_APP_NAME}/build/output/appset.o $(TARGET_DIR_NAME)/${$@_CONFIG_NAME}.o
endef

# define macro used to create ROMFS
define build_romfs
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

# define macro to clean kernel
define clean_kernel
	$(ECHO) -n "Cleaning kernel files: "
	$(MAKE) -C kernel -s -j 1 -fMakefile clean
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

# define macro used to configure kernel
define config_kernel
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure appset
define config_appset
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
endef

# define macro used to configure romfs
define config_romfs
	$(eval $@_CONFIG_NAME = $(1))
	$(CONFIGTOOL) ./config/${$@_CONFIG_NAME}/config.h
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
	@$(ECHO) "   build-btl           build bootloader (kernel + appset)"
	@$(ECHO) "   build-btl-kernel    build bootloader's kernel"
	@$(ECHO) "   build-sim           build POSIX simulator (Linux): build/dnx-sim"
	@$(ECHO) "   run-sim             build and start POSIX simulator (options: SIM_OPTS=...)"
	@$(ECHO) "   build-btl-appset    build bootloader's appset"
	@$(ECHO) "   build-app           build application (kernel + appset)"
	@$(ECHO) "   build-app-kernel    build application's kernel"
	@$(ECHO) "   build-app-appset    build application's appset"
	@$(ECHO) "   install             install (flash) bootloader and application"
	@$(ECHO) "   install-btl         install (flash) bootloader"
	@$(ECHO) "   install-app         install (flash) application"
	@$(ECHO) "   install-app-kernel  install (flash) application kernel"
	@$(ECHO) "   install-app-appset  install (flash) application appset"
	@$(ECHO) "   config-app-kernel   application kernel configuration"
	@$(ECHO) "   config-app-appset   application appset configuration"
	@$(ECHO) "   config-btl-kernel   bootloader kernel configuration"
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
	@$(call clean_kernel)
	@$(call clean_appset, app-appset)
	@$(call clean_appset, btl-appset)
	@$(call clean_romfs, app-romfs)
	@$(REMOVE) -r $(TARGET_DIR_NAME)


.PHONY : build-app
build-app : _build_prepare build-app-romfs build-app-kernel build-app-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/app-kernel.hex -Intel $(TARGET_DIR_NAME)/app-appset.hex -Intel $(TARGET_DIR_NAME)/app-romfs.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_app.hex -Intel
	@$(COPY) -R ./app-dist $(TARGET_DIR_NAME)/app-dist
	@$(COPY) $(TARGET_DIR_NAME)/app-kernel.bin $(TARGET_DIR_NAME)/app-dist/app-kernel.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-appset.bin $(TARGET_DIR_NAME)/app-dist/app-appset.bin
	@$(COPY) $(TARGET_DIR_NAME)/app-romfs.bin $(TARGET_DIR_NAME)/app-dist/app-romfs.bin
	@$(MKFPKG) $(TARGET_DIR_NAME)/app-dist $(TARGET_DIR_NAME)/$(PROJECT)-app.fpkg
	@$(ECHO) "Target app built successfully."


.PHONY : build-sim
build-sim : _build_prepare build-sim-romfs build-sim-kernel build-sim-appset
	@$(MAKE) -C kernel -s -j 1 -fMakefile link-sim CONFIG_DIR=../config/sim-kernel \
	         APPSET_OBJ=../$(TARGET_DIR_NAME)/sim-appset.o OUTPUT=../$(TARGET_DIR_NAME)/dnx-sim
	@$(ECHO) "Target sim built successfully: $(TARGET_DIR_NAME)/dnx-sim --rom $(TARGET_DIR_NAME)/sim-romfs.bin"

.PHONY : build-sim-kernel
build-sim-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_sim_kernel, sim-kernel)

.PHONY : build-sim-appset
build-sim-appset : _build_prepare
	@$(call clean_appset, app-appset)
	@$(call build_sim_appset, app-appset, sim-appset)

.PHONY : build-sim-romfs
build-sim-romfs : _build_prepare
	@$(call build_romfs, app-romfs, sim-romfs)

.PHONY : run-sim
run-sim : build-sim
	@$(TARGET_DIR_NAME)/dnx-sim --rom $(TARGET_DIR_NAME)/sim-romfs.bin $(SIM_OPTS)

.PHONY : build-btl
build-btl : _build_prepare build-btl-kernel build-btl-appset
	@$(SREC_CAT) $(TARGET_DIR_NAME)/btl-kernel.hex -Intel $(TARGET_DIR_NAME)/btl-appset.hex -Intel -o $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex -Intel
	@$(OBJCOPY) --input-target=ihex --output-target=binary $(TARGET_DIR_NAME)/$(PROJECT)_btl.hex $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin
	@$(MKDIR) $(TARGET_DIR_NAME)/btl-dist
	@$(COPY) $(TARGET_DIR_NAME)/$(PROJECT)_btl.bin $(TARGET_DIR_NAME)/btl-dist/btl.bin
	@$(MD5SUM) -b $(TARGET_DIR_NAME)/btl-dist/btl.bin | head -c 32 > $(TARGET_DIR_NAME)/btl-dist/btl.md5
	@$(MKFPKG) $(TARGET_DIR_NAME)/btl-dist $(TARGET_DIR_NAME)/$(PROJECT)-btl.fpkg


.PHONY : build-app-kernel
build-app-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, app-kernel)

.PHONY : build-app-appset
build-app-appset : _build_prepare
	@$(call build_appset, app-appset, app-appset, boardname, proj_opts)

.PHONY : build-app-romfs
build-app-romfs : _build_prepare
	@$(call build_romfs, app-romfs, app-romfs)

.PHONY : clean-app-appset
clean-app-appset : _build_prepare
	@$(call clean_appset, app-appset)


.PHONY : build-btl-kernel
build-btl-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_kernel, btl-kernel)

.PHONY : build-btl-appset
build-btl-appset : _build_prepare
	@$(call build_appset, btl-appset, btl-appset, boardname, proj_opts)

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


.PHONY : install-app-kernel
install-app-kernel :
	@$(call install_hex, app-kernel.hex)


.PHONY : install-app-appset
install-app-appset :
	@$(call install_hex, app-appset.hex)


.PHONY : install-app-romfs
install-app-romfs :
	@$(call install_hex, app-romfs.hex)


.PHONY : config-app-kernel
config-app-kernel :
	@$(call config_kernel, app-kernel)


.PHONY : config-app-appset
config-app-appset :
	@$(call config_appset, app-appset)


.PHONY : config-app-romfs
config-app-romfs :
	@$(call config_romfs, app-romfs)


.PHONY : config-btl-kernel
config-btl-kernel :
	@$(call config_kernel, btl-kernel)


.PHONY : config-btl-appset
config-btl-appset :
	@$(call config_appset, btl-appset)


.PHONY : config-sim-kernel
config-sim-kernel :
	@$(call config_kernel, sim-kernel)


.PHONY : config-sim-appset
config-sim-appset :
	@$(call config_appset, sim-appset)


.PHONY : config-sim-romfs
config-sim-romfs :
	@$(call config_romfs, sim-romfs)


.PHONY : appset-rungens
appset-rungens :
	@$(MAKE) -C appset -s -j 1 -fMakefile rungens

