/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_img.h
 * Description:
 */

#ifndef _CVI_VIP_IMG_H_
#define _CVI_VIP_IMG_H_

#ifdef __cplusplus
	extern "C" {
#endif

int img_create_instance(struct platform_device *pdev);
void img_irq_handler(union sclr_intr intr_status, u8 cmdq_intr_status, struct cvi_vip_dev *bdev);
int img_destroy_instance(struct platform_device *pdev);
int cvi_img_get_input(enum sclr_img_in img_type,
	enum cvi_input_type input_type, enum sclr_input *input);
void cvi_img_update(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg);
void cvi_img_v4l2_event_queue(
	struct cvi_img_vdev *vdev, const u32 type, const u8 grp_id, const u8 sc_early_ids);
void cvi_img_get_sc_bound(struct cvi_img_vdev *idev, bool sc_bound[]);

#ifdef __cplusplus
}
#endif

#endif /* _CVI_VIP_IMG_H_ */