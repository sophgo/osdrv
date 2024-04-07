#ifndef _CVI_VIP_DISP_H_
#define _CVI_VIP_DISP_H_

#include "uapi/cvi_vip_disp.h"

#define CVI_DISP_NEVENTS   5

extern int smooth;

int disp_create_instance(struct platform_device *pdev);
int disp_destroy_instance(struct platform_device *pdev);
void disp_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev);

#endif
