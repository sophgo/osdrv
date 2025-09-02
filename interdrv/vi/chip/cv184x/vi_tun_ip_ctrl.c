#include "vi_tun_ip_ctrl.h"
#include "vi_fe_ip_ctrl.h"
#include "vi_rgb_ip_ctrl.h"
#include "vi_yuv_ip_ctrl.h"
#include "vi_drv.h"
#include "comm_errno.h"
#include "comm_math.h"
#include "vi_reg.h"

#define POST_RUNTIME_TUN(_name) \
	{\
		struct sop_vip_isp_##_name##_config *cfg;\
		cfg = &post_tun->_name##_cfg;\
		ispblk_##_name##_tun_cfg(ctx, cfg);\
	}

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern int tuning_dis[3];

struct isp_tuning_cfg tuning_buf_addr;
static void *vi_tuning_ptr[VI_MAX_PIPE_NUM];

/*******************************************************
 *  Internal APIs
 ******************************************************/

/*******************************************************************************
 *	Tuning modules update
 ******************************************************************************/

void ispblk_gamma_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_gamma_config *cfg)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;

	union reg_isp_gamma_prog_data reg_data;
	union reg_isp_gamma_prog_ctrl prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_ctrl, gamma_enable, cfg->enable);

	if (!cfg->enable)
		return;

	prog_ctrl.raw = ISP_RD_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel		= prog_ctrl.bits.gamma_wsel ^ 1;

	prog_ctrl.bits.gamma_prog_en		= 1;
	prog_ctrl.bits.gamma_prog_1to3_en	= 1;
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_w, 1);
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_max, cfg->max);

	for (i = 0; i < 256; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.gamma_data_e = cfg->lut[i];
		reg_data.bits.gamma_data_o = cfg->lut[i + 1];
		reg_data.bits.gamma_w = 1;
		ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
	}

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, prog_ctrl.bits.gamma_wsel);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void vi_tuning_gamma_ips_update(struct isp_ctx *ctx, const u8 pipe)
{
	u8 tun_idx = 0;
	static int stop_update_gamma_ip = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (stop_update_gamma_ip > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_gamma_ip = 1;
			return;
		} else if ((tuning_dis[0] - 1) == pipe)
			stop_update_gamma_ip = 1; // stop on next
	} else
		stop_update_gamma_ip = 0;

	ispblk_gamma_tun_cfg(ctx, &post_tun->gamma_cfg);
	ispblk_ycur_tun_cfg(ctx, &post_tun->ycur_cfg);
}

void vi_tuning_dci_update(struct isp_ctx *ctx, const u8 pipe)
{
	u8 tun_idx = 0;
	static int stop_update_dci = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (stop_update_dci > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_dci = 1;
			return;
		} else if ((tuning_dis[0] - 1) == pipe)
			stop_update_dci = 1; // stop on next
	} else
		stop_update_dci = 0;

	ispblk_dci_tun_cfg(ctx, &post_tun->dci_cfg);
}

void vi_tuning_drc_update(struct isp_ctx *ctx, const u8 pipe)
{
	u8 tun_idx = 0;
	static int stop_update_drc = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	tun_idx  = post_cfg->tun_idx;

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (stop_update_drc > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_drc = 1;
			return;
		} else if ((tuning_dis[0] - 1) == pipe)
			stop_update_drc = 1; // stop on next
	} else
		stop_update_drc = 0;

	ispblk_drc_tun_cfg(ctx, &post_tun->drc_cfg);
}

void ispblk_clut_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_clut_config *cfg,
	const u8 pipe)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];

	if (!cfg->update)
		return;

	ISP_WR_BITS(clut, reg_isp_clut_t, clut_ctrl, clut_enable, cfg->enable);
	ISP_WR_BITS(clut, reg_isp_clut_t, lut_fill_select, fill_sel_sw, true);
}

void vi_tuning_clut_update(struct isp_ctx *ctx, const u8 pipe)
{
	u8 tun_idx = 0;
	static int stop_update_clut = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	tun_idx  = post_cfg->tun_idx;

	vi_pr(VI_DBG, "Postraw_(%d->%d) tuning update(%d):idx(%d)\n", ctx->isp_pipe_cfg[pipe].bind_raw,
			pipe, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (stop_update_clut > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update_clut = 1;
			return;
		} else if ((tuning_dis[0] - 1) == pipe)
			stop_update_clut = 1; // stop on next
	} else
		stop_update_clut = 0;

	ispblk_clut_tun_cfg(ctx, &post_tun->clut_cfg, pipe);
}

int vi_tuning_buf_setup(struct isp_ctx *ctx, uint8_t pipe)
{
	u64 post_paddr = 0, fe_paddr = 0;
	u64 phyAddr = 0;
	u32 size = 0;

	size = (VI_ALIGN(sizeof(struct sop_vip_isp_post_cfg)) + VI_ALIGN(sizeof(struct sop_vip_isp_fe_cfg)));

	if (!ctx->isp_pipe_cfg[pipe].is_enable) {
		vi_pr(VI_ERR, "pipe_%d is not enable\n", pipe);
		return 0;
	}

	if (vi_tuning_ptr[pipe] != NULL) {
		vi_pr(VI_INFO, "tuning_buf ptr[%d] already exist\n", pipe);
		return ERR_VI_FAILED_NOTCONFIG;
	}

	vi_tuning_ptr[pipe] = osal_kzalloc(size, OSAL_GFP_KERNEL);
	if (vi_tuning_ptr[pipe] == NULL) {
		vi_pr(VI_ERR, "tuning_buf ptr[%d] kmalloc size(%u) fail\n", pipe, size);
		return ERR_VI_NOMEM;
	}

	phyAddr = osal_virt_to_phys(vi_tuning_ptr[pipe]);

	post_paddr = phyAddr;
	tuning_buf_addr.post_addr[pipe] = post_paddr;
	tuning_buf_addr.post_vir[pipe] = osal_phys_to_virt(post_paddr);

	fe_paddr = phyAddr + VI_ALIGN(sizeof(struct sop_vip_isp_post_cfg));
	tuning_buf_addr.fe_addr[pipe] = fe_paddr;
	tuning_buf_addr.fe_vir[pipe] = osal_phys_to_virt(fe_paddr);

	vi_pr(VI_INFO, "pipe_%d tuning fe_addr[%d]=0x%llx, post_addr[%d]=0x%llx\n",
				pipe, pipe, (unsigned long long)tuning_buf_addr.fe_addr[pipe],
				pipe, (unsigned long long)tuning_buf_addr.post_addr[pipe]);

	return 0;
}

void vi_tuning_buf_release(struct isp_ctx *ctx, uint8_t pipe)
{
	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	osal_kfree(vi_tuning_ptr[pipe]);
	vi_tuning_ptr[pipe] = NULL;
}

