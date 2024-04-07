PWD := $(shell pwd)

ID_NAME := $(patsubst "%",%,$(CHIP_ARCH_L))

ccflags-y += -I$(src)/../include/uapi -I$(src)/common -I$(src)/hal/$(ID_NAME) -I$(srctree)/drivers/tee -I$(srctree)/arch/arm64/include -I$(srctree)/drivers/staging/android/ion

obj-m += $(ID_NAME)_tpu.o
$(ID_NAME)_tpu-y += common/cvi_tpu_interface.o
$(ID_NAME)_tpu-y += hal/$(ID_NAME)/tpu_platform.o
$(ID_NAME)_tpu-y += hal/$(ID_NAME)/tpu_pmu.o

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
