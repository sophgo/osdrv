#include "vi_reg.h"
#include "vi_fe_ip_ctrl.h"

#define CLK_600MHZ		(600000000)
#define CLK_400MHZ		(400000000)
#define CLK_300MHZ		(300000000)
#define DEF_FPS			(30)
#define MIN_VBLANKING		(112)

/*******************************************************************************
 *	FE IPs config
 ******************************************************************************/
enum sop_isp_raw find_phy_raw_num(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	if (raw_num < ISP_PRERAW_VIRT0)
		return raw_num;

	return ctx->isp_csi_cfg[raw_num].phy_raw;
}

int csibdg_find_hwid(enum sop_isp_raw raw_num)
{
	int csibdg_id = ISP_BLK_ID_CSIBDG0;

	switch (raw_num) {
	case ISP_PRERAW0:
		csibdg_id = ISP_BLK_ID_CSIBDG0;
		break;
	case ISP_PRERAW1:
		csibdg_id = ISP_BLK_ID_CSIBDG1;
		break;
	case ISP_PRERAW2:
		csibdg_id = ISP_BLK_ID_CSIBDG2;
		break;
	case ISP_PRERAW_LITE0:
		csibdg_id = ISP_BLK_ID_CSIBDG0_LITE;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_%d\n", raw_num);
		break;
	}

	return csibdg_id;
}

int fe_find_hwid(enum sop_isp_raw raw_num)
{
	int fe_id = ISP_BLK_ID_PRE_RAW_FE0;

	switch (raw_num) {
	case ISP_PRERAW0:
		fe_id = ISP_BLK_ID_PRE_RAW_FE0;
		break;
	case ISP_PRERAW1:
		fe_id = ISP_BLK_ID_PRE_RAW_FE1;
		break;
	case ISP_PRERAW2:
		fe_id = ISP_BLK_ID_PRE_RAW_FE2;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_%d\n", raw_num);
		break;
	}

	return fe_id;
}

int clsc_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn)
{
	int fe_id = ISP_BLK_ID_PRE_RAW_FE0_LSC0;

	switch (raw_num) {
	case ISP_PRERAW0:
		fe_id = (chn == ISP_FE_CH0) ? ISP_BLK_ID_PRE_RAW_FE0_LSC0 : ISP_BLK_ID_PRE_RAW_FE0_LSC1;
		break;
	case ISP_PRERAW1:
		fe_id = (chn == ISP_FE_CH0) ? ISP_BLK_ID_PRE_RAW_FE1_LSC0 : ISP_BLK_ID_PRE_RAW_FE1_LSC1;
		break;
	case ISP_PRERAW2:
		fe_id = ISP_BLK_ID_PRE_RAW_FE2_LSC0;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_%d\n", raw_num);
		break;
	}

	return fe_id;
}

int ae_find_hwid(enum sop_isp_raw raw_num)
{
	int ae_id = ISP_BLK_ID_AE_HIST_FE0;

	switch (raw_num) {
	case ISP_PRERAW0:
		ae_id = ISP_BLK_ID_AE_HIST_FE0;
		break;
	case ISP_PRERAW1:
		ae_id = ISP_BLK_ID_AE_HIST_FE1;
		break;
	case ISP_PRERAW2:
		ae_id = ISP_BLK_ID_AE_HIST_FE2;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_%d\n", raw_num);
		break;
	}

	return ae_id;
}

