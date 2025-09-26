#include "vi_reg.h"
#include "vi_raw_ip_ctrl.h"

/*******************************************************************************
 *	RAW IPs config
 ******************************************************************************/
void ispblk_blc_dg_wb_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool enable)
{
	uintptr_t blc_db_wb = ctx->phys_regs[blk_id];
	union reg_blc_dg_wb_base_config base_config;

	base_config.raw = ISP_RD_REG(blc_db_wb, reg_blc_dg_wb_t, base_config);
	base_config.bits.blc_le_enable = enable;
	base_config.bits.blc_se_enable = enable;
	base_config.bits.wbg_le_enable = enable;
	base_config.bits.wbg_se_enable = enable;

	ISP_WR_BITS(blc_db_wb, reg_blc_dg_wb_t, auto_cg_en, hw_auto_cg_en, 1);
	ISP_WR_REG(blc_db_wb, reg_blc_dg_wb_t, base_config, base_config.raw);
}

void ispblk_fusion_config(struct isp_ctx *ctx, bool enable, enum isp_fs_out_e out_sel)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_FUSION];
	union reg_fusion_ctrl_reg reg_ctrl;

	reg_ctrl.raw = ISP_RD_REG(fusion, reg_fusion_t, fusion_ctrl_reg);
	reg_ctrl.bits.u1_fusion_en = enable;
	reg_ctrl.bits.u2_fusion_fnum = (out_sel == ISP_FS_OUT_FS) ? 1 : 0;
	reg_ctrl.bits.u2_fusion_mode = out_sel;
	reg_ctrl.bits.u2_fusion_ysel = 3;
	reg_ctrl.bits.u2_fusion_nsel = 1;
	ISP_WR_BITS(fusion, reg_fusion_t, hw_auto, hw_auto_cg_en, 1);
	ISP_WR_REG(fusion, reg_fusion_t, fusion_ctrl_reg, reg_ctrl.raw);
}

void ispblk_map_curve_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t map_curve = ctx->phys_regs[ISP_BLK_ID_MAPCURVE];

	ISP_WR_BITS(map_curve, reg_map_curve_t, hw_auto, hw_auto_cg_en, 1);
	ISP_WR_BITS(map_curve, reg_map_curve_t, reg_01, u1_fcurve16_en, enable);
}

/**
 * ispblk_dpc_set_static - set defect pixels for static dpc.
 *
 * @param ctx: global settings
 * @param offset: mem-offset for 4k tile
 * @param bps: array of defect pixels. [23:12]-row, [11:0]-col.
 * @param count: number of defect pixels.
 */

void ispblk_dpc_set_static(struct isp_ctx *ctx, enum isp_raw_path_e path,
			     u16 offset, u32 *bps, u16 count)
{
#ifdef TODO_VI
	uintptr_t dpc = ctx->phys_regs[ISP_BLK_ID_DPC];
	u16 i = 0;

	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 1);

	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_st_addr, 0x80000000 | offset);

	for (i = 0; (i < count) && (i < 4096); ++i)
		ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x80000000 | *(bps + i));

	// write 1 3-fff-fff to end
	ISP_WR_REG(dpc, reg_isp_dpc_t, dpc_mem_w0, 0x83ffffff);
	ISP_WR_BITS(dpc, reg_isp_dpc_t, dpc_17, dpc_mem_prog_mode, 0);
#endif
}

void ispblk_dpc_config(struct isp_ctx *ctx, bool enable, u8 test_case)
{
	uintptr_t dpc = ctx->phys_regs[ISP_BLK_ID_DPC];
	union reg_dpc_base_config base_config;

	base_config.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, base_config);
	base_config.bits.dpc_enable = enable;
	base_config.bits.ge_enable = enable;
	base_config.bits.spc_enable = enable;
	ISP_WR_REG(dpc, reg_isp_dpc_t, base_config, base_config.raw);

	if (test_case == 1) { // test static dpc
		base_config.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, base_config);
		base_config.bits.dpc_enable = 0;
		base_config.bits.spc_enable = 1;
		ISP_WR_REG(dpc, reg_isp_dpc_t, base_config, base_config.raw);
	} else if (test_case == 2) { // test dynamic dpc
		base_config.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, base_config);
		base_config.bits.dpc_enable = 1;
		base_config.bits.spc_enable = 0;
		ISP_WR_REG(dpc, reg_isp_dpc_t, base_config, base_config.raw);
	}

	ISP_WR_BITS(dpc, reg_isp_dpc_t, auto_cg_en, hw_auto_cg_en, 1);
}

