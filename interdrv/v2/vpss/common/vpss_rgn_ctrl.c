#include <linux/clk.h>

#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>

#include <vpss_cb.h>
#include "vpss_debug.h"
#include "vpss.h"
#include "vpss_core.h"
#include "scaler.h"
#include "vpss_ctx.h"


s32 vpss_get_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer,
			RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	s32 ret, i;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].coverEx_handle[i];
	} else if (enType == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			hdls[i] = pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].mosaic_handle[i];
	} else {
		ret = CVI_ERR_VPSS_NOT_SUPPORT;
	}

	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return ret;
}

s32 vpss_set_rgn_hdls(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer,
			RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	s32 ret, i;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i] = hdls[i];
	} else if (enType == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].coverEx_handle[i] = hdls[i];
	} else if (enType == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].mosaic_handle[i] = hdls[i];
	} else {
		ret = CVI_ERR_VPSS_NOT_SUPPORT;
	}
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return ret;
}

s32 vpss_set_rgn_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer, struct cvi_rgn_cfg *cfg)
{
	s32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_cfg[layer], cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

s32 vpss_set_rgn_coverex_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_coverex_cfg *cfg)
{
	s32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;


	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_coverex_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

s32 vpss_set_rgn_mosaic_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_mosaic_cfg *cfg)
{
	s32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;


	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_mosaic_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

s32 vpss_get_rgn_ow_addr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 layer,
		RGN_HANDLE handle, u64 *addr)
{
#if 0
	s32 ret, dev_idx, i;
	u8 ow_inst;
	MMF_CHN_S stChn;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();

	ret = mod_check_null_ptr(CVI_ID_VPSS, addr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	stChn.enModId = CVI_ID_VPSS;
	stChn.s32DevId = VpssGrp;
	stChn.s32ChnId = VpssChn;
	dev_idx = get_dev_info_by_chn(stChn, CHN_TYPE_OUT);

	for (i = 0; i < RGN_MAX_NUM_VPSS; ++i) {
		if (pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_handle[layer][i] == handle) {
			ow_inst = i;
			break;
		}
	}
	if (i == RGN_MAX_NUM_VPSS) {
		mutex_unlock(&pVpssCtx[VpssGrp]->lock);
		return CVI_FAILURE;
	}

	sclr_gop_ow_get_addr(dev_idx, layer, ow_inst, addr);
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);
#endif
	return CVI_SUCCESS;
}

s32 vpss_set_rgn_lut_cfg(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, struct cvi_rgn_lut_cfg *cfg)
{
	s32 ret;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();
	u8 layer = cfg->lut_layer;

	ret = mod_check_null_ptr(CVI_ID_VPSS, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_id(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&pVpssCtx[VpssGrp]->lock);
	memcpy(&pVpssCtx[VpssGrp]->stChnCfgs[VpssChn].rgn_cfg[layer].rgn_lut_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&pVpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