void vi_tuning_buf_clear(uint8_t pipe)
{
	struct sop_vip_isp_post_cfg *post_cfg;
	struct sop_vip_isp_fe_cfg   *fe_cfg;
	u8 tun_idx = 0;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	fe_cfg   = (struct sop_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[pipe];

	if (tuning_buf_addr.post_vir[pipe] != NULL) {
		osal_memset((void *)tuning_buf_addr.post_vir[pipe], 0x0, sizeof(struct sop_vip_isp_post_cfg));
		tun_idx = post_cfg->tun_idx;
		vi_pr(VI_INFO, "Clear post tuning tun_update(%d), tun_idx(%d)\n",
				post_cfg->tun_update[tun_idx], tun_idx);
	}

	if (tuning_buf_addr.fe_vir[pipe] != NULL) {
		osal_memset((void *)tuning_buf_addr.fe_vir[pipe], 0x0, sizeof(struct sop_vip_isp_fe_cfg));
		tun_idx = fe_cfg->tun_idx;
		vi_pr(VI_INFO, "Clear fe tuning tun_update(%d), tun_idx(%d)\n",
				fe_cfg->tun_update[tun_idx], tun_idx);
	}
}

void *vi_get_tuning_buf_addr(u32 *size)
{
	*size = sizeof(struct isp_tuning_cfg);

	return (void *)&tuning_buf_addr;
}

//TODO need wbg ae ir????
void ispblk_ae_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ae_config *cfg)
{
	uintptr_t ba = 0;
	u32 raw;

	if (!cfg->update)
		return;

	switch (cfg->inst) {
	case 0: // LE
		ba = ctx->phys_regs[ISP_BLK_ID_AE_HIST_FE0];
		break;
	case 1: // SE
		ba = ctx->phys_regs[ISP_BLK_ID_AE_HIST_FE1];
		break;
	case 2: // SE
		ba = ctx->phys_regs[ISP_BLK_ID_AE_HIST_FE2];
		break;
	default:
		vi_pr(VI_ERR, "Wrong ae inst\n");
		return;
	}

	ISP_WR_BITS(ba, reg_isp_ae_hist_t, sts_ae0_hist_enable, sts_ae0_hist_enable, cfg->ae_enable);
	ISP_WR_BITS(ba, reg_isp_ae_hist_t, dmi_enable, dmi_enable, cfg->ae_enable);

	if (!cfg->ae_enable)
		return;

	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_offsetx, cfg->ae_offsetx);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_offsety, cfg->ae_offsety);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_numxm1, cfg->ae_numx - 1);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_numym1, cfg->ae_numy - 1);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_width, cfg->ae_width);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_height, cfg->ae_height);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, sts_ae_sts_div, cfg->ae_sts_div);

	raw = cfg->ae_face_offset_x[0] + (cfg->ae_face_offset_y[0] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face0_location, raw);

	raw = cfg->ae_face_size_minus1_x[0] + (cfg->ae_face_size_minus1_y[0] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face0_size, raw);

	raw = cfg->ae_face_offset_x[1] + (cfg->ae_face_offset_y[1] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face1_location, raw);

	raw = cfg->ae_face_size_minus1_x[1] + (cfg->ae_face_size_minus1_y[1] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face1_size, raw);

	raw = cfg->ae_face_offset_x[2] + (cfg->ae_face_offset_y[2] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face2_location, raw);

	raw = cfg->ae_face_size_minus1_x[2] + (cfg->ae_face_size_minus1_y[2] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face2_size, raw);

	raw = cfg->ae_face_offset_x[3] + (cfg->ae_face_offset_y[3] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face3_location, raw);

	raw = cfg->ae_face_size_minus1_x[3] + (cfg->ae_face_size_minus1_y[3] << 16);
	ISP_WR_REG(ba, reg_isp_ae_hist_t, ae_face3_size, raw);

	ISP_WR_REGS_BURST(ba, reg_isp_ae_hist_t, ae_face_enable_ctrl, cfg->ae_cfg, cfg->ae_cfg.ae_face_enable_ctrl);
	ISP_WR_REGS_BURST(ba, reg_isp_ae_hist_t, se_ae_blc_offset_r, cfg->ae_1_cfg, cfg->ae_1_cfg.se_ae_blc_offset_r);
	ISP_WR_REGS_BURST(ba, reg_isp_ae_hist_t, ae_wgt_00, cfg->ae_2_cfg, cfg->ae_2_cfg.ae_wgt_00);
}

void ispblk_lsc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lsc_config *cfg)
{
	int fe_clsc = 0;
	uintptr_t clsc = 0;
	union reg_isp_lsc_sc_wrap_3 sc_wrap_3;
	union reg_isp_lsc_sc_wrap_4 sc_wrap_4;
	union reg_isp_lsc_sc_wrap_13 sc_wrap_13;
	union reg_isp_lsc_sc_wrap_14 sc_wrap_14;

	if (!cfg->update)
		return;

	fe_clsc = clsc_find_hwid(cfg->inst >> 1, cfg->inst & BIT(0));
	clsc = ctx->phys_regs[fe_clsc];

	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, lsc_enable, cfg->lsc_enable);
	ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_11, dma_enable_lsc, cfg->lsc_enable);

	sc_wrap_3.raw = 0;
	sc_wrap_3.bits.lsc_boundary_interpolation_lf_range = cfg->lsc_boundary_interpolation_lf_range;
	sc_wrap_3.bits.lsc_boundary_interpolation_rt_range = cfg->lsc_boundary_interpolation_rt_range;
	sc_wrap_3.bits.lsc_boundary_interpolation_up_range = cfg->lsc_boundary_interpolation_up_range;
	sc_wrap_3.bits.lsc_boundary_interpolation_dn_range = cfg->lsc_boundary_interpolation_dn_range;
	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_3, sc_wrap_3.raw);

	sc_wrap_4.raw = 0;
	sc_wrap_4.bits.lsc_boundary_interpolation_mode = cfg->lsc_gain_bicubic_0_bilinear_1;
	sc_wrap_4.bits.lsc_gain_bicubic_0_bilinear_1 = cfg->lsc_gain_bicubic_0_bilinear_1;
	sc_wrap_4.bits.lsc_gain_3p9_0_4p8_1 = cfg->lsc_gain_3p9_0_4p8_1;
	sc_wrap_4.bits.lsc_strength = cfg->lsc_strength;
	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_4, sc_wrap_4.raw);

	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_8, cfg->lsc_intp_gain_min);
	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_9, cfg->lsc_intp_gain_max);

	sc_wrap_13.raw = 0;
	sc_wrap_13.bits.blc_offset_r = cfg->blc_offset_r;
	sc_wrap_13.bits.blc_offset_gr = cfg->blc_offset_gr;
	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_13, sc_wrap_13.raw);

	sc_wrap_14.raw = 0;
	sc_wrap_14.bits.blc_offset_gb = cfg->blc_offset_gb;
	sc_wrap_14.bits.blc_offset_b = cfg->blc_offset_b;
	ISP_WR_REG(clsc, reg_isp_lsc_t, sc_wrap_14, sc_wrap_14.raw);
}

void pre_fe_tuning_update(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num)
{
	u8 idx = 0, tun_idx = 0;
	u8 pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[0];
	static int stop_update = -1;
	struct sop_vip_isp_fe_cfg *fe_cfg;
	struct sop_vip_isp_fe_tun_cfg *fe_tun;
	struct sop_vip_isp_ae_config *ae_cfg;
	struct sop_vip_isp_lsc_config *lsc_cfg;

	fe_cfg = (struct sop_vip_isp_fe_cfg *)tuning_buf_addr.fe_vir[pipe];
	tun_idx = fe_cfg->tun_idx;

	vi_pr(VI_DBG, "Pre_fe_%d tuning update(%d):idx(%d)\n",
			raw_num, fe_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (fe_cfg->tun_update[tun_idx] == 0) ||
			ctx->isp_csi_cfg[raw_num].is_yuv_sensor)
		return;

	fe_tun = &fe_cfg->tun_cfg[tun_idx];

	if (tuning_dis[1]) {
		if (stop_update > 0)
			return;
		else if (tuning_dis[0] == 0) {
			stop_update = 1;
			return;
		} else if ((tuning_dis[0] - 1) == pipe)
			stop_update = 1; // stop on next
	} else
		stop_update = 0;

	ae_cfg = &fe_tun->ae_cfg;
	ispblk_ae_tun_cfg(ctx, ae_cfg);

	for (idx = 0; idx < (ctx->isp_csi_cfg[raw_num].is_hdr_on + 1); idx++) {
		lsc_cfg = &fe_tun->lsc_cfg[idx];
		ispblk_lsc_tun_cfg(ctx, lsc_cfg);
	}
}

void ispblk_blc_dg_wb_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_wbg_config *cfg)
{
	uintptr_t blc_db_wb;
	union reg_blc_dg_wb_base_config base_config;

	if (!cfg->update)
		return;

	blc_db_wb = cfg->inst ? ctx->phys_regs[ISP_BLK_ID_BLC_DG_WB1] : ctx->phys_regs[ISP_BLK_ID_BLC_DG_WB0];

	base_config.bits.shift_mode = 1;
	base_config.bits.cg_enable = 1;
	base_config.bits.blc_le_enable = cfg->blc_le_enable;
	base_config.bits.blc_se_enable = cfg->blc_se_enable;
	base_config.bits.wbg_le_enable = cfg->wbg_le_enable;
	base_config.bits.wbg_se_enable = cfg->wbg_se_enable;
	ISP_WR_REG(blc_db_wb, reg_blc_dg_wb_t, base_config, base_config.raw);

	ISP_WR_REGS_BURST(blc_db_wb, reg_blc_dg_wb_t, wbg_le_gain0, cfg->burst_cfg, cfg->burst_cfg.wbg_le_gain0);
}

