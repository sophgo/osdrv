#ifndef _VO_CTX_H_
#define _VO_CTX_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "osal.h"
#include "comm_vo.h"
#include "vbq.h"

struct vo_chn_ctx {
	bool is_chn_enable;
	bool hide;
	bool pause;
	bool is_drop;
	vo_chn_attr_s chn_attr;
	unsigned int src_width;
	unsigned int src_height;
	rotation_e rotation;

	struct vb_jobs_t chn_jobs;
	struct gdc_mesh mesh;

	unsigned long long display_pts;
	unsigned long long predone_pts;
	unsigned int frame_num;
	unsigned int src_frame_num;
	unsigned int frame_rate;
	unsigned int src_frame_rate;
	unsigned int frame_rate_user_set;
	unsigned int frame_index;
	unsigned int threshold;
};

struct vo_layer_ctx {
	vo_layer layer;
	bool is_layer_enable;
	int bind_dev_id;
	bool is_drop;
	vo_video_layer_attr_s layer_attr;
	unsigned int display_buflen;
	int proc_amp[PROC_AMP_MAX];
	struct vo_chn_ctx chn_ctx[VO_MAX_CHN_NUM];
	osal_mutex layer_lock;
	osal_task *thread;
	osal_wait wq;
	unsigned int event;
	osal_spinlock list_lock;
	struct osal_list_head list_wait;
	struct osal_list_head list_work;
	struct osal_list_head list_done;

	unsigned int frame_num;
	unsigned int src_frame_num;
	unsigned int frame_rate;
	unsigned int src_frame_rate;
	unsigned int frame_index;
	unsigned int done_cnt;
	unsigned long long display_pts;
	unsigned long long predone_pts;
	unsigned int bw_fail;
	unsigned int vgop_bw_fail;

	struct{
		unsigned int left;
		unsigned int top;
		unsigned int width;
		unsigned int height;
	} rect_crop;
};

struct vo_overlay_ctx {
	bool enable;
	int bind_dev_id;
	unsigned int priority;
};

struct vo_dev_ctx {
	//paltform
	vo_dev dev_id;
	int irq_num;
	bool disp_online;
	unsigned int frame_number;
	osal_atomic disp_streamon;
	//sdk
	bool is_dev_enable;
	vo_pub_attr_s pub_attr;
	int bind_layer_id;
	int bind_overlay_id[VO_MAX_GRAPHIC_LAYER_IN_DEV];
	vo_lvds_attr_s lvds_param;
	vo_bt_attr_s bt_param;
};

struct vo_ctx {
	bool suspend;
	osal_clk *clk_vo[3];
	struct vo_dev_ctx dev_ctx[VO_MAX_DEV_NUM];
	struct vo_layer_ctx layer_ctx[VO_MAX_VIDEO_LAYER_NUM];
	struct vo_overlay_ctx overlay_ctx[VO_MAX_GRAPHIC_LAYER_NUM];
};

extern struct vo_ctx *g_vo_ctx;

#ifdef __cplusplus
}
#endif

#endif /* __VO_CTX_H__ */
