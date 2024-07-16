arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_core.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_disp.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_img.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_sc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_dwa.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_isp.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_isp_ext.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_isp_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_vi_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_vpss_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_vo_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_gdc_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/cvi_vip_rgn_proc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/reg.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/cmdq.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/scaler.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/dsi_phy.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/ldc.o
$(arch_cvitek_chip)_vip-objs += chip/$(arch_cvitek_chip)/vip/isp_drv.o

$(arch_cvitek_chip)_sensor_i2c-objs += chip/$(arch_cvitek_chip)/cvi_vip_snsr_i2c.o

cvi_mipi_rx-objs += chip/$(arch_cvitek_chip)/cvi_vip_cif.o \
					chip/$(arch_cvitek_chip)/cif/cif_drv.o

cvi_mipi_tx-objs := chip/$(arch_cvitek_chip)/cvi_vip_mipi_tx.o \
					chip/$(arch_cvitek_chip)/cvi_vip_mipi_tx_proc.o
