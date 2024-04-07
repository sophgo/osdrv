#ifndef __VO_SDK_LAYER_H__
#define __VO_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/vo_uapi.h>
#include <linux/types.h>
#include <linux/cvi_errno.h>
#include "cvi_vo_ctx.h"
#include "vo_common.h"
#include "vo_defines.h"

extern struct cvi_vo_ctx *gVoCtx;

static inline s32 CHECK_VO_LAYER_DISABLE(VO_LAYER VoLayer)
{
	if (gVoCtx->astLayerCtx[VoLayer].is_layer_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) already enabled.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_DISABLED;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_LAYER_VALID(VO_LAYER VoLayer)
{
	if (VoLayer >= VO_MAX_LAYER_NUM) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) invalid.\n", VoLayer);
		return CVI_ERR_VO_INVALID_LAYERID;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_LAYER_ENABLE(VO_LAYER VoLayer)
{

	if (!gVoCtx->astLayerCtx[VoLayer].is_layer_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) not enable.\n", VoLayer);
		return CVI_ERR_VO_VIDEO_NOT_ENABLED;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_DEV_VALID(VO_DEV VoDev)
{
	if ((VoDev >= VO_MAX_DEV_NUM) || (VoDev < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoDev(%d) invalid.\n", VoDev);
		return CVI_ERR_VO_INVALID_DEVID;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_CHN_VALID(VO_LAYER VoLayer, VO_CHN VoChn)
{
	if ((VoLayer >= VO_MAX_LAYER_NUM) || (VoLayer < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) invalid.\n", VoLayer);
		return CVI_ERR_VO_INVALID_LAYERID;
	}
	if ((VoChn >= VO_MAX_CHN_NUM) || (VoChn < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoChn(%d) invalid.\n", VoChn);
		return CVI_ERR_VO_INVALID_CHNID;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_CHN_ENABLE(VO_LAYER VoLayer, VO_CHN VoChn)
{
	if (!gVoCtx->astLayerCtx[VoLayer].astChnCtx[VoChn].is_chn_enable) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) VoChn(%d) isn't enabled yet.\n", VoLayer, VoChn);
		return CVI_ERR_VO_CHN_NOT_ENABLED;
	}

	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_NULL_PTR(MOD_ID_E mod, const void *ptr)
{
	if (mod >= CVI_ID_BUTT)
		return CVI_FAILURE;

	if (!ptr) {
		CVI_TRACE_VO(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_VO_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline s32 CHECK_VO_WBC_VALID(VO_WBC VoWbc)
{
	if ((VoWbc >= VO_MAX_WBC_NUM) || (VoWbc < 0)) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoWbc(%d) invalid.\n", VoWbc);
		return CVI_ERR_VO_INVALID_WBCID;
	}

	return CVI_SUCCESS;
}

long vo_sdk_ctrl(struct cvi_vo_dev *vdev, struct vo_ext_control *p);
s32 vo_disable(VO_DEV VoDev);
s32 vo_disablevideolayer(VO_LAYER VoLayer);
s32 vo_disable_chn(VO_LAYER VoLayer, VO_CHN VoChn);
s32 vo_get_chnrotation(VO_LAYER VoLayer, VO_CHN VoChn, ROTATION_E *penRotation);
s32 vo_wbc_qbuf(struct cvi_vo_wbc_ctx *pstWbcCtx);
struct vo_fmt *vo_sdk_get_format(u32 pixelformat);

#ifdef __cplusplus
}
#endif

#endif //__VO_SDK_LAYER_H__
