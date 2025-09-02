#include "vi_reg.h"
#include "vi_yuv_ip_ctrl.h"
#include "comm_math.h"

/****************************************************************************
 * FBC_CONFIG
 ****************************************************************************/
#define DEFAULT_K	2
#define CPLX_SHIFT	3
#define PEN_POS_SHIFT	4

/*TODO maybe need change by Product*/
#define TARGET_CR	80//, 55, 68, 80, 93, 100

struct vi_fbc_cfg fbc_cfg = {
	.cu_size	= 8,
	.target_cr	= TARGET_CR,
	.is_lossless	= 0,
	.y_bs_size	= 0,
	.c_bs_size	= 0,
	.y_buf_size	= 0,
	.c_buf_size	= 0,
};

/***************************************************************************
 * CA global setting
 ***************************************************************************/
u8 ca_y_lut[] = {
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
};

u8 cp_y_lut[] = {
0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

u8 cp_u_lut[] = {
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
};

u8 cp_v_lut[] = {
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153, 153,
};

/*******************************************************************************
 *	YUV IPs config
 ******************************************************************************/

/**
 * ispblk_yuvdither_config - setup yuv dither.
 *
 * @param ctx: global settings
 * @param sel: y(0)/uv(1)
 * @param en: dither enable
 * @param mod_en: 0: mod 32, 1: mod 29
 * @param histidx_en: refer to previous dither number enable
 * @param fmnum_en: refer to frame index enable
 */
int ispblk_yuvdither_config(struct isp_ctx *ctx, u8 sel, bool en,
			    bool mod_en, bool histidx_en, bool fmnum_en)
{
	uintptr_t dither = ctx->phys_regs[ISP_BLK_ID_YUV_DITHER];

	if (sel == 0) {
		union reg_isp_yuv_dither_y_dither reg;

		reg.raw = 0;
		reg.bits.y_dither_enable = en;
		reg.bits.y_dither_mod_enable = mod_en;
		reg.bits.y_dither_histidx_enable = histidx_en;
		reg.bits.y_dither_fmnum_enable = fmnum_en;
		reg.bits.y_dither_shdw_sel = 1;
		reg.bits.y_dither_widthm1 = ctx->cfg_info.img_width - 1;
		reg.bits.y_dither_heightm1 = ctx->cfg_info.img_height - 1;

		ISP_WR_REG(dither, reg_isp_yuv_dither_t, y_dither, reg.raw);
	} else if (sel == 1) {
		union reg_isp_yuv_dither_uv_dither reg;

		reg.raw = 0;
		reg.bits.uv_dither_enable = en;
		reg.bits.uv_dither_mod_enable = mod_en;
		reg.bits.uv_dither_histidx_enable = histidx_en;
		reg.bits.uv_dither_fmnum_enable = fmnum_en;
		reg.bits.uv_dither_widthm1 = (ctx->cfg_info.img_width >> 1) - 1;
		reg.bits.uv_dither_heightm1 = (ctx->cfg_info.img_height >> 1) - 1;

		ISP_WR_REG(dither, reg_isp_yuv_dither_t, uv_dither, reg.raw);
	}

	return 0;
}

int ispblk_ee_front_back_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee_front = ctx->phys_regs[ISP_BLK_ID_PRE_EE_FRONT];
	uintptr_t ee_back = ctx->phys_regs[ISP_BLK_ID_PRE_EE_BACK];

	ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, hw_auto_cg_en, 1);
	ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, ee_enable, en);

	ISP_WR_BITS(ee_back, reg_ee_add_t, ee_add_reg0, hw_auto_cg_en, 1);
	ISP_WR_BITS(ee_back, reg_ee_add_back_t, ee_add_b_reg0, ee_enable, en);

	return 0;
}

