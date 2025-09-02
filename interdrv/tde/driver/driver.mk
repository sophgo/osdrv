ccflags-y += -I$(PWD)/driver/

$(CHIP_ARCH_L)_tde-objs += driver/tde_core.o
$(CHIP_ARCH_L)_tde-objs += driver/tde_sdk_layer.o
$(CHIP_ARCH_L)_tde-objs += driver/tde_inter_cb.o
