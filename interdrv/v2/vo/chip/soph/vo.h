#ifndef __VO_H__
#define __VO_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <linux/sys.h>
#include <linux/vo_uapi.h>
#include "cvi_vo_ctx.h"
#include "disp.h"
#include <vb.h>
#include <vo_sdk_layer.h>

#define VO_DV_BT_BLANKING_WIDTH(bt) \
	(bt->hfrontporch + bt->hsync + bt->hbackporch)
#define VO_DV_BT_FRAME_WIDTH(bt) \
	(bt->width + VO_DV_BT_BLANKING_WIDTH(bt))
#define VO_DV_BT_BLANKING_HEIGHT(bt) \
	(bt->vfrontporch + bt->vsync + bt->vbackporch + \
	 bt->il_vfrontporch + bt->il_vsync + bt->il_vbackporch)
#define VO_DV_BT_FRAME_HEIGHT(bt) \
	(bt->height + VO_DV_BT_BLANKING_HEIGHT(bt))


#define VO_DEV_INVALID              (-1)
#define VO_VIDEOLAYER_INVALID       (-1)

enum i80_op_type {
	I80_OP_GO = 0,
	I80_OP_TIMER,
	I80_OP_DONE,
	I80_OP_MAX,
};

enum i80_ctrl_type {
	I80_CTRL_CMD = 0,
	I80_CTRL_DATA,
	I80_CTRL_EOL = I80_CTRL_DATA,
	I80_CTRL_EOF,
	I80_CTRL_END = I80_CTRL_EOF,
	I80_CTRL_MAX
};

struct cvi_disp_buffer {
	struct vo_buffer buf;
	VB_BLK blk;
	VB_BLK blk_i80;
	struct list_head       list;
	__u32			sequence;
};

struct cvi_wbc_buffer {
	struct vo_wbc_buffer buf;
	struct list_head       list;
};

struct vo_disp_pattern {
	enum disp_pat_type type;
	enum disp_pat_color color;
	u16 rgb[3];
};

s32 vo_set_interface(VO_DEV VoDev, struct cvi_disp_intf_cfg *cfg);
s32 vo_start_streaming(VO_DEV VoDev);
s32 vo_stop_streaming(VO_DEV VoDev);
s32 vo_create_thread(VO_LAYER VoLayer);
s32 vo_destroy_thread(VO_LAYER VoLayer);
s32 vo_wbc_create_thread(VO_WBC VoWbc);
s32 vo_wbc_destroy_thread(VO_WBC VoWbc);
void vo_fill_disp_timing(struct disp_timing *timing, struct vo_bt_timings *bt_timing);
s32 vo_recv_frame(MMF_CHN_S chn, VB_BLK blk);

void vo_wbc_rdy_buf_queue(struct cvi_vo_wbc_ctx *pstWbcCtx, struct cvi_wbc_buffer *qbuf);
struct cvi_wbc_buffer *vo_wbc_rdy_buf_next(struct cvi_vo_wbc_ctx *pstWbcCtx);
int vo_wbc_rdy_buf_empty(struct cvi_vo_wbc_ctx *pstWbcCtx);
void vo_wbc_rdy_buf_pop(struct cvi_vo_wbc_ctx *pstWbcCtx);
void vo_wbc_rdy_buf_remove(struct cvi_vo_wbc_ctx *pstWbcCtx);
void vo_wbc_dqbuf_list(struct cvi_vo_wbc_ctx *pstWbcCtx);
int vo_wbc_dqbuf(struct cvi_vo_wbc_ctx *pstWbcCtx);


#ifdef __cplusplus
}
#endif

#endif /* __VO_H__ */
