#include "vi_reg.h"
#include "vi_subsys_ctrl.h"
#include "vi_fe_ip_ctrl.h"

/*******************************************************************************
 *	Subsys config
 ******************************************************************************/
void ispblk_preraw_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = fe_find_hwid(phy_raw);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	u32 width = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w;
	u32 height = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h;
	union reg_pre_raw_fe_pre_raw_ctrl raw_ctrl;
	union reg_pre_raw_fe_pre_raw_frame_size  frm_size;

	frm_size.raw = 0;

	raw_ctrl.raw = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_ctrl);
	raw_ctrl.bits.bayer_type_le = ctx->isp_csi_cfg[raw_num].rgb_color_mode;
	raw_ctrl.bits.bayer_type_se = ctx->isp_csi_cfg[raw_num].rgb_color_mode;
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_ctrl, raw_ctrl.raw);

	frm_size.bits.frame_widthm1 = width - 1;
	frm_size.bits.frame_heightm1 = height - 1;
	ISP_WR_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_frame_size, frm_size.raw);
}

void ispblk_preraw_vi_sel_config(struct isp_ctx *ctx)
{
	uintptr_t vi_sel = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_VI_SEL];
	union reg_pre_raw_vi_sel_1 vi_sel_1;
	int pipe = ctx->cfg_info.pipe;
	// union reg_pre_raw_vi_sel_11 vi_reg_11;
	vi_sel_1.raw = 0;
	vi_sel_1.bits.frame_widthm1 = ctx->cfg_info.img_width - 1;
	vi_sel_1.bits.frame_heightm1 = ctx->cfg_info.img_height - 1;
	ISP_WR_REG(vi_sel, reg_pre_raw_vi_sel_t, reg_1, vi_sel_1.raw);

	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, csi_in_format, 1);
		}
		//dpcm off
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 0);
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 0);
	} else { //RGB sensor
		ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, csi_in_format, 0);
		if (!_is_all_online(ctx) && ctx->is_dpcm_on) {
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 7);
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 8191);
		} else {
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dma_ld_dpcm_mode, 0);
			ISP_WR_BITS(vi_sel, reg_pre_raw_vi_sel_t, reg_0, dpcm_rx_xstr, 0);
		}
	}

	// vi_reg_11.raw = ISP_RD_REG(vi_sel, reg_pre_raw_vi_sel_t, reg_11);
	// vi_reg_11.bits.in_format_le = 3;
	// vi_reg_11.bits.in_format_se = 3;
	// vi_reg_11.bits.bayer_type = 1;
	// ISP_WR_REG(vi_sel, reg_pre_raw_vi_sel_t, reg_11, vi_reg_11.raw);
}

