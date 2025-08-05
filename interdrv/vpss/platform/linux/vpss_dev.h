#ifndef _VPSS_DEV_H_
#define _VPSS_DEV_H_

#include <linux/miscdevice.h>

#include "vpss_define.h"
#include "vpss_core.h"
#include "osal.h"

struct vpss_dev_data {
	struct miscdevice miscdev;
	osal_atomic open_count;
	struct vpss_cores cores;
};

#endif
