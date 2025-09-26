#include "vi_fe_ip_ctrl.h"
#include "vi_dma_setup.h"
#include "ion.h"

#define VI_CMDQ_BUF_SIZE	(0x20000)

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define CACL_DMA_SIZE(mempool, raw_num, id)				\
	do {								\
		bufsize = ispblk_dma_buf_get_size(ctx, raw_num, id);	\
		_mempool_pop(mempool, bufsize);				\
	} while (0)

#define CACL_AND_DMA_SETUP(mempool, raw_num, id)			\
	do {								\
		bufaddr = _mempool_get_addr(mempool);			\
		bufsize = ispblk_dma_config(ctx, raw_num, id, bufaddr);	\
		_mempool_pop(mempool, bufsize);				\
	} while (0)

/**
 * _mempool_reset - reset the byteused and assigned buffer for each dma
 *
 */
void _vi_mempool_reset(struct _mempool *isp_mempool)
{
	isp_mempool->byteused = 0;
}

/**
 * _mempool_get_addr - get mempool's latest address.
 *
 * @return: the latest address of the mempool.
 */
static u64 _mempool_get_addr(struct _mempool *isp_mempool)
{
	return isp_mempool->base + isp_mempool->byteused;
}

/**
 * _mempool_pop - acquire a buffer-space from mempool.
 *
 * @param size: the space acquired.
 * @return: negative if no enough space; o/w, the address of the buffer needed.
 */
static int64_t _mempool_pop(struct _mempool *isp_mempool, u32 size)
{
	int64_t addr;

	size = VI_ALIGN(size);

	if ((isp_mempool->byteused + size) > isp_mempool->size) {
		vi_pr(VI_ERR, "reserved_memory(0x%x) is not enough. byteused(0x%x) alloc_size(0x%x)\n",
				isp_mempool->size, isp_mempool->byteused, size);
		return ERR_VI_INVALID_PARA;
	}

	addr = isp_mempool->base + isp_mempool->byteused;
	isp_mempool->byteused += size;

	return addr;
}

void _vi_yuv_get_dma_size(struct isp_ctx *ctx, const enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u32 base = 0, dma = 0;
	u8 i = 0, j = 0;
	u8 chn_str = ctx->raw_chnstr_num[raw_num];
	u8 total_chn = chn_str + ctx->isp_csi_cfg[raw_num].mux_mode + 1;

	base = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);

	for (i = chn_str; i < total_chn; i++) {
		dma = base + i - chn_str;

		for (j = 0; j < OFFLINE_YUV_BUF_NUM; j++) {
			bufsize = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			vi_pr(VI_INFO, "YUV path dma_id = %d, buf_block = %d, mem byteused = 0x%x\n", dma, j, bufsize);
			_mempool_pop(&ctx->csi_mempool[raw_num], bufsize);
		}
	}
}

void _vi_pre_fe_get_dma_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u8  i = 0;
	u8  pre_fe_buf_num = OFFLINE_RAW_BUF_NUM;
	u32 raw_le, raw_se;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];

	raw_le = csibdg_dma_find_hwid(phy_raw, ISP_FE_CH0);
	raw_se = csibdg_dma_find_hwid(phy_raw, ISP_FE_CH1);

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
		ctx->isp_csi_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_BYPASS) { //YUV sensor
		_vi_yuv_get_dma_size(ctx, raw_num);
		return;
	}

	if (_is_fe_post_offline(ctx)) { //fe->dram->post
		if (raw_num < ISP_PRERAW_VIRT0) {
			for (i = 0; i < pre_fe_buf_num; i++) {
				CACL_DMA_SIZE(mempool, raw_num, raw_le);
				if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
					CACL_DMA_SIZE(mempool, raw_num, raw_se);
				}
			}
		}
	} else if (_is_fe_post_slice(ctx)) {
		CACL_DMA_SIZE(mempool, raw_num, raw_le);
		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			CACL_DMA_SIZE(mempool, raw_num, raw_se);
		}
	}

	//AE
	bufsize = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_FE0_AE_HIST_LE);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		_mempool_pop(mempool, bufsize);
		_mempool_pop(mempool, bufsize);
	}

	//CLSC
	bufsize = ispblk_dma_buf_get_size(ctx, raw_num, ISP_BLK_ID_DMA_CTL_FE0_CLSC_LE);
	_mempool_pop(mempool, bufsize);
}

