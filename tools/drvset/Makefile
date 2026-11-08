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
# Drvset build base: builds the drvset partition image (drivers built separately
# from the kernel) for the given kernel configuration:
#
#   make -C tools/drvset all DRVSET_DIR=<drvset> [CONFIG_DIR=<kernel config>] [BUILD_DIR=<build>]
#
#   DRVSET_DIR      directory with Makefile.in of drivers (see README.md)
#   CONFIG_DIR      kernel configuration used to build the kernel image
#                   (default: config/app-kernel), drvset shall be enabled
#   BUILD_DIR       build directory (default: build/<drvset name>)
#   KERNEL_VECTORS  kernel vector table address (default: origin of kernel
#                   flash region of <config>/custom_linker_script.ld)
#   CFLAGS_DRVSET   additional compiler flags of drivers (also from Makefile.in)
#   KERNEL_ELF      kernel image (ELF) used by the check target
#
# Relative paths are relative to the workspace directory.
#
####################################################################################################

# workspace and base paths
BASE_DIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WORKSPACE   := $(abspath $(BASE_DIR)/../..)
KERNEL      := $(WORKSPACE)/kernel
KERNEL_SRC  := $(KERNEL)/src

# path relative to workspace (or absolute)
ws_path = $(abspath $(if $(filter /%,$(1)),$(1),$(WORKSPACE)/$(1)))

ifeq ($(CONFIG_DIR),)
override CONFIG_DIR := $(WORKSPACE)/config/app-kernel
else
override CONFIG_DIR := $(call ws_path,$(CONFIG_DIR))
endif

ifneq ($(DRVSET_DIR),)
override DRVSET_DIR := $(call ws_path,$(DRVSET_DIR))
endif

ifeq ($(BUILD_DIR),)
override BUILD_DIR := $(WORKSPACE)/build/$(if $(DRVSET_DIR),$(notdir $(DRVSET_DIR)),drvset)
else
override BUILD_DIR := $(call ws_path,$(BUILD_DIR))
endif

CONFIG = $(CONFIG_DIR)/config.h
include $(CONFIG)

####################################################################################################
# PROJECT CONFIGURATION
####################################################################################################
PROJECT    = drvset
TARGET     = $(__CPU_ARCH__)
TOOLCHAIN  = $(__PROJECT_TOOLCHAIN__)
CC_WRAPPER =

GEN_DIR     = $(BUILD_DIR)/gen
OBJ_PATH    = $(BUILD_DIR)/$(TARGET)/obj
TARGET_PATH = $(BUILD_DIR)/output

# main RAM and flash regions of kernel linker script (kernel/src/cpu/<arch>/ld/common.ld)
ifeq ($(TARGET), stm32h7)
KERNEL_ROM_REGION = flash
KERNEL_RAM_REGION = axisram
else ifeq ($(TARGET), stm32f7)
KERNEL_ROM_REGION = rom
KERNEL_RAM_REGION = ram1
else
KERNEL_ROM_REGION = rom
KERNEL_RAM_REGION = ram
endif

KERNEL_LD = $(CONFIG_DIR)/custom_linker_script.ld

# drivers of drvset (DRVSET_ROOT: drvset directory with trailing slash)
DRVSET_ROOT = $(DRVSET_DIR)/
-include $(DRVSET_DIR)/Makefile.in

#---------------------------------------------------------------------------------------------------
# PROGRAMS
#---------------------------------------------------------------------------------------------------
SHELL   = /bin/bash
ECHO    = /bin/echo -e
RM      = /bin/rm -f
MKDIR   = /bin/mkdir -p
COPY    = /bin/cp
AWK     = awk
DATE    = /bin/date
CC      = $(CC_WRAPPER) $(TOOLCHAIN)gcc
AS      = $(TOOLCHAIN)gcc -x assembler-with-cpp
LD      = $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
SIZE    = $(TOOLCHAIN)size
SYNCDIR = python3 $(KERNEL)/tools/syncdir.py

