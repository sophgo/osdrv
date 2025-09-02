#ifndef GFBG_HAL_H
#define GFBG_HAL_H

#include "gfbg_main.h"
#include "disp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void gfbg_hal_set_layer_enable(bool enable, vo_dev dev_id, vo_layer layer_id);
void gfbg_hal_set_layer_data_fmt(vo_dev dev_id, vo_layer layer_id,
				 enum disp_gop_format pixel_format_for_hal);
void gfbg_hal_set_layer_stride(vo_dev dev_id, vo_layer layer_id, unsigned int stride);
void gfbg_hal_set_layer_rect(vo_dev dev_id, vo_layer layer_id, fb_rect *rect,
			     const gfbg_display_info *display_info);
void gfbg_hal_set_layer_zoom(vo_dev dev_id, vo_layer layer_id, bool hscl_en, bool vscl_en,
			     const gfbg_display_info *display_info, int rot);
void gfbg_hal_set_layer_addr(vo_dev dev_id, vo_layer layer_id, phys_addr_t addr);
void gfbg_hal_set_layer_colorkey(vo_dev dev_id, vo_layer layer_id,
				 const gfbg_colorkeyex *colorkey);
int gfbg_hal_set_color_reg(vo_dev dev_id, vo_layer layer_id, fb_color_format format,
			   unsigned char offset, unsigned short color);
void gfbg_hal_set_oenc_cfg(struct oenc_cfg oenc_cfg);
void gfbg_hal_close_odec(vo_dev dev_id, vo_layer layer_id);
void gfbg_hal_get_oenc(struct oenc_cfg *oenc_cfg);
void gfbg_hal_gop_odec_set_cfg_from_oenc(vo_layer layer_id, struct oenc_cfg oenc_cfg);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
