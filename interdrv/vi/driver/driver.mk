ccflags-y += -I$(PWD)/driver/

$(CHIP_ARCH_L)_vi-objs += driver/vi_ioctl.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_sdk_layer.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_perf_chk.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_isp_buf_ctrl.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_raw_dump.o
$(CHIP_ARCH_L)_vi-objs += driver/vi.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_ext.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_dump_register.o
$(CHIP_ARCH_L)_vi-objs += driver/vi_dma_setup.o