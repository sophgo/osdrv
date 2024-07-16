/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_vi_proc.h
 * Description:
 */

#ifndef _CVI_VIP_VI_PROC_H_
#define _CVI_VIP_VI_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "cvi_vip_isp.h"

int vi_proc_init(struct cvi_isp_vdev *_vdev, void *shm);
int vi_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_VI_PROC_H_ */
