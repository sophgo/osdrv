arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

TRNG_PATH = $(arch_cvitek_chip)


$(arch_cvitek_chip)_trng-objs = $(TRNG_PATH)/$(arch_cvitek_chip)_trng.o


obj-m += $(arch_cvitek_chip)_trng.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