//TODO bmtest not set hw_auto_cf_en
void ispblk_fusion_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_fusion_config *cfg)
{
	uintptr_t fusion = ctx->phys_regs[ISP_BLK_ID_FUSION];
	union reg_fusion_ctrl_reg fusion_ctrl_reg;
	union reg_fusion_lut_0 fusion_lut_0;
	union reg_fusion_lut_1 fusion_lut_1;
	union reg_fusion_lut_2 fusion_lut_2;

	if (!cfg->update)
		return;

	fusion_ctrl_reg.raw = 0;
	fusion_ctrl_reg.bits.u1_fusion_en = cfg->u1_fusion_en;
	fusion_ctrl_reg.bits.u2_fusion_fnum = cfg->u2_fusion_fnum;
	fusion_ctrl_reg.bits.u2_fusion_mode = cfg->u2_fusion_mode;
	fusion_ctrl_reg.bits.u4_fusion_dbg_mode = cfg->u4_fusion_dbg_mode;
	fusion_ctrl_reg.bits.u2_fusion_ysel = cfg->u2_fusion_ysel;
	fusion_ctrl_reg.bits.u2_fusion_nsel = cfg->u2_fusion_nsel;
	fusion_ctrl_reg.bits.u2_fusion_dsel = cfg->u2_fusion_dsel;
	fusion_ctrl_reg.bits.u13_fusion_evratio = cfg->u13_fusion_evratio;
	fusion_ctrl_reg.bits.u4_fusion_ev_fmt = cfg->u4_fusion_ev_fmt;

	ISP_WR_REG(fusion, reg_fusion_t, fusion_ctrl_reg, fusion_ctrl_reg.raw);

	fusion_lut_0.bits.u5_diff_dw16_0 = cfg->u5_diff_dw16[0];
	fusion_lut_0.bits.u5_diff_dw16_1 = cfg->u5_diff_dw16[1];
	fusion_lut_0.bits.u5_diff_dw16_2 = cfg->u5_diff_dw16[2];
	fusion_lut_0.bits.u5_diff_dw16_3 = cfg->u5_diff_dw16[3];
	fusion_lut_0.bits.u5_diff_dw16_4 = cfg->u5_diff_dw16[4];
	fusion_lut_0.bits.u5_diff_dw16_5 = cfg->u5_diff_dw16[5];
	ISP_WR_REG(fusion, reg_fusion_t, fusion_lut_0, fusion_lut_0.raw);

	fusion_lut_1.bits.u5_diff_dw16_6 = cfg->u5_diff_dw16[6];
	fusion_lut_1.bits.u5_diff_dw16_7 = cfg->u5_diff_dw16[7];
	fusion_lut_1.bits.u5_diff_dw16_8 = cfg->u5_diff_dw16[8];
	fusion_lut_1.bits.u5_diff_dw16_9 = cfg->u5_diff_dw16[9];
	fusion_lut_1.bits.u5_diff_dw16_10 = cfg->u5_diff_dw16[10];
	fusion_lut_1.bits.u5_diff_dw16_11 = cfg->u5_diff_dw16[11];
	ISP_WR_REG(fusion, reg_fusion_t, fusion_lut_1, fusion_lut_1.raw);

	fusion_lut_2.bits.u5_diff_dw16_12 = cfg->u5_diff_dw16[12];
	fusion_lut_2.bits.u5_diff_dw16_13 = cfg->u5_diff_dw16[13];
	fusion_lut_2.bits.u5_diff_dw16_14 = cfg->u5_diff_dw16[14];
	fusion_lut_2.bits.u5_diff_dw16_15 = cfg->u5_diff_dw16[15];
	ISP_WR_REG(fusion, reg_fusion_t, fusion_lut_2, fusion_lut_2.raw);

	ISP_WR_REGS_BURST(fusion, reg_fusion_t, fusion_ds1, cfg->burst_cfg, cfg->burst_cfg.fusion_ds1);
}

void ispblk_map_curve_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_map_curve_config *cfg)
{
	uintptr_t map_curve = ctx->phys_regs[ISP_BLK_ID_MAPCURVE];
	union reg_map_curve_01 reg_01;

	if (!cfg->update)
		return;

	reg_01.raw = 0;
	reg_01.bits.u1_fcurve16_en = cfg->u1_fcurve16_en;
	reg_01.bits.u2_fcurve16_ysel = cfg->u2_fcurve16_ysel;
	reg_01.bits.u4_fcurve16_yvwet = cfg->u4_fcurve16_yvwet;
	reg_01.bits.u4_ife_f_fcurve_ev_fmt = cfg->u4_ife_f_fcurve_ev_fmt;
	ISP_WR_REG(map_curve, reg_map_curve_t, reg_01, reg_01.raw);

	ISP_WR_REGS_BURST(map_curve, reg_map_curve_t, fcurve_flumw_lut0,
				cfg->burst_cfg, cfg->burst_cfg.fcurve_flumw_lut0);
}

void ispblk_dpc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dpc_config *cfg)
{
	uintptr_t dpc = ctx->phys_regs[ISP_BLK_ID_DPC];
	union reg_dpc_base_config base_config;
	u16 i;

	if (!cfg->update)
		return;

	base_config.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, base_config);
	base_config.bits.dpc_enable = cfg->dpc_enable;
	base_config.bits.spc_enable = cfg->spc_enable;
	ISP_WR_REG(dpc, reg_isp_dpc_t, base_config, base_config.raw);

	if (cfg->spc_enable && (cfg->bp_cnt > 0) && (cfg->bp_cnt < 4096)) {
		ISP_WR_BITS(dpc, reg_isp_dpc_t, spc_config, dpc_mem_sw_mode, 1);
		ISP_WR_BITS(dpc, reg_isp_dpc_t, write_mem_st_addr, dpc_bp_mem_st_addr_w, 0x1);

		for (i = 0; i < cfg->bp_cnt; i++)
			ISP_WR_REG(dpc, reg_isp_dpc_t, write_mem,
				0x80000000 | cfg->spc_defect_lut[i]);

		// write 1 fff-fff to end
		ISP_WR_REG(dpc, reg_isp_dpc_t, write_mem, 0x83ffffff);
		ISP_WR_BITS(dpc, reg_isp_dpc_t, spc_config, dpc_mem_sw_mode, 0);
	}

	ISP_WR_REGS_BURST(dpc, reg_isp_dpc_t, dark_threshold0, cfg->burst_cfg, cfg->burst_cfg.dark_threshold0);
}

void ispblk_ge_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ge_config *cfg)
{
	uintptr_t dpc = ctx->phys_regs[ISP_BLK_ID_DPC];
	union reg_dpc_base_config base_config;

	if (!cfg->update)
		return;

	base_config.raw = ISP_RD_REG(dpc, reg_isp_dpc_t, base_config);
	base_config.bits.ge_enable = cfg->ge_enable;
	ISP_WR_REG(dpc, reg_isp_dpc_t, base_config, base_config.raw);

	ISP_WR_REGS_BURST(dpc, reg_isp_dpc_t, ge_config, cfg->burst_cfg, cfg->burst_cfg.ge_config);
}

