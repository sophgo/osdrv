arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

WDT_PATH = $(arch_cvitek_chip)


$(arch_cvitek_chip)_wdt-objs = $(WDT_PATH)/$(arch_cvitek_chip)_wdt.o


obj-m += $(arch_cvitek_chip)_wdt.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
