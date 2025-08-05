include $(PWD)/../Makefile.interdrv.param
include $(PWD)/chip/$(CHIP_ARCH_L)/chip.mk
include $(PWD)/driver/driver.mk
include $(PWD)/platform/linux/platform.mk

obj-m += $(CHIP_ARCH_L)_vpss.o

KBUILD_EXTRA_SYMBOLS += $(PWD)/../osal/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
