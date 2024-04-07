#ifndef _CVI_VIP_VI_PROC_H_
#define _CVI_VIP_VI_PROC_H_

#include "cvi_vip_isp.h"

int vi_proc_init(struct cvi_isp_vdev *_vdev, void *shm);
int vi_proc_remove(void);

#endif // _CVI_VIP_VI_PROC_H_
