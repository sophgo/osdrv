#include "gfbg_hal.h"
#include "gfbg_debug.h"
#include "gfbg_disp.h"

void gfbg_hal_set_layer_enable(bool enable, vo_dev dev_id, vo_layer layer_id)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	cfg->gop_ctrl.raw &= ~0xff;
	cfg->gop_ctrl.b.ow0_en = enable;
	gfbg_gop_set_cfg(dev_id, layer_id, cfg, true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - %d\n", __func__, dev_id, layer_id, enable);
}

void gfbg_hal_set_layer_data_fmt(vo_dev dev_id, vo_layer layer_id, enum disp_gop_format pixel_format_for_hal)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);
	cfg->ow_cfg[0].fmt = pixel_format_for_hal;
	gfbg_gop_ow_set_cfg(dev_id, layer_id, 0, &(cfg->ow_cfg[0]), true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - %d\n",
		   __func__, dev_id, layer_id, pixel_format_for_hal);
}

void gfbg_hal_set_layer_stride(vo_dev dev_id, vo_layer layer_id, unsigned int stride)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);
	cfg->ow_cfg[0].pitch = stride;
	gfbg_gop_ow_set_cfg(dev_id, layer_id, 0, &(cfg->ow_cfg[0]), true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - %d\n", __func__, dev_id, layer_id, stride);
}

void gfbg_hal_set_layer_rect(vo_dev dev_id, vo_layer layer_id, fb_rect *rect, const gfbg_display_info *display_info)
{
	unsigned int bytesperpixel;
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	switch (cfg->ow_cfg[0].fmt) {
	case DISP_GOP_FMT_ARGB8888:
		bytesperpixel = 4;
		break;
	case DISP_GOP_FMT_ARGB4444:
	case DISP_GOP_FMT_ARGB1555:
		bytesperpixel = 2;
		break;
	case DISP_GOP_FMT_256LUT:
		bytesperpixel = 1;
		break;
	default:
		break;
	}

	cfg->ow_cfg[0].img_size.w = rect->width;
	cfg->ow_cfg[0].img_size.h = rect->height;
	cfg->ow_cfg[0].start.x = rect->x;
	cfg->ow_cfg[0].start.y = rect->y;

	if (cfg->ow_cfg[0].start.x + display_info->screen_width > display_info->max_screen_width) {
		cfg->ow_cfg[0].mem_size.w = ALIGN(((display_info->max_screen_width -
						   cfg->ow_cfg[0].start.x) >> cfg->gop_ctrl.b.hscl_en) *
						   bytesperpixel, 16);
	} else {
		cfg->ow_cfg[0].mem_size.w = ALIGN(cfg->ow_cfg[0].img_size.w * bytesperpixel, 16);
	}

	cfg->ow_cfg[0].mem_size.h = rect->height;

	cfg->ow_cfg[0].end.x = cfg->ow_cfg[0].start.x + (cfg->ow_cfg[0].img_size.w <<
			       cfg->gop_ctrl.b.hscl_en) - cfg->gop_ctrl.b.hscl_en;

	if (cfg->ow_cfg[0].end.x > display_info->max_screen_width)
		cfg->ow_cfg[0].end.x = display_info->max_screen_width - cfg->gop_ctrl.b.hscl_en;

	cfg->ow_cfg[0].end.y = cfg->ow_cfg[0].start.y + (cfg->ow_cfg[0].img_size.h <<
			       cfg->gop_ctrl.b.vscl_en) - cfg->gop_ctrl.b.vscl_en;

	if (cfg->ow_cfg[0].end.y > display_info->max_screen_height)
		cfg->ow_cfg[0].end.y = display_info->max_screen_height - cfg->gop_ctrl.b.vscl_en;

	gfbg_gop_ow_set_cfg(dev_id, layer_id, 0, &(cfg->ow_cfg[0]), true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - (%d,%d,%d,%d)\n", __func__,
		   dev_id, layer_id, cfg->ow_cfg[0].start.x, cfg->ow_cfg[0].start.y,
		   cfg->ow_cfg[0].end.x - cfg->ow_cfg[0].start.x + cfg->gop_ctrl.b.hscl_en,
		   cfg->ow_cfg[0].end.y - cfg->ow_cfg[0].start.y + cfg->gop_ctrl.b.vscl_en);
}

