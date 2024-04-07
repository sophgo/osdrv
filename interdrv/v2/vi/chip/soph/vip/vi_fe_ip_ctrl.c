#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_MAX], g_h_bit[ISP_PRERAW_MAX];
extern uint8_t g_rgbmap_chg_pre[ISP_PRERAW_MAX][2];
/*******************************************************************************
 *	FE IPs config
 ******************************************************************************/
static void _patgen_config_timing(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	uint16_t pat_height = (ctx->is_hdr_on) ?
				(ctx->isp_pipe_cfg[raw_num].csibdg_height * 2 - 1) :
				(ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_V_SIZE, VMDE_STR, 0x00);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_V_SIZE, VMDE_STP, pat_height);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_H_SIZE, HMDE_STR, 0x00);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_MDE_H_SIZE, HMDE_STP,
							ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_V_SIZE, VFDE_STR, 0x10);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_V_SIZE, VFDE_STP, 0x10 + pat_height);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_H_SIZE, HFDE_STR, 0x10);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FDE_H_SIZE, HFDE_STP,
							0x10 + ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_HSYNC_CTRL, HS_STR, 4);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_HSYNC_CTRL, HS_STP, 5);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_VSYNC_CTRL, VS_STR, 4);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_VSYNC_CTRL, VS_STP, 5);

#if defined(__CV180X__)
/**
 * cv180x's clk_mac is 594M, after division frequency = 204M
 * htt * vtt * fps <= clk_mac * divider ratio
 * ex: target 1920x1080p25, htt = 0x1000, vtt = 0x7D0
 */
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, VTT, 0x7D0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, HTT, 0x1000);
#else
	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, VTT, 0x17FF);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, HTT, 0x17FF);
	} else {
		if (raw_num == ISP_PRERAW0) { //raw_num0 4k 20fps
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, VTT, 0x15FF);
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, HTT, 0x1FFF);
		} else { //raw_num3 4k 20fps
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, VTT, 0x14FF);
			ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_TGEN_TT_SIZE, HTT, 0x15FF);
		}
	}
#endif
}

static void _patgen_config_pat(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, GRA_INV, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, AUTO_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, SNOW_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, FIX_MC, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_MD, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, BAYER_ID, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_PRD, 0);
	if (raw_num == ISP_PRERAW0)
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0x7);
	else
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, raw_num);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_R, 0xFFF);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_G, 0xFFF);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_1, PAT_B, 0xFFF);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_R, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_G, 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_1, FDE_B, 2);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_R, 0x457);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_G, 0x8AE);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_1, MDE_B, 0xD05);
}
#ifdef PORTING_TEST
void ispblk_patgen_config_pat(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, uint8_t test_case)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, GRA_INV, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, AUTO_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, SNOW_EN, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, FIX_MC, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, DITH_MD, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, BAYER_ID, 0);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_PRD, 0);

	if (raw_num == ISP_PRERAW0)
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0x7);
	else
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 1);

	if (test_case == 0) { //white
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0x0);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_R, 0xFFF);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_G, 0xFFF);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_1, PAT_B, 0xFFF);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_R, 0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_G, 1);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_1, FDE_B, 2);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_R, 0x457);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_G, 0x8AE);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_1, MDE_B, 0xD05);
	} else if (test_case == 1) { //black
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0x0);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_R, 0x0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_0, PAT_G, 0x0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_COLOR_1, PAT_B, 0x0);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_R, 0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_0, FDE_G, 0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BACKGROUND_COLOR_1, FDE_B, 0);

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_R, 0x0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_0, MDE_G, 0x0);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_FIX_COLOR_1, MDE_B, 0x0);
	} else if (test_case == 3) { // to test ca lite
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_IDX_CTRL, PAT_IDX, 0xF);
	}
}
#endif

void ispblk_csidbg_dma_wr_en(
	struct isp_ctx *ctx,
	const enum cvi_isp_raw raw_num,
	const enum cvi_isp_fe_chn_num chn_num,
	const u8 en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	switch (chn_num) {
	case ISP_FE_CH0:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, en);
		break;
	case ISP_FE_CH1:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH1_DMA_WR_ENABLE, en);
		break;
	case ISP_FE_CH2:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, en);
		break;
	case ISP_FE_CH3:
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, en);
		break;
	default:
		break;
	}
}

