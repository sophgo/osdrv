#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/timer.h>

#include <asm/div64.h>

#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>
#include <linux/delay.h>

#include <base_cb.h>
#include <base_common.h>
#include <vb.h>
#include <vbq.h>
#include "stitch.h"
#include "stitch_core.h"
#include "stitch_reg_cfg.h"
#include "sys.h"
#include "vb.h"

#define STITCH_SIZE_ZERO 0
#undef BIT
#undef BITMASK
#define BIT(nr)               (1U << (nr))
#define BITMASK(msb, lsb)     ((2U << (msb))-(1U << (lsb)))

#define SRC_NUM_2WAY 2
#define SRC_NUM_4WAY 4

#define IDLE_TIMEOUT_MS      10000
#define EOF_WAIT_TIMEOUT_MS  1000

//stitch evt
#define CTX_EVENT_WKUP       0x01
#define CTX_EVENT_EOF        0x02

extern bool gStitchDumpReg;

struct stitch_jobs_ctx {
	struct vb_jobs_t ins[STITCH_MAX_SRC_NUM];
	struct vb_jobs_t out;
};

static struct stitch_jobs_ctx gStitchJobs;
static struct vb_s *stitch_chn_vbq;

struct cvi_stitch_ctx *stitch_ctx;
struct stitch_handler_ctx evt_hdl_ctx;

struct stitch_hw_cfg_param {
	enum stitch_wgt_mode wgt_mode;
	enum stitch_data_src data_src;
	struct stitch_bj_size_param bj_size[STITCH_MAX_SRC_NUM -1];//crop2bj12, crop2bj23,crop2bj34
	struct stitch_rdma_size_param rdma_size[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_wdma_nbld_size_param wdma_nbld_size[STITCH_MAX_SRC_NUM -1][2];
	struct stitch_wdma_bld_size_param wdma_bld_size[STITCH_MAX_SRC_NUM -1][2];
	struct stitch_crop2bj_param crop2bj[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl rdma_l[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl rdma_r[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl rdma_alpha[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl rdma_beta[STITCH_MAX_SRC_NUM -1];
	struct stitch_dma_ctl wdma_l[STITCH_MAX_SRC_NUM -1][3];//img12/img23/img34 combine
	struct stitch_dma_ctl wdma_r[STITCH_MAX_SRC_NUM -1][3];
	struct stitch_dma_ctl wdma_bld[STITCH_MAX_SRC_NUM -1][3];
};

static struct stitch_hw_cfg_param *stitch_hw_cfg_obj;
static struct timespec64 hw_ts[2];
static struct timespec64 sw_ts[2];

static void stitch_record_hw_time_start(void)
{
	ktime_get_ts64(&hw_ts[0]);
	stitch_ctx->job.ts_start = hw_ts[0];
}

static void stitch_record_hw_time_end(void)
{
	ktime_get_ts64(&hw_ts[1]);
	stitch_ctx->job.ts_end = hw_ts[1];
}

static u32 stitch_record_hw_time(void)
{
	CVI_TRACE_STITCH(CVI_DBG_INFO, "start time(%llu)\n", (u64)(hw_ts[0].tv_sec * USEC_PER_SEC + hw_ts[0].tv_nsec / NSEC_PER_USEC));
	CVI_TRACE_STITCH(CVI_DBG_INFO, "end time(%llu)\n", (u64)(hw_ts[1].tv_sec * USEC_PER_SEC + hw_ts[1].tv_nsec / NSEC_PER_USEC));
	return get_diff_in_us(hw_ts[0], hw_ts[1]);
}

static inline u32 stitch_modified_bits_by_value(u32 orig, u32 value, u32 msb, u32 lsb)
{
	u32 bitmask = BITMASK(msb, lsb);

	orig &= ~bitmask;
	return (orig | ((value << lsb) & bitmask));
}

static inline u32 stitch_get_bits_from_value(u32 value, u32 msb, u32 lsb)
{
	return ((value & BITMASK(msb, lsb)) >> lsb);
}

static u8 stitch_handler_is_idle(void)
{
	if (stitch_ctx && stitch_ctx->isCreated && stitch_ctx->isStarted)
		return CVI_FALSE;

	return CVI_TRUE;
}

static void release_stitch_waitq(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	VB_BLK blk;

	if (chn_type == CHN_TYPE_OUT)
		blk = base_mod_jobs_waitq_pop(&gStitchJobs.out);
	else
		blk = base_mod_jobs_waitq_pop(&gStitchJobs.ins[chn.s32DevId]);

	if (blk != VB_INVALID_HANDLE)
		vb_release_block(blk);
}

static int trans_hw_crop2bj(struct stitch_handler_ctx * evt_ctx)
{
	int bj_id;
	u32 bld_size;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;
	bld_size = stitch_ctx->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	switch (evt_ctx->src_id) {
	case STITCH_SRC_ID_1:
	case STITCH_SRC_ID_3:
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height  = stitch_ctx->src_attr.size[bj_id].u32Height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width   = stitch_ctx->src_attr.size[bj_id].u32Width - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height = stitch_ctx->src_attr.size[bj_id + 1].u32Height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width  = stitch_ctx->src_attr.size[bj_id + 1].u32Width - 1;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str  = stitch_ctx->src_attr.bd_attr.bd_lx[bj_id];
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str = stitch_ctx->src_attr.bd_attr.bd_lx[bj_id + 1];
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width - stitch_ctx->src_attr.bd_attr.bd_rx[bj_id];
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width - stitch_ctx->src_attr.bd_attr.bd_rx[bj_id + 1];
		break;
	case STITCH_SRC_ID_2:// use top1 full ovlap blending
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height  = stitch_ctx->src_attr.size[bj_id].u32Height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width   = bld_size - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height = stitch_ctx->src_attr.size[bj_id + 1].u32Height - 1;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width  = bld_size - 1;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;

		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_str  = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_str = 0;
		stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_width;
		stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_width;
		break;
	default:
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);
		break;
	}

	return 0;
}

static int trans_hw_bj_size_sel(enum stitch_src_id src_id, int bj_id, u32 *l_nbldsize, u32 *bld_size, u32 *r_nbldsize)
{
	int ret = 0;

	switch (src_id) {
	case STITCH_SRC_ID_1:
		*l_nbldsize = stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] - stitch_ctx->src_attr.bd_attr.bd_lx[bj_id];
		*r_nbldsize = stitch_ctx->tmp_chn_size[bj_id].u32Width - *bld_size - *l_nbldsize;
		break;
	case STITCH_SRC_ID_2:
		*l_nbldsize = *r_nbldsize = 0;
		break;
	case STITCH_SRC_ID_3:
		*l_nbldsize = stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id - 1];
		*r_nbldsize = stitch_ctx->chn_attr.size.u32Width - stitch_ctx->src_attr.ovlap_attr.ovlp_rx[bj_id] - 1;
		break;
	default:
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "src_id(%d)\n", src_id);
		ret = -1;
		break;
	}
	return ret;
}

static int trans_hw_bj_size(struct stitch_handler_ctx * evt_ctx)
{
	int bj_id;
	u32 bld_size = 0;
	u32 l_nbldsize = 0, r_nbldsize = 0;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;
	bld_size = stitch_ctx->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	trans_hw_bj_size_sel(evt_ctx->src_id, bj_id, &l_nbldsize, &bld_size, &r_nbldsize);

	if (l_nbldsize > 0) {
		if (evt_ctx->src_id == STITCH_SRC_ID_2)
			stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = 0;
		else
			stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = stitch_ctx->src_attr.bd_attr.bd_lx[bj_id];
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left = l_nbldsize - 1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left = STITCH_MAGIC_SIZE;
	}

	if (bld_size > 0) {
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left = l_nbldsize;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left = bld_size + l_nbldsize -1;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right = 0;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right =  bld_size -1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right = STITCH_MAGIC_SIZE;
	}

	if (r_nbldsize > 0) {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_right = bld_size;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_right = bld_size + r_nbldsize -1;
	} else {
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_right = STITCH_MAGIC_SIZE;
		stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_right = STITCH_MAGIC_SIZE;
	}
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "bj_id(%d) lnb-b-rnb[%d-%d-%d] [%d-%d] [%d-%d] [%d-%d]\n", bj_id, l_nbldsize, bld_size, r_nbldsize
		, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_str_left, stitch_hw_cfg_obj->bj_size[bj_id].nbld_w_end_left
		, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_left, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_left
		, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_str_right, stitch_hw_cfg_obj->bj_size[bj_id].bld_w_end_right);

	return 0;
}

