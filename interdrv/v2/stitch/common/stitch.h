#ifndef _STITCH_H_
#define _STITCH_H_

#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_comm_stitch.h>
#include <linux/cvi_comm_region.h>
#include <linux/cvi_errno.h>
#include <linux/cvi_math.h>
#include <linux/stitch_uapi.h>

#include <stitch_ctx.h>
#include <base_ctx.h>
#include "stitch_debug.h"

extern struct cvi_stitch_ctx *stitch_ctx;
extern struct stitch_handler_ctx evt_hdl_ctx;

/* Configured from user, IOCTL */
s32 stitch_init(void);
s32 stitch_deinit(void);
s32 stitch_reset(void);
s32 stitch_set_src_attr(const STITCH_SRC_ATTR_S *src_attr);
s32 stitch_get_src_attr(STITCH_SRC_ATTR_S *src_attr);
s32 stitch_set_chn_attr(STITCH_CHN_ATTR_S *dst_attr);
s32 stitch_get_chn_attr(STITCH_CHN_ATTR_S *dst_attr);
s32 stitch_set_op_attr(STITCH_OP_ATTR_S *op_attr);
s32 stitch_get_op_attr(STITCH_OP_ATTR_S *op_attr);
s32 stitch_set_wgt_attr(STITCH_WGT_ATTR_S *wgt_attr);
s32 stitch_get_wgt_attr(STITCH_WGT_ATTR_S *wgt_attr);
s32 stitch_set_reg_x(u8 regx);
s32 stitch_dump_reg_info(void);
s32 stitch_enable_dev(void);
s32 stitch_disable_dev(void);
void stitch_post_job(s32 src_id);
s32 stitch_send_frame(STITCH_SRC_IDX src_idx, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);
s32 stitch_send_chn_frame(VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);
s32 stitch_get_chn_frame(VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);
s32 stitch_release_chn_frame(const VIDEO_FRAME_INFO_S *pstVideoFrame);
s32 stitch_attach_vb_pool(VB_POOL VbPool);
s32 stitch_detach_vb_pool(void);

/*internal*/
int stitch_thread_init(void);
void stitch_thread_deinit(void);
s32 stitch_suspend_handler(void);
s32 stitch_resume_handler(void);

struct cvi_stitch_ctx *stitch_get_ctx(void);

static inline s32 STITCH_SUPPORT_FMT(PIXEL_FORMAT_E fmt)
{
	if ((fmt != PIXEL_FORMAT_RGB_888_PLANAR) && (fmt != PIXEL_FORMAT_BGR_888_PLANAR) &&	\
		(fmt != PIXEL_FORMAT_YUV_PLANAR_420) && (fmt != PIXEL_FORMAT_YUV_PLANAR_422) &&	\
		(fmt != PIXEL_FORMAT_YUV_PLANAR_444) && (fmt != PIXEL_FORMAT_YUV_400)) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "Stitch not support fmt(%d)\n", fmt);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 STITCH_CHECK_NULL_PTR(const void *ptr)
{
	if (!ptr) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_STITCH_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline u8 STITCH_CTX_IS_CREAT(void)
{
	if (!stitch_ctx || !stitch_ctx->isCreated) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "ctx dev not yet creat.\n");
		return CVI_FALSE;
	}

	return CVI_TRUE;
}

static inline u8 STITCH_CTX_IS_VALID(struct cvi_stitch_ctx *p_stitch_ctx)
{
	if (!p_stitch_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "dev isn't created yet.\n");
		return CVI_FALSE;
	}

	if (p_stitch_ctx != stitch_ctx) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "dev ctx is invalid.\n");
		return CVI_FALSE;
	}

	return CVI_TRUE;
}

static inline u8 STITCH_CTX_IS_ENBALE(void)
{
	if (!STITCH_CTX_IS_CREAT())
		return CVI_FALSE;

	if (!stitch_ctx->isStarted) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "chn not yet started.\n");
		return CVI_FALSE;
	}

	return CVI_TRUE;
}

