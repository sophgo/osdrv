ccflags-y += -I$(PWD)/platform/linux/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
$(CHIP_ARCH_L)_vo-objs += platform/linux/vo_drv.o \
			  platform/linux/vo_platform.o \
			  platform/linux/proc/vo_disp_proc.o \
			  platform/linux/proc/vo_proc.o \
			  platform/linux/gfbg_ctrl/gfbg_ctrl.o \
