PWD := $(shell pwd)

#ARCH ?= arm
#CROSS_COMPILE ?= arm-linux-gnueabihf-
#KERNEL_DIR ?= ../../../out/prepare/build_sdk_kernel


#ccflags-y += -I./include/
#ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(src) -I$(src)/$(CHIP) -I$(srctree)/drivers/tee

obj-m := gt9xx.o
#gsl_point_id.o

#obj-m += cv_tp.o
#	cp gsl_point_id  gsl_point_id.o

#$(info CONFIG_TOUCHSCREEN_TSGSL = $(CONFIG_TOUCHSCREEN_TSGSL))

all:modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

clean:
	rm -f *.o* *.ko
	rm -f Module.symvers modules.order
	rm -f cvi_wiegand.mod.c
	rm -rf .tmp_versions
	rm -f .*cmd
	rm -f $(CHIP)/*.o*
	rm -f $(CHIP)/.*cmd


