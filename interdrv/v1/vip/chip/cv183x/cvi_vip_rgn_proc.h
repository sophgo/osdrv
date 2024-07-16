/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_rgn_proc.h
 * Description:
 */

#ifndef _CVI_VIP_RGN_PROC_H_
#define _CVI_VIP_RGN_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "mw/vpu_base.h"

int rgn_proc_init(void *shm);
int rgn_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_RGN_PROC_H_ */