void ispblk_af_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t sts = ctx->phys_regs[ISP_BLK_ID_AF];
	int numx = 17, numy = 15;

	ISP_WR_BITS(sts, reg_isp_af_t, kickoff, af_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, dmi_enable, dmi_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_horizon_0_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_horizon_1_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_vertical_0_enable, enable);
	ISP_WR_BITS(sts, reg_isp_af_t, enables, af_hlc_enable, 1);
	ISP_WR_REG(sts, reg_isp_af_t, bypass, !enable);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(sts, reg_isp_af_t, offset_x, af_offset_x, 0x8);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(sts, reg_isp_af_t, offset_x, af_offset_y, 0x2);
	// block_width >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_width, (ctx->cfg_info.img_width - 16) / numx);
	// block_height >= 15
	ISP_WR_REG(sts, reg_isp_af_t, block_height, (ctx->cfg_info.img_height - 4) / numy);
	ISP_WR_REG(sts, reg_isp_af_t, block_num_x, 17);
	ISP_WR_REG(sts, reg_isp_af_t, block_num_y, 15);

	ISP_WR_REG(sts, reg_isp_af_t, high_y_thre, 0x258);

	ISP_WR_REG(sts, reg_isp_af_t, image_width, ctx->cfg_info.img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_width, ctx->cfg_info.img_width - 1);
	ISP_WR_BITS(sts, reg_isp_af_t, mxn_image_width_m1, af_mxn_image_height, ctx->cfg_info.img_height - 1);
}

void ispblk_bnr_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];

	ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_00, hw_auto_cg_en, 1);
	ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_00, u1_bnr_enable, enable);
}

void ispblk_lscr_config(struct isp_ctx *ctx, bool enable, u16 *data, u32 size)
{
	uintptr_t lscr = ctx->phys_regs[ISP_BLK_ID_LSCR];
	uintptr_t addr = 0;
	uint32_t val = 0;
	int pipe = ctx->cfg_info.pipe;
	int img_width = ctx->cfg_info.img_width;
	int img_height = ctx->cfg_info.img_height;
	u8 bayer_fmt = ctx->cfg_info.ai_cfg.is_raw_planar ? ISP_BAYER_TYPE_BG : ctx->isp_pipe_cfg[pipe].rgb_color_mode;

	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, hw_auto_cg_en, 1);
	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, bypass, !enable);
	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, hw_auto_cg_en, enable);

	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_width, img_width - 1);
	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_height, img_height - 1);

	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, lsc_bayer_starting, bayer_fmt);

	addr = lscr + _OFST(reg_isp_lscr_t, sc_wrap_6);

	REG_ARRAY_UPDATE2_SIZE(addr, data, size);
}

void ispblk_drc_config(struct isp_ctx *ctx, bool enable)
{
	uintptr_t drc = ctx->phys_regs[ISP_BLK_ID_DRC];
	int sub_img_width = 32;
	int sub_img_height = 32;
	int pipe = ctx->cfg_info.pipe;
	int img_width = ctx->cfg_info.img_width;
	int img_height = ctx->cfg_info.img_height;
	int subimg_ratio_hori = img_width * 64 / sub_img_width;
	int subimg_ratio_vert = img_height * 64 / sub_img_height;
	unsigned int subimg_ratio_hori_div = sub_img_width * 65536 / img_width;
	unsigned int subimg_ratio_vert_div = sub_img_height * 65536 / img_height;
	u8 bayer_fmt = ctx->cfg_info.ai_cfg.is_raw_planar ? ISP_BAYER_TYPE_BG : ctx->isp_pipe_cfg[pipe].rgb_color_mode;

	ISP_WR_BITS(drc, reg_isp_drc_t, pipectrl_cg, hw_auto_cg_en, 1);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_enable, drc_enable, enable);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_hist, drc_hist_enable, enable);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_num, drc_subimg_width, sub_img_width - 1);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_num, drc_subimg_height, sub_img_height - 1);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_size, drc_subimg_ratio_hori, subimg_ratio_hori);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_size, drc_subimg_ratio_vert, subimg_ratio_vert);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_div, drc_subimg_ratio_hori_div, subimg_ratio_hori_div);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_blk_div, drc_subimg_ratio_vert_div, subimg_ratio_vert_div);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_img_size, img_width_m1, img_width - 1);
	ISP_WR_BITS(drc, reg_isp_drc_t, drc_img_size, img_height_m1, img_height - 1);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_bayerid, pre_img_bayerid, bayer_fmt);
}

void ispblk_cfa_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id)
{
	uintptr_t cfa = ctx->phys_regs[blk_id];
	union reg_isp_cfa_00 reg_0;

	reg_0.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_00);
	reg_0.bits.cfa_shdw_sel = 1;
	reg_0.bits.cfa_enable	= 1;
	//reg_0.bits.cfa_fcr_enable = 1;
	//reg_0.bits.cfa_moire_enable = 1;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_00, reg_0.raw);
}

