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
# Makefile of the project: settings, the core of the workspace (tools/workspace.mk: macros, help,
# clean, test-drvset) and example targets: application (tools/target_app.mk), bootloader
# (tools/target_btl.mk), POSIX simulator (tools/target_sim.mk) and tests in the simulator
# (tools/target_test_sim.mk). These files are updated with the workspace, this file belongs to the
# project: settings and own targets are defined here. Each example file can be removed from the
# includes (target_test_sim.mk needs target_sim.mk), the project defines own targets with the
# macros of tools/workspace.mk instead (see README.md).
####################################################################################################

#---------------------------------------------------------------------------------------------------
# SETTINGS (other settings and their defaults: tools/workspace.mk, tools/target_*.mk)
#---------------------------------------------------------------------------------------------------
PROJECT = project
TOOLCHAIN = arm-none-eabi-
TARGET_DIR_NAME = build

# board name and options of the project passed to appsets (BOARD_NAME, PROJECT_OPTS)
APPSET_BOARD = boardname
APPSET_OPTS = proj_opts

# definitions of the project in build/defs.h of all appsets, board and simulator (NAME or NAME=VALUE)
APPSET_DEFS =

# POSIX simulator: appset and romfs of the simulator system
SIM_APPSET_DIR = app-appset
SIM_ROMFS_DIR = app-romfs

# drvset partition (drivers built separately from the kernel, tools/drvset): directory with
# Makefile.in of drivers, built with the application kernel configuration (config/app-kernel)
DRVSET_DIR ?= tools/drvset/example

#---------------------------------------------------------------------------------------------------
# CORE OF THE WORKSPACE AND EXAMPLE TARGETS (remove includes of targets not used by the project)
#---------------------------------------------------------------------------------------------------
include tools/workspace.mk
include tools/target_app.mk
include tools/target_btl.mk
include tools/target_sim.mk
include tools/target_test_sim.mk

####################################################################################################
# own targets of the project (help of the project: "help ::" rules)
####################################################################################################