void _vi_pre_ai_isp_get_dma_size(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u32 bufsize = 0;
	u8  i = 0;
	u32 fe_dma_id;
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
					    ? ISP_FE_CH1 : ISP_FE_CH0;
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];

	if (!ctx->isp_csi_cfg[raw_num].is_ai_isp)
		return;

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor)
		return;

	for (; fe_chn <= fe_max; fe_chn++) {
		fe_dma_id = csibdg_dma_find_hwid(raw_num, fe_chn);
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
			CACL_DMA_SIZE(mempool, raw_num, fe_dma_id);
		}
	}
}

void _vi_rawtop_get_dma_size(struct isp_ctx *ctx, int pipe)
{
	u32 bufsize = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//af
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_AF_W);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);

	//lsc_hist
	_mempool_pop(mempool, bufsize);
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_LSCR_HIST);
	_mempool_pop(mempool, bufsize);

	//drc_poly
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_DRC_POLY_R);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);

	//drc_hist
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_DRC_HIST);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);
}

void _vi_rgbtop_get_dma_size(struct isp_ctx *ctx, int pipe)
{
	u32 bufsize = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//clut r
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_CLUT_R);
	_mempool_pop(mempool, bufsize);
}

static void _vi_mctf_buf_get_size(struct isp_ctx *ctx, int pipe)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u8 i = 0;
	u64 buf_unalign = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	if (!ctx->is_3dnr_on)
		return;

	if (ctx->is_fbc_on) {
		buf_unalign = _mempool_get_addr(mempool);

		//ring buffer constraint. reg_base is 256 byte-align
		bufaddr = VI_256_ALIGN(buf_unalign);
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_Y);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));

		buf_unalign = _mempool_get_addr(mempool);
		//ring buffer constraint. reg_base is 256 byte-align
		bufaddr = VI_256_ALIGN(buf_unalign);
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_C);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));

	} else {
		//TNR_ST_Y
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_Y);
		_mempool_pop(mempool, bufsize);

		//TNR_ST_C
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_C);
		_mempool_pop(mempool, bufsize);
	}

	//TNR_ST_MV
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MV);
	_mempool_pop(mempool, bufsize);

	//TNR_ST_MO
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
	_mempool_pop(mempool, bufsize);

	//TNR_ST_FCB
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_FCB);
	_mempool_pop(mempool, bufsize);

	for (i = 0; i < MSP_BUF_MAX; i++) {
		buf_unalign = _mempool_get_addr(mempool);
		bufaddr = VI_1K_ALIGN(buf_unalign);

		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MSP);
		//master chn msp
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));

		buf_unalign = _mempool_get_addr(mempool);
		bufaddr = VI_1K_ALIGN(buf_unalign);
		//slave chn msp
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));
	}
}

void _vi_yuvtop_get_dma_size(struct isp_ctx *ctx, int pipe)
{
	u32 bufsize = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//LDCI hist lmap
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_LDCI_HIST);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);

	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_LDCI_W);
	_mempool_pop(mempool, bufsize);
	_mempool_pop(mempool, bufsize);

	//mctf y_st/c_st/mv_st/momp_st/fcb_st/msp_st
	_vi_mctf_buf_get_size(ctx, pipe);

	//cnr y
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_Y_W);
	_mempool_pop(mempool, bufsize);
	//cnr c
	bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_CNR_C_W);
	_mempool_pop(mempool, bufsize);
}

void _vi_cmdq_get_dma_size(struct isp_ctx *ctx, int pipe)
{
	u32 bufsize = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	bufsize += VI_CMDQ_BUF_SIZE;

	_mempool_pop(mempool, bufsize);
}

void _vi_get_pipe_dma_buf_size(struct isp_ctx *ctx, const uint8_t pipe)
{
	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor &&
		ctx->isp_pipe_cfg[pipe].yuv_scene_mode != ISP_YUV_SCENE_ISP)
		return;

	_vi_rawtop_get_dma_size(ctx, pipe);
	_vi_rgbtop_get_dma_size(ctx, pipe);
	_vi_yuvtop_get_dma_size(ctx, pipe);
	_vi_cmdq_get_dma_size(ctx, pipe);
}