void ispblk_csibdg_wdma_crop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num, struct vi_rect crop, u8 en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH0_CROP_EN, ST_CH0_CROP_EN, en);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH0_HORZ_CROP, ST_CH0_HORZ_CROP_START, crop.x);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH0_HORZ_CROP, ST_CH0_HORZ_CROP_END, crop.x + crop.w - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH0_VERT_CROP, ST_CH0_VERT_CROP_START, crop.y);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH0_VERT_CROP, ST_CH0_VERT_CROP_END, crop.y + crop.h - 1);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH1_CROP_EN, ST_CH1_CROP_EN, en);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH1_HORZ_CROP, ST_CH1_HORZ_CROP_START, crop.x);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH1_HORZ_CROP, ST_CH1_HORZ_CROP_END, crop.x + crop.w - 1);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH1_VERT_CROP, ST_CH1_VERT_CROP_START, crop.y);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, WDMA_CH1_VERT_CROP, ST_CH1_VERT_CROP_END, crop.y + crop.h - 1);
	}
}

void ispblk_csibdg_crop_update(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, bool en)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	struct vi_rect crop, crop_se;

	crop.x = (ctx->isp_pipe_cfg[raw_num].crop.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.x;
	crop.y = (ctx->isp_pipe_cfg[raw_num].crop.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop.y;
	crop.w = (ctx->isp_pipe_cfg[raw_num].crop.x + ctx->isp_pipe_cfg[raw_num].crop.w) - 1;
	crop.h = (ctx->isp_pipe_cfg[raw_num].crop.y + ctx->isp_pipe_cfg[raw_num].crop.h) - 1;

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_CROP_EN, CH0_CROP_EN, en);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_HORZ_CROP, CH0_HORZ_CROP_START, crop.x);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_HORZ_CROP, CH0_HORZ_CROP_END, crop.w);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_VERT_CROP, CH0_VERT_CROP_START, crop.y);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_VERT_CROP, CH0_VERT_CROP_END, crop.h);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		crop_se.x = (ctx->isp_pipe_cfg[raw_num].crop_se.x == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.x;
		crop_se.y = (ctx->isp_pipe_cfg[raw_num].crop_se.y == 0) ? 0 : ctx->isp_pipe_cfg[raw_num].crop_se.y;
		crop_se.w = (ctx->isp_pipe_cfg[raw_num].crop_se.x + ctx->isp_pipe_cfg[raw_num].crop_se.w) - 1;
		crop_se.h = (ctx->isp_pipe_cfg[raw_num].crop_se.y + ctx->isp_pipe_cfg[raw_num].crop_se.h) - 1;

		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_CROP_EN, CH1_CROP_EN, en);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_HORZ_CROP, CH1_HORZ_CROP_START, crop.x);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_HORZ_CROP, CH1_HORZ_CROP_END, crop.w);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_VERT_CROP, CH1_VERT_CROP_START, crop.y);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_VERT_CROP, CH1_VERT_CROP_END, crop.h);
	}
}

