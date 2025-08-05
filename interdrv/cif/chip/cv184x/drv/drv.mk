ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/drv/inc/

$(CHIP_ARCH_L)_mipi_rx-objs +=chip/$(CHIP_ARCH_L)/drv/cif_drv.o