void ispblk_rawtop_config(struct isp_ctx *ctx)
{
	uintptr_t rawtop0 = ctx->phys_regs[ISP_BLK_ID_RAWTOP0];
	uintptr_t rawtop1 = ctx->phys_regs[ISP_BLK_ID_RAWTOP1];
	union reg_raw_top0_raw_2 rawtop0_raw_2;
	union reg_raw_top0_raw_3 rawtop0_raw_3;
	union reg_raw_top0_pass_sel pass_sel;
	union reg_raw_top1_raw_2 rawtop1_raw_2;
	union reg_raw_top1_raw_4 rawtop1_raw_4;
	union reg_raw_top1_chk_sum_en chk_sum_en;
	union reg_raw_top1_raw_bayer_type raw_bayer_type_topleft;
	int pipe = ctx->cfg_info.pipe;

	rawtop0_raw_2.raw = 0;
	rawtop0_raw_2.bits.img_widthm_1_2x = ctx->cfg_info.img_width - 1;
	rawtop0_raw_2.bits.img_heightm_1_2x = ctx->cfg_info.img_height - 1;
	ISP_WR_REG(rawtop0, reg_raw_top0_t, raw_2, rawtop0_raw_2.raw);

	rawtop0_raw_3.raw = 0;
	rawtop0_raw_3.bits.img_widthm_0 = ctx->cfg_info.img_width - 1;
	rawtop0_raw_3.bits.img_heightm_0 = ctx->cfg_info.img_height - 1;
	ISP_WR_REG(rawtop0, reg_raw_top0_t, raw_3, rawtop0_raw_3.raw);

	rawtop1_raw_4.raw = ISP_RD_REG(rawtop1, reg_raw_top1_t, raw_4);
	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			rawtop1_raw_4.bits.yuv_path = 1;
		}
	} else {
		rawtop1_raw_4.bits.yuv_path = 0;
	}
	ISP_WR_REG(rawtop1, reg_raw_top1_t, raw_4, rawtop1_raw_4.raw);

	chk_sum_en.raw = ISP_RD_REG(rawtop1, reg_raw_top1_t, chk_sum_en);
	chk_sum_en.bits.chk_sum_en = 1;
	ISP_WR_REG(rawtop1, reg_raw_top1_t, chk_sum_en, chk_sum_en.raw);

	//LLSC open
	ISP_WR_BITS(rawtop1, reg_raw_top1_t, ip_bypass, bypass_map_curve, 0);
	//MAP_CURVE open
	ISP_WR_BITS(rawtop1, reg_raw_top1_t, ip_bypass, bypass_llsc, 0);

	//hdr config
	pass_sel.raw = ISP_RD_REG(rawtop0, reg_raw_top0_t, pass_sel);
	if (!_is_fe_post_slice(ctx)) {
		pass_sel.bits.pass_sel = 0;
		pass_sel.bits.pass_cnt_m1 = 0;
		pass_sel.bits.pass_sel_bnr = 0;
		pass_sel.bits.pass_cnt_m1_bnr = 0;
	} else {
		pass_sel.bits.pass_sel = 1;
		pass_sel.bits.pass_sel_bnr = 1;
		pass_sel.bits.fifo_reverse_2x = 1;
	}
	ISP_WR_REG(rawtop0, reg_raw_top0_t, pass_sel, pass_sel.raw);

	rawtop1_raw_2.raw = 0;
	rawtop1_raw_2.bits.img_widthm_1_1x = ctx->cfg_info.img_width - 1;
	rawtop1_raw_2.bits.img_heightm_1_1x = ctx->cfg_info.img_height - 1;
	ISP_WR_REG(rawtop1, reg_raw_top1_t, raw_2, rawtop1_raw_2.raw);

	raw_bayer_type_topleft.raw = 0;
	raw_bayer_type_topleft.bits.bayer_type_1x = ctx->isp_pipe_cfg[pipe].rgb_color_mode;
	ISP_WR_REG(rawtop1, reg_raw_top1_t, raw_bayer_type, raw_bayer_type_topleft.raw);
}

void ispblk_rgbtop_config(struct isp_ctx *ctx)
{
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	union reg_rgb_top_patgen_05 patgen_05;

	patgen_05.raw = 0;

	patgen_05.bits.rgbtop_imgw = ctx->cfg_info.img_width;
	patgen_05.bits.rgbtop_imgh = ctx->cfg_info.img_height;
	ISP_WR_REG(rgbtop, reg_rgb_top_t, patgen_05, patgen_05.raw);
}

void ispblk_yuvtop_config(struct isp_ctx *ctx)
{
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	union reg_yuv_top_imgw_m1 imgw_m1;
	int pipe = ctx->cfg_info.pipe;
	u32 width = ctx->cfg_info.img_width;
	u32 height = ctx->cfg_info.img_height;

	imgw_m1.raw = 0;
	imgw_m1.bits.yuv_top_imgw_m1 = width - 1;
	imgw_m1.bits.yuv_top_imgh_m1 = height - 1;
	ISP_WR_REG(yuvtop, reg_yuv_top_t, yuv_top_imgw_m1, imgw_m1.raw);

	ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl_mars3, drop_mode, 1);

	//bypass_v = 1 -> 422P online to scaler
	ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl_mars3, bypass_v_1,
				!ctx->isp_pipe_cfg[pipe].is_offline_scaler);

	ISP_WR_BITS(yuvtop, reg_yuv_top_t, yuv_ctrl_mars3, uv_swap_en,
				ctx->isp_pipe_cfg[pipe].is_uv_swap && ctx->isp_pipe_cfg[pipe].is_offline_scaler);

}