#---------------------------------------------------------------------------------------------------
# FLAGS
#---------------------------------------------------------------------------------------------------
# kernel header search paths (public headers only, internal headers raise #error in modules),
# config.h of headers is the configuration of kernel (CONFIG_DIR is searched first), directories
# of kernel drivers as in the kernel build (driver interfaces and ioctl headers, e.g.
# "stm32h7/dma_ddi.h", "irq_ddi.h", "gpio_ddi.h")
include $(KERNEL_SRC)/cpu/Makefile
HDRLOC_KERNEL = $(KERNEL_SRC) $(KERNEL_SRC)/include $(KERNEL_SRC)/include/lib \
                $(KERNEL_SRC)/include/kernel $(KERNEL_SRC)/cpu $(addprefix $(KERNEL_SRC)/,$(HDRLOC_ARCH)) \
                $(KERNEL_SRC)/drivers $(patsubst %/,%,$(wildcard $(KERNEL_SRC)/drivers/*/))

HDRLOC     = $(CONFIG_DIR) $(GEN_DIR) $(HDRLOC_DRVSET) $(HDRLOC_KERNEL)
SEARCHPATH = $(foreach var,$(HDRLOC),-I$(var))

CFLAGS   = -c -MMD -MP \
           -g \
           -ggdb3 \
           -Os \
           -std=c18 \
           -ffunction-sections \
           -fdata-sections \
           -fno-common \
           -Wall \
           -Wextra \
           -Wparentheses \
           -nostdlib \
           $(ARCH_CFLAGS) \
           -Werror=implicit-function-declaration \
           -Werror=int-conversion \
           -Werror=incompatible-pointer-types \
           -Werror=return-type \
           -Werror=implicit-int \
           -Werror=discarded-qualifiers \
           -Wno-enum-conversion \
           -D__DNX_MODULE__ \
           -D__DNX_DRVSET__ \
           -include $(CONFIG) \
           -ffile-prefix-map=$(WORKSPACE)/= \
           $(CPUCONFIG_CFLAGS) \
           $(CFLAGS_DRVSET)

AFLAGS   = -c -MMD -MP -g -D__DNX_DRVSET__ $(CPUCONFIG_AFLAGS)

# architecture flags: ARM microcontrollers or POSIX simulator (host, i386 or x86_64)
ifeq ($(TARGET), posix)
ARCH_CFLAGS = -fno-pic -fno-asynchronous-unwind-tables -fno-stack-protector
ARCH_LFLAGS = $(filter -m32,$(CPUCONFIG_CFLAGS)) -static -no-pie -Wl,--build-id=none
else
ARCH_CFLAGS = -mno-unaligned-access
ARCH_LFLAGS = $(filter -mcpu=% -mthumb -mfloat-abi=% -mfpu=%,$(CPUCONFIG_CFLAGS))
endif

# CPUCONFIG_LDFLAGS of kernel configuration contain kernel linker script, only CPU flags are used
LFLAGS   = -g \
           $(ARCH_LFLAGS) \
           -nostdlib \
           -nostartfiles \
           -Wl,--gc-sections \
           -Wl,--no-undefined \
           -Wl,-L,$(GEN_DIR) \
           -Wl,-T,$(KERNEL_SRC)/drvset/drvset.ld \
           -Wl,-Map=$(TARGET_PATH)/$(PROJECT).map,--cref \
           -Wl,--print-memory-usage

LIBS     = -lgcc

#---------------------------------------------------------------------------------------------------
# SOURCES
#---------------------------------------------------------------------------------------------------
CSRC = $(CSRC_DRVSET) $(KERNEL_SRC)/drvset/drvset_header.c $(KERNEL_SRC)/drvset/drvset_fs.c
ASRC = $(KERNEL_SRC)/drvset/ksymstub.S

obj_of = $(foreach f,$(1),$(if $(filter $(WORKSPACE)/%,$(f)),$(OBJ_PATH)/$(patsubst $(WORKSPACE)/%,%,$(f)),$(OBJ_PATH)/_ext$(f)))

OBJECTS = $(call obj_of,$(CSRC:.c=.o)) $(call obj_of,$(ASRC:.S=.o))

# value of configuration flag (C preprocessor)
cfgval = $(shell echo '$(1)' | $(CC) -E -P -x c -include $(CONFIG) -I$(CONFIG_DIR) - 2>/dev/null | tail -n 1)

# origin of kernel memory region: $(1) region name
region_origin = $(shell $(AWK) -v region=$(1) -f $(BASE_DIR)/ldregion.awk $(KERNEL_LD))

.DELETE_ON_ERROR:

####################################################################################################
# targets
####################################################################################################
.PHONY : all
all : check_drvset
	@$(MAKE) -s -f$(firstword $(MAKEFILE_LIST)) generate
	@$(MAKE) -s -f$(firstword $(MAKEFILE_LIST)) build_start

.PHONY : build_start
build_start : buildobjects linkobjects hex ioctl status

.PHONY : help
help :
	@$(ECHO) "This is help for the drvset build base"
	@$(ECHO) "Usage: make -C tools/drvset <target> DRVSET_DIR=<drvset> [CONFIG_DIR=<kernel config>] [BUILD_DIR=<build>]"
	@$(ECHO) "Possible targets:"
	@$(ECHO) "   help                this help"
	@$(ECHO) "   all                 build drvset partition image"
	@$(ECHO) "   clean               clean build directory"
	@$(ECHO) "   check               check image against kernel image (KERNEL_ELF=<kernel.elf>)"

.PHONY : check_drvset
check_drvset :
ifeq ($(DRVSET_DIR),)
	$(error DRVSET_DIR is not defined (directory with Makefile.in of drivers))
endif
	@if [ ! -f "$(DRVSET_DIR)/Makefile.in" ]; then $(ECHO) "Drvset Makefile.in not found: $(DRVSET_DIR)/Makefile.in"; exit 1; fi
	@if [ "$(call cfgval,__OS_ENABLE_DRVSET__)" != "1" ]; then $(ECHO) "drvset is disabled in kernel configuration: $(CONFIG_DIR)"; exit 1; fi
	@if [ -z "$(DRVSET_MODULES)$(DRVSET_FILESYSTEMS)" ]; then \
	    $(ECHO) "No modules or file systems defined (DRVSET_MODULES, DRVSET_FILESYSTEMS of $(DRVSET_DIR)/Makefile.in)"; exit 1; fi

.PHONY : clean
clean :
	@$(ECHO) "Cleaning up drvset build..."
	@$(RM) -r $(BUILD_DIR)

####################################################################################################
# generated files: module list, interrupt list and memory layout
####################################################################################################
FLASH_ADDR    = $(call cfgval,__OS_DRVSET_FLASH_ADDRESS__)
MAX_MODULES   = $(call cfgval,__OS_DRVSET_MAX_MODULES__)
FLASH_SIZE    = $(call cfgval,__OS_DRVSET_FLASH_SIZE__)
RAM_SIZE      = $(call cfgval,__OS_DRVSET_RAM_SIZE__)
KSYMTAB_IDX   = $(shell echo '_KVECTOR_KSYMTAB' | $(CC) -E -P -x c -include $(KERNEL_SRC)/include/kernel/kvectors.h - 2>/dev/null | tail -n 1)
RAM_ORIGIN    = $(call region_origin,$(KERNEL_RAM_REGION))
KERNEL_VECTORS ?= $(call region_origin,$(KERNEL_ROM_REGION))

# module entry: NAME (character device) or NAME:blk (block device)
mod_name = $(word 1,$(subst :, ,$(1)))
mod_type = $(if $(filter blk,$(word 2,$(subst :, ,$(1)))),S_IFBLK,S_IFCHR)

# file system entry: name (mount name and API_FS_*() name) or name:prefix (API_FS_*() name of source)
fs_name   = $(word 1,$(subst :, ,$(1)))
fs_prefix = $(or $(word 2,$(subst :, ,$(1))),$(word 1,$(subst :, ,$(1))))

# ioctl headers of kernel drivers (drivers/ioctl_requests.h used by fs/fs.h, as fs/addfs.sh of kernel)
IOCTL_KERNEL = $(sort $(foreach d,$(wildcard $(KERNEL_SRC)/drivers/*/),$(wildcard $(d)$(notdir $(patsubst %/,%,$(d)))_ioctl.h)))

