# Makefile for GNU make
####################################################################################################
#
# AUTHOR: DANIEL ZORYCHTA
#
# Version: 20250517
#--------------------------------------------------------------------------------------------------
#
#    Copyright (C) 2025  Daniel Zorychta (daniel.zorychta@gmail.com)
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

include ./config/config.h

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
