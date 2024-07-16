arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

obj-m += cvi_fb.o
cvi_fb-y += cvifb.o

#cvi_fb-objs := cvifb.o
#obj-$(CONFIG_FB_CVITEK) += cvi_fb.o
ccflags-y += -I$(PWD)/../vip/chip/$(arch_cvitek_chip)/
ccflags-y += -I$(srctree)/drivers/staging/android/
ccflags-y += -I$(PWD)/../base/

KBUILD_EXTRA_SYMBOLS = $(PWD)/../vip/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	rm -rf .tmp_versions