static void _patgen_config_timing(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];
	u8 frm = ctx->isp_csi_cfg[raw_num].is_yuv_sensor
		? (ctx->isp_csi_cfg[raw_num].mux_mode + 1)
		: (ctx->isp_csi_cfg[raw_num].is_hdr_on ? 2 : 1);
	u16 pat_height = (ctx->isp_csi_cfg[raw_num].csibdg_height * frm - 1);
	u8 fps = ctx->isp_csi_cfg[raw_num].patgen_fps ? ctx->isp_csi_cfg[raw_num].patgen_fps : DEF_FPS;
	u16 htt = 0, vtt = 0;

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_v_size, vmde_str, 0x00);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_v_size, vmde_stp, pat_height);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_h_size, hmde_str, 0x00);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_mde_h_size, hmde_stp,
							ctx->isp_csi_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_v_size, vfde_str, 0x10);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_v_size, vfde_stp, 0x10 + pat_height);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_h_size, hfde_str, 0x10);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fde_h_size, hfde_stp,
							0x10 + ctx->isp_csi_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_hsync_ctrl, hs_str, 4);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_hsync_ctrl, hs_stp, 5);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_vsync_ctrl, vs_str, 4);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_vsync_ctrl, vs_stp, 5);

	if (patgen_vblanking < MIN_VBLANKING)
		patgen_vblanking = MIN_VBLANKING;

	vtt = pat_height * patgen_vblanking / 100;

	switch (raw_num) {
	case ISP_PRERAW0:
		htt = CLK_600MHZ / fps / vtt;
		break;
	case ISP_PRERAW1:
		htt = CLK_400MHZ / fps / vtt;
		break;
	default:
		htt = CLK_600MHZ / fps / vtt;
			break;
	}

	htt = (htt > 0x3FFF) ? 0x3FFF : htt;
	vtt = (vtt > 0x3FFF) ? 0x3FFF : vtt;

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, vtt, vtt);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_tgen_tt_size, htt, htt);

	vi_pr(VI_DBG, "VTT(%d) HTT(%d) fps(%d)\n", vtt, htt, fps);
}

static void _patgen_config_pat(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, gra_inv, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, auto_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, snow_en, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, fix_mc, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, dith_md, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, bayer_id, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_prd, 0);
	if (raw_num == ISP_PRERAW0)
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, 0x17);
	else
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_idx_ctrl, pat_idx, raw_num);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_r, 0xfff);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_0, pat_g, 0xfff);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_color_1, pat_b, 0xfff);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_r, 0);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_0, fde_g, 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_background_color_1, fde_b, 2);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_r, 0x457);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_0, mde_g, 0x8ae);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_fix_color_1, mde_b, 0xd05);
}

void ispblk_csidbg_dma_wr_en(
	struct isp_ctx *ctx,
	const enum sop_isp_raw raw_num,
	const enum sop_isp_fe_chn_num chn_num,
	const u8 en)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];

	switch (chn_num) {
	case ISP_FE_CH0:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, en);
		break;
	case ISP_FE_CH1:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_wr_enable, en);
		break;
	case ISP_FE_CH2:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch2_dma_wr_enable, en);
		break;
	case ISP_FE_CH3:
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch3_dma_wr_enable, en);
		break;
	default:
		break;
	}
}

void ispblk_csibdg_wdma_crop_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, struct vi_rect crop, u8 en)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_crop_en, st_ch0_crop_en, en);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_horz_crop, st_ch0_horz_crop_start, crop.x);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_horz_crop, st_ch0_horz_crop_end, crop.x + crop.w - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_vert_crop, st_ch0_vert_crop_start, crop.y);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch0_vert_crop, st_ch0_vert_crop_end, crop.y + crop.h - 1);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_crop_en, st_ch1_crop_en, en);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_horz_crop, st_ch1_horz_crop_start, crop.x);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_horz_crop, st_ch1_horz_crop_end, crop.x + crop.w - 1);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_vert_crop, st_ch1_vert_crop_start, crop.y);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, wdma_ch1_vert_crop, st_ch1_vert_crop_end, crop.y + crop.h - 1);
	}

	vi_pr(VI_DBG, "Preraw_%d, crop_x:y:w:h=%d:%d:%d:%d\n", raw_num, crop.x, crop.y, crop.w, crop.h);
}

void ispblk_csibdg_update_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_widthm1,
				ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_heightm1,
				ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_widthm1,
				ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_heightm1,
				ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_widthm1,
				ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_heightm1,
				ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_widthm1,
				ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_heightm1,
				ctx->isp_csi_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];
	struct vi_rect crop, crop_se;

	crop.x = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].x;
	crop.y = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].y;
	crop.w = (ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].x + ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w) - 1;
	crop.h = (ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].y + ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h) - 1;

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_crop_en, ch0_crop_en, en);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_horz_crop, ch0_horz_crop_start, crop.x);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_horz_crop, ch0_horz_crop_end, crop.w);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_vert_crop, ch0_vert_crop_start, crop.y);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_vert_crop, ch0_vert_crop_end, crop.h);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		crop_se.x = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].x;
		crop_se.y = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].y;
		crop_se.w = (ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].x
				+ ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].w) - 1;
		crop_se.h = (ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].y
				+ ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].h) - 1;

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_crop_en, ch1_crop_en, en);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_horz_crop, ch1_horz_crop_start, crop_se.x);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_horz_crop, ch1_horz_crop_end, crop_se.w);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_vert_crop, ch1_vert_crop_start, crop_se.y);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_vert_crop, ch1_vert_crop_end, crop_se.h);
	}

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, bayer_id,
		    ctx->isp_csi_cfg[raw_num].rgb_color_mode_pre_crop);
}

