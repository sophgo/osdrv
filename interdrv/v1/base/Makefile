arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

$(arch_cvitek_chip)_base-objs = base.o \
				$(arch_cvitek_chip)/vip_sys.o \
				reg.o \
				cvi_vb_proc.o \
				cvi_log_proc.o \
				cvi_sys_proc.o

obj-m += $(arch_cvitek_chip)_base.o

ccflags-y += -I$(PWD)/ -I$(PWD)/../vip/chip/$(arch_cvitek_chip)/ -I$(srctree)/drivers/tee

all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
