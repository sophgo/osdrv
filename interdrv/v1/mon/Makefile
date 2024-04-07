PWD := $(shell pwd)

ID_NAME := $(patsubst "%",%,$(CHIP_ARCH_L))

ccflags-y += -I$(src)/common -I$(src)/hal/$(ID_NAME)

obj-m += $(ID_NAME)_mon.o
$(ID_NAME)_mon-y += common/cvi_mon_interface.o
$(ID_NAME)_mon-y += hal/$(ID_NAME)/mon_platform.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o* *.ko
	@rm -f Module.symvers modules.order
	@rm -f *.mod.c
	@rm -rf .tmp_versions
	@rm -f .*cmd
	@rm -f $(CHIP)/*.o*
	@rm -f $(CHIP)/.*cmd
	@find . -name \*.o* -type f -delete
