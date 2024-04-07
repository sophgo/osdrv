/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_gdc_proc.h
 * Description:
 */

#ifndef _CVI_VIP_GDC_PROC_H_
#define _CVI_VIP_GDC_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int gdc_proc_init(void *shm);
int gdc_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_GDC_PROC_H_ */