void ispblk_csibdg_reset(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	switch (raw_num) {
	case ISP_PRERAW0:
		ISP_WR_BITS(isptopb, reg_isp_top_t, sw_rst, csi0_rst, 1);
		/* code */
		break;
	case ISP_PRERAW1:
		ISP_WR_BITS(isptopb, reg_isp_top_t, sw_rst, csi1_rst, 1);
		break;
	case ISP_PRERAW2:
	case ISP_PRERAW_LITE0:
		ISP_WR_BITS(isptopb, reg_isp_top_t, sw_rst, csi2_rst, 1);
		break;
	default:
		break;
	}

	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x0);
}

int ispblk_csibdg_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];
	u8 csi_mode = 0;
	union reg_isp_csi_bdg_top_ctrl top_ctrl;
	union reg_isp_csi_bdg_interrupt_ctrl int_ctrl;
	union reg_isp_csi_bdg_ch0_ai_isp_transform ch0_ai_isp_transform;
	union reg_isp_csi_bdg_ch1_ai_isp_transform ch1_ai_isp_transform;

	top_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
	top_ctrl.bits.reset_mode	= 0;
	top_ctrl.bits.abort_mode	= 0;
	top_ctrl.bits.csi_in_format	= 0;
	top_ctrl.bits.ch_num		= ctx->isp_csi_cfg[raw_num].is_hdr_on;

	if (!_is_all_online(ctx)) { //fe->dram->be->post
		top_ctrl.bits.ch0_dma_wr_enable = 1;
		top_ctrl.bits.ch1_dma_wr_enable = ctx->isp_csi_cfg[raw_num].is_hdr_on;
	} else {
		top_ctrl.bits.ch0_dma_wr_enable = 0;
		top_ctrl.bits.ch1_dma_wr_enable = 0;
	}

	if (ctx->isp_csi_cfg[raw_num].is_patgen_en) {
		csi_mode = 3;
		top_ctrl.bits.pxl_data_sel = 1;
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, pat_en, 1);

		_patgen_config_timing(ctx, raw_num);
		_patgen_config_pat(ctx, raw_num);
	} else {
		top_ctrl.bits.pxl_data_sel = 0;

		csi_mode = 1;
	}

	top_ctrl.bits.csi_mode	= csi_mode;
	if (ctx->isp_csi_cfg[raw_num].is_patgen_en)
		top_ctrl.bits.vs_mode	= 0;
	else
		top_ctrl.bits.vs_mode	= ctx->isp_csi_cfg[raw_num].is_stagger_vsync;
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, top_ctrl.raw);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	if (ctx->is_dpcm_on) {
		union reg_isp_csi_bdg_dma_dpcm_mode dpcm;

		dpcm.bits.dma_st_dpcm_mode = 0x7; //12->6 mode
		dpcm.bits.dpcm_xstr = 8191;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, dpcm.raw);
	} else {
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, 0);
	}

	int_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_interrupt_ctrl);
	int_ctrl.bits.ch0_vs_int_en		= 1;
	int_ctrl.bits.ch1_vs_int_en		= 1;
	int_ctrl.bits.ch2_vs_int_en		= 1;
	int_ctrl.bits.ch3_vs_int_en		= 1;
	int_ctrl.bits.ch0_trig_int_en		= 0;
	int_ctrl.bits.ch1_trig_int_en		= 0;
	int_ctrl.bits.ch2_trig_int_en		= 0;
	int_ctrl.bits.ch3_trig_int_en		= 0;
	int_ctrl.bits.line_intp_en		= ctx->isp_csi_cfg[raw_num].is_mux_dev;

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_interrupt_ctrl, int_ctrl.raw);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, bayer_id,
						ctx->isp_csi_cfg[raw_num].rgb_color_mode_pre_crop);

	//ai isp cfg
	ch0_ai_isp_transform.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, ch0_ai_isp_transform);
	ch0_ai_isp_transform.bits.ai_isp_transform_en_ch0 = ctx->isp_csi_cfg[raw_num].ai_cfg.is_raw_planar;
	ch0_ai_isp_transform.bits.out_format_ch0 = ctx->isp_csi_cfg[raw_num].ai_cfg.fmt; //12bit
	ch0_ai_isp_transform.bits.round_mode_ch0 = ctx->isp_csi_cfg[raw_num].ai_cfg.round;
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, ch0_ai_isp_transform, ch0_ai_isp_transform.raw);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		ch1_ai_isp_transform.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, ch1_ai_isp_transform);
		ch1_ai_isp_transform.bits.ai_isp_transform_en_ch1 = ctx->isp_csi_cfg[raw_num].ai_cfg.is_raw_planar;
		ch1_ai_isp_transform.bits.out_format_ch1 = ctx->isp_csi_cfg[raw_num].ai_cfg.fmt; //12bit
		ch1_ai_isp_transform.bits.round_mode_ch1 = ctx->isp_csi_cfg[raw_num].ai_cfg.round;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, ch1_ai_isp_transform, ch1_ai_isp_transform.raw);
	}

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, bayer_type, ctx->isp_csi_cfg[raw_num].rgb_color_mode);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, bayer_type_clk_gate_yuv_swap, bayer_type_post_crop_multich,
						ctx->isp_csi_cfg[raw_num].rgb_color_mode);

	return 0;
}

