include $(PWD)/../Makefile.interdrv.param
include $(PWD)/driver/driver.mk
include $(PWD)/platform/linux/platform.mk
include $(PWD)/chip/$(CHIP_ARCH_L)/chip.mk

$(info PWD = $(PWD))

obj-m += $(CHIP_ARCH_L)_rgn.o

EXTRA_CFLAGS += -I$(PWD)/../../../linux_5.10/drivers/soc/cvitek/rtos_cmdqu

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../osal/Module.symvers
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
