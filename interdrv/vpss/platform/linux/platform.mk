ccflags-y += -I$(PWD)/platform/linux/

$(CHIP_ARCH_L)_vpss-objs += platform/linux/vpss_dev.o
$(CHIP_ARCH_L)_vpss-objs += platform/linux/vpss_ioctl.o
$(CHIP_ARCH_L)_vpss-objs += platform/linux/vpss_proc.o

