#ifndef _VPSS_H_
#define _VPSS_H_

#include "comm_errno.h"
#include "comm_video.h"
#include "comm_vb.h"
#include "base_ctx.h"
#include "vpss_ctx.h"


//Check GRP and CHN VALID, CREATED and FMT
#define VPSS_GRP_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY) ||				\
	 (fmt == PIXEL_FORMAT_YUV_444))

#define VPSS_CHN_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_RGB_888_PLANAR) || (fmt == PIXEL_FORMAT_BGR_888_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_RGB_888) || (fmt == PIXEL_FORMAT_BGR_888) ||			\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_422) ||	\
	 (fmt == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt == PIXEL_FORMAT_YUV_400) ||		\
	 (fmt == PIXEL_FORMAT_HSV_888) || (fmt == PIXEL_FORMAT_HSV_888_PLANAR) ||		\
	 (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_NV21) ||				\
	 (fmt == PIXEL_FORMAT_NV16) || (fmt == PIXEL_FORMAT_NV61) ||				\
	 (fmt == PIXEL_FORMAT_YUYV) || (fmt == PIXEL_FORMAT_UYVY) ||				\
	 (fmt == PIXEL_FORMAT_YVYU) || (fmt == PIXEL_FORMAT_VYUY) ||				\
	 (fmt == PIXEL_FORMAT_YUV_444) ||							\
	 (fmt == PIXEL_FORMAT_FP32_C3_PLANAR) || (fmt == PIXEL_FORMAT_FP16_C3_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_BF16_C3_PLANAR) || (fmt == PIXEL_FORMAT_INT8_C3_PLANAR) ||	\
	 (fmt == PIXEL_FORMAT_UINT8_C3_PLANAR))

#define VPSS_UPSAMPLE(fmt_grp, fmt_chn) \
	((IS_FMT_YUV420(fmt_grp) \
	&& (IS_FMT_YUV422(fmt_chn) || IS_FMT_RGB(fmt_chn) || \
	(fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))) \
	|| (IS_FMT_YUV422(fmt_grp) && (IS_FMT_RGB(fmt_chn) || \
	(fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))))

#define FRC_INVALID(frame_rate)	\
	(frame_rate.dst_frame_rate <= 0 || frame_rate.src_frame_rate <= 0 ||	\
		frame_rate.dst_frame_rate >= frame_rate.src_frame_rate)


static inline int check_vpss_grp_valid(vpss_grp grp)
{
	if ((grp >= VPSS_MAX_GRP_NUM) || (grp < 0)) {
		TRACE_VPSS(DBG_ERR, "vpss_grp(%d) exceeds Max(%d)\n", grp, VPSS_MAX_GRP_NUM);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int check_vpss_chn_valid(vpss_grp grp_id, vpss_chn chn_id, u8 chn_max_num)
{
	if ((chn_id >= chn_max_num) || (chn_id < 0)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid channel ID, range[0, %d].\n",
					grp_id, chn_id, chn_max_num - 1);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int check_vpss_grp_created(vpss_grp grp, struct vpss_ctx *ctx)
{
	if (!ctx->grp_ctx[grp] || !ctx->grp_ctx[grp]->is_created) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return ERR_VPSS_UNEXIST;
	}
	return 0;
}

static inline int check_yuv_param(pixel_format_e fmt, u32 w, u32 h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_422 width(%d) should be even.\n", w);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)
		   || (fmt == PIXEL_FORMAT_NV12)
		   || (fmt == PIXEL_FORMAT_NV21)) {
		if (w & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if (h & 0x01) {
			TRACE_VPSS(DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline int check_vpss_grp_size(u32 w, u32 h)
{
	if ((w < VPSS_MIN_IMAGE_WIDTH) || (w > VPSS_MAX_IMAGE_WIDTH)) {
		TRACE_VPSS(DBG_ERR, "group width(%d) out of range[%d, %d]\n",
			w, VPSS_MIN_IMAGE_WIDTH, VPSS_MAX_IMAGE_WIDTH);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if ((h < VPSS_MIN_IMAGE_HEIGHT) || (h > VPSS_MAX_IMAGE_HEIGHT)) {
		TRACE_VPSS(DBG_ERR, "group height(%d) out of range[%d, %d]\n",
			h, VPSS_MIN_IMAGE_HEIGHT, VPSS_MAX_IMAGE_HEIGHT);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	return 0;
}

static inline int check_vpss_grp_fmt(vpss_grp grp, pixel_format_e fmt)
{
	if (!VPSS_GRP_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) enPixelFormat(%d) unsupported\n"
		, grp, fmt);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int check_vpss_chn_size(vpss_chn chn, u8 dev_id, u32 w, u32 h)
{
	if ((w < VPSS_MIN_IMAGE_WIDTH) || (w > VPSS_MAX_IMAGE_WIDTH)) {
		TRACE_VPSS(DBG_ERR, "chn width(%d) out of range[%d, %d]\n",
			w, VPSS_MIN_IMAGE_WIDTH, VPSS_MAX_IMAGE_WIDTH);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if ((h < VPSS_MIN_IMAGE_HEIGHT) || (h > VPSS_MAX_IMAGE_HEIGHT)) {
		TRACE_VPSS(DBG_ERR, "chn height(%d) out of range[%d, %d]\n",
			h, VPSS_MIN_IMAGE_HEIGHT, VPSS_MAX_IMAGE_HEIGHT);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	return 0;
}

static inline int check_vpss_chn_fmt(vpss_grp grp, vpss_chn chn, pixel_format_e fmt)
{
	if (!VPSS_CHN_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) enPixelFormat(%d) unsupported\n"
		, grp, chn, fmt);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int check_vpss_gdc_fmt(vpss_grp grp, vpss_chn chn, pixel_format_e fmt)
{
	if (!GDC_SUPPORT_FMT(fmt)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for GDC.\n"
		, grp, chn, (fmt));
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	return 0;
}

void vpss_gdc_callback(void *param, vb_blk blk, struct vpss_ctx *ctx);
void vpss_wkup_frame_done_handle(void *pdata);
void vpss_handle_frame_done(osal_workqueue *work);

int vpss_online_prepare(int working_grp, struct vpss_ctx *ctx);

void vpss_chn_cancel_block(struct vpss_grp_ctx *grp_ctx, vpss_chn chn_id);
void release_buffers(struct vpss_grp_ctx *grp_ctx);

int vpss_grp_qbuf(mmf_chn_s chn, vb_blk blk, void *data);

void _update_vpss_chn_real_frame_rate(struct vpss_ctx *ctx, unsigned int duration_us);
void vpss_ctx_param_init(struct vpss_ctx *ctx);
void vpss_init(struct vpss_ctx *ctx);
void vpss_deinit(struct vpss_ctx *ctx);


#endif /* _VPSS_H_ */
