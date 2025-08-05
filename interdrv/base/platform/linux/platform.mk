$(CHIP_ARCH_L)_base-objs += platform/linux/base.o \
				platform/linux/ion.o \
				platform/linux/reg.o \
				platform/linux/proc/log_proc.o \
				platform/linux/proc/sys_proc.o \
				platform/linux/proc/vb_proc.o

ccflags-y += -I$(PWD)/platform/linux
ccflags-y += -I$(PWD)/platform/linux/proc