int ispblk_csibdg_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	uint8_t csi_mode = 0;
	union REG_ISP_CSI_BDG_TOP_CTRL top_ctrl;
	union REG_ISP_CSI_BDG_INTERRUPT_CTRL int_ctrl;

	top_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
	top_ctrl.bits.RESET_MODE	= 0;
	top_ctrl.bits.ABORT_MODE	= 0;
	top_ctrl.bits.CSI_IN_FORMAT	= 0;
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	if (_is_be_post_online(ctx)) { //fe->dram->be->post
		top_ctrl.bits.CH0_DMA_WR_ENABLE = 1;
		top_ctrl.bits.CH1_DMA_WR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else {
		top_ctrl.bits.CH0_DMA_WR_ENABLE = 0;
		top_ctrl.bits.CH1_DMA_WR_ENABLE = 0;
	}
	// ToDo stagger sensor
	//ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, ctx->is_offline_be);
	//ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, ctx->is_offline_be);

	if (ctx->isp_pipe_cfg[raw_num].is_patgen_en) {
		csi_mode = 3;
		top_ctrl.bits.PXL_DATA_SEL = 1;
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_PAT_GEN_CTRL, PAT_EN, 1);

		_patgen_config_timing(ctx, raw_num);
		_patgen_config_pat(ctx, raw_num);
	} else {
		top_ctrl.bits.PXL_DATA_SEL = 0;

		csi_mode = (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) ? 2 : 1;
	}

	top_ctrl.bits.CSI_MODE	= csi_mode;
	if (ctx->isp_pipe_cfg[raw_num].is_patgen_en)
		top_ctrl.bits.VS_MODE	= 0;
	else
		top_ctrl.bits.VS_MODE	= ctx->isp_pipe_cfg[raw_num].is_stagger_vsync;
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, top_ctrl.raw);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	if (ctx->is_dpcm_on) {
		union REG_ISP_CSI_BDG_DMA_DPCM_MODE dpcm;

		dpcm.bits.DMA_ST_DPCM_MODE = 0x7; //12->6 mode
		dpcm.bits.DPCM_XSTR = 8191;
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_DMA_DPCM_MODE, dpcm.raw);
	} else {
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_DMA_DPCM_MODE, 0);
	}

	int_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL);
	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		int_ctrl.bits.CH0_VS_INT_EN		= 0;
		int_ctrl.bits.CH1_VS_INT_EN		= 0;
		int_ctrl.bits.CH2_VS_INT_EN		= 0;
		int_ctrl.bits.CH3_VS_INT_EN		= 0;
		int_ctrl.bits.CH0_TRIG_INT_EN		= 1;
		int_ctrl.bits.CH1_TRIG_INT_EN		= 1;
		int_ctrl.bits.CH2_TRIG_INT_EN		= 1;
		int_ctrl.bits.CH3_TRIG_INT_EN		= 1;
	} else {
		int_ctrl.bits.CH0_VS_INT_EN		= 1;
		int_ctrl.bits.CH1_VS_INT_EN		= 1;
		int_ctrl.bits.CH2_VS_INT_EN		= 1;
		int_ctrl.bits.CH3_VS_INT_EN		= 1;
		int_ctrl.bits.CH0_TRIG_INT_EN		= 0;
		int_ctrl.bits.CH1_TRIG_INT_EN		= 0;
		int_ctrl.bits.CH2_TRIG_INT_EN		= 0;
		int_ctrl.bits.CH3_TRIG_INT_EN		= 0;
	}
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_INTERRUPT_CTRL, int_ctrl.raw);

	return 0;
}