.PHONY : generate
generate :
	@$(MKDIR) $(GEN_DIR)-tmp $(TARGET_PATH)
	@$(ECHO) "Generating module list..."
	@$(ECHO) "/* Generated by drvset build, do not edit. */" > $(GEN_DIR)-tmp/drvset_modules.h
	@$(ECHO) "#define DRVSET_MODULE_LIST(X) \\" >> $(GEN_DIR)-tmp/drvset_modules.h
	@$(foreach m,$(DRVSET_MODULES),$(ECHO) "        X($(call mod_name,$(m)), $(call mod_type,$(m))) \\" >> $(GEN_DIR)-tmp/drvset_modules.h;)
	@$(ECHO) "" >> $(GEN_DIR)-tmp/drvset_modules.h
	@$(ECHO) "Generating file system list..."
	@$(ECHO) "/* Generated by drvset build, do not edit. */" > $(GEN_DIR)-tmp/drvset_fs.h
	@$(ECHO) "#define DRVSET_FS_LIST(X) \\" >> $(GEN_DIR)-tmp/drvset_fs.h
	@$(foreach f,$(DRVSET_FILESYSTEMS),$(ECHO) "        X($(call fs_name,$(f)), $(call fs_prefix,$(f))) \\" >> $(GEN_DIR)-tmp/drvset_fs.h;)
	@$(ECHO) "" >> $(GEN_DIR)-tmp/drvset_fs.h
	@$(ECHO) "#define DRVSET_FS_COUNT $(words $(DRVSET_FILESYSTEMS))" >> $(GEN_DIR)-tmp/drvset_fs.h
	@$(MKDIR) $(GEN_DIR)-tmp/drivers
	@$(ECHO) "/* Generated by drvset build, do not edit. */" > $(GEN_DIR)-tmp/drivers/ioctl_requests.h
	@$(foreach h,$(IOCTL_KERNEL),$(ECHO) "#include \"$(notdir $(h))\"" >> $(GEN_DIR)-tmp/drivers/ioctl_requests.h;)
	@$(ECHO) "Generating interrupt list..."
