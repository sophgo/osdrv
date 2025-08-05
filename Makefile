SHELL=/bin/bash
-include $(BUILD_PATH)/.config

export CHIP_ARCH_L := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
INTERDRV_PATH := interdrv

ifeq ($(KERNEL_DIR), )
$(error Please set KERNEL_DIR global variable!!)
endif

ifeq ($(INSTALL_DIR), )
INSTALL_DIR = ko
endif

CUR_DIR = $(PWD)

$(info ** [ CHIP_ARCH_L ] ** = $(CHIP_ARCH_L))
$(info ** [ KERNEL_DIR ] ** = $(KERNEL_DIR))
$(info ** [ INSTALL_DIR ] ** = $(INSTALL_DIR))
$(info ** [ BUILD_DIR ] ** = $(BUILD_DIR))
$(info ** [ CONFIG_DUAL_OS ] ** = $(CONFIG_DUAL_OS))

export INTRERDRV_FLAGS :=
ifeq ($(CONFIG_BUILD_FOR_DEBUG), y)
INTRERDRV_FLAGS += -DDRV_DEBUG -DDRV_TEST
endif

define MAKE_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) CONFIG_DUAL_OS=$(CONFIG_DUAL_OS) all -j$(shell nproc))
	if [ $$(find -L $(1) -name '*.ko' | wc -l) -gt 0 ]; then \
		cd $(1) && cp -f *.ko $(INSTALL_DIR); \
	fi
endef

MAKE_EXT_KO_CP :=
ifneq (${FLASH_SIZE_SHRINK},y)
define MAKE_EXT_KO_CP
	find $(1) -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;;
endef
endif

define MAKE_EXT_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) all -j$(shell nproc))
	$(call MAKE_EXT_KO_CP, $(1))
endef

SUBDIRS = $(shell find ./interdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git" | grep -v "include")
SUBDIRS += $(shell find ./interdrv -maxdepth 1 -mindepth 1 -type l | grep -v "git" | grep -v "include")
SUBDIRS += $(shell find ./extdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")
exclude_dirs = ./interdrv/include ./interdrv/vc_drv
SUBDIRS := $(filter-out $(exclude_dirs), $(SUBDIRS))

MEDIA_INCLUDE_DIR = $(BUILD_DIR)/media/include

# prepare ko list
ifeq ($(CONFIG_DUAL_OS), y)
KO_LIST = osal base ipcm tde gfbg
else
KO_LIST = osal base sys cif snsr_i2c vi vpss vc_drv rgn ldc vo mipi_tx gfbg tde
endif

OTHERS :=

ifeq (, ${CONFIG_NO_TP})
KO_LIST += tp
OTHERS += cp_ext_tp
endif

ifeq (y, ${CONFIG_CP_EXT_WIRELESS})
KO_LIST += wireless
OTHERS += cp_ext_wireless
endif

$(info ** [ KO_LIST ] ** = $(KO_LIST))

export CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_CROSS_COMPILE_KERNEL))
export ARCH=$(patsubst "%",%,$(CONFIG_ARCH))

.PHONY : prepare clean all
all: prepare $(KO_LIST) $(OTHERS)

prepare:
	@mkdir -p $(INSTALL_DIR)/3rd
	@cp -f $(MEDIA_INCLUDE_DIR)/internal/osdrv_uapi/* $(INTERDRV_PATH)/include/common/uapi/
	@cp -f $(MEDIA_INCLUDE_DIR)/internal/comm/* $(INTERDRV_PATH)/include/common/uapi/
	@cp -f $(MEDIA_INCLUDE_DIR)/release/cvi_defines.h $(INTERDRV_PATH)/include/chip/$(CHIP_ARCH_L)/uapi/defines.h

# osdrv/interdrv
osal:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

base: osal prepare
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

sys: osal base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ipcm:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

cif: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

snsr_i2c: sys
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vi: sys
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vpss: sys
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vc_drv: sys
	@cp -f ${INTERDRV_PATH}/${@}/${SDK_VER}/*.ko $(INSTALL_DIR)

rgn: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ldc: sys
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vo: sys
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

mipi_tx: vo
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

gfbg:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

tde: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

# osdrv/extdrv
tp:
	$(call MAKE_EXT_KO, extdrv/${@})

wireless:
	@$(call MAKE_EXT_KO, extdrv/${@})

wiegand-gpio:
	@$(call MAKE_EXT_KO, extdrv/${@})

gyro_i2c:
	@$(call MAKE_EXT_KO, extdrv/${@})

cp_ext_wireless:
	find extdrv/wireless -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;;

cp_ext_tp:
	find extdrv/tp -name '*.ko' -print -exec cp {} $(INSTALL_DIR)/3rd/ \;;

clean:
	@for subdir in $(SUBDIRS); do echo $$subdir; cd $$subdir && $(MAKE) OS_TYPE=LINUX clean && cd $(CUR_DIR); done
	@rm -f $(INTERDRV_PATH)/include/common/uapi/*.h
	@rm -f $(INTERDRV_PATH)/include/chip/$(CHIP_ARCH_L)/uapi/defines.h

