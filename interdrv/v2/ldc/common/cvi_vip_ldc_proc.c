/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_ldc_proc.c
 * Description: video pipeline graphic distortion correction engine info
 */

#include "cvi_vip_ldc_proc.h"
#include "ldc_debug.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define LDC_PROC_NAME "soph/ldc"

static struct ldc_proc_ctx *g_proc_ctx_ldc = NULL;
static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);

static struct ldc_proc_ctx * ldc_get_proc_ctx(void)
{
	return g_proc_ctx_ldc;
}

static int ldc_proc_show_tsk(struct seq_file *m, struct ldc_proc_ctx *pldcCtx, int idx)
{
	int  j;
	char c[32], c2[32];

	seq_printf(m, "\t\t%15s%15s%15s%15s%15s%15s%15s\n",
		"tskNo", "UseCoreID", "inSize","outSize", "State", "type", "HwTime(us)");

	for (j = 0; j < pldcCtx->stJobInfo[idx].u32TaskNum; j++) {
		if (pldcCtx->stJobInfo[idx].tsk_info[j].state == LDC_TASK_STATE_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].state == LDC_TASK_STATE_RUNNING)
			strncpy(c, "RUNNING", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].state == LDC_TASK_STATE_DONE)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].state == LDC_TASK_STATE_CANCLE)
			strncpy(c, "CANCEL", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].state == LDC_TASK_STATE_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		if (pldcCtx->stJobInfo[idx].tsk_info[j].type == LDC_TASK_TYPE_ROT)
			strncpy(c2, "ROT", sizeof(c2));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].type == LDC_TASK_TYPE_FISHEYE)
			strncpy(c2, "FISHEYE", sizeof(c2));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].type == LDC_TASK_TYPE_AFFINE)
			strncpy(c2, "AFFINE", sizeof(c2));
		else if (pldcCtx->stJobInfo[idx].tsk_info[j].type == LDC_TASK_TYPE_LDC)
			strncpy(c2, "LDC", sizeof(c2));
		else
			strncpy(c2, "UNKNOWN", sizeof(c2));

		seq_printf(m, "\t\t%14s%d%15d%15d%15d%15s%15s%15d\n",
			"#",
			j,
			pldcCtx->stJobInfo[idx].tsk_info[j].top_id,
			pldcCtx->stJobInfo[idx].tsk_info[j].u32InSize,
			pldcCtx->stJobInfo[idx].tsk_info[j].u32OutSize,
			c,
			c2,
			pldcCtx->stJobInfo[idx].tsk_info[j].u32HwTime);
		pldcCtx->stJobInfo[idx].u32HwTime += pldcCtx->stJobInfo[idx].tsk_info[j].u32HwTime;
	}
	seq_puts(m, "\n----------------------------------------------------------------------------------------------------------------------------------------\n");

	return 0;
}

/*************************************************************************
 *	LDC proc functions
 *************************************************************************/
