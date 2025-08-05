#ifndef _LDC_CORE_H_
#define _LDC_CORE_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <ldc_uapi.h>
#include <ldc_common.h>
#include <ldc_ctx.h>

#ifdef DEFAULT_STACK_SIZE
#undef DEFAULT_STACK_SIZE
#endif

#define DEFAULT_STACK_SIZE 0

struct ldc_vdev {
	struct miscdevice miscdev;
	struct ldc_ctx ctx;
};

#endif
