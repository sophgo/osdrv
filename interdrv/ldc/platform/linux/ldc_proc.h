/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: vip_ldc_proc.h
 * Description:
 */

#ifndef _LDC_PROC_H_
#define _LDC_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <generated/compile.h>
#include "ldc_common.h"
#include "ldc_core.h"

int ldc_proc_init(void *shm);
int ldc_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _LDC_PROC_H_ */
