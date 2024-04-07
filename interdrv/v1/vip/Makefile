arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(arch_cvitek_chip)/vip.mk

obj-$(CONFIG_VIDEO_CVITEK)         += $(arch_cvitek_chip)_vip.o
obj-$(CONFIG_VIDEO_CVITEK_SNS_I2C) += $(arch_cvitek_chip)_sensor_i2c.o
obj-$(CONFIG_VIDEO_CVITEK_CIF)     += cvi_mipi_rx.o
obj-$(CONFIG_VIDEO_CVITEK_MIPI_TX) += cvi_mipi_tx.o

ccflags-$(CONFIG_CVITEK_PINCTRL_CV1835) += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(PWD)/../vip/chip/$(arch_cvitek_chip)/ -I$(srctree)/drivers/tee

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
