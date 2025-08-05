#include "vi_reg.h"

#include "vi_drv.h"
#include "ion.h"
#include "cmdq.h"
#include "vi_defines.h"
#include "vi_fe_ip_ctrl.h"
#include "vi_raw_ip_ctrl.h"
#include "vi_rgb_ip_ctrl.h"
#include "vi_yuv_ip_ctrl.h"
#include "vi_subsys_ctrl.h"
#include "vi_sys.h"

#define SLICE_MAX_GRID_SIZE	5

/****************************************************************************
 * Function declaration
 ****************************************************************************/

/****************************************************************************
 * Global parameters
 ****************************************************************************/
extern struct vi_fbc_cfg fbc_cfg;

/****************************************************************************
 * SLICE_BUFFER_CONFIG
 ****************************************************************************/
struct slice_buf_s slc_b_cfg = {
	.line_delay		= 512,
	.buffer			= 16,
	.main_max_grid_size	= 32,
	.sub_max_grid_size	= 8,
	.min_r_thshd		= 1,
};

struct vi_overflow_reg_info {
	u8 enable;
	struct {
		u32 blk_idle;
		struct {
			u32 r_0;
			u32 r_4;
			u32 r_8;
			u32 r_c;
		} dbus_sel[7];
	} isp_top;
	struct {
		u32 preraw_info;
		u32 fe_idle_info;
	} preraw_fe;
	struct {
		u32 preraw_be_info;
		u32 be_dma_idle_info;
		u32 ip_idle_info;
		u32 stvalid_status;
		u32 stready_status;
	} preraw_be;
	struct {
		u32 stvalid_status;
		u32 stready_status;
		u32 dma_idle;
	} rawtop;
	struct {
		u32 ip_stvalid_status;
		u32 ip_stready_status;
		u32 dmi_stvalid_status;
		u32 dmi_stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} rgbtop;
	struct {
		u32 debug_state;
		u32 stvalid_status;
		u32 stready_status;
		u32 xcnt_rpt;
		u32 ycnt_rpt;
	} yuvtop;
	struct {
		u32 dbg_sel;
		u32 status;
	} rdma28[2];
};

/**********************************************************
 *	SW scenario path check APIs
 **********************************************************/
u32 _is_fe_post_offline(struct isp_ctx *ctx)
{
	return ctx->is_offline_postraw && !ctx->is_slice_buf_on; //fe->dram->post
}

u32 _is_fe_post_slice(struct isp_ctx *ctx)
{
	return ctx->is_offline_postraw && ctx->is_slice_buf_on; //fe->slice->post
}

u32 _is_all_online(struct isp_ctx *ctx)
{
	return !ctx->is_offline_postraw && !ctx->is_slice_buf_on; //fe->post
}

/****************************************************************************
 *  inner Interfaces
 ****************************************************************************/
void vi_fbc_calculate_size(struct isp_ctx *ctx, u8 pipe)
{
	u32 img_w = ctx->isp_pipe_cfg[pipe].crop.w;
	u32 img_h = ctx->isp_pipe_cfg[pipe].crop.h;

	u32 max_cu_bit = fbc_cfg.is_lossless ? 65 : 67; // = CU_SIZE * 8 + cu_md_bit
	u32 line_cu_num = (img_w + fbc_cfg.cu_size - 1) / fbc_cfg.cu_size;
	u32 total_line_bit_budget = fbc_cfg.is_lossless ?
				(max_cu_bit * line_cu_num) : ((img_w * 8 * fbc_cfg.target_cr) / 100);
	u32 total_first_line_bit_budget = fbc_cfg.is_lossless ? total_line_bit_budget : (img_w * 8);

	u32 y_bs_size = VI_ALIGN(((total_line_bit_budget * (img_h - 1) + total_first_line_bit_budget) / 512) * 64);
	u32 uv_bs_size = VI_ALIGN((((total_line_bit_budget * (img_h / 2) - 1)
				+ total_first_line_bit_budget) / 512) * 64);
	u32 y_buf_size = VI_4K_ALIGN(y_bs_size);
	u32 uv_buf_size = VI_4K_ALIGN(uv_bs_size);

	fbc_cfg.y_bs_size = y_bs_size;
	fbc_cfg.c_bs_size = uv_bs_size;
	fbc_cfg.y_buf_size = y_buf_size;
	fbc_cfg.c_buf_size = uv_buf_size;
}

/****************************************************************************
 *  extend Interfaces
 ****************************************************************************/
int vi_get_raw_num_by_dev(struct isp_ctx *ctx, u8 dev)
{
	return ctx->bind_raw[dev];
}

void vi_set_base_addr(void *base)
{
	uintptr_t *addr = isp_get_phys_reg_bases();
	int i = 0;

	for (i = 0; i < ISP_BLK_ID_MAX; ++i) {
		addr[i] += (uintptr_t)base;
	}
}

uintptr_t *isp_get_phys_reg_bases(void)
{
	static uintptr_t m_isp_phys_base_list[ISP_BLK_ID_MAX] = {
		[ISP_BLK_ID_PRE_RAW_FE0]		= (ISP_BLK_BA_PRE_RAW_FE0),
		[ISP_BLK_ID_CSIBDG0]			= (ISP_BLK_BA_CSIBDG0),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG1),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG2]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG2),
		[ISP_BLK_ID_DMA_CTL_CSI0_BDG3]		= (ISP_BLK_BA_DMA_CTL_CSI0_BDG3),
		[ISP_BLK_ID_PRE_RAW_FE0_LSC0]		= (ISP_BLK_BA_PRE_RAW_FE0_LSC0),
		[ISP_BLK_ID_PRE_RAW_FE0_LSC1]		= (ISP_BLK_BA_PRE_RAW_FE0_LSC1),
		[ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE]	= (ISP_BLK_BA_DMA_CTL_FE0_CLSC_LE),
		[ISP_BLK_ID_AE_HIST_FE0]		= (ISP_BLK_BA_AE_HIST_FE0),
		[ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE]	= (ISP_BLK_BA_DMA_CTL_FE0_AE_HIST_LE),
		[ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE]	= (ISP_BLK_BA_DMA_CTL_FE0_AE_HIST_SE),

		[ISP_BLK_ID_PRE_RAW_FE1]		= (ISP_BLK_BA_PRE_RAW_FE1),
		[ISP_BLK_ID_CSIBDG1]			= (ISP_BLK_BA_CSIBDG1),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG0),
		[ISP_BLK_ID_DMA_CTL_CSI1_BDG1]		= (ISP_BLK_BA_DMA_CTL_CSI1_BDG1),
		[ISP_BLK_ID_PRE_RAW_FE1_LSC0]		= (ISP_BLK_BA_PRE_RAW_FE1_LSC0),
		[ISP_BLK_ID_PRE_RAW_FE1_LSC1]		= (ISP_BLK_BA_PRE_RAW_FE1_LSC1),
		[ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE]	= (ISP_BLK_BA_DMA_CTL_FE1_CLSC_LE),
		[ISP_BLK_ID_AE_HIST_FE1]		= (ISP_BLK_BA_AE_HIST_FE1),
		[ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE]	= (ISP_BLK_BA_DMA_CTL_FE1_AE_HIST_LE),
		[ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE]	= (ISP_BLK_BA_DMA_CTL_FE1_AE_HIST_SE),

		[ISP_BLK_ID_PRE_RAW_FE2]		= (ISP_BLK_BA_PRE_RAW_FE2),
		[ISP_BLK_ID_CSIBDG2]			= (ISP_BLK_BA_CSIBDG2),
		[ISP_BLK_ID_DMA_CTL_CSI2_BDG0]		= (ISP_BLK_BA_DMA_CTL_CSI2_BDG0),
		[ISP_BLK_ID_PRE_RAW_FE2_LSC0]		= (ISP_BLK_BA_PRE_RAW_FE2_LSC0),
		[ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE]	= (ISP_BLK_BA_DMA_CTL_FE2_CLSC_LE),
		[ISP_BLK_ID_AE_HIST_FE2]		= (ISP_BLK_BA_AE_HIST_FE2),
		[ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE]	= (ISP_BLK_BA_DMA_CTL_FE2_AE_HIST_LE),

		[ISP_BLK_ID_WDMA_CORE1]			= (ISP_BLK_BA_WDMA_CORE1),
		[ISP_BLK_ID_WDMA_CORE2]			= (ISP_BLK_BA_WDMA_CORE2),
		[ISP_BLK_ID_WDMA_CORE3]			= (ISP_BLK_BA_WDMA_CORE3),
		[ISP_BLK_ID_WDMA_CORE4]			= (ISP_BLK_BA_WDMA_CORE4),

		[ISP_BLK_ID_RAWTOP0]			= (ISP_BLK_BA_RAWTOP0),
		[ISP_BLK_ID_RAWTOP1]			= (ISP_BLK_BA_RAWTOP1),
		[ISP_BLK_ID_BLC_DG_WB0]			= (ISP_BLK_BA_BLC_DG_WB0),
		[ISP_BLK_ID_BLC_DG_WB1]			= (ISP_BLK_BA_BLC_DG_WB1),

		[ISP_BLK_ID_FUSION]			= (ISP_BLK_BA_FUSION),
		[ISP_BLK_ID_MAPCURVE]			= (ISP_BLK_BA_MAPCURVE),
		[ISP_BLK_ID_DPC]			= (ISP_BLK_BA_DPC),
		[ISP_BLK_ID_AF]				= (ISP_BLK_BA_AF),
		[ISP_BLK_ID_DMA_CTL_AF_W]		= (ISP_BLK_BA_DMA_CTL_AF_W),
		[ISP_BLK_ID_BNR]			= (ISP_BLK_BA_BNR),
		[ISP_BLK_ID_LSCR]			= (ISP_BLK_BA_LSCR),
		[ISP_BLK_ID_DMA_CTL_LSCR_HIST]		= (ISP_BLK_BA_DMA_CTL_LSCR_HIST),
		[ISP_BLK_ID_DRC]			= (ISP_BLK_BA_DRC),
		[ISP_BLK_ID_DMA_CTL_DRC_POLY_R]		= (ISP_BLK_BA_DMA_CTL_DRC_POLY_R),
		[ISP_BLK_ID_DMA_CTL_DRC_POLY_W]		= (ISP_BLK_BA_DMA_CTL_DRC_POLY_W),
		[ISP_BLK_ID_DMA_CTL_DRC_HIST]		= (ISP_BLK_BA_DMA_CTL_DRC_HIST),
		[ISP_BLK_ID_CFA]			= (ISP_BLK_BA_CFA),

		[ISP_BLK_ID_RGBTOP]			= (ISP_BLK_BA_RGBTOP),
		[ISP_BLK_ID_PRE_EE_EXT]			= (ISP_BLK_BA_PRE_EE_EXT),
		[ISP_BLK_ID_PFR]			= (ISP_BLK_BA_PFR),
		[ISP_BLK_ID_CCM]			= (ISP_BLK_BA_CCM),
		[ISP_BLK_ID_RGBGAMMA]			= (ISP_BLK_BA_RGBGAMMA),
		[ISP_BLK_ID_CLUT]			= (ISP_BLK_BA_CLUT),
		[ISP_BLK_ID_DMA_CTL_CLUT_R]		= (ISP_BLK_BA_DMA_CTL_CLUT_R),
		[ISP_BLK_ID_CSC]			= (ISP_BLK_BA_CSC),
		[ISP_BLK_ID_RGB_DITHER]			= (ISP_BLK_BA_RGB_DITHER),

		[ISP_BLK_ID_YUVTOP]			= (ISP_BLK_BA_YUVTOP),
		[ISP_BLK_ID_YUV_DITHER]			= (ISP_BLK_BA_YUV_DITHER),
		[ISP_BLK_ID_PRE_EE_FRONT]		= (ISP_BLK_BA_PRE_EE_FRONT),
		[ISP_BLK_ID_PRE_EE_BACK]		= (ISP_BLK_BA_PRE_EE_BACK),
		[ISP_BLK_ID_LDCI]			= (ISP_BLK_BA_LDCI),
		[ISP_BLK_ID_DMA_CTL_LDCI_W]		= (ISP_BLK_BA_DMA_CTL_LDCI_W),
		[ISP_BLK_ID_DMA_CTL_LDCI_R]		= (ISP_BLK_BA_DMA_CTL_LDCI_R),
		[ISP_BLK_ID_DMA_CTL_LDCI_HIST]		= (ISP_BLK_BA_DMA_CTL_LDCI_HIST),
		[ISP_BLK_ID_LDCI_MAP_CORE]		= (ISP_BLK_BA_LDCI_MAP_CORE),
		[ISP_BLK_ID_TNR]			= (ISP_BLK_BA_TNR),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_Y]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_Y),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_C]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_C),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_MV]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_MV),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_MO]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_MO),
		[ISP_BLK_ID_DMA_CTL_TNR_LD_FCB]		= (ISP_BLK_BA_DMA_CTL_TNR_LD_FCB),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_Y]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_Y),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_C]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_C),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_MV]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_MV),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_MO]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_MO),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_FCB]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_FCB),
		[ISP_BLK_ID_DMA_CTL_TNR_ST_MSP]		= (ISP_BLK_BA_DMA_CTL_TNR_ST_MSP),
		[ISP_BLK_ID_FBCD]			= (ISP_BLK_BA_FBCD),
		[ISP_BLK_ID_FBCE]			= (ISP_BLK_BA_FBCE),
		[ISP_BLK_ID_CNR]			= (ISP_BLK_BA_CNR),
		[ISP_BLK_ID_DMA_CTL_CNR_Y_W]		= (ISP_BLK_BA_DMA_CTL_CNR_Y_W),
		[ISP_BLK_ID_DMA_CTL_CNR_Y_R]		= (ISP_BLK_BA_DMA_CTL_CNR_Y_R),
		[ISP_BLK_ID_DMA_CTL_CNR_C_W]		= (ISP_BLK_BA_DMA_CTL_CNR_C_W),
		[ISP_BLK_ID_DMA_CTL_CNR_C_R]		= (ISP_BLK_BA_DMA_CTL_CNR_C_R),
		[ISP_BLK_ID_CA]				= (ISP_BLK_BA_CA),
		[ISP_BLK_ID_CA_LITE]			= (ISP_BLK_BA_CA_LITE),
		[ISP_BLK_ID_POST_EE]			= (ISP_BLK_BA_POST_EE),
		[ISP_BLK_ID_YCURVE]			= (ISP_BLK_BA_YCURVE),
		[ISP_BLK_ID_YUV_CROP_Y]			= (ISP_BLK_BA_YUV_CROP_Y),
		[ISP_BLK_ID_DMA_CTL_YUV_CROP_Y]		= (ISP_BLK_BA_DMA_CTL_YUV_CROP_Y),
		[ISP_BLK_ID_YUV_CROP_C]			= (ISP_BLK_BA_YUV_CROP_C),
		[ISP_BLK_ID_DMA_CTL_YUV_CROP_C]		= (ISP_BLK_BA_DMA_CTL_YUV_CROP_C),
		[ISP_BLK_ID_DMA_CTL_YUV_RDMA_Y]		= (ISP_BLK_BA_DMA_CTL_YUV_RDMA_Y),
		[ISP_BLK_ID_DMA_CTL_YUV_RDMA_C]		= (ISP_BLK_BA_DMA_CTL_YUV_RDMA_C),
		[ISP_BLK_ID_RESIZE]			= (ISP_BLK_BA_RESIZE),
		[ISP_BLK_ID_DMA_CTL_RESIZE]		= (ISP_BLK_BA_DMA_CTL_RESIZE),

		[ISP_BLK_ID_ISPTOP]			= (ISP_BLK_BA_ISPTOP),
		[ISP_BLK_ID_RDMA_CORE1]			= (ISP_BLK_BA_RDMA_CORE1),
		[ISP_BLK_ID_RDMA_CORE2]			= (ISP_BLK_BA_RDMA_CORE2),
		[ISP_BLK_ID_RDMA_CORE3]			= (ISP_BLK_BA_RDMA_CORE3),
		[ISP_BLK_ID_CSIBDG0_LITE]		= (ISP_BLK_BA_CSIBDG0_LITE),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE0]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE0),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE1]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE1),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE2]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE2),
		[ISP_BLK_ID_DMA_CTL_BT0_LITE3]		= (ISP_BLK_BA_DMA_CTL_BT0_LITE3),
		[ISP_BLK_ID_PRE_RAW_VI_SEL]		= (ISP_BLK_BA_PRE_RAW_VI_SEL),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_LE),
		[ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE]	= (ISP_BLK_BA_DMA_CTL_PRE_RAW_VI_SEL_SE),
		[ISP_BLK_ID_PRE_RAW_VI_SEL_CROP_LE]	= (ISP_BLK_BA_PRE_RAW_VI_SEL_CROP_LE),
		[ISP_BLK_ID_PRE_RAW_VI_SEL_CROP_SE]	= (ISP_BLK_BA_PRE_RAW_VI_SEL_CROP_SE),
		[ISP_BLK_ID_CMDQ]			= (ISP_BLK_BA_CMDQ),
	};
	return m_isp_phys_base_list;
}

