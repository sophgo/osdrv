#ifndef __VO_PROCESS_H__
#define __VO_PROCESS_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "vo_uapi.h"
#include "vo_ctx.h"
#include "disp.h"

#define VO_DV_BT_BLANKING_WIDTH(bt) \
	(bt->hfrontporch + bt->hsync + bt->hbackporch)
#define VO_DV_BT_FRAME_WIDTH(bt) \
	(bt->width + VO_DV_BT_BLANKING_WIDTH(bt))
#define VO_DV_BT_BLANKING_HEIGHT(bt) \
	(bt->vfrontporch + bt->vsync + bt->vbackporch + \
	 bt->il_vfrontporch + bt->il_vsync + bt->il_vbackporch)
#define VO_DV_BT_FRAME_HEIGHT(bt) \
	(bt->height + VO_DV_BT_BLANKING_HEIGHT(bt))

struct disp_buffer {
	struct vo_buffer buf;
	vb_blk blk;
	struct osal_list_head list;
	unsigned int sequence;
};

struct disp_pattern {
	enum disp_pat_type type;
	enum disp_pat_color color;
	unsigned short rgb[3];
};

int vo_core_register_cb(void *dev);
int vo_core_rm_cb(void);
int vo_process_init_ctx(struct vo_ctx *ctx);
int vo_process_deinit_ctx(struct vo_ctx *ctx);
int vo_create_thread(vo_layer layer);
int vo_destroy_thread(vo_layer layer);
void vo_fill_disp_timing(struct disp_timing *timing, struct vo_bt_timings *bt_timing);
int vo_recv_frame(mmf_chn_s chn, vb_blk blk, void *data);
int vo_start_streaming(vo_dev dev);
int vo_stop_streaming(vo_dev dev);

#ifdef __cplusplus
}
#endif

#endif /* __VO_PROCESS_H__ */
