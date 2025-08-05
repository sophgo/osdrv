#include "osal_types.h"

#include "ldc.h"
#include "ldc_debug.h"
#include "ldc_comm_layer.h"
#include "base_common.h"

#ifdef USEC_PER_SEC
#undef USEC_PER_SEC
#endif
#define USEC_PER_SEC        1000000

static int ldc_handle_to_procIdx(struct ldc_job *job)
{
	int idx = -1;
	unsigned long flags;

	if (!job) {
		TRACE_LDC(DBG_ERR, "null job\n");
		return idx;
	}

	osal_spin_lock_irqsave(&job->lock, &flags);
	idx = job->proc_idx;
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	return idx;
}

static void ldc_enable_dev_clk(int coreid, struct ldc_ctx *dev)
{
	if (!dev || !dev->core[coreid].ldc_clk) {
		TRACE_LDC(DBG_ERR, "null dev or null clk_ldc[%d]\n", coreid);
		return;
	}

	if (!osal_clk_is_enabled(dev->core[coreid].ldc_clk))
		osal_clk_enable(dev->core[coreid].ldc_clk);
}

void ldc_clk_init(struct ldc_ctx *dev)
{
	int i;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (osal_atomic_read(&dev->core[i].clk_en))
			return;
		if (dev->core[i].ldc_clk)
			osal_clk_prepare_enable(dev->core[i].ldc_clk);

		ldc_enable_dev_clk(i, dev);
		osal_atomic_set(&dev->core[i].clk_en, true);
	}
}

void ldc_clk_deinit(struct ldc_ctx *dev)
{
	int i;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (!osal_atomic_read(&dev->core[i].clk_en))
			return;
		if (dev->core[i].ldc_clk)
			osal_clk_disable_unprepare(dev->core[i].ldc_clk);

		osal_atomic_set(&dev->core[i].clk_en, false);
	}
}

//dev core init
void ldc_core_init(int top_id)
{
	ldc_reset(top_id);
	ldc_init(top_id);
	ldc_intr_ctrl(0x00, top_id);//0x7 if you want get mesh tbl id err status
}

void ldc_core_deinit(int top_id)
{
	ldc_intr_ctrl(0x00, top_id);
	ldc_disable(top_id);
	ldc_reset(top_id);
}

#if !defined(CONFIG_LDC_SUPPORT_PROC) || (CONFIG_LDC_SUPPORT_PROC == 1)
static void _gdc_timer_callback(unsigned long data)
{
	int i;
	unsigned int duration;
	osal_timeval cur_time = {0};
	static osal_timeval pre_time = {0};
	struct ldc_ctx *dev = (struct ldc_ctx *)osal_timer_get_private_data((void *)data);
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	osal_gettimeofday(&cur_time);
	duration = get_diff_in_us(pre_time, cur_time);
	pre_time = cur_time;

	if (duration > 2000000)
		goto FIRSTTIME;

	for (i = 0; i < LDC_DEV_MAX_CNT; ++i) {
		proc_ctx->gdc_core_status[i].duty_ratio = (proc_ctx->gdc_core_status[i].core_all_hw_time * 100) / duration;
		proc_ctx->gdc_core_status[i].core_all_hw_time = 0;
	}

FIRSTTIME:
	osal_timer_mod(&dev->timer, 1000);
}
#endif

void ldc_dev_init(struct ldc_ctx *dev)
{
	int i;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		osal_atomic_set(&dev->core[i].state, LDC_CORE_STATE_IDLE);
		dev->core[i].dev_type = (enum ldc_type)i;
		ldc_core_init(i);
	}
	osal_atomic_set(&dev->state, LDC_DEV_STATE_STOP);

#if !defined(CONFIG_LDC_SUPPORT_PROC) || (CONFIG_LDC_SUPPORT_PROC == 1)
	/* Initialize the timer to update the proc data */
	dev->timer.handler = _gdc_timer_callback;
	dev->timer.data = (unsigned long)dev;
	dev->timer.interval = 1000;
	osal_timer_init(&dev->timer);
	osal_timer_start(&dev->timer);
#endif
}

void ldc_dev_deinit(struct ldc_ctx *dev)
{
	int i;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		osal_atomic_set(&dev->core[i].state, LDC_CORE_STATE_IDLE);
		ldc_core_deinit(i);
	}

	osal_atomic_set(&dev->state, LDC_DEV_STATE_STOP);

#if !defined(CONFIG_LDC_SUPPORT_PROC) || (CONFIG_LDC_SUPPORT_PROC == 1)
	/* Off timer */
	osal_timer_stop(&dev->timer);
	osal_timer_destroy_sync(&dev->timer);
#endif
}

//proc update
static struct ldc_proc_ctx *g_proc_ctx_ldc;

struct ldc_proc_ctx *ldc_get_proc_ctx(void)
{
	return g_proc_ctx_ldc;
}

struct ldc_proc_ctx **ldc_get_proc_ctx_addr(void)
{
	return &g_proc_ctx_ldc;
}