void isp_intr_status(
	struct isp_ctx *ctx,
	union reg_isp_top_int_event0 *s0,
	union reg_isp_top_int_event1 *s1,
	union reg_isp_top_int_event2 *s2)
{
	uintptr_t isp_top = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	s0->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event0);
	//clear isp top event0 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event0, s0->raw);

	s1->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event1);
	//clear isp top event1 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event1, s1->raw);

	s2->raw = ISP_RD_REG(isp_top, reg_isp_top_t, int_event2);
	//clear isp top event2 status
	ISP_WR_REG(isp_top, reg_isp_top_t, int_event2, s2->raw);
}

void isp_csi_intr_status(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	union reg_isp_csi_bdg_interrupt_status_0 *s0,
	union reg_isp_csi_bdg_interrupt_status_1 *s1)
{
	int id = csibdg_find_hwid(raw_num);
	uintptr_t ba = ctx->phys_regs[id];

	if (ctx->isp_csi_cfg[raw_num].is_bt_demux) {

		s0->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_0);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_0, s0->raw);

		s1->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_1);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_lite_t, interrupt_status_1, s1->raw);
	} else {
		s0->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_t, interrupt_status_0);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_t, interrupt_status_0, s0->raw);

		s1->raw = ISP_RD_REG(ba, reg_isp_csi_bdg_t, interrupt_status_1);
		//clear status
		ISP_WR_REG(ba, reg_isp_csi_bdg_t, interrupt_status_1, s1->raw);
	}
}

void isp_streaming(struct isp_ctx *ctx, u32 on, enum sop_isp_raw raw_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	int id = csibdg_find_hwid(phy_raw);
	uintptr_t csibdg = ctx->phys_regs[id];
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1 sw_ctrl_1;
	union reg_isp_csi_bdg_top_ctrl csibdg_topctl;
	union reg_isp_csi_bdg_lite_bdg_top_ctrl csibdg_lite_topctl;

	if (raw_num >= ISP_PRERAW_LITE0) {
		csibdg_lite_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl);
		csibdg_lite_topctl.bits.csi_up_reg = on;
		csibdg_lite_topctl.bits.mcsi_enable = on;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, csibdg_lite_topctl.raw);
		return;
	}

	if (on) {
		sw_ctrl_0.raw = ISP_RD_REG(isptopb, reg_isp_top_t, sw_ctrl_0);
		sw_ctrl_1.raw = ISP_RD_REG(isptopb, reg_isp_top_t, sw_ctrl_1);

		if (raw_num == ISP_PRERAW0) {
			if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe0	= 3;
				sw_ctrl_1.bits.pq_up_fe0	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe0	= 1;
				sw_ctrl_1.bits.pq_up_fe0	= 1;
			}
		} else if (raw_num == ISP_PRERAW1) {
			if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe1	= 3;
				sw_ctrl_1.bits.pq_up_fe1	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe1	= 1;
				sw_ctrl_1.bits.pq_up_fe1	= 1;
			}
		} else if (raw_num == ISP_PRERAW2) {
			if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_fe2	= 3;
				sw_ctrl_1.bits.pq_up_fe2	= 3;
			} else {
				sw_ctrl_0.bits.shaw_up_fe2	= 1;
				sw_ctrl_1.bits.pq_up_fe2	= 1;
			}
		}

		if (!_is_fe_post_offline(ctx)) {
			sw_ctrl_0.bits.shaw_up_be	= 1;
			sw_ctrl_1.bits.pq_up_be		= 1;
			sw_ctrl_0.bits.shaw_up_raw	= 1;
			sw_ctrl_1.bits.pq_up_raw	= 1;
			sw_ctrl_0.bits.shaw_up_post	= 1;
			sw_ctrl_1.bits.pq_up_post	= 1;
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);

		csibdg_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		if (ctx->is_rawreplay) {
			csibdg_topctl.bits.csi_up_reg = 0;
			csibdg_topctl.bits.csi_enable = 0;
			csibdg_topctl.bits.tgen_enable = 0;
		} else {
			csibdg_topctl.bits.csi_up_reg = 1;
			csibdg_topctl.bits.csi_enable = 1;

			if (ctx->isp_csi_cfg[raw_num].is_patgen_en)
				csibdg_topctl.bits.tgen_enable = 1;
			else
				csibdg_topctl.bits.tgen_enable = 0;
		}

		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_topctl.raw);
	} else {
		csibdg_topctl.raw = ISP_RD_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl);
		csibdg_topctl.bits.csi_enable = 0;
		csibdg_topctl.bits.tgen_enable = 0;
		ISP_WR_REG(csibdg, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, csibdg_topctl.raw);
	}
}

uint8_t vi_get_pipe_by_raw_chn(struct isp_ctx *ctx, u8 raw_num, enum sop_isp_fe_chn_num chn)
{
	return ctx->isp_csi_cfg[raw_num].bind_pipe[chn];
}

