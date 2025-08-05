/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ldc_proc.c
 * Description: video pipeline graphic distortion correction engine info
 */
#include <aos/cli.h>
#include "ldc_proc.h"
// #include "cvi_base.h"
// #include "cvi_vip.h"
#include "base_ctx.h"
#include "ldc_debug.h"
#include "ldc_comm_layer.h"
#include "ldc_sdk.h"
#include "osal.h"

#define GENERATE_STRING(STRING)	(#STRING),
#define LDC_PROC_NAME "soph/ldc"

static const char *const MOD_STRING[] = FOREACH_MOD(GENERATE_STRING);
/*************************************************************************
 *	GDC proc functions
 *************************************************************************/
int ldc_proc_init(void *shm)
{
	int rc = 0, idx;
	struct ldc_proc_ctx **proc_ctx_addr = ldc_get_proc_ctx_addr();
	struct ldc_proc_ctx *proc_ctx;

	*proc_ctx_addr = (struct ldc_proc_ctx *)shm;
	proc_ctx = *proc_ctx_addr;
	osal_spin_lock_init(&proc_ctx->lock);

	for (idx = 0; idx < LDC_PROC_JOB_INFO_NUM; ++idx)
		proc_ctx->job_info[idx].handle = 0;
	return rc;
}

int ldc_proc_remove(void)
{
	struct ldc_proc_ctx **proc_ctx_addr = ldc_get_proc_ctx_addr();

	*proc_ctx_addr = NULL;

	return 0;
}

static int ldc_proc_show_tsk(struct ldc_proc_ctx *pldcCtx, int idx)
{
	int  j;
	char c[32], c2[32];

	printf("\t\t%15s%15s%15s%15s%15s%15s%15s\n",
		"tskNo", "UseCoreID", "inSize", "outSize", "State", "type", "HwTime(us)");

	for (j = 0; j < pldcCtx->job_info[idx].task_num; j++) {
		if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_RUNNING)
			strncpy(c, "RUNNING", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_DONE)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_CANCLE)
			strncpy(c, "CANCEL", sizeof(c));
		else if (pldcCtx->job_info[idx].tsk_info[j].state == LDC_TASK_STATE_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_ROT)
			strncpy(c2, "ROT", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_FISHEYE)
			strncpy(c2, "FISHEYE", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_AFFINE)
			strncpy(c2, "AFFINE", sizeof(c2));
		else if (pldcCtx->job_info[idx].tsk_info[j].type == LDC_TASK_TYPE_LDC)
			strncpy(c2, "LDC", sizeof(c2));
		else
			strncpy(c2, "UNKNOWN", sizeof(c2));

		printf("\t\t%14s%d%15d%15d%15d%15s%15s%15d\n",
			"#",
			j,
			pldcCtx->job_info[idx].tsk_info[j].top_id,
			pldcCtx->job_info[idx].tsk_info[j].in_size,
			pldcCtx->job_info[idx].tsk_info[j].out_size,
			c,
			c2,
			pldcCtx->job_info[idx].tsk_info[j].hw_time);
		pldcCtx->job_info[idx].hw_time += pldcCtx->job_info[idx].tsk_info[j].hw_time;
	}
	printf("\n----------------------------------------------------------------------------------------------------------------------------------------\n");

	return 0;
}

