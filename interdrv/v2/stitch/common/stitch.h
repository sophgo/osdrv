#ifndef _STITCH_H_
#define _STITCH_H_

#include <common.h>
#include <comm_video.h>
#include <comm_sys.h>
#include <comm_stitch.h>
#include <comm_region.h>
#include <comm_errno.h>
#include <comm_math.h>
#include <stitch_uapi.h>

#include <stitch_ctx.h>
#include <base_ctx.h>
#include "stitch_debug.h"

extern struct __stitch_ctx *stitch_ctx;
extern struct stitch_handler_ctx evt_hdl_ctx;

/* Configured from user, IOCTL */
int stitch_init(void);
int stitch_deinit(void);
int stitch_reset(void);
int stitch_set_src_attr(const stitch_src_attr *src_attr);
int stitch_get_src_attr(stitch_src_attr *src_attr);
int stitch_set_chn_attr(stitch_chn_attr *dst_attr);
int stitch_get_chn_attr(stitch_chn_attr *dst_attr);
int stitch_set_op_attr(stitch_op_attr *op_attr);
int stitch_get_op_attr(stitch_op_attr *op_attr);
int stitch_set_wgt_attr(stitch_bld_wgt_attr *wgt_attr);
int stitch_get_wgt_attr(stitch_bld_wgt_attr *wgt_attr);
int stitch_set_reg_x(unsigned char regx);
int stitch_dump_reg_info(void);
int stitch_enable_dev(void);
int stitch_disable_dev(void);
void stitch_post_job(int src_id);
int stitch_send_frame(stitch_src_idx src_idx, video_frame_info_s *pstvideo_frame, int s32milli_sec);
int stitch_send_chn_frame(video_frame_info_s *pstvideo_frame, int s32milli_sec);
int stitch_get_chn_frame(video_frame_info_s *pstvideo_frame, int s32milli_sec);
int stitch_release_chn_frame(const video_frame_info_s *pstvideo_frame);
int stitch_attach_vb_pool(vb_pool vb_pool);
int stitch_detach_vb_pool(void);

/*internal*/
int stitch_thread_init(void);
void stitch_thread_deinit(void);
int stitch_suspend_handler(void);
int stitch_resume_handler(void);

struct __stitch_ctx *stitch_get_ctx(void);

