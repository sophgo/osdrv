#ifndef __VI_DRV_H__
#define __VI_DRV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_types.h"

#include "isp/vi_isp.h"
#include "vi_ip_comm.h"

enum ISP_DMA_ID {
	// wdma1 dma_group=1
	ISP_DMA_ID_CSI0_BDG0 = 0,
	ISP_DMA_ID_CSI0_BDG1,
	ISP_DMA_ID_CSI0_BDG2,
	ISP_DMA_ID_CSI0_BDG3,
	ISP_DMA_ID_CSI1_BDG0,
	ISP_DMA_ID_CSI1_BDG1,
	ISP_DMA_ID_BT0_LITE0,
	ISP_DMA_ID_BT0_LITE1,
	ISP_DMA_ID_BT0_LITE2,
	ISP_DMA_ID_BT0_LITE3,
	ISP_DMA_ID_AE_FE0_HIST_LE,
	ISP_DMA_ID_AE_FE0_HIST_SE,
	ISP_DMA_ID_AE_FE1_HIST_LE,
	ISP_DMA_ID_AE_FE1_HIST_SE,
	ISP_DMA_ID_AE_FE2_HIST_LE,
	// wdma2 dma_group=2
	ISP_DMA_ID_AF_W = 0,
	ISP_DMA_ID_DRC_POLY_W,
	ISP_DMA_ID_DRC_HIST,
	ISP_DMA_ID_LSCR_HIST,
	// wdma3 dma_group=3
	ISP_DMA_ID_CNR_Y_W = 0,
	ISP_DMA_ID_CNR_C_W,
	ISP_DMA_ID_LDCI_HIST_W,
	ISP_DMA_ID_LDCI_LMAP_W,
	ISP_DMA_ID_YUV_CROP_Y,
	ISP_DMA_ID_YUV_CROP_C,
	ISP_DMA_ID_TNR_ST_Y,
	ISP_DMA_ID_TNR_ST_C,
	ISP_DMA_ID_TNR_ST_MV,
	ISP_DMA_ID_TNR_ST_MO,
	ISP_DMA_ID_TNR_ST_FCB,
	ISP_DMA_ID_TNR_ST_MSP,
	ISP_DMA_ID_2D_W,
	ISP_DMA_ID_RESIZE_W,

	// rdma1 dma_group=7
	ISP_DMA_ID_PRE_RAW_VI_SEL_LE = 0,
	ISP_DMA_ID_PRE_RAW_VI_SEL_SE = 4,
	ISP_DMA_ID_FE0_LSC_RDMA_CTL = 8,
	ISP_DMA_ID_FE1_LSC_RDMA_CTL,
	ISP_DMA_ID_FE2_LSC_RDMA_CTL,

	// rdma2 dma_group=6
	ISP_DMA_ID_DRC_POLY_R = 0,

	// rdma3 dma_group=5
	ISP_DMA_ID_CNR_Y_R = 0,
	ISP_DMA_ID_CNR_C_R,
	ISP_DMA_ID_LDCI_LMAP_R,
	ISP_DMA_ID_YUV_RDMA_Y,
	ISP_DMA_ID_YUV_RDMA_C,
	ISP_DMA_ID_TNR_LD_Y,
	ISP_DMA_ID_TNR_LD_C,
	ISP_DMA_ID_TNR_LD_MV,
	ISP_DMA_ID_TNR_LD_MO,
	ISP_DMA_ID_TNR_LD_FCB,
	ISP_DMA_ID_2D_R,
	ISP_DMA_ID_CLUT_R,
	ISP_DMA_ID_MAX,
};

/****************************************************************************
 * Interfaces
 ****************************************************************************/
uint8_t vi_get_pipe_by_raw_chn(struct isp_ctx *ctx, u8 raw_num, enum sop_isp_fe_chn_num chn);
int vi_get_raw_num_by_dev(struct isp_ctx *ctx, u8 dev_num);
int vi_get_first_raw_num(struct isp_ctx *ctx);

void vi_set_base_addr(void *base);
uintptr_t *isp_get_phys_reg_bases(void);

/**
 * isp_reset - do reset. This can be activated only if dma stop to avoid
 * hang fabric.
 *
 */
void isp_reset(struct isp_ctx *ctx);

/**
 * isp_stream_on - start/stop isp stream.
 *
 * @param on: 1 for stream start, 0 for stream stop
 */
