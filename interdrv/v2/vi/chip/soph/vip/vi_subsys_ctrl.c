#include <vip/vi_drv.h>

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern uint8_t g_w_bit[ISP_PRERAW_MAX], g_h_bit[ISP_PRERAW_MAX];
extern struct lmap_cfg g_lmp_cfg[ISP_PRERAW_MAX];

/*******************************************************************************
 *	Subsys config
 ******************************************************************************/
void ispblk_splt_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num, bool enable)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_SPLT];
	union REG_ISP_LINE_SPLITER_SEL_CTRL sel_ctrl;
	union REG_ISP_LINE_SPLITER_DMA_CTRL dma_ctrl;
	uint32_t width = ctx->isp_pipe_cfg[raw_num].max_width;
	uint32_t height = ctx->isp_pipe_cfg[raw_num].max_height;
	uint32_t width_l, width_r;

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, ENABLE, LINE_SPLITER_ENABLE, enable);

	if (!enable) {
		return;
	}

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_FE0, IMG_IN_W_M1, width - 1);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_FE0, IMG_IN_H_M1, height - 1);

	if (ctx->isp_pipe_cfg[raw_num].is_tile) {
		width_l = ctx->isp_pipe_cfg[raw_num].csibdg_width;
		width_r = width - width_l;
		// left
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_BLD, BLD_W_STR, 0);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_BLD, BLD_W_END, width_l - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_BLD, IMG_WIDTH_B, width_l - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_BLD, IMG_HEIGHT_B, height - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE0, FRAME_WIDTHM1_FE0, width_l - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE0, FRAME_HEIGHTM1_FE0, height - 1);
		// right
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_FE1, IMG_WIDTH_FE1, width_r - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_FE1, IMG_HEIGHT_FE1, height - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_NBLD, NBLD_W_STR, width_l);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_NBLD, NBLD_W_END, width - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_NBLD, IMG_WIDTH_NB, width_r - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_NBLD, IMG_HEIGHT_NB, height - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE1, FRAME_WIDTHM1_FE1, width_r - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE1, FRAME_HEIGHTM1_FE1, height - 1);
	} else {
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_BLD, BLD_W_STR, 0);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_WIDTH_BLD, BLD_W_END, width - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_BLD, IMG_WIDTH_B, width - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, IMG_SIZE_BLD, IMG_HEIGHT_B, height - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE0, FRAME_WIDTHM1_FE0, width - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE0, FRAME_HEIGHTM1_FE0, height - 1);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE1, FRAME_WIDTHM1_FE1, 0);
		ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_SIZE_FE1, FRAME_HEIGHTM1_FE1, 0);
	}

	sel_ctrl.raw = 0;
	dma_ctrl.raw = 0;
	if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe) {
		sel_ctrl.bits.FE0_SEL		 = 0;
		sel_ctrl.bits.FE1_SEL		 = 0;
		sel_ctrl.bits.FE0_RDMA_SEL	 = 1;
		sel_ctrl.bits.FE1_RDMA_SEL	 = ctx->isp_pipe_cfg[raw_num].is_tile;

		dma_ctrl.bits.RDMA_L_ENABLE_FE0	 = 1;
		dma_ctrl.bits.RDMA_S_ENABLE_FE0	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		dma_ctrl.bits.RDMA_L_ENABLE_FE1	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.RDMA_S_ENABLE_FE1	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						   ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else if (ctx->isp_pipe_cfg[raw_num].is_raw_ai_isp) {
		sel_ctrl.bits.FE0_SEL		 = 0;
		sel_ctrl.bits.FE1_SEL		 = ctx->isp_pipe_cfg[raw_num].is_tile;
		sel_ctrl.bits.FE0_RDMA_SEL	 = 1;
		sel_ctrl.bits.FE1_RDMA_SEL	 = ctx->isp_pipe_cfg[raw_num].is_tile;

		dma_ctrl.bits.FE0_WDMA_L_ENABLE	 = 1;
		dma_ctrl.bits.FE0_WDMA_S_ENABLE	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		dma_ctrl.bits.FE1_WDMA_L_ENABLE	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.FE1_WDMA_S_ENABLE	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						   ctx->isp_pipe_cfg[raw_num].is_hdr_on;

		dma_ctrl.bits.RDMA_L_ENABLE_FE0	 = 1;
		dma_ctrl.bits.RDMA_S_ENABLE_FE0	 = ctx->isp_pipe_cfg[raw_num].is_hdr_on;
		dma_ctrl.bits.RDMA_L_ENABLE_FE1	 = ctx->isp_pipe_cfg[raw_num].is_tile;
		dma_ctrl.bits.RDMA_S_ENABLE_FE1	 = ctx->isp_pipe_cfg[raw_num].is_tile &&
						   ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	} else {
		sel_ctrl.bits.FE0_SEL		 = 0;
		sel_ctrl.bits.FE1_SEL		 = ctx->isp_pipe_cfg[raw_num].is_tile;
		sel_ctrl.bits.FE0_RDMA_SEL	 = 0;
		sel_ctrl.bits.FE1_RDMA_SEL	 = 0;
	}
	ISP_WR_REG(ba, REG_ISP_LINE_SPLITER_T, SEL_CTRL, sel_ctrl.raw);
	ISP_WR_REG(ba, REG_ISP_LINE_SPLITER_T, DMA_CTRL, dma_ctrl.raw);

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, YUV_MODE, YUV_FORMAT_FE0,
			ctx->isp_pipe_cfg[raw_num].is_yuv_sensor);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, YUV_MODE, YUV_FORMAT_FE1,
			ctx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, HDR_CTRL, HDR_FE0,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, HDR_CTRL, HDR_FE1,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, LESE_ARBITER_CTRL, CH_NUM_FE0,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, LESE_ARBITER_CTRL, CH_NUM_FE1,
			ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, VS_SW_CTRL, FE0_VS_SEL,
			ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, VS_SW_CTRL, FE1_VS_SEL,
			ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe &&
			ctx->isp_pipe_cfg[raw_num].is_tile);

	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_VLD_CTRL, VS_MODE_FE0,
			ctx->isp_pipe_cfg[raw_num].is_stagger_vsync);
	ISP_WR_BITS(ba, REG_ISP_LINE_SPLITER_T, FRAME_VLD_CTRL, VS_MODE_FE1, 0);
}