void isp_pre_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);

	if (ctx->is_rawreplay) { //dram->be
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
		union reg_isp_top_sw_ctrl_1 sw_ctrl_1;

		sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			sw_ctrl_0.bits.trig_str_be	= 0x3;
			sw_ctrl_1.bits.pq_up_be		= 0x3;
			sw_ctrl_0.bits.shaw_up_be	= 0x3;
			sw_ctrl_0.bits.trig_str_raw	= 0x1;
			sw_ctrl_0.bits.shaw_up_raw	= 0x1;
			sw_ctrl_1.bits.pq_up_raw	= 0x1;
			sw_ctrl_0.bits.trig_str_post	= 0x1;
			sw_ctrl_0.bits.shaw_up_post	= 0x1;
			sw_ctrl_1.bits.pq_up_post	= 0x1;
		} else {
			sw_ctrl_0.bits.trig_str_be	= 0x1;
			sw_ctrl_1.bits.pq_up_be		= 0x1;
			sw_ctrl_0.bits.shaw_up_be	= 0x1;
			sw_ctrl_0.bits.trig_str_raw	= 0x1;
			sw_ctrl_0.bits.shaw_up_raw	= 0x1;
			sw_ctrl_1.bits.pq_up_raw	= 0x1;
			sw_ctrl_0.bits.trig_str_post	= 0x1;
			sw_ctrl_0.bits.shaw_up_post	= 0x1;
			sw_ctrl_1.bits.pq_up_post	= 0x1;
		}
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);

		vi_pr(VI_DBG, "Raw replay trigger fe_%d\n", raw_num);
	} else if (ctx->isp_csi_cfg[raw_num].is_bt_demux) {
		int id = csibdg_find_hwid(phy_raw);
		uintptr_t csibdg_lite = ctx->phys_regs[id];

		vi_pr(VI_DBG, "trigger csibdg_lite_%d chn_num_%d frame_vld\n", raw_num, chn_num);

		switch (chn_num) {
		case ISP_FE_CH0:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch0, 1);
			break;
		case ISP_FE_CH1:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch1, 1);
			break;
		case ISP_FE_CH2:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch2, 1);
			break;
		case ISP_FE_CH3:
			ISP_WR_BITS(csibdg_lite, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch3, 1);
			break;
		default:
			break;
		}
	} else { // patgen or sensor->fe
		int id = fe_find_hwid(phy_raw);
		uintptr_t fe = ctx->phys_regs[id];

		vi_pr(VI_DBG, "trigger fe_%d chn_num_%d frame_vld\n", raw_num, chn_num);

		switch (chn_num) {
		case ISP_FE_CH0:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch0, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch0, 1);
			break;
		case ISP_FE_CH1:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch1, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch1, 1);
			break;
		case ISP_FE_CH2:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch2, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch2, 1);
			break;
		case ISP_FE_CH3:
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_pq_vld_ch3, 1);
			ISP_WR_BITS(fe, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch3, 1);
			break;
		default:
			break;
		}
	}
}

#ifdef ISP_CMDQ_SUPPORT
static void _isp_cmdq_post_trig(struct isp_ctx *ctx, const u8 pipe,
				u32 sw_ctrl_1, u32 sw_ctrl_0)
{
	uintptr_t cmqd = ctx->phys_regs[ISP_BLK_ID_CMDQ];
	u32 isptop_phy_reg = ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_ISPTOP;
	u16 cmd_idx = ctx->isp_pipe_cfg[pipe].cmdq_buf.cmd_idx;
	union cmdq_set *cmd_start = (union cmdq_set *)ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr;

	base_ion_cache_invalidate(ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr,
				  ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr,
				  ctx->isp_pipe_cfg[pipe].cmdq_buf.buf_size);

	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 isptop_phy_reg + _OFST(reg_isp_top_t, sw_ctrl_1), sw_ctrl_1);
	cmdq_set_package(&cmd_start[cmd_idx++].reg,
			 isptop_phy_reg + _OFST(reg_isp_top_t, sw_ctrl_0), sw_ctrl_0);

	cmd_start[cmd_idx - 1].reg.intr_end = 1;
	cmd_start[cmd_idx - 1].reg.intr_last = 1;

	base_ion_cache_flush(ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr,
			     ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr,
			     ctx->isp_pipe_cfg[pipe].cmdq_buf.buf_size);

	cmdq_intr_ctrl(cmqd, 0x1F);
	cmdq_engine(cmqd, (uintptr_t)ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr,
		    (ISP_TOP_PHY_REG_BASE + ISP_BLK_BA_CMDQ) >> 22, true, false, cmd_idx);

	//reset cmd_idx
	ctx->isp_pipe_cfg[pipe].cmdq_buf.cmd_idx = 0;
}
#endif

void isp_post_trig(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_sw_ctrl_0 sw_ctrl_0;
	union reg_isp_top_sw_ctrl_1 sw_ctrl_1;
	int pipe = ctx->cfg_info.pipe;

	sw_ctrl_0.raw = sw_ctrl_1.raw = 0;

	if (_is_fe_post_offline(ctx)) { //fe->dram->be->post
		vi_pr(VI_DBG, "dram->be post trig pipe(%d), is_hdr_on(%d)\n",
				pipe, ctx->cfg_info.is_hdr_on);

		if (!ctx->cfg_info.is_yuv) {
			if (ctx->cfg_info.is_hdr_on) {
				sw_ctrl_0.bits.shaw_up_be	= 3;
				sw_ctrl_0.bits.trig_str_be	= 3;
				sw_ctrl_1.bits.pq_up_be		= 3;
				sw_ctrl_0.bits.shaw_up_raw	= 1;
				sw_ctrl_0.bits.trig_str_raw	= 1;
				sw_ctrl_1.bits.pq_up_raw	= 1;
				sw_ctrl_0.bits.shaw_up_post	= 1;
				sw_ctrl_0.bits.trig_str_post	= 1;
				sw_ctrl_1.bits.pq_up_post	= 1;
			} else {
				sw_ctrl_0.bits.shaw_up_be	= 1;
				sw_ctrl_0.bits.trig_str_be	= 1;
				sw_ctrl_1.bits.pq_up_be		= 1;
				sw_ctrl_0.bits.shaw_up_raw	= 1;
				sw_ctrl_0.bits.trig_str_raw	= 1;
				sw_ctrl_1.bits.pq_up_raw	= 1;
				sw_ctrl_0.bits.shaw_up_post	= 1;
				sw_ctrl_0.bits.trig_str_post	= 1;
				sw_ctrl_1.bits.pq_up_post	= 1;
			}
		} else {
			sw_ctrl_0.bits.shaw_up_be	= 1;
			sw_ctrl_0.bits.trig_str_be	= 1;
			sw_ctrl_1.bits.pq_up_be		= 1;
			sw_ctrl_0.bits.shaw_up_raw	= 1;
			sw_ctrl_0.bits.trig_str_raw	= 1;
			sw_ctrl_1.bits.pq_up_raw	= 1;
			sw_ctrl_0.bits.shaw_up_post	= 1;
			sw_ctrl_0.bits.trig_str_post	= 1;
			sw_ctrl_1.bits.pq_up_post	= 1;
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_1, sw_ctrl_1.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_ctrl_0, sw_ctrl_0.raw);
		//_isp_cmdq_post_trig(ctx, pipe, sw_ctrl_1.raw, sw_ctrl_0.raw);

	} else if (_is_fe_post_slice(ctx)) { //slice buffer path
		vi_pr(VI_DBG, "dram->post trig pipe(%d), is_slice_buf_on(%d)\n",
				pipe, ctx->is_slice_buf_on);

		isp_slice_buf_trig(ctx);
	}
}

/*********************************************************************************
 *	Common IPs for subsys
 ********************************************************************************/
u64 ispblk_dma_getaddr(struct isp_ctx *ctx, u32 dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u64 addr_h = ISP_RD_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh);

	return ((u64)ISP_RD_REG(dmab, reg_isp_dma_ctl_t, base_addr) | (addr_h << 32));
}

