/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_mipi_tx_proc.h
 * Description:
 */

#ifndef _CVI_VIP_MIPI_TX_PROC_H_
#define _CVI_VIP_MIPI_TX_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <generated/compile.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "mw/vpu_base.h"
#include "cvi_vip_mipi_tx.h"

int mipi_tx_proc_init(void);
int mipi_tx_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_MIPI_TX_PROC_H_ */