static int trans_hw_rdma_size(struct stitch_handler_ctx * evt_ctx)
{
	int i;
	int bj_id;
	u32 bld_size;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;
	bld_size = stitch_ctx->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	for (i = 0; i < 2; i++) {
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_str_r_right = 0;

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_str_r_right = 0;

		if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444
			|| stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR
			|| stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422) {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right = (stitch_ctx->src_attr.size[bj_id + 1].u32Height - 1);
		} else {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right = (stitch_ctx->src_attr.size[bj_id + 1].u32Height - 1) / (i+1);
		}

		if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR  || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR) {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = (stitch_ctx->src_attr.size[bj_id].u32Width - 1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx->src_attr.size[bj_id + 1].u32Width - 1);

			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end;
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end;
		} else {
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left   = (stitch_ctx->src_attr.size[bj_id].u32Width - 1)  / (i+1);
			stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right  = (stitch_ctx->src_attr.size[bj_id + 1].u32Width - 1)  / (i+1);

			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_w_end  / (i+1);
			//stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_w_end / (i+1);
		}

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_left  = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_left;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_w_end_r_right = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_width_r_right;

		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_end_r_left  = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_left;
		stitch_hw_cfg_obj->rdma_size[bj_id][i].crop_h_end_r_right = stitch_hw_cfg_obj->rdma_size[bj_id][i].img_height_r_right;
	}

	if (stitch_ctx->op_attr.wgt_mode == STITCH_WGT_YUV_SHARE) {//mode 0, wgt
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_left  = (bld_size - 1) >> 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_right =  (bld_size - 1) >> 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_left   =  (bld_size - 1) >> 1;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_right  =  (bld_size - 1) >> 1;
	} else if (stitch_ctx->op_attr.wgt_mode == STITCH_WGT_UV_SHARE) {//mode 1, wgt
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_crop_h_end;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_h_end_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_crop_h_end;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_left  = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_left  = bld_size - 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_str_r_right = 0;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].crop_w_end_r_right = bld_size - 1;

		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_left  = stitch_hw_cfg_obj->crop2bj[bj_id].left_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_left   = bld_size - 1;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_height_r_right = stitch_hw_cfg_obj->crop2bj[bj_id].right_img_height;
		stitch_hw_cfg_obj->rdma_size[bj_id][2].img_width_r_right  = bld_size - 1;
	} else {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "do not support this wgt_mode(%d).\n\n", stitch_ctx->op_attr.wgt_mode);
		return -1;
	}

	return 0;
}

static int trans_hw_wdma_size(struct stitch_handler_ctx * evt_ctx)
{
	int i;
	int bj_id;
	u32 bld_size = 0;
	u32 l_nbldsize = 0, r_nbldsize = 0;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;
	bld_size = stitch_ctx->src_attr.ovlap_attr.ovlp_rx[bj_id] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[bj_id] + 1;

	trans_hw_bj_size_sel(evt_ctx->src_id, bj_id, &l_nbldsize, &bld_size, &r_nbldsize);

	for (i = 0; i < 2; i++) {
		stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_str_w_bld = 0;
		stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_str_w_bld = 0;

		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_str_w_left  = 0;
		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_str_w_right = 0;

		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_str_w_left  = 0;
		stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_str_w_right = 0;

		if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444
			|| stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_BGR_888_PLANAR
			|| stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422) {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_end_w_bld = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_right = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_right = (stitch_ctx->src_attr.size[bj_id].u32Height - 1);
		} else {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_h_end_w_bld = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_h_end_w_right = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_left  = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_height_w_right = (stitch_ctx->src_attr.size[bj_id].u32Height - 1) / (i+1);
		}

		if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_BGR_888_PLANAR) {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_end_w_bld = (bld_size) ? (bld_size - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld  = (bld_size) ? (bld_size - 1) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left    = (l_nbldsize) ? (l_nbldsize - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_left   = (l_nbldsize) ? (l_nbldsize - 1) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right   = (r_nbldsize) ? (r_nbldsize - 1) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_right  = (r_nbldsize) ? (r_nbldsize - 1) : (STITCH_SIZE_ZERO);
		} else {
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].crop_w_end_w_bld = (bld_size) ? ((bld_size - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld  = (bld_size) ? ((bld_size - 1) / (i+1)) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left    = (l_nbldsize) ? ((l_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_left   = (l_nbldsize) ? ((l_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);

			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right   = (r_nbldsize) ? ((r_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
			stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].crop_w_end_w_right  = (r_nbldsize) ? ((r_nbldsize - 1) / (i+1)) : (STITCH_SIZE_ZERO);
		}
	}

	return 0;
}

static int trans_hw_op(struct stitch_handler_ctx * evt_ctx)
{
	if (evt_ctx->src_id == STITCH_SRC_ID_1 || evt_ctx->src_id == STITCH_SRC_ID_3) {
		stitch_hw_cfg_obj->wgt_mode = stitch_ctx->op_attr.wgt_mode;
		stitch_hw_cfg_obj->data_src = stitch_ctx->op_attr.data_src;
	}

	return 0;
}

static int trans_hw_dma_id_cfg(struct stitch_handler_ctx * evt_ctx)
{
	int i;
	int bj_id;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;

	switch (evt_ctx->src_id) {
	case STITCH_SRC_ID_1:
	case STITCH_SRC_ID_3:
		for (i = 0; i < 3; i++) {
			stitch_hw_cfg_obj->rdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_LEFTY0);
			stitch_hw_cfg_obj->rdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_RIGHTY0);

			stitch_hw_cfg_obj->wdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_LEFTY0);
			stitch_hw_cfg_obj->wdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_RIGHTY0);
			stitch_hw_cfg_obj->wdma_bld[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_BLDY0);
		}
		stitch_hw_cfg_obj->rdma_alpha[bj_id].dma_id = ID_RDMA_STCH_ALPHA0;
		stitch_hw_cfg_obj->rdma_beta[bj_id].dma_id = ID_RDMA_STCH_BETA0;
		break;
	case STITCH_SRC_ID_2:
		for (i = 0; i < 3; i++) {
			stitch_hw_cfg_obj->rdma_l[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_LEFTY1);
			stitch_hw_cfg_obj->rdma_r[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_RDMA_STCH_RIGHTY1);

			stitch_hw_cfg_obj->wdma_bld[bj_id][i].dma_id = (enum stitch_dma_id) (i + ID_WDMA_STCH_BLDY1);
		}
		stitch_hw_cfg_obj->rdma_alpha[bj_id].dma_id = ID_RDMA_STCH_ALPHA1;
		stitch_hw_cfg_obj->rdma_beta[bj_id].dma_id = ID_RDMA_STCH_BETA1;
		break;
	default:
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);
		break;
	}

	return 0;
}

static int trans_hw_wgt_cfg(struct stitch_handler_ctx * evt_ctx)
{
	int bj_id;
	struct stitch_dma_ctl *rdma_alpha, *rdma_beta;
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);

	if (evt_ctx->src_id == STITCH_SRC_ID_0)
		return 0;

	bj_id = evt_ctx->src_id - 1;
	rdma_alpha = &stitch_hw_cfg_obj->rdma_alpha[bj_id];
	rdma_beta  = &stitch_hw_cfg_obj->rdma_beta[bj_id];

	rdma_alpha->addr = stitch_ctx->wgt_attr.phy_addr_wgt[bj_id][0];
	rdma_beta->addr  = stitch_ctx->wgt_attr.phy_addr_wgt[bj_id][1];

	rdma_alpha->height = stitch_ctx->wgt_attr.size_wgt[bj_id].u32Height;

	rdma_alpha->width  = (stitch_ctx->op_attr.wgt_mode == STITCH_WGT_UV_SHARE)
		? stitch_ctx->wgt_attr.size_wgt[bj_id].u32Width << 1
		: stitch_ctx->wgt_attr.size_wgt[bj_id].u32Width;

	rdma_beta->height  = rdma_alpha->height;
	rdma_beta->width   = rdma_alpha->width;

	rdma_alpha->stride = ALIGN(rdma_alpha->width, STITCH_STRIDE_ALIGN);
	rdma_beta->stride  = ALIGN(rdma_beta->width,  STITCH_STRIDE_ALIGN);

	return 0;
}

static int stitch_trans_to_hw_setting(struct stitch_handler_ctx * evt_ctx)
{
	if (evt_ctx != &evt_hdl_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "invalid thread(%s) param\n", __func__);
		return -1;
	}

	if (stitch_ctx->param_update) {
		if (trans_hw_crop2bj(evt_ctx))
			return -1;
		if (trans_hw_bj_size(evt_ctx))
			return -1;
		if (trans_hw_rdma_size(evt_ctx))
			return -1;
		if (trans_hw_wdma_size(evt_ctx))
			return -1;
		if (trans_hw_op(evt_ctx))
			return -1;
		if (trans_hw_dma_id_cfg(evt_ctx))
			return -1;
		if (trans_hw_wgt_cfg(evt_ctx))
			return -1;
	}

	return 0;
}

static void stitch_hw_trig(void)
{
	if (gStitchDumpReg)
		stitch_dump_register();

	stitch_enable();
}

