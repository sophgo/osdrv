ccflags-y += -I$(PWD)/platform/linux/

$(CHIP_ARCH_L)_tde-objs += platform/linux/tde_dev.o
$(CHIP_ARCH_L)_tde-objs += platform/linux/tde_ioctl.o
$(CHIP_ARCH_L)_tde-objs += platform/linux/tde_proc.o

