CONFIG_REG_DUMP = 0

$(CHIP_ARCH_L)_gfbg-objs += chip/$(CHIP_ARCH_L)/gfbg_disp.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/

ifeq ($(CONFIG_REG_DUMP), 1)
ccflags-y += -DCONFIG_REG_DUMP
endif