static void stitch_hw_cfg_reg(u8 top_id, struct stitch_handler_ctx * evt_ctx)
{
	int i;
	u8 bj_id;
	u8 disable_wdma_l = CVI_FALSE, disable_wdma_r = CVI_FALSE, disable_bld = CVI_FALSE;
	u8 disable_rdma_l = CVI_FALSE, disable_rdma_r = CVI_FALSE, disable_rdma_l_r = CVI_FALSE, disable_rdma_wgt = CVI_FALSE;

	if (top_id)
		bj_id = 1;
	else
		bj_id = evt_ctx->src_id - 1;

	if (stitch_ctx->param_update) {
		stitch_bj_image_size_cfg(top_id, &stitch_hw_cfg_obj->bj_size[bj_id]);
		stitch_rdma_image_size_cfg(top_id, stitch_hw_cfg_obj->rdma_size[bj_id]);
		stitch_wdma_image_size_cfg(top_id, stitch_hw_cfg_obj->wdma_nbld_size[bj_id]
			, stitch_hw_cfg_obj->wdma_bld_size[bj_id]);
		stitch_crop2bj_cfg(top_id, &stitch_hw_cfg_obj->crop2bj[bj_id]);

		stitch_mode_sel((u8)stitch_hw_cfg_obj->wgt_mode);

		if (stitch_hw_cfg_obj->rdma_alpha[bj_id].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_alpha[bj_id]);

		if (stitch_hw_cfg_obj->rdma_beta[bj_id].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_beta[bj_id]);

		if (!stitch_hw_cfg_obj->rdma_alpha[bj_id].width && !stitch_hw_cfg_obj->rdma_beta[bj_id].width)
			disable_rdma_wgt = CVI_TRUE;
	}

	for (i = 0; i < NUM_OF_PLANES; i++) {
		if (!stitch_hw_cfg_obj->rdma_l[bj_id][i].width)
			disable_rdma_l = CVI_TRUE;
		if (!stitch_hw_cfg_obj->rdma_r[bj_id][i].width)
			disable_rdma_r = CVI_TRUE;
		if (!stitch_hw_cfg_obj->rdma_l[bj_id][i].width && !stitch_hw_cfg_obj->rdma_r[bj_id][i].width)// src img from vpss
			disable_rdma_l_r = CVI_TRUE;
		else {
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_l[bj_id][i]);
			stitch_dma_cfg(&stitch_hw_cfg_obj->rdma_r[bj_id][i]);
		}
#if 0 // as bmtest
		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_l[0][i]);
		if (stitch_hw_cfg_obj->wdma_l[0][i].width == 0)
			disable_wdma_l = CVI_TRUE;

		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_r[0][i]);
		if (stitch_hw_cfg_obj->wdma_r[0][i].width == 0)
			disable_wdma_r = CVI_TRUE;

		stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_bld[0][i]);
		if (stitch_hw_cfg_obj->wdma_bld[0][i].width == 0)
			disable_bld = CVI_TRUE;
#else
		if (stitch_hw_cfg_obj->wdma_l[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_l[bj_id][i]);
		else
			disable_wdma_l = CVI_TRUE;

		if (stitch_hw_cfg_obj->wdma_r[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_r[bj_id][i]);
		else
			disable_wdma_r = CVI_TRUE;

		if (stitch_hw_cfg_obj->wdma_bld[bj_id][i].width)
			stitch_dma_cfg(&stitch_hw_cfg_obj->wdma_bld[bj_id][i]);
		else
			disable_bld = CVI_TRUE;
#endif
	}

	stitch_valid_param(top_id);

	if (disable_rdma_wgt)
		stitch_disable_wgt(top_id);

	if (disable_rdma_l)
		stitch_disable_left_rdma(top_id);

	if (disable_rdma_r)
		stitch_disable_right_rdma(top_id);

	if (disable_wdma_l)
		stitch_disable_left_wdma(top_id);

	if (disable_wdma_r)
		stitch_disable_right_wdma(top_id);

	if (disable_bld)
		stitch_disable_bld(top_id);

	if (stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR)
		stitch_r_uv_bypass(top_id);
	else if (stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_YUV_PLANAR_422)
		stitch_r_uv_half_bypass(top_id);

	if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_444 || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_RGB_888_PLANAR || stitch_ctx->src_attr.fmt_in == PIXEL_FORMAT_BGR_888_PLANAR)
		stitch_w_uv_bypass(top_id);
	else if (stitch_ctx->chn_attr.fmt_out == PIXEL_FORMAT_YUV_PLANAR_422)
		stitch_w_uv_half_bypass(top_id);
}

static void stitch_hw_cfg_reg_stage1(struct stitch_handler_ctx * evt_ctx)
{
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "src_id(%d) hw_trig stage1\n", evt_ctx->src_id);
	stitch_record_hw_time_start();
	stitch_enable_dev_clk(true);
	stitch_hw_cfg_reg(0, evt_ctx);
	stitch_hw_trig();
}

/*static void stitch_hw_cfg_reg_stage2(struct stitch_handler_ctx * evt_ctx)
{
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "src_id(%d) hw_trig stage2\n", evt_ctx->src_id);
	stitch_hw_cfg_reg(1, evt_ctx);
	stitch_hw_trig();
}*/

static u8 is_hw_buf_prepare_done(struct stitch_handler_ctx * evt_ctx)
{
	u8 prepare_done = CVI_FALSE;

	if (stitch_get_bits_from_value(evt_ctx->prepared_flag, evt_ctx->src_id, evt_ctx->src_id -1) == 0x3)
		prepare_done = CVI_TRUE;
	else {
		CVI_TRACE_STITCH(CVI_DBG_NOTICE, "src buf [%d] or [%d] not ready, prepared_flag[%#x]\n", evt_ctx->src_id, evt_ctx->src_id -1, evt_ctx->prepared_flag);
		if (!base_mod_jobs_workq_empty(&gStitchJobs.ins[evt_ctx->src_id]) && !base_mod_jobs_workq_empty(&gStitchJobs.ins[evt_ctx->src_id -1]))
			prepare_done = CVI_TRUE;
	}
	return prepare_done;
}

static u8 stitch_hw_buf_prepare_done(struct stitch_handler_ctx * evt_ctx)
{
	u8 prepare_done = CVI_FALSE;

	if (!evt_ctx)
		return prepare_done;

	switch (stitch_ctx->src_num) {
	case SRC_NUM_2WAY:
		if (evt_ctx->src_id == STITCH_SRC_ID_1)
			prepare_done = is_hw_buf_prepare_done(evt_ctx);
		break;
	case SRC_NUM_4WAY:
		if (evt_ctx->src_id == STITCH_SRC_ID_1 || evt_ctx->src_id == STITCH_SRC_ID_3)
			prepare_done = is_hw_buf_prepare_done(evt_ctx);
		break;
	default:
		CVI_TRACE_STITCH(CVI_DBG_ERR, "not support stitch_ctx->src_num(%d)\n", stitch_ctx->src_num);
		break;
	}

	return prepare_done;
}

static void stitch_hw_enable_param(struct stitch_handler_ctx * evt_ctx)
{
	unsigned long flags;

	if (stitch_hw_buf_prepare_done(evt_ctx)) {
		stitch_hw_cfg_reg_stage1(evt_ctx);

		spin_lock_irqsave(&evt_ctx->lock, flags);
		evt_ctx->prepared_flag = stitch_modified_bits_by_value(evt_ctx->prepared_flag, 0, evt_ctx->src_id, evt_ctx->src_id -1);
		spin_unlock_irqrestore(&evt_ctx->lock, flags);
	}
}

static void job_fill_buf_in(struct cvi_buffer *buf, struct stitch_handler_ctx * evt_ctx)
{
	u8 i;
	u32 stride;
	u32 width = buf->size.u32Width;
	u32 height = buf->size.u32Height;

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420 || buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422) {
			width  = ((i == 0) ? (buf->size.u32Width)  : (buf->size.u32Width  >> 1));

			if (buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420)
				height = ((i == 0) ? (buf->size.u32Height) : (buf->size.u32Height >> 1));
		}

		stride = ALIGN(width, STITCH_STRIDE_ALIGN);

		switch (evt_ctx->src_id) {
		case STITCH_SRC_ID_0:
			stitch_hw_cfg_obj->rdma_l[0][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_l[0][i].height = height;
			stitch_hw_cfg_obj->rdma_l[0][i].width = width;
			stitch_hw_cfg_obj->rdma_l[0][i].stride = stride;
			break;
		case STITCH_SRC_ID_1:
			stitch_hw_cfg_obj->rdma_r[0][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_r[0][i].height = height;
			stitch_hw_cfg_obj->rdma_r[0][i].width = width;
			stitch_hw_cfg_obj->rdma_r[0][i].stride = stride;
			break;
		case STITCH_SRC_ID_2:
			stitch_hw_cfg_obj->rdma_l[2][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_l[2][i].height = height;
			stitch_hw_cfg_obj->rdma_l[2][i].width = width;
			stitch_hw_cfg_obj->rdma_l[2][i].stride = stride;
			break;
		case STITCH_SRC_ID_3:
			stitch_hw_cfg_obj->rdma_r[2][i].addr = buf->phy_addr[i];
			stitch_hw_cfg_obj->rdma_r[2][i].height = height;
			stitch_hw_cfg_obj->rdma_r[2][i].width = width;
			stitch_hw_cfg_obj->rdma_r[2][i].stride = stride;
			break;
		default:
			CVI_TRACE_STITCH(CVI_DBG_DEBUG, "evt_ctx->src_id(%d)\n", evt_ctx->src_id);
			break;
		}
	}
	//todo:
	//need calulate rdma for img23
}

static void job_fill_buf_out(struct cvi_buffer *buf, struct stitch_handler_ctx * evt_ctx)
{
	int i = 0;
	u32 l_nbldwidth;
	u32 bldwidth;
	u32 r_nbldwidth;
	u32 height;
	u32 stride = ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN);
	(void)(evt_ctx);

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420 || buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422) {
			stride = (i == 0
				? ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN)
				: ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN) >> 1);
		}

		if (i < 2) {
			l_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_left) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_left + 1) : (STITCH_SIZE_ZERO);

			bldwidth = (stitch_hw_cfg_obj->wdma_bld_size[0][i].img_width_w_bld) ?
				(stitch_hw_cfg_obj->wdma_bld_size[0][i].img_width_w_bld + 1) : (STITCH_SIZE_ZERO);

			r_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_right) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[0][i].img_width_w_right + 1) : (STITCH_SIZE_ZERO);

			height = stitch_hw_cfg_obj->wdma_bld_size[0][i].img_height_w_bld + 1;
		}
		stitch_hw_cfg_obj->wdma_l[0][i].height = height;
		stitch_hw_cfg_obj->wdma_r[0][i].height = height;
		stitch_hw_cfg_obj->wdma_bld[0][i].height = height;

		stitch_hw_cfg_obj->wdma_l[0][i].width = l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[0][i].width = r_nbldwidth;
		stitch_hw_cfg_obj->wdma_bld[0][i].width = bldwidth;

		stitch_hw_cfg_obj->wdma_l[0][i].stride = stride;
		stitch_hw_cfg_obj->wdma_r[0][i].stride = stride;
		stitch_hw_cfg_obj->wdma_bld[0][i].stride = stride;

		stitch_hw_cfg_obj->wdma_l[0][i].addr = buf->phy_addr[i];
		stitch_hw_cfg_obj->wdma_r[0][i].addr = buf->phy_addr[i] + l_nbldwidth + bldwidth;
		stitch_hw_cfg_obj->wdma_bld[0][i].addr = buf->phy_addr[i] + l_nbldwidth;

		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "wdma_l:w[%d]-wdma_bld:w[%d]-wdma_r:w[%d]\n"
			, stitch_hw_cfg_obj->wdma_l[0][i].width, stitch_hw_cfg_obj->wdma_bld[0][i].width, stitch_hw_cfg_obj->wdma_r[0][i].width);
	}
}

