#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/vmalloc.h>

#include "base_common.h"
#include "reg.h"
#include "cdma.h"

#define CDMA_CHL_CTRL		 0x0800
#define CDMA_INT_MAST		 0x0808
#define CDMA_INT_STATUS 	 0x080C
#define CDMA_CMD_FIFO_STATUS 0x0810
#define CDMA_MAX_PAYLOAD	 0x09F8
#define CDMA_CFG			 0x0878
#define CDMA_STRIDE 		 0x087c
#define CDMA_SIZE			 0x0880
#define CDMA_SRC_ADDR_L 	 0x0884
#define CDMA_SRC_ADDR_H 	 0x0888
#define CDMA_DST_ADDR_L 	 0x088c
#define CDMA_DST_ADDR_H 	 0x0890
#define CDMA_DATA_LENGTH	 0x0894

#define FIFO_FULL_STATUS  0xff

union cdma_chl_ctrl {
	struct {
		u32 cdma_enable : 1;
		u32 resv1       : 2;
		u32 int_enable	: 1;
		u32 resv4       : 3;
		u32 QoS_mode           : 1;
		u32 QoS_dynamic_h_valu : 4;
		u32 QoS_dynamic_n_valu : 4;
		u32 QoS_dynamic_l_valu : 4;
		u32 QoS_fixed_valu     : 4;
		u32 QoS_write_valu     : 4;
		u32 resv28             : 4;
	} b;
	u32 raw;
};

union cdma_int_mask {
	struct {
		u32 rf_des_int_mask : 1;
		u32 resv1           : 2;
		u32 rf_cmd_ept_mask : 1;
		u32 resv4           : 28;
	} b;
	u32 raw;
};

union cdma_int_status {
	struct {
		u32 rf_des_int     : 1;
		u32 resv1          : 2;
		u32 rf_cmd_ept_int : 1;
		u32 resv4          : 28;
	} b;
	u32 raw;
};

union cdma_max_payload {
	struct {
		u32 max_read_width     : 3;
		u32 max_write_width    : 3;
		u32 resv6              : 26;
	} b;
	u32 raw;
};

union cdma_des_cfg {
	struct {
		u32 start           : 1;
		u32 resv1           : 1;
		u32 EOD             : 1;
		u32 EOD_int_enable  : 1;
		u32 mode            : 1; //0: 1-D   1: 2-d
		u32 fixed_enable    : 2; //0: no fixed  1:fixed data
		u32 data_fmt        : 1; //0: 8bit   1: 16bit
		u32 resv8           : 8;
		u32 fixed_value     : 16;
	} b;
	u32 raw;
};


enum CDMA_TASK_STATUS {
	CDMA_STATUS_STOP,
	CDMA_STATUS_RUN,
};

enum CDMA_JOB_TYPE {
	CDMA_JOB_1D,
	CDMA_JOB_2D,
};

struct cdma_job {
	enum CDMA_JOB_TYPE enJobType;
	union {
		struct cdma_1d_param Param1d;
		struct cdma_2d_param Param2d;
	};
	wait_queue_head_t cond_queue;
	bool avail;
	struct list_head list;
};

struct cdma_task_ctx {
	struct list_head job_queue;
	spinlock_t job_lock;
	atomic_t num;
	atomic_t status;
	struct cdma_job *pstWorkingJob;
};

static uintptr_t reg_base_cdma;
static struct timespec64 start_time;
static struct cdma_task_ctx stCdmaTaskCtx;