void ispblk_splt_wdma_ctrl_config(struct isp_ctx *ctx, enum ISP_BLK_ID_T blk_id, bool enable)
{
	uintptr_t wdma = ctx->phys_regs[blk_id];
	union REG_PRE_WDMA_CTRL wdma_ctrl;
	enum cvi_isp_raw raw_num = ISP_PRERAW0;

	if (blk_id == ISP_BLK_ID_SPLT_FE1_WDMA)
		raw_num = ISP_PRERAW1;

	wdma_ctrl.raw = ISP_RD_REG(wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL);
	wdma_ctrl.bits.WDMI_EN_LE = enable;
	wdma_ctrl.bits.WDMI_EN_SE = enable && ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	ISP_WR_REG(wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL, wdma_ctrl.raw);

	// just ai_isp need write to dram, and always no used compress mode.
	ISP_WR_BITS(wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_MODE, 0);
	ISP_WR_BITS(wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_XSTR, 0);
}

void ispblk_splt_rdma_ctrl_config(struct isp_ctx *ctx, enum ISP_BLK_ID_T blk_id, bool enable)
{
	uintptr_t rdma = ctx->phys_regs[blk_id];
	union REG_RAW_RDMA_CTRL_CONFIG ctrl_config;
	union REG_RAW_RDMA_CTRL_RDMA_SIZE rdma_size;
	enum cvi_isp_raw raw_num = ISP_PRERAW0;
	uint32_t width, height;

	if ((blk_id == ISP_BLK_ID_SPLT_FE1_RDMA_LE) || (blk_id == ISP_BLK_ID_SPLT_FE1_RDMA_SE))
		raw_num = ISP_PRERAW1;

	width = ctx->isp_pipe_cfg[raw_num].csibdg_width;
	height = ctx->isp_pipe_cfg[raw_num].csibdg_height;

	ctrl_config.raw = 0;
	ctrl_config.bits.LE_RDMA_EN = enable;
	// ctrl_config.bits.SE_RDMA_EN = enable; // unused
	ISP_WR_REG(rdma, REG_RAW_RDMA_CTRL_T, CONFIG, ctrl_config.raw);

	rdma_size.raw = 0;
	rdma_size.bits.RDMI_WIDTHM1 = width - 1;
	rdma_size.bits.RDMI_HEIGHTM1 = height - 1;
	ISP_WR_REG(rdma, REG_RAW_RDMA_CTRL_T, RDMA_SIZE, rdma_size.raw);

	if (ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_replay_fe) {
		if (ctx->is_dpcm_on) {
			ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 7);
			ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	} else if (ctx->isp_pipe_cfg[ISP_PRERAW0].is_raw_ai_isp) {
		ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 0);
		ISP_WR_BITS(rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 0);
	}
}

void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	uint32_t width = ctx->isp_pipe_cfg[raw_num].crop.w;
	uint32_t height = ctx->isp_pipe_cfg[raw_num].crop.h;
	union REG_PRE_RAW_FE_PRE_RAW_CTRL raw_ctrl;
	union REG_PRE_RAW_FE_PRE_RAW_FRAME_SIZE  frm_size;
	union REG_PRE_RAW_FE_LE_RGBMAP_GRID_NUMBER  rgbmap_le;
	union REG_PRE_RAW_FE_SE_RGBMAP_GRID_NUMBER  rgbmap_se;

	frm_size.raw = rgbmap_le.raw = rgbmap_se.raw = 0;

	raw_ctrl.raw = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL);
	raw_ctrl.bits.BAYER_TYPE_LE = ctx->rgb_color_mode[raw_num];
	raw_ctrl.bits.BAYER_TYPE_SE = ctx->rgb_color_mode[raw_num];
	raw_ctrl.bits.POST_BLC_BAYER_TYPE_LE = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_ctrl.bits.POST_BLC_BAYER_TYPE_SE = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_ctrl.bits.RGBIR_EN = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_CTRL, raw_ctrl.raw);

	frm_size.bits.FRAME_WIDTHM1 = width - 1;
	frm_size.bits.FRAME_HEIGHTM1 = height - 1;
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_FRAME_SIZE, frm_size.raw);

	rgbmap_le.bits.LE_RGBMP_H_GRID_SIZE = g_w_bit[raw_num];
	rgbmap_le.bits.LE_RGBMP_V_GRID_SIZE = g_h_bit[raw_num];
	rgbmap_se.bits.SE_RGBMP_H_GRID_SIZE = g_w_bit[raw_num];
	rgbmap_se.bits.SE_RGBMP_V_GRID_SIZE = g_h_bit[raw_num];
