#include "vi_reg.h"
#include "ion.h"
#include "cmdq.h"
#include "vi_rgb_ip_ctrl.h"

/****************************************************************************
 * Global parameters
 ****************************************************************************/
#define LTM_DARK_TONE_LUT_SIZE   0x100
#define LTM_BRIGHT_TONE_LUT_SIZE 0x100
#define LTM_GLOBAL_LUT_SIZE      0x100

/*******************************************************************************
 *	RGB IPs config
 ******************************************************************************/
void ispblk_pfr_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t pfr = ctx->phys_regs[ISP_BLK_ID_PFR];

	ISP_WR_BITS(pfr, reg_pfr_t, pfr_reg0, hw_auto_cg_en, 1);
	ISP_WR_BITS(pfr, reg_pfr_t, pfr_reg0, pfr_en, en);
}

void ispblk_edge_ext_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee_ext = ctx->phys_regs[ISP_BLK_ID_PRE_EE_EXT];

	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, hw_auto_cg_en, 1);
	ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_enable, en);
}

void ispblk_ccm_config(struct isp_ctx *ctx, enum isp_blk_id_t blk_id, bool en, struct isp_ccm_cfg *cfg)
{
	uintptr_t ccm = ctx->phys_regs[blk_id];

	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_shdw_sel, 1);
	ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_enable, en);

	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_00, cfg->coef[0][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_01, cfg->coef[0][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_02, cfg->coef[0][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_10, cfg->coef[1][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_11, cfg->coef[1][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_12, cfg->coef[1][2]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_20, cfg->coef[2][0]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_21, cfg->coef[2][1]);
	ISP_WR_REG(ccm, reg_isp_ccm_t, ccm_22, cfg->coef[2][2]);
}

void ispblk_gamma_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data, u8 inv)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	int16_t i;
	union reg_isp_gamma_prog_data reg_data;
	union reg_isp_gamma_prog_ctrl prog_ctrl;

	prog_ctrl.raw = ISP_RD_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl);
	prog_ctrl.bits.gamma_wsel    = sel;
	prog_ctrl.bits.gamma_prog_en = 1;
	prog_ctrl.bits.gamma_prog_1to3_en = 1;
	ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_ctrl, prog_ctrl.raw);

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_addr, 0);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_st_addr, gamma_st_w, 1);

	if (inv) {
		for (i = 255; i >= 0; i -= 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			reg_data.bits.gamma_w = 1;
			ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
		}

		// set max to 0
		ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_max, 0);
	} else {
		for (i = 0; i < 256; i += 2) {
			reg_data.raw = 0;
			reg_data.bits.gamma_data_e = data[i];
			reg_data.bits.gamma_data_o = data[i + 1];
			reg_data.bits.gamma_w = 1;
			ISP_WR_REG(gamma, reg_isp_gamma_t, gamma_prog_data, reg_data.raw);
		}
	}

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_rsel, sel);
	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_prog_ctrl, gamma_prog_en, 0);
}

void ispblk_gamma_enable(struct isp_ctx *ctx, bool enable)
{
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];

	ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_ctrl, gamma_enable, enable);
}

