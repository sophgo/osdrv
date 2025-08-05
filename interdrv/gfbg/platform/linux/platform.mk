ccflags-y += -I$(PWD)/platform/linux/

$(CHIP_ARCH_L)_gfbg-objs += platform/linux/gfbg_hal.o \
			  platform/linux/gfbg_init.o \
			  platform/linux/gfbg_main.o \
			  platform/linux/proc/gfbg_proc.o \
			  platform/linux/gfbg_callback.o \
