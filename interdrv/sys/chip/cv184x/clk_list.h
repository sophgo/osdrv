#ifndef __CLK_LIST_H__
#define __CLK_LIST_H__

#include "osal.h"

static osal_clk_ctrl_info clk_list[] = {
	{
		.name = "reg_vc_clk_video_axi_en",
		.reg_addr = 0x30020e8,
		.bit = 14,
	},
	{
		.name = "reg_vc_clk_vc_src0_en",
		.reg_addr = 0x30020e8,
		.bit = 15,
	},
	{
		.name = "reg_vc_clk_vc_src1_en",
		.reg_addr = 0x30020e8,
		.bit = 16,
	},
	{
		.name = "reg_vivo_clk_x2p_en",
		.reg_addr = 0x30020e8,
		.bit = 17,
	},
	{
		.name = "reg_vivo_clk_raw_axi_en",
		.reg_addr = 0x30020e8,
		.bit = 18,
	},
	{
		.name = "reg_vivo_clk_src_vip_sys_0_en",
		.reg_addr = 0x30020e8,
		.bit = 19,
	},
	{
		.name = "reg_vivo_clk_src_vip_sys_1_en",
		.reg_addr = 0x30020e8,
		.bit = 20,
	},
	{
		.name = "reg_vivo_clk_src_vip_sys_2_en",
		.reg_addr = 0x30020e8,
		.bit = 21,
	},
	{
		.name = "reg_vivo_clk_src_vip_sys_3_en",
		.reg_addr = 0x30020e8,
		.bit = 22,
	},
	{
		.name = "reg_vivo_clk_src_vip_sys_4_en",
		.reg_addr = 0x30020e8,
		.bit = 23,
	},
	{
		.name = "reg_vivo_clk_sys_disp_en",
		.reg_addr = 0x30020e8,
		.bit = 24,
	},
	{
		.name = "reg_vivo_clk_sys_disp_en",
		.reg_addr = 0x30020e8,
		.bit = 25,
	},
	{
		.name = "reg_vivo_clk_cyc_dsi_esc_en",
		.reg_addr = 0x30020e8,
		.bit = 26,
	},
	{
		.name = "reg_vivo_clk_cyc_scan_300m_en",
		.reg_addr = 0x30020e8,
		.bit = 27,
	},
	{
		.name = "reg_vivo_clk_cyc_dsi_syn_en",
		.reg_addr = 0x30020e8,
		.bit = 28,
	},
	{
		.name = "reg_vivo_clk_mipimpll_en",
		.reg_addr = 0x30020e8,
		.bit = 29,
	},
	{
		.name = "reg_clk_disp_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 4,
	},
	{
		.name = "reg_clk_csi_mac0_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 5,
	},
	{
		.name = "reg_clk_csi_mac1_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 6,
	},
	{
		.name = "reg_clk_csi_mac2_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 7,
	},
	{
		.name = "reg_clk_csi_be_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 8,
	},
	{
		.name = "reg_clk_isp_top_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 9,
	},
	{
		.name = "reg_clk_raw_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 10,
	},
	{
		.name = "reg_clk_vpss0_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 11,
	},
	{
		.name = "reg_clk_vpss1_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 12,
	},
	{
		.name = "reg_clk_vpss2_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 13,
	},
	{
		.name = "reg_clk_vpss3_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 14,
	},
	{
		.name = "reg_clk_ldc_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 15,
	},
	{
		.name = "reg_clk_cam0_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 16,
	},
	{
		.name = "reg_clk_cam1_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 17,
	},
	{
		.name = "reg_clk_cam2_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 18,
	},
	{
		.name = "reg_pad_vi0_clk0_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 19,
	},
	{
		.name = "reg_pad_vi0_clk1_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 20,
	},
	{
		.name = "reg_pad_vi1_clk_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 21,
	},
	{
		.name = "reg_pad_vi2_clk_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 22,
	},
	{
		.name = "reg_clk_lvds0_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 23,
	},
	{
		.name = "reg_clk_lvds1_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 24,
	},
	{
		.name = "reg_clk_dsi_mac_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 25,
	},
	{
		.name = "reg_clk_csi0_rx_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 26,
	},
	{
		.name = "reg_clk_csi1_rx_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 27,
	},
	{
		.name = "reg_clk_csi2_rx_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 28,
	},
	{
		.name = "reg_clk_vo_mac_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 29,
	},
	{
		.name = "reg_clk_2de_vip_en",
		.reg_addr = 0x30020f0,
		.bit = 30,
	},
	{
		.name = "reg_clk_apb_en_vcsys",
		.reg_addr = 0x30020f0,
		.bit = 31,
	},
	{
		.name = "reg_clk_apb_en_ve",
		.reg_addr = 0x30020f4,
		.bit = 0,
	},
	{
		.name = "reg_clk_apb_en_jpeg",
		.reg_addr = 0x30020f4,
		.bit = 1,
	},
	{
		.name = "reg_clk_en_ve",
		.reg_addr = 0x30020f4,
		.bit = 2,
	},
	{
		.name = "reg_clk_en_jpeg",
		.reg_addr = 0x30020f4,
		.bit = 3,
	},
	{
		.name = "reg_clk_apb_audsrc_en",
		.reg_addr = 0x30020f4,
		.bit = 4,
	},
	{
		.name = "reg_clk_apb_i2s0_en",
		.reg_addr = 0x30020f8,
		.bit = 2,
	},
	{
		.name = "reg_clk_apb_i2s1_en",
		.reg_addr = 0x30020f8,
		.bit = 3,
	},
	{
		.name = "reg_clk_apb_i2s2_en",
		.reg_addr = 0x30020f8,
		.bit = 4,
	},
	{
		.name = "reg_clk_apb_i2s3_en",
		.reg_addr = 0x30020f8,
		.bit = 5,
	},
	{
		.name = "reg_hsperi_clk_audsrc_en",
		.reg_addr = 0x30020ec,
		.bit = 12,
	},
	{
		.name = "reg_hsperi_clk_aud0_en",
		.reg_addr = 0x30020ec,
		.bit = 13,
	},
	{
		.name = "reg_hsperi_clk_aud1_en",
		.reg_addr = 0x30020ec,
		.bit = 14,
	},
	{
		.name = "reg_hsperi_clk_aud2_en",
		.reg_addr = 0x30020ec,
		.bit = 15,
	},
	{
		.name = "reg_hsperi_clk_aud3_en",
		.reg_addr = 0x30020ec,
		.bit = 16,
	}
};


#endif