int ispblk_dma_buf_get_size(struct isp_ctx *ctx, const uint8_t pipe, int dmaid)
{
	u32 len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	{
		/* csibdg */
		if (_is_fe_post_slice(ctx)) {
			len = ((dmaid == ISP_BLK_ID_DMA_CTL_CSI0_BDG0)
				|| (dmaid == ISP_BLK_ID_DMA_CTL_CSI1_BDG0))
				? slc_b_cfg.main_path.le_buf_size
				: slc_b_cfg.main_path.se_buf_size;
			num = 1;
		} else {
			w = ctx->isp_csi_cfg[pipe].crop[ISP_FE_CH0].w;
			if (ctx->is_dpcm_on)
				w >>= 1;

			num = ctx->isp_csi_cfg[pipe].crop[ISP_FE_CH0].h;
			len = 3 * UPPER(w, 1);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE:
	case ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE:
	case ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE:
	{
		/*gain size 8byte, [16bit][16bit][16bit][16bit]
		 * num 1370	   [0][r][g][b]
		 */
		len = 0x55A * 0x8;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE:
	case ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE:
	case ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE:
	{
		int ae_dma_counts, hist_dma_counts;

		/*block num x * block num y * block size (34 * 30 * 8(B/block))*/
		ae_dma_counts = 0x1FE0;
		/*size * range * num (4B * 256 * 3)*/
		hist_dma_counts = 0xC00;

		len = ae_dma_counts + hist_dma_counts;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_AF_W:
	{
		/*af size = (block_num_x * block_num_y) << 5*/
		num = 1;
		len = ((17 * 15) << 5);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LSCR_HIST:
	{
		/*256 * 4 * 3(rgb 3 channel) */
		len = 1024 * 3;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_DRC_POLY_R:
	case ISP_BLK_ID_DMA_CTL_DRC_POLY_W:
	{
		len = 48 * 48 * 4 * 2;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_DRC_HIST:
	{
		/*128 * 4*/
		len = 512;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_CLUT_R:
	{
		len = 4913 * 4;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LDCI_HIST:
	{
		/*256 * 4*/
		len = 256 * 4;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LDCI_W:
	case ISP_BLK_ID_DMA_CTL_LDCI_R:
	{
		/*sw cannot set, only use malloc*/
		len = 3969;
		num = 1;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_Y:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_Y:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, pipe);
			len = fbc_cfg.y_buf_size;
			num = 1;
		} else {
			w = ctx->isp_pipe_cfg[pipe].crop.w;

			len = (((((w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[pipe].crop.h;
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_C:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_C:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, pipe);
			len = fbc_cfg.c_buf_size;
			num = 1;
		} else {
			w = ctx->isp_pipe_cfg[pipe].crop.w;

			len = (((((w) << 3) + 127) >> 7) << 7) >> 3;
			num = (ctx->isp_pipe_cfg[pipe].crop.h >> 1);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MV:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MV:
	{
		/*motion vector 16bit * (w / 8) * (h / 4) */
		len = ((ctx->isp_pipe_cfg[pipe].crop.w + 7) >> 3) << 1;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 3) >> 2;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MO:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MO:
	{
		/*motion map width 1bit, w (w/16) h (h/16) */
		len = (((ctx->isp_pipe_cfg[pipe].crop.w + 15) >> 4) + 7) >> 3;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 15) >> 4;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_CNR_Y_W:
	case ISP_BLK_ID_DMA_CTL_CNR_Y_R:
	{
		/*cnr w > 2688 cnr scale 8, otherwise 4 */
		if (ctx->isp_pipe_cfg[pipe].crop.w > 2688) {
			len = VI_ALIGN(((ctx->isp_pipe_cfg[pipe].crop.w >> 1) + 7) >> 3);
			num = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.h + 7) >> 3);
		} else {
			len = VI_ALIGN(((ctx->isp_pipe_cfg[pipe].crop.w >> 1) + 3) >> 2);
			num = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.h + 3) >> 2);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_CNR_C_W:
	case ISP_BLK_ID_DMA_CTL_CNR_C_R:
	{
		/*cnr w > 2688 cnr scale 8, otherwise 4 */
		if (ctx->isp_pipe_cfg[pipe].crop.w > 2688) {
			len = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.w + 7) >> 3);
			num = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.h + 3) >> 2);
		} else {
			len = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.w + 3) >> 2);
			num = VI_ALIGN((ctx->isp_pipe_cfg[pipe].crop.h + 1) >> 1);
		}

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_FCB:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_FCB:
	{
		/*fast conbuf 8bit * w/16 * h/16*/
		len = (ctx->isp_pipe_cfg[pipe].crop.w + 15) >> 4;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 15) >> 4;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MSP:
	{
		/*2bit * w/4 * h/4*/
		len = (((ctx->isp_pipe_cfg[pipe].crop.w + 3) >> 2) + 3) >> 2;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 3) >> 2;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_INFO, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

void ispblk_dma_setaddr(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, base_addr, (buf_addr & 0xFFFFFFFF));
	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh, ((buf_addr >> 32) & 0xFFFFFFFF));
}

bool ispblk_dma_get_sw_mode(struct isp_ctx *ctx, u32 dmaid)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	//SW mode: 1 ; HW mode: 0
	sys_ctrl.raw = ISP_RD_REG(dmab, reg_isp_dma_ctl_t, sys_control);

	return sys_ctrl.bits.stride_sel;
}

void ispblk_dma_set_sw_mode(struct isp_ctx *ctx, u32 dmaid, bool is_sw_mode)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	//SW mode: config by SW
	sys_ctrl.raw = ISP_RD_REG(dmab, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= is_sw_mode;
	sys_ctrl.bits.seglen_sel	= is_sw_mode;
	sys_ctrl.bits.segnum_sel	= is_sw_mode;
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

int ispblk_dma_config(struct isp_ctx *ctx, const uint8_t pipe, int dmaid, u64 buf_addr)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 w = 0, len = 0, stride = 0, num = 0, cnr_shift = 1;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	{
		if (_is_fe_post_slice(ctx)) {
			len = ((dmaid == ISP_BLK_ID_DMA_CTL_CSI0_BDG0)
				|| (dmaid == ISP_BLK_ID_DMA_CTL_CSI1_BDG0))
				? slc_b_cfg.main_path.le_buf_size
				: slc_b_cfg.main_path.se_buf_size;
			num = 1;
			stride = len;
		} else {
			/* csibdg */
			w = ctx->isp_csi_cfg[pipe].crop[ISP_FE_CH0].w;
			num = ctx->isp_csi_cfg[pipe].crop[ISP_FE_CH0].h;
			if (ctx->is_dpcm_on)
				w >>= 1;

			len = 3 * UPPER(w, 1);
			stride = len;
		}
		break;
	}
	case ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE:
	case ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE:
	case ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE:
	{
		/*gain size 8byte, [16bit][16bit][16bit][16bit]
		 * num 1370	   [0][r][g][b]
		 */
		len = 0x55A * 0x8;
		num = 1;
		stride = len;
		break;
	}
	case ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE:
	case ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE:
	case ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE:
	case ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE:
	{
		int ae_dma_counts, hist_dma_counts;

		/*block num x * block num y * block size (34 * 30 * 8(B/block))*/
		ae_dma_counts = 0x1FE0;
		/*size * range * num (4B * 256 * 3)*/
		hist_dma_counts = 0xC00;

		len = ae_dma_counts + hist_dma_counts;
		num = 1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE:
	{
		// preraw be read/write dma
		u32 dpcm_on = (ctx->is_dpcm_on) ? 2 : 1;

		w = ctx->isp_pipe_cfg[pipe].crop.w;
		num = ctx->isp_pipe_cfg[pipe].crop.h;
		len = 3 * UPPER(w, dpcm_on);
		stride = len;
		break;
	}
	case ISP_BLK_ID_DMA_CTL_AF_W:
	{
		/*af size = (block_num_x * block_num_y) << 5*/
		num = 1;
		len = ((17 * 15) << 5);
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LSCR_HIST:
	{
		/*256 * 4 * 3(rgb 3 channel) */
		len = 1024 * 3;
		num = 1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_DRC_POLY_R:
	case ISP_BLK_ID_DMA_CTL_DRC_POLY_W:
	{
		len = 48 * 48 * 4 * 2;
		num = 1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_DRC_HIST:
	{
		/*128 * 4*/
		len = 512;
		num = 1;
		stride = len;

		break;
	}

	case ISP_BLK_ID_DMA_CTL_CLUT_R:
	{
		len = 4913 * 4;
		num = 1;
		stride = len;
		break;
	}

	case ISP_BLK_ID_DMA_CTL_LDCI_HIST:
	{
		/*256 * 4*/
		len = 256 * 4;
		num = 1;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_LDCI_W:
	case ISP_BLK_ID_DMA_CTL_LDCI_R:
	{
		/*sw cannot set, only use malloc*/
		len = 3969;
		num = 1;
		stride = 3969;
		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_C:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_C:
	{
		//TNR UV
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, pipe);
			len = fbc_cfg.c_bs_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[pipe].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = (ctx->isp_pipe_cfg[pipe].crop.h >> 1);
		}
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_Y:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_Y:
	{
		//TNR Y
		if (ctx->is_fbc_on) {
			vi_fbc_calculate_size(ctx, pipe);
			len = fbc_cfg.y_bs_size;
			num = 1;
		} else {
			len = (((((ctx->isp_pipe_cfg[pipe].crop.w) << 3) + 127) >> 7) << 7) >> 3;
			num = ctx->isp_pipe_cfg[pipe].crop.h;
		}
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MV:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MV:
	{
		/*motion vector 16bit * (w / 8) * (h / 4) */
		len = ((ctx->isp_pipe_cfg[pipe].crop.w + 7) >> 3) << 1;
		num = ctx->isp_pipe_cfg[pipe].crop.h >> 2;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MO:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_MO:
	{
		/*motion map width 1bit, w (w/16) h (h/16) */
		len = (((ctx->isp_pipe_cfg[pipe].crop.w + 15) >> 4) + 7) >> 3;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 15) >> 4;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_CNR_Y_W:
	case ISP_BLK_ID_DMA_CTL_CNR_Y_R:
	{
		cnr_shift = (dmaid == ISP_BLK_ID_DMA_CTL_CNR_Y_W) ?
			ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift :
			ctx->isp_pipe_cfg[pipe].cnr_pre_scale_shift;

		len = ((ctx->isp_pipe_cfg[pipe].crop.w >> 1) + (1 << cnr_shift) - 1) >> cnr_shift;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + (1 << cnr_shift) - 1) >> cnr_shift;

		len *= num;
		num = 1;
		stride = VI_ALIGN(len);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_CNR_C_W:
	case ISP_BLK_ID_DMA_CTL_CNR_C_R:
	{
		cnr_shift = (dmaid == ISP_BLK_ID_DMA_CTL_CNR_C_W) ?
			ctx->isp_pipe_cfg[pipe].cnr_cur_scale_shift :
			ctx->isp_pipe_cfg[pipe].cnr_pre_scale_shift;

		len = ((ctx->isp_pipe_cfg[pipe].crop.w >> 1) + (1 << cnr_shift) - 1) >> cnr_shift;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + (1 << cnr_shift) - 1) >> cnr_shift;

		len *= num * 2;
		num = 1;
		stride = VI_ALIGN(len);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_Y:
	{
		//yuvtop y out
		len = (ctx->isp_pipe_cfg[pipe].postout_crop.w) ?
			ctx->isp_pipe_cfg[pipe].postout_crop.w : ctx->cfg_info.img_width;
		stride = len;

		num = (ctx->isp_pipe_cfg[pipe].postout_crop.h) ?
				ctx->isp_pipe_cfg[pipe].postout_crop.h : ctx->cfg_info.img_height;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_C:
	{
		//yuvtop uv out
		len = (ctx->isp_pipe_cfg[pipe].postout_crop.w) ?
			ctx->isp_pipe_cfg[pipe].postout_crop.w : ctx->cfg_info.img_width;
		stride = len;

		num = (ctx->isp_pipe_cfg[pipe].postout_crop.h) ?
				(ctx->isp_pipe_cfg[pipe].postout_crop.h >> 1) : (ctx->cfg_info.img_height >> 1);

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_FCB:
	case ISP_BLK_ID_DMA_CTL_TNR_LD_FCB:
	{
		/*fast conbuf 8bit * w/16 * h/16*/
		len = (ctx->isp_pipe_cfg[pipe].crop.w + 15) >> 4;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 15) >> 4;
		stride = len;

		break;
	}
	case ISP_BLK_ID_DMA_CTL_TNR_ST_MSP:
	{
		/*2bit * w/4 * h/4*/
		len = (((ctx->isp_pipe_cfg[pipe].crop.w + 3) >> 2) + 3) >> 2;
		num = (ctx->isp_pipe_cfg[pipe].crop.h + 3) >> 2;
		stride = len;

		break;
	}
	default:
		break;
	}

	stride = VI_ALIGN(stride);

	if (dmaid == ISP_BLK_ID_DMA_CTL_LDCI_W || dmaid == ISP_BLK_ID_DMA_CTL_LDCI_R) {
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, 0);
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, 0);
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, 0);
	} else {
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, len);
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, stride);
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, num);
	}

	if (buf_addr) {
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, base_addr, (buf_addr & 0xFFFFFFFF));
		ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh, ((buf_addr >> 32) & 0xFFFFFFFF));
	}

	vi_pr(VI_DBG, "pipe_%d, dmaid=%d len=%d stride=%d num=%d size=%d addr=%llx\n",
			pipe, dmaid, len, stride, num, stride * num, buf_addr);

	return stride * num;
}

void ispblk_dma_enable(struct isp_ctx *ctx, u32 dmaid, u32 on, u8 dma_disable)
{
	uintptr_t srcb = 0;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_Y:
		/* yuvtop y crop4 */
		srcb = ctx->phys_regs[ISP_BLK_ID_YUV_CROP_Y];
		break;
	case ISP_BLK_ID_DMA_CTL_YUV_CROP_C:
		/* yuvtop uv crop5 */
		srcb = ctx->phys_regs[ISP_BLK_ID_YUV_CROP_C];
		break;
	default:
		break;
	}

	if (srcb) {
		ISP_WR_BITS(srcb, reg_crop_t, crop_0, dma_enable, on);
		ISP_WR_BITS(srcb, reg_crop_t, crop_debug, force_dma_disable, dma_disable);
	}
}

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];

	ISP_WR_BITS(cropb, reg_crop_t, crop_0, crop_enable, en);
}

int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vi_rect crop)
{
	uintptr_t cropb = ctx->phys_regs[crop_id];
	union reg_crop_1 reg1;
	union reg_crop_2 reg2;

	// crop out size
	reg1.bits.crop_start_y = crop.y;
	reg1.bits.crop_end_y = crop.y + crop.h - 1;
	reg2.bits.crop_start_x = crop.x;
	reg2.bits.crop_end_x = crop.x + crop.w - 1;
	ISP_WR_REG(cropb, reg_crop_t, crop_1, reg1.raw);
	ISP_WR_REG(cropb, reg_crop_t, crop_2, reg2.raw);
	ISP_WR_BITS(cropb, reg_crop_t, crop_0, crop_enable, true);

	return 0;
}