void ispblk_af_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_af_config *cfg)
{
	uintptr_t ba = ctx->phys_regs[ISP_BLK_ID_AF];
	uintptr_t rawtop1 = ctx->phys_regs[ISP_BLK_ID_RAWTOP1];

	union reg_isp_af_enables af_enables;
	union reg_isp_af_low_pass_horizon low_pass_horizon;
	union reg_isp_af_high_pass_horizon_0 high_pass_horizon_0;
	union reg_isp_af_high_pass_horizon_1 high_pass_horizon_1;
	union reg_isp_af_high_pass_vertical_0 high_pass_vertical_0;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ba, reg_isp_af_t, kickoff, af_enable, cfg->enable);
	ISP_WR_BITS(ba, reg_isp_af_t, dmi_enable, dmi_enable, cfg->enable);
	ISP_WR_REG(ba, reg_isp_af_t, bypass, !cfg->enable);

	ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg4, af_blc_enable, cfg->af_blc_enable);

	if (cfg->af_blc_enable) {
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg0, af_blc_offset_b, cfg->af_blc_offset_b);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg0, af_blc_offset_gb, cfg->af_blc_offset_gb);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg1, af_blc_offset_gr, cfg->af_blc_offset_gr);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg1, af_blc_offset_r, cfg->af_blc_offset_r);

		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg2, af_blc_gain_b, cfg->af_blc_gain_b);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg2, af_blc_gain_gb, cfg->af_blc_gain_gb);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg3, af_blc_gain_gr, cfg->af_blc_gain_gr);
		ISP_WR_BITS(rawtop1, reg_raw_top1_t, blc_cfg3, af_blc_gain_r, cfg->af_blc_gain_r);
	}

	if (!cfg->enable)
		return;

	af_enables.raw = ISP_RD_REG(ba, reg_isp_af_t, enables);
	af_enables.bits.af_dpc_enable = cfg->dpc_enable;
	af_enables.bits.af_hlc_enable = cfg->hlc_enable;
	ISP_WR_REG(ba, reg_isp_af_t, enables, af_enables.raw);

	ISP_WR_BITS(ba, reg_isp_af_t, square_enable, af_square_enable, cfg->square_enable);
	ISP_WR_BITS(ba, reg_isp_af_t, outshift, af_outshift, cfg->outshift);
	ISP_WR_BITS(ba, reg_isp_af_t, num_gapline, af_num_gapline, cfg->num_gapline);

	// 8 <= offset_x <= img_width - 8
	ISP_WR_BITS(ba, reg_isp_af_t, offset_x, af_offset_x, cfg->offsetx);
	// 2 <= offset_y <= img_height - 2
	ISP_WR_BITS(ba, reg_isp_af_t, offset_x, af_offset_y, cfg->offsety);

	ISP_WR_REG(ba, reg_isp_af_t, block_width, cfg->block_width);	// (ctx->cfg_info.img_width - 16) / numx
	ISP_WR_REG(ba, reg_isp_af_t, block_height, cfg->block_height);	// (ctx->cfg_info.img_height - 4) / numy
	ISP_WR_REG(ba, reg_isp_af_t, block_num_x, cfg->block_numx);		// should fixed to 17
	ISP_WR_REG(ba, reg_isp_af_t, block_num_y, cfg->block_numy);		// should fixed to 15

	ISP_WR_REG(ba, reg_isp_af_t, hor_low_pass_value_shift, cfg->h_low_pass_value_shift);
	ISP_WR_REG(ba, reg_isp_af_t, offset_horizontal_0, cfg->h_corning_offset_0);
	ISP_WR_REG(ba, reg_isp_af_t, offset_horizontal_1, cfg->h_corning_offset_1);
	ISP_WR_REG(ba, reg_isp_af_t, offset_vertical, cfg->v_corning_offset);
	ISP_WR_REG(ba, reg_isp_af_t, high_y_thre, cfg->high_luma_threshold);

	low_pass_horizon.raw = 0;
	low_pass_horizon.bits.af_low_pass_horizon_0 = cfg->h_low_pass_coef[0];
	low_pass_horizon.bits.af_low_pass_horizon_1 = cfg->h_low_pass_coef[1];
	low_pass_horizon.bits.af_low_pass_horizon_2 = cfg->h_low_pass_coef[2];
	low_pass_horizon.bits.af_low_pass_horizon_3 = cfg->h_low_pass_coef[3];
	low_pass_horizon.bits.af_low_pass_horizon_4 = cfg->h_low_pass_coef[4];
	ISP_WR_REG(ba, reg_isp_af_t, low_pass_horizon, low_pass_horizon.raw);

	high_pass_horizon_0.raw = 0;
	high_pass_horizon_0.bits.af_high_pass_horizon_0_0 = cfg->h_high_pass_coef_0[0];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_1 = cfg->h_high_pass_coef_0[1];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_2 = cfg->h_high_pass_coef_0[2];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_3 = cfg->h_high_pass_coef_0[3];
	high_pass_horizon_0.bits.af_high_pass_horizon_0_4 = cfg->h_high_pass_coef_0[4];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_horizon_0, high_pass_horizon_0.raw);

	high_pass_horizon_1.raw = 0;
	high_pass_horizon_1.bits.af_high_pass_horizon_1_0 = cfg->h_high_pass_coef_1[0];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_1 = cfg->h_high_pass_coef_1[1];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_2 = cfg->h_high_pass_coef_1[2];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_3 = cfg->h_high_pass_coef_1[3];
	high_pass_horizon_1.bits.af_high_pass_horizon_1_4 = cfg->h_high_pass_coef_1[4];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_horizon_1, high_pass_horizon_1.raw);

	high_pass_vertical_0.raw = 0;
	high_pass_vertical_0.bits.af_high_pass_vertical_0_0 = cfg->v_high_pass_coef[0];
	high_pass_vertical_0.bits.af_high_pass_vertical_0_1 = cfg->v_high_pass_coef[1];
	high_pass_vertical_0.bits.af_high_pass_vertical_0_2 = cfg->v_high_pass_coef[2];
	ISP_WR_REG(ba, reg_isp_af_t, high_pass_vertical_0, high_pass_vertical_0.raw);

	ISP_WR_BITS(ba, reg_isp_af_t, th_low, af_th_low, cfg->th_low);
	ISP_WR_BITS(ba, reg_isp_af_t, th_low, af_th_high, cfg->th_high);
	ISP_WR_BITS(ba, reg_isp_af_t, gain_low, af_gain_low, cfg->gain_low);
	ISP_WR_BITS(ba, reg_isp_af_t, gain_low, af_gain_high, cfg->gain_high);
	ISP_WR_BITS(ba, reg_isp_af_t, slop_low, af_slop_low, cfg->slop_low);
	ISP_WR_BITS(ba, reg_isp_af_t, slop_low, af_slop_high, cfg->slop_high);
}

void ispblk_bnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_bnr_config *cfg)
{
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	union reg_isp_bnr_10 bnr_10;
	union reg_isp_bnr_14 bnr_14;
	union reg_isp_bnr_18 bnr_18;
	union reg_isp_bnr_1c bnr_1c;

	if (!cfg->update)
		return;

	ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_00, u1_bnr_enable, cfg->u1_bnr_enable);
	ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_00, u2_bnr_debugmode, cfg->u2_bnr_debugmode);
	ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_04, out_enable, cfg->u1_bnr_out_enable);

	if (cfg->u1_bnr_enable) {
		bnr_10.bits.u1_bnr_filtmode = cfg->u1_bnr_filtmode;
		bnr_10.bits.u1_bnr_bilat_th_en = cfg->u1_bnr_bilat_th_en;
		bnr_10.bits.u1_bnr_bilat_center_sel = cfg->u1_bnr_bilat_center_sel;
		bnr_10.bits.u5_bnr_bilat_blend_w = cfg->u5_bnr_bilat_blend_w;
		ISP_WR_REG(bnr, reg_isp_bnr_t, bnr_10, bnr_10.raw);

		bnr_14.bits.u10_bnr_bilat_th1 = cfg->u10_bnr_bilat_th1;
		bnr_14.bits.u10_bnr_bilat_th2 = cfg->u10_bnr_bilat_th2;
		ISP_WR_REG(bnr, reg_isp_bnr_t, bnr_14, bnr_14.raw);

		bnr_18.bits.u5_bnr_sweight_0 = cfg->u5_bnr_sweight[0];
		bnr_18.bits.u5_bnr_sweight_1 = cfg->u5_bnr_sweight[1];
		bnr_18.bits.u5_bnr_sweight_2 = cfg->u5_bnr_sweight[2];
		bnr_18.bits.u5_bnr_sweight_3 = cfg->u5_bnr_sweight[3];
		bnr_18.bits.u5_bnr_sweight_4 = cfg->u5_bnr_sweight[4];
		ISP_WR_REG(bnr, reg_isp_bnr_t, bnr_18, bnr_18.raw);

		bnr_1c.bits.u5_bnr_sweight_5 = cfg->u5_bnr_sweight[5];
		bnr_1c.bits.u5_bnr_sweight_6 = cfg->u5_bnr_sweight[6];
		bnr_1c.bits.u5_bnr_sweight_7 = cfg->u5_bnr_sweight[7];
		bnr_1c.bits.u5_bnr_sweight_8 = cfg->u5_bnr_sweight[8];
		bnr_1c.bits.u5_bnr_sweight_9 = cfg->u5_bnr_sweight[9];
		ISP_WR_REG(bnr, reg_isp_bnr_t, bnr_1c, bnr_1c.raw);

		ISP_WR_REGS_BURST(bnr, reg_isp_bnr_t, bnr_20, cfg->nlm_burst_cfg, cfg->nlm_burst_cfg.bnr_20);
		ISP_WR_REGS_BURST(bnr, reg_isp_bnr_t, bnr_e0, cfg->bf_burst_cfg, cfg->bf_burst_cfg.bnr_e0);
	}
}

void ispblk_lscr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_lscr_config *cfg)
{
	uintptr_t lscr = ctx->phys_regs[ISP_BLK_ID_LSCR];
	union reg_isp_lscr_sc_wrap_3 sc_wrap_3;
	union reg_isp_lscr_sc_wrap_4 sc_wrap_4;
	union reg_isp_lscr_sc_wrap_5 sc_wrap_5;
	union reg_isp_lscr_sc_wrap_22 sc_wrap_22;
	union reg_isp_lscr_sc_wrap_23 sc_wrap_23;

	if (!cfg->update)
		return;

	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_1, lscr_enable, cfg->lscr_enable);
	ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_1, blc_enable, cfg->blc_enable);

	sc_wrap_22.bits.blc_offset_r = cfg->blc_offset_r;
	sc_wrap_22.bits.blc_offset_gr = cfg->blc_offset_gr;
	ISP_WR_REG(lscr, reg_isp_lscr_t, sc_wrap_22, sc_wrap_22.raw);

	sc_wrap_23.bits.blc_offset_gb = cfg->blc_offset_gb;
	sc_wrap_23.bits.blc_offset_b = cfg->blc_offset_b;
	ISP_WR_REG(lscr, reg_isp_lscr_t, sc_wrap_23, sc_wrap_23.raw);

	sc_wrap_3.bits.lscr_centerx = cfg->centerx;
	sc_wrap_3.bits.lscr_centery = cfg->centery;
	ISP_WR_REG(lscr, reg_isp_lscr_t, sc_wrap_3, sc_wrap_3.raw);

	sc_wrap_4.bits.lscr_norm = cfg->norm;
	sc_wrap_4.bits.lscr_strnth = cfg->strength;
	ISP_WR_REG(lscr, reg_isp_lscr_t, sc_wrap_4, sc_wrap_4.raw);

	sc_wrap_5.bits.lscr_nd_thr = cfg->lscr_nd_thr;
	sc_wrap_5.bits.lscr_nd_str = cfg->lscr_nd_str;
	ISP_WR_REG(lscr, reg_isp_lscr_t, sc_wrap_5, sc_wrap_5.raw);

	ISP_WR_REGS_BURST(lscr, reg_isp_lscr_t, sc_wrap_6, cfg->burst_cfg, cfg->burst_cfg.lscr_sc_wrap_6);

}