void ispblk_csibdg_lite_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];
	union reg_isp_csi_bdg_lite_bdg_top_ctrl csibdg_ctrl;
	union reg_isp_csi_bdg_lite_bdg_interrupt_ctrl_1 csibdg_intr_ctrl_1;
	u8 chn_num = ctx->isp_csi_cfg[raw_num].mux_mode;

	vi_pr(VI_DBG, "csibdg_lite[%d], chn_num[%d], inf_mode[%d]\n",
	      raw_num, chn_num, ctx->isp_csi_cfg[raw_num].inf_mode);

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, interrupt_status_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, interrupt_status_1, 0xFFFFFFFF);

	csibdg_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl);
	csibdg_ctrl.bits.csi_mode		= 0;
	csibdg_ctrl.bits.ch_num			= chn_num;
	csibdg_ctrl.bits.y_only			= 0;
	csibdg_ctrl.bits.reset_mode		= 0;
	csibdg_ctrl.bits.abort_mode		= 0;
	csibdg_ctrl.bits.ch0_dma_wr_enable	= true;
	csibdg_ctrl.bits.ch1_dma_wr_enable	= (chn_num > 0) ? true : false;
	csibdg_ctrl.bits.ch2_dma_wr_enable	= (chn_num > 1) ? true : false;
	csibdg_ctrl.bits.ch3_dma_wr_enable	= (chn_num > 2) ? true : false;
	if (ctx->isp_csi_cfg[raw_num].is_422_to_420) {
		csibdg_ctrl.bits.ch0_dma_420_wr_enable	= true;
		csibdg_ctrl.bits.ch1_dma_420_wr_enable	= (chn_num > 0) ? true : false;
	} else {
		csibdg_ctrl.bits.ch0_dma_420_wr_enable	= 0;
		csibdg_ctrl.bits.ch1_dma_420_wr_enable	= 0;
	}
	csibdg_ctrl.bits.ch2_dma_420_wr_enable	= 0;
	csibdg_ctrl.bits.ch3_dma_420_wr_enable	= 0;

	csibdg_intr_ctrl_1.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_1);
	csibdg_intr_ctrl_1.bits.line_intp_en		= 0;
	csibdg_intr_ctrl_1.bits.fifo_overflow_int_en	= 1;
	csibdg_intr_ctrl_1.bits.dma_error_intp_en	= 1;
	csibdg_intr_ctrl_1.bits.drop_mode		= 0;
	csibdg_intr_ctrl_1.bits.avg_mode		= (ctx->isp_csi_cfg[raw_num].is_422_to_420) ? true : false;

	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, csibdg_ctrl.raw);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_interrupt_ctrl_1, csibdg_intr_ctrl_1.raw);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_lite_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	union reg_isp_csi_bdg_top_ctrl csibdg_ctrl;
	u8 csi_mode = 1, pxl_data_sel = 0, csi_in_fmt = 1;
	u8 stagger_vsync = ctx->isp_csi_cfg[raw_num].is_stagger_vsync;
	u8 chn_num = ctx->isp_csi_cfg[raw_num].mux_mode;

	if (ctx->isp_csi_cfg[raw_num].is_patgen_en) {
		csi_mode = 3;
		pxl_data_sel = 1;
		csi_in_fmt = 0;
		stagger_vsync = 0;

		_patgen_config_timing(ctx, raw_num);
		_patgen_config_pat(ctx, raw_num);

		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_pat_gen_ctrl, pat_en, 1);
	}

	/*
	 * MIPI--->MUX use FE0 or FE1
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 * BT----->MUX use csibdg lite
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 */
	vi_pr(VI_DBG, "csibdg_yuv[%d], chn_num[%d], inf_mode[%d]\n",
	      raw_num, chn_num, ctx->isp_csi_cfg[raw_num].inf_mode);

	if (chn_num == 0 || (ctx->isp_csi_cfg[raw_num].inf_mode >= VI_MODE_MIPI_YUV420_NORMAL &&
	    ctx->isp_csi_cfg[raw_num].inf_mode <= VI_MODE_MIPI_YUV422)) {
		csibdg_ctrl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		csibdg_ctrl.bits.reset_mode		= 0;
		csibdg_ctrl.bits.abort_mode		= 1;
		csibdg_ctrl.bits.csi_mode		= csi_mode;
		csibdg_ctrl.bits.csi_in_format		= csi_in_fmt;
		csibdg_ctrl.bits.csi_in_yuv_format	= 0;
		csibdg_ctrl.bits.y_only			= 0;
		csibdg_ctrl.bits.yuv2bay_enable		= 0;
		csibdg_ctrl.bits.ch_num			= chn_num;
		csibdg_ctrl.bits.multi_ch_frame_sync_en	= 0;
		csibdg_ctrl.bits.pxl_data_sel		= pxl_data_sel;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_ctrl.raw);

	} else if (chn_num > 0 && ctx->isp_csi_cfg[raw_num].inf_mode >= VI_MODE_BT656 &&
		ctx->isp_csi_cfg[raw_num].inf_mode <= VI_MODE_BT1120_INTERLEAVED) {
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0_LITE];
	}

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, vs_mode, stagger_vsync);

	if (!_is_all_online(ctx)) // sensor->fe->dram
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, true);
	else // sensor->fe->yuvtop
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_wr_enable, false);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_wr_enable, (chn_num > 0) ? true : false);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch2_dma_wr_enable, (chn_num > 1) ? true : false);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch3_dma_wr_enable, (chn_num > 2) ? true : false);

	if (ctx->isp_csi_cfg[raw_num].is_422_to_420) {
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch0_dma_420_wr_enable, true);
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, ch1_dma_420_wr_enable,
									(chn_num > 0) ? true : false);
		//only chn0 support avg mode
		ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, csi_bdg_dma_dpcm_mode, avg_mode, true);
	}

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch0_size, ch0_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch1_size, ch1_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch2_size, ch2_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_widthm1,
					ctx->isp_csi_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, ch3_size, ch3_frame_heightm1,
					ctx->isp_csi_cfg[raw_num].csibdg_height - 1);

	// pre_raw must input UYVY or VYUY
	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
		ctx->isp_csi_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_BYPASS) {
		if (ctx->isp_csi_cfg[raw_num].data_seq == VI_DATA_SEQ_YUYV ||
				ctx->isp_csi_cfg[raw_num].data_seq == VI_DATA_SEQ_YVYU) {
			ISP_WR_BITS(csibdg, reg_isp_csi_bdg_t, bayer_type_clk_gate_yuv_swap, yuv_ch_swap_en, 1);
		}
	}

}

