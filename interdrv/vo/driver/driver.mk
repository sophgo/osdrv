ccflags-y += -I$(PWD)/driver/
$(CHIP_ARCH_L)_vo-objs += driver/vo_sdk_layer.o \
			  driver/vo_process.o \
