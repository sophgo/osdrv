SHELL=/bin/bash
-include $(BUILD_PATH)/.config
#
ccviarch := $(shell echo $(CVIARCH) | tr A-Z a-z)
ifeq ($(ccviarch), $(filter $(ccviarch), sophon bm1688))
export CVIARCH_L := soph
else
export CVIARCH_L := $(shell echo $(CVIARCH) | tr A-Z a-z)
endif
#
export CHIP_ARCH_L := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
INTERDRV_PATH := interdrv/$(shell echo $(MW_VER))

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
INTRERDRV_FLAGS += -DDRV_DEBUG -DDRV_TEST
endif

define MAKE_KO
	( cd $(1) && $(MAKE) KERNEL_DIR=$(KERNEL_DIR) all -j$(shell nproc))
    if [ -e $(1)/*.ko ]; then \
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

SUBDIRS = $(shell find ./interdrv -maxdepth 2 -mindepth 2 -type d | grep -v "git")
SUBDIRS += $(shell find ./extdrv -maxdepth 1 -mindepth 1 -type d | grep -v "git")
exclude_dirs = ./interdrv/v1/include ./interdrv/v2/include
SUBDIRS := $(filter-out $(exclude_dirs), $(SUBDIRS))


# prepare ko list

KO_LIST = base pwm  mon clock_cooling saradc keyscan irrx wiegand wiegand-gpio vc_drv rtc

# ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV183X CV182X))
# 	KO_LIST += vip
# 	FB_DEP = vip
# endif

# ifeq ($(CVIARCH), $(filter $(CVIARCH), CV181X CV186X))
# 	KO_LIST += sys vi snsr_i2c cif vpss ldc dwa rgn rtos_cmdqu fast_image audio ive 2d_engine
# 	BASE_DEP = sys
# 	FB_DEP = vpss
# else ifeq ($(CVIARCH), $(filter $(CVIARCH), CV180X))
# 	KO_LIST += sys vi snsr_i2c cif vpss dwa rgn rtos_cmdqu fast_image audio
# 	BASE_DEP = sys
# 	FB_DEP = vpss
# endif

ifeq ($(CVIARCH), $(filter $(CVIARCH), CV181X SOPHON))
	KO_LIST += sys vi snsr_i2c cif vpss ldc dwa vo mipi_tx rgn rtos_cmdqu ive 2d_engine dpu stitch
	BASE_DEP = sys
	FB_DEP = vpss vo
ifneq (${CONFIG_BOARD}, "fpga")
	KO_LIST += hdmi
endif

else ifeq ($(CVIARCH), $(filter $(CVIARCH), CV180X))
	KO_LIST += sys vi snsr_i2c cif vpss dwa rgn rtos_cmdqu audio
	FB_DEP = vpss
endif

ifeq (, ${CONFIG_NO_FB})
	KO_LIST += fb
endif
ifeq (, ${CONFIG_NO_TP})
	# KO_LIST += tp
endif

$(info ** [ KO_LIST ] ** = $(KO_LIST))

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
fb: base $(FB_DEP)
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

base:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

audio:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

# vcodec:
# 	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vpu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

#vpu_drv:
#	@$(call MAKE_KO, ${INTERDRV_PATH}/vc_drv/cnm_driver/vpu/vdi/linux/driver)

#jpu_drv:
#	@$(call MAKE_KO, ${INTERDRV_PATH}/vc_drv/cnm_driver/jpeg/jdi/linux/driver)

jpu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

usb:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

jpeg:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

pwm:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wdt:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rtc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vip: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

mon:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

clock_cooling:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

saradc:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

keyscan:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

irrx:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

wiegand:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vi: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

snsr_i2c: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

cif: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

sys:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vpss: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

ldc: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

dwa: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rgn: base vpss
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vo: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

mipi_tx: vo base
	@$(call MAKE_KO, ${INTERDRV_PATH}/vo/${@})

ive:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

# cvi_vc_drv: sys base vcodec jpeg
# 	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

vc_drv:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

rtos_cmdqu:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

2d_engine:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

dpu: base
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

hdmi:
	@$(call MAKE_KO, ${INTERDRV_PATH}/${@})

stitch: base
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
