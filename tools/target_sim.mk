# Example targets of the POSIX simulator for GNU make (tools/target_sim.mk)
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
# Targets of the POSIX simulator (Linux): kernel config/$(SIM_KERNEL_CONFIG), appset
# $(SIM_APPSET_DIR) with configuration config/$(SIM_APPSET_CONFIG), romfs $(SIM_ROMFS_DIR) with
# configuration config/$(SIM_ROMFS_CONFIG), other appsets $(SIM_APPSETS) and drvset partition
# $(SIM_DRVSET_DIR). Included by the project Makefile after tools/workspace.mk. The project which
# does not need these targets removes the include (also of tools/target_test_sim.mk) and defines
# own targets with the macros of tools/workspace.mk (this file is an example how to do it). This
# file belongs to the workspace and is updated with it, do not edit it.
####################################################################################################
TARGET_SIM_MK := 1

#---------------------------------------------------------------------------------------------------
# SETTINGS (defaults, set in the project Makefile before include)
#---------------------------------------------------------------------------------------------------
# POSIX simulator: appset and romfs directories of the simulator system (sources of the project),
# configuration names (config/<name>)
SIM_APPSET_DIR    ?= app-appset
SIM_ROMFS_DIR     ?= app-romfs
SIM_KERNEL_CONFIG ?= sim-kernel
SIM_APPSET_CONFIG ?= sim-appset
SIM_ROMFS_CONFIG  ?= sim-romfs

# POSIX simulator: board panel (run-sim PANEL=1, run-sim-example)
SIM_PANEL_SOCKET ?= $(TARGET_DIR_NAME)/dnx-panel.sock
SIM_PANEL_PORT   ?= 8080

# run-sim, run-sim-example: command prefix of simulator, e.g. SIM_DEBUG="gdbserver :2345" starts it
# under gdbserver for a debugger (tools/dnx-sim.launch of Eclipse)
SIM_DEBUG ?=

# run-sim-example: options of simulator (example board, files of sim-env in /host, network tap0)
SIM_EXAMPLE_OPTS ?= --board sim-env/boards/example.lua --hostfs host=sim-env --tap tap0

# POSIX simulator: other appsets linked with the simulator kernel (siminit mounts them by appsetfs
# in /opt/<appset directory name>, programs are started by path): list of <appset directory>
# [:<configuration directory>] (relative to the workspace or absolute), default configuration is
# config/sim-appset, e.g. SIM_APPSETS="btl-appset /home/user/project/appset:config/sim-appset";
# appset is built in $(TARGET_DIR_NAME)/sim-<appset directory name>, so names must be unique
SIM_APPSETS ?=

# drvset partition of POSIX simulator (option --drvset of simulator, used by test-sim): example
# driver and file system dsramfs
SIM_DRVSET_DIR ?= tools/drvset/test/sim

# objects of other appsets of the simulator
SIM_APPSET_OBJS = $(foreach a,$(SIM_APPSETS),$(TARGET_DIR_NAME)/$(call sim_appset_name,$(a)).o)

####################################################################################################
# targets
####################################################################################################
help ::
	@$(ECHO) "POSIX simulator targets:"
	@$(ECHO) "   build-sim           build POSIX simulator (Linux): $(TARGET_DIR_NAME)/dnx-sim"
	@$(ECHO) "                       other appsets in simulator: SIM_APPSETS=\"<appset>[:<config>] ...\""
	@$(ECHO) "   build-sim-drvset    build drvset partition of simulator: $(TARGET_DIR_NAME)/sim-drvset.bin (SIM_DRVSET_DIR=...,"
	@$(ECHO) "                       run-sim SIM_OPTS=\"--drvset $(TARGET_DIR_NAME)/sim-drvset.bin\")"
	@$(ECHO) "   run-sim             build and start POSIX simulator (options: SIM_OPTS=..., PANEL=1"
	@$(ECHO) "                       board panel in web browser, needs --board, PANEL_OPTS=...)"
	@$(ECHO) "   run-sim-example     build and start POSIX simulator with example board and panel"
	@$(ECHO) "                       (run-sim*: SIM_DEBUG=\"gdbserver :2345\" starts simulator for debugger)"
	@$(ECHO) "                       (http://localhost:$(SIM_PANEL_PORT), /host = sim-env, network tap0,"
	@$(ECHO) "                       tools/create_tap.sh, options: SIM_OPTS=...)"
	@$(ECHO) "   config-sim-kernel   simulator kernel configuration (also config-sim-appset, config-sim-romfs)"
	@$(ECHO) ""

