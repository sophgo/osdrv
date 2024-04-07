/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_dwa_proc.h
 * Description:
 */

#ifndef _CVI_VIP_DWA_PROC_H_
#define _CVI_VIP_DWA_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "dwa_common.h"

int dwa_proc_init(void *shm);
int dwa_proc_remove(void);
void dwa_proc_record_hw_tsk_start(struct dwa_job *job, struct dwa_task *tsk, u8 top_id);
void dwa_proc_record_hw_tsk_done(struct dwa_job *job, struct dwa_task *tsk);
void dwa_proc_record_job_start(struct dwa_job *job);
void dwa_proc_record_job_done(struct dwa_job *job);
void dwa_proc_commit_job(struct dwa_job *job);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_DWA_PROC_H_ */
