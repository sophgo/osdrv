#ifndef __STITCH_CTX_H__
#define __STITCH_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>

#include <defines.h>
#include <comm_stitch.h>
#include <base_ctx.h>
#include "comm_vb.h"

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
	unsigned int recv_cnt;
	unsigned int done_cnt;
	unsigned int lost_cnt;
	unsigned int fail_recv_cnt; //start job fail cnt
	unsigned int cost_time; // current job cost time in us
	unsigned int max_cost_time;
	unsigned int hw_cost_time; // current job Hw cost time in us
	unsigned int hw_max_cost_time;
	unsigned int duration;
	unsigned int sw_duration;
	unsigned int hw_duration;
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

struct stitch_job {
	stitch_job_cb fn_job_cb;
	void *data;
	atomic_t job_state; //stitch_job_state
	struct timespec64 ts_start;
	struct timespec64 ts_end;
	struct list_head list;
};

struct __stitch_ctx {
	atomic_t hdl_state;//specific enum stitch_handler_state
	stitch_src_attr src_attr;
	unsigned char src_num;
	stitch_chn_attr chn_attr;
	size_s tmp_chn_size[STITCH_MAX_SRC_NUM - 1];
	stitch_op_attr op_attr;
	stitch_bld_wgt_attr wgt_attr;
	vb_pool vb_pool;
	unsigned int vb_size;
	unsigned char is_created;
	unsigned char is_started;
	unsigned char param_update;
	unsigned char update_status;//specific enum stitch_update_status
	spinlock_t lock;
	struct stitch_work_status work_status;
	struct stitch_job job;
	struct work_struct work_frm_done;
	struct timespec64 time;
	struct vb_s *vb_out;
	atomic_t enable_count;
	struct mutex io_lock;
};

struct stitch_handler_ctx {
	spinlock_t lock;
	wait_queue_head_t wait;
	struct semaphore sem_2nd;
	struct task_struct *thread;
	struct workqueue_struct *workqueue;
	enum stitch_src_id src_id;
	unsigned char prepared_flag;//bit[n] set 1, mean src[n] prepared
	unsigned char events;
};

#ifdef __cplusplus
}
#endif

#endif /* __STITCH_CTX_H__ */
