#ifndef __U_VPSS_CTX_H__
#define __U_VPSS_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include "defines.h"
#include "osal.h"
#include "comm_vpss.h"
#include "comm_region.h"
#include "base_ctx.h"
#include "vbq.h"
#include "vpss_hal.h"
#include "ldc_cb.h"

#define VPSS_ONLINE_JOB_NUM 2

#define PROC_AMP_MIN_VALUE 0
#define PROC_AMP_MAX_VALUE 100
#define PROC_AMP_DEFAULT_VALUE 50
#define PROC_AMP_STEP 1

#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_VI_ERR     0x0004

#ifndef FPGA_PORTING
#define IDLE_TIMEOUT_MS      1000
#define EOF_WAIT_TIMEOUT_MS  1000
#else
#define IDLE_TIMEOUT_MS      60000
#define EOF_WAIT_TIMEOUT_MS  60000
#endif

struct vpss_jobs_ctx {
	struct vb_jobs_t ins;
	struct vb_jobs_t outs[VPSS_MAX_CHN_NUM];
};

struct vpss_handler_ctx {
	osal_wait wait;
	osal_task *thread;
	osal_spinlock hdl_lock;
	osal_atomic active_cnt;
	u32 stop_flag;
	u8 events;
	u8 is_suspend;
};

struct _vpss_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

struct vpss_sbm_ctx {
	u32 sb_mode;
	u64 phy_addr[3];
	u32 stride[3];
};

struct vpss_stitch_data {
	osal_wait wait;
	unsigned char flag;
};

enum handler_state {
	HANDLER_STATE_STOP = 0,
	HANDLER_STATE_RUN,
	HANDLER_STATE_SUSPEND,
	HANDLER_STATE_RESUME,
	HANDLER_STATE_MAX,
};

struct vpss_grp_work_status_s {
	u32 recv_cnt;
	u32 frc_recv_cnt;
	u32 lost_cnt;
	u32 start_fail_cnt; //start job fail cnt
	u32 cost_time; // current job cost time in us
	u32 max_cost_time;
	u32 hw_cost_time; // current job Hw cost time in us
	u32 hw_max_cost_time;
};

struct vpss_chn_work_status_s {
	u32 send_ok; // send OK cnt after latest chn enable
	u64 prev_time; // latest time (us)
	u32 frame_num;  //The number of Frame in one second
	u32 real_frame_rate; // chn real time frame rate
};

struct vpss_chn_ctx {
	u8 is_enabled;
	u8 is_muted;
	u8 is_drop;
	u8 is_cfg_changed;
	u32 blk_size;
	u32 align;
	u32 y_ratio;
	u32 vb_pool;
	vpss_chn_attr_s chn_attr;
	vpss_crop_info_s crop_info;
	rotation_e rotation;
	vpss_scale_coef_e coef;
	vpss_draw_rect_s draw_rect;
	vpss_convert_s convert;
	vpss_ldc_attr_s ldc_attr;
	vpss_chn_buf_wrap_s buf_wrap;
	struct vpss_sbm_ctx sbm_ctx;
	struct csc_cfg csc;
	struct gdc_mesh mesh;

	//ext
	struct vpss_chn_work_status_s chn_work_status;
	struct mlv_i_s mlv;

	//rgn cfg
	rgn_handle rgn_handle[RGN_MAX_LAYER_VPSS][RGN_MAX_NUM_VPSS]; //only overlay
	rgn_handle cover_ex_handle[RGN_COVEREX_MAX_NUM];
	rgn_handle mosaic_handle[RGN_MOSAIC_MAX_NUM];
	struct rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct rgn_coverex_cfg rgn_coverex_cfg;
	struct rgn_mosaic_cfg rgn_mosaic_cfg;
};

FIFO_HEAD(vpssjobq, vpss_job*);

struct vpss_grp_ctx {
	vpss_grp grp_id;
	u8 dev_id;
	u8 chn_max_num;
	u8 is_created;
	u8 is_started;
	u8 online_from_isp;
	u8 is_cfg_changed;
	u8 is_copy_upsample;

	osal_mutex lock;
	//status
	osal_timeval time;
	osal_atomic hdl_state;
	struct vpss_grp_work_status_s grp_work_status;
	//attr
	vpss_grp_attr_s grp_attr;
	struct vpss_chn_ctx chn_ctxs[VPSS_MAX_CHN_NUM];
	//csc
	struct csc_cfg csc;
	int proc_amp[PROC_AMP_MAX];
	//crop
	vpss_crop_info_s grp_crop_info;
	struct crop_size frame_crop;
	s16 offset_top;
	s16 offset_bottom;
	s16 offset_left;
	s16 offset_right;
	//job
	void *job_buffer;
	struct vpssjobq jobq;
	//handle
	void *hal_ctx_ptr;
	void *hdl_ctx_ptr;
	//vbq
	struct vpss_jobs_ctx vb_jobs;
	//hw cfg
	struct vpss_hw_cfg hw_cfg;
};

struct vpss_ctx {
	struct vpss_grp_ctx *grp_ctx[VPSS_MAX_GRP_NUM];
	struct vpss_handler_ctx hdl_ctx;
	bool grp_used[VPSS_MAX_GRP_NUM];
	vpss_mod_param_s mod_param;
	u64 sb_phy_addr;
	u32 sb_width;
	u32 sb_height;
	u32 sb_buf_line;
	u32 sb_buffer_size;
	osal_mutex lock;
};

#ifdef __cplusplus
}
#endif

#endif /* __U_VPSS_CTX_H__ */
