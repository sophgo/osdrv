include $(PWD)/../Makefile.interdrv.param
include $(PWD)/driver/driver.mk

ifneq ($(KERNELRELEASE),)
$(CHIP_ARCH_L)_snsr_i2c-objs += platform/linux/snsr_i2c.o

obj-m += $(CHIP_ARCH_L)_snsr_i2c.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/ -I$(PWD)/common/

ccflags-y += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../osal/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers

else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
