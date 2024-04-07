/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_ldc_proc.h
 * Description:
 */

#ifndef _CVI_VIP_LDC_PROC_H_
#define _CVI_VIP_LDC_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "ldc_common.h"

int ldc_proc_init(void *shm);
int ldc_proc_remove(void);
void ldc_proc_record_hw_tsk_start(struct ldc_job *job, struct ldc_task *tsk, u8 top_id);
void ldc_proc_record_hw_tsk_done(struct ldc_job *job, struct ldc_task *tsk);
void ldc_proc_record_job_start(struct ldc_job *job);
void ldc_proc_record_job_done(struct ldc_job *job);
void ldc_proc_commit_job(struct ldc_job *job);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_LDC_PROC_H_ */
