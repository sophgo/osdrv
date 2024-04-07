#ifndef _CVI_VIP_LDC_H_
#define _CVI_VIP_LDC_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/ldc_uapi.h>
#include <ldc_common.h>

enum ldc_core_state {
	LDC_CORE_STATE_IDLE,
	LDC_CORE_STATE_RUNNING,
	LDC_CORE_STATE_END,
};

enum ldc_dev_state {
	LDC_DEV_STATE_RUNNING,
	LDC_DEV_STATE_STOP,
};

enum cvi_ldc_type {
	CVI_DEV_LDC_0 = 0,
	CVI_DEV_LDC_1,
	CVI_DEV_LDC_MAX,
};

enum ldc_wait_evt {
	LDC_EVENT_BUSY_OR_NOT_STAT = 0x0,
	LDC_EVENT_WKUP = 0x1,
	LDC_EVENT_EOF =  0x2,
	LDC_EVENT_RST =  0x4,
};

struct ldc_tsk_list {
	struct list_head work_list;
	struct list_head done_list;
};

struct ldc_core {
	enum cvi_ldc_type dev_type;
	int irq_num;
	struct clk *clk_sys;
	struct clk *clk;
	atomic_t state;//ldc_core_state
	struct ldc_tsk_list list;
	struct work_struct work_frm_done;
	wait_queue_head_t cmdq_wq;
	bool cmdq_evt;
};

struct ldc_job_list {
	struct list_head work_list[LDC_DEV_MAX_CNT];
	struct list_head done_list[LDC_DEV_MAX_CNT];
};

struct cvi_ldc_vdev {
	struct miscdevice miscdev;
	atomic_t state;//ldc_dev_state
	struct ldc_core core[LDC_DEV_MAX_CNT];
	int core_num;
	//atomic_t cur_irq_core_id;
	u32 clk_sys_freq;
	void *shared_mem;
	struct task_struct *thread;
	wait_queue_head_t wait;
	enum ldc_wait_evt evt;
	spinlock_t job_lock;
	struct list_head job_list;
	int job_cnt;
	//struct workqueue_struct *workqueue;
	struct ldc_job_list list;
	struct ldc_vb_doneq vb_doneq;
	VB_POOL VbPool;
};

struct cvi_ldc_vdev *ldc_get_dev(void);
struct fasync_struct *ldc_get_dev_fasync(void);

#endif /* _CVI_VIP_LDC_H_ */