void ispblk_csibdg_lite_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	union REG_ISP_CSI_BDG_LITE_BDG_TOP_CTRL csibdg_ctrl;
	union REG_ISP_CSI_BDG_LITE_BDG_INTERRUPT_CTRL_1 csibdg_intr_ctrl_1;
	u8 chn_num = ctx->isp_pipe_cfg[raw_num].muxMode;

	vi_pr(VI_DBG, "csibdg_lite[%d], chn_num[%d], infMode[%d]\n",
			raw_num, chn_num, ctx->isp_pipe_cfg[raw_num].infMode);

	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, INTERRUPT_STATUS_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, INTERRUPT_STATUS_1, 0xFFFFFFFF);

	csibdg_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, CSI_BDG_TOP_CTRL);
	csibdg_ctrl.bits.CSI_MODE		= 0;
	csibdg_ctrl.bits.CH_NUM			= chn_num;
	csibdg_ctrl.bits.Y_ONLY			= 0;
	csibdg_ctrl.bits.RESET_MODE		= 0;
	csibdg_ctrl.bits.ABORT_MODE		= 0;
	csibdg_ctrl.bits.CH0_DMA_WR_ENABLE	= true;
	csibdg_ctrl.bits.CH1_DMA_WR_ENABLE	= (chn_num > 0) ? true : false;
	csibdg_ctrl.bits.CH2_DMA_WR_ENABLE	= (chn_num > 1) ? true : false;
	csibdg_ctrl.bits.CH3_DMA_WR_ENABLE	= (chn_num > 2) ? true : false;
	if (ctx->isp_pipe_cfg[raw_num].is_422_to_420) {
		csibdg_ctrl.bits.CH0_DMA_420_WR_ENABLE	= true;
		csibdg_ctrl.bits.CH1_DMA_420_WR_ENABLE	= (chn_num > 0) ? true : false;
	} else {
		csibdg_ctrl.bits.CH0_DMA_420_WR_ENABLE	= 0;
		csibdg_ctrl.bits.CH1_DMA_420_WR_ENABLE	= 0;
	}
	csibdg_ctrl.bits.CH2_DMA_420_WR_ENABLE	= 0;
	csibdg_ctrl.bits.CH3_DMA_420_WR_ENABLE	= 0;

	csibdg_intr_ctrl_1.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, CSI_BDG_INTERRUPT_CTRL_1);
	csibdg_intr_ctrl_1.bits.LINE_INTP_EN		= 0;
	csibdg_intr_ctrl_1.bits.FIFO_OVERFLOW_INT_EN	= 1;
	csibdg_intr_ctrl_1.bits.DMA_ERROR_INTP_EN	= 1;
	csibdg_intr_ctrl_1.bits.DROP_MODE		= 0;
	csibdg_intr_ctrl_1.bits.AVG_MODE		= (ctx->isp_pipe_cfg[raw_num].is_422_to_420) ? true : false;

	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, CSI_BDG_TOP_CTRL, csibdg_ctrl.raw);
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, CSI_BDG_INTERRUPT_CTRL_0, 0xFFFFFFFF);
	ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_LITE_T, CSI_BDG_INTERRUPT_CTRL_1, csibdg_intr_ctrl_1.raw);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH0_SIZE, CH0_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH0_SIZE, CH0_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH1_SIZE, CH1_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH1_SIZE, CH1_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH2_SIZE, CH2_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH2_SIZE, CH2_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH3_SIZE, CH3_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_LITE_T, CH3_SIZE, CH3_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_csibdg_yuv_bypass_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t csibdg = ctx->phys_regs[id];
	union REG_ISP_CSI_BDG_TOP_CTRL csibdg_ctrl;
	u8 chn_num = ctx->isp_pipe_cfg[raw_num].muxMode;

	/*
	 * MIPI--->MUX use FE0 or FE1
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 * BT----->MUX use csibdg lite
	 *      |->No Mux use FE0 or FE1 -> chn_num = 0
	 */
	vi_pr(VI_DBG, "csibdg_yuv[%d], chn_num[%d], infMode[%d]\n",
			raw_num, chn_num, ctx->isp_pipe_cfg[raw_num].infMode);

	if (chn_num == 0 || (ctx->isp_pipe_cfg[raw_num].infMode >= VI_MODE_MIPI_YUV420_NORMAL &&
		ctx->isp_pipe_cfg[raw_num].infMode <= VI_MODE_MIPI_YUV422)) {
		csibdg_ctrl.raw = ISP_RD_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL);
		csibdg_ctrl.bits.RESET_MODE		= 0;
		csibdg_ctrl.bits.ABORT_MODE		= 0;
		csibdg_ctrl.bits.CSI_MODE		= 1;
		csibdg_ctrl.bits.CSI_IN_FORMAT		= 1;
		csibdg_ctrl.bits.CSI_IN_YUV_FORMAT	= 0;
		csibdg_ctrl.bits.Y_ONLY			= 0;
		csibdg_ctrl.bits.YUV2BAY_ENABLE		= (ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_ISP) ?
								ctx->is_3dnr_on : 0;
		csibdg_ctrl.bits.CH_NUM			= chn_num;
		csibdg_ctrl.bits.MULTI_CH_FRAME_SYNC_EN	= 0;
		ISP_WR_REG(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, csibdg_ctrl.raw);

	} else if (chn_num > 0 && ctx->isp_pipe_cfg[raw_num].infMode >= VI_MODE_BT656 &&
		ctx->isp_pipe_cfg[raw_num].infMode <= VI_MODE_BT1120_INTERLEAVED) {
		csibdg = ctx->phys_regs[ISP_BLK_ID_CSIBDG0_LITE];
	}

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, VS_MODE,
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync);

	if (!_is_all_online(ctx)) // sensor->fe->dram
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, true);
	else // sensor->fe->yuvtop
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_WR_ENABLE, false);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH1_DMA_WR_ENABLE, (chn_num > 0) ? true : false);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH2_DMA_WR_ENABLE, (chn_num > 1) ? true : false);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH3_DMA_WR_ENABLE, (chn_num > 2) ? true : false);

	if (ctx->isp_pipe_cfg[raw_num].is_422_to_420) {
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH0_DMA_420_WR_ENABLE, true);
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_TOP_CTRL, CH1_DMA_420_WR_ENABLE,
									(chn_num > 0) ? true : false);
		//only chn0 support avg mode
		ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CSI_BDG_DMA_DPCM_MODE, AVG_MODE, true);
	}

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH0_SIZE, CH0_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH1_SIZE, CH1_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH2_SIZE, CH2_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);

	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_WIDTHM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_width - 1);
	ISP_WR_BITS(csibdg, REG_ISP_CSI_BDG_T, CH3_SIZE, CH3_FRAME_HEIGHTM1,
					ctx->isp_pipe_cfg[raw_num].csibdg_height - 1);
}

