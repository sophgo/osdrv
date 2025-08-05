#include "vpss_debug.h"
#include "common.h"
#include "vpss.h"


static inline int check_vpss_id(vpss_grp grp, vpss_chn chn, struct vpss_ctx *ctx)
{
	if (check_vpss_grp_valid(grp))
		return ERR_VPSS_ILLEGAL_PARAM;

	if (check_vpss_grp_created(grp, ctx)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return ERR_VPSS_UNEXIST;
	}

	if (check_vpss_chn_valid(grp, chn, ctx->grp_ctx[grp]->chn_max_num))
		return ERR_VPSS_ILLEGAL_PARAM;
	return 0;
}

s32 vpss_get_rgn_hdls(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
			rgn_type_e type, rgn_handle hdls[], struct vpss_ctx *ctx)
{
	s32 ret, i;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	if (hdls == NULL) {
		TRACE_VPSS(DBG_ERR, "hdls is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	if (type == OVERLAY_RGN || type == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			hdls[i] = chn_ctx->rgn_handle[layer][i];
	} else if (type == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			hdls[i] = chn_ctx->cover_ex_handle[i];
	} else if (type == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			hdls[i] = chn_ctx->mosaic_handle[i];
	} else {
		ret = ERR_VPSS_NOT_SUPPORT;
	}

	osal_mutex_unlock(&grp_ctx->lock);

	return ret;
}

s32 vpss_set_rgn_hdls(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
			rgn_type_e type, rgn_handle hdls[], struct vpss_ctx *ctx)
{
	s32 ret, i;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	if (hdls == NULL) {
		TRACE_VPSS(DBG_ERR, "hdls is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	if (type == OVERLAY_RGN || type == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			chn_ctx->rgn_handle[layer][i] = hdls[i];
	} else if (type == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			chn_ctx->cover_ex_handle[i] = hdls[i];
	} else if (type == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			chn_ctx->mosaic_handle[i] = hdls[i];
	} else {
		ret = ERR_VPSS_NOT_SUPPORT;
	}
	osal_mutex_unlock(&grp_ctx->lock);

	return ret;
}

s32 vpss_set_rgn_cfg(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
		struct rgn_cfg *cfg, struct vpss_ctx *ctx)
{
	s32 ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	if (cfg == NULL) {
		TRACE_VPSS(DBG_ERR, "cfs is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(&chn_ctx->rgn_cfg[layer], cfg, sizeof(*cfg));
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

s32 vpss_set_rgn_coverex_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_coverex_cfg *cfg, struct vpss_ctx *ctx)
{
	s32 ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	if (cfg == NULL) {
		TRACE_VPSS(DBG_ERR, "hdls is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(&chn_ctx->rgn_coverex_cfg, cfg, sizeof(*cfg));
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

s32 vpss_set_rgn_mosaic_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_mosaic_cfg *cfg, struct vpss_ctx *ctx)
{
	s32 ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	if (cfg == NULL) {
		TRACE_VPSS(DBG_ERR, "hdls is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(&chn_ctx->rgn_mosaic_cfg, cfg, sizeof(*cfg));
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

s32 vpss_get_rgn_ow_addr(vpss_grp grp_id, vpss_chn chn_id, u32 layer,
		rgn_handle handle, u64 *addr, struct vpss_ctx *ctx, u8 dev_idx)
{
	s32 ret, i;
	u8 ow_inst;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&grp_ctx->lock);
	for (i = 0; i < RGN_MAX_NUM_VPSS; ++i) {
		if (chn_ctx->rgn_handle[layer][i] == handle) {
			ow_inst = i;
			break;
		}
	}
	if (i == RGN_MAX_NUM_VPSS) {
		osal_mutex_unlock(&grp_ctx->lock);
		return -1;
	}
	vpss_get_gop_addr(dev_idx, layer, ow_inst, addr);
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

s32 vpss_set_rgn_lut_cfg(vpss_grp grp_id, vpss_chn chn_id,
		struct rgn_lut_cfg *cfg, struct vpss_ctx *ctx)
{
	s32 ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	u8 layer = cfg->lut_layer;

	if (cfg == NULL) {
		TRACE_VPSS(DBG_ERR, "hdls is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	ret = check_vpss_id(grp_id, chn_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(&chn_ctx->rgn_cfg[layer].rgn_lut_cfg, cfg, sizeof(*cfg));
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