void ispblk_isptop_fe_config(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool en)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_ctrl_mode_sel0 trig_sel0;
	union reg_isp_top_ctrl_mode_sel1 trig_sel1;
	union reg_isp_top_ip_enable3 ip_enable3;
	uint8_t pre_fe_trig_by_hw = 0;
	uint8_t mux = ctx->isp_csi_cfg[raw_num].mux_mode;

	if (ctx->is_rawreplay) { // RAW replay
		pre_fe_trig_by_hw = 0x0;
	} else if (!ctx->isp_csi_cfg[raw_num].is_yuv_sensor) { //RGB sensor
		pre_fe_trig_by_hw = ctx->isp_csi_cfg[raw_num].is_hdr_on ? 0x3 : 0x1;
	} else { //YUV sensor
		if (mux <= VI_WORK_MODE_4MULTIPLEX)
			pre_fe_trig_by_hw = (1 << (mux + 1)) - 1;
		else
			pre_fe_trig_by_hw = 0;
	}

	ip_enable3.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ip_enable3);
	trig_sel0.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0);
	trig_sel1.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1);
	switch (raw_num) {
	case ISP_PRERAW0:
		ip_enable3.bits.csi0_ae_l_enable = en;
		ip_enable3.bits.csi0_ae_s_enable = en;
		ip_enable3.bits.csi0_clsc_l_enable = en;
		ip_enable3.bits.csi0_clsc_s_enable = en;
		trig_sel0.bits.trig_str_sel_fe0 = pre_fe_trig_by_hw;
		trig_sel0.bits.shaw_up_sel_fe0 = pre_fe_trig_by_hw;
		trig_sel1.bits.pq_up_sel_fe0 = pre_fe_trig_by_hw;
		break;
	case ISP_PRERAW1:
		ip_enable3.bits.csi1_ae_l_enable = en;
		ip_enable3.bits.csi1_ae_s_enable = en;
		ip_enable3.bits.csi1_clsc_l_enable = en;
		ip_enable3.bits.csi1_clsc_s_enable = en;
		trig_sel0.bits.trig_str_sel_fe1 = pre_fe_trig_by_hw;
		trig_sel0.bits.shaw_up_sel_fe1 = pre_fe_trig_by_hw;
		trig_sel1.bits.pq_up_sel_fe1 = pre_fe_trig_by_hw;
		break;
	case ISP_PRERAW2:
		ip_enable3.bits.csi2_ae_l_enable = en;
		ip_enable3.bits.csi2_ae_s_enable = en;
		ip_enable3.bits.csi2_clsc_l_enable = en;
		ip_enable3.bits.csi2_clsc_s_enable = en;
		trig_sel0.bits.trig_str_sel_fe2 = pre_fe_trig_by_hw;
		trig_sel0.bits.shaw_up_sel_fe2 = pre_fe_trig_by_hw;
		trig_sel1.bits.pq_up_sel_fe2 = pre_fe_trig_by_hw;
		break;
	default:
		// No action is required for the default case as it handles unsupported raw_num values.
		break;
	}

	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0, trig_sel0.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1, trig_sel1.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable3, ip_enable3.raw);
}

