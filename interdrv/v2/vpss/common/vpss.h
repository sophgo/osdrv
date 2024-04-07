#ifndef _VPSS_H_
#define _VPSS_H_

#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_region.h>
#include <linux/cvi_errno.h>
#include <linux/vpss_uapi.h>

#include <vpss_ctx.h>
#include <base_ctx.h>
#include <base_cb.h>
#include <ldc_cb.h>
#include "vpss_debug.h"

typedef void (*vpss_timer_cb)(void *data);

/* Configured from user, IOCTL */
s32 vpss_create_grp(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);
s32 vpss_destroy_grp(VPSS_GRP VpssGrp);
VPSS_GRP vpss_get_available_grp(void);

s32 vpss_start_grp(VPSS_GRP VpssGrp);
s32 vpss_stop_grp(VPSS_GRP VpssGrp);

s32 vpss_reset_grp(VPSS_GRP VpssGrp);

s32 vpss_set_grp_attr(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr);
s32 vpss_get_grp_attr(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstGrpAttr);

s32 vpss_set_grp_crop(VPSS_GRP VpssGrp, const VPSS_CROP_INFO_S *pstCropInfo);
s32 vpss_get_grp_crop(VPSS_GRP VpssGrp, VPSS_CROP_INFO_S *pstCropInfo);

s32 vpss_get_grp_frame(VPSS_GRP VpssGrp, VIDEO_FRAME_INFO_S *pstVideoFrame);
s32 vpss_release_grp_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame);

s32 vpss_send_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);
s32 vpss_send_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn
	, const VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);

s32 vpss_set_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_ATTR_S *pstChnAttr);
s32 vpss_get_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_ATTR_S *pstChnAttr);

s32 vpss_enable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
s32 vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

s32 vpss_set_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CROP_INFO_S *pstCropInfo);
s32 vpss_get_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo);

s32 vpss_set_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation);
s32 vpss_get_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E *penRotation);

s32 vpss_set_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_LDC_ATTR_S *pstLDCAttr,
			u64 mesh_addr);
s32 vpss_get_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr);

s32 vpss_set_chn_fisheye_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation,
			const FISHEYE_ATTR_S *pstFishEyeAttr, u64 mesh_addr);
s32 vpss_get_chn_fisheye_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, FISHEYE_ATTR_S *pstFishEyeAttr);

s32 vpss_get_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
			 s32 s32MilliSec);
s32 vpss_release_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame);

s32 vpss_set_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 u32Align);
s32 vpss_get_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 *pu32Align);

s32 vpss_set_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 YRatio);
s32 vpss_get_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 *pYRatio);

s32 vpss_set_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E enCoef);
s32 vpss_get_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E *penCoef);

s32 vpss_set_chn_draw_rect(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_DRAW_RECT_S *pstDrawRect);
s32 vpss_get_chn_draw_rect(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_DRAW_RECT_S *pstDrawRect);

s32 vpss_set_chn_convert(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CONVERT_S *pstConvert);
s32 vpss_get_chn_convert(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CONVERT_S *pstConvert);

s32 vpss_show_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);
s32 vpss_hide_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

s32 vpss_attach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VB_POOL hVbPool);
s32 vpss_detach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

s32 vpss_trigger_snap_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 frame_cnt);

s32 vpss_stitch(u32 u32ChnNum, VPSS_STITCH_CHN_ATTR_S *pstInput,
			VPSS_STITCH_OUTPUT_ATTR_S *pstOutput, VIDEO_FRAME_INFO_S *pstVideoFrame);

s32 vpss_set_mod_param(const VPSS_MOD_PARAM_S *pstModParam);
s32 vpss_get_mod_param(VPSS_MOD_PARAM_S *pstModParam);

s32 vpss_bm_send_frame(bm_vpss_cfg *vpss_cfg);

