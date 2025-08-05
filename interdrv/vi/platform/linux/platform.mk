ccflags-y += -I$(PWD)/platform/linux

$(CHIP_ARCH_L)_vi-objs += platform/linux/vi_core.o
$(CHIP_ARCH_L)_vi-objs += platform/linux/proc/vi_proc.o
$(CHIP_ARCH_L)_vi-objs += platform/linux/proc/vi_dbg_proc.o
$(CHIP_ARCH_L)_vi-objs += platform/linux/vi_misc.o
