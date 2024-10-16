#ifndef _DWA_CORE_H_
#define _DWA_CORE_H_

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

enum dwa_type {
	DEV_DWA_0 = 0,
	DEV_DWA_1,
	DEV_DWA_MAX,
};

enum dwa_wait_evt {
	DWA_EVENT_BUSY_OR_NOT_STAT = 0x0,
	DWA_EVENT_WKUP = 0x1,
	DWA_EVENT_EOF =  0x2,
	DWA_EVENT_RST =  0x4,
};

struct dwa_core {
	spinlock_t core_lock;
	enum dwa_type dev_type;
	int irq_num;
	struct clk *clk_src;
	struct clk *clk_apb;
	struct clk *clk_dwa;
	unsigned int clk_sys_freq;
	atomic_t state;//dwa_core_state
	struct list_head list;
#if DWA_USE_WORKQUEUE
	struct work_struct work_frm_done;
#endif
	wait_queue_head_t cmdq_wq;
	bool cmdq_evt;
};

struct dwa_vdev {
	struct miscdevice miscdev;
	atomic_t state;//dwa_dev_state
	struct dwa_core core[DWA_DEV_MAX_CNT];
	int core_num;
	//atomic_t cur_irq_core_id;
	void *shared_mem;
	struct task_struct *thread;
	wait_queue_head_t wait;
	enum dwa_wait_evt evt;
	struct list_head job_list;
	int job_cnt;
	spinlock_t wdev_lock;
	//struct workqueue_struct *workqueue;
	struct dwa_vb_doneq vb_doneq;
	vb_pool vb_pool;
	struct semaphore sem;//for suspend
};

struct dwa_vdev *dwa_get_dev(void);
struct fasync_struct *dwa_get_dev_fasync(void);
void dwa_enable_dev_clk(int coreid, bool en);
void dwa_core_init(int top_id);
void dwa_core_deinit(int top_id);
void dwa_dev_init(struct dwa_vdev *dev);
void dwa_dev_deinit(struct dwa_vdev *dev);
int dwa_suspend(struct device *dev);
int dwa_resume(struct device *dev);
bool is_dwa_suspended(void);

#endif /* _VIP_DWA_H_ */
