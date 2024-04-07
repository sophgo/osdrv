#include <linux/version.h>
#include <proc/vi_dbg_proc.h>
#include <vi_isp_buf_ctrl.h>

#define VI_DBG_PROC_NAME	"soph/vi_dbg"

extern int cvi_isp_rdy_buf_empty(struct cvi_vi_dev *vdev, const u8 chn_num);
extern struct isp_queue splt_out_q[][ISP_SPLT_CHN_MAX],
			pre_fe_in_q[][ISP_SPLT_CHN_MAX],
			pre_fe_out_q[][ISP_FE_CHN_MAX],
			pre_be_in_q, pre_be_in_se_q[],
			pre_be_out_q[],
			postraw_in_q[];

/* Switch the output of proc.
 *
 * 0 ~ ISP_BLK_ID_MAX: block reg-dump
 * 255: vi debug info
 */
static int proc_isp_mode = 255;

/*************************************************************************
 *	Proc functions
 *************************************************************************/
static inline void _vi_dbg_reg_dump(struct isp_ctx *ctx, struct seq_file *m, int blk)
{
	static u8 init_done;
	static struct isp_dump_info m_block[ISP_BLK_ID_MAX] = {0};

	if (init_done)
		goto DUMP;

#define BLK_INFO(_para, _name, _struct) \
	do {\
		_para[ISP_BLK_ID_##_name].phy_base = ISP_BLK_BA_##_name + ISP_TOP_PHY_REG_BASE;\
		_para[ISP_BLK_ID_##_name].reg_base = ctx->phys_regs[ISP_BLK_ID_##_name];\
		_para[ISP_BLK_ID_##_name].blk_size = sizeof(struct _struct) / 4;\
	} while (0)

	BLK_INFO(m_block, PRE_RAW_FE0, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG0, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG2, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI0_BDG3, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE0_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE0_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE0_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG0, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE0_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE0_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG1, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE0_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_FE1, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG1, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG2, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI1_BDG3, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE1_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE1_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE1_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG2, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE1_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE1_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG3, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE1_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_FE2, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG2, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI2_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI2_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE2_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE2_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE2_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG4, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE2_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE2_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG5, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE2_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_FE3, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG3, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI3_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI3_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE3_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE3_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE3_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG6, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE3_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE3_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG7, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE3_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_FE4, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG4, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI4_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI4_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE4_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE4_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE4_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG8, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE4_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE4_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG9, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE4_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_FE5, REG_PRE_RAW_FE_T);
	BLK_INFO(m_block, CSIBDG5, REG_ISP_CSI_BDG_T);
	BLK_INFO(m_block, DMA_CTL_CSI5_BDG0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_CSI5_BDG1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_FE5_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_FE5_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, RGBMAP_FE5_LE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG10, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE5_RGBMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RGBMAP_FE5_SE, REG_ISP_RGBMAP_T);
	BLK_INFO(m_block, RGBMAP_WBG11, REG_ISP_WBG_T);
	BLK_INFO(m_block, DMA_CTL_FE5_RGBMAP_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, PRE_RAW_BE, REG_PRE_RAW_BE_T);
	BLK_INFO(m_block, BE_CROP_LE, REG_CROP_T);
	BLK_INFO(m_block, BE_CROP_SE, REG_CROP_T);
	BLK_INFO(m_block, PRE_RAW_BE_BLC0, REG_ISP_BLC_T);
	BLK_INFO(m_block, PRE_RAW_BE_BLC1, REG_ISP_BLC_T);
	BLK_INFO(m_block, AF, REG_ISP_AF_T);
	BLK_INFO(m_block, DMA_CTL_AF_W, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DPC0, REG_ISP_DPC_T);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_BE_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_BE_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_WDMA, REG_PRE_WDMA_CTRL_T);
	// BLK_INFO(m_block, PCHK0, );
	// BLK_INFO(m_block, PCHK1, );
	BLK_INFO(m_block, RGBIR0, REG_ISP_RGBIR_T);
	BLK_INFO(m_block, DMA_CTL_RGBIR_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DPC1, REG_ISP_DPC_T);
	BLK_INFO(m_block, RGBIR1, REG_ISP_RGBIR_T);
	BLK_INFO(m_block, DMA_CTL_RGBIR_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, WDMA_CORE0, REG_WDMA_CORE_T);
	BLK_INFO(m_block, WDMA_CORE1, REG_WDMA_CORE_T);
	BLK_INFO(m_block, WDMA_CORE2, REG_WDMA_CORE_T);
	BLK_INFO(m_block, WDMA_CORE3, REG_WDMA_CORE_T);

	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_WDMA_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_WDMA_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE0_WDMA, REG_PRE_WDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_RDMA_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE0_RDMA_LE, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE0_RDMA_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE0_RDMA_SE, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_WDMA_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_WDMA_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE1_WDMA, REG_PRE_WDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_RDMA_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE1_RDMA_LE, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_SPLT_FE1_RDMA_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, SPLT_FE1_RDMA_SE, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, SPLT, REG_ISP_LINE_SPLITER_T);

	BLK_INFO(m_block, RAWTOP, REG_RAW_TOP_T);
	BLK_INFO(m_block, CFA0, REG_ISP_CFA_T);
	BLK_INFO(m_block, LSC0, REG_ISP_LSC_T);
	BLK_INFO(m_block, DMA_CTL_LSC_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, GMS, REG_ISP_GMS_T);
	BLK_INFO(m_block, DMA_CTL_GMS, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, AE_HIST0, REG_ISP_AE_HIST_T);
	BLK_INFO(m_block, DMA_CTL_AE_HIST_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, AE_HIST1, REG_ISP_AE_HIST_T);
	BLK_INFO(m_block, DMA_CTL_AE_HIST_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_RAW_RDMA0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RAW_RDMA0, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, DMA_CTL_RAW_RDMA1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RAW_RDMA1, REG_RAW_RDMA_CTRL_T);
	BLK_INFO(m_block, CFA1, REG_ISP_CFA_T);
	BLK_INFO(m_block, LSC1, REG_ISP_LSC_T);
	BLK_INFO(m_block, DMA_CTL_LSC_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, LMAP1, REG_ISP_LMAP_T);
	BLK_INFO(m_block, DMA_CTL_LMAP_SE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, BNR0, REG_ISP_BNR_T);
	BLK_INFO(m_block, BNR1, REG_ISP_BNR_T);
	BLK_INFO(m_block, RAW_CROP_LE, REG_CROP_T);
	BLK_INFO(m_block, RAW_CROP_SE, REG_CROP_T);
	BLK_INFO(m_block, LMAP0, REG_ISP_LMAP_T);
	BLK_INFO(m_block, DMA_CTL_LMAP_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, RAW_WBG0, REG_ISP_WBG_T);
	BLK_INFO(m_block, RAW_WBG1, REG_ISP_WBG_T);
	// BLK_INFO(m_block, PCHK2, );
	// BLK_INFO(m_block, PCHK3, );
	BLK_INFO(m_block, LCAC0, REG_ISP_LCAC_T);
	BLK_INFO(m_block, RGBCAC0, REG_ISP_RGBCAC_T);
	BLK_INFO(m_block, LCAC1, REG_ISP_LCAC_T);
	BLK_INFO(m_block, RGBCAC1, REG_ISP_RGBCAC_T);

	BLK_INFO(m_block, RGBTOP, REG_ISP_RGB_TOP_T);
	BLK_INFO(m_block, CCM0, REG_ISP_CCM_T);
	BLK_INFO(m_block, CCM1, REG_ISP_CCM_T);
	BLK_INFO(m_block, RGBGAMMA, REG_ISP_GAMMA_T);
	BLK_INFO(m_block, YGAMMA, REG_YGAMMA_T);
	BLK_INFO(m_block, MMAP, REG_ISP_MMAP_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_PRE_LE_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_PRE_SE_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_CUR_LE_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_CUR_SE_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_IIR_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_IIR_W, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_MMAP_AI_ISP, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, CLUT, REG_ISP_CLUT_T);
	BLK_INFO(m_block, DEHAZE, REG_ISP_DEHAZE_T);
	BLK_INFO(m_block, CSC, REG_ISP_CSC_T);
	BLK_INFO(m_block, RGB_DITHER, REG_ISP_RGB_DITHER_T);
	// BLK_INFO(m_block, PCHK4, );
	// BLK_INFO(m_block, PCHK5, );
	BLK_INFO(m_block, HIST_EDGE_V, REG_ISP_HIST_EDGE_V_T);
	BLK_INFO(m_block, DMA_CTL_HIST_EDGE_V, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, FUSION, REG_FUSION_T);
	BLK_INFO(m_block, LTM, REG_LTM_T);
	BLK_INFO(m_block, DMA_CTL_LTM_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_LTM_SE, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, YUVTOP, REG_YUV_TOP_T);
	BLK_INFO(m_block, TNR, REG_ISP_444_422_T);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_MO, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_MO, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_Y, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_TNR_ST_C, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, FBCE, REG_FBCE_T);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_Y, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_TNR_LD_C, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, FBCD, REG_FBCD_T);
	BLK_INFO(m_block, YUV_DITHER, REG_ISP_YUV_DITHER_T);
	BLK_INFO(m_block, CA, REG_CA_T);
	BLK_INFO(m_block, CA_LITE, REG_CA_LITE_T);
	BLK_INFO(m_block, YNR, REG_ISP_YNR_T);
	BLK_INFO(m_block, CNR, REG_ISP_CNR_T);
	BLK_INFO(m_block, EE_POST, REG_ISP_EE_T);
	BLK_INFO(m_block, YCURVE, REG_ISP_YCURV_T);
	BLK_INFO(m_block, DCI, REG_ISP_DCI_T);
	BLK_INFO(m_block, DMA_CTL_DCI, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DCI_GAMMA, REG_ISP_GAMMA_T);
	BLK_INFO(m_block, YUV_CROP_Y, REG_CROP_T);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_Y, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, YUV_CROP_C, REG_CROP_T);
	BLK_INFO(m_block, DMA_CTL_YUV_CROP_C, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, LDCI, REG_ISP_LDCI_T);
	BLK_INFO(m_block, DMA_CTL_LDCI_W, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_LDCI_R, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, EE_PRE, REG_ISP_PREYEE_T);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_Y, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_U, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_AI_ISP_RDMA_V, REG_ISP_DMA_CTL_T);

	BLK_INFO(m_block, ISPTOP, REG_ISP_TOP_T);
	BLK_INFO(m_block, RDMA_CORE0, REG_RDMA_CORE_T);
	BLK_INFO(m_block, RDMA_CORE1, REG_RDMA_CORE_T);
	BLK_INFO(m_block, CSIBDG0_LITE, REG_ISP_CSI_BDG_LITE_T);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE2, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT0_LITE3, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, CSIBDG1_LITE, REG_ISP_CSI_BDG_LITE_T);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE0, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE1, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE2, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_BT1_LITE3, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, PRE_RAW_VI_SEL, REG_PRE_RAW_VI_SEL_T);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_LE, REG_ISP_DMA_CTL_T);
	BLK_INFO(m_block, DMA_CTL_PRE_RAW_VI_SEL_SE, REG_ISP_DMA_CTL_T);
	// BLK_INFO(m_block, CMDQ, );

	init_done = true;