#if 0 //only grid size need to program in lmap/rgbmap hw mode
	w_grid_num = UPPER(width, g_w_bit[raw_num]) - 1;
	h_grid_num = UPPER(height, g_h_bit[raw_num]) - 1;

	rgbmap_le.bits.LE_RGBMP_H_GRID_NUMM1 = w_grid_num;
	rgbmap_le.bits.LE_RGBMP_V_GRID_NUMM1 = h_grid_num;
	rgbmap_se.bits.SE_RGBMP_H_GRID_NUMM1 = w_grid_num;
	rgbmap_se.bits.SE_RGBMP_V_GRID_NUMM1 = h_grid_num;
#endif
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, LE_RGBMAP_GRID_NUMBER, rgbmap_le.raw);
	ISP_WR_REG(preraw_fe, REG_PRE_RAW_FE_T, SE_RGBMAP_GRID_NUMBER, rgbmap_se.raw);
}

void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	union REG_PRE_RAW_VI_SEL_1 vi_sel_1;

	vi_sel_1.raw = 0;
	vi_sel_1.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	vi_sel_1.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_1, vi_sel_1.raw);

	if (_is_be_post_online(ctx) && ctx->is_dpcm_on) { // dram->be
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 7);
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 8191);
	} else {
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DMA_LD_DPCM_MODE, 0);
		ISP_WR_BITS(vi_sel, REG_PRE_RAW_VI_SEL_T, REG_0, DPCM_RX_XSTR, 0);
	}
}

void ispblk_pre_wdma_ctrl_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t pre_wdma = ctx->phys_regs[ISP_BLK_ID_PRE_WDMA];
	union REG_PRE_WDMA_CTRL wdma_ctrl;

	wdma_ctrl.raw = ISP_RD_REG(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL);
	wdma_ctrl.bits.WDMI_EN_LE = ctx->is_offline_postraw;
	wdma_ctrl.bits.WDMI_EN_SE = (ctx->is_offline_postraw && ctx->isp_pipe_cfg[raw_num].is_hdr_on);
	ISP_WR_REG(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL, wdma_ctrl.raw);

	// NOTE: for be->dram, 'PRE_RAW_BE_RDMI_DPCM' naming is misleading
	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { // be->dram
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_MODE, 7);
			// 1 if dpcm_mode 7; 0 if dpcm_mode 5
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_WDMA_CTRL, DMA_WR_MSB, 1);
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_MODE, 0);
			ISP_WR_BITS(pre_wdma, REG_PRE_WDMA_CTRL_T, PRE_RAW_BE_RDMI_DPCM, DPCM_XSTR, 0);
		}
	}
}

void ispblk_preraw_be_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	union REG_PRE_RAW_BE_TOP_CTRL top_ctrl;
	union REG_PRE_RAW_BE_IMG_SIZE_LE img_size;

	top_ctrl.raw = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL);
	top_ctrl.bits.BAYER_TYPE_LE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.BAYER_TYPE_SE	= ctx->rgb_color_mode[raw_num];
	top_ctrl.bits.RGBIR_EN		= ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	top_ctrl.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;
	top_ctrl.bits.POST_RGBIR_BAYER_TYPE_LE	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	top_ctrl.bits.POST_RGBIR_BAYER_TYPE_SE	= bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, TOP_CTRL, top_ctrl.raw);

	img_size.raw = 0;
	img_size.bits.FRAME_WIDTHM1 = ctx->img_width - 1;
	img_size.bits.FRAME_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(preraw_be, REG_PRE_RAW_BE_T, IMG_SIZE_LE, img_size.raw);

	ISP_WO_BITS(preraw_be, REG_PRE_RAW_BE_T, UP_PQ_EN, UP_PQ_EN, 1);
	ISP_WR_BITS(preraw_be, REG_PRE_RAW_BE_T, DEBUG_ENABLE, DEBUG_EN, 1);
}

void ispblk_raw_rdma_ctrl_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num,
					enum ISP_BLK_ID_T blk_id, bool enable)
{
	uintptr_t raw_rdma = ctx->phys_regs[blk_id];
	union REG_RAW_RDMA_CTRL_CONFIG ctrl_config;
	union REG_RAW_RDMA_CTRL_RDMA_SIZE rdma_size;

	ctrl_config.raw = 0;
	ctrl_config.bits.LE_RDMA_EN = enable;
	// ctrl_config.bits.SE_RDMA_EN = enable; // unused
	ISP_WR_REG(raw_rdma, REG_RAW_RDMA_CTRL_T, CONFIG, ctrl_config.raw);

	rdma_size.raw = 0;
	rdma_size.bits.RDMI_WIDTHM1 = ctx->img_width - 1;
	rdma_size.bits.RDMI_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(raw_rdma, REG_RAW_RDMA_CTRL_T, RDMA_SIZE, rdma_size.raw);

	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on &&
			!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //dram->post
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 7);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(raw_rdma, REG_RAW_RDMA_CTRL_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	}
}