.PHONY : build-sim
build-sim : _build_prepare build-sim-romfs build-sim-kernel build-sim-appset
	@$(call licenses_report, sim, dnx RTOS simulator, $(TARGET_DIR_NAME)/$(SIM_KERNEL_CONFIG).files \
	        $(TARGET_DIR_NAME)/sim-appset.files $(SIM_APPSET_OBJS:.o=.files), \
	        $(SIM_APPSET_DIR) $(foreach a,$(SIM_APPSETS),$(call sim_appset_dir,$(a))))
	@$(MAKE) -C kernel -s -j 1 -fMakefile link-sim CONFIG_DIR=../config/$(SIM_KERNEL_CONFIG) \
	         APPSET_OBJ="../$(TARGET_DIR_NAME)/sim-appset.o $(addprefix ../,$(SIM_APPSET_OBJS))" \
	         OUTPUT=../$(TARGET_DIR_NAME)/dnx-sim
	@$(ECHO) "Target sim built successfully: $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro"

.PHONY : build-sim-kernel
build-sim-kernel : _build_prepare
	@$(call clean_kernel)
	@$(call build_sim_kernel, $(SIM_KERNEL_CONFIG))

.PHONY : build-sim-appset
build-sim-appset : _build_prepare $(foreach a,$(SIM_APPSETS),build-$(call sim_appset_name,$(a)))
	@$(call build_sim_appset, $(SIM_APPSET_DIR), config/$(SIM_APPSET_CONFIG), sim-appset, _program_table_desc)

$(foreach a,$(SIM_APPSETS),$(eval $(call sim_appset_rule,$(a))))

.PHONY : build-sim-drvset
build-sim-drvset : _build_prepare
	@$(MAKE) -C tools/drvset -s -fMakefile all DRVSET_DIR=$(SIM_DRVSET_DIR) CONFIG_DIR=config/$(SIM_KERNEL_CONFIG) \
	         BUILD_DIR=$(TARGET_DIR_NAME)/sim-drvset
	@$(COPY) $(TARGET_DIR_NAME)/sim-drvset/output/drvset.bin $(TARGET_DIR_NAME)/sim-drvset.bin

.PHONY : build-sim-romfs
build-sim-romfs : _build_prepare
	@$(call build_romfs, $(SIM_ROMFS_DIR), $(SIM_ROMFS_CONFIG))

.PHONY : run-sim-example
run-sim-example : build-sim
	@$(PYTHON3) tools/simpanel.py --exit-with-parent --socket $(SIM_PANEL_SOCKET) --port $(SIM_PANEL_PORT)\
	 $(PANEL_OPTS) >$(TARGET_DIR_NAME)/simpanel.log 2>&1 & pid=$$!;\
	 sleep 0.5; if ! kill -0 $$pid 2>/dev/null; then $(CAT) $(TARGET_DIR_NAME)/simpanel.log; exit 1; fi;\
	 $(ECHO) "Board panel: http://localhost:$(SIM_PANEL_PORT) (log: $(TARGET_DIR_NAME)/simpanel.log)";\
	 $(SIM_DEBUG) $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro --panel $(SIM_PANEL_SOCKET) \
	 $(SIM_EXAMPLE_OPTS) $(SIM_OPTS);\
	 st=$$?; kill $$pid 2>/dev/null; exit $$st

.PHONY : run-sim
run-sim : build-sim
ifeq ($(PANEL),1)
	@$(PYTHON3) tools/simpanel.py --exit-with-parent --socket $(SIM_PANEL_SOCKET) --port $(SIM_PANEL_PORT)\
	 $(PANEL_OPTS) >$(TARGET_DIR_NAME)/simpanel.log 2>&1 & pid=$$!;\
	 sleep 0.5; if ! kill -0 $$pid 2>/dev/null; then $(CAT) $(TARGET_DIR_NAME)/simpanel.log; exit 1; fi;\
	 $(ECHO) "Board panel: http://localhost:$(SIM_PANEL_PORT) (log: $(TARGET_DIR_NAME)/simpanel.log)";\
	 $(SIM_DEBUG) $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro --panel $(SIM_PANEL_SOCKET) $(SIM_OPTS);\
	 st=$$?; kill $$pid 2>/dev/null; exit $$st
else
	@$(SIM_DEBUG) $(TARGET_DIR_NAME)/dnx-sim --disk 0=$(TARGET_DIR_NAME)/$(SIM_ROMFS_CONFIG).bin:ro $(SIM_OPTS)
endif

.PHONY : config-sim-kernel
config-sim-kernel :
	@$(call config_kernel, $(SIM_KERNEL_CONFIG))

.PHONY : config-sim-appset
config-sim-appset :
	@$(call config_appset, $(SIM_APPSET_CONFIG))

.PHONY : config-sim-romfs
config-sim-romfs :
	@$(call config_romfs, $(SIM_ROMFS_CONFIG))