void ispblk_drc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_drc_config *cfg)
{
	uintptr_t drc = ctx->phys_regs[ISP_BLK_ID_DRC];
	union reg_isp_drc_blk_num drc_blk_num;
	union reg_isp_drc_blk_size drc_blk_size;
	union reg_isp_drc_blk_div drc_blk_div;
	union reg_isp_drc_hist drc_hist;
	union reg_isp_drc_hist_step drc_hist_step;
	union reg_isp_drc_intensit_mode drc_intensit_mode;

	if (!cfg->update)
		return;

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_enable, drc_enable, cfg->drc_enable);

	drc_blk_num.bits.drc_subimg_width = cfg->drc_subimg_width;
	drc_blk_num.bits.drc_subimg_height = cfg->drc_subimg_height;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_blk_num, drc_blk_num.raw);

	drc_blk_size.bits.drc_subimg_ratio_hori = cfg->drc_subimg_ratio_hori;
	drc_blk_size.bits.drc_subimg_ratio_vert = cfg->drc_subimg_ratio_vert;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_blk_size, drc_blk_size.raw);

	drc_blk_div.bits.drc_subimg_ratio_hori_div = cfg->drc_subimg_ratio_hori_div;
	drc_blk_div.bits.drc_subimg_ratio_vert_div = cfg->drc_subimg_ratio_vert_div;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_blk_div, drc_blk_div.raw);

	ISP_WR_BITS(drc, reg_isp_drc_t, drc_lpf_mode, drc_subimg_lpf_mode, cfg->drc_subimg_lpf_mode);

	drc_hist.bits.drc_hist_enable = cfg->drc_hist_enable;
	drc_hist.bits.drc_hist_mode = cfg->drc_hist_mode;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_hist, drc_hist.raw);

	drc_hist_step.bits.drc_hist_step_x = cfg->drc_hist_step_x;
	drc_hist_step.bits.drc_hist_step_y = cfg->drc_hist_step_y;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_hist_step, drc_hist_step.raw);

	drc_intensit_mode.bits.drc_intensity_mode = cfg->drc_intensity_mode;
	drc_intensit_mode.bits.drc_tone_curve_enable = cfg->drc_tone_curve_enable;
	ISP_WR_REG(drc, reg_isp_drc_t, drc_intensit_mode, drc_intensit_mode.raw);

	ISP_WR_REGS_BURST(drc, reg_isp_drc_t, drc_hori_lpf_th12,
				cfg->burst_cfg, cfg->burst_cfg.drc_hori_lpf_th12);
}

void ispblk_demosiac_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_demosiac_config *cfg)
{
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];
	union reg_isp_cfa_00 reg_00;
	union reg_isp_cfa_04 reg_04;
	union reg_isp_cfa_110 reg_110;

	if (!cfg->update)
		return;

	reg_00.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_00);
	reg_00.bits.cfa_enable			= cfg->cfa_enable;
	reg_00.bits.cfa_force_dir_enable	= cfg->cfa_force_dir_enable;
	reg_00.bits.cfa_force_dir_sel		= cfg->cfa_force_dir_sel;
	reg_00.bits.cfa_ymoire_enable		= cfg->cfa_ymoire_enable;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_00, reg_00.raw);

	if (!cfg->cfa_enable)
		return;

	reg_04.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_04);
	reg_04.bits.cfa_out_sel		= cfg->cfa_out_sel;
	reg_04.bits.cfa_edgee_thd2	= cfg->cfa_edgee_thd2;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_04, reg_04.raw);

	ISP_WR_BITS(cfa, reg_isp_cfa_t, reg_20, cfa_rbsig_luma_thd, cfg->cfa_rbsig_luma_thd);

	reg_110.raw = ISP_RD_REG(cfa, reg_isp_cfa_t, reg_110);
	reg_110.bits.cfa_ymoire_lpf_w	= cfg->cfa_ymoire_lpf_w;
	reg_110.bits.cfa_ymoire_dc_w	= cfg->cfa_ymoire_dc_w;
	ISP_WR_REG(cfa, reg_isp_cfa_t, reg_110, reg_110.raw);

	ISP_WR_REG_LOOP_SHFT(cfa, reg_isp_cfa_t, reg_30, 32, 4, cfg->cfa_ghp_lut, 8);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_0c, cfg->demosiac_cfg, cfg->demosiac_cfg.reg_0c);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_120, cfg->demosiac_1_cfg, cfg->demosiac_1_cfg.reg_120);

	ISP_WR_REGS_BURST(cfa, reg_isp_cfa_t, reg_90, cfg->demosiac_2_cfg, cfg->demosiac_2_cfg.reg_90);
}

//rgb_top
void ispblk_ee_ext_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ee_ext_config *cfg)
{
	uintptr_t ee_ext = ctx->phys_regs[ISP_BLK_ID_PRE_EE_EXT];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_enable, cfg->enable);
	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, hw_auto_cg_en, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_ch_selection, cfg->ee_ch_selection);
	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_gamma_selection, cfg->ee_gamma_selection);
	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_debug_mode, cfg->ee_debug_mode);

	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_gamma_lut_u10_0, cfg->gamma_lut[0]);
	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_gamma_lut_u10_1, cfg->gamma_lut[1]);

	ISP_WR_REGS_BURST(ee_ext, reg_ee_ext_t, ee_ext_gamma_0,
				cfg->gamma_burst_cfg, cfg->gamma_burst_cfg.ee_ext_gamma_0);

	ISP_WR_REGS_BURST(ee_ext, reg_ee_ext_t, ee_ext_e5c_0,
			cfg->cpef_burst_cfg, cfg->cpef_burst_cfg.ee_ext_e5c_0);

	ISP_WR_REGS_BURST(ee_ext, reg_ee_ext_t, ee_ext_norm,
			cfg->reg0_burst_cfg, cfg->reg0_burst_cfg.ee_ext_norm);

	ISP_WR_REGS_BURST(ee_ext, reg_ee_ext_t, ee_ext_region_2,
			cfg->reg1_burst_cfg, cfg->reg1_burst_cfg.ee_ext_region_2);
}

void ispblk_pfr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_pfr_config *cfg)
{
	uintptr_t pfr = ctx->phys_regs[ISP_BLK_ID_PFR];
	union reg_pfr_pfr_reg0 pfr_reg0;
	union reg_pfr_pfr_luma_set0 pfr_luma_set0;

	if (!cfg->update)
		return;

	pfr_reg0.raw = ISP_RD_REG(pfr, reg_pfr_t, pfr_reg0);
	pfr_reg0.bits.pfr_en = cfg->pfr_en;
	pfr_reg0.bits.pfr_luma_level_en = cfg->pfr_luma_level_en;
	pfr_reg0.bits.hw_auto_cg_en = cfg->pfr_en;
	pfr_reg0.bits.pfr_uvset_en_0 = cfg->pfr_uvset_en[0];
	pfr_reg0.bits.pfr_uvset_en_1 = cfg->pfr_uvset_en[1];
	pfr_reg0.bits.pfr_hueset_en_0 = cfg->pfr_hueset_en[0];
	pfr_reg0.bits.pfr_hueset_en_1 = cfg->pfr_hueset_en[1];
	ISP_WR_REG(pfr, reg_pfr_t, pfr_reg0, pfr_reg0.raw);

	pfr_luma_set0.bits.pfr_luma_level_th = cfg->pfr_luma_level_th;
	pfr_luma_set0.bits.pfr_luma_inivalue = cfg->pfr_luma_inivalue;
	ISP_WR_REG(pfr, reg_pfr_t, pfr_luma_set0, pfr_luma_set0.raw);

	ISP_WR_REGS_BURST(pfr, reg_pfr_t, pfr_luma_level0,
			cfg->burst0_cfg, cfg->burst0_cfg.pfr_luma_level0);
	ISP_WR_REGS_BURST(pfr, reg_pfr_t, pfr_luma_level1,
			cfg->burst1_cfg, cfg->burst1_cfg.pfr_luma_level1);
	ISP_WR_REGS_BURST(pfr, reg_pfr_t, pfr_uv_diff_lut1,
			cfg->burst2_cfg, cfg->burst2_cfg.pfr_uv_diff_lut1);
	ISP_WR_REGS_BURST(pfr, reg_pfr_t, pfr_hue_range,
			cfg->burst3_cfg, cfg->burst3_cfg.pfr_hue_range);
}