static int ldc_proc_show(struct seq_file *m, void *v)
{
	struct ldc_proc_ctx *pldcCtx = ldc_get_proc_ctx();
	unsigned long flags;
	int i, j, idx, total_handletime, total_hwTime, total_busyTime;
	char c[32];

	if (unlikely(!pldcCtx)) {
		seq_puts(m, "ldc proc ctx is NULL\n");
		return 0;
	}

	spin_lock_irqsave(&pldcCtx->lock, flags);
	idx = pldcCtx->u16JobIdx - 1;
	spin_unlock_irqrestore(&pldcCtx->lock, flags);

	seq_printf(m, "\nModule: [LDC], Build Time[%s]\n", UTS_VERSION);

	if (idx < 0 || idx >= LDC_PROC_JOB_INFO_NUM) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid proc job idx[%d], out of range[%d]\n", idx, LDC_PROC_JOB_INFO_NUM);
		return 0;
	}

	// recent job info
	seq_puts(m, "\n------------------------------------------------------------RECENT JOB INFO--------------------------------------------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "ModName", "jobName", "syncIo", "u32ID", "TaskNum", "State", "CostTime(us)");

	for (i = 0 ; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		if (!pldcCtx->stJobInfo[idx].hHandle)
			break;

		memset(c, 0, sizeof(c));
		if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			i,
			(pldcCtx->stJobInfo[idx].identity.enModId == CVI_ID_BUTT) ?
				"USER" : MOD_STRING[pldcCtx->stJobInfo[idx].identity.enModId],
			pldcCtx->stJobInfo[idx].identity.Name,
			pldcCtx->stJobInfo[idx].identity.syncIo,
			pldcCtx->stJobInfo[idx].identity.u32ID,
			pldcCtx->stJobInfo[idx].u32TaskNum,
			c,
			pldcCtx->stJobInfo[idx].u32CostTime);

		ldc_proc_show_tsk(m, pldcCtx, idx);

		idx = (--idx < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : idx;
	}

	// Max waste time job info
	seq_puts(m, "\n-------------------------------MAX WASTE TIME JOB INFO--------------------\n");
	seq_printf(m, "%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "ModName", "jobName", "syncIo", "u32ID", "TaskNum", "State",
		"CostTime(us)");

	idx = i = pldcCtx->u16JobIdx;
	for (j = 1; j < LDC_PROC_JOB_INFO_NUM; ++j) {
		i = (--i < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : i;
		if (!pldcCtx->stJobInfo[i].hHandle)
			break;

		if (pldcCtx->stJobInfo[i].u32CostTime > pldcCtx->stJobInfo[idx].u32CostTime)
			idx = i;
	}

	if (pldcCtx->stJobInfo[idx].hHandle) {
		memset(c, 0, sizeof(c));
		if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->stJobInfo[idx].eState == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		seq_printf(m, "%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			idx,
			MOD_STRING[pldcCtx->stJobInfo[idx].identity.enModId],
			pldcCtx->stJobInfo[idx].identity.Name,
			pldcCtx->stJobInfo[idx].identity.syncIo,
			pldcCtx->stJobInfo[idx].identity.u32ID,
			pldcCtx->stJobInfo[idx].u32TaskNum,
			c,
			pldcCtx->stJobInfo[idx].u32CostTime);

		ldc_proc_show_tsk(m, pldcCtx, idx);
	}

	// LDC job status
	seq_puts(m, "\n-------------------------------LDC JOB STATUS-----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n",
		"Success", "Fail", "Cancel", "BeginNum", "WaitNum", "ProcingNum");

	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->stJobStatus.u32Success,
				pldcCtx->stJobStatus.u32Fail,
				pldcCtx->stJobStatus.u32Cancel,
				pldcCtx->stJobStatus.u32BeginNum,
				pldcCtx->stJobStatus.u32waitNum,
				pldcCtx->stJobStatus.u32ProcingNum);

	// LDC task status
	seq_puts(m, "\n-------------------------------LDC TASK STATUS----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s\n", "Success", "Fail", "Cancel", "Begin", "Procing", "WaitNum");
	seq_printf(m, "%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->stTskstatus.u32Success,
				pldcCtx->stTskstatus.u32Fail,
				pldcCtx->stTskstatus.u32Cancel,
				pldcCtx->stTskstatus.u32BeginNum,
				pldcCtx->stTskstatus.u32ProcingNum,
				pldcCtx->stTskstatus.u32waitNum);

	// LDC interrupt status
	seq_puts(m, "\n-------------------------------LDC HW AVG STATUS-----------------------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s%15s%15s\n", "AVGNum", "TotalHwTm", "TotalBusyTm", "TotalCostTm", "AVGHwTm", "AVGBusyTm", "AVGCostTm(us)");

	total_handletime = total_hwTime = total_busyTime = 0;
	for (i = 0; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		total_hwTime += pldcCtx->stJobInfo[i].u32HwTime;
		total_busyTime += pldcCtx->stJobInfo[i].u32BusyTime;
		total_handletime = total_hwTime + total_busyTime;
	}
	seq_printf(m, "%15d%15d%15d%15d%15d%15d%15d\n",
				LDC_PROC_JOB_INFO_NUM,
				total_hwTime,
				total_busyTime,
				total_handletime,
				total_hwTime / LDC_PROC_JOB_INFO_NUM,
				total_busyTime / LDC_PROC_JOB_INFO_NUM,
				total_handletime / LDC_PROC_JOB_INFO_NUM);

	// LDC call correction status
	seq_puts(m, "\n-------------------------------LDC CALL CORRECTION STATUS-----------------\n");
	seq_printf(m, "%15s%15s%15s%15s%15s\n", "TaskSuc", "TaskFail", "EndSuc", "EndFail", "CbCnt");
	seq_printf(m, "%15d%15d%15d%15d%15d\n",
				pldcCtx->stFishEyeStatus.u32AddTaskSuc,
				pldcCtx->stFishEyeStatus.u32AddTaskFail,
				pldcCtx->stFishEyeStatus.u32EndJobSuc,
				pldcCtx->stFishEyeStatus.u32EndJobFail,
				pldcCtx->stFishEyeStatus.u32CbCnt);

	return 0;
}