void ispblk_rawtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rawtop = ctx->phys_regs[ISP_BLK_ID_RAWTOP];
	union REG_RAW_TOP_RAW_2 raw_2;
	union REG_RAW_TOP_RAW_BAYER_TYPE_TOPLEFT raw_bayer_type_topleft;
	union REG_RAW_TOP_RDMI_ENABLE rdmi_enable;
	union REG_RAW_TOP_LE_LMAP_GRID_NUMBER   le_lmap_size;
	union REG_RAW_TOP_SE_LMAP_GRID_NUMBER   se_lmap_size;

	raw_2.raw = 0;
	raw_2.bits.IMG_WIDTHM1 = ctx->img_width - 1;
	raw_2.bits.IMG_HEIGHTM1 = ctx->img_height - 1;
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, RAW_2, raw_2.raw);

	raw_bayer_type_topleft.raw = 0;
	raw_bayer_type_topleft.bits.BAYER_TYPE_TOPLEFT = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	raw_bayer_type_topleft.bits.RGBIR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, RAW_BAYER_TYPE_TOPLEFT, raw_bayer_type_topleft.raw);

	rdmi_enable.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE);
	rdmi_enable.bits.CH_NUM		= ctx->isp_pipe_cfg[raw_num].is_hdr_on;

	ISP_WR_REG(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, rdmi_enable.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 1);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 1);
	} else {
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, CTRL, LS_CROP_DST_SEL, 0);
		ISP_WO_BITS(rawtop, REG_RAW_TOP_T, RAW_4, YUV_IN_MODE, 0);
	}
#if 0
	if (_is_fe_be_online(ctx)) { // fe->be->dram->post single sensor frame_base/slice buffer
		union REG_RAW_TOP_RDMA_SIZE rdma_size;

		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 1);
		rdma_size.raw = 0;
		rdma_size.bits.RDMI_WIDTHM1 = ctx->img_width - 1;
		rdma_size.bits.RDMI_HEIGHTM1 = ctx->img_height - 1;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, RDMA_SIZE, rdma_size.raw);
	} else { //fe->dram->be->post (2/3 sensors) or fe->be->post (onthefly)
		ISP_WR_BITS(rawtop, REG_RAW_TOP_T, RDMI_ENABLE, RDMI_EN, 0);
	}
#endif
	if (!ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) {
		if (_is_fe_be_online(ctx) && ctx->is_dpcm_on) { //dram->post
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 7);
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 8191);
		} else {
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_MODE, 0);
			ISP_WR_BITS(rawtop, REG_RAW_TOP_T, DPCM_MODE, DPCM_XSTR, 0);
		}
	}

	le_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER);
	le_lmap_size.bits.LE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
	le_lmap_size.bits.LE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
	ISP_WR_REG(rawtop, REG_RAW_TOP_T, LE_LMAP_GRID_NUMBER, le_lmap_size.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
		se_lmap_size.raw = ISP_RD_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER);
		se_lmap_size.bits.SE_LMP_H_GRID_SIZE = g_lmp_cfg[raw_num].post_w_bit;
		se_lmap_size.bits.SE_LMP_V_GRID_SIZE = g_lmp_cfg[raw_num].post_h_bit;
		ISP_WR_REG(rawtop, REG_RAW_TOP_T, SE_LMAP_GRID_NUMBER, se_lmap_size.raw);
	}
}

void ispblk_rgbtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	union REG_ISP_RGB_TOP_0 reg_0;
	union REG_ISP_RGB_TOP_9 reg_9;

	reg_0.raw = reg_9.raw = 0;

	reg_0.raw = ISP_RD_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_0);
	reg_0.bits.RGBTOP_BAYER_TYPE = bayer_type_mapping(ctx->rgb_color_mode[raw_num]);
	reg_0.bits.RGBTOP_RGBIR_ENABLE = ctx->isp_pipe_cfg[raw_num].is_rgbir_sensor;
	ISP_WR_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_0, reg_0.raw);

	reg_9.bits.RGBTOP_IMGW_M1 = ctx->img_width - 1;
	reg_9.bits.RGBTOP_IMGH_M1 = ctx->img_height - 1;
	ISP_WR_REG(rgbtop, REG_ISP_RGB_TOP_T, REG_9, reg_9.raw);
	ISP_WR_BITS(rgbtop, REG_ISP_RGB_TOP_T, DBG_IP_S_VLD, IP_DBG_EN, 1);
}