void ispblk_dci_map_config(struct isp_ctx *ctx, u8 sel, u16 *data)
{
	uintptr_t ldci_map = ctx->phys_regs[ISP_BLK_ID_LDCI_MAP_CORE];
	union reg_ldci_map_lut_ldci_map_ctl ldci_map_ctl;
	union reg_ldci_map_lut_ldci_rw_ctl ldci_rw_ctl;
	int i = 0;

	//step 1 config map enable
	ldci_map_ctl.raw = 0;
	ldci_map_ctl.bits.shdw_sel = 1;
	ldci_map_ctl.bits.map_en = 1;
	ldci_map_ctl.bits.map_prog_hdk_dis = 0;
	ldci_map_ctl.bits.ck_enable = 1;
	ISP_WR_REG(ldci_map, reg_ldci_map_lut_t, ldci_map_ctl, ldci_map_ctl.raw);

	//step 2 program enable
	ISP_WR_BITS(ldci_map, reg_ldci_map_lut_t, ldci_map_ctl, lut_prog_en, 1);

	//step 3 config lut sel
	ldci_rw_ctl.raw = ISP_RD_REG(ldci_map, reg_ldci_map_lut_t, ldci_rw_ctl);
	ldci_rw_ctl.bits.lut_wsel = sel;
	ldci_rw_ctl.bits.lut_rsel = sel;
	ISP_WR_REG(ldci_map, reg_ldci_map_lut_t, ldci_rw_ctl, ldci_rw_ctl.raw);

	//step 4 start w1t
	ISP_WR_BITS(ldci_map, reg_ldci_map_lut_t, ldci_rw_ctl, lut_st_w1t, 1);

	//step 5 start w1t
	for (i = 0; i < 256; i++) {
		ISP_WR_BITS(ldci_map, reg_ldci_map_lut_t, ldci_rw_ctl, lut_wdata, data[i]);
		ISP_WR_BITS(ldci_map, reg_ldci_map_lut_t, ldci_rw_ctl, lut_w1t, 1);
	}

	ISP_WR_BITS(ldci_map, reg_ldci_map_lut_t, ldci_map_ctl, lut_prog_en, 0);
}

void ispblk_ldci_config(struct isp_ctx *ctx, bool ldci_en, bool dci_en)
{
	uintptr_t ldci = ctx->phys_regs[ISP_BLK_ID_LDCI];
	union reg_ldci_ldci_ctl ldci_ctl;
	u32 blk_num_x = 16, blk_num_y = 8; //blk_num [4,63]
	u32 blk_size_x, blk_size_y;
	u32 reciprocal, reciprocal_h, reciprocal_v;
	enum sop_isp_raw pipe = ctx->cfg_info.pipe;
	u32 img_w = ctx->isp_pipe_cfg[pipe].crop.w;
	u32 img_h = ctx->isp_pipe_cfg[pipe].crop.h;

	ldci_ctl.raw = ISP_RD_REG(ldci, reg_ldci_t, ldci_ctl);
	ldci_ctl.bits.ldci_enable = ldci_en;
	ldci_ctl.bits.dci_enable = dci_en;
	ISP_WR_REG(ldci, reg_ldci_t, ldci_ctl, ldci_ctl.raw);
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_hw_ctl, ldci_force_ck_en, 1);

	blk_size_x = img_w / blk_num_x;
	blk_size_y = img_h / blk_num_y;
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_blk_num, ldci_blk_num_h, blk_num_x);
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_blk_num, ldci_blk_num_v, blk_num_y);
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_blk_size, ldci_blk_width, blk_size_x);
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_blk_size, ldci_blk_height, blk_size_y);

	reciprocal = (1 << 27) / (blk_size_x * blk_size_y);
	ISP_WR_REG(ldci, reg_ldci_t, ldci_reciprocal_1, reciprocal);

	reciprocal_h = (1 << 16) / blk_size_x;
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_reciprocal_0, ldci_reciprocal_h, reciprocal_h);

	reciprocal_v = (1 << 16) / blk_size_y;
	ISP_WR_BITS(ldci, reg_ldci_t, ldci_reciprocal_0, ldci_reciprocal_v, reciprocal_v);
}

int ispblk_ee_back_config(struct isp_ctx *ctx, bool en)
{
	// uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_PRE_EE_BACK];

	return 0;
}

int ispblk_post_ee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t post_ee = ctx->phys_regs[ISP_BLK_ID_POST_EE];

	ISP_WR_BITS(post_ee, reg_isp_ee_t, reg_00, hw_auto_cg_en, 1);
	ISP_WR_BITS(post_ee, reg_isp_ee_t, reg_00, ee_enable, en);

	return 0;
}