static void job_fill_buf_out34(struct cvi_buffer *buf, struct stitch_handler_ctx * evt_ctx)
{
	int i = 0;
	u32 l_nbldwidth;
	u32 bldwidth;
	u32 r_nbldwidth;
	u32 height;
	u32 stride = ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN);
	u8 bj_id = evt_ctx->src_id - 1;
	u64 addr_offset;

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_420 || buf->enPixelFormat == PIXEL_FORMAT_YUV_PLANAR_422) {
			stride = (i == 0
				? ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN)
				: ALIGN(buf->size.u32Width, STITCH_STRIDE_ALIGN) >> 1);
		}

		if (i < 2) {
			l_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_left + 1) : (STITCH_SIZE_ZERO);

			bldwidth = (stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld) ?
				(stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_width_w_bld + 1) : (STITCH_SIZE_ZERO);

			r_nbldwidth = (stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right) ?
				(stitch_hw_cfg_obj->wdma_nbld_size[bj_id][i].img_width_w_right + 1) : (STITCH_SIZE_ZERO);

			height = stitch_hw_cfg_obj->wdma_bld_size[bj_id][i].img_height_w_bld + 1;
		}
		stitch_hw_cfg_obj->wdma_l[bj_id][i].height = height;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].height = height;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].height = height;

		stitch_hw_cfg_obj->wdma_l[bj_id][i].width = l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].width = r_nbldwidth;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].width = bldwidth;

		stitch_hw_cfg_obj->wdma_l[bj_id][i].stride = stride;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].stride = stride;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].stride = stride;

		addr_offset = (i==0 ? stitch_ctx->src_attr.ovlap_attr.ovlp_lx[1] : stitch_ctx->src_attr.ovlap_attr.ovlp_lx[1]>>1);

		stitch_hw_cfg_obj->wdma_l[bj_id][i].addr   = buf->phy_addr[i] + addr_offset;
		stitch_hw_cfg_obj->wdma_bld[bj_id][i].addr = stitch_hw_cfg_obj->wdma_l[bj_id][i].addr + l_nbldwidth;
		stitch_hw_cfg_obj->wdma_r[bj_id][i].addr   = stitch_hw_cfg_obj->wdma_bld[bj_id][i].addr + bldwidth;

		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "wdma_l:w[%d]-wdma_bld:w[%d]-wdma_r:w[%d]\n"
			, stitch_hw_cfg_obj->wdma_l[bj_id][i].width, stitch_hw_cfg_obj->wdma_bld[bj_id][i].width, stitch_hw_cfg_obj->wdma_r[bj_id][i].width);
	}
}

static void stitch_fill_cvi_buffer(struct cvi_buffer *src_buf, uint64_t phy_addr, struct cvi_buffer *buf)
{
	u8 align = (stitch_ctx->chn_attr.size.u32Width % (STITCH_ALIGN << 1))
		? (STITCH_ALIGN >> 1)
		: (STITCH_ALIGN);

	base_get_frame_info(stitch_ctx->chn_attr.fmt_out, stitch_ctx->chn_attr.size, buf, phy_addr, align);
	buf->s16OffsetTop = 0;
	buf->s16OffsetBottom = 0;
	buf->s16OffsetLeft = 0;
	buf->s16OffsetRight = 0;

	if (src_buf) {
		buf->u64PTS = src_buf->u64PTS;
		buf->frm_num = src_buf->frm_num;
		//buf->motion_lv = grp_buf->motion_lv;
		//memcpy(buf->motion_table, grp_buf->motion_table, MO_TBL_SIZE);
	}
}

static s32 stitch_chn_qbuf(struct cvi_buffer *src_buf, VB_BLK blk, struct stitch_handler_ctx * evt_ctx)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH};
	struct vb_s *vb = (struct vb_s *)blk;
	s32 ret = CVI_SUCCESS;

	stitch_fill_cvi_buffer(src_buf, vb_handle2phys_addr(blk), &vb->buf);//convert chn blk to cvi_buf, and copy pts info

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "frm_num(%d) size[%d-%d] Chn buf[0x%lx-0x%lx-0x%lx] stride[%d-%d-%d] len[%zu-%zu-%zu]\n"
			, vb->buf.frm_num
			, vb->buf.size.u32Width, vb->buf.size.u32Height
			, (unsigned long)vb->buf.phy_addr[0]
			, (unsigned long)vb->buf.phy_addr[1]
			, (unsigned long)vb->buf.phy_addr[2]
			, vb->buf.stride[0], vb->buf.stride[1], vb->buf.stride[2]
			, vb->buf.length[0], vb->buf.length[1], vb->buf.length[2]);

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &gStitchJobs.out, blk);
	if (ret != CVI_SUCCESS)
		CVI_TRACE_STITCH(CVI_DBG_ERR, "Chn qbuf failed\n");
	else
		job_fill_buf_out(&vb->buf, evt_ctx);

	vb_release_block(blk);

	return ret;
}

static int fill_buffers(struct stitch_handler_ctx * evt_ctx)
{
	struct cvi_buffer *buf_in = NULL, *buf_out = NULL;
	VB_BLK blk_out = VB_INVALID_HANDLE, blk_src;
	s32 ret = CVI_SUCCESS;
	struct vb_s *vb = NULL;
	int i;

	if (base_mod_jobs_waitq_empty(&gStitchJobs.ins[evt_ctx->src_id])) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src_id(%d) waitq_empty, buf not ready\n", evt_ctx->src_id);
		ret = CVI_ERR_STITCH_BUF_EMPTY;
		goto ERR_FILL_QBUF;
	}

	buf_in = base_mod_jobs_enque_work(&gStitchJobs.ins[evt_ctx->src_id]);
	if (buf_in == NULL) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src_id(%d) qbuf failed.\n", evt_ctx->src_id);
		ret = CVI_ERR_STITCH_NOTREADY;
		goto ERR_FILL_QBUF;
	}

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "src_id(%d) buf: 0x%lx-0x%lx-0x%lx\n"
		, evt_ctx->src_id
		, (unsigned long)buf_in->phy_addr[0]
		, (unsigned long)buf_in->phy_addr[1]
		, (unsigned long)buf_in->phy_addr[2]);

	job_fill_buf_in(buf_in, evt_ctx);

	if (evt_ctx->src_id == STITCH_SRC_ID_1) {
		if (!base_mod_jobs_waitq_empty(&gStitchJobs.out)) {// chn buffer from user
			CVI_TRACE_STITCH(CVI_DBG_DEBUG, "chn buffer from user.\n");

			buf_out = base_mod_jobs_enque_work(&gStitchJobs.out);
			if (!buf_out) {
				CVI_TRACE_STITCH(CVI_DBG_ERR, "chn qbuf failed.\n");
				ret = CVI_ERR_STITCH_NOTREADY;
				goto ERR_FILL_QBUF;
			}

			memcpy(&stitch_chn_vbq->buf, buf_out, sizeof(*buf_out));
			blk_out = (VB_BLK)stitch_chn_vbq;
			job_fill_buf_out(&stitch_chn_vbq->buf, evt_ctx);
		} else {// chn buffer from pool
			blk_out = vb_get_block_with_id(stitch_ctx->VbPool, stitch_ctx->u32VBSize, CVI_ID_STITCH);
			if (blk_out == VB_INVALID_HANDLE) {
				CVI_TRACE_STITCH(CVI_DBG_ERR, "Can't acquire VB BLK for STITCH chn\n");
				ret = CVI_ERR_STITCH_NOBUF;
				goto ERR_FILL_QBUF;
			}
			if (stitch_chn_qbuf(buf_in, blk_out, evt_ctx)) {
				CVI_TRACE_STITCH(CVI_DBG_ERR, "Chn qbuf failed.\n");
				ret = CVI_ERR_STITCH_NOTREADY;
				goto ERR_FILL_QBUF;
			}
		}

		stitch_ctx->vb_out = (struct vb_s *)blk_out;
	} else {
		if (evt_ctx->src_id == STITCH_SRC_ID_3) {
			vb = stitch_ctx->vb_out;
			job_fill_buf_out34(&vb->buf, evt_ctx);
		}
	}
	return ret;

