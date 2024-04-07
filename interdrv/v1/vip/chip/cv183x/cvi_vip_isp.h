/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_vip_isp.h
 * Description:
 */

#ifndef __CVI_VIP_ISP_H__
#define __CVI_VIP_ISP_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/timekeeping.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/version.h>

#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>

#include "uapi/cvi_vip_isp.h"
#include "uapi/cvi_vip_isp_ext.h"
#include "uapi/cvi_vip_snsr.h"
#include "uapi/isp_reg.h"
#include "uapi/cvi_vip_tun_cfg.h"
#include "vip/isp_drv.h"
#include "vip/vip_common.h"
#include "cif/cif_drv.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_snsr_i2c.h"

#include "mw/vpu_base.h"

#define CVI_ISP_NEVENTS   11

void _isp_v4l2_event_queue(struct cvi_isp_vdev *vdev, const u32 type, const u32 frame_num);
void isp_mempool_setup(void);
int isp_create_instance(struct platform_device *pdev);
int isp_destroy_instance(struct platform_device *pdev);
void isp_irq_handler(union isp_intr top_sts, union isp_csi_intr cbdg_sts,
				struct cvi_vip_dev *bdev, union isp_csi_intr cbdg_sts_b);
void isp_ol_sc_trig_post(struct cvi_isp_vdev *vdev);

#ifdef __cplusplus
}
#endif

#endif /* __CVI_VIP_ISP_H__ */