static inline s32 STITCH_CHECK_YUV_PARAM(PIXEL_FORMAT_E fmt, u32 w, u32 h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w % 0x1) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "YUV_422 width(%d) should be even\n", w);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)) {
		if (w & 0x1) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (h & 0x1) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline s32 STITCH_CHECK_SRC_SIZE(const STITCH_SRC_ATTR_S *src_attr)
{
	if (src_attr->size[0].u32Width != src_attr->ovlap_attr.ovlp_rx[0] + src_attr->bd_attr.bd_lx[0] + src_attr->bd_attr.bd_rx[0] +1) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch size[0] %d with ovlp12 [%d] param not match\n", src_attr->size[0].u32Width, src_attr->ovlap_attr.ovlp_rx[0]);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}
	if (src_attr->size[0].u32Height != src_attr->size[1].u32Height) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "size[0].u32Height(%d) != size[1].u32Height(%d)\n", src_attr->size[0].u32Height, src_attr->size[1].u32Height);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (src_attr->way_num == STITCH_4_WAY) {
		if (src_attr->size[1].u32Width != src_attr->ovlap_attr.ovlp_rx[1] - src_attr->ovlap_attr.ovlp_lx[0] + src_attr->bd_attr.bd_lx[1] + src_attr->bd_attr.bd_rx[1] +1) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch size[1] with ovlp12/ovlp23 param not match\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (src_attr->size[2].u32Width != src_attr->ovlap_attr.ovlp_rx[2] - src_attr->ovlap_attr.ovlp_lx[1] + src_attr->bd_attr.bd_lx[2] + src_attr->bd_attr.bd_lx[2] +1) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch size[2] with ovlp34/ovlp23 param not match\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (src_attr->size[0].u32Height != src_attr->size[1].u32Height || src_attr->size[2].u32Height != src_attr->size[3].u32Height) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "u32Height param not match\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline s32 STITCH_CHECK_INPUT_PARAM(const STITCH_SRC_ATTR_S *src_attr)
{
	s32 ret = CVI_SUCCESS;

	ret = STITCH_CHECK_NULL_PTR(src_attr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_SUPPORT_FMT(src_attr->fmt_in);
	if (ret != CVI_SUCCESS)
		return ret;

	if (src_attr->way_num < STITCH_2_WAY || src_attr->way_num >= STITCH_WAY_SEP) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch way_num(%d) invalid\n", src_attr->way_num);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	ret = STITCH_CHECK_SRC_SIZE(src_attr);

	return ret;
}

static inline s32 STITCH_CHECK_INPUT_VIDEO_PARAM(STITCH_SRC_IDX src_idx, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	s32 ret = CVI_SUCCESS;

	ret = STITCH_CHECK_NULL_PTR(pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_SUPPORT_FMT(pstVideoFrame->stVFrame.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_CHECK_YUV_PARAM(pstVideoFrame->stVFrame.enPixelFormat, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
	if (ret != CVI_SUCCESS)
		return ret;

	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src(%d) frame offset (%d %d %d %d) invalid\n"
			, src_idx, pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight
			, pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	if (stitch_ctx->src_attr.fmt_in != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "fmt (%d) not match with ctx fmt(%d)\n", pstVideoFrame->stVFrame.enPixelFormat, stitch_ctx->src_attr.fmt_in);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (pstVideoFrame->stVFrame.u32Width != stitch_ctx->src_attr.size[src_idx].u32Width ||
		pstVideoFrame->stVFrame.u32Height != stitch_ctx->src_attr.size[src_idx].u32Height) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "src_id(%d), size (%d, %d) not match with ctx(%d, %d)\n"
			, src_idx, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height
			, stitch_ctx->src_attr.size[src_idx].u32Width, stitch_ctx->src_attr.size[src_idx].u32Height);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline s32 STITCH_CHECK_OUTPUT_VIDEO_PARAM(const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	s32 ret = CVI_SUCCESS;

	ret = STITCH_CHECK_NULL_PTR(pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_SUPPORT_FMT(pstVideoFrame->stVFrame.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_CHECK_YUV_PARAM(pstVideoFrame->stVFrame.enPixelFormat, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
	if (ret != CVI_SUCCESS)
		return ret;

	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "frame offset (%d %d %d %d) invalid\n"
			, pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight
			, pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (!STITCH_CTX_IS_CREAT())
		return CVI_ERR_STITCH_NOTREADY;

	if (stitch_ctx->chn_attr.fmt_out != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "fmt (%d) not match with ctx fmt(%d)\n", pstVideoFrame->stVFrame.enPixelFormat, stitch_ctx->chn_attr.fmt_out);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (pstVideoFrame->stVFrame.u32Width != stitch_ctx->chn_attr.size.u32Width ||
		pstVideoFrame->stVFrame.u32Height != stitch_ctx->chn_attr.size.u32Height) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "size (%d, %d) not match with ctx(%d, %d)\n"
			, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height
			, stitch_ctx->chn_attr.size.u32Width, stitch_ctx->chn_attr.size.u32Height);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline s32 STITCH_CHECK_OUTPUT_PARAM(const STITCH_CHN_ATTR_S *attr)
{
	s32 ret = CVI_SUCCESS;

	ret = STITCH_CHECK_NULL_PTR(attr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_SUPPORT_FMT(attr->fmt_out);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = STITCH_CHECK_YUV_PARAM(attr->fmt_out, attr->size.u32Width, attr->size.u32Height);
	if (ret != CVI_SUCCESS)
		return ret;

	if (attr->size.u32Height != stitch_ctx->src_attr.size[0].u32Height) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "u32Height param not match\n");
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline s32 STITCH_CHECK_OP_PARAM(const STITCH_OP_ATTR_S *attr)
{
	s32 ret = CVI_SUCCESS;

	ret = STITCH_CHECK_NULL_PTR(attr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (attr->wgt_mode < STITCH_WGT_YUV_SHARE || attr->wgt_mode >= STITCH_WGT_SEP) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch wgt_mode(%d) invalid\n", attr->wgt_mode);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (attr->data_src < STITCH_DATA_SRC_DDR || attr->data_src >= STITCH_DATA_SRC_SEP) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "stitch data_src(%d) invalid\n", attr->data_src);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline s32 STITCH_CHECK_WGT_PARAM(const STITCH_WGT_ATTR_S *attr)
{
	if (STITCH_CHECK_NULL_PTR(attr))
		return CVI_ERR_STITCH_NULL_PTR;

	if (stitch_ctx->src_attr.way_num < STITCH_2_WAY || stitch_ctx->src_attr.way_num >= STITCH_WAY_SEP) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "invalid way_num(%d), need set way num first\n", stitch_ctx->src_attr.way_num);
		return CVI_ERR_STITCH_ILLEGAL_PARAM;
	}

	if (stitch_ctx->src_attr.way_num == STITCH_2_WAY) {
		if (attr->size_wgt[0].u32Width) {
			if (!attr->phy_addr_wgt[0][0] || !attr->phy_addr_wgt[0][1]) {
				CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt NULL phy addr\n");
				return CVI_ERR_STITCH_ILLEGAL_PARAM;
			}
		}
		if (!IS_ALIGNED(attr->size_wgt[0].u32Width, STITCH_ALIGN)) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt12.u32Width %d not %d align\n", attr->size_wgt[0].u32Width, STITCH_ALIGN);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		CVI_TRACE_STITCH(CVI_DBG_DEBUG, "wgt12.u32Width(%d), ovlp12_rx(%d), ovlp12_lx(%d)\n"
			, attr->size_wgt[0].u32Width, stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0], stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0]);

		if (attr->size_wgt[0].u32Width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] + 1), STITCH_ALIGN)) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt12.u32Width(%d) not match, ovlp12_rx(%d), ovlp12_lx(%d)\n"
				, attr->size_wgt[0].u32Width, stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0], stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0]);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].u32Height != stitch_ctx->src_attr.size[0].u32Height) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt12.u32Height(%d) not match, src u32Height(%d)\n", attr->size_wgt[0].u32Height, stitch_ctx->src_attr.size[0].u32Height);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}

	} else {
		if (!attr->phy_addr_wgt[0][0] || !attr->phy_addr_wgt[0][1] ||
			!attr->phy_addr_wgt[1][0] || !attr->phy_addr_wgt[1][1] ||
			!attr->phy_addr_wgt[2][0] || !attr->phy_addr_wgt[2][1]) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt NULL phy addr\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (!IS_ALIGNED(attr->size_wgt[0].u32Width, STITCH_ALIGN) ||
			!IS_ALIGNED(attr->size_wgt[1].u32Width, STITCH_ALIGN) ||
			!IS_ALIGNED(attr->size_wgt[2].u32Width, STITCH_ALIGN)) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt u32Width not %d align\n", STITCH_ALIGN);
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].u32Width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] + 1), STITCH_ALIGN) ||
			attr->size_wgt[1].u32Width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[1] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[1] + 1), STITCH_ALIGN) ||
			attr->size_wgt[2].u32Width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[2] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[2] + 1), STITCH_ALIGN)) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt u32Width not match with ovlp\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].u32Height != attr->size_wgt[1].u32Height || attr->size_wgt[1].u32Height != attr->size_wgt[2].u32Height) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt u32Height not match\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].u32Height != stitch_ctx->src_attr.size[0].u32Height ||
			attr->size_wgt[1].u32Height != stitch_ctx->src_attr.size[1].u32Height ||
			attr->size_wgt[2].u32Height != stitch_ctx->src_attr.size[2].u32Height) {
			CVI_TRACE_STITCH(CVI_DBG_ERR, "wgt u32Height not match witch src u32Height\n");
			return CVI_ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline u8 STITCH_PARAM_IS_CFG(void)
{
	if ((stitch_ctx->update_status & STITCH_UPDATE_SRC) ||
		(stitch_ctx->update_status & STITCH_UPDATE_CHN) ||
		(stitch_ctx->update_status & STITCH_UPDATE_OP) ||
		(stitch_ctx->update_status & STITCH_UPDATE_WGT))
		return CVI_TRUE;

	return CVI_FALSE;
}

#endif /* _STITCH_H_ */