ERR_FILL_QBUF:
	for (i = evt_ctx->src_id; i >= 0; i--) {
		while (!base_mod_jobs_waitq_empty(&gStitchJobs.ins[i])) {
			blk_src = base_mod_jobs_waitq_pop(&gStitchJobs.ins[i]);
			CVI_TRACE_STITCH(CVI_DBG_WARN, "in [%d] waitq pop\n", i);
			if (blk_src != VB_INVALID_HANDLE)
				vb_release_block(blk_src);
		}
		while (!base_mod_jobs_workq_empty(&gStitchJobs.ins[i])) {
			blk_src = base_mod_jobs_workq_pop(&gStitchJobs.ins[i]);
			CVI_TRACE_STITCH(CVI_DBG_WARN, "in [%d] workq pop\n", i);
			if (blk_src != VB_INVALID_HANDLE)
				vb_release_block(blk_src);
		}
	}

	while (!base_mod_jobs_waitq_empty(&gStitchJobs.out)) {
		blk_out = base_mod_jobs_waitq_pop(&gStitchJobs.out);
		CVI_TRACE_STITCH(CVI_DBG_WARN, "out waitq pop\n");
		if (blk_out != VB_INVALID_HANDLE)
			vb_release_block(blk_src);
	}
	while (!base_mod_jobs_workq_empty(&gStitchJobs.out)) {
		blk_out = base_mod_jobs_workq_pop(&gStitchJobs.out);
		CVI_TRACE_STITCH(CVI_DBG_WARN, "out workq pop\n");
		if (blk_out != VB_INVALID_HANDLE)
			vb_release_block(blk_out);
	}

	return ret;
}

static void stitch_notify_wkup_evt_th(void)
{
	unsigned long flags;

	spin_lock_irqsave(&evt_hdl_ctx.lock, flags);
	evt_hdl_ctx.events |= CTX_EVENT_WKUP;
	spin_unlock_irqrestore(&evt_hdl_ctx.lock, flags);

	wake_up_interruptible(&evt_hdl_ctx.wait);
}

static int stitch_src_qbuf(MMF_CHN_S chn, VB_BLK blk)
{
	s32 ret;
	int src_id = chn.s32DevId;

	if(!STITCH_CTX_IS_ENBALE())
		return CVI_FAILURE;

	ret = vb_qbuf(chn, CHN_TYPE_IN, &gStitchJobs.ins[src_id], blk);
	if (ret != CVI_SUCCESS)
		return ret;

	stitch_ctx->work_status.recv_cnt++;
	stitch_notify_wkup_evt_th();

	return CVI_SUCCESS;
}

static void stitch_handle_frm_done(struct cvi_stitch_ctx *p_stitch_ctx)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH};
	struct timespec64 time;
	VB_BLK blk;
	int i;

	for (i = 0; i < p_stitch_ctx->src_num; i++) {
		vb_dqbuf(chn, &gStitchJobs.ins[i], &blk);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "Mod(%d) src_id(%d) can't get vb-blk\n", chn.enModId, i);
		} else {
			vb_done_handler(chn, CHN_TYPE_IN, &gStitchJobs.ins[i], blk);
		}
	}

	vb_dqbuf(chn, &gStitchJobs.out, &blk);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.enModId);
	} else {
		struct vb_s * vb = (struct vb_s *)blk;

		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "done buf:0x%llx\n", vb->buf.phy_addr[0]);
		vb_done_handler(chn, CHN_TYPE_OUT, &gStitchJobs.out, blk);
	}

	p_stitch_ctx->work_status.done_cnt++;

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get eof cnt(%d)\n", p_stitch_ctx->work_status.done_cnt);

	ktime_get_ts64(&time);
	p_stitch_ctx->work_status.duration = get_diff_in_us(p_stitch_ctx->time, time);
}

static void stitch_work_handle_frm_done(struct cvi_stitch_ctx *p_stitch_ctx)
{
	unsigned long flags;
	enum stitch_handler_state hdlState;
	if(!STITCH_CTX_IS_VALID(p_stitch_ctx))
		return;

	// need update in fill buf???
	// Update vpss grp proc info
	//_update_vpss_grp_proc(workingGrp, duration, u32HwDuration);

	spin_lock_irqsave(&p_stitch_ctx->lock, flags);
	hdlState = (enum stitch_handler_state)atomic_read(&p_stitch_ctx->enHdlState);
	spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);

	if (hdlState == STITCH_HANDLER_STATE_RUN || hdlState == STITCH_HANDLER_STATE_DONE) {
		if (p_stitch_ctx->src_num == 2) {
		//if (p_stitch_ctx->src_num == 2 || p_stitch_ctx->src_num == 4) {//this is debug only bld img23
			stitch_handle_frm_done(p_stitch_ctx);
			spin_lock_irqsave(&p_stitch_ctx->lock, flags);
			atomic_set(&p_stitch_ctx->enHdlState, STITCH_HANDLER_STATE_DONE);
			stitch_ctx->evt = 1;
			wake_up_interruptible(&stitch_ctx->wq);
			if (p_stitch_ctx->param_update)
				p_stitch_ctx->param_update = CVI_FALSE;
			spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
		} else {
			spin_lock_irqsave(&p_stitch_ctx->lock, flags);
			atomic_set(&p_stitch_ctx->enHdlState, STITCH_HANDLER_STATE_RUN_STAGE2);
			p_stitch_ctx->param_update = CVI_TRUE;
			spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
		}
	} else if (hdlState == STITCH_HANDLER_STATE_RUN_STAGE2) {
		stitch_handle_frm_done(p_stitch_ctx);
		spin_lock_irqsave(&p_stitch_ctx->lock, flags);
		atomic_set(&p_stitch_ctx->enHdlState, STITCH_HANDLER_STATE_DONE);
		stitch_ctx->evt = 1;
		wake_up_interruptible(&stitch_ctx->wq);
		if (p_stitch_ctx->param_update)
			p_stitch_ctx->param_update = CVI_FALSE;
		spin_unlock_irqrestore(&p_stitch_ctx->lock, flags);
	} else
		CVI_TRACE_STITCH(CVI_DBG_ERR, "invalid handler state(%d)\n", hdlState);
}

static void stitch_work_frm_done(struct work_struct *work)
{
	struct cvi_stitch_ctx *p_stitch_ctx = container_of(work, struct cvi_stitch_ctx, work_frm_done);

	if(!STITCH_CTX_IS_VALID(p_stitch_ctx))
		return;

	stitch_work_handle_frm_done(p_stitch_ctx);

	stitch_notify_wkup_evt_th();
}

static void stitch_wkup_frm_done_work(void *data)
{
	struct cvi_stitch_ctx *p_stitch_ctx = (struct cvi_stitch_ctx *)data;

	if (!p_stitch_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "dev isn't created yet.\n");
		return;
	}

	if (p_stitch_ctx != stitch_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "dev ctx is invalid.\n");
		return;
	}

	stitch_record_hw_time_end();
	p_stitch_ctx->work_status.hw_cost_time = stitch_record_hw_time();
	if (p_stitch_ctx->work_status.hw_cost_time > p_stitch_ctx->work_status.hw_max_cost_time) {
		p_stitch_ctx->work_status.hw_max_cost_time = p_stitch_ctx->work_status.hw_cost_time;
	}
	p_stitch_ctx->work_status.u32HwDuration += p_stitch_ctx->work_status.hw_cost_time;
	CVI_TRACE_STITCH(CVI_DBG_INFO, "hardware time cost:[%u] us\n", p_stitch_ctx->work_status.hw_cost_time);

	queue_work(evt_hdl_ctx.workqueue, &stitch_ctx->work_frm_done);
}

