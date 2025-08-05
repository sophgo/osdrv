CONFIG_REG_DUMP = 0

$(CHIP_ARCH_L)_vpss-objs += chip/$(CHIP_ARCH_L)/scaler.o
$(CHIP_ARCH_L)_vpss-objs += chip/$(CHIP_ARCH_L)/vpss_ip_ctrl.o
$(CHIP_ARCH_L)_vpss-objs += chip/$(CHIP_ARCH_L)/vpss_hal.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/

ifeq ($(CONFIG_REG_DUMP), 1)
ccflags-y += -DCONFIG_REG_DUMP
endif