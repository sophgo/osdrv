ccflags-y += -I$(PWD)/driver/
$(CHIP_ARCH_L)_ldc-objs += driver/ldc_comm_layer.o
$(CHIP_ARCH_L)_ldc-objs += driver/mesh.o
$(CHIP_ARCH_L)_ldc-objs += driver/ldc_sdk.o