#ifndef _TDE_DEV_H_
#define _TDE_DEV_H_

#include <linux/miscdevice.h>

#include "tde_core.h"
#include "osal.h"

struct tde_dev_data {
	struct miscdevice miscdev;
	unsigned int irq_num;
	osal_atomic open_count;
	struct tde_core core;
};

#endif

