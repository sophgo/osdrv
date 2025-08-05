$(CHIP_ARCH_L)_vo-objs += chip/$(CHIP_ARCH_L)/disp.o \
			  chip/$(CHIP_ARCH_L)/dsi_phy.o \
			  chip/$(CHIP_ARCH_L)/dsi_mac.o \
			  chip/$(CHIP_ARCH_L)/vo_mac.o \

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/
