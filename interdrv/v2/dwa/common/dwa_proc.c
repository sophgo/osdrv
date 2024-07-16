/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File name: vip_dwa_proc.c
 * Description: video pipeline graphic distortion correction engine info
 */

#include "dwa_proc.h"
#include "dwa_debug.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define DWA_PROC_NAME "soph/dwa"

static struct dwa_proc_ctx *g_proc_ctx_dwa = NULL;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

static struct dwa_proc_ctx * dwa_get_proc_ctx(void)
{
	return g_proc_ctx_dwa;
}

static int dwa_proc_show_tsk(struct seq_file *m, struct dwa_proc_ctx *pdwaCtx, int idx)
{
	int  j;
	char c[32], c2[32];

	seq_printf(m, "\t\t%15s%15s%15s%15s%15s%15s%15s\n",
		"tskNo", "UseCoreID", "inSize","outSize", "State", "type", "HwTime(us)");

	for (j = 0; j < pdwaCtx->job_info[idx].task_num; j++) {
		if (pdwaCtx->job_info[idx].tsk_info[j].state == DWA_TASK_STATE_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pdwaCtx->job_info[idx].tsk_info[j].state == DWA_TASK_STATE_RUNNING)
			strncpy(c, "RUNNING", sizeof(c));
		else if (pdwaCtx->job_info[idx].tsk_info[j].state == DWA_TASK_STATE_DONE)
			strncpy(c, "DONE", sizeof(c));
		else if (pdwaCtx->job_info[idx].tsk_info[j].state == DWA_TASK_STATE_CANCLE)
			strncpy(c, "CANCEL", sizeof(c));
		else if (pdwaCtx->job_info[idx].tsk_info[j].state == DWA_TASK_STATE_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		if (pdwaCtx->job_info[idx].tsk_info[j].type == DWA_TASK_TYPE_ROT)
			strncpy(c2, "ROT", sizeof(c2));
		else if (pdwaCtx->job_info[idx].tsk_info[j].type == DWA_TASK_TYPE_FISHEYE)
			strncpy(c2, "FISHEYE", sizeof(c2));
		else if (pdwaCtx->job_info[idx].tsk_info[j].type == DWA_TASK_TYPE_AFFINE)
			strncpy(c2, "AFFINE", sizeof(c2));
		else if (pdwaCtx->job_info[idx].tsk_info[j].type == DWA_TASK_TYPE_LDC)
			strncpy(c2, "LDC", sizeof(c2));
		else if (pdwaCtx->job_info[idx].tsk_info[j].type == DWA_TASK_TYPE_WARP)
			strncpy(c2, "WARP", sizeof(c2));
		else
			strncpy(c2, "UNKNOWN", sizeof(c2));

		seq_printf(m, "\t\t%14s%d%15d%15d%15d%15s%15s%15d\n",
			"#",
			j,
			pdwaCtx->job_info[idx].tsk_info[j].top_id,
			pdwaCtx->job_info[idx].tsk_info[j].in_size,
			pdwaCtx->job_info[idx].tsk_info[j].out_size,
			c,
			c2,
			pdwaCtx->job_info[idx].tsk_info[j].hw_time);

		pdwaCtx->job_info[idx].hw_time += pdwaCtx->job_info[idx].tsk_info[j].hw_time;
	}
	seq_puts(m, "\n----------------------------------------------------------------------------------------------------------------------------------------\n");

	return 0;
}

/*************************************************************************
 *	DWA proc functions
 *************************************************************************/
static int dwa_proc_show(struct seq_file *m, void *v)
{
	struct dwa_proc_ctx *pdwaCtx = dwa_get_proc_ctx();
	unsigned long flags;
	int i, j, idx, total_handletime, total_hwTime, total_busyTime;
	char c[32];
	int idxs[DWA_PROC_JOB_INFO_NUM] = {0};

	if (unlikely(!pdwaCtx)) {
		seq_puts(m, "dwa proc ctx is NULL\n");
		return 0;
	}

	spin_lock_irqsave(&pdwaCtx->lock, flags);
	idx = pdwaCtx->job_idx - 1;
	spin_unlock_irqrestore(&pdwaCtx->lock, flags);

	seq_printf(m, "\nModule: [DWA], Build Time[%s]\n", UTS_VERSION);

	if (idx < 0 || idx >= DWA_PROC_JOB_INFO_NUM) {
		TRACE_DWA(DBG_ERR, "invalid proc job idx[%d], out of range[%d]\n", idx, DWA_PROC_JOB_INFO_NUM);
		return 0;
	}

	// recent job info
	seq_puts(m, "\n------------------------------------------------------------RECENT JOB INFO--------------------------------------------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State", "CostTime(us)");

	for (i = 0 ; i < DWA_PROC_JOB_INFO_NUM; ++i) {
		if (!pdwaCtx->job_info[idx].handle)
			break;

		memset(c, 0, sizeof(c));
		idxs[i] = idx;
		if (pdwaCtx->job_info[idx].state == DWA_JOB_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pdwaCtx->job_info[idx].state == DWA_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pdwaCtx->job_info[idx].state == DWA_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else if (pdwaCtx->job_info[idx].state == DWA_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			i,
			(pdwaCtx->job_info[idx].identity.mod_id == ID_BUTT) ?
				"USER" : MOD_STRING[pdwaCtx->job_info[idx].identity.mod_id],
			pdwaCtx->job_info[idx].identity.name,
			pdwaCtx->job_info[idx].identity.sync_io,
			pdwaCtx->job_info[idx].identity.id,
			pdwaCtx->job_info[idx].task_num,
			c,
			pdwaCtx->job_info[idx].cost_time);

		dwa_proc_show_tsk(m, pdwaCtx, idx);

		idx = (--idx < 0) ? (DWA_PROC_JOB_INFO_NUM - 1) : idx;
	}

	// Max waste time job info
	seq_puts(m, "\n-----------------------------------------------------------MAX WASTE TIME JOB INFO---------------------------------------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State",
		"CostTime(us)");

	idx = i = pdwaCtx->job_idx;
	for (j = 1; j < DWA_PROC_JOB_INFO_NUM; ++j) {
		i = (--i < 0) ? (DWA_PROC_JOB_INFO_NUM - 1) : i;
		if (!pdwaCtx->job_info[i].handle)
			break;

		if (pdwaCtx->job_info[i].cost_time > pdwaCtx->job_info[idx].cost_time)
			idx = i;
	}

	for (i = 0 ; i < DWA_PROC_JOB_INFO_NUM; ++i) {
		if (idxs[i] == idx)
			break;
	}

	if (pdwaCtx->job_info[idx].handle) {
		memset(c, 0, sizeof(c));
		if (pdwaCtx->job_info[idx].state == DWA_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else if (pdwaCtx->job_info[idx].state == DWA_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pdwaCtx->job_info[idx].state == DWA_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			i,
			MOD_STRING[pdwaCtx->job_info[idx].identity.mod_id],
			pdwaCtx->job_info[idx].identity.name,
			pdwaCtx->job_info[idx].identity.sync_io,
			pdwaCtx->job_info[idx].identity.id,
			pdwaCtx->job_info[idx].task_num,
			c,
			pdwaCtx->job_info[idx].cost_time);

		dwa_proc_show_tsk(m, pdwaCtx, idx);
	}

	// DWA job status
	seq_puts(m, "\n-----------------------------------DWA JOB STATUS-----------------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n",
		"Success", "Fail", "Cancel", "BeginNum", "WaitNum", "ProcingNum");

	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pdwaCtx->job_status.success,
				pdwaCtx->job_status.fail,
				pdwaCtx->job_status.cancel,
				pdwaCtx->job_status.begin_num,
				pdwaCtx->job_status.wait_num,
				pdwaCtx->job_status.procing_num);

	// DWA task status
	seq_puts(m, "\n-----------------------------------DWA TASK STATUS----------------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n", "Success", "Fail", "Cancel", "Begin", "Procing", "WaitNum");
	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pdwaCtx->tsk_status.success,
				pdwaCtx->tsk_status.fail,
				pdwaCtx->tsk_status.cancel,
				pdwaCtx->tsk_status.begin_num,
				pdwaCtx->tsk_status.procing_num,
				pdwaCtx->tsk_status.wait_num);

	// DWA interrupt status
	seq_puts(m, "\n---------------------------------------------------DWA HW AVG STATUS-----------------------------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s%15s\n", "AVGNum", "TotalHwTm", "TotalBusyTm", "TotalCostTm", "AVGHwTm", "AVGBusyTm", "AVGCostTm(us)");

	total_handletime = total_hwTime = total_busyTime = 0;
	for (i = 0; i < DWA_PROC_JOB_INFO_NUM; ++i) {
		total_hwTime += pdwaCtx->job_info[i].hw_time;
		total_busyTime += pdwaCtx->job_info[i].busy_time;
		total_handletime = total_hwTime + total_busyTime;
	}
	seq_printf(m, "%15d%15d%15d%15d%15d%15d%15d\n",
				DWA_PROC_JOB_INFO_NUM,
				total_hwTime,
				total_busyTime,
				total_handletime,
				total_hwTime / DWA_PROC_JOB_INFO_NUM,
				total_busyTime / DWA_PROC_JOB_INFO_NUM,
				total_handletime / DWA_PROC_JOB_INFO_NUM);

	// DWA call correction status
	seq_puts(m, "\n-----------------------------------DWA CALL CORRECTION STATUS----------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s\n", "TaskSuc", "TaskFail", "EndSuc", "EndFail", "CbCnt");
	seq_printf(m, "%15d%15d%15d%15d%15d\n",
				pdwaCtx->fisheye_status.add_task_suc,
				pdwaCtx->fisheye_status.add_task_fail,
				pdwaCtx->fisheye_status.end_job_suc,
				pdwaCtx->fisheye_status.end_job_fail,
				pdwaCtx->fisheye_status.cb_cnt);

	return 0;
}

static int dwa_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwa_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops dwa_proc_fops = {
	.proc_open = dwa_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations dwa_proc_fops = {
	.owner = THIS_MODULE,
	.open = dwa_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int dwa_handle_to_procIdx(bool is_end, struct dwa_job *job)
{
	int idx = -1;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;

	if (proc_ctx && job) {
		spin_lock_irqsave(&proc_ctx->lock, flags);
		for (idx = 0; idx < DWA_PROC_JOB_INFO_NUM; ++idx) {
			if (proc_ctx->job_info[idx].handle == (u64)(uintptr_t)job) {
				if (is_end && proc_ctx->job_info[idx].state == DWA_JOB_WORKING)
					break;
				if (!is_end && proc_ctx->job_info[idx].state == DWA_JOB_WAIT)
					break;
			}
		}
		spin_unlock_irqrestore(&proc_ctx->lock, flags);

		if (idx >= DWA_PROC_JOB_INFO_NUM) {
			TRACE_DWA(DBG_NOTICE, "(%d)canot find match job(%px) from proc_ctx,idx out of range\n"
				, idx, job);
			idx = -1;
		}
	}

	return idx;
}

int dwa_proc_init(void *shm)
{
	int rc = 0, idx;

	if (proc_create_data(DWA_PROC_NAME, 0644, NULL, &dwa_proc_fops, NULL) == NULL) {
		pr_err("dwa proc creation failed\n");
		rc = -1;
	}

	g_proc_ctx_dwa = (struct dwa_proc_ctx *)shm;
	spin_lock_init(&g_proc_ctx_dwa->lock);

	for (idx = 0; idx < DWA_PROC_JOB_INFO_NUM; ++idx)
		g_proc_ctx_dwa->job_info[idx].handle = 0;

	return rc;
}

int dwa_proc_remove(void)
{
	remove_proc_entry(DWA_PROC_NAME, NULL);
	g_proc_ctx_dwa = NULL;

	return 0;
}

void dwa_proc_record_hw_tsk_start(struct dwa_job *job, struct dwa_task *tsk, unsigned char top_id)
{
	int idx;
	struct timespec64 curTime;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;

	idx = dwa_handle_to_procIdx(true, job);
	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < DWA_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = DWA_TASK_STATE_RUNNING;
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].top_id = top_id;

		proc_ctx->tsk_status.procing_num++;
		proc_ctx->tsk_status.wait_num =
			proc_ctx->tsk_status.begin_num - proc_ctx->tsk_status.procing_num;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		TRACE_DWA(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void dwa_proc_record_hw_tsk_done(struct dwa_job *job, struct dwa_task *tsk)
{
	int idx;
	unsigned long long end_time;
	struct timespec64 curTime;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;
	enum dwa_task_state state;

	idx = dwa_handle_to_procIdx(true, job);
	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < DWA_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);
		end_time = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		state = atomic_read(&tsk->state);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_time =
			(u32)(end_time - proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].hw_start_time);

		if (state == DWA_TASK_STATE_DONE)
			proc_ctx->tsk_status.success++;
		else if (state == DWA_TASK_STATE_CANCLE)
			proc_ctx->tsk_status.cancel++;
		else if (state == DWA_TASK_STATE_FAIL)
			proc_ctx->tsk_status.fail++;
		else
			TRACE_DWA(DBG_ERR, "invalid tsk id(%d) state (%d).\n", tsk->tsk_id, state);

		proc_ctx->job_info[idx].tsk_info[tsk->tsk_id].state = state;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		TRACE_DWA(DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void dwa_proc_record_job_start(struct dwa_job *job)
{
	int idx;
	struct timespec64 curTime;
	unsigned long long curTimeUs;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;

	idx = dwa_handle_to_procIdx(false, job);

	if (job && proc_ctx && idx >= 0 && idx < DWA_PROC_JOB_INFO_NUM) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->job_info[idx].busy_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		proc_ctx->job_status.procing_num++;
		proc_ctx->job_status.wait_num =
			proc_ctx->job_status.begin_num - proc_ctx->job_status.procing_num;
		proc_ctx->job_info[idx].state = DWA_JOB_WORKING;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		TRACE_DWA(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void dwa_proc_record_job_done(struct dwa_job *job)
{
	int idx;
	struct timespec64 curTime;
	unsigned long long curTimeUs;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;
	enum dwa_job_state state;

	idx = dwa_handle_to_procIdx(true, job);

	if (job && proc_ctx && idx >= 0) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		state = atomic_read(&job->job_state);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		if (state == DWA_JOB_END)
			proc_ctx->job_status.success++;
		else if (state == DWA_JOB_CANCLE)
			proc_ctx->job_status.cancel++;
		else if (state == DWA_JOB_FAIL)
			proc_ctx->job_status.fail++;
		else
			TRACE_DWA(DBG_ERR, "invalid job state (%d).\n", state);

		proc_ctx->job_info[idx].state = state;
		proc_ctx->job_info[idx].cost_time =
			(u32)(curTimeUs - proc_ctx->job_info[idx].submit_time);
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		TRACE_DWA(DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void dwa_proc_commit_job(struct dwa_job *job)
{
	unsigned short u16Idx, i = 0;
	struct dwa_task *curelm, *tmp;
	struct timespec64 curTime;
	struct dwa_proc_ctx *proc_ctx = dwa_get_proc_ctx();
	unsigned long flags;

	if (job && proc_ctx) {
		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->job_idx = (proc_ctx->job_idx >= DWA_PROC_JOB_INFO_NUM) ?
			0 : proc_ctx->job_idx;
		u16Idx = proc_ctx->job_idx;

		memset(&proc_ctx->job_info[u16Idx], 0, sizeof(proc_ctx->job_info[u16Idx]));
		proc_ctx->job_info[u16Idx].handle = (u64)(uintptr_t)job;
		memcpy(&proc_ctx->job_info[u16Idx].identity, &job->identity, sizeof(job->identity));

		list_for_each_entry_safe(curelm, tmp, &job->task_list, node) {
			proc_ctx->job_info[u16Idx].task_num++;
			proc_ctx->job_info[u16Idx].tsk_info[i].in_size =
				curelm->attr.img_in.video_frame.width *
				curelm->attr.img_in.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].out_size =
				curelm->attr.img_out.video_frame.width *
				curelm->attr.img_out.video_frame.height;
			proc_ctx->job_info[u16Idx].tsk_info[i].state = DWA_TASK_STATE_WAIT;
			proc_ctx->job_info[u16Idx].tsk_info[i].type = curelm->type;
			i++;
			proc_ctx->tsk_status.begin_num++;
		}

		proc_ctx->job_info[u16Idx].state = DWA_JOB_WAIT;
		proc_ctx->job_status.begin_num++;
		proc_ctx->job_idx++;

		ktime_get_ts64(&curTime);
		proc_ctx->job_info[u16Idx].submit_time =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	}else {
		TRACE_DWA(DBG_NOTICE, "job or proc_ctx invalid.\n");
	}
}

