#ifndef _CVI_VIP_DWA_H_
#define _CVI_VIP_DWA_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/dwa_uapi.h>
#include <dwa_common.h>

enum dwa_core_state {
	DWA_CORE_STATE_IDLE,
	DWA_CORE_STATE_RUNNING,
	DWA_CORE_STATE_END,
};

enum dwa_dev_state {
	DWA_DEV_STATE_RUNNING,
	DWA_DEV_STATE_STOP,
};

enum cvi_dwa_type {
	CVI_DEV_DWA_0 = 0,
	CVI_DEV_DWA_1,
	CVI_DEV_DWA_MAX,
};

enum dwa_wait_evt {
	DWA_EVENT_BUSY_OR_NOT_STAT = 0x0,
	DWA_EVENT_WKUP = 0x1,
	DWA_EVENT_EOF =  0x2,
	DWA_EVENT_RST =  0x4,
};

struct dwa_tsk_list {
	struct list_head work_list;
	struct list_head done_list;
};

struct dwa_core {
	enum cvi_dwa_type dev_type;
	int irq_num;
	struct clk *clk_sys;
	struct clk *clk;
	atomic_t state;//dwa_core_state
	struct dwa_tsk_list list;
#if DWA_USE_WORKQUEUE
	struct work_struct work_frm_done;
#endif
	wait_queue_head_t cmdq_wq;
	bool cmdq_evt;
};

struct dwa_job_list {
	struct list_head work_list[DWA_DEV_MAX_CNT];
	struct list_head done_list[DWA_DEV_MAX_CNT];
};

struct cvi_dwa_vdev {
	struct miscdevice miscdev;
	atomic_t state;//dwa_dev_state
	struct dwa_core core[DWA_DEV_MAX_CNT];
	int core_num;
	//atomic_t cur_irq_core_id;
	struct clk *clk_src[DWA_DEV_MAX_CNT];
	struct clk *clk_apb[DWA_DEV_MAX_CNT];
	struct clk *clk_dwa[DWA_DEV_MAX_CNT];
	u32 clk_sys_freq[DWA_DEV_MAX_CNT];
	void *shared_mem;
	struct task_struct *thread;
	wait_queue_head_t wait;
	enum dwa_wait_evt evt;
	spinlock_t job_lock;
	struct list_head job_list;
	int job_cnt;
	//struct workqueue_struct *workqueue;
	struct dwa_job_list list;
	struct dwa_vb_doneq vb_doneq;
	VB_POOL VbPool;
	struct semaphore sem;
};

struct cvi_dwa_vdev *dwa_get_dev(void);
struct fasync_struct *dwa_get_dev_fasync(void);
void dwa_enable_dev_clk(int coreid, bool en);
void dwa_core_init(int top_id);
void dwa_core_deinit(int top_id);
void dwa_dev_init(struct cvi_dwa_vdev *dev);
void dwa_dev_deinit(struct cvi_dwa_vdev *dev);
int dwa_suspend(struct device *dev);
int dwa_resume(struct device *dev);
bool is_dwa_suspended(void);

#endif /* _CVI_VIP_DWA_H_ */
