SHELL=/bin/bash
-include $(BUILD_PATH)/.config

export CHIP_ARCH_L := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
INTERDRV_PATH := interdrv

KO_RLS_DIR :=
ifeq ($(DUAL_OS),y)
KO_RLS_DIR = ko_$(SDK_VER)_dual
else
KO_RLS_DIR = ko_$(SDK_VER)_single
endif
ifeq ($(wildcard $(INTERDRV_PATH)/$(KO_RLS_DIR)),)
$(error $(INTERDRV_PATH)/$(KO_RLS_DIR) not exist!)
else
$(shell rm -rf $(INTERDRV_PATH)/ko && cd $(INTERDRV_PATH) && cp -rlf $(KO_RLS_DIR) ko)
endif

ifeq ($(KERNEL_DIR), )
$(error Please set KERNEL_DIR global variable!!)
endif

ifeq ($(INSTALL_DIR), )
$(error Please set INSTALL_DIR global variable!!)
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
	if [ $$(find $(1) -name '*.ko' | wc -l) -gt 0 ]; then \
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

SUBDIRS = $(shell find ./interdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git" | grep -v "include" | grep -v "ko")
SUBDIRS += $(shell find ./extdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")


# prepare ko list
KO_LIST :=
ifeq ($(CONFIG_DUAL_OS), y)
KO_LIST = ipcm
endif

OTHERS :=

ifeq (, ${CONFIG_NO_TP})
KO_LIST += tp
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
	@cp -f $(INTERDRV_PATH)/ko/*.ko $(INSTALL_DIR)

# osdrv/interdrv
ipcm:
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

clean:
	@for subdir in $(SUBDIRS); do echo $$subdir; cd $$subdir && $(MAKE) OS_TYPE=LINUX clean && cd $(CUR_DIR); done
	@rm -f $(INSTALL_DIR)/*.ko
	@rm -f $(INSTALL_DIR)/3rd/*.ko