static int ldc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ldc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops ldc_proc_fops = {
	.proc_open = ldc_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations ldc_proc_fops = {
	.owner = THIS_MODULE,
	.open = ldc_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

static int ldc_handle_to_procIdx(bool is_end, struct ldc_job *job)
{
	int idx = -1;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;

	if (proc_ctx && job) {
		spin_lock_irqsave(&proc_ctx->lock, flags);
		for (idx = 0; idx < LDC_PROC_JOB_INFO_NUM; ++idx) {
			if (proc_ctx->stJobInfo[idx].hHandle == (u64)(uintptr_t)job) {
				if (is_end && proc_ctx->stJobInfo[idx].eState == LDC_JOB_WORKING)
					break;
				if (!is_end && proc_ctx->stJobInfo[idx].eState == LDC_JOB_WAIT)
					break;
			}
		}
		spin_unlock_irqrestore(&proc_ctx->lock, flags);

		if (idx >= LDC_PROC_JOB_INFO_NUM) {
			CVI_TRACE_LDC(CVI_DBG_NOTICE, "(%d)canot find match job(%px) from proc_ctx,idx out of range\n"
				, idx, job);
			idx = -1;
		}
	}

	return idx;
}

int ldc_proc_init(void *shm)
{
	int rc = 0, idx;

	if (proc_create_data(LDC_PROC_NAME, 0644, NULL, &ldc_proc_fops, NULL) == NULL) {
		pr_err("ldc proc creation failed\n");
		rc = -1;
	}

	g_proc_ctx_ldc = (struct ldc_proc_ctx *)shm;
	spin_lock_init(&g_proc_ctx_ldc->lock);

	for (idx = 0; idx < LDC_PROC_JOB_INFO_NUM; ++idx)
		g_proc_ctx_ldc->stJobInfo[idx].hHandle = 0;

	return rc;
}

int ldc_proc_remove(void)
{
	remove_proc_entry(LDC_PROC_NAME, NULL);
	g_proc_ctx_ldc = NULL;

	return 0;
}

void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, u8 top_id)
{
	int idx;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;

	idx = ldc_handle_to_procIdx(true, job);
	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].u64HwStartTime =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].state = LDC_TASK_STATE_RUNNING;
		proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].top_id = top_id;

		proc_ctx->stTskstatus.u32ProcingNum++;
		proc_ctx->stTskstatus.u32waitNum =
			proc_ctx->stTskstatus.u32BeginNum - proc_ctx->stTskstatus.u32ProcingNum;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk)
{
	int idx;
	u64 end_time;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;
	enum ldc_task_state state;

	idx = ldc_handle_to_procIdx(true, job);
	if (job && tsk && proc_ctx && idx >= 0
		&& (tsk->tsk_id >= 0 && tsk->tsk_id < LDC_JOB_MAX_TSK_NUM)) {
		ktime_get_ts64(&curTime);
		end_time = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		state = atomic_read(&tsk->state);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].u32HwTime =
			(u32)(end_time - proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].u64HwStartTime);

		if (state == LDC_TASK_STATE_DONE)
			proc_ctx->stTskstatus.u32Success++;
		else if (state == LDC_TASK_STATE_CANCLE)
			proc_ctx->stTskstatus.u32Cancel++;
		else if (state == LDC_TASK_STATE_FAIL)
			proc_ctx->stTskstatus.u32Fail++;
		else
			CVI_TRACE_LDC(CVI_DBG_ERR, "invalid tsk id(%d) state (%d).\n", tsk->tsk_id, state);

		proc_ctx->stJobInfo[idx].tsk_info[tsk->tsk_id].state = state;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "job or tsk or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_job_start(struct ldc_job *job)
{
	int idx;
	struct timespec64 curTime;
	u64 curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;

	idx = ldc_handle_to_procIdx(false, job);

	if (job && proc_ctx && idx >= 0 && idx < LDC_PROC_JOB_INFO_NUM) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->stJobInfo[idx].u32BusyTime =
			(u32)(curTimeUs - proc_ctx->stJobInfo[idx].u64SubmitTime);
		proc_ctx->stJobStatus.u32ProcingNum++;
		proc_ctx->stJobStatus.u32waitNum =
			proc_ctx->stJobStatus.u32BeginNum - proc_ctx->stJobStatus.u32ProcingNum;
		proc_ctx->stJobInfo[idx].eState = LDC_JOB_WORKING;
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_record_job_done(struct ldc_job *job)
{
	int idx;
	struct timespec64 curTime;
	u64 curTimeUs;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;
	enum ldc_job_state state;

	idx = ldc_handle_to_procIdx(true, job);

	if (job && proc_ctx && idx >= 0) {
		ktime_get_ts64(&curTime);
		curTimeUs = (u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		state = atomic_read(&job->enJobState);

		spin_lock_irqsave(&proc_ctx->lock, flags);
		if (state == LDC_JOB_END)
			proc_ctx->stJobStatus.u32Success++;
		else if (state == LDC_JOB_CANCLE)
			proc_ctx->stJobStatus.u32Cancel++;
		else if (state == LDC_JOB_FAIL)
			proc_ctx->stJobStatus.u32Fail++;
		else
			CVI_TRACE_LDC(CVI_DBG_ERR, "invalid job state (%d).\n", state);

		proc_ctx->stJobInfo[idx].eState = state;
		proc_ctx->stJobInfo[idx].u32CostTime =
			(u32)(curTimeUs - proc_ctx->stJobInfo[idx].u64SubmitTime);
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	} else {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "job or proc_ctx or job_idx(%d) invalid.\n", idx);
	}
}

void ldc_proc_commit_job(struct ldc_job *job)
{
	u16 u16Idx, i = 0;
	struct ldc_task *curelm, *tmp;
	struct timespec64 curTime;
	struct ldc_proc_ctx *proc_ctx = ldc_get_proc_ctx();
	unsigned long flags;

	if (job && proc_ctx) {
		spin_lock_irqsave(&proc_ctx->lock, flags);
		proc_ctx->u16JobIdx = (proc_ctx->u16JobIdx >= LDC_PROC_JOB_INFO_NUM) ?
			0 : proc_ctx->u16JobIdx;
		u16Idx = proc_ctx->u16JobIdx;

		memset(&proc_ctx->stJobInfo[u16Idx], 0, sizeof(proc_ctx->stJobInfo[u16Idx]));
		proc_ctx->stJobInfo[u16Idx].hHandle = (u64)(uintptr_t)job;
		memcpy(&proc_ctx->stJobInfo[u16Idx].identity, &job->identity, sizeof(job->identity));

		list_for_each_entry_safe(curelm, tmp, &job->task_list, node) {
			proc_ctx->stJobInfo[u16Idx].u32TaskNum++;
			proc_ctx->stJobInfo[u16Idx].tsk_info[i].u32InSize =
				curelm->attr.stImgIn.stVFrame.u32Width *
				curelm->attr.stImgIn.stVFrame.u32Height;
			proc_ctx->stJobInfo[u16Idx].tsk_info[i].u32OutSize =
				curelm->attr.stImgOut.stVFrame.u32Width *
				curelm->attr.stImgOut.stVFrame.u32Height;
			proc_ctx->stJobInfo[u16Idx].tsk_info[i].state = LDC_TASK_STATE_WAIT;
			proc_ctx->stJobInfo[u16Idx].tsk_info[i].type = curelm->type;
			i++;
			proc_ctx->stTskstatus.u32BeginNum++;
		}

		proc_ctx->stJobInfo[u16Idx].eState = LDC_JOB_WAIT;
		proc_ctx->stJobStatus.u32BeginNum++;
		proc_ctx->u16JobIdx++;

		ktime_get_ts64(&curTime);
		proc_ctx->stJobInfo[u16Idx].u64SubmitTime =
			(u64)(curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC);
		spin_unlock_irqrestore(&proc_ctx->lock, flags);
	}else {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "job or proc_ctx invalid.\n");
	}
}

