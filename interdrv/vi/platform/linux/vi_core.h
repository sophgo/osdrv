#ifndef __VI_CORE_H__
#define __VI_CORE_H__

#include "vi_defines.h"
#include <linux/atomic.h>

struct platform_vi_dev {
	struct device			*dev;
	struct class			*vi_class;
	struct cdev			cdev;
	dev_t				cdev_id;
	atomic_t			dev_open_cnt;

	struct vi_dev		vdev;
};

#endif
