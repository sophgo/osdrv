#ifndef __RGN_H__
#define __RGN_H__

#include "common.h"
#include "comm_video.h"
#include "comm_vpss.h"
#include "comm_region.h"
#include "rgn_uapi.h"
#include "base_cb.h"
#include "base_ctx.h"
#include "rgn_debug.h"
#include "rgn_core.h"

/*********************************************************************************************/
/* Configured from user, IOCTL */
int rgn_create(rgn_handle handle, const rgn_attr_s *pregion);
int rgn_destory(rgn_handle handle);
int rgn_get_attr(rgn_handle handle, rgn_attr_s *pregion);
int rgn_set_attr(rgn_handle handle, const rgn_attr_s *pregion);
int rgn_set_bit_map(rgn_handle handle, const bitmap_s *pbitmap);
int rgn_attach_to_chn(rgn_handle handle, const mmf_chn_s *pchn, const rgn_chn_attr_s *pchn_attr);
int rgn_detach_from_chn(rgn_handle handle, const mmf_chn_s *pchn);
int rgn_set_display_attr(rgn_handle handle, const mmf_chn_s *pchn, const rgn_chn_attr_s *pchn_attr);
int rgn_get_display_attr(rgn_handle handle, const mmf_chn_s *pchn, rgn_chn_attr_s *pchn_attr);
int rgn_get_canvas_info(rgn_handle handle, rgn_canvas_info_s *pcanvas_info);
int rgn_update_canvas(rgn_handle handle);
int rgn_invert_color(rgn_handle handle, mmf_chn_s *pchn, unsigned int *pcolor);
int rgn_set_chn_palette(rgn_handle handle, const mmf_chn_s *pchn, rgn_palette *ppalette,
			rgn_rgbquad_s *input_pixel_table);

/* INTERNAL */
int32_t _rgn_init(void);
int32_t _rgn_exit(void);
unsigned int _rgn_proc_get_idx(rgn_handle hhandle);
bool is_rect_overlap(rect_s *r0, rect_s *r1);
unsigned char _rgn_check_order(rect_s *r0, rect_s *r1);
int _rgn_insert(rgn_handle hdls[], unsigned char size, rgn_handle hdl);
int _rgn_ex_insert(rgn_handle hdls[], unsigned char size, rgn_handle hdl);
int _rgn_remove(rgn_handle hdls[], unsigned char size, rgn_handle hdl);
void _rgn_fill_pattern(void *buf, unsigned int len, unsigned int color, unsigned char bpp);
int _rgn_check_chn_attr(rgn_handle handle, const mmf_chn_s *pchn, const rgn_chn_attr_s *pchn_attr);

#endif /* __RGN_H__ */
