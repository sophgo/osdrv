include $(PWD)/../Makefile.interdrv.param

ccflags-y += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CVIARCH_L)/chip.mk
$(CVIARCH_L)_hdmi-y += common/hdmi_ioctl.o
$(CVIARCH_L)_hdmi-y += common/hdmi_proc.o

CROSS_COMPILE=aarch64-linux-gnu
obj-m += $(CVIARCH_L)_hdmi.o
#ccflags-y += -DPORTING_TEST
ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/src/  -I$(PWD)/
ccflags-y += -I$(PWD)/common/
ccflags-y += -I$(PWD)/../vo/chip/soph/
ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/ -I$(PWD)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(PWD)/../include/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)
ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

# ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vo/Module.symvers

else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