static s32 stitch_try_schedule(struct stitch_handler_ctx * evt_ctx)
{

	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH, .s32DevId = evt_ctx->src_id};
	unsigned long flags;
	ktime_get_ts64(&sw_ts[0]);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "cur src_id[%d]\n", evt_ctx->src_id);

	if(!STITCH_CTX_IS_ENBALE())
		goto stitch_ctx_not_enable;

	if (stitch_trans_to_hw_setting(evt_ctx)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "trans to hw settings NG.\n");
		release_stitch_waitq(chn, CHN_TYPE_IN);
		stitch_ctx->work_status.fail_recv_cnt++;
		goto stitch_next_job;
	}

	if (fill_buffers(evt_ctx)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "fill buffer NG.\n");
		stitch_ctx->work_status.fail_recv_cnt++;
		goto stitch_next_job;
	}

	spin_lock_irqsave(&evt_ctx->lock, flags);
	evt_ctx->prepared_flag |= BIT(evt_ctx->src_id);
	spin_unlock_irqrestore(&evt_ctx->lock, flags);

	/* Update state first, isr could occur immediately */
	spin_lock_irqsave(&stitch_ctx->lock, flags);
	evt_ctx->src_id >= STITCH_SRC_ID_2
		? atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_RUN_STAGE2)
		: atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_RUN);
	stitch_ctx->job.data = (void *)stitch_ctx;
	atomic_set(&stitch_ctx->job.enJobState, STITCH_JOB_WORKING);
	stitch_ctx->job.pfnJobCB = stitch_wkup_frm_done_work;
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	//if(evt_ctx->src_id == 1) //this is debug only bld img23
		//return CVI_SUCCESS;
	stitch_hw_enable_param(evt_ctx);

	//we send buf to hw(vpss hal)
	//cvi_vpss_hal_push_try_schedule(&vpss_ctx->stJob);
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "post job src[%d] done\n", evt_ctx->src_id);

	// wait for h/w done
	return CVI_SUCCESS;

stitch_next_job:
stitch_ctx_not_enable:

	return CVI_FAILURE;
}

static int stitch_event_handler_th(void *arg)
{
	struct stitch_handler_ctx *evt_ctx = (struct stitch_handler_ctx *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	unsigned long flags;
	int ret;
	u8 i;
	struct vb_jobs_t *jobs;
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH};
	enum stitch_handler_state enHdlState;

	if (evt_ctx != &evt_hdl_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "invalid thread(%s) param\n", __func__);
		return -1;
	}

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(evt_ctx->wait,
			evt_ctx->events || kthread_should_stop(), timeout);

		/* -ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		//suspend
		if (is_stitch_suspended())
			continue;

		/* timeout */
		if (!ret && stitch_handler_is_idle())
			continue;

		spin_lock_irqsave(&evt_ctx->lock, flags);
		evt_ctx->events &= ~CTX_EVENT_WKUP;
		spin_unlock_irqrestore(&evt_ctx->lock, flags);

		if (!stitch_ctx)
			continue;
		if (!stitch_ctx->isStarted)
			continue;

		spin_lock_irqsave(&stitch_ctx->lock, flags);
		enHdlState = (enum stitch_handler_state)atomic_read(&stitch_ctx->enHdlState);
		spin_unlock_irqrestore(&stitch_ctx->lock, flags);

		if (enHdlState != STITCH_HANDLER_STATE_RUN &&
			enHdlState != STITCH_HANDLER_STATE_RUN_STAGE2 &&
			enHdlState != STITCH_HANDLER_STATE_DONE)
			continue;
		/*if ((atomic_read(&stitch_ctx->enHdlState == STITCH_HANDLER_STATE_RUN_STAGE2)) {
			stitch_hw_cfg_reg_stage2(evt_ctx);
			//continue;
		}*/
		for (i = 0; i < stitch_ctx->src_num; i++) {
			//if (atomic_read(&stitch_ctx->enHdlState) == STITCH_HANDLER_STATE_RUN && i >= STITCH_SRC_ID_2) //this is debug only bld img23
				//break;
			if (enHdlState == STITCH_HANDLER_STATE_RUN && i >= STITCH_SRC_ID_2)
				break;
			chn.s32DevId = i;
			jobs = &gStitchJobs.ins[chn.s32DevId];
			if (!jobs) {
				CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get (%d) jobs failed, src num(%d)\n", i, stitch_ctx->src_num);
				continue;
			}

			if (!down_trylock(&jobs->sem)) {
				evt_ctx->src_id = chn.s32DevId;
				stitch_try_schedule(evt_ctx);
			}
		}
		/* Adjust timeout */
		timeout = stitch_handler_is_idle() ? idle_timeout : eof_timeout;
	}

	return 0;
}

static int stitch_creat_hw_cfg_obj(void)
{
	if (!stitch_hw_cfg_obj) {
		stitch_hw_cfg_obj = kzalloc(sizeof(struct stitch_hw_cfg_param), GFP_ATOMIC);//auto memset
		if (!stitch_hw_cfg_obj) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "kzalloc fail.\n");
			return CVI_ERR_STITCH_NOMEM;
		}
	}

	return 0;
}

static int stitch_destrory_hw_cfg_obj(void)
{
	if (stitch_hw_cfg_obj) {
		kfree(stitch_hw_cfg_obj);
		stitch_hw_cfg_obj = CVI_NULL;
	}

	return 0;
}

static int stitch_creat_ctx(void)
{
	if (!stitch_ctx) {
		stitch_ctx = kzalloc(sizeof(struct cvi_stitch_ctx), GFP_ATOMIC);//auto memset
		if (!stitch_ctx) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "kzalloc fail.\n");
			return CVI_ERR_STITCH_NOMEM;
		}
		stitch_ctx->isCreated = CVI_TRUE;
	}

	return 0;
}

static void stitch_destrory_ctx(void)
{
	if (stitch_ctx) {
		stitch_ctx->isCreated = CVI_FALSE;
		kfree(stitch_ctx);
		stitch_ctx = CVI_NULL;
	}
}

static void stitch_init_sw_param_ctx(void)
{
	if (stitch_ctx) {
		spin_lock_init(&stitch_ctx->lock);
		init_waitqueue_head(&stitch_ctx->wq);
		stitch_ctx->evt = 0;
	}
}

struct cvi_stitch_ctx *stitch_get_ctx(void)
{
	return stitch_ctx;
}

static u8 stitch_creat_chn_vbq(void)
{
	if (!stitch_chn_vbq) {
		stitch_chn_vbq = kzalloc(sizeof(*stitch_chn_vbq), GFP_ATOMIC);
		if (!stitch_chn_vbq)
			return 1;
	}

	return 0;
}
static u8 stitch_destroy_chn_vbq(void)
{
	if (stitch_chn_vbq) {
		kfree(stitch_chn_vbq);
		stitch_chn_vbq = CVI_NULL;
	}

	return 0;
}

s32 stitch_init(void)
{
	if (stitch_creat_ctx())
		return -1;

	stitch_init_sw_param_ctx();
	stitch_ctx->update_status = 0;

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "stitch ctx init+\n");
	return 0;
}

s32 stitch_deinit(void)
{
	stitch_disable();
	stitch_invalid_param(0);
	stitch_invalid_param(1);

	stitch_destrory_ctx();

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "stitch ctx deinit+\n");
	return 0;
}

s32 stitch_reset(void)
{
	stitch_disable();
	stitch_reset_init();
	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "stitch_reset+\n");
	return 0;
}

s32 stitch_send_frame(STITCH_SRC_IDX src_idx, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH, .s32DevId = src_idx, .s32ChnId = 0};
	VB_BLK blk;
	s32 ret;
	unsigned long flags;

	if (is_stitch_suspended()) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch dev suspend\n");
		return CVI_ERR_STITCH_NOT_PERM;
	}

	ret = STITCH_CHECK_INPUT_VIDEO_PARAM(src_idx, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	pstVideoFrame->stVFrame.u32Align = STITCH_ALIGN;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "src_idx(%d) no space for malloc.\n", src_idx);
			return CVI_ERR_STITCH_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src_idx(%d) base_fill_videoframe2buffer fail\n", src_idx);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (stitch_src_qbuf(chn, blk) != CVI_SUCCESS) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src_idx(%d) qbuf failed\n", src_idx);
		return CVI_ERR_STITCH_BUSY;
	}

	if (src_idx == STITCH_SRC_ID_1) {
		spin_lock_irqsave(&stitch_ctx->lock, flags);
		atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_RUN);
		spin_unlock_irqrestore(&stitch_ctx->lock, flags);
	}

	CVI_TRACE_STITCH(CVI_DBG_INFO, "src_id[%d] phy_addr(%llx-%llx-%llx)\n", src_idx, pstVideoFrame->stVFrame.u64PhyAddr[0]
		, pstVideoFrame->stVFrame.u64PhyAddr[1], pstVideoFrame->stVFrame.u64PhyAddr[2]);
	return CVI_SUCCESS;
}

s32 stitch_send_chn_frame(VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH};
	VB_BLK blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;
	s32 ret;

	if (is_stitch_suspended()) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch dev suspend\n");
		return CVI_ERR_STITCH_NOT_PERM;
	}

	ret = STITCH_CHECK_OUTPUT_VIDEO_PARAM(pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	pstVideoFrame->stVFrame.u32Align = STITCH_ALIGN;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "chn no space for malloc.\n");
			return CVI_ERR_STITCH_NOMEM;
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "chn base_fill_videoframe2buffer fail\n");
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	jobs = &gStitchJobs.out;
	if (!jobs) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "chn get job failed\n");
		return CVI_FAILURE;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "chn waitq is full\n");
		mutex_unlock(&jobs->lock);
		return CVI_FAILURE;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	CVI_TRACE_STITCH(CVI_DBG_INFO, "chn phy_addr(%llx-%llx-%llx)\n", pstVideoFrame->stVFrame.u64PhyAddr[0]
		, pstVideoFrame->stVFrame.u64PhyAddr[1], pstVideoFrame->stVFrame.u64PhyAddr[2]);
	return CVI_SUCCESS;
}