void ispblk_yuvtop_config(struct isp_ctx *ctx, const enum cvi_isp_raw raw_num)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	union REG_YUV_TOP_IMGW_M1 imgw_m1;
	union REG_YUV_TOP_AI_ISP_RDMA_CTRL ai_isp_ctrl;
	uint32_t width = ctx->img_width;
	uint32_t height = ctx->img_height;

	imgw_m1.raw = 0;
	ai_isp_ctrl.raw = 0;

	ISP_WO_BITS(yuvtop, REG_YUV_TOP_T, YUV_3, YONLY_EN, ctx->isp_pipe_cfg[raw_num].is_yuv_sensor);

	imgw_m1.bits.YUV_TOP_IMGW_M1 = width - 1;
	imgw_m1.bits.YUV_TOP_IMGH_M1 = height - 1;
	ISP_WR_REG(yuvtop, REG_YUV_TOP_T, IMGW_M1, imgw_m1.raw);

	if (ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp) {
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_IMG_SIZE_Y, AI_ISP_IMG_WIDTH_CROP_0, width - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_IMG_SIZE_Y, AI_ISP_IMG_HEIGHT_CROP_0, height - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_W_CROP_Y, AI_ISP_CROP_W_STR_0, 0);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_W_CROP_Y, AI_ISP_CROP_W_END_0, width - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_H_CROP_Y, AI_ISP_CROP_H_STR_0, 0);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_H_CROP_Y, AI_ISP_CROP_H_END_0, height - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_IMG_SIZE_UV, AI_ISP_IMG_WIDTH_CROP_1, (width >> 1) - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_IMG_SIZE_UV, AI_ISP_IMG_HEIGHT_CROP_1, height - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_W_CROP_UV, AI_ISP_CROP_W_STR_1, 0);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_W_CROP_UV, AI_ISP_CROP_W_END_1, (width >> 1) - 1);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_H_CROP_UV, AI_ISP_CROP_H_STR_1, 0);
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, AI_ISP_H_CROP_UV, AI_ISP_CROP_H_END_1, height - 1);

		ai_isp_ctrl.bits.AI_ISP_RDMA_ENABLE = ctx->isp_pipe_cfg[raw_num].is_tnr_ai_isp_rdy ? 0x7 : 0x0;
		ai_isp_ctrl.bits.AI_ISP_CROP_ENABLE = false;
		ai_isp_ctrl.bits.AI_ISP_ENABLE      = true;
		ai_isp_ctrl.bits.AI_ISP_MASK        = true;
		ISP_WR_REG(yuvtop, REG_YUV_TOP_T, AI_ISP_RDMA_CTRL, ai_isp_ctrl.raw);
	} else {
		ISP_WR_REG(yuvtop, REG_YUV_TOP_T, AI_ISP_RDMA_CTRL, 0);
	}

	if (_is_all_online(ctx) && !ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
		//bypass_v = 1 -> 422P online to scaler
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_CTRL, BYPASS_V, 1);
	} else {
		ISP_WR_BITS(yuvtop, REG_YUV_TOP_T, YUV_CTRL, BYPASS_V,
					!ctx->isp_pipe_cfg[raw_num].is_offline_scaler);
	}
}

