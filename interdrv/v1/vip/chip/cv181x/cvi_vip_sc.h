#ifndef _CVI_VIP_SC_H_
#define _CVI_VIP_SC_H_

#include "uapi/cvi_vip_sc.h"

int sc_create_instance(struct platform_device *pdev);
int sc_destroy_instance(struct platform_device *pdev);
void sc_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev);
void cvi_sc_device_run(void *priv, bool is_tile, bool is_work_on_r_tile, u8 grp_id);
void cvi_sc_update(struct cvi_sc_vdev *sdev, const struct cvi_vpss_chn_cfg *chn_cfg);

#endif
