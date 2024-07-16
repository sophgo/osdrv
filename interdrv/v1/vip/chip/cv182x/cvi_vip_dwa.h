/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_dwa.h
 * Description:
 */

#ifndef _CVI_VIP_DWA_H_
#define _CVI_VIP_DWA_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "uapi/cvi_vip_dwa.h"

struct cvi_dwa_data {
	__u32 bytesperline[VIP_MAX_PLANES];
	__u32 sizeimage[VIP_MAX_PLANES];
	__u16 w;
	__u16 h;
};

struct cvi_dwa_job {
	enum cvi_dwa_op op;
	struct cvi_dwa_data cap_data, out_data;

	__u64 mesh_id_addr;
	__u32 bgcolor;
};

struct cvi_dwa_ctx {
	struct list_head list;
	struct v4l2_fh fh;
	struct cvi_dwa_vdev *wdev;
	const struct cvi_vip_fmt *fmt;
	u32 colorspace;
	struct cvi_dwa_data cap_data, out_data;

	enum cvi_dwa_op op;
	u64 mesh_id_addr;
	u32 bgcolor;
};

int dwa_create_instance(struct platform_device *pdev);
int dwa_destroy_instance(struct platform_device *pdev);
void dwa_irq_handler(u8 intr_status, struct cvi_vip_dev *bdev);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_DWA_H_ */