void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, unsigned char top_id)
{
	int idx;
	osal_timeval curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	idx = ldc_handle_to_procIdx(job);

	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		osal_gettimeofday(&curTime);

		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_usec);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = LDC_TASK_STATE_RUNNING;
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].top_id = top_id;
		proc_ctx->tsk_status.procing_num++;
		proc_ctx->tsk_status.wait_num =
			proc_ctx->tsk_status.begin_num - proc_ctx->tsk_status.procing_num;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk)
{
	int idx;
	unsigned long long end_time;
	osal_timeval curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	enum ldc_task_state state;

	idx = ldc_handle_to_procIdx(job);

	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		osal_gettimeofday(&curTime);
		end_time = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_usec);

		state = osal_atomic_read(&tsk->state);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = state;
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_time =
			(u32)(end_time - proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time);
		proc_ctx->gdc_core_status[DEV_LDC_0].core_all_hw_time +=
			proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_time;

		if (state == LDC_TASK_STATE_DONE)
			proc_ctx->tsk_status.success++;
		else if (state == LDC_TASK_STATE_CANCLE)
			proc_ctx->tsk_status.cancel++;
		else if (state == LDC_TASK_STATE_FAIL)
			proc_ctx->tsk_status.fail++;
		else
			TRACE_LDC(DBG_ERR, "invalid tsk id(%d) state (%d).\n", tsk->tsk_id, state);

	} else {
		TRACE_LDC(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}

}

void ldc_proc_record_job_start(struct ldc_job *job)
{
	int idx;
	osal_timeval curTime;
	unsigned long long curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();

	idx = ldc_handle_to_procIdx(job);

	if (job && proc_ctx && idx >= 0 && idx < LDC_PROC_JOB_INFO_NUM) {
		osal_gettimeofday(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_usec);

		proc_ctx->job_info[idx].busy_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		proc_ctx->job_status.procing_num++;
		proc_ctx->job_status.wait_num =
			proc_ctx->job_status.begin_num - proc_ctx->job_status.procing_num;
		proc_ctx->job_info[idx].state = LDC_JOB_WORKING;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_job_done(struct ldc_job *job)
{
	int idx;
	osal_timeval curTime;
	unsigned long long curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	enum ldc_job_state state;

	idx = ldc_handle_to_procIdx(job);

	if (job && proc_ctx && idx >= 0) {
		osal_gettimeofday(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_usec);
		state = osal_atomic_read(&job->job_state);

		if (state == LDC_JOB_END)
			proc_ctx->job_status.success++;
		else if (state == LDC_JOB_CANCLE)
			proc_ctx->job_status.cancel++;
		else if (state == LDC_JOB_FAIL)
			proc_ctx->job_status.fail++;
		else
			TRACE_LDC(DBG_ERR, "invalid job state (%d).\n", state);

		proc_ctx->job_info[idx].state = state;
		proc_ctx->job_info[idx].cost_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		proc_ctx->job_info[idx].core_id = job->coreid;
	} else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_commit_job(struct ldc_job *job)
{
	unsigned short u16Idx, i;
	struct ldc_task *curelm;
	osal_timeval curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;
	int tsk_num;

	if (!job)
		TRACE_LDC(DBG_INFO, "job null\n");
	if (!proc_ctx)
		TRACE_LDC(DBG_INFO, "proc ctx null\n");

	if (job && proc_ctx) {
		tsk_num = osal_atomic_read(&job->task_num);
		osal_spin_lock_irqsave(&proc_ctx->lock, &flags);
		proc_ctx->job_idx = (proc_ctx->job_idx >= LDC_PROC_JOB_INFO_NUM) ?
			0 : proc_ctx->job_idx;
		u16Idx = proc_ctx->job_idx;
		proc_ctx->job_idx++;
		osal_spin_unlock_irqrestore(&proc_ctx->lock, &flags);

		osal_spin_lock_irqsave(&job->lock, &flags);
		job->proc_idx = u16Idx;
		osal_spin_unlock_irqrestore(&job->lock, &flags);

		TRACE_LDC(DBG_INFO, "proc ctx idx (%d)\n", u16Idx);

		osal_memset(&proc_ctx->job_info[u16Idx], 0, sizeof(proc_ctx->job_info[u16Idx]));
		proc_ctx->job_info[u16Idx].handle = (u64)(uintptr_t)job;
		osal_memcpy(&proc_ctx->job_info[u16Idx].identity, &job->identity, sizeof(job->identity));

		for (i = 0; i < tsk_num; i++) {
			osal_spin_lock_irqsave(&job->lock, &flags);
			curelm = osal_list_first_entry(&job->task_list, struct ldc_task, node);
			osal_spin_unlock_irqrestore(&job->lock, &flags);

			if (!curelm) {
				TRACE_LDC(DBG_ERR, "task index %d invalid\n", i);
				return;
			}
			proc_ctx->job_info[u16Idx].task_num++;
			proc_ctx->job_info[u16Idx].tsk_info[i].in_size =
				curelm->attr.img_in.video_frame.width *
				curelm->attr.img_in.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].out_size =
				curelm->attr.img_out.video_frame.width *
				curelm->attr.img_out.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].state = LDC_TASK_STATE_WAIT;
			proc_ctx->job_info[u16Idx].tsk_info[i].type = curelm->type;
			proc_ctx->tsk_status.begin_num++;
		}

		proc_ctx->job_info[u16Idx].state = LDC_JOB_WAIT;
		proc_ctx->job_status.begin_num++;

		osal_gettimeofday(&curTime);
		proc_ctx->job_info[u16Idx].submit_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_usec);
	} else {
		TRACE_LDC(DBG_NOTICE, "job or proc_ctx invalid.\n");
	}
}