void ispblk_clsc_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn, bool en)
{
	int blk_id;
	uintptr_t clsc;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int width = ctx->isp_csi_cfg[raw_num].is_mux_dev ? ctx->isp_csi_cfg[raw_num].csibdg_width
							 : ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w;
	int height = ctx->isp_csi_cfg[raw_num].is_mux_dev ? ctx->isp_csi_cfg[raw_num].csibdg_height
							 : ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h;
	int mesh_num = 37;
	int InnerBlkX = mesh_num - 1 - 2;
	int InnerBlkY = mesh_num - 1 - 2;
	int mesh_x_coord_unit = (InnerBlkX * (1 << 15)) / width;
	int mesh_y_coord_unit = (InnerBlkY * (1 << 15)) / height;
	u32 reg_lsc_xstep = mesh_x_coord_unit + 1;
	u32 reg_lsc_ystep = mesh_y_coord_unit + 1;
	int image_w_in_mesh_unit = width * reg_lsc_xstep;
	int image_h_in_mesh_unit = height * reg_lsc_ystep;
	int OuterBlkX = InnerBlkX + 2;
	int OuterBlkY = InnerBlkY + 2;
	u32 reg_lsc_imgx0 = (OuterBlkX * (1 << 15) - image_w_in_mesh_unit) / 2;
	u32 reg_lsc_imgy0 = (OuterBlkY * (1 << 15) - image_h_in_mesh_unit) / 2;

	blk_id = clsc_find_hwid(phy_raw, chn);
	clsc = ctx->phys_regs[blk_id];
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, hw_auto_cg_en, 1);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, lsc_enable, en);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, lsc_bayer_starting,
				ctx->isp_csi_cfg[raw_num].rgb_color_mode);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_11, dma_enable_lsc, en);

	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_5, lsc_imgx0, reg_lsc_imgx0);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_6, lsc_imgy0, reg_lsc_imgy0);

	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_7, lsc_xstep, reg_lsc_xstep);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_7, lsc_ystep, reg_lsc_ystep);
}

