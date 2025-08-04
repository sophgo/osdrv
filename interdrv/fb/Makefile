include $(PWD)/../Makefile.interdrv.param

obj-m += cvi_fb.o
cvi_fb-y += common/cvifb.o \
			common/scaler.o \
			common/reg.o

ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(PWD)/../include/common/uapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod common/*.o
	rm -rf .tmp_versions