static int _cdma_1d_copy(struct cdma_1d_param *param)
{
	union cdma_chl_ctrl ctrl;
	union cdma_int_mask int_mask;
	union cdma_des_cfg cfg;

	if (_reg_read(reg_base_cdma + CDMA_CMD_FIFO_STATUS) == FIFO_FULL_STATUS) {
		pr_err("command fifo full.\n");
		return -1;
	}

	_reg_write(reg_base_cdma + CDMA_CFG, 0);

	//src addr
	_reg_write(reg_base_cdma + CDMA_SRC_ADDR_L, param->src_addr);
	_reg_write(reg_base_cdma + CDMA_SRC_ADDR_H, param->src_addr >> 32);

	//dst addr
	_reg_write(reg_base_cdma + CDMA_DST_ADDR_L, param->dst_addr);
	_reg_write(reg_base_cdma + CDMA_DST_ADDR_H, param->dst_addr >> 32);

	//length
	_reg_write(reg_base_cdma + CDMA_DATA_LENGTH, param->len);

	/* clear rf_des_int_mask */
	int_mask.raw = 0;
	int_mask.b.rf_des_int_mask = 0;
	int_mask.b.rf_cmd_ept_mask = 1;
	_reg_write(reg_base_cdma + CDMA_INT_MAST, int_mask.raw);

	ctrl.b.cdma_enable = 1;
	ctrl.b.int_enable = 1;
	_reg_write_mask(reg_base_cdma + CDMA_CHL_CTRL, BIT(0) | BIT(3), ctrl.raw);

	ktime_get_ts64(&start_time);
	//cfg
	cfg.raw = 0;
	cfg.b.start = 1;
	cfg.b.EOD = 1;
	cfg.b.EOD_int_enable = 1;
	cfg.b.mode = 0;  //1-D
	cfg.b.data_fmt = param->data_type;
	_reg_write(reg_base_cdma + CDMA_CFG, cfg.raw);

	return 0;
}

static int _cdma_2d_copy(struct cdma_2d_param *param)
{
	union cdma_chl_ctrl ctrl;
	union cdma_int_mask int_mask;
	union cdma_des_cfg cfg;

	if (_reg_read(reg_base_cdma + CDMA_CMD_FIFO_STATUS) == FIFO_FULL_STATUS) {
		pr_err("command fifo full.\n");
		return -1;
	}

	//src addr
	_reg_write(reg_base_cdma + CDMA_SRC_ADDR_L, param->src_addr);
	_reg_write(reg_base_cdma + CDMA_SRC_ADDR_H, param->src_addr >> 32);

	//dst addr
	_reg_write(reg_base_cdma + CDMA_DST_ADDR_L, param->dst_addr);
	_reg_write(reg_base_cdma + CDMA_DST_ADDR_H, param->dst_addr >> 32);

	//w,h
	_reg_write(reg_base_cdma + CDMA_SIZE, param->height | (param->width << 16));

	//stride
	_reg_write(reg_base_cdma + CDMA_STRIDE, param->dst_stride | (param->src_stride << 16));

	/* clear rf_des_int_mask */
	int_mask.raw = 0;
	int_mask.b.rf_des_int_mask = 0;
	int_mask.b.rf_cmd_ept_mask = 1;
	_reg_write(reg_base_cdma + CDMA_INT_MAST, int_mask.raw);

	ctrl.b.cdma_enable = 1;
	ctrl.b.int_enable = 1;
	_reg_write_mask(reg_base_cdma + CDMA_CHL_CTRL, BIT(0) | BIT(3), ctrl.raw);

	ktime_get_ts64(&start_time);
	//cfg
	cfg.raw = 0;
	cfg.b.start = 1;
	cfg.b.EOD = 1;
	cfg.b.EOD_int_enable = 1;
	cfg.b.mode = 1;  //2-D
	cfg.b.fixed_enable = param->isFixed ? 1 : 0;
	cfg.b.data_fmt = param->data_type;
	cfg.b.fixed_value = param->isFixed ? param->fixed_value : 0;
	_reg_write(reg_base_cdma + CDMA_CFG, cfg.raw);

	return 0;
}



int _cdma_push_job(struct cdma_job *pstJob)
{
	unsigned long flags;

	spin_lock_irqsave(&stCdmaTaskCtx.job_lock, flags);
	list_add_tail(&pstJob->list, &stCdmaTaskCtx.job_queue);
	atomic_inc(&stCdmaTaskCtx.num);
	spin_unlock_irqrestore(&stCdmaTaskCtx.job_lock, flags);

	return 0;
}

void _cdma_try_schedule(void)
{
	struct cdma_job *pstJob = NULL;
	unsigned long flags;
	int ret = 0;

	if (atomic_read(&stCdmaTaskCtx.status) == CDMA_STATUS_RUN)
		return;

	if (atomic_read(&stCdmaTaskCtx.num) > 0) {
		spin_lock_irqsave(&stCdmaTaskCtx.job_lock, flags);
		if (!list_empty(&stCdmaTaskCtx.job_queue)) {
			pstJob = list_first_entry(&stCdmaTaskCtx.job_queue,
				struct cdma_job, list);
		}
		if (!pstJob) {
			pr_err("cdma job NULL\n");
			spin_unlock_irqrestore(&stCdmaTaskCtx.job_lock, flags);
			return;
		}

		if (pstJob->enJobType == CDMA_JOB_1D)
			ret = _cdma_1d_copy(&pstJob->Param1d);
		else
			ret = _cdma_2d_copy(&pstJob->Param2d);

		if (!ret) {
			stCdmaTaskCtx.pstWorkingJob = pstJob;
			list_del_init(&pstJob->list);
			atomic_dec(&stCdmaTaskCtx.num);
			atomic_set(&stCdmaTaskCtx.status, CDMA_STATUS_RUN);
		}
		spin_unlock_irqrestore(&stCdmaTaskCtx.job_lock, flags);
	}
}

