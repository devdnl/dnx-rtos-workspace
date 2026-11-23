# Example targets of tests in the POSIX simulator for GNU make (tools/target_test_sim.mk)
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
# Tests of the system in the POSIX simulator (tools/simtest.py): the simulator is built with the
# test appset sim-env/testappset (added to SIM_APPSETS) and drvset partition $(SIM_DRVSET_DIR).
# Needs targets of the simulator (tools/target_sim.mk). Included by the project Makefile after
# tools/workspace.mk. The project which does not need these targets removes the include. This file
# belongs to the workspace and is updated with it, do not edit it.
####################################################################################################
ifndef TARGET_SIM_MK
$(error tools/target_test_sim.mk needs targets of the simulator: include tools/target_sim.mk before)
endif

####################################################################################################
# targets
####################################################################################################
help ::
	@$(ECHO) "Tests in POSIX simulator:"
	@$(ECHO) "   test-sim            build and test system in POSIX simulator (options: SIMTEST_OPTS=...),"
	@$(ECHO) "                       sim-env/testappset is added to SIM_APPSETS"
	@$(ECHO) ""

# the simulator is built by make called again (the test appset added to SIM_APPSETS, appset rules
# are created when the Makefile is read)
.PHONY : test-sim
test-sim :
	@$(MAKE) --no-print-directory -f $(THIS_MAKEFILE) build-sim build-sim-drvset \
	         SIM_APPSETS="$(strip $(SIM_APPSETS) sim-env/testappset)"
	@$(MAKE) -C tools/drvset -s -fMakefile check DRVSET_DIR=$(SIM_DRVSET_DIR) CONFIG_DIR=config/$(SIM_KERNEL_CONFIG) \
	         BUILD_DIR=$(TARGET_DIR_NAME)/sim-drvset KERNEL_ELF=$(TARGET_DIR_NAME)/dnx-sim
	@$(PYTHON3) tools/simtest.py --sim $(TARGET_DIR_NAME)/dnx-sim --rom $(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin\
	 --drvset $(TARGET_DIR_NAME)/sim-drvset.bin --log $(TARGET_DIR_NAME)/simtest.log $(SIMTEST_OPTS)
