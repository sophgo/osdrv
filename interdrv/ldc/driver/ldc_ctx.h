#ifndef _LDC_CTX_H_
#define _LDC_CTX_H_

#include "comm_sys.h"
#include "comm_vb.h"
#include "base_ctx.h"
#include "osal.h"
#include "ldc_common.h"

#define MAX_CB_MOD_NUM 3
#define MAX_CB_DEV_NUM MAX(VO_MAX_LAYER_NUM, MAX(VPSS_MAX_GRP_NUM, VI_MAX_PIPE_NUM))
#define MAX_CB_CHN_NUM MAX(VO_MAX_CHN_NUM, MAX(VPSS_MAX_CHN_NUM, VI_MAX_CHN_NUM))

enum ldc_core_state {
	LDC_CORE_STATE_IDLE,
	LDC_CORE_STATE_RUNNING,
	LDC_CORE_STATE_END,
};

enum ldc_dev_state {
	LDC_DEV_STATE_RUNNING,
	LDC_DEV_STATE_STOP,
};

enum ldc_type {
	DEV_LDC_0 = 0,
	DEV_LDC_1,
	DEV_DWA_0,
	DEV_DWA_1,
	DEV_LDC_MAX,
};

enum ldc_wait_evt {
	LDC_EVENT_BUSY_OR_NOT_STAT = 0x0,
	LDC_EVENT_WKUP = 0x1,
	LDC_EVENT_EOF =  0x2,
	LDC_EVENT_RST =  0x4,
};

struct ldc_core {
	osal_spinlock core_lock;
	enum ldc_type dev_type;
	int irq_num;
	unsigned int clk_sys_freq;
	osal_atomic state;//ldc_core_state
	struct osal_list_head list;
#if LDC_USE_WORKQUEUE
	osal_workqueue work_frm_done;
#endif
	osal_wait cmdq_wq;
	bool cmdq_evt;
	osal_clk *ldc_clk;
	osal_atomic clk_en;
};

struct ldc_ctx {
	osal_atomic state;//ldc_dev_state
	struct ldc_core core[LDC_DEV_MAX_CNT];
	int core_num;
	//osal_atomic cur_irq_core_id;
	void *shared_mem;
	osal_task *thread;
	osal_wait wait;
	enum ldc_wait_evt evt;
	u32 stop_flag;
	struct osal_list_head job_list;
	int job_cnt;
	osal_spinlock ctx_lock;
	//struct workqueue_struct *workqueue;
	struct ldc_vb_doneq vb_doneq;
	vb_pool vb_pool[MAX_CB_MOD_NUM][MAX_CB_DEV_NUM][MAX_CB_CHN_NUM];
	osal_semaphore sem;
	bool thread_created;
	osal_timer timer;
	bool suspend;
};
#endif