ifeq ($(TARGET), posix)
	@$(ECHO) "/* POSIX simulator does not emulate interrupts. */\n#define DRVSET_IRQ_COUNT 0\n#define DRVSET_IRQ_LIST(X)" \
	       > $(GEN_DIR)-tmp/drvset_irq.h
else
	@$(CC) -E -P -include $(CONFIG) -I$(CONFIG_DIR) $(SEARCHPATH) $(CPUCONFIG_CFLAGS) $(KERNEL_SRC)/cpu/$(TARGET)/*_vectors.c \
	       | $(AWK) -f $(KERNEL_SRC)/drvset/mkirqtab.awk > $(GEN_DIR)-tmp/drvset_irq.h
endif
	@$(ECHO) "Generating memory layout..."
	@$(ECHO) "/* Generated by drvset build from $(patsubst $(WORKSPACE)/%,%,$(CONFIG_DIR)), do not edit. */" > $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "MEMORY" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "{" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "   drvset_flash (rx)  : ORIGIN = $(FLASH_ADDR), LENGTH = $(FLASH_SIZE) * 1024" >> $(GEN_DIR)-tmp/drvset_memory.ld
ifeq ($(TARGET), posix)
	@# simulator: RAM mapped after flash, first 8 bytes: pointer of kernel symbol table (kernel drivers/drvset.c)
	@$(ECHO) "   drvset_ram   (rwx) : ORIGIN = $(FLASH_ADDR) + $(FLASH_SIZE) * 1024 + 8, LENGTH = $(RAM_SIZE) * 1024 - 8" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "}" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "__ksymtab_vector = $(FLASH_ADDR) + $(FLASH_SIZE) * 1024;" >> $(GEN_DIR)-tmp/drvset_memory.ld
else
	@if [ -z "$(RAM_ORIGIN)" ] || [ -z "$(KERNEL_VECTORS)" ]; then \
	        $(ECHO) "Kernel memory regions $(KERNEL_ROM_REGION)/$(KERNEL_RAM_REGION) not found in $(KERNEL_LD)"; exit 1; fi
	@$(ECHO) "   drvset_ram   (rwx) : ORIGIN = $(RAM_ORIGIN), LENGTH = $(RAM_SIZE) * 1024" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "}" >> $(GEN_DIR)-tmp/drvset_memory.ld
	@$(ECHO) "__ksymtab_vector = $(KERNEL_VECTORS) + 4 * $(KSYMTAB_IDX);" >> $(GEN_DIR)-tmp/drvset_memory.ld
endif
	@$(SYNCDIR) $(GEN_DIR)-tmp/ $(GEN_DIR)/

####################################################################################################
# build and link
####################################################################################################
.PHONY : buildobjects
buildobjects : $(OBJECTS)

.PHONY : linkobjects
linkobjects : $(OBJECTS)
	@$(ECHO) "Linking..."
	@$(LD) $(OBJECTS) $(LFLAGS) $(LIBS) -o $(TARGET_PATH)/$(PROJECT).elf

.PHONY : hex
hex :
	@$(ECHO) "Creating IHEX image..."
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -O ihex $(TARGET_PATH)/$(PROJECT).hex
	@$(ECHO) "Creating binary image..."
	@$(OBJCOPY) $(TARGET_PATH)/$(PROJECT).elf -O binary $(TARGET_PATH)/$(PROJECT).bin

# ioctl headers of drivers (used by programs)
.PHONY : ioctl
ioctl :
	@$(MKDIR) $(TARGET_PATH)/ioctl
	@$(if $(IOCTL_DRVSET),$(COPY) $(IOCTL_DRVSET) $(TARGET_PATH)/ioctl/)

# offline check of image against kernel image (the same checks as kernel at start and binary
# interface of kernel functions)
.PHONY : check
check :
ifeq ($(KERNEL_ELF),)
	$(error KERNEL_ELF is not defined (kernel image, e.g. KERNEL_ELF=build/app-kernel.elf))
endif
	@python3 $(BASE_DIR)/drvset_check.py $(call ws_path,$(KERNEL_ELF)) $(TARGET_PATH)/$(PROJECT).elf \
	         --flash $(FLASH_ADDR) --size $$(( $(FLASH_SIZE) * 1024 )) --ram $$(( $(RAM_SIZE) * 1024 )) \
	         --max-modules $(MAX_MODULES) $(CHECK_OPTS)

.PHONY : status
status :
	@$(SIZE) $(TARGET_PATH)/$(PROJECT).elf
	@$(ECHO) "Image: $(patsubst $(WORKSPACE)/%,%,$(TARGET_PATH))/$(PROJECT).{elf,hex,bin} at $(FLASH_ADDR)"
	@$(ECHO) "-----------------------------------"
	@$(ECHO) "| `$(DATE) "+Compilation completed: %H:%M:%S"` |"
	@$(ECHO) "-----------------------------------"

define compile_rules
$(OBJ_PATH)/$(1)%.o : $(2)%.c
	@$$(ECHO) "Compiling: $$(patsubst $(WORKSPACE)/%,%,$$<)"
	@$$(MKDIR) $$(dir $$@)
	@$$(CC) $$(CFLAGS) $$(SEARCHPATH) $$< -o $$@

$(OBJ_PATH)/$(1)%.o : $(2)%.S
	@$$(ECHO) "Assembling: $$(patsubst $(WORKSPACE)/%,%,$$<)"
	@$$(MKDIR) $$(dir $$@)
	@$$(AS) $$(AFLAGS) $$(SEARCHPATH) $$< -o $$@
endef

$(eval $(call compile_rules,,$(WORKSPACE)/))
$(eval $(call compile_rules,_ext/,/))

-include $(OBJECTS:.o=.d)