static void gdc_proc_show(int argc, char **argv)
{
	struct ldc_proc_ctx *pldcCtx = ldc_get_proc_ctx();
	struct ldc_ctx *ldc_ctx = get_ldc_ctx();
	unsigned long flags;
	int i, j, k, idx, total_handletime, total_hwTime, total_busyTime;
	char c[32];
	int idxs[LDC_PROC_JOB_INFO_NUM] = {0};
	static const char * const dev_name[] = {"ldc0", "ldc1", "dwa0", "dwa1"};

	if ((!pldcCtx)) {
		printf("ldc proc ctx is NULL\n");
		return;
	}

	osal_spin_lock_irqsave(&pldcCtx->lock, &flags);
	idx = pldcCtx->job_idx - 1;
	osal_spin_unlock_irqrestore(&pldcCtx->lock, &flags);

	// printf("\nModule: [LDC], Build Time[%s]\n", UTS_VERSION);

	if (idx < 0 || idx >= LDC_PROC_JOB_INFO_NUM) {
		if (idx >= LDC_PROC_JOB_INFO_NUM) {
			TRACE_LDC(DBG_ERR, "invalid proc job idx[%d], out of range[%d]\n", idx, LDC_PROC_JOB_INFO_NUM);
		}
		return;
	}

	// recent job info
	printf("\n------------------------------------------------------------RECENT JOB INFO--------------------------------------------------------\n");
	printf("%10s%15s%20s%15s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State", "CostTime(us)", "CoreID");
	printf("\n-----------------------------------------------------------------------------------------------------------------------------------\n");

	for (i = 0 ; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		if (!pldcCtx->job_info[idx].handle)
			break;

		osal_memset(c, 0, sizeof(c));
		idxs[i] = idx;
		if (pldcCtx->job_info[idx].state == LDC_JOB_WAIT)
			strncpy(c, "WAIT", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		printf("%9s%d%15s%20s%15d%15d%15d%15s%15d%15d\n",
			"#",
			i,
			(pldcCtx->job_info[idx].identity.mod_id == ID_BUTT) ?
				"USER" : MOD_STRING[pldcCtx->job_info[idx].identity.mod_id],
			pldcCtx->job_info[idx].identity.name,
			pldcCtx->job_info[idx].identity.sync_io,
			pldcCtx->job_info[idx].identity.id,
			pldcCtx->job_info[idx].task_num,
			c,
			pldcCtx->job_info[idx].cost_time,
			pldcCtx->job_info[idx].core_id);
		ldc_proc_show_tsk(pldcCtx, idx);

		idx = (--idx < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : idx;
	}

	// Max waste time job info
	printf("\n-------------------------------MAX WASTE TIME JOB INFO--------------------\n");
	printf("%10s%15s%20s%15s%15s%15s%15s%15s\n",
		"SeqNo", "Modname", "jobname", "sync_io", "id", "TaskNum", "State",
		"CostTime(us)");

	idx = i = pldcCtx->job_idx;
	for (j = 1; j < LDC_PROC_JOB_INFO_NUM; ++j) {
		i = (--i < 0) ? (LDC_PROC_JOB_INFO_NUM - 1) : i;
		if (!pldcCtx->job_info[i].handle)
			break;

		if (pldcCtx->job_info[i].cost_time > pldcCtx->job_info[idx].cost_time)
			idx = i;
	}

	for (i = 0 ; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		if (idxs[i] == idx)
			break;
	}

	if (pldcCtx->job_info[idx].handle) {
		osal_memset(c, 0, sizeof(c));
		if (pldcCtx->job_info[idx].state == LDC_JOB_END)
			strncpy(c, "DONE", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_FAIL)
			strncpy(c, "FAIL", sizeof(c));
		else if (pldcCtx->job_info[idx].state == LDC_JOB_WORKING)
			strncpy(c, "WORKING", sizeof(c));
		else
			strncpy(c, "UNKNOWN", sizeof(c));

		printf("%9s%d%15s%20s%15d%15d%15d%15s%15d\n",
			"#",
			i,
			MOD_STRING[pldcCtx->job_info[idx].identity.mod_id],
			pldcCtx->job_info[idx].identity.name,
			pldcCtx->job_info[idx].identity.sync_io,
			pldcCtx->job_info[idx].identity.id,
			pldcCtx->job_info[idx].task_num,
			c,
			pldcCtx->job_info[idx].cost_time);

		ldc_proc_show_tsk(pldcCtx, idx);
	}

	// LDC job status
	printf("\n-------------------------------LDC JOB STATUS-----------------------------\n");
	printf("%15s%15s%15s%15s%15s%15s\n",
		"Success", "Fail", "Cancel", "BeginNum", "WaitNum", "ProcingNum");

	printf("%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->job_status.success,
				pldcCtx->job_status.fail,
				pldcCtx->job_status.cancel,
				pldcCtx->job_status.begin_num,
				pldcCtx->job_status.wait_num,
				pldcCtx->job_status.procing_num);

	// LDC task status
	printf("\n-------------------------------LDC TASK STATUS----------------------------\n");
	printf("%15s%15s%15s%15s%15s%15s\n", "Success", "Fail", "Cancel", "Begin", "Procing", "WaitNum");
	printf("%15d%15d%15d%15d%15d%15d\n",
				pldcCtx->tsk_status.success,
				pldcCtx->tsk_status.fail,
				pldcCtx->tsk_status.cancel,
				pldcCtx->tsk_status.begin_num,
				pldcCtx->tsk_status.procing_num,
				pldcCtx->tsk_status.wait_num);

	// LDC interrupt status
	printf("\n-------------------------------LDC HW AVG STATUS-----------------------------\n");
	printf("%15s%15s%15s%15s%15s%15s%15s\n", "AVGNum", "TotalHwTm", "TotalBusyTm"
		, "TotalCostTm", "AVGHwTm", "AVGBusyTm", "AVGCostTm(us)");

	total_handletime = total_hwTime = total_busyTime = 0;
	for (i = 0; i < LDC_PROC_JOB_INFO_NUM; ++i) {
		total_hwTime += pldcCtx->job_info[i].hw_time;
		total_busyTime += pldcCtx->job_info[i].busy_time;
		total_handletime = total_hwTime + total_busyTime;
	}
	printf("%15d%15d%15d%15d%15d%15d%15d\n",
				LDC_PROC_JOB_INFO_NUM,
				total_hwTime,
				total_busyTime,
				total_handletime,
				total_hwTime / LDC_PROC_JOB_INFO_NUM,
				total_busyTime / LDC_PROC_JOB_INFO_NUM,
				total_handletime / LDC_PROC_JOB_INFO_NUM);

	// LDC call correction status
	printf("\n-------------------------------LDC CALL CORRECTION STATUS-----------------\n");
	printf("%15s%15s%15s%15s%15s\n", "TaskSuc", "TaskFail", "EndSuc", "EndFail", "CbCnt");
	printf("%15d%15d%15d%15d%15d\n",
				pldcCtx->fisheye_status.add_task_suc,
				pldcCtx->fisheye_status.add_task_fail,
				pldcCtx->fisheye_status.end_job_suc,
				pldcCtx->fisheye_status.end_job_fail,
				pldcCtx->fisheye_status.cb_cnt);


	// LDC core duty_ratio status
	printf("\n-------------------------------LDC HW DUTY RATIO STATUS-----------------\n");
	printf("%15s%15s%15s\n", "ID", "DEV", "DutyRatio");

	for (i = DEV_LDC_0; i < LDC_DEV_MAX_CNT; ++i) {
		printf("%14s%d%15s%15d\n",
			"#",
			i,
			dev_name[i],
			pldcCtx->gdc_core_status[i].duty_ratio);
	}

	// LDC attach vb_pool status
	printf("\n-------------------------------LDC ATTACH VB_POOL STATUS-----------------\n");
	printf("%15s%15s%15s%15s\n", "Module", "Device", "Channel", "PoolId");

	for (i = 0; i < MAX_CB_MOD_NUM; ++i) {
		for (j = 0; j < MAX_CB_DEV_NUM; ++j) {
			for (k = 0; k < MAX_CB_CHN_NUM; ++k) {
				if (ldc_ctx->vb_pool[i][j][k] != VB_INVALID_POOLID) {
					char dev_str[16], chn_str[16];
					snprintf(dev_str, sizeof(dev_str), "%s%d",
						(i == 0) ? "Pipe" : (i == 1) ? "Grp" : "Layer", j);
					snprintf(chn_str, sizeof(chn_str), "Chn%d", k);
					printf("%15s%15s%15s%15d\n",
						MOD_STRING[(i == 0) ? ID_VI : (i == 1) ? ID_VPSS : ID_VO],
						dev_str,
						chn_str,
						ldc_ctx->vb_pool[i][j][k]);
				}
			}
		}
	}
}

ALIOS_CLI_CMD_REGISTER(gdc_proc_show, proc_gdc, gdc proc info);
