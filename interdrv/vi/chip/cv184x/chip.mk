ccflags-y += -I$(PWD)/chip/$(CHIP_ARCH_L)/inc/

$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_drv.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_subsys_ctrl.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_fe_ip_ctrl.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_raw_ip_ctrl.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_rgb_ip_ctrl.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_yuv_ip_ctrl.o
$(CHIP_ARCH_L)_vi-objs += chip/$(CHIP_ARCH_L)/vi_tun_ip_ctrl.o
