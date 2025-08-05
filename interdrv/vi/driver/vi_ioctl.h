#ifndef __VI_IOCTL_H__
#define __VI_IOCTL_H__

#include "vi_defines.h"
#include "vi_sdk_layer.h"

long vi_ioctl(struct sop_vi_dev *vdev, u_int cmd, struct vi_ctrl *ctrl_cfg);

#endif
