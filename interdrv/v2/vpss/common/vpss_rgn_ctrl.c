#include <linux/clk.h>

#include "base_ctx.h"
#include <linux/defines.h>
#include <linux/common.h>
#include <linux/comm_buffer.h>

#include <vpss_cb.h>
#include "vpss_debug.h"
#include "vpss.h"
#include "vpss_core.h"
#include "scaler.h"
#include "vpss_ctx.h"


s32 vpss_get_rgn_hdls(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer,
			rgn_type_e type, rgn_handle hdls[])
{
	s32 ret, i;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, hdls);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	if (type == OVERLAY_RGN || type == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			hdls[i] = vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_handle[layer][i];
	} else if (type == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			hdls[i] = vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].cover_ex_handle[i];
	} else if (type == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			hdls[i] = vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].mosaic_handle[i];
	} else {
		ret = ERR_VPSS_NOT_SUPPORT;
	}

	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return ret;
}

s32 vpss_set_rgn_hdls(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer,
			rgn_type_e type, rgn_handle hdls[])
{
	s32 ret, i;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, hdls);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	if (type == OVERLAY_RGN || type == COVER_RGN) {
		for (i = 0; i < RGN_MAX_NUM_VPSS; ++i)
			vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_handle[layer][i] = hdls[i];
	} else if (type == COVEREX_RGN) {
		for (i = 0; i < RGN_COVEREX_MAX_NUM; ++i)
			vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].cover_ex_handle[i] = hdls[i];
	} else if (type == MOSAIC_RGN) {
		for (i = 0; i < RGN_MOSAIC_MAX_NUM; ++i)
			vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].mosaic_handle[i] = hdls[i];
	} else {
		ret = ERR_VPSS_NOT_SUPPORT;
	}
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return ret;
}

s32 vpss_set_rgn_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer, struct rgn_cfg *cfg)
{
	s32 ret;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, cfg);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;

	if (layer >= RGN_MAX_LAYER_VPSS) {
		TRACE_VPSS(DBG_ERR, "invalid layer(%d), vpss only has gop0 & gop1\n", layer);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	memcpy(&vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_cfg[layer], cfg, sizeof(*cfg));
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return 0;
}

s32 vpss_set_rgn_coverex_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_coverex_cfg *cfg)
{
	s32 ret;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, cfg);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;


	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	memcpy(&vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_coverex_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return 0;
}

s32 vpss_set_rgn_mosaic_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_mosaic_cfg *cfg)
{
	s32 ret;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, cfg);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;


	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	memcpy(&vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_mosaic_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return 0;
}

s32 vpss_get_rgn_ow_addr(vpss_grp vpss_grp, vpss_chn vpss_chn, u32 layer,
		rgn_handle handle, u64 *addr)
{
	s32 ret, dev_idx, i, j;
	u8 ow_inst;
	mmf_chn_s chn;
	struct vpss_job *job;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();

	ret = mod_check_null_ptr(ID_VPSS, addr);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;

	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	chn.mod_id = ID_VPSS;
	chn.dev_id = vpss_grp;
	chn.chn_id = vpss_chn;

	job = (struct vpss_job *)vpss_ctx[vpss_grp]->job_buffer;
	for (i = 0, j = 0; i < VPSS_MAX; i++) {
		if ((job->vpss_dev_mask & BIT(i))) {
			if (j == vpss_chn) {
				dev_idx = i;
				break;
			}
			j++;
		}
	}

	for (i = 0; i < RGN_MAX_NUM_VPSS; ++i) {
		if (vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_handle[layer][i] == handle) {
			ow_inst = i;
			break;
		}
	}
	if (i == RGN_MAX_NUM_VPSS) {
		mutex_unlock(&vpss_ctx[vpss_grp]->lock);
		return -1;
	}

	sclr_gop_ow_get_addr(dev_idx, layer, ow_inst, addr);
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return 0;
}

s32 vpss_set_rgn_lut_cfg(vpss_grp vpss_grp, vpss_chn vpss_chn, struct rgn_lut_cfg *cfg)
{
	s32 ret;
	struct vpss_ctx **vpss_ctx = vpss_get_ctx();
	u8 layer = cfg->lut_layer;

	ret = mod_check_null_ptr(ID_VPSS, cfg);
	if (ret != 0)
		return ret;

	ret = check_vpss_id(vpss_grp, vpss_chn);
	if (ret != 0)
		return ret;

	mutex_lock(&vpss_ctx[vpss_grp]->lock);
	memcpy(&vpss_ctx[vpss_grp]->chn_cfgs[vpss_chn].rgn_cfg[layer].rgn_lut_cfg, cfg, sizeof(*cfg));
	mutex_unlock(&vpss_ctx[vpss_grp]->lock);

	return 0;
}