void ispblk_mctf_config(struct isp_ctx *ctx, bool en, u8 test_case)
{
	uintptr_t mctf = ctx->phys_regs[ISP_BLK_ID_TNR];

	ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, tdnr_enable, en ? 0x3 : 0);
	ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, dma_enable, en ? 0x7FF : 0x0);

	ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, hw_auto_cg_en_blend, 1);
	ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, hw_auto_cg_en_all, 1);

	ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_2, bypass_v, en);
}

void ispblk_fbcd_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbcd = ctx->phys_regs[ISP_BLK_ID_FBCD];
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union reg_fbcd_24	d_reg_24;
	union reg_fbcd_28	d_reg_28;
	union reg_fbce_10	reg_10;
	union reg_fbce_20	reg_20;

	if (en) {
		reg_10.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_10);
		reg_20.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_20);

		d_reg_24.raw = ISP_RD_REG(fbcd, reg_fbcd_t, reg_24);
		d_reg_24.bits.y_lossless		= reg_10.bits.y_lossless;
		d_reg_24.bits.y_base_qdpcm_q		= reg_10.bits.y_base_qdpcm_q;
		d_reg_24.bits.y_base_pcm_bd_minus2	= reg_10.bits.y_base_pcm_bd_minus2;
		d_reg_24.bits.y_default_gr_k		= DEFAULT_K;
		ISP_WR_REG(fbcd, reg_fbcd_t, reg_24, d_reg_24.raw);

		d_reg_28.raw = ISP_RD_REG(fbcd, reg_fbcd_t, reg_28);
		d_reg_28.bits.c_lossless		= reg_20.bits.c_lossless;
		d_reg_28.bits.c_base_qdpcm_q		= reg_20.bits.c_base_qdpcm_q;
		d_reg_28.bits.c_base_pcm_bd_minus2	= reg_20.bits.c_base_pcm_bd_minus2;
		d_reg_28.bits.c_default_gr_k		= DEFAULT_K;
		ISP_WR_REG(fbcd, reg_fbcd_t, reg_28, d_reg_28.raw);
	}

	ISP_WR_BITS(fbcd, reg_fbcd_t, reg_00, fbcd_en, en);
}

void ispblk_fbce_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t fbce = ctx->phys_regs[ISP_BLK_ID_FBCE];
	union reg_fbce_10	reg_10;
	union reg_fbce_14	reg_14;
	union reg_fbce_1c	reg_1c;
	union reg_fbce_20	reg_20;
	union reg_fbce_24	reg_24;
	union reg_fbce_2c	reg_2c;
	union reg_fbce_30	reg_30;
	union reg_fbce_34	reg_34;
	u8 pipe = ctx->cfg_info.pipe;

	u32 img_w = ctx->isp_pipe_cfg[pipe].crop.w;

	u32 cu_md_bit = fbc_cfg.is_lossless ? 1 : 3;
	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);
	u32 cu_target_bit = total_line_bit_budget / line_cu_num;
	u32 base_dpcm_q = ((cu_target_bit <= 27) ? 1 : 0);
	u32 base_pcm_bd = (cu_target_bit - cu_md_bit) / fbc_cfg.cu_size;
	u32 min_cu_bit = fbc_cfg.is_lossless ? max_cu_bit : (base_pcm_bd * fbc_cfg.cu_size + cu_md_bit);

	if (base_pcm_bd < 2)
		base_pcm_bd = 2;
	else if (base_pcm_bd > 8)
		base_pcm_bd = 8;

	if (en) {
		reg_14.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_14);
		reg_14.bits.y_max_cu_bit		= max_cu_bit;
		reg_14.bits.y_min_cu_bit		= min_cu_bit;
		ISP_WR_REG(fbce, reg_fbce_t, reg_14, reg_14.raw);

		reg_1c.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_1c);
		reg_1c.bits.y_total_line_bit_budget	= total_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_1c, reg_1c.raw);

		reg_10.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_10);
		reg_10.bits.y_base_qdpcm_q		= base_dpcm_q;
		reg_10.bits.y_base_pcm_bd_minus2	= base_pcm_bd - 2;
		reg_10.bits.y_cplx_shift		= CPLX_SHIFT;
		reg_10.bits.y_pen_pos_shift		= PEN_POS_SHIFT;
		reg_10.bits.y_default_gr_k		= DEFAULT_K;
		reg_10.bits.y_lossless			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, reg_fbce_t, reg_10, reg_10.raw);

		reg_30.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_30);
		reg_30.bits.y_total_first_line_bit_budget = total_first_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_30, reg_30.raw);

		reg_24.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_24);
		reg_24.bits.c_max_cu_bit		= max_cu_bit;
		reg_24.bits.c_min_cu_bit		= min_cu_bit;
		ISP_WR_REG(fbce, reg_fbce_t, reg_24, reg_24.raw);

		reg_2c.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_2c);
		reg_2c.bits.c_total_line_bit_budget	= total_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_2c, reg_2c.raw);

		reg_20.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_20);
		reg_20.bits.c_base_qdpcm_q		= 0;
		reg_20.bits.c_base_pcm_bd_minus2	= base_pcm_bd - 2;
		reg_20.bits.c_cplx_shift		= CPLX_SHIFT;
		reg_20.bits.c_pen_pos_shift		= PEN_POS_SHIFT;
		reg_20.bits.c_default_gr_k		= DEFAULT_K;
		reg_20.bits.c_lossless			= fbc_cfg.is_lossless;
		ISP_WR_REG(fbce, reg_fbce_t, reg_20, reg_20.raw);

		reg_34.raw = ISP_RD_REG(fbce, reg_fbce_t, reg_34);
		reg_34.bits.c_total_first_line_bit_budget = total_first_line_bit_budget;
		ISP_WR_REG(fbce, reg_fbce_t, reg_34, reg_34.raw);
	}

	ISP_WR_BITS(fbce, reg_fbce_t, reg_00, fbce_en, en);
}

