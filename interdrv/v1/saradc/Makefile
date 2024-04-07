arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

SARADC_PATH = $(arch_cvitek_chip)


$(arch_cvitek_chip)_saradc-objs = $(SARADC_PATH)/$(arch_cvitek_chip)_saradc.o


obj-m += $(arch_cvitek_chip)_saradc.o

ccflags-y += -I$(PWD)/

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
