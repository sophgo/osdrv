CHIP_ID := $(patsubst "%",%,$(CHIP_ARCH_L))

ifneq ($(KERNELRELEASE),)

obj-m += $(CHIP_ID)_ive.o
$(CHIP_ID)_ive-y += common/cvi_ive_interface.o
$(CHIP_ID)_ive-y += hal/$(CHIP_ID)/cvi_reg.o
$(CHIP_ID)_ive-y += hal/$(CHIP_ID)/cvi_ive_platform.o

ccflags-y += -I$(src)/../include/uapi -I$(src)/common
ccflags-y += -I$(src)/hal/$(CHIP_ID) -I$(src)/hal/$(CHIP_ID)/builder
#ccflags-y += -I$(src)/hal/$(CHIP_ID)/dma -I$(src)/hal/$(CHIP_ID)/dpu_common  -I$(src)/hal/$(CHIP_ID)/dpu_register -I$(src)/hal/$(CHIP_ID)/filter -I$(src)/hal/$(CHIP_ID)/reg -I$(src)/hal/$(CHIP_ID)/top

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