DUMP:

#define BLK_DUMP(_para, _blk)\
	do {\
		int i = 0;\
		for (i = 0; i < _para[_blk].blk_size; i += 4) {\
			if (m == NULL) {\
				if (i == 0) {\
					vi_pr(VI_ERR, "%22X, %10X, %10X, %10X\n", 0x00, 0x04, 0x08, 0x0C);\
				} \
				vi_pr(VI_ERR, "0x%llx: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",\
					(_para[_blk].phy_base + i * 0x4),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 0) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 1) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 2) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 3) * 0x4)));\
			} else {\
				if (i == 0) {\
					seq_printf(m, "%22X, %10X, %10X, %10X\n", 0x00, 0x04, 0x08, 0x0C);\
				} \
				seq_printf(m, "0x%llx: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",\
					(_para[_blk].phy_base + i * 0x4),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 0) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 1) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 2) * 0x4)),\
					ISP_RD_REG_BA((_para[_blk].reg_base + (i + 3) * 0x4)));\
			} \
		} \
	} while (0)

	if (blk >= 0 && blk < ISP_BLK_ID_MAX)
		BLK_DUMP(m_block, blk);
}

static inline void _vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vi_dev *vdev = m->private;
	struct isp_ctx *ctx = &vdev->ctx;
	enum cvi_isp_raw raw_num = ISP_PRERAW0;
	enum cvi_isp_fe_chn_num chn_num = ISP_FE_CH0;
	struct timespec64 ts1, ts2;
	u32 sofCnt1[ISP_PRERAW_MAX], sofCnt2[ISP_PRERAW_MAX];
	u32 frmCnt1[ISP_PRERAW_MAX], frmCnt2[ISP_PRERAW_MAX];
	u64 t2 = 0, t1 = 0;

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!vdev->ctx.isp_pipe_enable[raw_num])
			continue;
		if (_is_right_tile(ctx, raw_num))
			continue;
		sofCnt1[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
		if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) //YUV sensor
			frmCnt1[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
		else //RGB sensor
			frmCnt1[raw_num] = vdev->postraw_frame_number[raw_num];
	}

	ktime_get_real_ts64(&ts1);
	t1 = ts1.tv_sec * 1000000 + ts1.tv_nsec / 1000;

	msleep(940);
	do {
		for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
			if (!vdev->ctx.isp_pipe_enable[raw_num])
				continue;
			if (_is_right_tile(ctx, raw_num))
				continue;
			sofCnt2[raw_num] = vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0];
			if (vdev->ctx.isp_pipe_cfg[raw_num].is_yuv_sensor) //YUV sensor
				frmCnt2[raw_num] = vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0];
			else //RGB sensor
				frmCnt2[raw_num] = vdev->postraw_frame_number[raw_num];
		}

		ktime_get_real_ts64(&ts2);
		t2 = ts2.tv_sec * 1000000 + ts2.tv_nsec / 1000;
	} while ((t2 - t1) < 1000000);

	for (raw_num = ISP_PRERAW0; raw_num < ISP_PRERAW_MAX; raw_num++) {
		if (!ctx->isp_pipe_enable[raw_num])
			continue;

		if (raw_num == vi_get_first_raw_num(ctx)) {
			seq_puts(m, "[VI BE_Dbg_Info]\n");
			seq_printf(m, "VIPreBEDoneSts\t\t:0x%x\t\tVIPreBEDmaIdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_done_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.be_sts.be_dma_idle_sts);
			seq_puts(m, "[VI Post_Dbg_Info]\n");
			seq_printf(m, "VIIspTopStatus0\t\t:0x%x\t\tVIIspTopStatus1\t\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.post_sts.top_sts_0,
					ctx->isp_pipe_cfg[raw_num].dg_info.post_sts.top_sts_1);
			seq_puts(m, "[VI DMA_Dbg_Info]\n");
			seq_printf(m, "VIWdma0ErrStatus\t:0x%x\tVIWdma0IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_0_idle);
			seq_printf(m, "VIWdma1ErrStatus\t:0x%x\tVIWdma1IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_1_idle);
			seq_printf(m, "VIWdma2ErrStatus\t:0x%x\tVIWdma2IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_2_idle);
			seq_printf(m, "VIWdma3ErrStatus\t:0x%x\tVIWdma3IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.wdma_3_idle);
			seq_printf(m, "VIRdma0ErrStatus\t:0x%x\tVIRdma0IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_0_idle);
			seq_printf(m, "VIRdma1ErrStatus\t:0x%x\tVIRdma1IdleStatus\t:0x%x\n",
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_err_sts,
					ctx->isp_pipe_cfg[raw_num].dg_info.dma_sts.rdma_1_idle);
		}

		seq_printf(m, "[VI ISP_PIPE_%d FE_Dbg_Info]\n", raw_num);
		seq_printf(m, "VIPreFERawDbgSts\t:0x%x\t\tVIPreFEDbgInfo\t\t:0x%x\n",
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_idle_sts,
				ctx->isp_pipe_cfg[raw_num].dg_info.fe_sts.fe_done_sts);

		if (ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "[VI ISP_PIPE_%d %s]\n", raw_num,
				(!ctx->isp_pipe_cfg[raw_num].is_work_on_r_tile) ? "Left_Tile" : "Right_Tile");
		} else {
			seq_printf(m, "[VI ISP_PIPE_%d]\n", raw_num);
		}
		seq_printf(m, "VIOutImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_w);
		seq_printf(m, "VIOutImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].post_img_h);
		seq_printf(m, "VIInImgWidth\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_width);
		seq_printf(m, "VIInImgHeight\t\t:%4d\n", ctx->isp_pipe_cfg[raw_num].csibdg_height);

		seq_printf(m, "VIDevFPS\t\t:%4d\n", sofCnt2[raw_num] - sofCnt1[raw_num]);
		seq_printf(m, "VIFPS\t\t\t:%4d\n", frmCnt2[raw_num] - frmCnt1[raw_num]);

		seq_printf(m, "VISofCh0Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
		    ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_1Multiplex)
			seq_printf(m, "VISofCh1Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH1]);
		if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_2Multiplex)
			seq_printf(m, "VISofCh2Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH2]);
		if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_3Multiplex)
			seq_printf(m, "VISofCh3Cnt\t\t:%4d\n", vdev->pre_fe_sof_cnt[raw_num][ISP_FE_CH3]);

		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
		    ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "VISpltWdmaCh0Cnt\t:%4d\n", vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH0]);
			seq_printf(m, "VISpltRdmaCh0Cnt\t:%4d\n", vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH0]);
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				seq_printf(m, "VISpltWdmaCh1Cnt\t:%4d\n", vdev->splt_wdma_frm_num[raw_num][ISP_FE_CH1]);
				seq_printf(m, "VISpltRdmaCh1Cnt\t:%4d\n", vdev->splt_rdma_frm_num[raw_num][ISP_FE_CH1]);
			}
		}

		seq_printf(m, "VIPreFECh0Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on ||
		    ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_1Multiplex)
			seq_printf(m, "VIPreFECh1Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH1]);
		if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_2Multiplex)
			seq_printf(m, "VIPreFECh2Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH2]);
		if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_3Multiplex)
			seq_printf(m, "VIPreFECh3Cnt\t\t:%4d\n", vdev->pre_fe_frm_num[raw_num][ISP_FE_CH3]);

		seq_printf(m, "VIPreBECh0Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH0]);
		if (ctx->isp_pipe_cfg[raw_num].is_hdr_on)
			seq_printf(m, "VIPreBECh1Cnt\t\t:%4d\n", vdev->pre_be_frm_num[raw_num][ISP_BE_CH1]);

		seq_printf(m, "VIPostCnt\t\t:%4d\n", vdev->postraw_frame_number[raw_num]);
		seq_printf(m, "VIDropCnt\t\t:%4d\n", vdev->drop_frame_number[raw_num]);
		seq_printf(m, "VIDumpCnt\t\t:%4d\n", vdev->dump_frame_number[raw_num]);

		seq_printf(m, "[VI ISP_PIPE_%d Csi_Dbg_Info]\n", raw_num);
		seq_printf(m, "VICsiIntStatus0\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_0);
		seq_printf(m, "VICsiIntStatus1\t\t:0x%x\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_int_sts_1);
		seq_printf(m, "VICsiOverFlowCnt\t:%4d\n", ctx->isp_pipe_cfg[raw_num].dg_info.bdg_fifo_of_cnt);

		for (chn_num = ISP_FE_CH0; chn_num < ISP_FE_CHN_MAX; chn_num++) {
			if (chn_num == ISP_FE_CH1) {
				if (!ctx->isp_pipe_cfg[raw_num].is_hdr_on &&
				    ctx->isp_pipe_cfg[raw_num].muxMode < VI_WORK_MODE_2Multiplex)
					break;
			} else if (chn_num == ISP_FE_CH2) {
				if (ctx->isp_pipe_cfg[raw_num].muxMode < VI_WORK_MODE_3Multiplex)
					break;
			} else if (chn_num == ISP_FE_CH3) {
				if (ctx->isp_pipe_cfg[raw_num].muxMode < VI_WORK_MODE_4Multiplex)
					break;
			}

			seq_printf(m, "VICsiCh%dDbg0\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_0);
			seq_printf(m, "VICsiCh%dDbg1\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_1);
			seq_printf(m, "VICsiCh%dDbg2\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_2);
			seq_printf(m, "VICsiCh%dDbg3\t\t:0x%x\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_chn_debug[chn_num].dbg_3);

			seq_printf(m, "VICsiCh%dWidthGTCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_gt_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dWidthLSCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_w_ls_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dHeightGTCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_gt_cnt[chn_num]);
			seq_printf(m, "VICsiCh%dHeightLSCnt\t:%4d\n", chn_num,
					ctx->isp_pipe_cfg[raw_num].dg_info.bdg_h_ls_cnt[chn_num]);
		}

		seq_printf(m, "[VI ISP_PIPE_%d Buf_Dbg_Info]\n", raw_num);
		if (ctx->isp_pipe_cfg[raw_num].is_raw_replay_fe ||
		    ctx->isp_pipe_cfg[raw_num].is_tile) {
			seq_printf(m, "VISpltCh0OutBufEmpty\t:%4d\n",
					isp_buf_empty(&splt_out_q[raw_num][ISP_SPLT_CHN0]));
			seq_printf(m, "VIPreFECh0InBufEmpty\t:%4d\n",
					isp_buf_empty(&pre_fe_in_q[raw_num][ISP_SPLT_CHN1]));
			if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
				seq_printf(m, "VISpltCh1OutBufEmpty\t:%4d\n",
						isp_buf_empty(&splt_out_q[raw_num][ISP_SPLT_CHN0]));
				seq_printf(m, "VIPreFECh1InBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_fe_in_q[raw_num][ISP_SPLT_CHN1]));
			}
		}
		if (ctx->isp_pipe_cfg[raw_num].is_yuv_sensor &&
		    ctx->isp_pipe_cfg[raw_num].yuv_scene_mode == ISP_YUV_SCENE_BYPASS) {
			seq_printf(m, "VIYuvCh0OutBufEmpty\t:%4d\n",
				cvi_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH0));
			if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_1Multiplex)
				seq_printf(m, "VIYuvCh1OutBufEmpty\t:%4d\n",
					cvi_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH1));
			if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_2Multiplex)
				seq_printf(m, "VIYuvCh2OutBufEmpty\t:%4d\n",
					cvi_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH2));
			if (ctx->isp_pipe_cfg[raw_num].muxMode > VI_WORK_MODE_3Multiplex)
				seq_printf(m, "VIYuvCh3OutBufEmpty\t:%4d\n",
					cvi_isp_rdy_buf_empty(vdev, vdev->ctx.raw_chnstr_num[raw_num] + ISP_FE_CH3));
		} else {
			if (_is_fe_be_online(ctx) && !ctx->is_slice_buf_on) { // fe->be->dram->post
				seq_printf(m, "VIPreBECh0OutBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_be_out_q[ISP_FE_CH0]));
				seq_printf(m, "VIPostCh0InBufEmpty\t:%4d\n",
						isp_buf_empty(&postraw_in_q[ISP_FE_CH0]));
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					seq_printf(m, "VIPreBECh1OutBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_be_out_q[ISP_FE_CH1]));
					seq_printf(m, "VIPostCh1InBufEmpty\t:%4d\n",
							isp_buf_empty(&postraw_in_q[ISP_FE_CH1]));
				}
			} else if (_is_be_post_online(ctx)) { // fe->dram->be->post
				seq_printf(m, "VIPreFECh0OutBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_fe_out_q[raw_num][ISP_FE_CH0]));
				seq_printf(m, "VIPreBECh0InBufEmpty\t:%4d\n",
						isp_buf_empty(&pre_be_in_q));
				if (ctx->isp_pipe_cfg[raw_num].is_hdr_on) {
					seq_printf(m, "VIPreFECh1OutBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_fe_out_q[raw_num][ISP_FE_CH1]));
					seq_printf(m, "VIPreBECh1InBufEmpty\t:%4d\n",
							isp_buf_empty(&pre_be_in_se_q[raw_num]));
				}
			}
			if (ctx->isp_pipe_cfg[raw_num].is_offline_scaler) {
				seq_printf(m, "VIPostOutBufEmpty\t:%4d\n", cvi_isp_rdy_buf_empty(vdev, raw_num));
			}
		}
	}
}

static int vi_dbg_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vi_dev *vdev = m->private;

	if (proc_isp_mode == 255)
		_vi_dbg_proc_show(m, v);
	else
		_vi_dbg_reg_dump(&vdev->ctx, m, proc_isp_mode);

	return 0;
}

static ssize_t vi_dbg_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_isp_mode))
		proc_isp_mode = 255;

	return count;
}

static int vi_dbg_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vi_dbg_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops vi_dbg_proc_fops = {
	.proc_open = vi_dbg_proc_open,
	.proc_read = seq_read,
	.proc_write = vi_dbg_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vi_dbg_proc_fops = {
	.owner = THIS_MODULE,
	.open = vi_dbg_proc_open,
	.read = seq_read,
	.write = vi_dbg_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_dbg_proc_init(struct cvi_vi_dev *_vdev)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_DBG_PROC_NAME, 0644, NULL, &vi_dbg_proc_fops, _vdev) == NULL) {
		pr_err("vi_dbg proc creation failed\n");
		rc = -EAGAIN;
	}

	return rc;
}

int vi_dbg_proc_remove(void)
{
	remove_proc_entry(VI_DBG_PROC_NAME, NULL);

	return 0;
}
