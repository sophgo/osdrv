$(CHIP_ARCH_L)_tde-objs += chip/$(CHIP_ARCH_L)/tde_reg.o
$(CHIP_ARCH_L)_tde-objs += chip/$(CHIP_ARCH_L)/tde_ip_ctrl.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/