//split isptop_config and isptop_fe_config
void ispblk_isptop_config(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	u8 pre_be_trig_by_hw = 0;
	u8 post_trig_by_hw = 0;
	u8 raw_num = ctx->cfg_info.raw_num;
	u8 pipe = ctx->cfg_info.pipe;

	union reg_isp_top_int_event0_en ev0_en;
	union reg_isp_top_int_event1_en ev1_en;
	union reg_isp_top_int_event2_en ev2_en;
	union reg_isp_top_ctrl_mode_sel0 trig_sel0;
	union reg_isp_top_ctrl_mode_sel1 trig_sel1;
	union reg_isp_top_scenarios_ctrl scene_ctrl;

	ev0_en.raw = ev1_en.raw = ev2_en.raw = 0;
	scene_ctrl.raw = 0;

	trig_sel0.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0);
	trig_sel1.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1);

	if (ctx->is_rawreplay || _is_fe_post_offline(ctx))
		pre_be_trig_by_hw = 0x0;
	else {
		if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor)
			pre_be_trig_by_hw = 0x0;
		else { //Single RGB sensor
			if (ctx->is_hdr_on)
				pre_be_trig_by_hw = 0x3;
			else
				pre_be_trig_by_hw = 0x1;
		}
	}

	// fly mode or single sensor and slice buffer mode on. post trigger by vsync
	if (!ctx->is_rawreplay && (_is_all_online(ctx) || (_is_fe_post_slice(ctx))))
		post_trig_by_hw = 0x1;
	else //trigger by SW
		post_trig_by_hw = 0x0;

	//pre_fe0
	ev0_en.bits.frame_done_enable_fe0	= 0xF;
	ev2_en.bits.frame_start_enable_fe0	= 0xF;

	//pre_fe1
	ev0_en.bits.frame_done_enable_fe1	= 0xF;
	ev2_en.bits.frame_start_enable_fe1	= 0xF;

	//pre_fe2
	ev0_en.bits.frame_done_enable_fe2	= 0x3;
	ev2_en.bits.frame_start_enable_fe2	= 0x3;

	//pre_be
	ev0_en.bits.frame_done_enable_be	= 0x3;
	trig_sel0.bits.trig_str_sel_be		= pre_be_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_be		= pre_be_trig_by_hw;
	trig_sel1.bits.pq_up_sel_be		= pre_be_trig_by_hw;

	//postraw
	ev0_en.bits.frame_done_enable_post	= 0x1;
	ev0_en.bits.shaw_done_enable_post	= 0x1;
	trig_sel0.bits.trig_str_sel_raw		= post_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_raw		= post_trig_by_hw;
	trig_sel1.bits.pq_up_sel_raw		= post_trig_by_hw;
	trig_sel0.bits.trig_str_sel_post	= post_trig_by_hw;
	trig_sel0.bits.shaw_up_sel_post		= post_trig_by_hw;
	trig_sel1.bits.pq_up_sel_post		= post_trig_by_hw;

	//csibdg_lite
	ev2_en.bits.int_bdg0_lite_enable	= 0x1;
	ev2_en.bits.cmdq_int_enable		= 0x1;
	ev2_en.bits.line_intp_enable_fe0	= 0x1;
	ev2_en.bits.line_intp_enable_fe1	= 0x1;

	//err int
	ev2_en.bits.frame_err_enable		= 0x1;
	ev2_en.bits.int_dma_err_enable		= 0x1;

	//scenario_ctrl mode
	//single sensor
	scene_ctrl.raw = ISP_RD_REG(isptopb, reg_isp_top_t, scenarios_ctrl);
	scene_ctrl.bits.pre2be_l_enable		= _is_all_online(ctx);
	scene_ctrl.bits.pre2be_s_enable		= 0;
	scene_ctrl.bits.pre2yuv_422_enable	= 0;
	//multi sensors
	scene_ctrl.bits.be2raw_l_enable		= 1;
	scene_ctrl.bits.be2raw_s_enable		= 1;
	// multi sensors or raw replay
	scene_ctrl.bits.be_rdma_l_enable	= !_is_all_online(ctx);
	scene_ctrl.bits.be_rdma_s_enable	= !_is_all_online(ctx) &&
						  ctx->cfg_info.is_hdr_on;

	scene_ctrl.bits.hdr_enable		= ctx->cfg_info.is_hdr_on;
	// to verify ip, turn off hw lut of rgbgamma, ynr, and cnr.
	scene_ctrl.bits.hw_auto_enable		= 0;
	scene_ctrl.bits.multi_sensor_enable	= ctx->is_multi_sensor;

	scene_ctrl.bits.be_src_sel		= raw_num;
	scene_ctrl.bits.yuv_edge_en		= 1;

	scene_ctrl.bits.dst2dma			= ctx->isp_pipe_cfg[pipe].is_offline_scaler;

	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2, 0xFFFFFFFF);

	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0_en, ev0_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1_en, ev1_en.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en, ev2_en.raw);

	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0, trig_sel0.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel1, trig_sel1.raw);

	ISP_WR_REG(isptopb, reg_isp_top_t, scenarios_ctrl, scene_ctrl.raw);

	ISP_WR_BITS(isptopb, reg_isp_top_t, dummy, dbus_sel, 4);
	//ISP_WR_REG(isptopb, reg_isp_top_t, reg_1c, 7);

	//af llsc clsc
	// ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable1, 0x3c000010);
	// ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable2, 0x63);
	// ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable3, 0xCC0);
}