/**
 * ispblk_rgbdither_config - setup rgb dither.
 *
 * @param ctx: global settings
 * @param en: rgb dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
void ispblk_rgbdither_config(struct isp_ctx *ctx, bool en, bool mod_en,
			    bool histidx_en, bool fmnum_en)
{
	uintptr_t rgbdither = ctx->phys_regs[ISP_BLK_ID_RGB_DITHER];
	union reg_isp_rgb_dither_rgb_dither reg;

	reg.raw = 0;
	reg.bits.rgb_dither_enable = en;
	reg.bits.rgb_dither_mod_en = mod_en;
	reg.bits.rgb_dither_histidx_en = histidx_en;
	reg.bits.rgb_dither_fmnum_en = fmnum_en;
	reg.bits.rgb_dither_shdw_sel = 1;
	reg.bits.crop_widthm1 = ctx->cfg_info.img_width - 1;
	reg.bits.crop_heightm1 = ctx->cfg_info.img_height - 1;

	ISP_WR_REG(rgbdither, reg_isp_rgb_dither_t, rgb_dither, reg.raw);
}

void ispblk_clut_cmdq_config(struct isp_ctx *ctx, const u8 pipe, bool en,
			int16_t *r_lut, int16_t *g_lut, int16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	u16 r_idx, g_idx, b_idx;
	union reg_isp_clut_ctrl      ctrl;
	union reg_isp_clut_prog_addr prog_addr;
	union reg_isp_clut_prog_data prog_data;
	u32 clut_phy_reg = ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CLUT;
	u32 idx = 0;
	u16 cmd_idx = 0;

	union cmdq_set *cmd_start = (union cmdq_set *)ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr;

	base_ion_cache_invalidate(ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr,
				  ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr,
				  ctx->isp_pipe_cfg[pipe].cmdq_buf.buf_size);

	ctrl.raw = ISP_RD_REG(clut, reg_isp_clut_t, clut_ctrl);
	ctrl.bits.prog_en = 1;

	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 clut_phy_reg + _OFST(reg_isp_clut_t, clut_ctrl),
			 ctrl.raw);

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				prog_addr.raw = 0;
				prog_addr.bits.sram_r_idx = r_idx;
				prog_addr.bits.sram_g_idx = g_idx;
				prog_addr.bits.sram_b_idx = b_idx;
				cmdq_set_package(&cmd_start[cmd_idx++].reg,
						 clut_phy_reg + _OFST(reg_isp_clut_t, clut_prog_addr),
						 prog_addr.raw);

				prog_data.raw = 0;
				prog_data.bits.sram_wdata = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				prog_data.bits.sram_wr = 1;
				cmdq_set_package(&cmd_start[cmd_idx++].reg,
						 clut_phy_reg + _OFST(reg_isp_clut_t, clut_prog_data),
						 prog_data.raw);
			}
		}
	}

	ctrl.bits.clut_enable = en;
	ctrl.bits.prog_en = 0;
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			clut_phy_reg + _OFST(reg_isp_clut_t, clut_ctrl),
			ctrl.raw);

	base_ion_cache_flush(ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr,
				ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr,
				ctx->isp_pipe_cfg[pipe].cmdq_buf.buf_size);

	ctx->isp_pipe_cfg[pipe].cmdq_buf.cmd_idx = cmd_idx;
}

void ispblk_clut_config(struct isp_ctx *ctx, bool en, bool is_rdma_mode,
		uint16_t *r_lut, uint16_t *g_lut, uint16_t *b_lut)
{
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	u16 r_idx, g_idx, b_idx;
	union reg_isp_clut_ctrl      ctrl;
	union reg_isp_clut_prog_addr prog_addr;
	union reg_isp_clut_prog_data prog_data;
	u32 idx = 0;
	uint32_t *clut_phy = NULL;
	u32 raw_num = ctx->cfg_info.raw_num;

	ISP_WR_BITS(clut, reg_isp_clut_t, lut_fill_select, fill_sel_sw, is_rdma_mode);

	if (!is_rdma_mode) {
		ctrl.raw = ISP_RD_REG(clut, reg_isp_clut_t, clut_ctrl);
		ctrl.bits.prog_en = 1;
		ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);
	}

	for (b_idx = 0; b_idx < 17; b_idx++) {
		for (g_idx = 0; g_idx < 17; g_idx++) {
			for (r_idx = 0; r_idx < 17; r_idx++) {
				idx = b_idx * 289 + g_idx * 17 + r_idx;

				if (!is_rdma_mode) {
					prog_addr.raw = 0;
					prog_addr.bits.sram_r_idx = r_idx;
					prog_addr.bits.sram_g_idx = g_idx;
					prog_addr.bits.sram_b_idx = b_idx;
					ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_addr, prog_addr.raw);

					prog_data.raw = 0;
					prog_data.bits.sram_wdata = b_lut[idx] + (g_lut[idx] << 10) +
								    (r_lut[idx] << 20);
					prog_data.bits.sram_wr = 1;
					ISP_WR_REG(clut, reg_isp_clut_t, clut_prog_data, prog_data.raw);
				} else {
					clut_phy = (uint32_t *)osal_phys_to_virt(ctx->isp_bufpool[raw_num].clut);
					clut_phy[idx] = b_lut[idx] + (g_lut[idx] << 10) + (r_lut[idx] << 20);
				}
			}
		}
	}

	if (!is_rdma_mode)
		ctrl.bits.prog_en = 0;

	ctrl.bits.clut_enable = en;
	ISP_WR_REG(clut, reg_isp_clut_t, clut_ctrl, ctrl.raw);
}

void ispblk_csc_config(struct isp_ctx *ctx)
{
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];

	ISP_WR_BITS(csc, reg_isp_csc_t, reg_0, csc_enable, 1);
}