bool ispblk_clsc_is_enable(struct isp_ctx *ctx, enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn)
{
	int blk_le;
	uintptr_t clsc;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);

	if (chn > ISP_FE_CH1) {
		vi_pr(VI_ERR, "Invalid channel number %d\n", chn);
		return false;
	}

	blk_le = clsc_find_hwid(phy_raw, chn);
	clsc = ctx->phys_regs[blk_le];

	return ISP_RD_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, lsc_enable) == 0 ? false : true;
}

void ispblk_clsc_dma_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, uint64_t buf_addr)
{
	int blk_le_id, blk_se_id;
	uintptr_t clsc_le, clsc_se;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);

	blk_le_id = clsc_find_hwid(phy_raw, ISP_FE_CH0);
	clsc_le = ctx->phys_regs[blk_le_id];
	ISP_WR_BITS(clsc_le, reg_isp_lsc_t, sc_wrap_11, lsc_dma_addr_high8, ((buf_addr >> 32) & 0xFFFFFFFF));
	ISP_WR_BITS(clsc_le, reg_isp_lsc_t, sc_wrap_12, lsc_dma_addr_low32, (buf_addr & 0xFFFFFFFF));

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		blk_se_id = clsc_find_hwid(phy_raw, ISP_FE_CH1);
		clsc_se = ctx->phys_regs[blk_se_id];
		ISP_WR_BITS(clsc_se, reg_isp_lsc_t, sc_wrap_11, lsc_dma_addr_high8, ((buf_addr >> 32) & 0xFFFFFFFF));
		ISP_WR_BITS(clsc_se, reg_isp_lsc_t, sc_wrap_12, lsc_dma_addr_low32, (buf_addr & 0xFFFFFFFF));
	}
}

void ispblk_aehist_reset(struct isp_ctx *ctx, int blk_id, enum sop_isp_raw raw_num)
{
	uintptr_t sts = ctx->phys_regs[blk_id];

	ISP_WR_REG(sts, reg_isp_ae_hist_t, ae_hist_grace_reset, 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, ae_hist_grace_reset, 0);
}

void ispblk_aehist_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool enable)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int ae_id = ae_find_hwid(phy_raw);

	uintptr_t sts = ctx->phys_regs[ae_id];
	u8 num_x = 34, num_y = 30;
	u8 sub_window_w = 0, sub_window_h = 0;
	union reg_isp_ae_hist_sts_ae0_hist_enable ae_enable;

	ae_enable.raw = ISP_RD_REG(sts, reg_isp_ae_hist_t, sts_ae0_hist_enable);
	ae_enable.bits.sts_ae0_hist_enable	= enable;
	ae_enable.bits.ae0_gain_enable	= enable;
	ae_enable.bits.hist0_enable	= enable;
	ae_enable.bits.hist0_gain_enable = enable;
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae0_hist_enable, ae_enable.raw);
	ISP_WR_BITS(sts, reg_isp_ae_hist_t, dmi_enable, dmi_enable, enable);
	ISP_WR_BITS(sts, reg_isp_ae_hist_t, hw_auto_cg_en, hw_auto_cg_en, 1);

	if (!enable)
		return;

	sub_window_w = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w / num_x;
	sub_window_h = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h / num_y;

	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_numxm1, num_x - 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_numym1, num_y - 1);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_width, sub_window_w);
	ISP_WR_REG(sts, reg_isp_ae_hist_t, sts_ae_height, sub_window_h);
}