void _vi_yuv_dma_setup(struct sop_vi_dev *vdev, const enum sop_isp_raw raw_num)
{
	struct isp_ctx *ctx = &vdev->ctx;
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];
	struct _membuf *pool = &ctx->csi_bufpool[raw_num];
	struct isp_buffer *b;
	// u64 bufaddr = 0;
	u32 bufsize = 0;
	u32 base = 0, dma = 0;
	u8 i = 0, j = 0;
	u8 total_chn = ctx->isp_csi_cfg[raw_num].mux_mode + 1;

	base = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);

	for (i = 0; i < total_chn; i++) {
		dma = base + i;

		for (j = 0; j < OFFLINE_YUV_BUF_NUM; j++) {
			b = osal_vmalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "yuv_buf isp_buf_%d vmalloc size(%zu) fail\n", j, sizeof(*b));
				return;
			}
			osal_memset(b, 0, sizeof(*b));
			b->raw_num = raw_num;
			b->chn_num = i;
			b->is_ext = INTERNAL_BUFFER;
			b->source = PRE_FE;
			bufsize = ispblk_dma_yuv_bypass_config(ctx, dma, 0, raw_num);
			pool->yuv_yuyv[i][j] = b->addr = _mempool_pop(mempool, bufsize);

			if (j == 0)
				ispblk_dma_setaddr(ctx, dma, b->addr);

			isp_buf_queue(&vdev->pre_fe_out_q[raw_num][i], b);
		}
	}
}

static void _isp_preraw_fe_dma_dump(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u8 i = 0;
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];
	struct _membuf *csi_pool = &ctx->csi_bufpool[raw_num];

	vi_pr(VI_INFO, "***************PRERAW_FE_%d************************\n", raw_num);
	for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
		vi_pr(VI_INFO, "pre_fe_le(0x%llx)\n", csi_pool->pre_fe[ISP_FE_CH0][i]);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
			vi_pr(VI_INFO, "pre_fe_se(0x%llx)\n", csi_pool->pre_fe[ISP_FE_CH1][i]);
	}

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
		ctx->isp_csi_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_BYPASS) {
		for (i = 0; i < ISP_FE_CHN_MAX; i++) {
			vi_pr(VI_INFO, "yuyv_yuv(0x%llx), yuyv_yuv(0x%llx)\n",
				csi_pool->yuv_yuyv[i][0], csi_pool->yuv_yuyv[i][1]);
		}
	}

	vi_pr(VI_INFO, "ae_le(0x%llx, 0x%llx)\n", csi_pool->sts_mem[0].ae_le.phy_addr,
							csi_pool->sts_mem[1].ae_le.phy_addr);
	vi_pr(VI_INFO, "ae_se(0x%llx, 0x%llx)\n", csi_pool->sts_mem[0].ae_se.phy_addr,
							csi_pool->sts_mem[1].ae_se.phy_addr);
	vi_pr(VI_INFO, "clsc(0x%llx)\n", csi_pool->clsc);

	vi_pr(VI_INFO, "***************PRERAW_FE_%d************************\n", raw_num);

	vi_pr(VI_INFO, "*************************************************\n");
	vi_pr(VI_INFO, "VI total reserved memory(0x%x)\n", mempool->byteused);
	vi_pr(VI_INFO, "*************************************************\n");
}

static void _vi_rgb_dma_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u8 i = 0;
	u32 fe_dma_id;
	struct isp_buffer *b;
	struct isp_ctx *ctx = &vdev->ctx;
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];
	struct _membuf *pool = &ctx->csi_bufpool[raw_num];
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
					    ? ISP_FE_CH1 : ISP_FE_CH0;

	for (; fe_chn <= fe_max; fe_chn++) {
		fe_dma_id = csibdg_dma_find_hwid(raw_num, fe_chn);
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {

			bufaddr = _mempool_get_addr(mempool);
			bufsize = ispblk_dma_buf_get_size(ctx, raw_num, fe_dma_id);
			_mempool_pop(mempool, bufsize);

			b = osal_vzalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "raw_le isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
				return;
			}
			b->addr = bufaddr;
			b->raw_num = raw_num;
			b->chn_num = fe_chn;
			b->ir_idx = i;
			b->is_ext = INTERNAL_BUFFER;
			b->source = PRE_FE;
			b->size = bufsize;
			pool->pre_fe[fe_chn][i] = b->addr;
			isp_buf_queue(&vdev->pre_fe_out_q[raw_num][fe_chn], b);

			//default use idx 0 buffer
			if (i != 0)
				continue;
			if (ctx->isp_csi_cfg[raw_num].is_ai_isp) {
				//dmaid is bggr, but tpu need rggb, so swap br
				ispblk_dma_config(ctx, raw_num, fe_dma_id + 3, bufaddr);
				ispblk_dma_config(ctx, raw_num, fe_dma_id + 2, bufaddr + bufsize / 4);
				ispblk_dma_config(ctx, raw_num, fe_dma_id + 0, bufaddr + bufsize / 2);
				ispblk_dma_config(ctx, raw_num, fe_dma_id + 1, bufaddr + 3 * bufsize / 4);
			} else {
				ispblk_dma_config(ctx, raw_num, fe_dma_id, bufaddr);
			}
		}
	}
}