/* INTERNAL */
s32 vpss_set_vivpss_mode(const VI_VPSS_MODE_S *pstVIVPSSMode);
s32 vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg);
s32 vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg);
s32 vpss_get_proc_amp_ctrl(PROC_AMP_E type, PROC_AMP_CTRL_S *ctrl);
s32 vpss_get_proc_amp(VPSS_GRP VpssGrp, s32 *proc_amp);
s32 vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg);

void vpss_set_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i);
void vpss_get_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i);

int _vpss_call_cb(u32 m_id, u32 cmd_id, void *data);
void vpss_init(void);
void vpss_deinit(void);
void vpss_gdc_callback(void *pParam, VB_BLK blk);

s32 check_vpss_id(VPSS_GRP VpssGrp, VPSS_CHN VpssChn);

void vpss_mode_init(void);
void vpss_mode_deinit(void);

void register_timer_fun(vpss_timer_cb cb, void *data);


struct cvi_vpss_ctx **vpss_get_ctx(void);

void vpss_get_mod_param_void(VPSS_MOD_PARAM_S *pstModParam);

void vpss_release_grp(void);

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
	&& (IS_FMT_YUV422(fmt_chn) || IS_FMT_RGB(fmt_chn) || (fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))) \
	|| (IS_FMT_YUV422(fmt_grp) && (IS_FMT_RGB(fmt_chn) || (fmt_chn == PIXEL_FORMAT_YUV_PLANAR_444) || (fmt_chn == PIXEL_FORMAT_YUV_444))))

#define FRC_INVALID(stFrameRate)	\
	(stFrameRate.s32DstFrameRate <= 0 || stFrameRate.s32SrcFrameRate <= 0 ||	\
		stFrameRate.s32DstFrameRate >= stFrameRate.s32SrcFrameRate)

static inline s32 mod_check_null_ptr(MOD_ID_E mod, const void *ptr)
{
	if (mod >= CVI_ID_BUTT)
		return CVI_FAILURE;
	if (!ptr) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_VPSS_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline s32 check_vpss_grp_valid(VPSS_GRP grp)
{
	if ((grp >= VPSS_MAX_GRP_NUM) || (grp < 0)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "VpssGrp(%d) exceeds Max(%d)\n", grp, VPSS_MAX_GRP_NUM);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 check_yuv_param(enum _PIXEL_FORMAT_E fmt, u32 w, u32 h)
{
	if (fmt == PIXEL_FORMAT_YUV_PLANAR_422) {
		if (w & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_422 width(%d) should be even.\n", w);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	} else if ((fmt == PIXEL_FORMAT_YUV_PLANAR_420)
		   || (fmt == PIXEL_FORMAT_NV12)
		   || (fmt == PIXEL_FORMAT_NV21)) {
		if (w & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420 width(%d) should be even.\n", w);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		if (h & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420 height(%d) should be even.\n", h);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline s32 check_vpss_grp_fmt(VPSS_GRP grp, enum _PIXEL_FORMAT_E fmt)
{
	if (!VPSS_GRP_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) enPixelFormat(%d) unsupported\n"
		, grp, fmt);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 check_vpss_chn_fmt(VPSS_GRP grp, VPSS_CHN chn, enum _PIXEL_FORMAT_E fmt)
{
	if (!VPSS_CHN_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) enPixelFormat(%d) unsupported\n"
		, grp, chn, fmt);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 check_vpss_gdc_fmt(VPSS_GRP grp, VPSS_CHN chn, enum _PIXEL_FORMAT_E fmt)
{
	if (!GDC_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for GDC.\n"
		, grp, chn, (fmt));
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 check_vpss_dwa_fmt(VPSS_GRP grp, VPSS_CHN chn, enum _PIXEL_FORMAT_E fmt)
{
	if (!DWA_SUPPORT_FMT(fmt)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid PixFormat(%d) for DWA.\n"
		, grp, chn, (fmt));
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

#endif /* _VPSS_H_ */