void ispblk_ccm_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ccm_config *cfg)
{
	uintptr_t ccm = ctx->phys_regs[ISP_BLK_ID_CCM];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_22, cfg->coef[2][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_overexp_str, cfg->over_str);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_overexp_thr, cfg->over_thr);
}

void ispblk_csc_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_csc_config *cfg)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];
	union reg_isp_csc_4 csc_4;
	union reg_isp_csc_5 csc_5;
	union reg_isp_csc_6 csc_6;
	union reg_isp_csc_7 csc_7;
	union reg_isp_csc_8 csc_8;
	union reg_isp_csc_9 csc_9;

	if (!cfg->update)
		return;

	ISP_WR_BITS(csc, reg_isp_csc_t, reg_0, csc_enable, cfg->enable);

	if (!cfg->enable)
		return;

	csc_4.raw = 0;
	csc_4.bits.coeff_00 = cfg->coeff[0] & 0x3fff;
	csc_4.bits.coeff_01 = cfg->coeff[1] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_4, csc_4.raw);

	csc_5.raw = 0;
	csc_5.bits.coeff_02 = cfg->coeff[2] & 0x3fff;
	csc_5.bits.coeff_10 = cfg->coeff[3] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_5, csc_5.raw);

	csc_6.raw = 0;
	csc_6.bits.coeff_11 = cfg->coeff[4] & 0x3fff;
	csc_6.bits.coeff_12 = cfg->coeff[5] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_6, csc_6.raw);

	csc_7.raw = 0;
	csc_7.bits.coeff_20 = cfg->coeff[6] & 0x3fff;
	csc_7.bits.coeff_21 = cfg->coeff[7] & 0x3fff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_7, csc_7.raw);

	csc_8.raw = 0;
	csc_8.bits.coeff_22 = cfg->coeff[8] & 0x3fff;
	csc_8.bits.offset_0 = cfg->offset[0] & 0x7ff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_8, csc_8.raw);

	csc_9.raw = 0;
	csc_9.bits.offset_1 = cfg->offset[1] & 0x7ff;
	csc_9.bits.offset_2 = cfg->offset[2] & 0x7ff;
	ISP_WR_REG(csc, reg_isp_csc_t, reg_9, csc_9.raw);
}

void ispblk_ldci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ldci_config *cfg)
{
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	// union reg_ldci_ldci_crop_width ldci_crop_width;
	// union reg_ldci_ldci_crop_height ldci_crop_height;
	union reg_ldci_ldci_ctl ldci_ctl;

	if (!cfg->update)
		return;

	ldci_ctl.raw = ISP_RD_REG(ldci, reg_ldci_t, ldci_ctl);
	ldci_ctl.bits.ldci_enable = cfg->ldci_enable;
	ldci_ctl.bits.ldci_luma_prot_en = cfg->ldci_luma_prot_en;
	ISP_WR_REG(ldci, reg_ldci_t, ldci_ctl, ldci_ctl.raw);

	if (cfg->ldci_enable) {
		ISP_WR_REGS_BURST(ldci, reg_ldci_t, ldci_reciprocal_0,
				cfg->burst_0_cfg, cfg->burst_0_cfg.ldci_reciprocal_0);
		ISP_WR_REGS_BURST(ldci, reg_ldci_t, ldci_luma_gain_lut0,
				cfg->burst_1_cfg, cfg->burst_1_cfg.ldci_luma_gain_lut0);
	}

	// ISP_WR_REG(ldci, reg_ldci_t, ldci_crop_enable, cfg->ldci_crop_enable);
	// ldci_crop_width.bits.ldci_crop_w_str = cfg->drc_crop_w_str;
	// ldci_crop_width.bits.ldci_crop_w_end = cfg->drc_crop_w_end;
	// ISP_WR_REG(ldci, reg_ldci_t, ldci_crop_width, ldci_crop_width.raw);

	// ldci_crop_height.bits.ldci_crop_h_str = cfg->drc_crop_h_str;
	// ldci_crop_height.bits.ldci_crop_h_end = cfg->drc_crop_h_end;
	// ISP_WR_REG(ldci, reg_ldci_t, ldci_crop_height, ldci_crop_height.raw);
}

void ispblk_dci_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_dci_config *cfg)
{
	uintptr_t dci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union reg_ldci_ldci_roi_axis_0 ldci_roi_axis_0;
	union reg_ldci_ldci_roi_axis_1 ldci_roi_axis_1;

	if (!cfg->update)
		return;

	ISP_WR_BITS(dci, reg_ldci_t, ldci_ctl, dci_enable, cfg->dci_enable);

	ISP_WR_BITS(dci, reg_ldci_t, ldci_roi_ctl, dci_histsample_step, cfg->dci_histsample_step);
	ISP_WR_BITS(dci, reg_ldci_t, ldci_roi_ctl, dci_roi_enable, cfg->dci_roi_enable);

	ldci_roi_axis_0.bits.dci_roi_start_x = cfg->dci_roi_start_x;
	ldci_roi_axis_0.bits.dci_roi_start_y = cfg->dci_roi_start_y;
	ISP_WR_REG(dci, reg_ldci_t, ldci_roi_axis_0, ldci_roi_axis_0.raw);

	ldci_roi_axis_1.bits.dci_roi_width = cfg->dci_roi_width;
	ldci_roi_axis_1.bits.dci_roi_height = cfg->dci_roi_height;
	ISP_WR_REG(dci, reg_ldci_t, ldci_roi_axis_1, ldci_roi_axis_1.raw);

	ispblk_dci_map_config(ctx, 1, cfg->dci_ygamma_curve);
}

void ispblk_pre_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_pre_ee_config *cfg)
{
	uintptr_t ee_front = ctx->phys_regs[ISP_BLK_ID_PRE_EE_FRONT];
	uintptr_t ee_back = ctx->phys_regs[ISP_BLK_ID_PRE_EE_BACK];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, ee_enable, cfg->ee_enable);
	ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, hw_auto_cg_en, cfg->ee_enable);
	ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, ee_debug_mode, cfg->ee_debug_mode);
	ISP_WR_BITS(ee_back, reg_ee_add_back_t, ee_add_b_reg0, ee_enable, cfg->ee_enable);
	ISP_WR_BITS(ee_back, reg_ee_add_back_t, ee_add_b_reg0, hw_auto_cg_en, cfg->ee_enable);
	ISP_WR_BITS(ee_back, reg_ee_add_back_t, ee_add_b_reg0, ee_debug_mode, cfg->ee_debug_mode);

	if (cfg->ee_enable) {
		ISP_WR_REGS_BURST(ee_front, reg_ee_add_t, ee_add_overshoot_0,
				cfg->burst_cfg, cfg->burst_cfg.ee_add_overshoot_0);
	}
}

static void ispblk_tnr_disable_frm_ref(struct isp_ctx *ctx)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	union reg_isp_444_422_51 reg_51;
	uint32_t val = 0;
	uint32_t lut_16[64] = {0};
	uint32_t lut_8[8] = {0, 16, 32, 48, 64, 80, 96, 112};
	uintptr_t addr = 0;

	//workaround for first frame, no reference first frame

	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_38, 0);
	reg_51.raw = ISP_RD_REG(tnr, reg_isp_444_422_t, reg_51);
	reg_51.bits.reg_3dnr_uv_l0_st_jnd_sigma_u8_0 = 0;
	reg_51.bits.reg_3dnr_uv_l0_st_jnd_sigma_u8_1 = 0;
	reg_51.bits.reg_3dnr_uv_l0_st_jnd_sigma_u8_2 = 0;
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_51, reg_51.raw);

	addr = tnr + _OFST(reg_isp_444_422_t, reg_54);
	REG_ARRAY_UPDATE4(addr, lut_16);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_70);
	REG_ARRAY_UPDATE4(addr, lut_16);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_86);
	REG_ARRAY_UPDATE4(addr, lut_16);

	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_46, 0);

	addr = tnr + _OFST(reg_isp_444_422_t, reg_102);
	REG_ARRAY_UPDATE4(addr, lut_8);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_104);
	REG_ARRAY_UPDATE4(addr, lut_8);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_106);
	REG_ARRAY_UPDATE4(addr, lut_8);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_109);
	REG_ARRAY_UPDATE4(addr, lut_8);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_111);
	REG_ARRAY_UPDATE4(addr, lut_8);
	addr = tnr + _OFST(reg_isp_444_422_t, reg_113);
	REG_ARRAY_UPDATE4(addr, lut_8);
}