void ispblk_cnr_config(struct isp_ctx *ctx, bool en, bool pfc_en, u8 str_mode, u8 test_case)
{
	uintptr_t cnr = ctx->phys_regs[ISP_BLK_ID_CNR];
	union reg_cnr_cnr_ctrl_hw_only cnr_ctrl_hw;
	union reg_cnr_cnr_ctrl_sw_hw cnr_ctrl_sw_hw;
	union reg_cnr_scl_down_ctrl scl_down_ctrl;
	int pipe = ctx->cfg_info.pipe;
	u32 cnr_pre_scale_shift = ctx->isp_pipe_cfg[pipe].cnr_pre_scale_shift;
	u32 cnr_cur_scale_shift = ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift;
	int lca_src_w, lca_src_h, lca_sub_w, lca_sub_h, fact;

	cnr_ctrl_hw.raw = ISP_RD_REG(cnr, reg_cnr_t, cnr_ctrl_hw_only);
	cnr_ctrl_hw.bits.cnr_enable = en;
	cnr_ctrl_hw.bits.hw_auto_cg_en = 1;
	ISP_WR_REG(cnr, reg_cnr_t, cnr_ctrl_hw_only, cnr_ctrl_hw.raw);

	cnr_ctrl_sw_hw.raw = ISP_RD_REG(cnr, reg_cnr_t, cnr_ctrl_sw_hw);
	cnr_ctrl_sw_hw.bits.cnr_cmf_en = en;
	cnr_ctrl_sw_hw.bits.cnr_lca_enable = en;
	cnr_ctrl_sw_hw.bits.cnr_ife2_filter_en = en;
	cnr_ctrl_sw_hw.bits.cnr_chra_en = en;
	cnr_ctrl_sw_hw.bits.cnr_chra_sat_outbld_en = 0;

	ISP_WR_REG(cnr, reg_cnr_t, cnr_ctrl_sw_hw, cnr_ctrl_sw_hw.raw);

	lca_src_w = ctx->isp_pipe_cfg[pipe].crop.w >> 1;
	lca_src_h = ctx->isp_pipe_cfg[pipe].crop.h;
	lca_sub_w = ISP_ALIGN(lca_src_w, 1 << cnr_pre_scale_shift) >> cnr_pre_scale_shift;
	lca_sub_h = ISP_ALIGN(lca_src_h, 1 << cnr_pre_scale_shift) >> cnr_pre_scale_shift;
	fact = 0x10000 >> cnr_pre_scale_shift;

	/*if w or h is odd, must set scl_down_ctrl_en */
	scl_down_ctrl.bits.scl_down_ctrl_en = 1;
	scl_down_ctrl.bits.scl_down_mode = (cnr_cur_scale_shift - 2);
	ISP_WR_REG(cnr, reg_cnr_t, scl_down_ctrl, scl_down_ctrl.raw);

	ISP_WR_BITS(cnr, reg_cnr_t, mmp_scl_in_size, mmp_scl_in_imgw, ctx->isp_pipe_cfg[pipe].crop.w >> 2);
	ISP_WR_BITS(cnr, reg_cnr_t, mmp_scl_in_size, mmp_scl_in_imgh, ctx->isp_pipe_cfg[pipe].crop.h >> 2);

	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_h_sfact, fact);
	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_v_sfact, fact);

	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_src_img_size_h, lca_src_w);
	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_src_img_size_v, lca_src_h);
	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_sub_img_size_h, lca_sub_w);
	ISP_WR_REG(cnr, reg_cnr_t, cnr_lca_sub_img_size_v, lca_sub_h);
}

