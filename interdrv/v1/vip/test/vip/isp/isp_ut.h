/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: isp_ut.h
 * Description:
 */

#ifndef __ISP_UT_H__
#define __ISP_UT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <devmem.h>
#include <vip_msg_com.h>
#include <linux/cvi_vip_tun_cfg.h>

#define V4L2_EVENT_CVI_VIP_CLASS      (V4L2_EVENT_PRIVATE_START | 0x100)
#define V4L2_EVENT_CVI_VIP_PRE0_SOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x1)
#define V4L2_EVENT_CVI_VIP_PRE1_SOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x2)
#define V4L2_EVENT_CVI_VIP_PRE0_EOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x3)
#define V4L2_EVENT_CVI_VIP_PRE1_EOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x4)
#define V4L2_EVENT_CVI_VIP_POST_EOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x5)
#define V4L2_EVENT_CVI_VIP_POST1_EOF   (V4L2_EVENT_CVI_VIP_CLASS | 0x6)

#define ISP_EXT_CTRL(_fd, _cfg, _IOCTL_CFG)\
	do {\
		struct v4l2_ext_controls ecs1;\
		struct v4l2_ext_control ec1;\
		memset(&ecs1, 0, sizeof(ecs1));\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _IOCTL_CFG;\
		ec1.ptr = (void *)_cfg;\
		ecs1.controls = &ec1;\
		ecs1.count = 1;\
		ecs1.ctrl_class = V4L2_CTRL_ID2CLASS(ec1.id);\
		ut_v4l2_cmd(_fd, VIDIOC_S_EXT_CTRLS, (void *)&ecs1);\
	} while (0)

void isp_get_stt(int fd, enum cvi_isp_raw raw_num);
void isp_tuning_ctrl(int fd, const struct size s);

#ifdef __cplusplus
}
#endif

#endif /* __ISP_UT_H__ */