static inline int stitch_support_fmt(pixel_format_e fmt)
{
	if ((fmt != PIXEL_FORMAT_RGB_888_PLANAR) && (fmt != PIXEL_FORMAT_BGR_888_PLANAR) &&	\
		(fmt != PIXEL_FORMAT_YUV_PLANAR_420) && (fmt != PIXEL_FORMAT_YUV_PLANAR_422) &&	\
		(fmt != PIXEL_FORMAT_YUV_PLANAR_444) && (fmt != PIXEL_FORMAT_YUV_400)) {
			TRACE_STITCH(DBG_ERR, "Stitch not support fmt(%d)\n", fmt);
			return ERR_STITCH_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int stitch_check_null_ptr(const void *ptr)
{
	if (!ptr) {
		TRACE_STITCH(DBG_ERR, "NULL pointer\n");
		return ERR_STITCH_NULL_PTR;
	}
	return 0;
}

static inline unsigned char stitch_ctx_is_creat(void)
{
	if (!stitch_ctx || !stitch_ctx->is_created) {
		TRACE_STITCH(DBG_ERR, "ctx dev not yet creat.\n");
		return false;
	}

	return true;
}

static inline unsigned char stitch_ctx_is_valid(struct __stitch_ctx *p_stitch_ctx)
{
	if (!p_stitch_ctx) {
		TRACE_STITCH(DBG_ERR, "dev isn't created yet.\n");
		return false;
	}

	if (p_stitch_ctx != stitch_ctx) {
		TRACE_STITCH(DBG_ERR, "dev ctx is invalid.\n");
		return false;
	}

	return true;
}

static inline unsigned char stitch_ctx_is_enbale(void)
{
	if (!stitch_ctx_is_creat())
		return false;

	if (!stitch_ctx->is_started) {
		TRACE_STITCH(DBG_ERR, "chn not yet started.\n");
		return false;
	}

	return true;
}

static inline int stitch_check_yuv_param(pixel_format_e fmt, unsigned int w, unsigned int h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w % 0x1) {
			TRACE_STITCH(DBG_ERR, "YUV_422 width(%d) should be even\n", w);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)) {
		if (w & 0x1) {
			TRACE_STITCH(DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (h & 0x1) {
			TRACE_STITCH(DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline int stitch_check_src_size(const stitch_src_attr *src_attr)
{
	if (src_attr->size[0].width != src_attr->ovlap_attr.ovlp_rx[0] + src_attr->bd_attr.bd_lx[0] + src_attr->bd_attr.bd_rx[0] +1) {
		TRACE_STITCH(DBG_ERR, "stitch size[0] %d with ovlp12 [%d] param not match\n", src_attr->size[0].width, src_attr->ovlap_attr.ovlp_rx[0]);
		return ERR_STITCH_ILLEGAL_PARAM;
	}
	if (src_attr->size[0].height != src_attr->size[1].height) {
		TRACE_STITCH(DBG_ERR, "size[0].height(%d) != size[1].height(%d)\n", src_attr->size[0].height, src_attr->size[1].height);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (src_attr->way_num == STITCH_4_WAY) {
		if (src_attr->size[1].width != src_attr->ovlap_attr.ovlp_rx[1] - src_attr->ovlap_attr.ovlp_lx[0] + src_attr->bd_attr.bd_lx[1] + src_attr->bd_attr.bd_rx[1] +1) {
			TRACE_STITCH(DBG_ERR, "stitch size[1] with ovlp12/ovlp23 param not match\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (src_attr->size[2].width != src_attr->ovlap_attr.ovlp_rx[2] - src_attr->ovlap_attr.ovlp_lx[1] + src_attr->bd_attr.bd_lx[2] + src_attr->bd_attr.bd_lx[2] +1) {
			TRACE_STITCH(DBG_ERR, "stitch size[2] with ovlp34/ovlp23 param not match\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (src_attr->size[0].height != src_attr->size[1].height || src_attr->size[2].height != src_attr->size[3].height) {
			TRACE_STITCH(DBG_ERR, "height param not match\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline int stitch_check_input_param(const stitch_src_attr *src_attr)
{
	int ret = 0;

	ret = stitch_check_null_ptr(src_attr);
	if (ret != 0)
		return ret;

	ret = stitch_support_fmt(src_attr->fmt_in);
	if (ret != 0)
		return ret;

	if (src_attr->way_num < STITCH_2_WAY || src_attr->way_num >= STITCH_WAY_SEP) {
		TRACE_STITCH(DBG_ERR, "stitch way_num(%d) invalid\n", src_attr->way_num);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	ret = stitch_check_src_size(src_attr);

	return ret;
}

static inline int stitch_check_input_video_param(stitch_src_idx src_idx, const video_frame_info_s *pstvideo_frame)
{
	int ret = 0;

	ret = stitch_check_null_ptr(pstvideo_frame);
	if (ret != 0)
		return ret;

	ret = stitch_support_fmt(pstvideo_frame->video_frame.pixel_format);
	if (ret != 0)
		return ret;

	ret = stitch_check_yuv_param(pstvideo_frame->video_frame.pixel_format, pstvideo_frame->video_frame.width, pstvideo_frame->video_frame.height);
	if (ret != 0)
		return ret;

	if (IS_FRAME_OFFSET_INVALID(pstvideo_frame->video_frame)) {
		TRACE_STITCH(DBG_ERR, "src(%d) frame offset (%d %d %d %d) invalid\n"
			, src_idx, pstvideo_frame->video_frame.offset_left, pstvideo_frame->video_frame.offset_right
			, pstvideo_frame->video_frame.offset_top, pstvideo_frame->video_frame.offset_bottom);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (!stitch_ctx_is_creat())
		return ERR_STITCH_NOTREADY;

	if (stitch_ctx->src_attr.fmt_in != pstvideo_frame->video_frame.pixel_format) {
		TRACE_STITCH(DBG_ERR, "fmt (%d) not match with ctx fmt(%d)\n", pstvideo_frame->video_frame.pixel_format, stitch_ctx->src_attr.fmt_in);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (pstvideo_frame->video_frame.width != stitch_ctx->src_attr.size[src_idx].width ||
		pstvideo_frame->video_frame.height != stitch_ctx->src_attr.size[src_idx].height) {
		TRACE_STITCH(DBG_ERR, "src_id(%d), size (%d, %d) not match with ctx(%d, %d)\n"
			, src_idx, pstvideo_frame->video_frame.width, pstvideo_frame->video_frame.height
			, stitch_ctx->src_attr.size[src_idx].width, stitch_ctx->src_attr.size[src_idx].height);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline int stitch_check_output_video_param(const video_frame_info_s *pstvideo_frame)
{
	int ret = 0;

	ret = stitch_check_null_ptr(pstvideo_frame);
	if (ret != 0)
		return ret;

	ret = stitch_support_fmt(pstvideo_frame->video_frame.pixel_format);
	if (ret != 0)
		return ret;

	ret = stitch_check_yuv_param(pstvideo_frame->video_frame.pixel_format, pstvideo_frame->video_frame.width, pstvideo_frame->video_frame.height);
	if (ret != 0)
		return ret;

	if (IS_FRAME_OFFSET_INVALID(pstvideo_frame->video_frame)) {
		TRACE_STITCH(DBG_ERR, "frame offset (%d %d %d %d) invalid\n"
			, pstvideo_frame->video_frame.offset_left, pstvideo_frame->video_frame.offset_right
			, pstvideo_frame->video_frame.offset_top, pstvideo_frame->video_frame.offset_bottom);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (!stitch_ctx_is_creat())
		return ERR_STITCH_NOTREADY;

	if (stitch_ctx->chn_attr.fmt_out != pstvideo_frame->video_frame.pixel_format) {
		TRACE_STITCH(DBG_ERR, "fmt (%d) not match with ctx fmt(%d)\n", pstvideo_frame->video_frame.pixel_format, stitch_ctx->chn_attr.fmt_out);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (pstvideo_frame->video_frame.width != stitch_ctx->chn_attr.size.width ||
		pstvideo_frame->video_frame.height != stitch_ctx->chn_attr.size.height) {
		TRACE_STITCH(DBG_ERR, "size (%d, %d) not match with ctx(%d, %d)\n"
			, pstvideo_frame->video_frame.width, pstvideo_frame->video_frame.height
			, stitch_ctx->chn_attr.size.width, stitch_ctx->chn_attr.size.height);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline int stitch_check_output_param(const stitch_chn_attr *attr)
{
	int ret = 0;

	ret = stitch_check_null_ptr(attr);
	if (ret != 0)
		return ret;

	ret = stitch_support_fmt(attr->fmt_out);
	if (ret != 0)
		return ret;

	ret = stitch_check_yuv_param(attr->fmt_out, attr->size.width, attr->size.height);
	if (ret != 0)
		return ret;

	if (attr->size.height != stitch_ctx->src_attr.size[0].height) {
		TRACE_STITCH(DBG_ERR, "height param not match\n");
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline int stitch_check_op_param(const stitch_op_attr *attr)
{
	int ret = 0;

	ret = stitch_check_null_ptr(attr);
	if (ret != 0)
		return ret;

	if (attr->wgt_mode < STITCH_WGT_YUV_SHARE || attr->wgt_mode >= STITCH_WGT_SEP) {
		TRACE_STITCH(DBG_ERR, "stitch wgt_mode(%d) invalid\n", attr->wgt_mode);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (attr->data_src < STITCH_DATA_SRC_DDR || attr->data_src >= STITCH_DATA_SRC_SEP) {
		TRACE_STITCH(DBG_ERR, "stitch data_src(%d) invalid\n", attr->data_src);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	return ret;
}

static inline int stitch_check_wgt_param(const stitch_bld_wgt_attr *attr)
{
	if (stitch_check_null_ptr(attr))
		return ERR_STITCH_NULL_PTR;

	if (stitch_ctx->src_attr.way_num < STITCH_2_WAY || stitch_ctx->src_attr.way_num >= STITCH_WAY_SEP) {
		TRACE_STITCH(DBG_ERR, "invalid way_num(%d), need set way num first\n", stitch_ctx->src_attr.way_num);
		return ERR_STITCH_ILLEGAL_PARAM;
	}

	if (stitch_ctx->src_attr.way_num == STITCH_2_WAY) {
		if (attr->size_wgt[0].width) {
			if (!attr->phy_addr_wgt[0][0] || !attr->phy_addr_wgt[0][1]) {
				TRACE_STITCH(DBG_ERR, "wgt NULL phy addr\n");
				return ERR_STITCH_ILLEGAL_PARAM;
			}
		}
		if (!IS_ALIGNED(attr->size_wgt[0].width, STITCH_ALIGN)) {
			TRACE_STITCH(DBG_ERR, "wgt12.width %d not %d align\n", attr->size_wgt[0].width, STITCH_ALIGN);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		TRACE_STITCH(DBG_DEBUG, "wgt12.width(%d), ovlp12_rx(%d), ovlp12_lx(%d)\n"
			, attr->size_wgt[0].width, stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0], stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0]);

		if (attr->size_wgt[0].width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] + 1), STITCH_ALIGN)) {
			TRACE_STITCH(DBG_ERR, "wgt12.width(%d) not match, ovlp12_rx(%d), ovlp12_lx(%d)\n"
				, attr->size_wgt[0].width, stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0], stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0]);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].height != stitch_ctx->src_attr.size[0].height) {
			TRACE_STITCH(DBG_ERR, "wgt12.height(%d) not match, src height(%d)\n", attr->size_wgt[0].height, stitch_ctx->src_attr.size[0].height);
			return ERR_STITCH_ILLEGAL_PARAM;
		}

	} else {
		if (!attr->phy_addr_wgt[0][0] || !attr->phy_addr_wgt[0][1] ||
			!attr->phy_addr_wgt[1][0] || !attr->phy_addr_wgt[1][1] ||
			!attr->phy_addr_wgt[2][0] || !attr->phy_addr_wgt[2][1]) {
			TRACE_STITCH(DBG_ERR, "wgt NULL phy addr\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (!IS_ALIGNED(attr->size_wgt[0].width, STITCH_ALIGN) ||
			!IS_ALIGNED(attr->size_wgt[1].width, STITCH_ALIGN) ||
			!IS_ALIGNED(attr->size_wgt[2].width, STITCH_ALIGN)) {
			TRACE_STITCH(DBG_ERR, "wgt width not %d align\n", STITCH_ALIGN);
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[0] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[0] + 1), STITCH_ALIGN) ||
			attr->size_wgt[1].width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[1] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[1] + 1), STITCH_ALIGN) ||
			attr->size_wgt[2].width != ALIGN((stitch_ctx->src_attr.ovlap_attr.ovlp_rx[2] - stitch_ctx->src_attr.ovlap_attr.ovlp_lx[2] + 1), STITCH_ALIGN)) {
			TRACE_STITCH(DBG_ERR, "wgt width not match with ovlp\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].height != attr->size_wgt[1].height || attr->size_wgt[1].height != attr->size_wgt[2].height) {
			TRACE_STITCH(DBG_ERR, "wgt height not match\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
		if (attr->size_wgt[0].height != stitch_ctx->src_attr.size[0].height ||
			attr->size_wgt[1].height != stitch_ctx->src_attr.size[1].height ||
			attr->size_wgt[2].height != stitch_ctx->src_attr.size[2].height) {
			TRACE_STITCH(DBG_ERR, "wgt height not match witch src height\n");
			return ERR_STITCH_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline unsigned char stitch_param_is_cfg(void)
{
	if ((stitch_ctx->update_status & STITCH_UPDATE_SRC) ||
		(stitch_ctx->update_status & STITCH_UPDATE_CHN) ||
		(stitch_ctx->update_status & STITCH_UPDATE_OP) ||
		(stitch_ctx->update_status & STITCH_UPDATE_WGT))
		return true;

	return false;
}

#endif /* _STITCH_H_ */