int ispblk_postee_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ee = ctx->phys_regs[ISP_BLK_ID_PRE_EE_BACK];
	union reg_isp_ee_00  reg_0;

	reg_0.raw = ISP_RD_REG(ee, reg_isp_ee_t, reg_00);
	reg_0.bits.ee_enable = en;
	ISP_WR_REG(ee, reg_isp_ee_t, reg_00, reg_0.raw);

	return 0;
}

void ispblk_ca_config(struct isp_ctx *ctx, bool en, u8 mode)
{
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	u16 i = 0;
	union reg_ca_04 wdata;

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_enable, en);
	// 0 CA mode, 1 Cp mode
	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mode, mode);

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_iso_ratio, 64);

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 1);

	if (mode == 0) {
		for (i = 0; i < sizeof(ca_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ca_y_lut[i];
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	} else { //cp mode
		for (i = 0; i < sizeof(cp_y_lut) / sizeof(u8); i++) {
			wdata.raw = 0;
			wdata.bits.cacp_mem_d = ((cp_v_lut[i]) | (cp_u_lut[i] << 8) | (cp_y_lut[i] << 16));
			wdata.bits.cacp_mem_w = 1;
			ISP_WR_REG(cacp, reg_ca_t, reg_04, wdata.raw);
		}
	}

	ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_mem_sw_mode, 0);
}

void ispblk_ca_lite_config(struct isp_ctx *ctx, bool en)
{
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, en);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_0, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_04, ca_lite_lut_in_1, 0x80);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_2, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_08, ca_lite_lut_in_3, 0x100);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_4, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_0c, ca_lite_lut_in_5, 0x100);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_0, 0x100);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_10, ca_lite_lut_out_1, 0x80);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_2, 0x40);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_14, ca_lite_lut_out_3, 0x40);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_4, 0x40);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_18, ca_lite_lut_out_5, 0x40);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_0, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_1c, ca_lite_lut_slp_1, 0x0);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_2, 0x0);
	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_20, ca_lite_lut_slp_3, 0x0);

	ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_24, ca_lite_lut_slp_4, 0x0);
}

void ispblk_ycur_config(struct isp_ctx *ctx, bool en, u8 sel, u16 *data)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	u16 i;
	union reg_isp_ycurv_ycur_prog_data reg_data;

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 1);

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_wsel, sel);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_addr, 0);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_st_addr, ycur_st_w, 1);
	ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_max, data[64]);
	for (i = 0; i < 64; i += 2) {
		reg_data.raw = 0;
		reg_data.bits.ycur_data_e = data[i];
		reg_data.bits.ycur_data_o = data[i + 1];
		reg_data.bits.ycur_w = 1;
		ISP_WR_REG(ycur, reg_isp_ycurv_t, ycur_prog_data, reg_data.raw);
	}

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, sel);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_prog_en, 0);
}

void ispblk_ycur_enable(struct isp_ctx *ctx, bool enable, u8 sel)
{
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];

	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, enable);
	ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_prog_ctrl, ycur_rsel, sel);
}