void ispblk_isptop_config(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uint8_t pre_fe_trig_by_hw[ISP_PRERAW_MAX] = { 0 };
	uint8_t pre_be_trig_by_hw = 0;
	uint8_t post_trig_by_hw = 0;
	uint8_t first_raw_num = vi_get_first_raw_num(ctx);
	enum cvi_isp_raw raw_num = ISP_PRERAW0;

	union REG_ISP_TOP_INT_EVENT0_EN ev0_en;
	union REG_ISP_TOP_INT_EVENT1_EN ev1_en;
	union REG_ISP_TOP_INT_EVENT2_EN ev2_en;
	union REG_ISP_TOP_INT_EVENT0_EN_FE345 ev0_en_fe345;
	union REG_ISP_TOP_INT_EVENT1_EN_FE345 ev1_en_fe345;
	union REG_ISP_TOP_INT_EVENT2_EN_FE345 ev2_en_fe345;
	union REG_ISP_TOP_CTRL_MODE_SEL0 trig_sel0;
	union REG_ISP_TOP_CTRL_MODE_SEL1 trig_sel1;
	union REG_ISP_TOP_CTRL_MODE_SEL0_FE345 trig_sel0_fe345;
	union REG_ISP_TOP_CTRL_MODE_SEL1_FE345 trig_sel1_fe345;
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;

	ev0_en.raw = ev1_en.raw = ev2_en.raw = 0;
	ev0_en_fe345.raw = ev1_en_fe345.raw = ev2_en_fe345.raw = 0;
	trig_sel0.raw = trig_sel1.raw = 0;
	trig_sel0_fe345.raw = trig_sel1_fe345.raw = 0;
	scene_ctrl.raw = 0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_be) { //RAW replay
			pre_fe_trig_by_hw[raw_num] = 0x0;
		} else if (!ctx->isp_pipe_cfg[raw_num].is_yuv_sensor) { //RGB sensor
			if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on)
				pre_fe_trig_by_hw[raw_num] = 0x1;
			else
				pre_fe_trig_by_hw[raw_num] = 0x3;
		} else { //YUV sensor
			switch (ctx->isp_pipe_cfg[raw_num].muxMode) {
			case VI_WORK_MODE_1Multiplex:
				pre_fe_trig_by_hw[raw_num] = 0x1;
				break;
			case VI_WORK_MODE_2Multiplex:
				pre_fe_trig_by_hw[raw_num] = 0x3;
				break;
			case VI_WORK_MODE_3Multiplex:
				pre_fe_trig_by_hw[raw_num] = 0x7;
				break;
			case VI_WORK_MODE_4Multiplex:
				pre_fe_trig_by_hw[raw_num] = 0xF;
				break;
			default:
				break;
			}
		}
	}

	if (ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be)
		pre_be_trig_by_hw = 0x0;
	else if (ctx->is_offline_be)
		pre_be_trig_by_hw = 0x0;
	else { //be online, on the fly mode or fe_A->be
		if (ctx->isp_pipe_cfg[first_raw_num].is_yuv_sensor)
			pre_be_trig_by_hw = 0x0;
		else { //Single RGB sensor
			if (ctx->is_hdr_on)
				pre_be_trig_by_hw = 0x3;
			else
				pre_be_trig_by_hw = 0x1;
		}
	}

	// fly mode or single sensor and slice buffer mode on. post trigger by vsync
	if ((ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be == 0) &&
		(_is_all_online(ctx) || (_is_fe_be_online(ctx) && ctx->is_slice_buf_on)))
		post_trig_by_hw = 0x1;
	else //trigger by SW
		post_trig_by_hw = 0x0;

	//line_spliter intr ctrl, now no use dma intr, close intr enable
	ev0_en_fe345.bits.LINE_SPLITER_DMA_DONE_ENABLE_FE0	= 0;// 0xF
	ev0_en_fe345.bits.LINE_SPLITER_DMA_DONE_ENABLE_FE1	= 0;// 0xF
	ev2_en.bits.FRAME_ERR_LINE_SPLITER_ENABLE_FE0		= 0;// 0x1
	ev2_en.bits.FRAME_ERR_LINE_SPLITER_ENABLE_FE1		= 0;// 0x1

	//pre_fe0
	ev0_en.bits.FRAME_DONE_ENABLE_FE0	= 0xF;
	ev2_en.bits.FRAME_START_ENABLE_FE0	= 0xF;
	trig_sel0.bits.TRIG_STR_SEL_FE0		= pre_fe_trig_by_hw[ISP_PRERAW0];
	trig_sel0.bits.SHAW_UP_SEL_FE0		= pre_fe_trig_by_hw[ISP_PRERAW0];
	trig_sel1.bits.PQ_UP_SEL_FE0		= pre_fe_trig_by_hw[ISP_PRERAW0];

	//pre_fe1
	ev0_en.bits.FRAME_DONE_ENABLE_FE1	= 0xF;
	ev2_en.bits.FRAME_START_ENABLE_FE1	= 0xF;
	trig_sel0.bits.TRIG_STR_SEL_FE1		= pre_fe_trig_by_hw[ISP_PRERAW1];
	trig_sel0.bits.SHAW_UP_SEL_FE1		= pre_fe_trig_by_hw[ISP_PRERAW1];
	trig_sel1.bits.PQ_UP_SEL_FE1		= pre_fe_trig_by_hw[ISP_PRERAW1];

	//pre_fe2
	ev0_en.bits.FRAME_DONE_ENABLE_FE2	= 0x3;
	ev2_en.bits.FRAME_START_ENABLE_FE2	= 0x3;
	trig_sel0.bits.TRIG_STR_SEL_FE2		= pre_fe_trig_by_hw[ISP_PRERAW2];
	trig_sel0.bits.SHAW_UP_SEL_FE2		= pre_fe_trig_by_hw[ISP_PRERAW2];
	trig_sel1.bits.PQ_UP_SEL_FE2		= pre_fe_trig_by_hw[ISP_PRERAW2];

	//pre_fe3
	ev0_en_fe345.bits.FRAME_DONE_ENABLE_FE3	 = 0x3;
	ev2_en_fe345.bits.FRAME_START_ENABLE_FE3 = 0x3;
	trig_sel0_fe345.bits.TRIG_STR_SEL_FE3	 = pre_fe_trig_by_hw[ISP_PRERAW3];
	trig_sel0_fe345.bits.SHAW_UP_SEL_FE3	 = pre_fe_trig_by_hw[ISP_PRERAW3];
	trig_sel1_fe345.bits.PQ_UP_SEL_FE3	 = pre_fe_trig_by_hw[ISP_PRERAW3];

	//pre_fe4
	ev0_en_fe345.bits.FRAME_DONE_ENABLE_FE4	 = 0x3;
	ev2_en_fe345.bits.FRAME_START_ENABLE_FE4 = 0x3;
	trig_sel0_fe345.bits.TRIG_STR_SEL_FE4	 = pre_fe_trig_by_hw[ISP_PRERAW4];
	trig_sel0_fe345.bits.SHAW_UP_SEL_FE4	 = pre_fe_trig_by_hw[ISP_PRERAW4];
	trig_sel1_fe345.bits.PQ_UP_SEL_FE4	 = pre_fe_trig_by_hw[ISP_PRERAW4];

	//pre_fe5
	ev0_en_fe345.bits.FRAME_DONE_ENABLE_FE5	 = 0x3;
	ev2_en_fe345.bits.FRAME_START_ENABLE_FE5 = 0x3;
	trig_sel0_fe345.bits.TRIG_STR_SEL_FE5	 = pre_fe_trig_by_hw[ISP_PRERAW5];
	trig_sel0_fe345.bits.SHAW_UP_SEL_FE5	 = pre_fe_trig_by_hw[ISP_PRERAW5];
	trig_sel1_fe345.bits.PQ_UP_SEL_FE5	 = pre_fe_trig_by_hw[ISP_PRERAW5];

	//pre_be
	ev0_en.bits.FRAME_DONE_ENABLE_BE	= 0x3;
	trig_sel0.bits.TRIG_STR_SEL_BE		= pre_be_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_BE		= pre_be_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_BE		= pre_be_trig_by_hw;

	//postraw
	ev0_en.bits.FRAME_DONE_ENABLE_POST	= 0x1;
	ev0_en.bits.SHAW_DONE_ENABLE_POST	= 0x1;
	trig_sel0.bits.TRIG_STR_SEL_RAW		= post_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_RAW		= post_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_RAW		= post_trig_by_hw;
	trig_sel0.bits.TRIG_STR_SEL_POST	= post_trig_by_hw;
	trig_sel0.bits.SHAW_UP_SEL_POST		= post_trig_by_hw;
	trig_sel1.bits.PQ_UP_SEL_POST		= post_trig_by_hw;

	//csibdg_lite
	ev2_en.bits.INT_BDG0_LITE_ENABLE	= 0x1;
	ev2_en.bits.INT_BDG1_LITE_ENABLE	= 0x1;
	ev2_en.bits.CMDQ_INT_ENABLE		= 0x1;

	//err int
	ev2_en.bits.FRAME_ERR_ENABLE		= 0x1;
	ev2_en.bits.INT_DMA_ERR_ENABLE		= 0x1;

	//scenario_ctrl mode
	//single sensor
	scene_ctrl.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL);
	scene_ctrl.bits.PRE2BE_L_ENABLE		= !ctx->is_offline_be;
	scene_ctrl.bits.PRE2BE_S_ENABLE		= !ctx->is_offline_be && ctx->is_hdr_on;
	scene_ctrl.bits.PRE2YUV_422_ENABLE	= 0;
	//Multi sensors
	scene_ctrl.bits.BE2RAW_L_ENABLE		= !ctx->is_offline_postraw;
	scene_ctrl.bits.BE2RAW_S_ENABLE		= !ctx->is_offline_postraw && ctx->is_hdr_on;
	// Multi sensors or raw replay
	scene_ctrl.bits.BE_RDMA_L_ENABLE	= ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be;
	scene_ctrl.bits.BE_RDMA_S_ENABLE	= (ctx->is_offline_be ||
						  ctx->isp_pipe_cfg[first_raw_num].is_raw_replay_be) &&
						  ctx->is_hdr_on;
	// onthefly -> 0 or single sensor
	scene_ctrl.bits.BE_WDMA_L_ENABLE	= ctx->is_offline_postraw;
	scene_ctrl.bits.BE_WDMA_S_ENABLE	= ctx->is_offline_postraw && ctx->is_hdr_on;
	scene_ctrl.bits.AF_RAW0YUV1		= 0; //0 from be/1 from post
	scene_ctrl.bits.RGBMP_ONLINE_L_ENABLE	= _is_all_online(ctx); //onthefly -> 1
	//single sensor hdr and slice buffer mode on
	scene_ctrl.bits.RGBMP_ONLINE_S_ENABLE	= 0;
	scene_ctrl.bits.RAW2YUV_422_ENABLE	= 0;

