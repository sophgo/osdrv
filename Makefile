SHELL=/bin/bash
-include $(BUILD_PATH)/.config
#
export CVIARCH_L := $(shell echo $(CVIARCH) | tr A-Z a-z)
#
export CHIP_ARCH_L := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
INTERDRV_PATH := interdrv

ifeq ($(KERNEL_DIR), )
$(info Please set KERNEL_DIR global variable!!)
endif

ifeq ($(INSTALL_DIR), )
INSTALL_DIR = ko
endif
CUR_DIR = $(PWD)

$(info ** [ KERNEL_DIR ] ** = $(KERNEL_DIR))
$(info ** [ INSTALL_DIR ] ** = $(INSTALL_DIR))

export INTRERDRV_FLAGS :=
ifeq ($(CONFIG_BUILD_FOR_DEBUG), y)
INTRERDRV_FLAGS += -DDRV_DEBUG -DDRV_TEST -DIPCM_INFO_REC
endif

define MAKE_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) all -j$(shell nproc))
	if ls $(1)/*.ko &> /dev/null; then \
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

SUBDIRS = $(shell find ./interdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")
SUBDIRS += $(shell find ./extdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")
exclude_dirs = ./interdrv/include
SUBDIRS := $(filter-out $(exclude_dirs), $(SUBDIRS))

# prepare ko list
KO_LIST = pwm rtc wdt tpu mon clock_cooling saradc wiegand wiegand-gpio ipcm keyscan irrx

ifneq ($(CONFIG_USB_OSDRV_CVITEK_GADGET),)
KO_LIST += usb
endif

ifeq ($(CVIARCH), $(filter $(CVIARCH), CV181X))
	KO_LIST += sys ive trng
	BASE_DEP = sys
else ifeq ($(CVIARCH), $(filter $(CVIARCH), CV180X))
	KO_LIST += sys
	BASE_DEP = sys
endif

ifeq (, ${CONFIG_NO_FB})
	KO_LIST += fb
endif
ifeq (, ${CONFIG_NO_TP})
	KO_LIST += tp
endif

$(info ** [ KO_LIST ] ** = $(KO_LIST))

$(info ** [ CVIARCH ] ** = $(CVIARCH))

OTHERS :=

ifeq (y, ${CONFIG_CP_EXT_WIRELESS})
KO_LIST += wireless
OTHERS += cp_ext_wireless
endif

export CROSS_COMPILE=$(patsubst "%",%,$(CONFIG_CROSS_COMPILE_KERNEL))
export ARCH=$(patsubst "%",%,$(CONFIG_ARCH))

.PHONY : prepare clean all
all: prepare $(KO_LIST) $(OTHERS)

prepare:
	@mkdir -p $(INSTALL_DIR)/3rd

# osdrv/interdrv
fb: $(FB_DEP)
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

base: $(BASE_DEP)
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

usb:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

fast_image: rtos_cmdqu
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

pwm:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wdt:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

trng:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rtc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

tpu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

mon:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

clock_cooling:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

saradc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wiegand:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

snsr_i2c: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

sys:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ive:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rtos_cmdqu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ipcm:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

keyscan:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

irrx:
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
	@for subdir in $(SUBDIRS); do cd $$subdir && $(MAKE) clean && cd $(CUR_DIR); done
	@rm -f  $(INSTALL_DIR)/*.ko
	@rm -f  $(INSTALL_DIR)/3rd/*.ko
