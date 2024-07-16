/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_ive_interface.h
 * Description: ive driver interface header file
 */

#ifndef __CVI_IVE_INTERFACE_H__
#define __CVI_IVE_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>

struct cvi_ive_device {
	struct device *dev;
	struct cdev cdev;
	void __iomem *ive_base; /* NULL if not initialized. */
	int ive_irq; /* alarm and periodic irq */
	struct proc_dir_entry *proc_dir;
	struct clk *clk;
	spinlock_t close_lock;
	struct completion ive_done;
	int use_count;
	void *private_data;
	void *IVE_BLK_BA;
};

#endif /* __CVI_IVE_INTERFACE_H__ */
