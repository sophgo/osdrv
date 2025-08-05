ccflags-y += -I$(PWD)/platform/linux

$(CHIP_ARCH_L)_mipi_rx-objs += platform/linux/cif_l.o
$(CHIP_ARCH_L)_mipi_rx-objs += platform/linux/cif.o
$(CHIP_ARCH_L)_mipi_rx-objs += platform/linux/cif_ioctl.o