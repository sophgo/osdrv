$(CHIP_ARCH_L)_rgn-objs += platform/linux/rgn_core.o \
				platform/linux/proc/rgn_proc.o

ccflags-y += -I$(PWD)/platform/linux/proc/ -I$(PWD)/platform/linux/