int bayer_type_mapping(enum isp_bayer_type_e bayer_type)
{
	int mapping = bayer_type;

	if (bayer_type == ISP_BAYER_TYPE_IGRGB || bayer_type == ISP_BAYER_TYPE_IGBGR)
		mapping = ISP_BAYER_TYPE_BG;
	else if (bayer_type == ISP_BAYER_TYPE_IRGBG || bayer_type == ISP_BAYER_TYPE_IBGRG)
		mapping = ISP_BAYER_TYPE_GB;
	else if (bayer_type == ISP_BAYER_TYPE_GRGBI || bayer_type == ISP_BAYER_TYPE_GBGRI)
		mapping = ISP_BAYER_TYPE_GR;
	else if (bayer_type == ISP_BAYER_TYPE_RGBGI || bayer_type == ISP_BAYER_TYPE_BGRGI)
		mapping = ISP_BAYER_TYPE_RG;

	return mapping;
}

/**
 * Calculate the Bayer type before cropping based on the current Bayer type
 * and crop parameters (x, y).
 *
 * @param current_bayer_type The current Bayer type after cropping.
 * @param crop_x The x-offset of the crop.
 * @param crop_y The y-offset of the crop.
 * @return The Bayer type before cropping.
 */
enum isp_bayer_type_e bayer_type_before_crop(enum isp_bayer_type_e current_bayer_type, u16 crop_x, u16 crop_y)
{
	// Adjust the Bayer type based on the crop offsets
	u16 x_offset = crop_x % 2;
	u16 y_offset = crop_y % 2;

	switch (current_bayer_type) {
	case ISP_BAYER_TYPE_BG:
		return (y_offset == 0) ? ((x_offset == 0) ? ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_GB)
					: ((x_offset == 0) ? ISP_BAYER_TYPE_GR : ISP_BAYER_TYPE_RG);
	case ISP_BAYER_TYPE_GB:
		return (y_offset == 0) ? ((x_offset == 0) ? ISP_BAYER_TYPE_GB : ISP_BAYER_TYPE_BG)
					: ((x_offset == 0) ? ISP_BAYER_TYPE_RG : ISP_BAYER_TYPE_GR);
	case ISP_BAYER_TYPE_GR:
		return (y_offset == 0) ? ((x_offset == 0) ? ISP_BAYER_TYPE_GR : ISP_BAYER_TYPE_RG)
					: ((x_offset == 0) ? ISP_BAYER_TYPE_BG : ISP_BAYER_TYPE_GB);
	case ISP_BAYER_TYPE_RG:
		return (y_offset == 0) ? ((x_offset == 0) ? ISP_BAYER_TYPE_RG : ISP_BAYER_TYPE_GR)
					: ((x_offset == 0) ? ISP_BAYER_TYPE_GB : ISP_BAYER_TYPE_BG);
	default:
		// Handle RGBIR types or unsupported types
		return current_bayer_type;
	}
}

int csibdg_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num)
{
	int dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG0;

	switch (raw_num) {
	case ISP_PRERAW0:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG1;
		else if (chn_num == ISP_FE_CH2)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG2;
		else if (chn_num == ISP_FE_CH3)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI0_BDG3;
		break;
	case ISP_PRERAW1:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG0;
		else if (chn_num == ISP_FE_CH1)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI1_BDG1;
		break;
	case ISP_PRERAW2:
		if (chn_num == ISP_FE_CH0)
			dma_id = ISP_BLK_ID_DMA_CTL_CSI2_BDG0;
		break;
	case ISP_PRERAW_LITE0:
		dma_id = ISP_BLK_ID_DMA_CTL_BT0_LITE0 + chn_num;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_num[%d] chn[%d]\n", raw_num, chn_num);
		break;
	}

	return dma_id;
}

int ae_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num)
{
	int dma_id = ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE;

	switch (raw_num) {
	case ISP_PRERAW0:
		dma_id = (chn_num == ISP_FE_CH0)
			? ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE
			: ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_SE;
		break;
	case ISP_PRERAW1:
		dma_id = (chn_num == ISP_FE_CH0)
			? ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_LE
			: ISP_BLK_ID_DMA_CTL_FE1_AE_HIST_SE;
		break;
	case ISP_PRERAW2:
		dma_id = ISP_BLK_ID_DMA_CTL_FE2_AE_HIST_LE;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_num[%d]\n", raw_num);
		break;
	}

	return dma_id;
}

int clsc_dma_find_hwid(enum sop_isp_raw raw_num)
{
	int dma_id = ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE;

	switch (raw_num) {
	case ISP_PRERAW0:
		dma_id = ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE;
		break;
	case ISP_PRERAW1:
		dma_id = ISP_BLK_ID_DMA_CTL_FE1_CLSC_LE;
		break;
	case ISP_PRERAW2:
		dma_id = ISP_BLK_ID_DMA_CTL_FE2_CLSC_LE;
		break;
	default:
		vi_pr(VI_ERR, "invalid raw_num[%d]\n", raw_num);
		break;
	}
	return dma_id;
}
/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void isp_first_frm_reset(struct isp_ctx *ctx, u8 reset, bool is_drop_frm)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t mctf = ctx->phys_regs[ISP_BLK_ID_TNR];
	bool is_3dnr = true;

	is_3dnr = ISP_RD_BITS(mctf, reg_isp_444_422_t, reg_0, tdnr_enable);

	if (!ctx->is_3dnr_on || !is_3dnr)
		return;

	//0: reg_first_frame_reset(in IP)
	//[0]: no useful
	//[1]: mctf first set en
	//[2]: cnr first set en
	//[3]: no useful
	ISP_WR_BITS(isptopb, reg_isp_top_t, first_frame, first_frame_top, reset ? 0xf : 0x0);

	//reg_first_frame_sw
	//[0]: drc first set
	//[1]: ldci first set
	//[2]: tnr first set
	//[3]: cnr first set
	ISP_WR_BITS(isptopb, reg_isp_top_t, first_frame, first_frame_sw, reset ? 0xf : 0x0);

	//[0,4]rdma y/uv/mv/mo/fcb
	//[5,10]wdma y/uv/mv/mo/fcb/msp
	if (is_drop_frm) {
		ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, dma_enable, reset ? 0x0 : 0x7FF);
	} else {
		ISP_WR_BITS(mctf, reg_isp_444_422_t, reg_0, dma_enable, reset ? 0x7E0 : 0x7FF);
	}

	vi_pr(VI_DBG, "reset_%d\n", reset);
}


static void _ispblk_isptop_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_scenarios_ctrl scene_ctrl;
	union reg_isp_top_ip_enable1 ip_en1;
	union reg_isp_top_ip_enable2 ip_en2;

	scene_ctrl.raw = ISP_RD_REG(isptopb, reg_isp_top_t, scenarios_ctrl);
	ip_en1.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ip_enable1);
	ip_en2.raw = ISP_RD_REG(isptopb, reg_isp_top_t, ip_enable2);

	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			scene_ctrl.bits.pre2yuv_422_enable = 1;
			scene_ctrl.bits.hdr_enable = 0;
			scene_ctrl.bits.be2raw_l_enable = 0;
			scene_ctrl.bits.be2raw_s_enable = 0;
			scene_ctrl.bits.pre2be_l_enable = 0;
			scene_ctrl.bits.pre2be_s_enable = 0;
			scene_ctrl.bits.be_rdma_l_enable = 1;
			scene_ctrl.bits.be_rdma_s_enable = 0;
			scene_ctrl.bits.yuv_edge_en = 0;
			scene_ctrl.bits.yuv_in_sel_mode = 1;
			scene_ctrl.bits.yuv_format = 1;
			scene_ctrl.bits.be_src_sel = 0;
			// close ip
			// raw_top
			ip_en1.bits.raw_llsc_enable = 0;
			ip_en1.bits.raw_clsc_enable = 0;
			ip_en1.bits.raw_drc_enable = 0;
			ip_en1.bits.raw_dpc_enable = 0;
			ip_en1.bits.raw_ae_0_enable = 0;
			ip_en1.bits.raw_ae_1_enable = 0;
			// rgb_top
			ip_en1.bits.rgb_clut_enable = 0;
			ip_en1.bits.sram_ee_ext_enable = 0;
			ip_en1.bits.pfr_enable = 0;
			ip_en1.bits.sram_pfr_enable = 0;
			ip_en1.bits.ee_ext_enable = 0;
			ip_en1.bits.rgb_ccm_enable = 0;
			ip_en1.bits.rgb_gamma_enable = 0;
			ip_en1.bits.rgb_dhz_enable = 0;
			ip_en1.bits.rgb_rgbdither_enable = 0;
			ip_en1.bits.rgb_clut_enable = 0;
			// yuv_top
			ip_en2.bits.yuv_ee_f_enable = 0;
			ip_en2.bits.yuv_ee_b_enable = 0;
			ip_en2.bits.yuv_3dnr_enable = 0;
			ip_en2.bits.yuv_cnr_enable = 0;
			ip_en2.bits.yuv_postee_enable = 0;
			ip_en2.bits.yuv_ycurve_enable = 0;
			ip_en2.bits.yuv_ldci_enable = 0;
		} else if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			scene_ctrl.bits.pre2yuv_422_enable = 0;
			scene_ctrl.bits.hdr_enable = 0;
			scene_ctrl.bits.be2raw_l_enable = 0;
			scene_ctrl.bits.be2raw_s_enable = 0;
			scene_ctrl.bits.be_rdma_l_enable = 0;
			scene_ctrl.bits.be_rdma_s_enable = 0;
		}
	} else { //RGB sensor
		scene_ctrl.bits.pre2yuv_422_enable = 0;
		scene_ctrl.bits.yuv_in_sel_mode = 0;
		scene_ctrl.bits.yuv_format = 0;
		scene_ctrl.bits.be_src_sel = ctx->isp_pipe_cfg[pipe].bind_raw;
		scene_ctrl.bits.hdr_enable = ctx->isp_pipe_cfg[pipe].is_hdr_on;
		scene_ctrl.bits.pre2be_l_enable = _is_all_online(ctx);
		scene_ctrl.bits.pre2be_s_enable = 0;
		scene_ctrl.bits.yuv_edge_en = 1;
		scene_ctrl.bits.be2raw_l_enable = 1;
		scene_ctrl.bits.be2raw_s_enable = ctx->isp_pipe_cfg[pipe].is_hdr_on;
		scene_ctrl.bits.be_rdma_l_enable = 1;
		scene_ctrl.bits.be_rdma_s_enable = ctx->isp_pipe_cfg[pipe].is_hdr_on;
		// close ip
		// raw_top
		ip_en1.bits.raw_llsc_enable = 1;
		ip_en1.bits.raw_clsc_enable = 1;
		ip_en1.bits.raw_drc_enable = 1;
		ip_en1.bits.raw_dpc_enable = 1;
		ip_en1.bits.raw_ae_0_enable = 3;
		ip_en1.bits.raw_ae_1_enable = 1;
		// rgb_top
		ip_en1.bits.rgb_clut_enable = 1;
		ip_en1.bits.sram_ee_ext_enable = 1;
		ip_en1.bits.pfr_enable = 1;
		ip_en1.bits.sram_pfr_enable = 1;
		ip_en1.bits.ee_ext_enable = 1;
		ip_en1.bits.rgb_ccm_enable = 1;
		ip_en1.bits.rgb_gamma_enable = 1;
		ip_en1.bits.rgb_dhz_enable = 1;
		ip_en1.bits.rgb_rgbdither_enable = 1;
		ip_en1.bits.rgb_clut_enable = 1;
		// yuv_top
		ip_en2.bits.yuv_ee_f_enable = 1;
		ip_en2.bits.yuv_ee_b_enable = 1;
		ip_en2.bits.yuv_3dnr_enable = 1;
		ip_en2.bits.yuv_cnr_enable = 1;
		ip_en2.bits.yuv_postee_enable = 1;
		ip_en2.bits.yuv_ycurve_enable = 1;
		ip_en2.bits.yuv_ldci_enable = 1;
	}

	ISP_WR_REG(isptopb, reg_isp_top_t, scenarios_ctrl, scene_ctrl.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable1, ip_en1.raw);
	ISP_WR_REG(isptopb, reg_isp_top_t, ip_enable2, ip_en2.raw);
}

