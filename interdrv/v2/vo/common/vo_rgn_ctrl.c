#include <linux/types.h>
#include <linux/mm.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>

#include <vo.h>
#include <vo_cb.h>
#include "disp.h"

static u8 _gop_get_bpp(enum disp_gop_format fmt)
{
	return (fmt == DISP_GOP_FMT_ARGB8888) ? 4 :
		(fmt == DISP_GOP_FMT_256LUT) ? 1 : 2;
}

static s32 vo_set_rgn_cfg(const u8 inst, const u8 layer, const struct cvi_rgn_cfg *cfg, const struct disp_size *size)
{
	u8 i;
	struct disp_gop_cfg *gop_cfg = disp_gop_get_cfg(inst, layer);
	struct disp_gop_ow_cfg *ow_cfg;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = cfg->colorkey_en;
	gop_cfg->colorkey = cfg->colorkey;

	for (i = 0; i < cfg->num_of_rgn; ++i) {
		u8 bpp = _gop_get_bpp((enum disp_gop_format)cfg->param[i].fmt);

		ow_cfg = &gop_cfg->ow_cfg[i];
		gop_cfg->gop_ctrl.raw |= BIT(i);

		ow_cfg->fmt = (enum disp_gop_format)cfg->param[i].fmt;
		ow_cfg->addr = cfg->param[i].phy_addr;
		ow_cfg->pitch = cfg->param[i].stride;
		if (cfg->param[i].rect.left < 0) {
			ow_cfg->start.x = 0;
			ow_cfg->addr -= bpp * cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width + cfg->param[i].rect.left;
		} else if ((cfg->param[i].rect.left + cfg->param[i].rect.width) > size->w) {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = size->w - cfg->param[i].rect.left;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		} else {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		}

		if (cfg->param[i].rect.top < 0) {
			ow_cfg->start.y = 0;
			ow_cfg->addr -= ow_cfg->pitch * cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height + cfg->param[i].rect.top;
		} else if ((cfg->param[i].rect.top + cfg->param[i].rect.height) > size->h) {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = size->h - cfg->param[i].rect.top;
		} else {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height;
		}

		ow_cfg->end.x = ow_cfg->start.x + (ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en)
						- gop_cfg->gop_ctrl.b.hscl_en;
		ow_cfg->end.y = ow_cfg->start.y + (ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en)
						- gop_cfg->gop_ctrl.b.vscl_en;
		ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
		ow_cfg->mem_size.h = ow_cfg->img_size.h;
#if 0
		CVI_TRACE_VO(CVI_DBG_INFO, "gop(%d) fmt(%d) rect(%d %d %d %d) addr(%llx) pitch(%d).\n", inst
			, ow_cfg->fmt, ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h
			, ow_cfg->addr, ow_cfg->pitch);
#endif
		disp_gop_ow_set_cfg(inst, layer, i, ow_cfg, true);
	}

	disp_gop_set_cfg(inst, layer, gop_cfg, true);

	return CVI_SUCCESS;
}

#if 0
static void vo_set_rgn_coverex_cfg(u8 inst, struct cvi_rgn_coverex_cfg *cfg)
{
	s32 i;
	struct disp_cover_cfg sc_cover_cfg;

	for (i = 0; i < RGN_COVEREX_MAX_NUM; i++) {
		if (cfg->rgn_coverex_param[i].enable) {
			sc_cover_cfg.start.raw = 0;
			sc_cover_cfg.color.raw = 0;
			sc_cover_cfg.start.b.enable = 1;
			sc_cover_cfg.start.b.x = cfg->rgn_coverex_param[i].rect.left;
			sc_cover_cfg.start.b.y = cfg->rgn_coverex_param[i].rect.top;
			sc_cover_cfg.img_size.w = cfg->rgn_coverex_param[i].rect.width;
			sc_cover_cfg.img_size.h = cfg->rgn_coverex_param[i].rect.height;
			sc_cover_cfg.color.b.cover_color_r = (cfg->rgn_coverex_param[i].color >> 16) & 0xff;
			sc_cover_cfg.color.b.cover_color_g = (cfg->rgn_coverex_param[i].color >> 8) & 0xff;
			sc_cover_cfg.color.b.cover_color_b = cfg->rgn_coverex_param[i].color & 0xff;
		} else {
			memset(&sc_cover_cfg, 0, sizeof(sc_cover_cfg));
		}
		disp_cover_set_cfg(inst, i, &sc_cover_cfg);
	}
}
#endif

