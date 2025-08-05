#ifndef __RGN_CORE_H__
#define __RGN_CORE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
#endif

#include <linux/miscdevice.h>
#include "base_cb.h"
#include "rgn_debug.h"
#include "rgn_uapi.h"
#include "osal.h"

/**
 * struct rgn - RGN IP abstraction
 */
struct rgn_dev {
	struct miscdevice	miscdev;
	struct device		*dev;
	osal_spinlock		lock;
	osal_spinlock		rdy_lock;
	osal_mutex		mutex;
	bool			bind_fb;
};

/*******************************************************
 *  File operations for core
 ******************************************************/
int rgn_release(struct inode *inode, struct file *filp);
int rgn_mmap(struct file *filp, struct vm_area_struct *vm);
unsigned int rgn_poll(struct file *filp, struct poll_table_struct *wait);
long _rgn_s_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p);
long _rgn_g_ctrl(struct rgn_dev *rdev, struct rgn_ext_control *p);

/*******************************************************
 *  Common interface for core
 ******************************************************/
int rgn_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg);
int rgn_create_instance(struct platform_device *pdev);
int rgn_destroy_instance(struct platform_device *pdev);
int _rgn_sw_init(struct rgn_dev *rdev);
int _rgn_release_op(struct rgn_dev *rdev);

#ifdef __cplusplus
}
#endif

#endif /* __RGN_CORE_H__ */
