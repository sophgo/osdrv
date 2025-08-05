ccflags-y += -I$(PWD)/platform/linux/

$(CHIP_ARCH_L)_ldc-objs += platform/linux/ldc_core.o
$(CHIP_ARCH_L)_ldc-objs += platform/linux/ldc_proc.o