s32 stitch_get_chn_frame(VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret, i;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	MMF_CHN_S chn = {.enModId = CVI_ID_STITCH};

	ret = STITCH_CHECK_NULL_PTR(pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_ENBALE()) {
		ret = CVI_ERR_STITCH_NOTREADY;
		return ret;
	}

	memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));
	ret = base_get_chn_buffer(chn, &gStitchJobs.out, &blk, s32MilliSec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "get chn frame fail, s32MilliSec=%d, ret=%d\n", s32MilliSec, ret);
		return CVI_ERR_STITCH_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.u32Width) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "buf already released\n");
		return CVI_ERR_STITCH_BUF_EMPTY;
	}

	pstVideoFrame->stVFrame.enPixelFormat = stitch_ctx->chn_attr.fmt_out;
	pstVideoFrame->stVFrame.u32Width = vb->buf.size.u32Width;
	pstVideoFrame->stVFrame.u32Height = vb->buf.size.u32Height;
	pstVideoFrame->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstVideoFrame->stVFrame.u64PTS = vb->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstVideoFrame->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstVideoFrame->stVFrame.u32Length[i] = vb->buf.length[i];
		pstVideoFrame->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstVideoFrame->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstVideoFrame->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstVideoFrame->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstVideoFrame->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstVideoFrame->stVFrame.pPrivateData = vb;
	pstVideoFrame->stVFrame.u32Align = STITCH_ALIGN;

	CVI_TRACE_STITCH(CVI_DBG_INFO, "end to get pstVideoFrame width:%d height:%d buf:0x%llx\n"
		, pstVideoFrame->stVFrame.u32Width
		, pstVideoFrame->stVFrame.u32Height
		, pstVideoFrame->stVFrame.u64PhyAddr[0]);

	ktime_get_ts64(&sw_ts[1]);
	stitch_ctx->work_status.cost_time = get_diff_in_us(sw_ts[0], sw_ts[1]);
	stitch_ctx->work_status.u32SwDuration += stitch_ctx->work_status.cost_time;
	if (stitch_ctx->work_status.cost_time > stitch_ctx->work_status.max_cost_time) {
		stitch_ctx->work_status.max_cost_time = stitch_ctx->work_status.cost_time;
	}
	CVI_TRACE_STITCH(CVI_DBG_INFO, "software time cost:[%u] us\n", stitch_ctx->work_status.cost_time);
	return CVI_SUCCESS;
}

s32 stitch_release_chn_frame(const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VB_BLK blk;
	s32 ret;

	ret = STITCH_CHECK_NULL_PTR(pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_SUPPORT_FMT(pstVideoFrame->stVFrame.enPixelFormat);
	if (ret != CVI_SUCCESS) {
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "fmt[%d]\n", pstVideoFrame->stVFrame.enPixelFormat);
		return ret;
	}

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (pstVideoFrame->stVFrame.pPrivateData == 0) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "phy-address(0x%llx) invalid to locate.\n"
				, (unsigned long long)pstVideoFrame->stVFrame.u64PhyAddr[0]);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (VB_BLK)pstVideoFrame->stVFrame.pPrivateData;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "buf:0x%llx\n", pstVideoFrame->stVFrame.u64PhyAddr[0]);
	return CVI_SUCCESS;
}

