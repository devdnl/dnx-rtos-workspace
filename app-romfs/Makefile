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

#---------------------------------------------------------------------------------------------------
# VARIABLE DEFINITIONS
#---------------------------------------------------------------------------------------------------
ROMFS_NAME      = $(__ROMFS_NAME__)
ROMFS_ADDRESS   = $(__ROMFS_ADDRESS__)
TARGET_DIR_NAME = build
ECHO            = /bin/echo -e
RM              = /bin/rm -rf
MKDIR           = /bin/mkdir -p
SIZEOF          = /usr/bin/stat -c %s
SREC_CAT        = /bin/srec_cat
MKFPKG          = $(PYTHON3) ../system/tools/mkfpkg.py

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all :
	@$(ECHO) "Creating '$(ROMFS_NAME)' ROMFS image..."
	@$(MKDIR) $(TARGET_DIR_NAME)
	@$(MKFPKG) content $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin
	@$(SREC_CAT) $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin -binary -offset $(ROMFS_ADDRESS) -output $(TARGET_DIR_NAME)/$(ROMFS_NAME).hex -Intel
	@$(ECHO) "ROMFS '$(ROMFS_NAME)' size: $$($(SIZEOF) $(TARGET_DIR_NAME)/$(ROMFS_NAME).bin) bytes."

.PHONY : clean
clean :
	@$(RM) $(TARGET_DIR_NAME)
