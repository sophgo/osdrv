#include "gfbg_ctrl.h"
#include "vo_ctx.h"
#include "vo_sdk_layer.h"

int vo_gfbg_get_bind_dev_id(vo_layer layer, vo_dev *dev)
{
	int ret;
	struct vo_overlay_ctx *overlay_ctx;

	ret = check_vo_null_ptr(ID_VO, dev);
	if (ret != 0)
		return ret;

	if (layer < 0 || layer >= VO_MAX_GRAPHIC_LAYER_NUM) {
		return ERR_VO_INVALID_LAYERID;
	}

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer];

	*dev = overlay_ctx->bind_dev_id;
	if (*dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(vo_gfbg_get_bind_dev_id);

int vo_gfbg_get_hw_layer_id(vo_layer layer, vo_layer *hw_layer)
{
	int ret, i;
	struct vo_overlay_ctx *overlay_ctx;
	struct vo_dev_ctx *dev_ctx;
	vo_dev dev;
	int vgop_index = -1;

	ret = check_vo_null_ptr(ID_VO, hw_layer);
	if (ret != 0)
		return ret;

	if (layer < 0 || layer >= VO_MAX_GRAPHIC_LAYER_NUM) {
		return ERR_VO_INVALID_LAYERID;
	}

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer];
	dev = overlay_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i)
		if (dev_ctx->bind_overlay_id[i] == (layer + VO_MAX_VIDEO_LAYER_NUM))
			vgop_index = i;

	if (vgop_index == -1)
		return ERR_VO_DEV_NOT_BINDED;

	*hw_layer = vgop_index;

	return 0;
}
EXPORT_SYMBOL_GPL(vo_gfbg_get_hw_layer_id);

int vo_gfbg_dev_is_enable(vo_dev dev, bool *is_enable)
{
	int ret;
	struct vo_dev_ctx *dev_ctx;

	ret = check_vo_null_ptr(ID_VO, is_enable);
	if (ret != 0)
		return ret;

	ret = check_vo_dev_valid(dev);
	if (ret != 0)
		return ret;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];
	*is_enable = dev_ctx->is_dev_enable;

	return 0;
}
EXPORT_SYMBOL_GPL(vo_gfbg_dev_is_enable);

int vo_gfbg_set_layer_enable(vo_layer layer, bool enable)
{
	int i;
	struct vo_overlay_ctx *overlay_ctx;
	struct vo_dev_ctx *dev_ctx;
	vo_dev dev;
	int vgop_index = -1;

	if (layer < 0 || layer >= VO_MAX_GRAPHIC_LAYER_NUM) {
		return ERR_VO_INVALID_LAYERID;
	}

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer];
	dev = overlay_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i)
		if (dev_ctx->bind_overlay_id[i] == (layer + VO_MAX_VIDEO_LAYER_NUM))
			vgop_index = i;

	if (vgop_index == -1)
		return ERR_VO_DEV_NOT_BINDED;

	overlay_ctx->enable = enable;

	return 0;
}
EXPORT_SYMBOL_GPL(vo_gfbg_set_layer_enable);

int vo_gfbg_get_layer_enable(vo_layer layer, bool *enable)
{
	int i;
	struct vo_overlay_ctx *overlay_ctx;
	struct vo_dev_ctx *dev_ctx;
	vo_dev dev;
	int vgop_index = -1;

	if (layer < 0 || layer >= VO_MAX_GRAPHIC_LAYER_NUM) {
		return ERR_VO_INVALID_LAYERID;
	}

	overlay_ctx = &g_vo_ctx->overlay_ctx[layer];
	dev = overlay_ctx->bind_dev_id;
	if (dev == -1) {
		TRACE_VO(DBG_ERR, "layer(%d) not bind any Dev\n", layer);
		return ERR_VO_DEV_NOT_BINDED;
	}

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++i)
		if (dev_ctx->bind_overlay_id[i] == (layer + VO_MAX_VIDEO_LAYER_NUM))
			vgop_index = i;

	if (vgop_index == -1)
		return ERR_VO_DEV_NOT_BINDED;

	*enable = overlay_ctx->enable;

	return 0;
}
EXPORT_SYMBOL_GPL(vo_gfbg_get_layer_enable);