void ispblk_rgbmap_dma_mode(struct isp_ctx *ctx, uint32_t dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union REG_ISP_DMA_CTL_SYS_CONTROL sys_ctrl;

	//1: SW mode: config by SW 0: HW mode: auto config by HW
	sys_ctrl.raw = ISP_RD_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL);
	sys_ctrl.bits.BASE_SEL		= 0x1;
	sys_ctrl.bits.STRIDE_SEL	= 0x1;
	sys_ctrl.bits.SEGLEN_SEL	= 0x1;
	sys_ctrl.bits.SEGNUM_SEL	= 0x1;
	ISP_WR_REG(dmab, REG_ISP_DMA_CTL_T, SYS_CONTROL, sys_ctrl.raw);
}

int ispblk_rgbmap_config(struct isp_ctx *ctx, int map_id, bool en)
{
	int id = rgbmap_find_hwid(map_id);
	uintptr_t map;

	if (id < 0)
		return -EINVAL;

	map = ctx->phys_regs[id];
	ISP_WR_BITS(map, REG_ISP_RGBMAP_T, RGBMAP_0, RGBMAP_ENABLE, en);

	return 0;
}

void ispblk_tnr_rgbmap_chg(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, const u8 chn_num)
{
	uintptr_t preraw_fe;
	u32 fe_id, dma_id;

	if (g_rgbmap_chg_pre[raw_num][chn_num] == false)
		return;

	fe_id = fe_find_hwid(raw_num);
	dma_id = rgbmap_dma_find_hwid(raw_num,
			(chn_num == ISP_FE_CH0) ? ISP_RAW_PATH_LE : ISP_RAW_PATH_SE);
	preraw_fe = ctx->phys_regs[fe_id];

	if (chn_num == ISP_FE_CH0) {
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER,
					LE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);

		ispblk_rgbmap_dma_config(ctx, raw_num, dma_id);
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_LE);
			ispblk_rgbmap_dma_config(ctx, raw_num + 1, dma_id);
		}
	} else {
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_H_GRID_SIZE, g_w_bit[raw_num]);
		ISP_WR_BITS(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER,
					SE_RGBMP_V_GRID_SIZE, g_h_bit[raw_num]);

		ispblk_rgbmap_dma_config(ctx, raw_num, dma_id);
		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			dma_id = rgbmap_dma_find_hwid(raw_num + 1, ISP_RAW_PATH_SE);
			ispblk_rgbmap_dma_config(ctx, raw_num + 1, dma_id);
		}
	}

	g_rgbmap_chg_pre[raw_num][chn_num] = false;
}

struct isp_grid_s_info ispblk_rgbmap_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	struct isp_grid_s_info ret;

	ret.w_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_H_GRID_SIZE);
	ret.h_bit = ISP_RD_BITS(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, LE_RGBMP_V_GRID_SIZE);

	return ret;
}
