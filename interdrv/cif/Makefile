include $(PWD)/../Makefile.interdrv.param
include $(PWD)/chip/$(CHIP_ARCH_L)/drv/drv.mk
include $(PWD)/driver/driver.mk
include $(PWD)/platform/linux/platform.mk

ifneq ($(KERNELRELEASE),)
# include $(PWD)/chip/$(CHIP_ARCH_L)/chip.mk

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/drv/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/

ifeq ($(SUBTYPE), fpga)
export FPGA_TESTING := true
ccflags-y += -DFPGA_TESTING -DDEBUG
endif

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

obj-m += $(CHIP_ARCH_L)_mipi_rx.o

KBUILD_EXTRA_SYMBOLS = $(PWD)/../osal/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
