CONFIG_REG_DUMP = 0

$(CHIP_ARCH_L)_ldc-objs += chip/$(CHIP_ARCH_L)/ldc.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/

ifeq ($(CONFIG_REG_DUMP), 1)
ccflags-y += -DCONFIG_REG_DUMP
endif