void _isp_pre_fe_dma_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u32 raw_le, raw_se, ae_le, ae_se, clsc;
	struct isp_ctx *ctx = &vdev->ctx;
	enum sop_isp_raw phy_raw = find_phy_raw_num(ctx, raw_num);
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];
	struct _membuf *bufpool = &ctx->csi_bufpool[raw_num];

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor &&
		ctx->isp_csi_cfg[raw_num].yuv_scene_mode != ISP_YUV_SCENE_BYPASS) { //YUV sensor
		_vi_yuv_dma_setup(vdev, raw_num);
		goto EXIT;
	}

	// mux dev use phy raw buffer
	if (_is_fe_post_offline(ctx) && raw_num < ISP_PRERAW_VIRT0) { //fe->dram->post
		_vi_rgb_dma_setup(vdev, raw_num);
	} else if (_is_fe_post_slice(ctx)) {
		raw_le = csibdg_dma_find_hwid(raw_num, ISP_FE_CH0);
		raw_se = csibdg_dma_find_hwid(raw_num, ISP_FE_CH1);

		CACL_AND_DMA_SETUP(mempool, raw_num, raw_le);
		bufpool->pre_fe[ISP_FE_CH0][BUF_IDX0] = bufaddr;
		ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL_PRE_VI_SEL_LE, bufaddr);
		if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
			CACL_AND_DMA_SETUP(mempool, raw_num, raw_se);
			bufpool->pre_fe[ISP_FE_CH1][BUF_IDX0] = bufaddr;
			ispblk_dma_setaddr(ctx, ISP_BLK_ID_DMA_CTL_PRE_VI_SEL_SE, bufaddr);
		}
	}

	ispblk_slice_buf_config(ctx, raw_num, _is_fe_post_slice(ctx));

	ae_le = ae_dma_find_hwid(phy_raw, ISP_FE_CH0);
	ae_se = ae_dma_find_hwid(phy_raw, ISP_FE_CH1);
	clsc = clsc_dma_find_hwid(phy_raw);

	//TODO also use csi raw_num not use pipe, for isp mw, find clsc or ae by pipe
	CACL_AND_DMA_SETUP(mempool, raw_num, ae_le);
	bufpool->sts_mem[BUF_IDX0].ae_le.phy_addr = bufaddr;
	bufpool->sts_mem[BUF_IDX0].ae_le.size = bufsize;
	bufpool->sts_mem[BUF_IDX1].ae_le.phy_addr = _mempool_pop(mempool, bufsize);
	bufpool->sts_mem[BUF_IDX1].ae_le.size = bufsize;

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		CACL_AND_DMA_SETUP(mempool, raw_num, ae_se);
		bufpool->sts_mem[BUF_IDX0].ae_se.phy_addr = bufaddr;
		bufpool->sts_mem[BUF_IDX0].ae_se.size = bufsize;
		bufpool->sts_mem[BUF_IDX1].ae_se.phy_addr = _mempool_pop(mempool, bufsize);
		bufpool->sts_mem[BUF_IDX1].ae_se.size = bufsize;
	}

	CACL_AND_DMA_SETUP(mempool, raw_num, clsc);
	bufpool->clsc = bufaddr;

	ispblk_clsc_dma_config(ctx, raw_num, bufpool->clsc);
EXIT:
	_isp_preraw_fe_dma_dump(ctx, raw_num);
}