void isp_streaming(struct isp_ctx *ctx, u32 on, enum sop_isp_raw raw_num);
void ispblk_fbc_debug_info(struct isp_ctx *ctx);
void ispblk_fbc_clear_fbcd_ring_base(struct isp_ctx *ctx);
void ispblk_fbc_chg_to_sw_mode(struct isp_ctx *ctx);
void ispblk_fbc_ring_buf_config(struct isp_ctx *ctx, u8 en);
void vi_fbc_calculate_size(struct isp_ctx *ctx, u8 raw_num);
void ispblk_fe_sof_interrupt_ctrl(struct isp_ctx *ctx, bool en);
void ispblk_overflow_record_reg_info(struct isp_ctx *ctx, void *reg_info);

void ispblk_crop_enable(struct isp_ctx *ctx, int crop_id, bool en);
int ispblk_crop_config(struct isp_ctx *ctx, int crop_id, struct vi_rect crop);

int bayer_type_mapping(enum isp_bayer_type_e bayer_type);
enum isp_bayer_type_e bayer_type_before_crop(enum isp_bayer_type_e current_bayer_type, u16 crop_x, u16 crop_y);
int csibdg_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num);
int ae_dma_find_hwid(enum sop_isp_raw raw_num, enum sop_isp_fe_chn_num chn_num);
int clsc_dma_find_hwid(enum sop_isp_raw raw_num);

u64 ispblk_dma_getaddr(struct isp_ctx *ctx, u32 dmaid);
int ispblk_dma_config(struct isp_ctx *ctx, const uint8_t pipe, int dmaid, u64 buf_addr);
void ispblk_dma_setaddr(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr);
void ispblk_dma_enable(struct isp_ctx *ctx, u32 dmaid, u32 on, uint8_t dma_disable);
int ispblk_dma_buf_get_size(struct isp_ctx *ctx, const uint8_t pipe, int dmaid);
void ispblk_dma_set_sw_mode(struct isp_ctx *ctx, u32 dmaid, bool is_sw_mode);
bool ispblk_dma_get_sw_mode(struct isp_ctx *ctx, u32 dmaid);

void ispblk_mctf_mmap_status(struct isp_ctx *ctx, uint32_t *still, uint32_t *trans, uint32_t *motion);
void ispblk_mctf_resize_config(struct isp_ctx *ctx, uint8_t pipe, u64 phy_addr, bool en);

void isp_pre_trig(struct isp_ctx *ctx, enum sop_isp_raw raw_num, const u8 chn_num);
void isp_post_trig(struct isp_ctx *ctx);

void isp_intr_set_mask(struct isp_ctx *ctx);
void isp_intr_status(
	struct isp_ctx *ctx,
	union reg_isp_top_int_event0 *s0,
	union reg_isp_top_int_event1 *s1,
	union reg_isp_top_int_event2 *s2);
void isp_csi_intr_status(
	struct isp_ctx *ctx,
	enum sop_isp_raw raw_num,
	union reg_isp_csi_bdg_interrupt_status_0 *s0,
	union reg_isp_csi_bdg_interrupt_status_1 *s1);

/****************************************************************************
 *	Runtime Control Flow Config
 ****************************************************************************/
void isp_first_frm_reset(struct isp_ctx *ctx, u8 reset, bool is_drop_frm);
void ispblk_post_cfg_update(struct isp_ctx *ctx);
void ispblk_post_yuv_cfg_update(struct isp_ctx *ctx, const u8 pipe);
void ispblk_post_cfg_update(struct isp_ctx *ctx);
int ispblk_dma_get_size(struct isp_ctx *ctx, int dmaid, u32 _w, u32 _h);
int isp_frm_err_handler(struct isp_ctx *ctx, const enum sop_isp_raw err_raw_num, const u8 step);

u32 ispblk_dma_yuv_bypass_config(struct isp_ctx *ctx, u32 dmaid, u64 buf_addr,
				 const enum sop_isp_raw raw_num);

/****************************************************************************
 *	Slice buffer Control
 ****************************************************************************/
void vi_calculate_slice_buf_setting(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void isp_slice_buf_trig(struct isp_ctx *ctx);
void ispblk_slice_buf_config(struct isp_ctx *ctx, const enum sop_isp_raw raw_num, u8 en);

#ifdef __cplusplus
}
#endif

#endif /* __VI_DRV_H__ */
