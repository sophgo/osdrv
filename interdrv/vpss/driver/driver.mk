ccflags-y += -I$(PWD)/driver/

$(CHIP_ARCH_L)_vpss-objs += driver/vpss.o
$(CHIP_ARCH_L)_vpss-objs += driver/vpss_core.o
$(CHIP_ARCH_L)_vpss-objs += driver/vpss_sdk_layer.o
$(CHIP_ARCH_L)_vpss-objs += driver/vpss_rgn_ctrl.o