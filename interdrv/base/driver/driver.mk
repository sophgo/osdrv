$(CHIP_ARCH_L)_base-objs += driver/base_common.o \
				driver/bind.o \
				driver/callback.o \
				driver/vb.o \
				driver/vbq.o

ccflags-y += -I$(PWD)/driver/
