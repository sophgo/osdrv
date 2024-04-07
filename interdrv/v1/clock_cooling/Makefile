PWD := $(shell pwd)

arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

ccflags-y += -I$(src) -I$(src)/$(arch_cvitek_chip)

$(arch_cvitek_chip)_clock_cooling-objs = $(arch_cvitek_chip)/clock_cooling.o

obj-m += $(arch_cvitek_chip)_clock_cooling.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
