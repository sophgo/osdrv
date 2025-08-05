#ifndef VI_DMA_SETUP_H
#define VI_DMA_SETUP_H

#include "vi_ip_comm.h"
#include "vi_defines.h"

void _vi_mempool_reset(struct _mempool *isp_mempool);
void _vi_dma_set_sw_mode(struct isp_ctx *ctx, enum sop_isp_raw raw_num, bool is_csi);
void _vi_yuv_get_dma_size(struct isp_ctx *ctx, const enum sop_isp_raw raw_num);
void _vi_pre_fe_get_dma_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num);
void _vi_rawtop_get_dma_size(struct isp_ctx *ctx, int pipe);
void _vi_rgbtop_get_dma_size(struct isp_ctx *ctx, int pipe);
void _vi_yuvtop_get_dma_size(struct isp_ctx *ctx, int pipe);
void _vi_cmdq_get_dma_size(struct isp_ctx *ctx, int pipe);
void _vi_get_raw_dma_buf_size(struct isp_ctx *ctx);
void _vi_get_pipe_dma_buf_size(struct isp_ctx *ctx, const uint8_t pipe);
void _vi_get_dma_buf_size(struct isp_ctx *ctx);
int vi_get_isp_ion_buf(struct sop_vi_dev *vdev, const uint8_t pipe);
int vi_get_csi_ion_buf(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num);
int vi_free_ion_buf(struct sop_vi_dev *vdev, struct _mempool *mempool);
void _isp_pre_fe_dma_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num);
void _isp_pre_ai_isp_dma_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num);
void _vi_dma_setup(struct sop_vi_dev *vdev, uint8_t pipe);

#endif // VI_DMA_SETUP_H