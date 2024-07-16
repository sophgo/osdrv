#ifndef _CVI_VIP_ISP_PROC_H_
#define _CVI_VIP_ISP_PROC_H_

#include "cvi_vip_isp.h"

int isp_proc_init(struct cvi_isp_vdev *_vdev);
int isp_proc_remove(void);
int isp_proc_setProcContent(void *buffer, size_t count);

#endif // _CVI_VIP_ISP_PROC_H_
