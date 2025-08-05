$(CHIP_ARCH_L)_sys-objs += chip/$(CHIP_ARCH_L)/sys_common.o
$(CHIP_ARCH_L)_sys-objs += chip/$(CHIP_ARCH_L)/vi_sys.o
$(CHIP_ARCH_L)_sys-objs += chip/$(CHIP_ARCH_L)/cmdq.o

ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/