void ispblk_tnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_tnr_config *cfg,
	const u8 pipe)
{
	uintptr_t tnr = ctx->phys_regs[ISP_BLK_ID_TNR];
	union reg_isp_444_422_6 reg_6;
	union reg_isp_444_422_reg116 reg_116;

	if (!ctx->is_3dnr_on || !cfg->update)
		return;

	reg_6.bits.reg_3dnr_debug_mode = cfg->reg_3dnr_debug_mode;
	reg_6.bits.reg_3dnr_subpixel_enable = cfg->reg_3dnr_subpixel_enable;
	reg_6.bits.reg_3dnr_y_l0_pss_blur = cfg->reg_3dnr_y_L0_PSS_blur;
	reg_6.bits.reg_3dnr_luma_jnd_ratio = cfg->reg_3dnr_luma_jnd_ratio;
	reg_6.bits.reg_3dnr_sad_lpf_mode = cfg->reg_3dnr_sad_lpf_mode;
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_6, reg_6.raw);

	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_120, y_debug_en, cfg->reg_y_debug_en);

	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_0, tdnr_enable, cfg->tdnr_enable ? 3 : 0);
	ISP_WR_BITS(tnr, reg_isp_444_422_t, reg_0, dma_enable, cfg->tdnr_enable ? 0x7FF : 0x0);

	reg_116.bits.reg_3dnr_y_bundle_nr_enable = cfg->y_bundle_nr_enable;
	reg_116.bits.reg_3dnr_uv_bundle_nr_enable = cfg->uv_bundle_nr_enable;
	reg_116.bits.reg_3dnr_uv_mono_enable = cfg->mono_enable;
	reg_116.bits.reg_3dnr_uv_mono_val = cfg->mono_uv_val;
	ISP_WR_REG(tnr, reg_isp_444_422_t, reg_116, reg_116.raw);

	if (!cfg->tdnr_enable)
		return;

	ISP_WR_REGS_BURST(tnr, reg_isp_444_422_t, reg_7,
			cfg->sad_burst_cfg, cfg->sad_burst_cfg.reg_7);
	ISP_WR_REGS_BURST(tnr, reg_isp_444_422_t, reg_32,
			cfg->misc_burst_cfg, cfg->misc_burst_cfg.reg_32);
	ISP_WR_REGS_BURST(tnr, reg_isp_444_422_t, reg_46,
			cfg->ych_burst_cfg, cfg->ych_burst_cfg.reg_46);

	if (!ctx->isp_pipe_cfg[pipe].first_frm_cnt) {
		// first frame, no reference frame
		ispblk_tnr_disable_frm_ref(ctx);
	}
}

void ispblk_cnr_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cnr_config *cfg,
	const u8 pipe)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	union reg_cnr_cnr_ctrl_sw_hw cnr_ctrl_sw_hw;

	if (!cfg->update)
		return;

	ISP_WR_BITS(cnr, reg_cnr_t, cnr_ctrl_hw_only, cnr_enable, cfg->cnr_enable);
	ISP_WR_BITS(cnr, reg_cnr_t, cnr_ctrl_hw_only, hw_auto_cg_en, cfg->cnr_enable);

	cnr_ctrl_sw_hw.raw = ISP_RD_REG(cnr, reg_cnr_t, cnr_ctrl_sw_hw);
	cnr_ctrl_sw_hw.bits.cnr_cmf_en = cfg->cnr_cmf_en;
	cnr_ctrl_sw_hw.bits.cnr_lca_enable = cfg->cnr_lca_enable;
	cnr_ctrl_sw_hw.bits.cnr_ife2_filter_en = cfg->cnr_ife2_filter_en;
	cnr_ctrl_sw_hw.bits.cnr_chra_en = cfg->cnr_chra_en;
	cnr_ctrl_sw_hw.bits.cnr_m_chra_bypass = cfg->cnr_m_chra_bypass;
	cnr_ctrl_sw_hw.bits.cnr_chra_dbgmode = cfg->cnr_chra_dbgmode;
	cnr_ctrl_sw_hw.bits.cnr_chra_sat_outbld_en = cfg->cnr_chra_sat_outbld_en;

	ISP_WR_REG(cnr, reg_cnr_t, cnr_ctrl_sw_hw, cnr_ctrl_sw_hw.raw);

	if (ctx->isp_pipe_cfg[pipe].cnr_scale_shift != cfg->cnr_scale_shift) {
		if (ctx->isp_pipe_cfg[pipe].cnr_scale_shift == ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift) {
			vi_pr(VI_DBG, "cnr_scale_shift %d -> %d\n",
				ctx->isp_pipe_cfg[pipe].cnr_scale_shift,
				cfg->cnr_scale_shift);
			ctx->isp_pipe_cfg[pipe].cnr_scale_shift = cfg->cnr_scale_shift;
		} else {
			vi_pr(VI_WARN, "cnr change not idle, please wait next frame\n");
		}
	}

	if (!cfg->cnr_enable)
		return;

	ISP_WR_REG(cnr, reg_cnr_t, cnr_subim_outsel, cfg->cnr_subim_outsel);

	ISP_WR_REGS_BURST(cnr, reg_cnr_t, cnr_cmf_ksize,
				cfg->burst0_cfg, cfg->burst0_cfg.cnr_cmf_ksize);
}

void ispblk_ee_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ee_config *cfg)
{
	uintptr_t post_ee = ctx->phys_regs[ISP_BLK_ID_POST_EE];
	union reg_isp_ee_00 reg_00;
	union reg_isp_ee_04 reg_04;
	union reg_isp_ee_0c reg_0c;
	union reg_isp_ee_10 reg_10;

	if (!cfg->update)
		return;

	reg_00.raw = ISP_RD_REG(post_ee, reg_isp_ee_t, reg_00);
	reg_00.bits.ee_enable = cfg->ee_enable;
	reg_00.bits.ee_debug_mode = cfg->ee_debug_mode;
	reg_00.bits.ee_coring_th = cfg->ee_coring_th;
	reg_00.bits.ee_chroma_adptctrl_en = cfg->ee_chroma_adptctrl_en;
	reg_00.bits.hw_auto_cg_en = cfg->ee_enable;
	ISP_WR_REG(post_ee, reg_isp_ee_t, reg_00, reg_00.raw);

	reg_04.bits.ee_overshoot_clip_ratio = cfg->ee_overshoot_clip_ratio;
	reg_04.bits.ee_undershoot_clip_ratio = cfg->ee_undershoot_clip_ratio;
	ISP_WR_REG(post_ee, reg_isp_ee_t, reg_04, reg_04.raw);

	reg_0c.bits.ee_delta_wt_src_opt = cfg->ee_delta_wt_src_opt;
	reg_0c.bits.ee_delta_wt_coring_th = cfg->ee_delta_wt_coring_th;
	reg_0c.bits.ee_delta_wt_gain = cfg->ee_delta_wt_gain;
	reg_0c.bits.ee_noise_level = cfg->ee_noise_level;
	ISP_WR_REG(post_ee, reg_isp_ee_t, reg_0c, reg_0c.raw);

	reg_10.bits.ee_blend_degamma = cfg->ee_blend_degamma;
	reg_10.bits.ee_sharp_str = cfg->ee_sharp_str;
	ISP_WR_REG(post_ee, reg_isp_ee_t, reg_10, reg_10.raw);

	ISP_WR_REGS_BURST(post_ee, reg_isp_ee_t, reg_a4,
			cfg->nlut_burst_cfg, cfg->nlut_burst_cfg.reg_a4);
	ISP_WR_REGS_BURST(post_ee, reg_isp_ee_t, reg_178,
			cfg->chra_burst_cfg, cfg->chra_burst_cfg.reg_178);
}

void ispblk_cacp_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_cacp_config *cfg)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	u16 i;
	union reg_ca_00 ca_00;
	union reg_ca_04 wdata;

	if (!cfg->update)
		return;

	ca_00.raw = ISP_RD_REG(cacp, reg_ca_t, reg_00);
	ca_00.bits.cacp_enable		= cfg->enable;
	ca_00.bits.cacp_mode		= cfg->mode; // 0 CA mode, 1 CP mode
	ca_00.bits.cacp_iso_ratio	= cfg->iso_ratio;
	ISP_WR_REG(cacp, reg_ca_t, reg_00, ca_00.raw);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 1);
	if (cfg->mode == 0) {
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = cfg->ca_y_ratio_lut[i];
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < 256; i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ((cfg->cp_y_lut[i] << 16) |
					(cfg->cp_u_lut[i] << 8) | (cfg->cp_v_lut[i]));
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 0);
}

