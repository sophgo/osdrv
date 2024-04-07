/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_vo_proc.h
 * Description:
 */

#ifndef _CVI_VIP_VO_PROC_H_
#define _CVI_VIP_VO_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/seq_file.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int vo_proc_init(void *shm);
int vo_proc_remove(void);
int vo_proc_show(struct seq_file *m, void *v);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_VO_PROC_H_ */