static void _isp_pre_ai_isp_dma_dump(struct isp_ctx *ctx, enum sop_isp_raw raw_num)
{
	u8 i = 0;
	struct _membuf *csi_pool = &ctx->csi_bufpool[raw_num];

	vi_pr(VI_INFO, "***************PRERAW_AI_ISP_%d************************\n", raw_num);

	for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
		vi_pr(VI_INFO, "pre_ai_isp_le(0x%llx)\n", csi_pool->pre_ai_isp[ISP_FE_CH0][i]);

	if (ctx->isp_csi_cfg[raw_num].is_hdr_on) {
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++)
			vi_pr(VI_INFO, "pre_ai_isp_se(0x%llx)\n", csi_pool->pre_ai_isp[ISP_FE_CH1][i]);
	}

	vi_pr(VI_INFO, "************************************************\n");
}

void _isp_pre_ai_isp_dma_setup(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	u8 i = 0;
	u32 fe_dma_id;
	u64 bufaddr = 0;
	u32 bufsize = 0;
	struct isp_ctx *ctx = &vdev->ctx;
	struct isp_buffer *b = NULL;
	struct _mempool *mempool = &ctx->csi_mempool[raw_num];
	struct _membuf *pool = &ctx->csi_bufpool[raw_num];
	enum sop_isp_fe_chn_num fe_chn = ISP_FE_CH0;
	enum sop_isp_fe_chn_num fe_max = ctx->isp_csi_cfg[raw_num].is_hdr_on
					    ? ISP_FE_CH1 : ISP_FE_CH0;
	if (!ctx->isp_csi_cfg[raw_num].is_ai_isp)
		return;

	if (ctx->isp_csi_cfg[raw_num].is_yuv_sensor)
		return;

	for (; fe_chn <= fe_max; fe_chn++) {
		fe_dma_id = csibdg_dma_find_hwid(raw_num, fe_chn);
		for (i = 0; i < OFFLINE_RAW_BUF_NUM; i++) {
			CACL_AND_DMA_SETUP(mempool, raw_num, fe_dma_id);
			b = osal_vzalloc(sizeof(*b));
			if (b == NULL) {
				vi_pr(VI_ERR, "raw_le isp_buf_%d vmalloc size(%zu) fail\n", i, sizeof(*b));
				return;
			}
			b->addr = bufaddr;
			b->size = bufsize;
			b->raw_num = raw_num;
			b->chn_num = fe_chn;
			b->ir_idx = i;
			b->is_ext = INTERNAL_BUFFER;
			b->source = PRE_AI_ISP;
			pool->pre_ai_isp[fe_chn][i] = b->addr;
			isp_buf_queue(&vdev->pre_ai_isp_out_q[raw_num][fe_chn], b);
		}
	}

	_isp_pre_ai_isp_dma_dump(ctx, raw_num);
}

/*isptop dma caclulate*/

static void _isp_rawtop_dma_dump(struct isp_ctx *ctx, const u8 pipe)
{
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];

	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	vi_pr(VI_INFO, "***************RAW_TOP************************\n");

	vi_pr(VI_INFO, "af(0x%llx, 0x%llx)\n", bufpool->sts_mem[BUF_IDX0].af.phy_addr,
		bufpool->sts_mem[1].af.phy_addr);
	vi_pr(VI_INFO, "clsc(0x%llx, 0x%llx)\n", bufpool->sts_mem[BUF_IDX0].lsc_hist.phy_addr,
		bufpool->sts_mem[BUF_IDX1].lsc_hist.phy_addr);
	vi_pr(VI_INFO, "drc_poly(0x%llx, 0x%llx)\n", bufpool->drc_poly[BUF_IDX0],
		bufpool->drc_poly[BUF_IDX1]);
	vi_pr(VI_INFO, "drc_hist(0x%llx, 0x%llx)\n", bufpool->sts_mem[BUF_IDX0].drc_hist.phy_addr,
		bufpool->sts_mem[BUF_IDX1].drc_hist.phy_addr);

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_rgbtop_dma_dump(struct isp_ctx *ctx, const u8 pipe)
{
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];

	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	vi_pr(VI_INFO, "***************RGB_TOP************************\n");

	vi_pr(VI_INFO, "clut(0x%llx)\n", bufpool->clut);

	vi_pr(VI_INFO, "*************************************************\n");
}