void ispblk_ca2_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ca2_config *cfg)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	if (!cfg->update)
		return;

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, cfg->enable);

	if (!cfg->enable)
		return;

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_0, cfg->lut_in[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_1, cfg->lut_in[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_2, cfg->lut_in[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_3, cfg->lut_in[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_4, cfg->lut_in[4]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_5, cfg->lut_in[5]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_0, cfg->lut_out[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_1, cfg->lut_out[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_2, cfg->lut_out[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_3, cfg->lut_out[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_4, cfg->lut_out[4]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_5, cfg->lut_out[5]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_0, cfg->lut_slp[0]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_1, cfg->lut_slp[1]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_2, cfg->lut_slp[2]);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_3, cfg->lut_slp[3]);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_24, ca_lite_lut_slp_4, cfg->lut_slp[4]);
}

void ispblk_ycur_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_ycur_config *cfg)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	u16 i = 0;

	union reg_isp_ycurv_ycur_prog_data reg_data;
	union reg_isp_ycurv_ycur_prog_ctrl prog_ctrl;

	if (!cfg->update)
		return;

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, cfg->enable);

	if (cfg->enable) {
		prog_ctrl.raw = ISP_RD_REG(ycur, reg_isp_ycurv_t, ycur_prog_ctrl);
		prog_ctrl.bits.ycur_wsel = prog_ctrl.bits.ycur_wsel ^ 1;
		prog_ctrl.bits.ycur_prog_en = 1;
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, prog_ctrl.raw);

		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_addr, 0);
		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_w, 1);
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_max, cfg->lut_256);

		for (i = 0; i < 64; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.ycur_data_e = cfg->lut[i];
			reg_data.bits.ycur_data_o = cfg->lut[i + 1];
			reg_data.bits.ycur_w = 1;
			ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_data, reg_data.raw);
		}

		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, prog_ctrl.bits.ycur_wsel);
		ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 0);
	}
}

void ispblk_post_rawtop_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_post_tun_cfg *post_tun,
	const u8 pipe)
{
	//rawtop
	struct sop_vip_isp_wbg_config *wbg_cfg;
	struct sop_vip_isp_fusion_config *fusion_cfg;
	struct sop_vip_isp_map_curve_config *map_curve_cfg;
	struct sop_vip_isp_dpc_config *dpc_cfg;
	struct sop_vip_isp_ge_config *ge_cfg;
	struct sop_vip_isp_af_config *af_cfg;
	struct sop_vip_isp_bnr_config *bnr_cfg;
	struct sop_vip_isp_lscr_config *lscr_cfg;
	struct sop_vip_isp_drc_config *drc_cfg;
	struct sop_vip_isp_demosiac_config *demosiac_cfg;

	wbg_cfg = &post_tun->wbg_cfg;
	ispblk_blc_dg_wb_tun_cfg(ctx, wbg_cfg);

	fusion_cfg = &post_tun->fusion_cfg;
	ispblk_fusion_tun_cfg(ctx, fusion_cfg);

	map_curve_cfg = &post_tun->map_curve_cfg;
	ispblk_map_curve_tun_cfg(ctx, map_curve_cfg);

	dpc_cfg = &post_tun->dpc_cfg;
	ispblk_dpc_tun_cfg(ctx, dpc_cfg);

	ge_cfg = &post_tun->ge_cfg;
	ispblk_ge_tun_cfg(ctx, ge_cfg);

	af_cfg = &post_tun->af_cfg;
	ispblk_af_tun_cfg(ctx, af_cfg);

	bnr_cfg = &post_tun->bnr_cfg;
	ispblk_bnr_tun_cfg(ctx, bnr_cfg);

	lscr_cfg = &post_tun->lscr_cfg;
	ispblk_lscr_tun_cfg(ctx, lscr_cfg);

	drc_cfg = &post_tun->drc_cfg;
	ispblk_drc_tun_cfg(ctx, drc_cfg);

	demosiac_cfg = &post_tun->demosiac_cfg;
	ispblk_demosiac_tun_cfg(ctx, demosiac_cfg);
}

void ispblk_post_rgbtop_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_post_tun_cfg *post_tun,
	const u8 pipe)
{
	//rgb_top
	struct sop_vip_isp_ee_ext_config *ee_ext_cfg; // New
	struct sop_vip_isp_pfr_config *pfr_cfg; // New
	struct sop_vip_isp_ccm_config *ccm_cfg; // A2 change
	struct sop_vip_isp_gamma_config *gamma_cfg; // A2 change
	struct sop_vip_isp_clut_config *clut_cfg; // A2
	struct sop_vip_isp_csc_config *csc_cfg; // A2

	ee_ext_cfg = &post_tun->ee_ext_cfg;
	ispblk_ee_ext_tun_cfg(ctx, ee_ext_cfg);

	pfr_cfg = &post_tun->pfr_cfg;
	ispblk_pfr_tun_cfg(ctx, pfr_cfg);

	ccm_cfg = &post_tun->ccm_cfg;
	ispblk_ccm_tun_cfg(ctx, ccm_cfg);

	gamma_cfg = &post_tun->gamma_cfg;
	ispblk_gamma_tun_cfg(ctx, gamma_cfg);

	clut_cfg = &post_tun->clut_cfg;
	ispblk_clut_tun_cfg(ctx, clut_cfg, pipe);

	csc_cfg = &post_tun->csc_cfg;
	ispblk_csc_tun_cfg(ctx, csc_cfg);
}

void ispblk_post_yuvtop_tun_cfg(
	struct isp_ctx *ctx,
	struct sop_vip_isp_post_tun_cfg *post_tun,
	const u8 pipe)
{
	//yuvtop
	struct sop_vip_isp_pre_ee_config *pre_ee_cfg; // New
	struct sop_vip_isp_ldci_config *ldci_cfg; // New
	struct sop_vip_isp_dci_config *dci_cfg; // New
	struct sop_vip_isp_tnr_config *tnr_cfg; // New
	struct sop_vip_isp_cnr_config *cnr_cfg; // New
	struct sop_vip_isp_ee_config *ee_cfg; // New
	struct sop_vip_isp_cacp_config *cacp_cfg; // A2
	struct sop_vip_isp_ca2_config *ca2_cfg; // A2
	struct sop_vip_isp_ycur_config *ycur_cfg; // A2

	pre_ee_cfg = &post_tun->pre_ee_cfg;
	ispblk_pre_ee_tun_cfg(ctx, pre_ee_cfg);

	ldci_cfg = &post_tun->ldci_cfg;
	ispblk_ldci_tun_cfg(ctx, ldci_cfg);

	dci_cfg = &post_tun->dci_cfg;
	ispblk_dci_tun_cfg(ctx, dci_cfg);

	tnr_cfg = &post_tun->tnr_cfg;
	ispblk_tnr_tun_cfg(ctx, tnr_cfg, pipe);

	cnr_cfg = &post_tun->cnr_cfg;
	ispblk_cnr_tun_cfg(ctx, cnr_cfg, pipe);

	ee_cfg = &post_tun->ee_cfg;
	ispblk_ee_tun_cfg(ctx, ee_cfg);

	cacp_cfg = &post_tun->cacp_cfg;
	ispblk_cacp_tun_cfg(ctx, cacp_cfg);

	ca2_cfg = &post_tun->ca2_cfg;
	ispblk_ca2_tun_cfg(ctx, ca2_cfg);

	ycur_cfg = &post_tun->ycur_cfg;
	ispblk_ycur_tun_cfg(ctx, ycur_cfg);
}

void postraw_tuning_update(
	struct isp_ctx *ctx,
	const u8 pipe)
{
	u8 tun_idx = 0;
	static int stop_update = -1;
	struct sop_vip_isp_post_cfg     *post_cfg;
	struct sop_vip_isp_post_tun_cfg *post_tun;

	post_cfg = (struct sop_vip_isp_post_cfg *)tuning_buf_addr.post_vir[pipe];
	tun_idx  = post_cfg->tun_idx;

	vi_pr(VI_DBG, "Postraw_(%d->%d) tuning update(%d):idx(%d)\n", ctx->isp_pipe_cfg[pipe].bind_raw,
			pipe, post_cfg->tun_update[tun_idx], tun_idx);

	if ((tun_idx >= TUNING_NODE_NUM) || (post_cfg->tun_update[tun_idx] == 0))
		return;

	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor &&
		ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ONLINE) {
		vi_pr(VI_DBG, "Postraw_%d stop tuning_update for yuv sensor\n", pipe);
		return;
	}

	post_tun = &post_cfg->tun_cfg[tun_idx];

	if (tuning_dis[2]) {
		if (tuning_dis[0] == 0) {
			vi_pr(VI_DBG, "raw_%d stop tuning_update immediately\n", pipe);
			return;
		} else if ((tuning_dis[0] - 1) == pipe) {//stop on next
			if (stop_update > 0) {
				vi_pr(VI_DBG, "raw_%d stop tuning_update\n", pipe);
				return;
			}
			stop_update = 1;
		} else {//must update tuning buf for sensor, it's will be not trrigered
			stop_update = 0;
		}
	} else
		stop_update = 0;


	if (!ctx->isp_pipe_cfg[pipe].is_yuv_sensor) {
		ispblk_post_rawtop_tun_cfg(ctx, post_tun, pipe);
		ispblk_post_rgbtop_tun_cfg(ctx, post_tun, pipe);
	}

	ispblk_post_yuvtop_tun_cfg(ctx, post_tun, pipe);
}