s32 stitch_set_src_attr(const STITCH_SRC_ATTR_S *src_attr)
{
	s32 ret;
	u8 i;
	u8 src_num;
	unsigned long flags;

	ret = STITCH_CHECK_INPUT_PARAM(src_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	switch (src_attr->way_num) {
	case STITCH_2_WAY:
		src_num = SRC_NUM_2WAY;
		break;
	case STITCH_4_WAY:
		src_num = SRC_NUM_4WAY;
		break;
	default:
		CVI_TRACE_STITCH(CVI_DBG_ERR, "not support this waynum:%d\n", src_attr->way_num);
		break;
	}

	for (i = 0; i < src_num; i++) {
		ret = STITCH_CHECK_YUV_PARAM(src_attr->fmt_in, src_attr->size[i].u32Width, src_attr->size[i].u32Height);
		if (ret != CVI_SUCCESS)
			return ret;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->src_num = src_num;
	stitch_ctx->param_update = CVI_TRUE;
	stitch_ctx->update_status |= STITCH_UPDATE_SRC;
	memcpy(&stitch_ctx->src_attr, src_attr, sizeof(*src_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "set src_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_get_src_attr(STITCH_SRC_ATTR_S *src_attr)
{
	s32 ret;
	unsigned long flags;

	ret = STITCH_CHECK_NULL_PTR(src_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	memcpy(src_attr, &stitch_ctx->src_attr, sizeof(*src_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get src_attr+\n");
	return CVI_SUCCESS;
}

static void stitch_cfg_tmp_chn_size(void)
{
	if ((stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] == 0)
		&& (stitch_ctx->src_attr.size[0].u32Width == stitch_ctx->src_attr.size[1].u32Width)
		&& (stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] + 1) == stitch_ctx->src_attr.size[0].u32Width) //full ovlp
		stitch_ctx->tmp_chn_size[0].u32Width = stitch_ctx->src_attr.size[0].u32Width;
	else {
		if (stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] > stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0]) //none
			stitch_ctx->tmp_chn_size[0].u32Width = stitch_ctx->chn_attr.size.u32Width;
		else if (stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] == 0) //lft in rht
			stitch_ctx->tmp_chn_size[0].u32Width = stitch_ctx->src_attr.size[1].u32Width;
		else if ((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] + 1) == stitch_ctx->chn_attr.size.u32Width
			&& stitch_ctx->src_attr.size[0].u32Width == stitch_ctx->chn_attr.size.u32Width) //rht in lft
			stitch_ctx->tmp_chn_size[0].u32Width = stitch_ctx->src_attr.size[0].u32Width;
		else
			stitch_ctx->tmp_chn_size[0].u32Width = stitch_ctx->src_attr.size[0].u32Width - stitch_ctx->src_attr.bd_attr.bd_rx[0]
			+ (stitch_ctx->src_attr.size[1].u32Width - (stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] + 1));
	}

	if (stitch_ctx->src_num == STITCH_MAX_SRC_NUM) {
			stitch_ctx->tmp_chn_size[2].u32Width = stitch_ctx->src_attr.size[2].u32Width
			+ (stitch_ctx->src_attr.size[3].u32Width - (stitch_ctx->src_attr.ovlap_attr.ovlp_rx[2] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[2] + 1));
	}
}

s32 stitch_set_chn_attr(STITCH_CHN_ATTR_S *dst_attr)
{
	s32 ret;
	unsigned long flags;
	VB_CAL_CONFIG_S stVbCalConfig;

	ret = STITCH_CHECK_OUTPUT_PARAM(dst_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	if (stitch_ctx->src_num == SRC_NUM_2WAY) {
		if (stitch_ctx->src_attr.size[1].u32Width != (dst_attr->size.u32Width
			- stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] - stitch_ctx->src_attr.bd_attr.bd_rx[1])) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch size[1] with ovlp12 param not match\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	} else if (stitch_ctx->src_num == SRC_NUM_4WAY) {
		if (stitch_ctx->src_attr.size[3].u32Width != (dst_attr->size.u32Width
			- stitch_ctx->src_attr.ovlap_attr.ovlp_lx[2] + stitch_ctx->src_attr.bd_attr.bd_lx[3]
			+ stitch_ctx->src_attr.bd_attr.bd_rx[3])) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch size[3][%d] with ovlp34[%d] param not match\n"
				, stitch_ctx->src_attr.size[3].u32Width, stitch_ctx->src_attr.ovlap_attr.ovlp_lx[2]);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	} else {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "invalid src num (%d)\n", stitch_ctx->src_num);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	COMMON_GetPicBufferConfig(stitch_ctx->chn_attr.size.u32Width, stitch_ctx->chn_attr.size.u32Height
				, stitch_ctx->chn_attr.fmt_out, DATA_BITWIDTH_8
				, COMPRESS_MODE_NONE, STITCH_ALIGN, &stVbCalConfig);

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->param_update = CVI_TRUE;
	stitch_ctx->update_status |= STITCH_UPDATE_CHN;
	stitch_ctx->VbPool = VB_INVALID_POOLID;
	stitch_ctx->u32VBSize = stVbCalConfig.u32VBSize;
	memcpy(&stitch_ctx->chn_attr, dst_attr, sizeof(*dst_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	stitch_cfg_tmp_chn_size();

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "set chn_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_get_chn_attr(STITCH_CHN_ATTR_S *dst_attr)
{
	s32 ret;
	unsigned long flags;

	ret = STITCH_CHECK_NULL_PTR(dst_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	memcpy(dst_attr, &stitch_ctx->chn_attr, sizeof(*dst_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get chn_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_set_op_attr(STITCH_OP_ATTR_S *op_attr)
{
	s32 ret;
	unsigned long flags;

	ret = STITCH_CHECK_OP_PARAM(op_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->param_update = CVI_TRUE;
	stitch_ctx->update_status |= STITCH_UPDATE_OP;
	memcpy(&stitch_ctx->op_attr, op_attr, sizeof(*op_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "set op_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_get_op_attr(STITCH_OP_ATTR_S *op_attr)
{
	s32 ret;
	unsigned long flags;

	ret = STITCH_CHECK_NULL_PTR(op_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	memcpy(op_attr, &stitch_ctx->op_attr, sizeof(*op_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get op_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_set_wgt_attr(STITCH_WGT_ATTR_S *wgt_attr)
{
	s32 ret;
	unsigned long flags;
	u8 i;

	ret = STITCH_CHECK_WGT_PARAM(wgt_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->param_update = CVI_TRUE;
	stitch_ctx->update_status |= STITCH_UPDATE_WGT;
	memcpy(&stitch_ctx->wgt_attr, wgt_attr, sizeof(*wgt_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	for (i = 0; i < STITCH_MAX_SRC_NUM; i++) {
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "set wgt[%d]_attr phy_addr(%llx-%llx), size(%d-%d)\n", i
			, wgt_attr->phy_addr_wgt[i][0], wgt_attr->phy_addr_wgt[i][1]
			, wgt_attr->size_wgt[i].u32Width, wgt_attr->size_wgt[i].u32Height);
		if (stitch_ctx->src_num == 2)
			break;
	}
	return CVI_SUCCESS;
}

s32 stitch_get_wgt_attr(STITCH_WGT_ATTR_S *wgt_attr)
{
	s32 ret;
	unsigned long flags;

	ret = STITCH_CHECK_NULL_PTR(wgt_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	memcpy(wgt_attr, &stitch_ctx->op_attr, sizeof(*wgt_attr));
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "get wgt_attr+\n");
	return CVI_SUCCESS;
}

s32 stitch_set_reg_x(u8 regx)
{
	stitch_set_regx(0, regx);
	stitch_set_regx(1, regx);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "set reg_x(%d)\n", regx);
	return CVI_SUCCESS;
}

s32 stitch_dump_reg_info(void)
{
	stitch_dump_register();

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "dump regs+\n");
	return CVI_SUCCESS;
}

s32 stitch_enable_dev(void)
{
	int i;
	unsigned long flags;

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	if (is_stitch_suspended()) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch is suspend.\n");
		return CVI_ERR_STITCH_NOT_PERM;
	}

	if (stitch_ctx->isStarted) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch ctx is already start.\n");
		return CVI_ERR_STITCH_NOTREADY;
	}

	if (!STITCH_PARAM_IS_CFG()) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "cfg param is not ready.\n");
		return CVI_ERR_STITCH_NOTREADY;
	}

	if (stitch_creat_hw_cfg_obj()) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "creat_hw_cfg_obj fail.\n");
		return CVI_ERR_STITCH_NOMEM;
	}

	if (stitch_creat_chn_vbq())
	{
		CVI_TRACE_STITCH(CVI_DBG_ERR, "creat chn vbq fail.\n");
		return CVI_ERR_STITCH_NOMEM;
	}

	INIT_WORK(&stitch_ctx->work_frm_done, stitch_work_frm_done);

	for (i = 0; i < stitch_ctx->src_num; i++) {
		base_mod_jobs_init(&gStitchJobs.ins[i], 2, 2, 0);
	}
	base_mod_jobs_init(&gStitchJobs.out, 2, 2, 2);

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_START);
	atomic_set(&stitch_ctx->job.enJobState, STITCH_JOB_WAIT);
	stitch_ctx->isStarted = CVI_TRUE;
	stitch_ctx->param_update = CVI_TRUE;
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "enable dev+\n");
	return CVI_SUCCESS;
}

s32 stitch_disable_dev(void)
{
	int i, ret = CVI_SUCCESS;
	unsigned long flags;
	enum stitch_handler_state enHdlState;

	if (!STITCH_CTX_IS_ENBALE()) {
		return CVI_ERR_STITCH_NOTREADY;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	enHdlState = (enum stitch_handler_state)atomic_read(&stitch_ctx->enHdlState);
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	if (enHdlState == STITCH_HANDLER_STATE_RUN || enHdlState == STITCH_HANDLER_STATE_RUN_STAGE2) {
		stitch_ctx->evt = 0;
		ret = wait_event_interruptible_timeout(stitch_ctx->wq, stitch_ctx->evt, msecs_to_jiffies(IDLE_TIMEOUT_MS));
		if (ret <= 0) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "disable dev timeout, out of [%d] ms, ret:%d\n", IDLE_TIMEOUT_MS, ret);
			return CVI_ERR_STITCH_BUSY;
		}
		ret = CVI_SUCCESS;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_STOP);
	atomic_set(&stitch_ctx->job.enJobState, STITCH_JOB_INVALID);
	stitch_ctx->isStarted = CVI_FALSE;
	stitch_ctx->param_update = CVI_FALSE;
	stitch_ctx->update_status = STITCH_UPDATE_SRC | STITCH_UPDATE_CHN | STITCH_UPDATE_OP | STITCH_UPDATE_WGT;
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	base_mod_jobs_exit(&gStitchJobs.out);
	for (i = 0; i < stitch_ctx->src_num; i++) {
		base_mod_jobs_exit(&gStitchJobs.ins[i]);
	}

	cancel_work_sync(&stitch_ctx->work_frm_done);

	stitch_destroy_chn_vbq();

	stitch_destrory_hw_cfg_obj();

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "disable dev+\n");
	return ret;
}

s32 stitch_attach_vb_pool(VB_POOL VbPool)
{
	unsigned long flags;

	if (!stitch_ctx || !stitch_ctx->isCreated) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch ctx is not creat.\n");
		return CVI_ERR_STITCH_NOTREADY;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->VbPool = VbPool;
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "attach vb pool(%d)+\n", VbPool);
	return CVI_SUCCESS;
}

s32 stitch_detach_vb_pool(void)
{
	unsigned long flags;

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	stitch_ctx->VbPool = VB_INVALID_POOLID;
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "dettach vb pool+\n");
	return CVI_SUCCESS;
}

int stitch_thread_init(void)
{
	int ret;
	struct sched_param tsk;

	init_waitqueue_head(&evt_hdl_ctx.wait);
	evt_hdl_ctx.events = 0;
	evt_hdl_ctx.prepared_flag = 0;
	spin_lock_init(&evt_hdl_ctx.lock);

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	evt_hdl_ctx.thread = kthread_run(stitch_event_handler_th, &evt_hdl_ctx, "stitch_event_handler_th");
	if (IS_ERR(evt_hdl_ctx.thread)) {
		pr_err("failed to create stitch kthread\n");
		ret = -1;
		goto kthread_fail;
	}

	ret = sched_setscheduler(evt_hdl_ctx.thread, SCHED_FIFO, &tsk);
	if (ret)
		pr_warn("stitch thread priority update failed: %d\n", ret);

	evt_hdl_ctx.workqueue = create_singlethread_workqueue("stitch_workqueue");
	if (!evt_hdl_ctx.workqueue) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch evt_hdl_ctx create_workqueue failed.\n");
		ret = -2;
		goto workqueue_creat_fail;
	}

	ret = stitch_init();
	if (ret)
		pr_err("stitch ctx creat failed: %d\n", ret);

kthread_fail:
workqueue_creat_fail:
	return ret;
}

void stitch_thread_deinit(void)
{
	int ret;

	if (evt_hdl_ctx.workqueue)
		destroy_workqueue(evt_hdl_ctx.workqueue);

	if (!evt_hdl_ctx.thread) {
		pr_err("stitch thread not initialized yet\n");
		return;
	}

	ret = kthread_stop(evt_hdl_ctx.thread);
	if (ret)
		pr_err("fail to stop stitch thread, err=%d\n", ret);

	memset(&evt_hdl_ctx, 0, sizeof(evt_hdl_ctx));
}

s32 stitch_suspend_handler(void)
{
	enum stitch_handler_state hdlState;
	int ret;
	unsigned long flags;

	if (!STITCH_CTX_IS_VALID(stitch_ctx))
		stitch_init();

	if (!evt_hdl_ctx.thread) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch thread not initialized yet\n");
		return CVI_ERR_STITCH_NOTREADY;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	hdlState = (enum stitch_handler_state)atomic_read(&stitch_ctx->enHdlState);
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	if (hdlState == STITCH_HANDLER_STATE_RUN || hdlState == STITCH_HANDLER_STATE_RUN_STAGE2) {
		stitch_ctx->evt = 0;
		ret = wait_event_interruptible_timeout(stitch_ctx->wq, stitch_ctx->evt, msecs_to_jiffies(IDLE_TIMEOUT_MS));
		if (ret <= 0) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "suspend dev timeout, out of [%d] ms, ret:%d\n", IDLE_TIMEOUT_MS, ret);
			return CVI_ERR_STITCH_BUSY;
		}
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_SUSPEND);
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "suspend handler+\n");
	return CVI_SUCCESS;
}

s32 stitch_resume_handler(void)
{
	unsigned long flags;

	if (!STITCH_CTX_IS_VALID(stitch_ctx))
		stitch_init();

	if (!evt_hdl_ctx.thread) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch thread not initialized yet\n");
		return CVI_ERR_STITCH_NOTREADY;
	}

	spin_lock_irqsave(&stitch_ctx->lock, flags);
	atomic_set(&stitch_ctx->enHdlState, STITCH_HANDLER_STATE_RESUME);
	spin_unlock_irqrestore(&stitch_ctx->lock, flags);

	CVI_TRACE_STITCH(CVI_DBG_DEBUG, "resume handler+\n");
	return CVI_SUCCESS;
}
