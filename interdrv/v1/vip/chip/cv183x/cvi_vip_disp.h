/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_disp.h
 * Description:
 */

#ifndef _CVI_VIP_DISP_H_
#define _CVI_VIP_DISP_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "uapi/cvi_vip_disp.h"

#define CVI_DISP_NEVENTS   5

extern int smooth;

int disp_create_instance(struct platform_device *pdev);
int disp_destroy_instance(struct platform_device *pdev);
void disp_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_DISP_H_ */
