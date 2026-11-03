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
#
# ROMFS build base: creates the ROMFS image of the given directory:
#
#   make -C tools/romfs all ROMFS_DIR=<content> [CONFIG_DIR=<config>] [BUILD_DIR=<build>]
#
#   ROMFS_DIR   directory with files of the image
#   CONFIG_DIR  ROMFS configuration (default: tools/romfs/config)
#   BUILD_DIR   build directory (default: build/<ROMFS_DIR name>)
#
# Relative paths are relative to the workspace directory.
#
####################################################################################################

# workspace and base paths
BASE_DIR  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WORKSPACE := $(abspath $(BASE_DIR)/../..)

# path relative to workspace (or absolute)
ws_path = $(abspath $(if $(filter /%,$(1)),$(1),$(WORKSPACE)/$(1)))

ifeq ($(CONFIG_DIR),)
override CONFIG_DIR := $(BASE_DIR)/config
else
override CONFIG_DIR := $(call ws_path,$(CONFIG_DIR))
endif

ifneq ($(ROMFS_DIR),)
override ROMFS_DIR := $(call ws_path,$(ROMFS_DIR))
endif

ifeq ($(BUILD_DIR),)
override BUILD_DIR := $(WORKSPACE)/build/$(if $(ROMFS_DIR),$(notdir $(ROMFS_DIR)),romfs)
else
override BUILD_DIR := $(call ws_path,$(BUILD_DIR))
endif

CONFIG=$(CONFIG_DIR)/config.h
include $(CONFIG)

#---------------------------------------------------------------------------------------------------
# VARIABLE DEFINITIONS
#---------------------------------------------------------------------------------------------------
ROMFS_NAME      = $(__ROMFS_NAME__)
ROMFS_ADDRESS   = $(__ROMFS_ADDRESS__)
TARGET_DIR_NAME = $(BUILD_DIR)
ECHO            = /bin/echo -e
RM              = /bin/rm -rf
MKDIR           = /bin/mkdir -p
SIZEOF          = /usr/bin/stat -c %s
SREC_CAT        = /bin/srec_cat
PYTHON3         = /usr/bin/python3
MKFPKG          = $(PYTHON3) $(WORKSPACE)/kernel/tools/mkfpkg.py

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all :
ifeq ($(ROMFS_DIR),)
	$(error ROMFS_DIR is not defined (directory with files of the image, e.g. ROMFS_DIR=app-romfs))
endif
	@if [ ! -d "$(ROMFS_DIR)" ]; then $(ECHO) "ROMFS directory not found: $(ROMFS_DIR)"; exit 1; fi
	@$(ECHO) "Creating '$(ROMFS_NAME)' ROMFS image..."
	@$(MKDIR) $(TARGET_DIR_NAME)
	@$(MKFPKG) $(ROMFS_DIR) $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin -binary -offset $(ROMFS_ADDRESS) -output $(TARGET_DIR_NAME)/$(ROMFS_NAME).hex -Intel
	@$(ECHO) "ROMFS '$(ROMFS_NAME)' size: $$($(SIZEOF) $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin) bytes."

.PHONY : clean
clean :
	@$(RM) $(TARGET_DIR_NAME)