void gfbg_hal_set_layer_zoom(vo_dev dev_id, vo_layer layer_id, bool hscl_en, bool vscl_en,
			     const gfbg_display_info *display_info)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	cfg->gop_ctrl.b.hscl_en = hscl_en;
	cfg->gop_ctrl.b.vscl_en = vscl_en;

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - hscl_en(%d) vscl_en(%d)\n",
		   __func__, dev_id, layer_id, hscl_en, vscl_en);

	cfg->ow_cfg[0].end.x = cfg->ow_cfg[0].start.x + (cfg->ow_cfg[0].img_size.w <<
			       cfg->gop_ctrl.b.hscl_en) - cfg->gop_ctrl.b.hscl_en;

	if (cfg->ow_cfg[0].end.x > display_info->max_screen_width)
		cfg->ow_cfg[0].end.x = display_info->max_screen_width - cfg->gop_ctrl.b.hscl_en;

	cfg->ow_cfg[0].end.y = cfg->ow_cfg[0].start.y + (cfg->ow_cfg[0].img_size.h <<
			       cfg->gop_ctrl.b.vscl_en) - cfg->gop_ctrl.b.vscl_en;

	if (cfg->ow_cfg[0].end.y > display_info->max_screen_height)
		cfg->ow_cfg[0].end.y = display_info->max_screen_height - cfg->gop_ctrl.b.vscl_en;

	gfbg_gop_set_cfg(dev_id, layer_id, cfg, true);
	gfbg_gop_ow_set_cfg(dev_id, layer_id, 0, &(cfg->ow_cfg[0]), true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - (%d,%d,%d,%d)\n", __func__,
		   dev_id, layer_id, cfg->ow_cfg[0].start.x, cfg->ow_cfg[0].start.y,
		   cfg->ow_cfg[0].end.x - cfg->ow_cfg[0].start.x + cfg->gop_ctrl.b.hscl_en,
		   cfg->ow_cfg[0].end.y - cfg->ow_cfg[0].start.y + cfg->gop_ctrl.b.vscl_en);
}

void gfbg_hal_set_layer_addr(vo_dev dev_id, vo_layer layer_id, phys_addr_t addr)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	cfg->ow_cfg[0].addr = addr;

	gfbg_gop_ow_set_cfg(dev_id, layer_id, 0, &(cfg->ow_cfg[0]), true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - (0x%llx)\n",
		   __func__, dev_id, layer_id, cfg->ow_cfg[0].addr);
}

void gfbg_hal_set_layer_colorkey(vo_dev dev_id, vo_layer layer_id, const gfbg_colorkeyex *colorkey)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	cfg->gop_ctrl.b.colorkey_en = colorkey->key_enable;
	cfg->colorkey = colorkey->key & 0xffffff;

	gfbg_gop_set_cfg(dev_id, layer_id, cfg, true);

	TRACE_GFBG(DBG_INFO, "%s: dev(%d) layer (%d) - enable(%d) key(0x%x)\n",
		   __func__, dev_id, layer_id, colorkey->key_enable, colorkey->key);
}

int gfbg_hal_set_color_reg(vo_dev dev_id, vo_layer layer_id, fb_color_format format,
			   unsigned char offset, unsigned short color)
{
	if (format == FB_FORMAT_LUT_256)
		return gfbg_gop_update_256LUT(dev_id, layer_id, offset, color);
	else if (format == FB_FORMAT_LUT_16)
		return gfbg_gop_update_16LUT(dev_id, layer_id, offset, color);
	else
		return -1;
}

void gfbg_hal_set_oenc_cfg(struct oenc_cfg oenc_cfg)
{
	oenc_set_cfg(oenc_cfg);
	TRACE_GFBG(DBG_INFO, "%s: oenc_cfg\n", __func__);
}

void gfbg_hal_close_odec(vo_dev dev_id, vo_layer layer_id)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(dev_id, layer_id);

	cfg->odec_cfg.odec_ctrl.raw = 0;
	gfbg_gop_set_cfg(dev_id, layer_id, cfg, true);
}

void gfbg_hal_get_oenc(struct oenc_cfg *oenc_cfg)
{
	oenc_get_cfg(oenc_cfg);
}

void gfbg_hal_gop_odec_set_cfg_from_oenc(vo_layer layer_id, struct oenc_cfg oenc_cfg)
{
	struct disp_gop_cfg *cfg = gfbg_gop_get_cfg(0, layer_id);
	struct disp_gop_odec_cfg odec_cfg = {0};

	odec_cfg.odec_ctrl.b.odec_en = cfg->odec_cfg.odec_ctrl.b.odec_en = 1;
	odec_cfg.odec_ctrl.b.odec_int_en = 1;
	gfbg_gop_odec_set_cfg_from_oenc(odec_cfg, oenc_cfg);
}