void _ispblk_vi_sel_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	ispblk_preraw_vi_sel_config(ctx);
}

void _ispblk_rawtop_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, ctx->isp_pipe_cfg[pipe].bind_raw);
	int blk_id = clsc_find_hwid(phy_raw, 0);
	uintptr_t clsc = ctx->phys_regs[blk_id];
	uintptr_t bnr = ctx->phys_regs[ISP_BLK_ID_BNR];
	uintptr_t lscr = ctx->phys_regs[ISP_BLK_ID_LSCR];
	uintptr_t blc_db_wb_0 = ctx->phys_regs[ISP_BLK_ID_BLC_DG_WB0];
	uintptr_t blc_db_wb_1 = ctx->phys_regs[ISP_BLK_ID_BLC_DG_WB1];
	uintptr_t dpc = ctx->phys_regs[ISP_BLK_ID_DPC];
	uintptr_t map_curve = ctx->phys_regs[ISP_BLK_ID_MAPCURVE];
	uintptr_t cfa = ctx->phys_regs[ISP_BLK_ID_CFA];

	ispblk_rawtop_config(ctx);

	// close raw_top ip
	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			//AF
			ispblk_af_config(ctx, false);
			//LSC
			ISP_WR_BITS(clsc, reg_isp_lsc_t, sc_wrap_0, lsc_enable, 0);
			//LSCR
			ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_1, lscr_enable, 0);
			ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_1, blc_enable, 0);
			ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_width, ctx->isp_pipe_cfg[pipe].post_img_w - 1);
			ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_height, ctx->isp_pipe_cfg[pipe].post_img_h - 1);
			ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, lsc_bayer_starting, ctx->isp_pipe_cfg[pipe].rgb_color_mode);
			//BNR
			ISP_WR_BITS(bnr, reg_isp_bnr_t, bnr_00, u1_bnr_enable, 0);
			//BLC
			ISP_WR_BITS(blc_db_wb_0, reg_blc_dg_wb_t, base_config, blc_le_enable, 0);
			ISP_WR_BITS(blc_db_wb_0, reg_blc_dg_wb_t, base_config, blc_se_enable, 0);
			ISP_WR_BITS(blc_db_wb_0, reg_blc_dg_wb_t, base_config, wbg_le_enable, 0);
			ISP_WR_BITS(blc_db_wb_0, reg_blc_dg_wb_t, base_config, wbg_se_enable, 0);
			ISP_WR_BITS(blc_db_wb_1, reg_blc_dg_wb_t, base_config, blc_le_enable, 0);
			ISP_WR_BITS(blc_db_wb_1, reg_blc_dg_wb_t, base_config, blc_se_enable, 0);
			ISP_WR_BITS(blc_db_wb_1, reg_blc_dg_wb_t, base_config, wbg_le_enable, 0);
			ISP_WR_BITS(blc_db_wb_1, reg_blc_dg_wb_t, base_config, wbg_se_enable, 0);
			//DPC
			ISP_WR_BITS(dpc, reg_isp_dpc_t, base_config, dpc_enable, 0);
			ISP_WR_BITS(dpc, reg_isp_dpc_t, base_config, spc_enable, 0);
			//GE
			ISP_WR_BITS(dpc, reg_isp_dpc_t, base_config, ge_enable, 0);
			//DRC
			ispblk_drc_config(ctx, false);
			//FUSION
			ispblk_fusion_config(ctx, false, ISP_FS_OUT_LONG);
			//MAP_CURVE
			ISP_WR_BITS(map_curve, reg_map_curve_t, reg_01, u1_fcurve16_en, 0);
			//CFA
			ISP_WR_BITS(cfa, reg_isp_cfa_t, reg_00, cfa_enable, 0);
		}
	} else {
		//AF
		ispblk_af_config(ctx, true);
		//DRC
		ispblk_drc_config(ctx, false);
		//FUSION
		ispblk_fusion_config(ctx, !_is_all_online(ctx), ctx->cfg_info.is_hdr_on ? ISP_FS_OUT_FS : ISP_FS_OUT_LONG);
		//LSCR
		ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_width, ctx->isp_pipe_cfg[pipe].post_img_w - 1);
		ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_2, img_height, ctx->isp_pipe_cfg[pipe].post_img_h - 1);
		ISP_WR_BITS(lscr, reg_isp_lscr_t, sc_wrap_0, lsc_bayer_starting, ctx->isp_pipe_cfg[pipe].rgb_color_mode);
	}
}

void _ispblk_rgbtop_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	uintptr_t pfr = ctx->phys_regs[ISP_BLK_ID_PFR];
	uintptr_t ccm = ctx->phys_regs[ISP_BLK_ID_CCM];
	uintptr_t gamma = ctx->phys_regs[ISP_BLK_ID_RGBGAMMA];
	uintptr_t clut = ctx->phys_regs[ISP_BLK_ID_CLUT];
	uintptr_t ee_ext = ctx->phys_regs[ISP_BLK_ID_PRE_EE_EXT];
	uintptr_t csc = ctx->phys_regs[ISP_BLK_ID_CSC];

	ispblk_rgbtop_config(ctx);

	// close raw_top ip
	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			//PFR
			ISP_WR_BITS(pfr, reg_pfr_t, pfr_reg0, pfr_en, 0);
			//CCM
			ISP_WR_BITS(ccm, reg_isp_ccm_t, ccm_ctrl, ccm_enable, 0);
			//RGB_GAMMA
			ISP_WR_BITS(gamma, reg_isp_gamma_t, gamma_ctrl, gamma_enable, 0);
			//CLUT
			ISP_WR_BITS(clut, reg_isp_clut_t, clut_ctrl, clut_enable, 0);
			//EE_EXIT
			ISP_WR_BITS(ee_ext, reg_ee_ext_t, ee_ext_reg0, ee_enable, 0);
			//CSC
			ISP_WR_BITS(csc, reg_isp_csc_t, reg_0, csc_enable, 0);
			//RGB_dither
			ispblk_rgbdither_config(ctx, false, false, false, false);
		}
	} else {
		//RGB_dither
		ispblk_rgbdither_config(ctx, true, false, false, false);
	}
}

void _ispblk_yuvtop_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	uintptr_t ee_front = ctx->phys_regs[ISP_BLK_ID_PRE_EE_FRONT];
	uintptr_t ee_back = ctx->phys_regs[ISP_BLK_ID_PRE_EE_BACK];
	uintptr_t post_ee = ctx->phys_regs[ISP_BLK_ID_POST_EE];
	uintptr_t cacp = ctx->phys_regs[ISP_BLK_ID_CA];
	uintptr_t ca_lite = ctx->phys_regs[ISP_BLK_ID_CA_LITE];
	uintptr_t ycur = ctx->phys_regs[ISP_BLK_ID_YCURVE];
	uintptr_t resize = ctx->phys_regs[ISP_BLK_ID_RESIZE];

	ispblk_yuvtop_config(ctx);

	//close yuv_top ip
	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor) { //YUV sensor
		if (ctx->isp_pipe_cfg[pipe].yuv_scene_mode == ISP_YUV_SCENE_ISP) {
			//PRE_EE
			ISP_WR_BITS(ee_front, reg_ee_add_t, ee_add_reg0, ee_enable, 0);
			ISP_WR_BITS(ee_back, reg_ee_add_back_t, ee_add_b_reg0, ee_enable, 0);
			//EE
			ISP_WR_BITS(post_ee, reg_isp_ee_t, reg_00, ee_enable, 0);
			//TNR
			ispblk_yuvdither_config(ctx, 0, false, false, false, false);
			ispblk_yuvdither_config(ctx, 1, false, false, false, false);
			//CNR
			ispblk_cnr_config(ctx, false, 0, 0, 0);
			//CACP
			ISP_WR_BITS(cacp, reg_ca_t, reg_00, cacp_enable, 0);
			//CA_LITE
			ISP_WR_BITS(ca_lite, reg_ca_lite_t, reg_00, ca_lite_enable, 0);
			//YCUR
			ISP_WR_BITS(ycur, reg_isp_ycurv_t, ycur_ctrl, ycur_enable, 0);
			//LDCI&DCI
			ispblk_ldci_config(ctx, false, false);
			//RESIZE
			if (!ctx->isp_pipe_cfg[pipe].is_offline_scaler) {
				ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, resize_enable, false);
				ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, dma_enable, false);
			}
		}
	} else {
		//TNR
		ispblk_yuvdither_config(ctx, 0, true, true, true, true);
		ispblk_yuvdither_config(ctx, 1, true, true, true, true);
		//CNR
		ispblk_cnr_config(ctx, true, 0, 0, 0);
		//LDCI&DCI
		ispblk_ldci_config(ctx, false, false);
		//RESIZE
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, resize_enable,
				osal_atomic_read(&ctx->isp_pipe_cfg[pipe].resize_en));
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, dma_enable,
				osal_atomic_read(&ctx->isp_pipe_cfg[pipe].resize_en));
	}
}