static void _isp_yuvtop_dma_dump(struct isp_ctx *ctx, const u8 pipe)
{
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;

	vi_pr(VI_INFO, "***************YUV_TOP_%d************************\n", pipe);
	vi_pr(VI_INFO, "ldci_hist(0x%llx, 0x%llx)\n",
			bufpool->sts_mem[BUF_IDX0].ldci_hist.phy_addr,
			bufpool->sts_mem[BUF_IDX1].ldci_hist.phy_addr);
	vi_pr(VI_INFO, "ldci_lmap(0x%llx, 0x%llx)\n",
			bufpool->ldci_lmap[BUF_IDX0],
			bufpool->ldci_lmap[BUF_IDX1]);
	vi_pr(VI_INFO, "tnr_y(0x%llx, 0x%llx)\n",
			bufpool->tdnr[TNR_ST_Y][BUF_IDX0],
			bufpool->tdnr[TNR_ST_Y][BUF_IDX1]);
	vi_pr(VI_INFO, "tnr_c(0x%llx, 0x%llx)\n",
			bufpool->tdnr[TNR_ST_C][BUF_IDX0],
			bufpool->tdnr[TNR_ST_C][BUF_IDX1]);
	vi_pr(VI_INFO, "tnr_mv(0x%llx, 0x%llx)\n",
			bufpool->tdnr[TNR_ST_MV][BUF_IDX0],
			bufpool->tdnr[TNR_ST_MV][BUF_IDX1]);
	vi_pr(VI_INFO, "tnr_mo(0x%llx, 0x%llx)\n",
			bufpool->tdnr[TNR_ST_MO][BUF_IDX0],
			bufpool->tdnr[TNR_ST_MO][BUF_IDX1]);
	vi_pr(VI_INFO, "tnr_fcb(0x%llx, 0x%llx)\n",
			bufpool->tdnr[TNR_ST_FCB][BUF_IDX0],
			bufpool->tdnr[TNR_ST_FCB][BUF_IDX1]);
	vi_pr(VI_INFO, "msp_master(0x%llx, 0x%llx, 0x%llx)\n",
			bufpool->msp[MSP_MASTER_CHN][BUF_IDX0],
			bufpool->msp[MSP_MASTER_CHN][BUF_IDX1],
			bufpool->msp[MSP_MASTER_CHN][BUF_MAX]);
	vi_pr(VI_INFO, "msp_slave(0x%llx, 0x%llx, 0x%llx)\n",
			bufpool->msp[MSP_SLAVE_CHN][BUF_IDX0],
			bufpool->msp[MSP_SLAVE_CHN][BUF_IDX1],
			bufpool->msp[MSP_SLAVE_CHN][BUF_MAX]);
	vi_pr(VI_INFO, "cnr_y(0x%llx) cnr_c(0x%llx)\n",
			bufpool->cnr_y, bufpool->cnr_c);

	vi_pr(VI_INFO, "*************************************************\n");
	vi_pr(VI_INFO, "VI total reserved memory(0x%x)\n", mempool->byteused);
	vi_pr(VI_INFO, "*************************************************\n");
}

void _isp_rawtop_dma_setup(struct isp_ctx *ctx, const u8 pipe)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//af
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_AF_W);
	bufpool->sts_mem[0].af.phy_addr = bufaddr;
	bufpool->sts_mem[0].af.size = bufsize;
	bufpool->sts_mem[1].af.phy_addr = _mempool_pop(mempool, bufsize);
	bufpool->sts_mem[1].af.size = bufsize;

	//lsc_hist
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_LSCR_HIST);
	bufpool->sts_mem[0].lsc_hist.phy_addr = bufaddr;
	bufpool->sts_mem[0].lsc_hist.size = bufsize;
	bufpool->sts_mem[1].lsc_hist.phy_addr = _mempool_pop(mempool, bufsize);
	bufpool->sts_mem[1].lsc_hist.size = bufsize;

	//drc_poly
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_DRC_POLY_R);
	bufpool->drc_poly[BUF_IDX0] = bufaddr;
	bufpool->drc_poly[BUF_IDX1] = _mempool_pop(mempool, bufsize);

	//drc_hist
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_DRC_HIST);
	bufpool->sts_mem[0].drc_hist.phy_addr = bufaddr;
	bufpool->sts_mem[0].drc_hist.size = bufsize;
	bufpool->sts_mem[1].drc_hist.phy_addr = _mempool_pop(mempool, bufsize);
	bufpool->sts_mem[1].drc_hist.size = bufsize;

	_isp_rawtop_dma_dump(ctx, pipe);
}

