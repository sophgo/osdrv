/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_isp_proc.h
 * Description:
 */

#ifndef _CVI_VIP_ISP_PROC_H_
#define _CVI_VIP_ISP_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cvi_vip_isp.h"

int isp_proc_init(struct cvi_isp_vdev *_vdev);
int isp_proc_remove(void);
int isp_proc_setProcContent(void *buffer, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_ISP_PROC_H_ */