void isp_intr_set_mask(struct isp_ctx *ctx)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0_en, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1_en, 0);
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2_en, 0);
}

void isp_reset(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_ctrl_mode_sel0 mode_sel0;

	// disable interrupt
	isp_intr_set_mask(ctx);

	// switch back to hw trig.
	mode_sel0.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0);
	mode_sel0.bits.trig_str_sel_fe0  = 0xf;
	mode_sel0.bits.trig_str_sel_fe1  = 0xf;
	mode_sel0.bits.trig_str_sel_fe2  = 0x3;
	mode_sel0.bits.trig_str_sel_be   = 0x3;
	mode_sel0.bits.trig_str_sel_raw  = 0x1;
	mode_sel0.bits.trig_str_sel_post = 0x1;
	ISP_WR_REG(isptopb, reg_isp_top_t, ctrl_mode_sel0, mode_sel0.raw);

	// reset
	//AXI_RST first, then reset other
	//work round reset
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x3ff);
	ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0x0);


	// clear intr
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event0, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event1, 0xFFFFFFFF);
	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2, 0xFFFFFFFF);
}

/*******************************************************************************
 *	Subsys Debug Infomation
 ******************************************************************************/
struct _csi_bdg_chn_dbg_i ispblk_csibdg_chn_dbg(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	enum sop_isp_fe_chn_num chn_num)
{
	int id = -1;
	uintptr_t ba = 0;
	struct _csi_bdg_chn_dbg_i chn_dbg;

	if (ctx->isp_csi_cfg[raw_num].is_bt_demux) {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch0_debug_3);
			break;
		case ISP_FE_CH1:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch1_debug_3);
			break;
		case ISP_FE_CH2:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch2_debug_3);
			break;
		case ISP_FE_CH3:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, ch3_debug_3);
			break;
		default:
			break;
		}
	} else {
		id = csibdg_find_hwid(raw_num);
		ba = ctx->phys_regs[id];

		switch (chn_num) {
		case ISP_FE_CH0:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch0_debug_3);
			break;
		case ISP_FE_CH1:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch1_debug_3);
			break;
		case ISP_FE_CH2:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch2_debug_3);
			break;
		case ISP_FE_CH3:
			chn_dbg.dbg_0 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_0);
			chn_dbg.dbg_1 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_1);
			chn_dbg.dbg_2 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_2);
			chn_dbg.dbg_3 = ISP_RD_REG(ba, reg_isp_csi_bdg_t, ch3_debug_3);
			break;
		default:
			break;
		}

	}

	return chn_dbg;
}

struct _fe_dbg_i ispblk_fe_dbg_info(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = fe_find_hwid(phy_raw);
	uintptr_t preraw_fe = ctx->phys_regs[id];
	struct _fe_dbg_i data;

	data.fe_idle_sts = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_debug_state);
	data.fe_done_sts = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, fe_idle_info);

	return data;
}

struct _post_dbg_i ispblk_post_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	struct _post_dbg_i data;

	data.top_sts_0 = ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle);
	data.top_sts_1 = ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle_1);

	return data;
}

struct _dma_dbg_i ispblk_dma_dbg_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
	uintptr_t rdma_com_2 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE2];
	uintptr_t rdma_com_3 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE3];
	struct _dma_dbg_i data;

	data.wdma_1_err_sts = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, norm_status0);
	data.wdma_1_idle = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, norm_status1);

	data.wdma_2_err_sts = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, norm_status0);
	data.wdma_2_idle = ISP_RD_REG(wdma_com_2, reg_wdma_core_t, norm_status1);

	data.wdma_3_err_sts = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, norm_status0);
	data.wdma_3_idle = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, norm_status1);

	data.rdma_1_err_sts = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, norm_status0);
	data.rdma_1_idle = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, norm_status1);

	data.rdma_2_err_sts = ISP_RD_REG(rdma_com_2, reg_rdma_core_t, norm_status0);
	data.rdma_2_idle = ISP_RD_REG(rdma_com_2, reg_rdma_core_t, norm_status1);

	data.rdma_3_err_sts = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, norm_status0);
	data.rdma_3_idle = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, norm_status1);

	return data;
}