void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const u8 pipe)
{
	_ispblk_isptop_cfg_update(ctx, pipe);
	_ispblk_vi_sel_cfg_update(ctx, pipe);
	_ispblk_rawtop_cfg_update(ctx, pipe);
	_ispblk_rgbtop_cfg_update(ctx, pipe);
	_ispblk_yuvtop_cfg_update(ctx, pipe);
}

int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, u32 _w, u32 _h)
{
	u32 len = 0, num = 0, w;

	switch (dmaid) {
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	{
		w = _w;
		num = _h;
		if (ctx->is_dpcm_on)
			w >>= 1;

		len = 3 * UPPER(w, 1);

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	vi_pr(VI_INFO, "dmaid=%d, size=%d\n", dmaid, len * num);

	return len * num;
}

int isp_frm_err_handler(struct isp_ctx *ctx, const enum sop_isp_raw err_raw_num, const u8 step)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	union reg_isp_top_sw_rst sw_rst;
	union vi_sys_reset vi_rst;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, err_raw_num);

	if (step == 1) {
		int id = -1;
		uintptr_t ba = 0;

		if (ctx->isp_csi_cfg[err_raw_num].is_bt_demux) {
			id = csibdg_find_hwid(phy_raw);
			ba = ctx->phys_regs[id];

			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch0, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch1, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch2, 0);
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, frame_vld, frame_vld_ch3, 0);

		} else {
			id = fe_find_hwid(phy_raw);
			ba = ctx->phys_regs[id];

			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch0, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch1, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch2, 0);
			ISP_WR_BITS(ba, reg_pre_raw_fe_t, pre_raw_frame_vld, fe_frame_vld_ch3, 0);
		}
	} else if (step == 3) {
		int id = csibdg_find_hwid(phy_raw);
		uintptr_t ba = ctx->phys_regs[id];
		uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
		uintptr_t wdma_com_2 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE2];
		uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
		uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
		uintptr_t rdma_com_2 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE2];
		uintptr_t rdma_com_3 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE3];
		u8 count = 10;

		if (ctx->isp_csi_cfg[err_raw_num].is_bt_demux) {
			ISP_WR_BITS(ba, reg_isp_csi_bdg_lite_t, csi_bdg_top_ctrl, abort, 1);
		} else {
			ISP_WR_BITS(ba, reg_isp_csi_bdg_t, csi_bdg_top_ctrl, abort, 1);
		}

		while (--count > 0) {
			if (ISP_RD_BITS(wdma_com_1, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(wdma_com_2, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(wdma_com_3, reg_wdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(rdma_com_1, reg_rdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(rdma_com_2, reg_rdma_core_t, norm_status0, abort_done) &&
				ISP_RD_BITS(rdma_com_3, reg_rdma_core_t, norm_status0, abort_done)) {
				vi_pr(VI_INFO, "W/RDMA_CORE abort done, count(%d)\n", count);
				break;
			}
			osal_usleep_range(1, 5);
		}

		if (count == 0) {
			vi_pr(VI_ERR, "WDMA/RDMA_CORE abort fail\n");
			return -1;
		}
	} else if (step == 4) {
		sw_rst.raw = 0;
		sw_rst.bits.axi_rst = 1;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);
		sw_rst.raw = 0x6FF;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);

		vi_rst.raw = 0;
		vi_rst.b.isp_top = 1;
		vi_sys_set_reset(vi_rst);

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull up\n");
	} else if (step == 5) {
		vi_rst = vi_sys_get_reset();
		vi_rst.b.isp_top = 0;
		vi_sys_set_reset(vi_rst);

		sw_rst.raw = ISP_RD_REG(isptopb, reg_isp_top_t, sw_rst);
		sw_rst.bits.axi_rst = 0;
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, sw_rst.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, sw_rst, 0);

		vi_pr(VI_INFO, "ISP and vip_sys isp rst pull down\n");
	} else if (step == 6) {
		uintptr_t fe0 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
		uintptr_t fe1 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE1];
		uintptr_t fe2 = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE2];
		uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
		u8 cnt = 10;

		vi_pr(VI_INFO, "Wait ISP idle\n");

		while (--cnt > 0) {
			if ((ISP_RD_REG(fe0, reg_pre_raw_fe_t, fe_idle_info) == 0x7FFFF) &&
			    (ISP_RD_REG(fe1, reg_pre_raw_fe_t, fe_idle_info) == 0x7FFFF) &&
			    (ISP_RD_REG(fe2, reg_pre_raw_fe_t, fe_idle_info) == 0x3FFFF) &&
			    (ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle)) == 0x3FF) {
				vi_pr(VI_INFO, "FE/BE/ISP idle done, count(%d)\n", cnt);
				break;
			}
			osal_usleep_range(1, 5);
		}

		if (cnt == 0) {
			vi_pr(VI_ERR, "FE(0:0x%x, 1:0x%x, 2:0x%x/ISP(0x%x) not idle.",
				ISP_RD_REG(fe0, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe1, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(fe2, reg_pre_raw_fe_t, fe_idle_info),
				ISP_RD_REG(isptopb, reg_isp_top_t, blk_idle));
			return -1;
		}
	}
	return 0;
}
/****************************************************************************
 *	YUV Bypass Control Flow Config
 ****************************************************************************/
//TODO maybe use sw control
u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr,
				 const enum sop_isp_raw raw_num)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];
	u32 len = 0, num = 0;

	switch (dmaid) {
	/* csibdg */
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG2:
	case ISP_BLK_ID_DMA_CTL_CSI0_BDG3:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG0:
	case ISP_BLK_ID_DMA_CTL_CSI1_BDG1:
	case ISP_BLK_ID_DMA_CTL_CSI2_BDG0:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE0:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE1:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE2:
	case ISP_BLK_ID_DMA_CTL_BT0_LITE3:
	/* preraw_vi_sel */
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE:
	case ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE:
	{
		/* csibdg */
		num = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].h;
		len = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w * 2;

		break;
	}
	default:
		break;
	}

	len = VI_ALIGN(len);

	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_seglen, len);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_stride, len);
	ISP_WR_REG(dmab, reg_isp_dma_ctl_t, dma_segnum, num);

	if (buf_addr) {
		ISP_WR_REG(dmab, reg_isp_dma_ctl_t, base_addr, (buf_addr & 0xFFFFFFFF));
		ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, baseh, ((buf_addr >> 32) & 0xFFFFFFFF));
	}

	return len * num;
}

/****************************************************************************
 *	Slice buffer Control
 ****************************************************************************/
void vi_calculate_slice_buf_setting(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u32 main_le_num = 0, main_se_num = 0;
	u32 main_le_size = 0, main_se_size = 0;
	u16 main_le_w_th = 0, main_le_r_th = 0;
	u16 main_se_w_th = 0, main_se_r_th = 0;

	u32 line_delay = slc_b_cfg.line_delay, buffer = slc_b_cfg.buffer;
	u32 main_max_grid_size = slc_b_cfg.main_max_grid_size;

	u32 w = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH0].w;

	// Calculate the ring buffer line number
	main_le_num = line_delay + 2 * main_max_grid_size + buffer;
	main_le_size = VI_256_ALIGN(main_le_num * ((w * 3) / 2));

	// Calculate the r/w threshold
	main_le_r_th = 2 * main_max_grid_size;
	main_le_w_th = main_le_num - 1;

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		w = ctx->isp_csi_cfg[raw_num].crop[ISP_FE_CH1].w;
		main_se_num = 2 * main_max_grid_size + buffer;
		main_se_size = VI_256_ALIGN(main_se_num * ((w * 3) / 2));

		main_se_r_th = 2 * main_max_grid_size;
		main_se_w_th = main_se_num - 1;
	}

	slc_b_cfg.main_path.le_buf_size = main_le_size;
	slc_b_cfg.main_path.le_w_thshd  = main_le_w_th;
	slc_b_cfg.main_path.le_r_thshd  = main_le_r_th;

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		slc_b_cfg.main_path.se_buf_size = main_se_size;
		slc_b_cfg.main_path.se_w_thshd  = main_se_w_th;
		slc_b_cfg.main_path.se_r_thshd  = main_se_r_th;
	}

	vi_pr(VI_INFO, "main_path le_buf_config=(%d, %d, %d), se_buf_config=(%d, %d, %d)\n",
			slc_b_cfg.main_path.le_buf_size,
			slc_b_cfg.main_path.le_w_thshd,
			slc_b_cfg.main_path.le_r_thshd,
			slc_b_cfg.main_path.se_buf_size,
			slc_b_cfg.main_path.se_w_thshd,
			slc_b_cfg.main_path.se_r_thshd);
}

void isp_slice_buf_trig(struct isp_ctx *ctx)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	if (_is_fe_post_slice(ctx)) {
		ISP_WR_REG(isptopb, reg_isp_top_t, vi_sel_frame_valid,
			ctx->cfg_info.is_hdr_on ? 0xF : 0x3);
	}
}

void _ispblk_dma_slice_config(struct isp_ctx *ctx, int dmaid, int en)
{
	uintptr_t dmab = ctx->phys_regs[dmaid];

	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, dma_slicesize, slice_size, 1);
	ISP_WR_BITS(dmab, reg_isp_dma_ctl_t, sys_control, slice_enable, en);
}

void ispblk_slice_buf_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, u8 en)
{
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t wdma_com_1 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE1];
	uintptr_t rdma_com_1 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE1];
	union reg_wdma_core_ring_buffer_en w_ring_buf_en_1;
	union reg_rdma_core_ring_buffer_en r_ring_buf_en_1;

	// Multi-sensor don't support slice buffer mode.
	if (en) {
		union reg_isp_top_sclie_enable			slice_en;
		union reg_isp_top_w_slice_thresh_main		w_th_main;
		union reg_isp_top_r_slice_thresh_main		r_th_main;

		slice_en.raw = 0;
		slice_en.bits.slice_enable_main_lexp = en;

		w_th_main.raw = 0;
		w_th_main.bits.w_slice_thr_main_lexp = slc_b_cfg.main_path.le_w_thshd;

		r_th_main.raw = 0;
		r_th_main.bits.r_slice_thr_main_lexp = slc_b_cfg.main_path.le_r_thshd;

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			slice_en.bits.slice_enable_main_sexp = en;

			w_th_main.bits.w_slice_thr_main_sexp = slc_b_cfg.main_path.se_w_thshd;
			r_th_main.bits.r_slice_thr_main_sexp = slc_b_cfg.main_path.se_r_thshd;
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sclie_enable, slice_en.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, w_slice_thresh_main, w_th_main.raw);
		ISP_WR_REG(isptopb, reg_isp_top_t, r_slice_thresh_main, r_th_main.raw);

		// wdma/rdma core config
		w_ring_buf_en_1.raw = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_CSI0_BDG0);

		r_ring_buf_en_1.raw = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_en);
		r_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_PRE_RAW_VI_SEL_LE);

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_CSI0_BDG1);

			r_ring_buf_en_1.raw |= (1 << ISP_DMA_ID_PRE_RAW_VI_SEL_SE);
		}

		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_1.raw);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_size0, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_size1, slc_b_cfg.main_path.se_buf_size);
		ISP_WR_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_en, r_ring_buf_en_1.raw);
		ISP_WR_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_size0, slc_b_cfg.main_path.le_buf_size);
		ISP_WR_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_size4, slc_b_cfg.main_path.se_buf_size);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG0, true);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, true);

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG1, true);
			_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE, true);
		}
	} else {
		w_ring_buf_en_1.raw = ISP_RD_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en);
		w_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_CSI0_BDG0);

		r_ring_buf_en_1.raw = ISP_RD_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_en);
		r_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_PRE_RAW_VI_SEL_LE);

		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			w_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_CSI0_BDG1);
			r_ring_buf_en_1.raw &= ~(1 << ISP_DMA_ID_PRE_RAW_VI_SEL_SE);
		}

		ISP_WR_REG(isptopb, reg_isp_top_t, sclie_enable, 0);
		ISP_WR_REG(wdma_com_1, reg_wdma_core_t, ring_buffer_en, w_ring_buf_en_1.raw);
		ISP_WR_REG(rdma_com_1, reg_rdma_core_t, ring_buffer_en, r_ring_buf_en_1.raw);

		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG0, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_CSI0_BDG1, false);
		_ispblk_dma_slice_config(ctx, ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_SE, false);
	}
}

