#ifndef __VPSS_HAL_H__
#define __VPSS_HAL_H__

#include "base_ctx.h"
#include "defines.h"
#include "vpss_define.h"
#include "vpss_cb.h"
#include "osal.h"
#include "vpss_ip_ctrl.h"

#define VPSS_CMDQ_BUF_SIZE (0x8000)

enum job_state {
	JOB_WAIT,
	JOB_WORKING,
	JOB_HALF,
	JOB_END,
	JOB_INVALID,
};

enum vpss_state {
	VPSS_IDLE,
	VPSS_RUNNING,
	VPSS_END,
};

struct vpss_online_cb {
	__u8 snr_num;
	__u8 is_tile;
	__u8 is_left_tile;
	__u64 frm_num;
	struct vip_line l_in;
	struct vip_line l_out;
	struct vip_line r_in;
	struct vip_line r_out;
	osal_timeval ts;
};

struct vpss_hw_cfg {
	__u8 chn_enable[VPSS_MAX_CHN_NUM];
	struct vpss_img_in_cfg grp_cfg;
	struct vpss_sc_cfg chn_cfg[VPSS_MAX_CHN_NUM];
};

struct vpss_job_info {
	__u32 hw_duration;
	__u32 checksum[VPSS_MAX_CHN_NUM];
};

typedef void (*vpss_job_cb)(void *data);

struct vpss_job {
	__u8 grp_id;
	__u8 dev_id;
	struct vpss_hw_cfg cfg;
	struct vpss_job_info info;
	struct vpss_online_cb online_param;
	vpss_job_cb job_cb;
	void *data;
	osal_spinlock lock;
	osal_atomic job_state;
	struct osal_list_head list;
	osal_workqueue work;
};

struct vpss_hal_ctx {
	osal_spinlock task_lock;
	struct osal_list_head job_wait_queue;
	struct osal_list_head job_online_queue;
	struct vpss_cmdq_buf cmdq_buf;
	struct vpss_device *online_dev;
	u8 is_suspend;
};

struct vpss_device {
	u8 id;
	u8 core_num;
	struct vpss_core *core_list[VPSS_MAX];
	osal_spinlock dev_lock;
	osal_timeval ts_start;
	osal_timeval ts_end;
	osal_atomic state;
	u32 start_cnt;
	u32 int_cnt;
	u32 hw_duration;
	u32 hw_duration_total;
	u32 duty_ratio;
	u8 tile_mode;
	bool is_online;
	bool is_tile;
	bool is_work_on_r_tile;
	bool isp_triggered;
	void *job;
};


int vpss_hal_init(struct vpss_hal_ctx *hal_ctx);
void vpss_hal_deinit(struct vpss_hal_ctx *hal_ctx);
int vpss_hal_push_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx);
int vpss_hal_push_online_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx);
int vpss_hal_remove_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx);
int vpss_hal_try_schedule(struct vpss_hal_ctx *hal_ctx);
int vpss_hal_stitch_schedule(struct vpss_stitch_cfg *cfg, struct vpss_hal_ctx *hal_ctx);
int vpss_hal_online_run(struct vpss_online_cb *param, struct vpss_hal_ctx *hal_ctx);

void vpss_hal_job_finish(struct vpss_device *device);
int vpss_hal_reset(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx);

#endif // _U_SC_UAPI_H_