#if defined(__CV180X__)
	scene_ctrl.bits.HDR_ENABLE		= ctx->is_hdr_on;
#else
	//In order for linearMode use guideWeight
	scene_ctrl.bits.HDR_ENABLE		= !_is_all_online(ctx) && ctx->is_hdr_on;
#endif
	// to verify IP, turn off HW LUT of rgbgamma, ynr, and cnr.
	scene_ctrl.bits.HW_AUTO_ENABLE		= 0;
	// set the position of the beginning of YUV suggested by HW
	scene_ctrl.bits.DCI_RGB0YUV1		= 0;
	scene_ctrl.bits.BE_SRC_SEL		= first_raw_num;
	scene_ctrl.bits.DST2SC			= !ctx->isp_pipe_cfg[first_raw_num].is_offline_scaler;

#if defined(__CV186X__)
	//SW PATCH: To succeed in the next run, these IPs need to be reset.
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x6FF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x0);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST_FE345, 0x7);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST_FE345, 0x0);
#endif

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0_FE345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1_FE345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2_FE345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0_EN, ev0_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1_EN, ev1_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2_EN, ev2_en.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0_EN_FE345, ev0_en_fe345.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1_EN_FE345, ev1_en_fe345.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2_EN_FE345, ev2_en_fe345.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0, trig_sel0.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL1, trig_sel1.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0_FE345, trig_sel0_fe345.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL1_FE345, trig_sel1_fe345.raw);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);

	ISP_WR_BITS(isptopb, REG_ISP_TOP_T, DUMMY, DBUS_SEL, 4);
	//ISP_WR_REG(isptopb, REG_ISP_TOP_T, REG_1C, 7);
}

#ifdef PORTING_TEST

void ispblk_isptop_fpga_config(struct isp_ctx *ctx, uint16_t test_case)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_SCENARIOS_CTRL scene_ctrl;

	scene_ctrl.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL);
	// to verify IP, turn off HW LUT of rgbgamma, ynr, and cnr.
	if (test_case == 0) {
		scene_ctrl.bits.HW_AUTO_ENABLE		= 0;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	} else if (test_case == 1) {
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	} else if (test_case == 2) { //auto_iso
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 2;
	} else if (test_case == 0xFF) { //store default config
		scene_ctrl.bits.HW_AUTO_ENABLE		= 1;
		scene_ctrl.bits.HW_AUTO_ISO		= 0;
	}

	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SCENARIOS_CTRL, scene_ctrl.raw);
}

#endif

void isp_intr_set_mask(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT0_EN, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT1_EN, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT2_EN, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT0_EN_FE345, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT1_EN_FE345, 0);
	ISP_WR_REG(isp_top, REG_ISP_TOP_T, INT_EVENT2_EN_FE345, 0);
}