void ispblk_fbc_debug_info(struct isp_ctx *ctx)
{
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_3 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE3];
	u32 y_wdma, c_wdma;
	u32 y_rdma, c_rdma;

	y_wdma = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, next_dma_addr_sts6);
	c_wdma = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, next_dma_addr_sts7);

	y_rdma = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, next_dma_addr_sts5);
	c_rdma = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, next_dma_addr_sts6);

	vi_pr(VI_DBG, "next y wdma=0x%x, c_wdma=0x%x, y_rdma=0x%x, c_rdma=0x%x\n", y_wdma, c_wdma, y_rdma, c_rdma);
}

void ispblk_fbc_ring_buf_config(struct isp_ctx *ctx, u8 en)
{
	//WDMA_CORE, RDMA_CORE ring buffer ctrl
	uintptr_t wdma_com_3 = ctx->phys_regs[ISP_BLK_ID_WDMA_CORE3];
	uintptr_t rdma_com_3 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE3];
	union reg_wdma_core_disable_seglen	disable_seglen;
	union reg_wdma_core_ring_buffer_en	ring_buf_en;
	u8 pipe = ctx->cfg_info.pipe;

	if (en) {
		vi_fbc_calculate_size(ctx, pipe);

		disable_seglen.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, disable_seglen);
		disable_seglen.bits.seglen_disable |= ((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, disable_seglen, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable |= ((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable |= ((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_en, ring_buf_en.raw);

		//WDMA ctrl cfg
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_size6, fbc_cfg.y_buf_size);
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_size7, fbc_cfg.c_buf_size);
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, up_ring_base,
					((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C)));

		//RDMA ctrl cfg
		ISP_WR_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_size5, fbc_cfg.y_buf_size);
		ISP_WR_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_size6, fbc_cfg.c_buf_size);
		ISP_WR_REG(rdma_com_3, reg_rdma_core_t, up_ring_base,
					((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C)));
	} else {
		disable_seglen.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, disable_seglen);
		disable_seglen.bits.seglen_disable &= ~((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, disable_seglen, disable_seglen.raw);

		ring_buf_en.raw = ISP_RD_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable &= ~((1 << ISP_DMA_ID_TNR_ST_Y) | (1 << ISP_DMA_ID_TNR_ST_C));
		ISP_WR_REG(wdma_com_3, reg_wdma_core_t, ring_buffer_en, ring_buf_en.raw);

		ring_buf_en.raw = ISP_RD_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_en);
		ring_buf_en.bits.ring_enable &= ~((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C));
		ISP_WR_REG(rdma_com_3, reg_rdma_core_t, ring_buffer_en, ring_buf_en.raw);
	}
}

void ispblk_fbc_clear_fbcd_ring_base(struct isp_ctx *ctx)
{
	uintptr_t rdma_com_3 = ctx->phys_regs[ISP_BLK_ID_RDMA_CORE3];

	ISP_WR_REG(rdma_com_3, reg_rdma_core_t, up_ring_base,
				((1 << ISP_DMA_ID_TNR_LD_Y) | (1 << ISP_DMA_ID_TNR_LD_C)));
}

void ispblk_fbc_chg_to_sw_mode(struct isp_ctx *ctx)
{
	uintptr_t y_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_TNR_LD_Y];
	uintptr_t uv_rdma = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_TNR_LD_C];
	union reg_isp_dma_ctl_sys_control sys_ctrl;

	// Y/UV rdma config seglen to HW mode, other SW mode
	sys_ctrl.raw = ISP_RD_REG(y_rdma, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= true;
	sys_ctrl.bits.seglen_sel	= false;
	sys_ctrl.bits.segnum_sel	= true;
	ISP_WR_REG(y_rdma, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);

	sys_ctrl.raw = ISP_RD_REG(uv_rdma, reg_isp_dma_ctl_t, sys_control);
	sys_ctrl.bits.base_sel		= 0x1;
	sys_ctrl.bits.stride_sel	= true;
	sys_ctrl.bits.seglen_sel	= false;
	sys_ctrl.bits.segnum_sel	= true;
	ISP_WR_REG(uv_rdma, reg_isp_dma_ctl_t, sys_control, sys_ctrl.raw);
}

void ispblk_mctf_mmap_status(struct isp_ctx *ctx, uint32_t *still, uint32_t *trans, uint32_t *motion)
{
	uintptr_t mctf = ctx->phys_regs[ISP_BLK_ID_TNR];

	*still = ISP_RD_REG(mctf, reg_isp_444_422_t, reg_117);
	*trans = ISP_RD_REG(mctf, reg_isp_444_422_t, reg_118);
	*motion = ISP_RD_REG(mctf, reg_isp_444_422_t, reg_119);

	vi_pr(VI_DBG, "still(%d), trans(%d), motion(%d)\n", *still, *trans, *motion);
}

void ispblk_mctf_resize_config(struct isp_ctx *ctx, uint8_t pipe, u64 phy_addr, bool en)
{
	uintptr_t resize = ctx->phys_regs[ISP_BLK_ID_RESIZE];
	int src_width = ctx->isp_pipe_cfg[pipe].crop.w >> 2;
	int src_height = ctx->isp_pipe_cfg[pipe].crop.h >> 2;
	int dst_width = ctx->isp_pipe_cfg[pipe].resize_w >> 2;
	int dst_height = ctx->isp_pipe_cfg[pipe].resize_h >> 2;

	if (en) {
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_04, resize_fg_width, src_width);
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_04, resize_fg_height, src_height);
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_08, resize_dst_width, dst_width);
		ISP_WR_BITS(resize, reg_motion_resize_t, resize_08, resize_dst_height, dst_height);

		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL_RESIZE, phy_addr);
	}

	ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, resize_enable, en);
	ISP_WR_BITS(resize, reg_motion_resize_t, resize_00, dma_enable, en);

}

void ispblk_fe_sof_interrupt_ctrl(struct isp_ctx *ctx, bool en)
{
	union reg_isp_top_int_event2_en ev2_en;
	uintptr_t isptopb = ctx->phys_regs[ISP_BLK_ID_ISPTOP];

	ev2_en.raw = 0;
	ev2_en.bits.frame_start_enable_fe0	= en ? 0xF : 0;
	ev2_en.bits.frame_start_enable_fe1	= en ? 0xF : 0;
	ev2_en.bits.frame_start_enable_fe2	= en ? 0x3 : 0;

	ISP_WR_REG(isptopb, reg_isp_top_t, int_event2_en, ev2_en.raw);
}

void ispblk_overflow_record_reg_info(struct isp_ctx *ctx, void *reg_info)
{
	u8 i = 0;
	uintptr_t isptop = ctx->phys_regs[ISP_BLK_ID_ISPTOP];
	uintptr_t preraw_fe = ctx->phys_regs[ISP_BLK_ID_PRE_RAW_FE0];
	uintptr_t yuvtop = ctx->phys_regs[ISP_BLK_ID_YUVTOP];
	uintptr_t rgbtop = ctx->phys_regs[ISP_BLK_ID_RGBTOP];
	uintptr_t rawtop1 = ctx->phys_regs[ISP_BLK_ID_RAWTOP1];
	uintptr_t rdma28 = ctx->phys_regs[ISP_BLK_ID_DMA_CTL_PRE_RAW_VI_SEL_LE];
	struct vi_overflow_reg_info *vi_info = (struct vi_overflow_reg_info *)reg_info;

	//isp_top
	vi_info->isp_top.blk_idle = ISP_RD_REG(isptop, reg_isp_top_t, blk_idle);
	for (i = 0; i <= 6; i++) {
		//Debug
		ISP_WR_BITS(isptop, reg_isp_top_t, dummy, dbus_sel, i);
		vi_info->isp_top.dbus_sel[i].r_0 = ISP_RD_REG(isptop, reg_isp_top_t, dbus0); //0x0A070040
		vi_info->isp_top.dbus_sel[i].r_4 = ISP_RD_REG(isptop, reg_isp_top_t, dbus1); //0x0A070044
		vi_info->isp_top.dbus_sel[i].r_8 = ISP_RD_REG(isptop, reg_isp_top_t, dbus2); //0x0A070048
		vi_info->isp_top.dbus_sel[i].r_c = ISP_RD_REG(isptop, reg_isp_top_t, dbus3); //0x0A07004C
	}

	//pre_raw_fe
	vi_info->preraw_fe.preraw_info = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, pre_raw_info);
	vi_info->preraw_fe.fe_idle_info = ISP_RD_REG(preraw_fe, reg_pre_raw_fe_t, fe_idle_info);

	//rawtop
	vi_info->rawtop.dma_idle = ISP_RD_REG(rawtop1, reg_raw_top1_t, raw_top_dma_idle);

	//rgbtop
	vi_info->rgbtop.xcnt_rpt = ISP_RD_BITS(rgbtop, reg_rgb_top_t, patgen_04, xcnt_rpt);
	vi_info->rgbtop.ycnt_rpt = ISP_RD_BITS(rgbtop, reg_rgb_top_t, patgen_04, ycnt_rpt);

	//yuvtop
	vi_info->yuvtop.xcnt_rpt = ISP_RD_BITS(yuvtop, reg_yuv_top_t, patgen4, xcnt_rpt);
	vi_info->yuvtop.ycnt_rpt = ISP_RD_BITS(yuvtop, reg_yuv_top_t, patgen4, ycnt_rpt);

	//rdma28
	ISP_WR_BITS(rdma28, reg_isp_dma_ctl_t, sys_control, dbg_sel, 0x1);
	vi_info->rdma28[0].dbg_sel = 0x1;
	vi_info->rdma28[0].status = ISP_RD_REG(rdma28, reg_isp_dma_ctl_t, dma_status);
	ISP_WR_BITS(rdma28, reg_isp_dma_ctl_t, sys_control, dbg_sel, 0x2);
	vi_info->rdma28[1].dbg_sel = 0x2;
	vi_info->rdma28[1].status = ISP_RD_REG(rdma28, reg_isp_dma_ctl_t, dma_status);
	vi_info->enable = true;
}