void _isp_rgbtop_dma_setup(struct isp_ctx *ctx, const u8 pipe)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//clut
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_CLUT_R);
	bufpool->clut = bufaddr;

	_isp_rgbtop_dma_dump(ctx, pipe);
}

static void _isp_mctf_dma_setup(struct isp_ctx *ctx, int pipe)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	u8 i = 0;
	u64 buf_unalign = 0;
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	if (!ctx->is_3dnr_on)
		return;

	//TNR_ST_Y/C
	if (ctx->is_fbc_on) {
		buf_unalign = _mempool_get_addr(mempool);

		//ring buffer constraint. reg_base is 256 byte-align
		bufaddr = VI_256_ALIGN(buf_unalign);
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_Y);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));
		bufpool->tdnr[TNR_ST_Y][BUF_IDX0] = bufaddr;

		buf_unalign = _mempool_get_addr(mempool);
		//ring buffer constraint. reg_base is 256 byte-align
		bufaddr = VI_256_ALIGN(buf_unalign);
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_C);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));
		bufpool->tdnr[TNR_ST_C][BUF_IDX0] = bufaddr;
	} else {
		bufaddr = _mempool_get_addr(mempool);
		CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_Y);
		bufpool->tdnr[TNR_ST_Y][BUF_IDX0] = bufaddr;

		bufaddr = _mempool_get_addr(mempool);
		CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_C);
		bufpool->tdnr[TNR_ST_C][BUF_IDX0] = bufaddr;
	}

	//TNR_LD_MO
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MO);
	bufpool->tdnr[TNR_ST_MO][BUF_IDX0] = bufaddr;

	//TNR_LD_MV
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MV);
	bufpool->tdnr[TNR_ST_MV][BUF_IDX0] = bufaddr;

	//TNR_LD_FCB
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_FCB);
	bufpool->tdnr[TNR_ST_FCB][BUF_IDX0] = bufaddr;

	for (i = 0; i < MSP_BUF_MAX; i++) {
		//TNR_LD_MSP, reg_base is 1024 byte-align(venc hw limit)
		buf_unalign = _mempool_get_addr(mempool);
		bufaddr = VI_1K_ALIGN(buf_unalign);
		bufsize = ispblk_dma_buf_get_size(ctx, pipe, ISP_BLK_ID_DMA_CTL_TNR_ST_MSP);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));
		ctx->isp_bufpool[pipe].msp[MSP_MASTER_CHN][i] = bufaddr;

		buf_unalign = _mempool_get_addr(mempool);
		bufaddr = VI_1K_ALIGN(buf_unalign);
		_mempool_pop(mempool, bufsize + (u32)(bufaddr - buf_unalign));
		ctx->isp_bufpool[pipe].msp[MSP_SLAVE_CHN][i] = bufaddr;
	}
}

void _isp_yuvtop_dma_setup(struct isp_ctx *ctx, const u8 pipe)
{
	u64 bufaddr = 0;
	u32 bufsize = 0;
	struct _membuf *bufpool = &ctx->isp_bufpool[pipe];
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	//LDCI hist lmap
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_LDCI_HIST);
	bufpool->sts_mem[0].ldci_hist.phy_addr = bufaddr;
	bufpool->sts_mem[0].ldci_hist.size = bufsize;
	bufpool->sts_mem[1].ldci_hist.phy_addr = _mempool_pop(mempool, bufsize);
	bufpool->sts_mem[1].ldci_hist.size = bufsize;

	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_LDCI_W);
	bufpool->ldci_lmap[BUF_IDX0] = bufaddr;
	bufpool->ldci_lmap[BUF_IDX1] = _mempool_pop(mempool, bufsize);

	//mctf tnr y,c mv, momap, fcb, msp
	_isp_mctf_dma_setup(ctx, pipe);

	//cnr y,uv
	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_CNR_Y_W);
	bufpool->cnr_y = bufaddr;

	bufaddr = _mempool_get_addr(mempool);
	CACL_DMA_SIZE(mempool, pipe, ISP_BLK_ID_DMA_CTL_CNR_C_W);
	bufpool->cnr_c = bufaddr;

	_isp_yuvtop_dma_dump(ctx, pipe);
}