static s32 _check_vo_status(VO_LAYER VoLayer)
{
	s32 ret = CVI_SUCCESS;

	ret = CHECK_VO_OVERLAY_VALID(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	return ret;
}

s32 vo_cb_get_rgn_hdls(VO_LAYER VoLayer, RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	s32 ret, i;
	struct cvi_vo_overlay_ctx *pstOverlayCtx;

	ret = CHECK_VO_NULL_PTR(CVI_ID_VO, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstOverlayCtx = &gVoCtx->astOverlayCtx[VoLayer - VO_MAX_LAYER_NUM];
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			hdls[i] = pstOverlayCtx->rgn_handle[i];
	} else {
		ret = CVI_ERR_VO_NOT_SUPPORT;
	}

	return ret;
}

s32 vo_cb_set_rgn_hdls(VO_LAYER VoLayer, RGN_TYPE_E enType, RGN_HANDLE hdls[])
{
	s32 ret, i;
	struct cvi_vo_overlay_ctx *pstOverlayCtx;

	ret = CHECK_VO_NULL_PTR(CVI_ID_VO, hdls);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstOverlayCtx = &gVoCtx->astOverlayCtx[VoLayer - VO_MAX_LAYER_NUM];
	if (enType == OVERLAY_RGN || enType == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VO; ++i)
			pstOverlayCtx->rgn_handle[i] = hdls[i];
	} else {
		ret = CVI_ERR_VO_NOT_SUPPORT;
	}

	return ret;
}

s32 vo_cb_set_rgn_cfg(VO_LAYER VoLayer, struct cvi_rgn_cfg *cfg)
{
	s32 ret, i;
	struct cvi_vo_overlay_ctx *pstOverlayCtx;
	struct cvi_vo_dev_ctx *pstDevCtx;
	struct disp_timing *timing;
	struct disp_size size;
	VO_DEV VoDev;
	VO_LAYER Overlay;
	s32 VgopIndex = -1;

	ret = CHECK_VO_NULL_PTR(CVI_ID_VO, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	Overlay = VoLayer - VO_MAX_LAYER_NUM;
	pstOverlayCtx = &gVoCtx->astOverlayCtx[Overlay];
	memcpy(&pstOverlayCtx->rgn_cfg, cfg, sizeof(*cfg));

	VoDev = pstOverlayCtx->s32BindDevId;
	if (VoDev == -1) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) not bind any Dev\n", VoLayer);
		return CVI_ERR_VO_DEV_NOT_BINDED;
	}

	pstDevCtx = &gVoCtx->astDevCtx[VoDev];

	mutex_lock(&gVoCtx->astDevCtx[VoDev].dev_lock);
	for (i = 0; i < VO_MAX_OVERLAY_IN_DEV; ++i) {
		if (pstDevCtx->s32BindOverlayId[i] == VoLayer) {
			VgopIndex = i;
		}
	}
	mutex_unlock(&gVoCtx->astDevCtx[VoDev].dev_lock);

	if (VgopIndex == -1)
		return CVI_ERR_VO_DEV_NOT_BINDED;

	timing = disp_get_timing(VoDev);
	size.w = timing->hfde_end - timing->hfde_start + 1;
	size.h = timing->vfde_end - timing->vfde_start + 1;

	vo_set_rgn_cfg(pstOverlayCtx->s32BindDevId, VgopIndex, cfg, &size);

	return CVI_SUCCESS;
}

#if 0
s32 vo_cb_set_rgn_coverex_cfg(VO_LAYER VoLayer, struct cvi_rgn_coverex_cfg *cfg)
{
	s32 ret;
	struct cvi_vo_layer_ctx *pstLayerCtx;

	ret = CHECK_VO_NULL_PTR(CVI_ID_VO, cfg);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstLayerCtx = &gVoCtx->astLayerCtx[VoLayer];
	memcpy(&pstLayerCtx->rgn_coverex_cfg, cfg, sizeof(*cfg));
	vo_set_rgn_coverex_cfg(pstLayerCtx->s32BindDevId, cfg);

	return CVI_SUCCESS;
}
#endif

s32 vo_cb_get_chn_size(VO_LAYER VoLayer, RECT_S *rect)
{
	s32 ret;
	struct cvi_vo_overlay_ctx *pstOverlayCtx;
	VO_DEV VoDev;
	VO_LAYER VideoLayer;

	ret = CHECK_VO_NULL_PTR(CVI_ID_VO, rect);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = _check_vo_status(VoLayer);
	if (ret != CVI_SUCCESS)
		return ret;

	pstOverlayCtx = &gVoCtx->astOverlayCtx[VoLayer - VO_MAX_LAYER_NUM];

	VoDev = pstOverlayCtx->s32BindDevId;
	if (VoDev == -1) {
		CVI_TRACE_VO(CVI_DBG_ERR, "VoLayer(%d) not bind any Dev\n", VoLayer);
		return CVI_ERR_VO_DEV_NOT_BINDED;
	}

	mutex_lock(&gVoCtx->astDevCtx[VoDev].dev_lock);
	VideoLayer = gVoCtx->astDevCtx[VoDev].s32BindLayerId;
	mutex_unlock(&gVoCtx->astDevCtx[VoDev].dev_lock);

	memcpy(rect, &gVoCtx->astLayerCtx[VideoLayer].stLayerAttr.stDispRect, sizeof(*rect));

	return CVI_SUCCESS;
}
