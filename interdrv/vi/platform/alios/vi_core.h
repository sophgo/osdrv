#ifndef __VI_CORE_H__
#define __VI_CORE_H__

#include "vi_defines.h"

struct platform_vi_dev {
	osal_atomic			dev_open_cnt;

	struct sop_vi_dev		vdev;
};

#endif
