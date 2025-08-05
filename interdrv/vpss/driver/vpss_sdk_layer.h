#ifndef __VPSS_SDK_LAYER__
#define __VPSS_SDK_LAYER__

#include "vpss_uapi.h"
#include "vpss_core.h"

int vpss_set_mode(const vpss_mode_s *mode, struct vpss_cores *cores);
int vpss_get_mode(vpss_mode_s *mode, struct vpss_cores *cores);

int vpss_set_mod_param(const vpss_mod_param_s *mod_param_info, struct vpss_ctx *ctx);
int vpss_get_mod_param(vpss_mod_param_s *mod_param_info, struct vpss_ctx *ctx);

//grp api
vpss_grp vpss_get_available_grp(struct vpss_cores *cores);
int vpss_create_grp(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr, struct vpss_cores *cores);
int vpss_destroy_grp(vpss_grp grp_id, struct vpss_cores *cores);
int vpss_start_grp(vpss_grp grp_id, struct vpss_ctx *ctx);
int vpss_stop_grp(vpss_grp grp_id, struct vpss_ctx *ctx);
int vpss_reset_grp(vpss_grp grp_id, struct vpss_ctx *ctx);
int vpss_get_grp_attr(vpss_grp grp_id, vpss_grp_attr_s *grp_attr, struct vpss_ctx *ctx);
int vpss_set_grp_attr(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr, struct vpss_ctx *ctx);
int vpss_get_grp_crop(vpss_grp grp_id, vpss_crop_info_s *crop_info, struct vpss_ctx *ctx);
int vpss_set_grp_crop(vpss_grp grp_id, const vpss_crop_info_s *crop_info, struct vpss_ctx *ctx);
int vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg, struct vpss_ctx *ctx);
int vpss_get_proc_amp_ctrl(proc_amp_e type, proc_amp_ctrl_s *ctrl);
int vpss_get_proc_amp(vpss_grp grp_id, int *proc_amp, struct vpss_ctx *ctx);
int vpss_get_all_proc_amp(vpss_all_proc_amp_s *cfg, struct vpss_ctx *ctx);
int vpss_send_frame(vpss_grp grp_id, const video_frame_info_s *video_frame, int milli_sec, struct vpss_ctx *ctx);

//chn api
int vpss_set_chn_attr(vpss_grp grp_id, vpss_chn chn_id, const vpss_chn_attr_s *chn_attr, struct vpss_ctx *ctx);
int vpss_get_chn_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_chn_attr_s *chn_attr, struct vpss_ctx *ctx);
int vpss_enable_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx);
int vpss_disable_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx);
int vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg, struct vpss_ctx *ctx);
int vpss_set_chn_crop(vpss_grp grp_id, vpss_chn chn_id, const vpss_crop_info_s *crop_info, struct vpss_ctx *ctx);
int vpss_get_chn_crop(vpss_grp grp_id, vpss_chn chn_id, vpss_crop_info_s *crop_info, struct vpss_ctx *ctx);
int vpss_show_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx);
int vpss_hide_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx);
int vpss_send_chn_frame(vpss_grp grp_id, vpss_chn chn_id
	, const video_frame_info_s *video_frame, int milli_sec, struct vpss_ctx *ctx);
int vpss_get_chn_frame(vpss_grp grp_id, vpss_chn chn_id, video_frame_info_s *frame_info,
			   int milli_sec, struct vpss_ctx *ctx);
int vpss_release_chn_frame(vpss_grp grp_id, vpss_chn chn_id,
		const video_frame_info_s *video_frame);
int vpss_set_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation, struct vpss_ctx *ctx);
int vpss_get_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e *rotation, struct vpss_ctx *ctx);
int vpss_set_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int align, struct vpss_ctx *ctx);
int vpss_get_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int *palign, struct vpss_ctx *ctx);
int vpss_set_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id,
		vpss_scale_coef_e coef, struct vpss_ctx *ctx);
int vpss_get_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id,
		vpss_scale_coef_e *coef, struct vpss_ctx *ctx);
int vpss_set_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_draw_rect_s *draw_rect, struct vpss_ctx *ctx);
int vpss_get_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id,
		vpss_draw_rect_s *draw_rect, struct vpss_ctx *ctx);
int vpss_set_chn_convert(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_convert_s *convert, struct vpss_ctx *ctx);
int vpss_get_chn_convert(vpss_grp grp_id, vpss_chn chn_id,
		vpss_convert_s *convert, struct vpss_ctx *ctx);
int vpss_set_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int y_ratio, struct vpss_ctx *ctx);
int vpss_get_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int *y_ratio, struct vpss_ctx *ctx);
int vpss_set_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id,
				const vpss_ldc_attr_s *ldc_attr, unsigned long long mesh_addr, struct vpss_ctx *ctx);
int vpss_get_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_ldc_attr_s *ldc_attr, struct vpss_ctx *ctx);
int vpss_attach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, vb_pool vb_pool, struct vpss_ctx *ctx);
int vpss_detach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx);

int vpss_set_chn_bufwrap_attr(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_chn_buf_wrap_s *buf_wrap, struct vpss_ctx *ctx);
int vpss_get_chn_bufwrap_attr(vpss_grp grp_id, vpss_chn chn_id,
		vpss_chn_buf_wrap_s *buf_wrap, struct vpss_ctx *ctx);

void vpss_release_all_grp(struct vpss_ctx *ctx);

#endif