void isp_reset(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union REG_ISP_TOP_CTRL_MODE_SEL0 mode_sel0;
	union REG_ISP_TOP_CTRL_MODE_SEL0_FE345 mode_sel0_fe345;

	// disable interrupt
	isp_intr_set_mask(ctx);

	// switch back to hw trig.
	mode_sel0.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0);
	mode_sel0.bits.TRIG_STR_SEL_FE0  = 0xF;
	mode_sel0.bits.TRIG_STR_SEL_FE1  = 0xF;
	mode_sel0.bits.TRIG_STR_SEL_FE2  = 0x3;
	mode_sel0.bits.TRIG_STR_SEL_BE   = 0x3;
	mode_sel0.bits.TRIG_STR_SEL_RAW  = 0x1;
	mode_sel0.bits.TRIG_STR_SEL_POST = 0x1;
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0, mode_sel0.raw);

	mode_sel0_fe345.raw = ISP_RD_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0_FE345);
	mode_sel0_fe345.bits.TRIG_STR_SEL_FE3 = 0x3;
	mode_sel0_fe345.bits.TRIG_STR_SEL_FE4 = 0x3;
	mode_sel0_fe345.bits.TRIG_STR_SEL_FE5 = 0x3;
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, CTRL_MODE_SEL0_FE345, mode_sel0_fe345.raw);

	// reset
	//AXI_RST first, then reset other
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x80);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x6FF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST_FE345, 0x7);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x80);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST, 0x0);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, SW_RST_FE345, 0x0);

	// clear intr
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT0_FE345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT1_FE345, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, REG_ISP_TOP_T, INT_EVENT2_FE345, 0xFFFFFFFF);
}

/*******************************************************************************
 *	Subsys Debug Infomation
 ******************************************************************************/
struct _csibdg_dbg_i ispblk_csibdg_chn_dbg(
	struct isp_ctx *ctx,
	enum cvi_isp_raw raw_num,
	enum cvi_isp_fe_chn_num chn_num)
{
	int id = -1;
	uintptr_t ba = 0;
	struct _csibdg_dbg_i data;

	if (ctx->isp_pipe_cfg[raw_num].is_bt_demux) {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH0_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH0_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH0_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH0_DEBUG_3);
			break;
		case ISP_FE_CH1:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH1_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH1_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH1_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH1_DEBUG_3);
			break;
		case ISP_FE_CH2:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH2_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH2_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH2_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH2_DEBUG_3);
			break;
		case ISP_FE_CH3:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH3_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH3_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH3_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_LITE_T, CH3_DEBUG_3);
			break;
		default:
			break;
		}
	} else {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH0_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH0_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH0_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH0_DEBUG_3);
			break;
		case ISP_FE_CH1:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH1_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH1_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH1_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH1_DEBUG_3);
			break;
		case ISP_FE_CH2:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH2_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH2_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH2_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH2_DEBUG_3);
			break;
		case ISP_FE_CH3:
			data.dbg_0 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH3_DEBUG_0);
			data.dbg_1 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH3_DEBUG_1);
			data.dbg_2 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH3_DEBUG_2);
			data.dbg_3 = ISP_RD_REG(ba, REG_ISP_CSI_BDG_T, CH3_DEBUG_3);
			break;
		default:
			break;
		}

	}

	return data;
}

struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum cvi_isp_raw raw_num)
{
	int id = fe_find_hwid(raw_num);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	struct _fe_dbg_i data;

	data.fe_idle_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, PRE_RAW_DEBUG_STATE);
	data.fe_done_sts = ISP_RD_REG(preraw_fe, REG_PRE_RAW_FE_T, FE_IDLE_INFO);

	return data;
}

struct _be_dbg_i ispblk_be_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t preraw_be = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_BE];
	struct _be_dbg_i data;

	data.be_done_sts = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_INFO);
	data.be_dma_idle_sts = ISP_RD_REG(preraw_be, REG_PRE_RAW_BE_T, BE_DMA_IDLE_INFO);

	return data;
}

struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	struct _post_dbg_i data;

	data.top_sts_0 = ISP_RD_REG(isptopb, REG_ISP_TOP_T, BLK_IDLE);
	data.top_sts_1 = ISP_RD_REG(isptopb, REG_ISP_TOP_T, BLK_IDLE_1);

	return data;
}

struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com_0 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE0];
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_0 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE0];
	uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
	struct _dma_dbg_i data;

	data.wdma_0_err_sts = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, NORM_STATUS0);
	data.wdma_0_idle = ISP_RD_REG(wdma_com_0, REG_WDMA_CORE_T, NORM_STATUS1);

	data.wdma_1_err_sts = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, NORM_STATUS0);
	data.wdma_1_idle = ISP_RD_REG(wdma_com_1, REG_WDMA_CORE_T, NORM_STATUS1);

	data.wdma_2_err_sts = ISP_RD_REG(wdma_com_2, REG_WDMA_CORE_T, NORM_STATUS0);
	data.wdma_2_idle = ISP_RD_REG(wdma_com_2, REG_WDMA_CORE_T, NORM_STATUS1);

	data.wdma_3_err_sts = ISP_RD_REG(wdma_com_3, REG_WDMA_CORE_T, NORM_STATUS0);
	data.wdma_3_idle = ISP_RD_REG(wdma_com_3, REG_WDMA_CORE_T, NORM_STATUS1);

	data.rdma_0_err_sts = ISP_RD_REG(rdma_com_0, REG_RDMA_CORE_T, NORM_STATUS0);
	data.rdma_0_idle = ISP_RD_REG(rdma_com_0, REG_RDMA_CORE_T, NORM_STATUS1);

	data.rdma_1_err_sts = ISP_RD_REG(rdma_com_1, REG_RDMA_CORE_T, NORM_STATUS0);
	data.rdma_1_idle = ISP_RD_REG(rdma_com_1, REG_RDMA_CORE_T, NORM_STATUS1);
	return data;
}
