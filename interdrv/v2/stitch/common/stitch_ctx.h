#ifndef __U_CVI_STITCH_CTX_H__
#define __U_CVI_STITCH_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <linux/cvi_defines.h>
#include <linux/cvi_comm_stitch.h>
#include <linux/cvi_base_ctx.h>
#include "linux/cvi_comm_vb.h"
#include "base_ctx.h"

enum stitch_handler_state {
	STITCH_HANDLER_STATE_STOP = 0,
	STITCH_HANDLER_STATE_START,
	STITCH_HANDLER_STATE_RUN,
	STITCH_HANDLER_STATE_RUN_STAGE2,
	STITCH_HANDLER_STATE_DONE,
	STITCH_HANDLER_STATE_SUSPEND,
	STITCH_HANDLER_STATE_RESUME,
	STITCH_HANDLER_STATE_MAX,
};

struct stitch_work_status {
	u32 recv_cnt;
	u32 done_cnt;
	u32 lost_cnt;
	u32 fail_recv_cnt; //start job fail cnt
	u32 cost_time; // current job cost time in us
	u32 max_cost_time;
	u32 hw_cost_time; // current job Hw cost time in us
	u32 hw_max_cost_time;
	u32 duration;
	u32 u32SwDuration;
	u32 u32HwDuration;
};

typedef void (*stitch_job_cb)(void *data);

enum stitch_job_state {
	STITCH_JOB_WAIT,
	STITCH_JOB_WORKING,
	STITCH_JOB_END,
	STITCH_JOB_INVALID,
};

enum stitch_update_status {
	STITCH_UPDATE_SRC = 1 << 0,
	STITCH_UPDATE_CHN = 1 << 1,
	STITCH_UPDATE_OP = 1 << 2,
	STITCH_UPDATE_WGT = 1 << 3,
};

struct cvi_stitch_job {
	stitch_job_cb pfnJobCB;
	void *data;
	atomic_t enJobState; //stitch_job_state
	struct timespec64 ts_start;
	struct timespec64 ts_end;
	struct list_head list;
};

struct cvi_stitch_ctx {
	atomic_t enHdlState;//specific enum stitch_handler_state
	STITCH_SRC_ATTR_S src_attr;
	u8 src_num;
	STITCH_CHN_ATTR_S chn_attr;
	SIZE_S tmp_chn_size[STITCH_MAX_SRC_NUM - 1];
	STITCH_OP_ATTR_S op_attr;
	STITCH_WGT_ATTR_S wgt_attr;
	VB_POOL VbPool;
	u32 u32VBSize;
	u8 isCreated;
	u8 isStarted;
	u8 param_update;
	u8 update_status;//specific enum stitch_update_status
	spinlock_t lock;
	struct stitch_work_status work_status;
	struct cvi_stitch_job job;
	struct work_struct work_frm_done;
	struct timespec64 time;
	struct vb_s *vb_out;
	wait_queue_head_t wq;
	u8 evt;
};

struct stitch_handler_ctx {
	spinlock_t lock;
	wait_queue_head_t wait;
	struct task_struct *thread;
	struct workqueue_struct *workqueue;
	enum stitch_src_id src_id;
	u8 prepared_flag;//bit[n] set 1, mean src[n] prepared
	u8 events;
};

#ifdef __cplusplus
}
#endif

#endif /* __U_CVI_STITCH_CTX_H__ */
