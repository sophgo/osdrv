include $(PWD)/../Makefile.interdrv.param

obj-m += cvi_fb.o
cvi_fb-y += cvifb.o \
			scaler.o \
			reg.o

ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(PWD)/../include/common/uapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	rm -rf .tmp_versions