void _isp_cmdq_dma_setup(struct isp_ctx *ctx, const u8 pipe)
{
	u64 bufaddr = 0;
	struct _mempool *mempool = &ctx->isp_mempool[pipe];

	bufaddr = _mempool_get_addr(mempool);
	ctx->isp_pipe_cfg[pipe].cmdq_buf.buf_size = VI_CMDQ_BUF_SIZE;
	ctx->isp_pipe_cfg[pipe].cmdq_buf.phy_addr = bufaddr;
	ctx->isp_pipe_cfg[pipe].cmdq_buf.vir_addr = osal_phys_to_virt(bufaddr);
	_mempool_pop(mempool, VI_CMDQ_BUF_SIZE);
}

void _vi_dma_setup(struct sop_vi_dev *vdev, uint8_t pipe)
{
	struct isp_ctx *ctx = &vdev->ctx;

	if (!ctx->isp_pipe_cfg[pipe].is_enable)
		return;
	if (ctx->isp_pipe_cfg[pipe].is_yuv_sensor &&
		ctx->isp_pipe_cfg[pipe].yuv_scene_mode != ISP_YUV_SCENE_ISP)
		return;

	_isp_rawtop_dma_setup(ctx, pipe);
	_isp_rgbtop_dma_setup(ctx, pipe);
	_isp_yuvtop_dma_setup(ctx, pipe);
	_isp_cmdq_dma_setup(ctx, pipe);
}

static int vi_get_ion_buf(struct sop_vi_dev *vdev, bool is_csi,
			uint8_t idx, struct _mempool *mempool)
{
	int ret = 0;
	uint64_t pAddr = 0;
	void *ion_v = NULL;
	struct isp_ctx *ctx = &vdev->ctx;
	char buf_name[64] = {0};

	mempool->base = 0x80000000;
	mempool->size = 0x40000000;
	mempool->byteused = 0;

	if (is_csi) {
		if (ctx->is_rawreplay)
			return 0;
		_vi_pre_fe_get_dma_size(ctx, idx);
		_vi_pre_ai_isp_get_dma_size(ctx, idx);
	} else {
		_vi_get_pipe_dma_buf_size(ctx, idx);
	}

	if (mempool->byteused == 0) {
		vi_pr(VI_DBG, "yuv sensor not need dma size\n");
		memset(mempool, 0, sizeof(struct _mempool));
		return 0;
	}

	sprintf(buf_name, "VI_%s_%d", is_csi ? "CSI" : "ISP", idx);

	ret = base_ion_alloc(&pAddr, &ion_v, (uint8_t *)buf_name, mempool->byteused, true);
	if (ret != 0) {
		vi_pr(VI_ERR, "VI ion alloc size(%d) failed.\n", mempool->byteused);
		memset(mempool, 0, sizeof(struct _mempool));
		return ERR_VI_NOMEM;
	}

	mempool->base = pAddr;
	mempool->size = mempool->byteused;
	mempool->byteused = 0;

	vi_pr(VI_INFO, "%s dma buf paddr(0x%llx) size=0x%x\n",
		is_csi ? "CSI" : "ISP", mempool->base, mempool->size);

	return 0;
}

int vi_get_csi_ion_buf(struct sop_vi_dev *vdev, enum sop_isp_raw raw_num)
{
	return vi_get_ion_buf(vdev, true, raw_num, &vdev->ctx.csi_mempool[raw_num]);
}

int vi_get_isp_ion_buf(struct sop_vi_dev *vdev, const uint8_t pipe)
{
	return vi_get_ion_buf(vdev, false, pipe, &vdev->ctx.isp_mempool[pipe]);
}

int vi_free_ion_buf(struct sop_vi_dev *vdev, struct _mempool *mempool)
{
	int ret = 0;

	if (mempool == NULL) {
		vi_pr(VI_ERR, "VI ion buf is null.\n");
		return ERR_VI_NOMEM;
	}

	if (mempool->base) {
		ret = base_ion_free(mempool->base);
		if (ret < 0) {
			vi_pr(VI_ERR, "free ion phy fail.\n");
			return ERR_SYS_ILLEGAL_PARAM;
		}
	}

	memset(mempool, 0, sizeof(struct _mempool));

	return 0;
}