static void cdma_job_finish(void)
{
	struct cdma_job *pstJob = stCdmaTaskCtx.pstWorkingJob;
	unsigned long flags;

	spin_lock_irqsave(&stCdmaTaskCtx.job_lock, flags);
	stCdmaTaskCtx.pstWorkingJob = NULL;
	atomic_set(&stCdmaTaskCtx.status, CDMA_STATUS_STOP);
	if (pstJob) {
		pstJob->avail = CVI_TRUE;
		wake_up_interruptible(&pstJob->cond_queue);
	}
	spin_unlock_irqrestore(&stCdmaTaskCtx.job_lock, flags);

	_cdma_try_schedule();
}

int _cdma_copy(enum CDMA_JOB_TYPE enJobType, void *param)
{
	struct cdma_job *pJob;
	int ret = 0;

	pJob = vzalloc(sizeof(*pJob));
	if (pJob == NULL) {
		pr_err("[%s] vzalloc fail.\n", __func__);
		return -1;
	}

	pJob->enJobType = enJobType;
	pJob->avail = false;
	memcpy(&pJob->Param1d, param, sizeof(*param));
	init_waitqueue_head(&pJob->cond_queue);

	if (enJobType == CDMA_JOB_1D)
		memcpy(&pJob->Param1d, param, sizeof(struct cdma_1d_param));
	else
		memcpy(&pJob->Param2d, param, sizeof(struct cdma_2d_param));

	_cdma_push_job(pJob);
	_cdma_try_schedule();

	ret = wait_event_interruptible(pJob->cond_queue, pJob->avail);
	// ret < 0, interrupt by a signal
	// ret = 0, condition true

	if (pJob->avail)
		ret = 0;
	else
		ret = -1;

	vfree(pJob);
	return ret;
}

void cdma_set_base_addr(void *cdma_base)
{
	reg_base_cdma = (uintptr_t)cdma_base;

	stCdmaTaskCtx.pstWorkingJob = NULL;
	spin_lock_init(&stCdmaTaskCtx.job_lock);
	INIT_LIST_HEAD(&stCdmaTaskCtx.job_queue);
	atomic_set(&stCdmaTaskCtx.num, 0);
	atomic_set(&stCdmaTaskCtx.status, CDMA_STATUS_STOP);
}

void cdma_irq_handler(void)
{
	union cdma_chl_ctrl ctrl;
	union cdma_int_status int_status;
	struct timespec64 time;
	u32 duration;

	int_status.raw = _reg_read(reg_base_cdma + CDMA_INT_STATUS);

	//clear int
	_reg_write(reg_base_cdma + CDMA_INT_STATUS, int_status.raw);

	if (int_status.b.rf_des_int == 1) {
		ktime_get_ts64(&time);
		duration = get_diff_in_us(start_time, time);
		pr_info("cdma interrupt status:%x\n", int_status.raw);
		pr_info("%s: time-consumption(%d)us\n", __func__, duration);
		//disable cdma
		ctrl.b.cdma_enable = 0;
		ctrl.b.int_enable = 0;
		_reg_write_mask(reg_base_cdma + CDMA_CHL_CTRL, BIT(0) | BIT(3), ctrl.raw);
		cdma_job_finish();
	}
}

int cvi_cdma_copy1d(struct cdma_1d_param *param)
{
	return _cdma_copy(CDMA_JOB_1D, param);
}
EXPORT_SYMBOL_GPL(cvi_cdma_copy1d);


int cvi_cdma_copy2d(struct cdma_2d_param *param)
{
	return _cdma_copy(CDMA_JOB_2D, param);
}
EXPORT_SYMBOL_GPL(cvi_cdma_copy2d);

