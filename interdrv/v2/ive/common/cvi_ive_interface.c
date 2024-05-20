// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_ive_interface.c
 * Description: ive kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <asm/atomic.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/signal.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "cvi_ive_interface.h"
#include "cvi_ive_platform.h"
#include "ive_debug.h"

#define CVI_IVE_CDEV_NAME "soph-ive"
#define CVI_IVE_CLASS_NAME "soph-ive"
#define CVI_IVE_PROC_NAME "soph/ive_hw_profiling"

static const char *const ive_clk_name[IVE_DEV_MAX][2] = {
						{"clk_vi_sys3", "clk_ive0"},
						{"clk_vi_sys3", "clk_ive1"}};


#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
uint32_t get_duration_us(const struct timespec64 *start,
				const struct timespec64 *end)
#else
uint32_t get_duration_us(const struct timespec *start,
				const struct timespec *end)
#endif
{
	uint32_t event_duration_us;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	struct timespec64 temp;
#else
	struct timespec temp;
#endif
	if ((end->tv_nsec - start->tv_nsec) < 0) {
		temp.tv_sec = end->tv_sec - start->tv_sec - 1;
		temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
	} else {
		temp.tv_sec = end->tv_sec - start->tv_sec;
		temp.tv_nsec = end->tv_nsec - start->tv_nsec;
	}
	event_duration_us = (uint32_t) (temp.tv_nsec / 1000);
	event_duration_us += temp.tv_sec * 1000000;

	return event_duration_us;
}

#define IVE_IRQ_THREAD_FUNC 0
u32 ive_log_lv = CVI_DBG_ERR;
// static char *g_kdata;
struct class *class_id;
static dev_t cdev_id;
static uint32_t g_enable_usage_profiling;
static struct ive_profiling_info *g_time_infos;

const char * const ive_irq_name[IVE_DEV_MAX] = {"ive_irq0", "ive_irq1"};

// proc_operations function
static int ive_proc_open(struct inode *inode, struct file *file);
static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos);
// file_operations function
static int cvi_ive_open(struct inode *inode, struct file *filp);
static int cvi_ive_close(struct inode *inode, struct file *filp);
static long cvi_ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg);
#ifdef CONFIG_COMPAT
static long cvi_ive_compat_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg);
#endif
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops ive_proc_ops = {
	.proc_open = ive_proc_open,
	.proc_read = seq_read,
	.proc_write = ive_proc_write,
	.proc_release = single_release,
};
#else
static const struct file_operations ive_proc_ops = {
	.owner = THIS_MODULE,
	.open = ive_proc_open,
	.read = seq_read,
	.write = ive_proc_write,
	.release = single_release,
};
#endif

static const struct file_operations ive_fops = {
	.owner = THIS_MODULE,
	.open = cvi_ive_open,
	.release = cvi_ive_close,
	.unlocked_ioctl = cvi_ive_ioctl, //2.6.36
#ifdef CONFIG_COMPAT
	.compat_ioctl = cvi_ive_compat_ioctl, //2.6.36
#endif
};

static void start_ioctl_time(struct ive_profiling_info *pinfo, char *name)
{
	int i = 0;

	if (g_enable_usage_profiling) {
		strcpy(pinfo->op_name, name);
		pinfo->time_ioctl_diff_us = 0;
		for (i = 0; i < 6; i++) {
			pinfo->time_vld_diff_us[i] = 0;
		}
		pinfo->time_tile_diff_us = 0;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&pinfo->time_ioctl_start);
#else
		getnstimeofday(&pinfo->time_ioctl_start);
#endif
	}
}

static void stop_ioctl_time(struct ive_profiling_info *pinfo)
{
	if (g_enable_usage_profiling) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&pinfo->time_ioctl_end);
#else
		getnstimeofday(&pinfo->time_ioctl_end);
#endif
		pinfo->time_ioctl_diff_us =
			get_duration_us(&pinfo->time_ioctl_start, &pinfo->time_ioctl_end);
	}
}

void start_vld_time(int optype)
{
	if (g_enable_usage_profiling && optype < MOD_ALL &&
		optype >= MOD_BYP &&
		strlen(g_time_infos[optype].op_name) > 0) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&g_time_infos[optype].time_vld_start);
#else
		getnstimeofday(&g_time_infos[optype].time_vld_start);
#endif
	}
}

void stop_vld_time(int optype, int tile_num)
{
	if (tile_num > 6)
		return;
	if (g_enable_usage_profiling && optype < MOD_ALL &&
		optype >= MOD_BYP &&
		strlen(g_time_infos[optype].op_name) > 0) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		ktime_get_real_ts64(&g_time_infos[optype].time_vld_end);
#else
		getnstimeofday(&g_time_infos[optype].time_vld_end);
#endif
		g_time_infos[optype].time_vld_diff_us[tile_num] =
			get_duration_us(&g_time_infos[optype].time_vld_start,
			&g_time_infos[optype].time_vld_end);
	}
	g_time_infos[optype].time_tile_diff_us +=
		g_time_infos[optype].time_vld_diff_us[tile_num];
}


#if IVE_IRQ_THREAD_FUNC
static void ive_frame_finish(struct cvi_ive_device *dev, int dev_id)
{
	//struct ive_dev_core *ive_core = &dev->core[dev_id];

	return;

	//to do
#if 0
	if (atomic_read(&core->state) != IVE_CORE_STATE_END)
		return;

	atomic_set(&core->state, IVE_CORE_STATE_IDLE);
#endif
}

static irqreturn_t ive_irq_finish_thread_func(int irq, void *data)
{
	struct cvi_ive_device *ndev = (struct cvi_ive_device *)data;
	int i, dev_id = -1;

	if (!ndev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid ive_device\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < IVE_DEV_MAX; i++) {
		if (ndev->ive_irq[i] == irq) {
			dev_id = i;
			break;
		}
	}

	if (dev_id >= 0 && dev_id < IVE_DEV_MAX)
		ive_frame_finish(ndev, dev_id);
	else
		CVI_TRACE_IVE(CVI_DBG_ERR, "invalid ive_dev num[%d]\n", dev_id);
	return IRQ_HANDLED;
}
#endif

static irqreturn_t cvi_ive_irq_handler(int irq, void *data)
{
	struct cvi_ive_device *ndev = data;
	irqreturn_t ret;
	int i, dev_id = -1;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		if (ndev->ive_irq[i] == irq) {
			dev_id = i;
			break;
		}
	}

	spin_lock(&ndev->core[dev_id].dev_lock);
	//pr_info("[IVE] ive use_count %d\n", ndev->use_count);
	if (ndev->use_count == 0) {
		atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
		spin_unlock(&ndev->core[dev_id].dev_lock);
		return IRQ_HANDLED;
	}

	ret = platform_ive_irq(ndev, dev_id);
	atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
	spin_unlock(&ndev->core[dev_id].dev_lock);

	return ret;
}

static int ive_proc_show(struct seq_file *m, void *v)
{
	int i = 0, tile = 0;

	if (g_enable_usage_profiling) {
		char const *row_name[] = {"op name", "start(s)", "ioctl(us)",
							"tile0(us)", "tile1(us)", "tile2(us)", "tile3(us)",
							"tile4(us)", "tile5(us)", "tileSum(us)"};
		int row_space[] = { -15, 10, 10, 10, 10, 10, 10, 10, 10, 10};
		int table[] = { 20, 21, 22, 23, 24, 3, 2, 25, 26, 27,
						28, 31, 33, 35, 1, 29, 30, 4, 6, 7,
						8, 9, 10, 11, 15, 16, 17, 19, 18, 36,
						12, 34, 13, 14, 32, 5};

		seq_puts(m, "[IVE] ive time profiling\n");
		seq_printf(m, "%*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s| %*s\n",
		row_space[0], row_name[0], row_space[1], row_name[1],
		row_space[2], row_name[2], row_space[3], row_name[3],
		row_space[4], row_name[4], row_space[5], row_name[5],
		row_space[6], row_name[6], row_space[7], row_name[7],
		row_space[8], row_name[8], row_space[9], row_name[9]);

		for (i = 0; i < 36; i++) {
			uint32_t second_vld_time[6] = {0};
			uint32_t second_tile_time = 0;
			uint32_t id = table[i];

			if (strlen(g_time_infos[id].op_name) > 0) {
				if (id == 10) {
					for (tile = 0; tile < 6; tile++) {
						second_vld_time[tile] = g_time_infos[5].time_vld_diff_us[tile];
					}
					second_tile_time = g_time_infos[5].time_tile_diff_us;
				} else if (id == 5) {
					continue;
				}
				seq_printf(
					m, "%*s| %*lld| %*u| %*d| %*d| %*d| %*d| %*d| %*d| %*d\n",
					row_space[0],
					g_time_infos[id].op_name,
					row_space[1],
					g_time_infos[id].time_ioctl_start.tv_sec,
					row_space[2],
					g_time_infos[id].time_ioctl_diff_us,
					row_space[3],
					g_time_infos[id].time_vld_diff_us[0] + second_vld_time[0],
					row_space[4],
					g_time_infos[id].time_vld_diff_us[1] + second_vld_time[1],
					row_space[5],
					g_time_infos[id].time_vld_diff_us[2] + second_vld_time[2],
					row_space[6],
					g_time_infos[id].time_vld_diff_us[3] + second_vld_time[3],
					row_space[7],
					g_time_infos[id].time_vld_diff_us[4] + second_vld_time[4],
					row_space[8],
					g_time_infos[id].time_vld_diff_us[5] + second_vld_time[5],
					row_space[9],
					g_time_infos[id].time_tile_diff_us + second_tile_time);
			}
		}
	} else {
		seq_puts(m, "[IVE] ive time profiling is disabled\n");
	}
	return 0;
}

static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	uint32_t user_input_param = 0, i = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &user_input_param)) {
		pr_err("\n[IVE] input parameter incorrect\n");
		return count;
	}

	// reset related info
	if (user_input_param == 0) {
		g_enable_usage_profiling = 0;
		pr_err("\n[IVE] Time profiling is ended\n");
	} else if (user_input_param == 1) {
		for (i = 0; i < MOD_ALL; i++) {
			memset(&g_time_infos[i], 0,
			       sizeof(struct ive_profiling_info));
		}
		g_enable_usage_profiling = 1;
		pr_err("\n[IVE] Time profiling is started\n");
	} else if (user_input_param == 2) {
		cvi_ive_set_reg_dump(true);
		pr_err("\n[IVE] Enable dump reg state\n");
	} else if (user_input_param == 3) {
		cvi_ive_set_reg_dump(false);
		pr_err("\n[IVE] Disable dump reg state\n");
	} else if (user_input_param == 4) {
		cvi_ive_set_dma_dump(true);
		pr_err("\n[IVE] Enable dump dma phy addr\n");
	} else if (user_input_param == 5) {
		cvi_ive_set_dma_dump(false);
		pr_err("\n[IVE] Disable dump dma phy addr\n");
	} else if (user_input_param == 6) {
		cvi_ive_set_img_dump(true);
		pr_err("\n[IVE] Enable dump IVE_IMAGE_S, IVE_DATA_S, IVE_MEM_INFO_S\n");
	} else if (user_input_param == 7) {
		cvi_ive_set_img_dump(false);
		pr_err("\n[IVE] Disable dump IVE_IMAGE_S, IVE_DATA_S, IVE_MEM_INFO_S\n");
	} else if (user_input_param == 10) {
		cvi_ive_dump_op1_op2_info();
	} else if (user_input_param == 11) {
		cvi_ive_dump_hw_flow();
	} else {
		pr_err("\nIVE Command List:\n"
				"\t0: Set time profiling stop\n"
				"\t1: Set time profiling start\n"
				"\t2: Enable print reg state\n"
				"\t3: Disable print reg state\n"
				"\t4: Enable print dma phy addr\n"
				"\t5: Disable print dma phy addr\n"
				"\t6: Enable print IVE_IMAGE_S, IVE_DATA_S, IVE_MEM_INFO_S\n"
				"\t7: Disable print IVE_IMAGE_S, IVE_DATA_S, IVE_MEM_INFO_S\n"
				"\t10: Dump ive op1/op2 mode info\n"
				"\t11: Dump hardware info\n");
	}
	return count;
}

static int ive_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ive_proc_show, PDE_DATA(inode));
}

#ifdef CONFIG_COMPAT
static long cvi_ive_compat_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd,
					  (unsigned long)compat_ptr(arg));
}
#endif
static long ive_add_task(struct cvi_ive_device *ndev, unsigned int cmd, unsigned long arg, char *kdata, void *buffer)
{
	struct ive_task *task;
	unsigned long flags;
	s32 sync_io_ret = 1;
	s32 ret = -1;
	unsigned long timeout = msecs_to_jiffies(IVE_SYNC_IO_WAIT_TIMEOUT_MS);

	if (ndev == NULL) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "drv point is NULL\n");
		return ret;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	task = kzalloc(sizeof(struct ive_task), GFP_ATOMIC);
	atomic_set(&task->state, IVE_TASK_STATE_READY);
	list_add_tail(&task->node, &ndev->tsk_list);
	task->dev_id = -1;
	task->task_type = cmd;
	task->input_data = kdata;
	task->buffer = buffer;
	init_waitqueue_head(&task->task_done_wait);
	task->task_done_evt = false;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	CVI_TRACE_IVE(CVI_DBG_DEBUG, "add task success, task[%p], input data [%p], task type[%d]\n"
		, task, task->input_data, _IOC_NR(cmd));

	ive_notify_wkup_evt_kth(ndev, IVE_EVENT_WKUP);

	sync_io_ret = wait_event_timeout(task->task_done_wait, task->task_done_evt, timeout);
	if (sync_io_ret <= 0) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "user thread wait timeout, ret[%d]\n", sync_io_ret);
		ret = -1;
	} else
		ret = 0;
	ive_notify_wkup_evt_kth(ndev, IVE_EVENT_EOF);


	if (atomic_read(&task->state) == IVE_TASK_STATE_DONE) {
		kfree(task);
		task = NULL;
	}
	return ret;

}
static void ive_task_done(struct cvi_ive_device *ndev, struct ive_task *task)
{
	unsigned long flags;

	if (!task) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "task is NULL\n");
		return;
	}

	if (atomic_read(&task->state) == IVE_TASK_STATE_RUNNING) {
		atomic_set(&task->state, IVE_TASK_STATE_DONE);
		atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);

		spin_lock_irqsave(&ndev->close_lock, flags);
		task->task_done_evt = true;
		spin_unlock_irqrestore(&ndev->close_lock, flags);
		wake_up(&task->task_done_wait);

		CVI_TRACE_IVE(CVI_DBG_DEBUG, "task done ,wake up user thread\n");
	} else {
		CVI_TRACE_IVE(CVI_DBG_DEBUG, "task state is invaild\n");
	}
}

void ive_notify_wkup_evt_kth(void *data, enum ive_wait_evt evt)
{
	struct cvi_ive_device *ndev = (struct cvi_ive_device *)data;
	unsigned long flags;


	if (!ndev) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "ive device isn't created yet\n");
		return;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->evt |= evt;
	CVI_TRACE_IVE(CVI_DBG_DEBUG, "wakeup kthread dev evt[%d]", ndev->evt);
	spin_unlock_irqrestore(&ndev->close_lock, flags);

	wake_up_interruptible(&ndev->wait);
}

static long cvi_ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct cvi_ive_device *ndev = filp->private_data;
	struct cvi_ive_ioctl_arg *ioctl_arg;
	s32 ret = -1;
	char *kdata = NULL;
	bool copy_buf = false;
	void *buffer = NULL;

	ioctl_arg = (struct cvi_ive_ioctl_arg *)arg;
	kdata = kmalloc(512,GFP_KERNEL);
	if (!kdata) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "vmalloc fail\n");
		goto error;
	}
	if (copy_from_user(ioctl_arg, (void __user *)arg, sizeof(ioctl_arg)) != 0) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
		goto error;
	}
	if (copy_from_user(kdata, (void __user *)ioctl_arg->input_data, 512) != 0) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
		goto error;
	}

	if (cmd == CVI_IVE_IOC_Map || cmd == CVI_IVE_IOC_NCC ||
		cmd == CVI_IVE_IOC_CCL || cmd == CVI_IVE_IOC_MatchBgModel ||
		cmd == CVI_IVE_IOC_UpdateBgModel) {

		buffer = kmalloc(ioctl_arg->u32Size,GFP_KERNEL);
		if (!buffer) {
			CVI_TRACE_IVE(CVI_DBG_ERR, "vmalloc fail\n");
			goto error;
		}
		copy_buf = true;
		if (copy_from_user(buffer, (void __user *)ioctl_arg->buffer, ioctl_arg->u32Size) != 0) {
			CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
			goto error;
		}
	}

	ret = ive_add_task(ndev, cmd, arg, kdata, buffer);

	if (copy_buf) {
		if (copy_to_user((void __user *)ioctl_arg->buffer, buffer, ioctl_arg->u32Size) != 0) {
			CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
			ret = -EFAULT;
		}
	}

	if (copy_to_user((void __user *)ioctl_arg->input_data, kdata, 512) != 0) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
		ret = -EFAULT;
	}

	if (copy_to_user((void __user *)arg, ioctl_arg, sizeof(ioctl_arg)) != 0) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "copy to user fail\n");
		ret = -EFAULT;
	}
	if (kdata) {
		kfree(kdata);
		kdata = NULL;
	}

	if (copy_buf && buffer) {
		kfree(buffer);
		buffer= NULL;
	}

	return ret;

error:
	if (kdata) {
		kfree(kdata);
		kdata = NULL;
	}

	if (copy_buf && buffer) {
		kfree(buffer);
		buffer= NULL;
	}
	return -1;
}

static void ive_clk_init(struct cvi_ive_device *ndev)
{
	u8 i;
	struct ive_dev_core core;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		core = ndev->core[i];
		if (core.clk_src)
			clk_prepare_enable(core.clk_src);

		if (core.clk_ive)
			clk_prepare_enable(core.clk_ive);
	}
}

static void ive_clk_deinit(struct cvi_ive_device *ndev)
{
	u8 i;
	struct ive_dev_core core;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		core = ndev->core[i];
		if (core.clk_src)
			clk_disable_unprepare(core.clk_src);

		if (core.clk_ive)
			clk_disable_unprepare(core.clk_ive);
	}
}

static int cvi_ive_open(struct inode *inode, struct file *filp)
{
	//struct cvi_ive_device *ndev =
	//	container_of(filp->private_data, struct cvi_ive_device, miscdev);
	struct cvi_ive_device *ndev =
		container_of(inode->i_cdev, struct cvi_ive_device, cdev);
	unsigned long flags = 0;

	//open clk
	ive_clk_init(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count++;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = ndev;
	return 0;
}

static int cvi_ive_close(struct inode *inode, struct file *filp)
{
	struct cvi_ive_device *ndev = filp->private_data;
	unsigned long flags = 0;

	//close clk
	ive_clk_deinit(ndev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count--;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	filp->private_data = NULL;

	return 0;
}

//int cvi_ive_register_misc(struct cvi_ive_device *ndev)
//{
//	int rc;
//
//	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
//	ndev->miscdev.name = CVI_IVE_CDEV_NAME;
//	ndev->miscdev.fops = &ive_fops;
//
//	rc = misc_register(&ndev->miscdev);
//	if (rc) {
//		dev_err(ndev->dev,
//		"cvi_ive: failed to register misc device.\n");
//		return rc;
//	}
//
//	return 0;
//}

int cvi_ive_register_cdev(struct cvi_ive_device *ndev)
{
	int ret;
	// Create device to /sys/class/
	class_id = class_create(THIS_MODULE, CVI_IVE_CLASS_NAME);
	if (IS_ERR(class_id)) {
		pr_err("[IVE] create class failed\n");
		return PTR_ERR(class_id);
	}
	// Apply for a character device driver id (cdev)
	ret = alloc_chrdev_region(&cdev_id, 0, 1, CVI_IVE_CDEV_NAME);
	if (ret < 0) {
		pr_err("[IVE] alloc chrdev failed\n");
		return ret;
	}
	// Init character device and link file_ops
	cdev_init(&ndev->cdev, &ive_fops);
	ndev->cdev.owner = THIS_MODULE;
	// Add cdev to kernel character device list
	cdev_add(&ndev->cdev, cdev_id, 1);
	// Automatically create device node under /dev/
	device_create(class_id, ndev->dev, cdev_id, NULL, "%s",
		      CVI_IVE_CDEV_NAME);

	return 0;
}

static u8 ive_get_idle_coreid(struct cvi_ive_device *ndev)
{
	u8 coreid;
	enum ive_core_state state;

	for (coreid = 0; coreid < IVE_DEV_MAX; coreid++) {
		state = atomic_read(&ndev->core[coreid].dev_state);
		if (state == IVE_CORE_STATE_END)
			break;
	}

	return coreid;
}

static void ive_submit_hw(struct cvi_ive_device *ndev, int top_id, struct ive_task *task)
{
	unsigned long flags;
	s32 ret = -1;
	char *g_kdata;

	g_kdata = task->input_data;
	spin_lock_irqsave(&ndev->close_lock, flags);
	task->dev_id = top_id;
	atomic_set(&task->state, IVE_TASK_STATE_RUNNING);
	atomic_set(&ndev->core[top_id].dev_state, IVE_CORE_STATE_RUNNING);
	ndev->core[top_id].work_tsk = task;
	spin_unlock_irqrestore(&ndev->close_lock, flags);
	CVI_TRACE_IVE(CVI_DBG_DEBUG, "use core[%d]\n", task->dev_id);
	CVI_TRACE_IVE(CVI_DBG_DEBUG, "task type[%d], input ptr[%p]\n", _IOC_NR(task->task_type), task->input_data);


		switch (task->task_type) {
		case CVI_IVE_IOC_QUERY: {
			bool bFinish;
			struct cvi_ive_query_arg *val =
					(struct cvi_ive_query_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_QUERY], "QUERY");
			ret = copy_from_user(&bFinish,
							(void __user *)val->pbFinish,
							sizeof(bool));
			ret = cvi_ive_query(ndev, &bFinish, val->bBlock, task->dev_id);
			ret = copy_to_user((void __user *)val->pbFinish,
						&bFinish, sizeof(bool));
			stop_ioctl_time(&g_time_infos[MOD_QUERY]);
		} break;
		case CVI_IVE_IOC_RESET: {
			start_ioctl_time(&g_time_infos[MOD_RESET], "RESET");
			ret = cvi_ive_reset(ndev, *((int *) g_kdata), task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_RESET]);
		} break;
		case CVI_IVE_IOC_DUMP: {
			start_ioctl_time(&g_time_infos[MOD_DUMP], "DUMP");
			ret = cvi_ive_dump_reg_state(true);    // not use dev_id ,use d_num
			stop_ioctl_time(&g_time_infos[MOD_DUMP]);
		} break;
		case CVI_IVE_IOC_TEST: {
			struct cvi_ive_test_arg *val =
					(struct cvi_ive_test_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_TEST], "Test");
			ret = cvi_ive_test(ndev, val->pAddr, &val->u16Width,
						&val->u16Height);
			stop_ioctl_time(&g_time_infos[MOD_TEST]);
		} break;
		case CVI_IVE_IOC_DMA: {
			struct cvi_ive_ioctl_dma_arg *val =
					(struct cvi_ive_ioctl_dma_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_DMA], "DMA");
			ret = cvi_ive_dma(ndev, &val->stSrc, &val->stDst,
						&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_DMA]);
		} break;
		case CVI_IVE_IOC_And: {
			struct cvi_ive_ioctl_and_arg *val =
					(struct cvi_ive_ioctl_and_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_AND], "And");
			ret = cvi_ive_and(ndev, &val->stSrc1, &val->stSrc2,
						&val->stDst, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_AND]);
		} break;
		case CVI_IVE_IOC_Or: {
			struct cvi_ive_ioctl_or_arg *val =
					(struct cvi_ive_ioctl_or_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_OR], "Or");
			ret = cvi_ive_or(ndev, &val->stSrc1, &val->stSrc2,
						&val->stDst, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_OR]);
		} break;
		case CVI_IVE_IOC_Xor: {
			struct cvi_ive_ioctl_xor_arg *val =
					(struct cvi_ive_ioctl_xor_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_XOR], "Xor");
			ret = cvi_ive_xor(ndev, &val->stSrc1, &val->stSrc2,
						&val->stDst, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_XOR]);
		} break;
		case CVI_IVE_IOC_Add: {
			struct cvi_ive_ioctl_add_arg *val =
					(struct cvi_ive_ioctl_add_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ADD], "Add");
			ret = cvi_ive_add(ndev, &val->stSrc1, &val->stSrc2,
						&val->stDst, &val->pstCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ADD]);
		} break;
		case CVI_IVE_IOC_Sub: {
			struct cvi_ive_ioctl_sub_arg *val =
					(struct cvi_ive_ioctl_sub_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SUB], "Sub");
			ret = cvi_ive_sub(ndev, &val->stSrc1, &val->stSrc2,
						&val->stDst, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SUB]);
		} break;
		case CVI_IVE_IOC_Thresh: {
			struct cvi_ive_ioctl_thresh_arg *val =
					(struct cvi_ive_ioctl_thresh_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRESH], "Thresh");
			ret = cvi_ive_thresh(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRESH]);
		} break;
		case CVI_IVE_IOC_Dilate: {
			struct cvi_ive_ioctl_dilate_arg *val =
					(struct cvi_ive_ioctl_dilate_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_DILA], "Dilate");
			ret = cvi_ive_dilate(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_DILA]);
		} break;
		case CVI_IVE_IOC_Erode: {
			struct cvi_ive_ioctl_erode_arg *val =
					(struct cvi_ive_ioctl_erode_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ERO], "Erode");
			ret = cvi_ive_erode(ndev, &val->stSrc, &val->stDst,
						&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ERO]);
		} break;
		case CVI_IVE_IOC_MatchBgModel: {
			struct cvi_ive_ioctl_match_bgmodel_arg *val =
					(struct cvi_ive_ioctl_match_bgmodel_arg *) g_kdata;

			task->buffer = (IVE_BG_STAT_DATA_S *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_BGM], "MatchBgModel");
			ret = cvi_ive_match_bg_model(ndev, &val->stCurImg,
							&val->stBgModel,
							&val->stFgFlag, &val->stDiffFg,
							task->buffer, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BGM]);
		} break;
		case CVI_IVE_IOC_UpdateBgModel: {
			struct cvi_ive_ioctl_update_bgmodel_arg *val =
					(struct cvi_ive_ioctl_update_bgmodel_arg *) g_kdata;

			task->buffer = (IVE_BG_STAT_DATA_S *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_BGU], "UpdateBgModel");
			ret = cvi_ive_update_bg_model(ndev, &val->stCurImg,
							&val->stBgModel,
							&val->stFgFlag, &val->stBgImg,
							&val->stChgSta,
							task->buffer,
							&val->stCtrl, val->bInstant,
							task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BGU]);
		} break;
		case CVI_IVE_IOC_GMM: {
			struct cvi_ive_ioctl_gmm_arg *val =
					(struct cvi_ive_ioctl_gmm_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GMM], "GMM");
			ret = cvi_ive_gmm(ndev, &val->stSrc, &val->stFg,
						&val->stBg, &val->stModel, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GMM]);
		} break;
		case CVI_IVE_IOC_GMM2: {
			struct cvi_ive_ioctl_gmm2_arg *val =
					(struct cvi_ive_ioctl_gmm2_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GMM2], "GMM2");
			ret = cvi_ive_gmm2(ndev, &val->stSrc, &val->stFactor,
						&val->stFg, &val->stBg, &val->stInfo,
						&val->stModel, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GMM2]);
		} break;

		case CVI_IVE_IOC_Bernsen: {
			struct cvi_ive_ioctl_bernsen_arg *val =
					(struct cvi_ive_ioctl_bernsen_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_BERNSEN], "Bernsen");
			ret = cvi_ive_bernsen(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BERNSEN]);
		} break;
		case CVI_IVE_IOC_Filter: {
			struct cvi_ive_ioctl_filter_arg *val =
					(struct cvi_ive_ioctl_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_FILTER3CH], "Filter");
			ret = cvi_ive_filter(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_FILTER3CH]);
		} break;
		case CVI_IVE_IOC_Sobel: {
			struct cvi_ive_ioctl_sobel_arg *val =
					(struct cvi_ive_ioctl_sobel_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SOBEL], "Sobel");
			ret = cvi_ive_sobel(ndev, &val->stSrc, &val->stDstH,
						&val->stDstV, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SOBEL]);
		} break;
		case CVI_IVE_IOC_MagAndAng: {
			struct cvi_ive_ioctl_maganang_arg *val =
					(struct cvi_ive_ioctl_maganang_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_MAG], "MagAndAng");
			ret = cvi_ive_mag_and_ang(ndev, &val->stSrc, &val->stDstMag,
						&val->stDstAng, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MAG]);
		} break;
		case CVI_IVE_IOC_CSC: {
			struct cvi_ive_ioctl_csc_arg *val =
					(struct cvi_ive_ioctl_csc_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_CSC], "CSC");
			ret = cvi_ive_csc(ndev, &val->stSrc, &val->stDst,
						&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CSC]);
		} break;
		case CVI_IVE_IOC_Hist: {
			struct cvi_ive_ioctl_hist_arg *val =
					(struct cvi_ive_ioctl_hist_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_HIST], "Hist");
			ret = cvi_ive_hist(ndev, &val->stSrc, &val->stDst,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_HIST]);
		} break;
		case CVI_IVE_IOC_FilterAndCSC: {
			struct cvi_ive_ioctl_filter_and_csc_arg *val =
					(struct cvi_ive_ioctl_filter_and_csc_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_FILTERCSC], "FilterAndCSC");
			ret = cvi_ive_FilterAndCSC(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_FILTERCSC]);
		} break;
		case CVI_IVE_IOC_Map: {
			struct cvi_ive_ioctl_map_arg *val =
					(struct cvi_ive_ioctl_map_arg *) g_kdata;

			task->buffer = (u16 *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_MAP], "Map");
			ret = cvi_ive_map(ndev, &val->stSrc, task->buffer,
						&val->stDst, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MAP]);
		} break;
		case CVI_IVE_IOC_NCC: {
			struct cvi_ive_ioctl_ncc_arg *val =
					(struct cvi_ive_ioctl_ncc_arg *) g_kdata;

			task->buffer = (IVE_NCC_DST_MEM_S *)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_NCC], "NCC");
			ret = cvi_ive_ncc(ndev, &val->stSrc1, &val->stSrc2,
						task->buffer, val->bInstant, task->dev_id);

			stop_ioctl_time(&g_time_infos[MOD_NCC]);
		} break;
		case CVI_IVE_IOC_Integ: {
			struct cvi_ive_ioctl_integ_arg *val =
					(struct cvi_ive_ioctl_integ_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_INTEG], "Integ");
			ret = cvi_ive_integ(ndev, &val->stSrc, &val->stDst,
						&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_INTEG]);
		} break;
		case CVI_IVE_IOC_LBP: {
			struct cvi_ive_ioctl_lbp_arg *val =
					(struct cvi_ive_ioctl_lbp_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_LBP], "LBP");
			ret = cvi_ive_lbp(ndev, &val->stSrc, &val->stDst,
						&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_LBP]);
		} break;
		case CVI_IVE_IOC_Thresh_S16: {
			struct cvi_ive_ioctl_thresh_s16_arg *val =
					(struct cvi_ive_ioctl_thresh_s16_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRS16], "Thresh_S16");
			ret = cvi_ive_thresh_s16(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRS16]);
		} break;
		case CVI_IVE_IOC_Thresh_U16: {
			struct cvi_ive_ioctl_thres_su16_arg *val =
					(struct cvi_ive_ioctl_thres_su16_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_THRU16], "Thresh_U16");
			ret = cvi_ive_thresh_u16(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_THRU16]);
		} break;
		case CVI_IVE_IOC_16BitTo8Bit: {
			struct cvi_ive_ioctl_16bit_to_8bit_arg *val =
					(struct cvi_ive_ioctl_16bit_to_8bit_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_16To8], "16BitTo8Bit");
			ret = cvi_ive_16bit_to_8bit(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_16To8]);
		} break;
		case CVI_IVE_IOC_OrdStatFilter: {
			struct cvi_ive_ioctl_ord_stat_filter_arg *val =
					(struct cvi_ive_ioctl_ord_stat_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ORDSTAFTR], "OrdStatFilter");
			ret = cvi_ive_ord_stat_filter(ndev, &val->stSrc,
							&val->stDst, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ORDSTAFTR]);
		} break;
		case CVI_IVE_IOC_CannyHysEdge: {
			struct cvi_ive_ioctl_canny_hys_edge_arg *val =
					(struct cvi_ive_ioctl_canny_hys_edge_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_CANNY], "CannyHysEdge");
			ret = cvi_ive_canny_hys_edge(ndev, &val->stSrc, &val->stDst,
							&val->stStack, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CANNY]);
		} break;
		case CVI_IVE_IOC_NormGrad: {
			struct cvi_ive_ioctl_norm_grad_arg *val =
					(struct cvi_ive_ioctl_norm_grad_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_NORMG], "NormGrad");
			ret = cvi_ive_norm_grad(ndev, &val->stSrc, &val->stDstH,
							&val->stDstV, &val->stDstHV,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_NORMG]);
		} break;
		case CVI_IVE_IOC_GradFg: {
			struct cvi_ive_ioctl_grad_fg_arg *val =
					(struct cvi_ive_ioctl_grad_fg_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_GRADFG], "GradFg");
			ret = cvi_ive_grad_fg(ndev, &val->stBgDiffFg,
							&val->stCurGrad, &val->stBgGrad,
							&val->stGradFg, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_GRADFG]);
		} break;
		case CVI_IVE_IOC_SAD: {
			struct cvi_ive_ioctl_sad_arg *val =
					(struct cvi_ive_ioctl_sad_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_SAD], "SAD");
			ret = cvi_ive_sad(ndev, &val->stSrc1, &val->stSrc2,
						&val->stSad, &val->stThr, &val->stCtrl,
						val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_SAD]);
		} break;
		case CVI_IVE_IOC_Resize: {
			struct cvi_ive_ioctl_resize_arg *val =
					(struct cvi_ive_ioctl_resize_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_RESIZE], "Resize");
			ret = cvi_ive_resize(ndev, &val->stSrc, &val->stDst, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_RESIZE]);
		} break;
		case CVI_IVE_IOC_CCL: {
			struct cvi_ive_ioctl_ccl_arg *val =
					(struct cvi_ive_ioctl_ccl_arg *) g_kdata;

			val->stBlob.u64VirAddr = (uintptr_t)task->buffer;
			start_ioctl_time(&g_time_infos[MOD_NORMG], "CCL");
			ret = cvi_ive_ccl(ndev, &val->stSrcDst,
							&val->stBlob, &val->stCclCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_NORMG]);
		} break;
		case CVI_IVE_IOC_imgInToOdma: {
			struct cvi_ive_ioctl_filter_arg *val =
					(struct cvi_ive_ioctl_filter_arg *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_BYP], "imgInToOdma");
			ret = cvi_ive_imgIn_to_odma(ndev, &val->stSrc, &val->stDst,
							&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_BYP]);
		} break;
		case CVI_IVE_IOC_rgbPToYuvToErodeToDilate: {
			struct cvi_ive_ioctl_rgbPToYuvToErodeToDilate *val =
					(struct cvi_ive_ioctl_rgbPToYuvToErodeToDilate *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_ED],
					"rgbPToYuvToErodeToDilate");
			ret = cvi_ive_rgbp_to_yuv_to_erode_to_dilate(
				ndev, &val->stSrc, &val->stDst1, &val->stDst2,
				&val->stCtrl, val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_ED]);
		} break;
		case CVI_IVE_IOC_STCandiCorner: {
			struct cvi_ive_ioctl_stcandicorner *val =
					(struct cvi_ive_ioctl_stcandicorner *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_STCANDI], "STCandiCorner");
			start_ioctl_time(&g_time_infos[MOD_STBOX], "STBox");
			ret = cvi_ive_stcandi_corner(ndev, &val->stSrc,
							&val->stDst, &val->stCtrl,
							val->bInstant, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_STCANDI]);
			stop_ioctl_time(&g_time_infos[MOD_STBOX]);
		} break;
		case CVI_IVE_IOC_MD: {
			struct cvi_ive_ioctl_md *val = (struct cvi_ive_ioctl_md *) g_kdata;

			start_ioctl_time(&g_time_infos[MOD_MD], "FrameDiffDetect");
			ret = cvi_ive_frame_diff_motion(ndev, &val->stSrc1,
								&val->stSrc2, &val->stDst,
								&val->stCtrl,
								val->bInstant,
								task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_MD]);
		} break;
		case CVI_IVE_IOC_CMDQ: {
			start_ioctl_time(&g_time_infos[MOD_CMDQ], "CmdQ");
			ret = cvi_ive_cmdq(ndev, task->dev_id);
			stop_ioctl_time(&g_time_infos[MOD_CMDQ]);
		} break;
		default:
			atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
			ive_task_done(ndev, task);
			CVI_TRACE_IVE(CVI_DBG_ERR, "invalid ioctl cmd[%d]\n", task->task_type);
		}
		if (ret) {
			atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
			ive_task_done(ndev, task);
			dev_err(ndev->dev,
				"[IVE] ioctl _IOC_NR(%d) fail\n", _IOC_NR(task->task_type));
		}
	// }
	ive_task_done(ndev, task);
	atomic_set(&ndev->core[task->dev_id].dev_state, IVE_CORE_STATE_END);
}

static int ive_try_submit_hw(struct cvi_ive_device *ndev, struct ive_task *task)
{
	int i, top_id, ret = -1;
	enum ive_core_state state;

	for (i = 0; i < IVE_DEV_MAX; i++) {
		state = atomic_read(&ndev->core[i].dev_state);

		if (state == IVE_CORE_STATE_END) {
			top_id = i;

			ive_submit_hw(ndev, top_id, task);
			ret = 0;
			break;
		}
	}

	if (ret)
		CVI_TRACE_IVE(CVI_DBG_NOTICE, "ive submit hw fail, hw busy\n");
	return ret;
}

static bool ive_have_idle_core(struct cvi_ive_device *ndev)
{
	u8 coreid;
	enum ive_core_state state;

	for (coreid = 0; coreid < IVE_DEV_MAX; coreid++) {
		state = atomic_read(&ndev->core[coreid].dev_state);
		if (state == IVE_CORE_STATE_END)
			return true;
	}
	return false;
}

static void ive_try_commit_task(struct cvi_ive_device *ndev, struct ive_task *task)
{

	if (!ndev || !task) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "ndev or task is nullptr\n");
		return;
	}

	if (atomic_read(&task->state) != IVE_TASK_STATE_RUNNING) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "task is working, try commit task failed\n");
		return;
	}

	if (!ive_have_idle_core(ndev)) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "no idle core\n");
		return;
	}

	ive_try_submit_hw(ndev, task);

	return;

}
static void ive_clr_evt_kth(void *data)
{
	struct cvi_ive_device *ndev = (struct cvi_ive_device *)data;
	unsigned long flags;
	enum ive_wait_evt evt;

	if (!ndev) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "ive device isn't created yet\n");
		return;
	}

	spin_lock_irqsave(&ndev->close_lock, flags);
	evt = ndev->evt;
	ndev->evt &= ~evt;
	CVI_TRACE_IVE(CVI_DBG_DEBUG, "old evt[%d], new evt[%d]", evt, ndev->evt);
	spin_unlock_irqrestore(&ndev->close_lock, flags);
}

static int ive_event_handler_th(void *data)
{
	struct cvi_ive_device *ndev = (struct cvi_ive_device *)data;
	struct ive_task *task, *task_tmp;
	unsigned long flags;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(IVE_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(IVE_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	u8 idle_coreid;
	bool have_idle_task = false;

	if (!ndev) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "ive device isn't created yet\n");
		return -1;
	}

	while (!kthread_should_stop()) {
		if (!ndev)
			break;

		ret = wait_event_interruptible_timeout(ndev->wait,
			ndev->evt || kthread_should_stop(), timeout);

		if (ret < 0 || kthread_should_stop())
			break;

		idle_coreid = ive_get_idle_coreid(ndev);

		if (idle_coreid >= IVE_DEV_MAX) {
			CVI_TRACE_IVE(CVI_DBG_ERR, "ive device hw busy, no idle core\n");
			goto continue_th;
		}

		if (list_empty(&ndev->tsk_list)) {
			CVI_TRACE_IVE(CVI_DBG_DEBUG, "task list is empty\n");
			goto continue_th;
		}

		spin_lock_irqsave(&ndev->close_lock, flags);
		list_for_each_entry_safe(task, task_tmp, &ndev->tsk_list, node) {
			if (task->dev_id == -1) {
				CVI_TRACE_IVE(CVI_DBG_DEBUG, "got idle task[%p]", task);
				have_idle_task = true;
				break;
			}
		}

		if (!have_idle_task) {
			CVI_TRACE_IVE(CVI_DBG_DEBUG, "no idle task ,task[%p] task_tmp[%p] dev_id[%d]"
				, task, task_tmp, task_tmp->dev_id);
			spin_unlock_irqrestore(&ndev->close_lock, flags);
			goto continue_th;
		}
		atomic_set(&task->state, IVE_TASK_STATE_RUNNING);
		list_del(&task->node);
		CVI_TRACE_IVE(CVI_DBG_DEBUG, "del list node[%p]", task);
		spin_unlock_irqrestore(&ndev->close_lock, flags);

		ive_try_commit_task(ndev, task);

continue_th:
		ive_clr_evt_kth(ndev);

		timeout = list_empty(&ndev->tsk_list) ? idle_timeout : eof_timeout;

	}

	return 0;
}

static int cvi_ive_sw_init(struct cvi_ive_device *ndev)
{
	s32 ret = CVI_SUCCESS;
	struct sched_param task;

	if (ndev == NULL) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "drv point is NULL\n");
		return CVI_FAILURE;
	}

	INIT_LIST_HEAD(&ndev->tsk_list);
	init_waitqueue_head(&ndev->wait);
	ndev->evt = IVE_EVENT_BUSY_OR_NOT_STAT;
	ndev->work_thread = kthread_run(ive_event_handler_th, (void *)ndev, "ive_event_handler_th");
	if (IS_ERR(ndev->work_thread)) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "create ive thread failed\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	task.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(ndev->work_thread, SCHED_FIFO, &task);
	if (ret)
		CVI_TRACE_IVE(CVI_DBG_WARN, "ive thread priority update failed: %d\n", ret);


	return ret;
}

static void cvi_ive_sw_deinit(struct cvi_ive_device *ndev)
{
	if (ndev == NULL) {
		return;
	}

	if (!IS_ERR(ndev->work_thread)) {
		if (kthread_stop(ndev->work_thread))
			CVI_TRACE_IVE(CVI_DBG_ERR, "stop ive work_thread failed\n");
	}

	list_del_init(&ndev->tsk_list);
}
static int instance_init(struct platform_device *pdev)
{
	s32 ret = 0;
	struct cvi_ive_device *ndev;

	ndev = dev_get_drvdata(&pdev->dev);
	if (!ndev) {
		CVI_TRACE_IVE(CVI_DBG_ERR, "ive get driver data failed!\n");
		return CVI_FAILURE;
	}

	if (cvi_ive_sw_init(ndev)) {
		pr_err("ive software init failed\n");
		goto err_sw_init;
	}

	return ret;

err_sw_init:
	cvi_ive_sw_deinit(ndev);

	return ret;
}

static int cvi_ive_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_ive_device *ndev;
	struct resource *res[IVE_DEV_MAX];
	int i,ret;

	CVI_TRACE_IVE(CVI_DBG_INFO, "ive probe statrt\n");
	// Alloc a zero cvi_ive_device struct, and it will auto free when remod
	ndev = devm_kzalloc(&pdev->dev, sizeof(struct cvi_ive_device),
			    GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	for (i = 0; i < ARRAY_SIZE(ndev->ive_base); ++i) {
		res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (unlikely(res[i] == NULL)) {
			dev_err(&pdev->dev, "invalid ive resource\n");
			return -EINVAL;
		}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ndev->ive_base[i] = devm_ioremap(&pdev->dev, res[i]->start,
					res[i]->end - res[i]->start);
				//ndev->ive_base[i] = devm_ioremap_resource(dev, res[i]);
#else
		ndev->ive_base[i] = devm_ioremap_nocache(&pdev->dev, res[i]->start,
								res[i]->end - res[i]->start);
#endif

		//if (IS_ERR(ndev->ive_base[i]))
			//return PTR_ERR(ndev->ive_base[i]);

		if (assign_ive_block_addr(ndev->ive_base[i], i))
			return PTR_ERR(ndev->ive_base[i]);

		ndev->ive_irq[i] = platform_get_irq_byname(pdev, ive_irq_name[i]);
		if (ndev->ive_irq[i] <= 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n", ive_irq_name[i]);
			return -EBUSY;
		}
		spin_lock_init(&ndev->core[i].dev_lock);
		init_completion(&ndev->core[i].frame_done);
		init_completion(&ndev->core[i].op_done);
		atomic_set(&ndev->core[i].dev_state, IVE_CORE_STATE_END);
#if 1
		ret = devm_request_irq(&pdev->dev, ndev->ive_irq[i], cvi_ive_irq_handler,
						IRQF_TRIGGER_NONE, ive_irq_name[i], ndev);
#else
		ret = devm_request_threaded_irq(&pdev->dev, irq_num[i], cvi_ive_irq_handler, ive_irq_finish_thread_func,
						IRQF_TRIGGER_NONE, ive_irq_name[i], ndev)
#endif
		if (ret) {
			dev_err(&pdev->dev,
				"Unable to request interrupt for device (err=%d).\n",
				ret);
			return -ENXIO;
		}

		ndev->core[i].irq_num =  ndev->ive_irq[i];

	}
	ndev->use_count = 0;
	spin_lock_init(&ndev->close_lock);
	init_completion(&ndev->frame_done);
	init_completion(&ndev->op_done);
	//ret = cvi_ive_register_misc(ndev);
	//if (ret < 0) {
	//	pr_err("[IVE] register misc error\n");
	//	return ret;
	//}
	// g_kdata = vmalloc(512);
	g_time_infos = devm_kzalloc(&pdev->dev,
				  MOD_ALL * sizeof(struct ive_profiling_info),
				  GFP_KERNEL);

	ret = cvi_ive_register_cdev(ndev);
	if (ret < 0) {
		pr_err("[IVE] register chrdev error\n");
		return ret;
	}

	// Create drvdata(global variables)
	platform_set_drvdata(pdev, ndev);
	// Create ive proc descript
	// ndev->proc_dir = proc_mkdir("soph", NULL);
	if (proc_create_data(CVI_IVE_PROC_NAME, 0644, NULL,
			     &ive_proc_ops, ndev) == NULL)
		pr_err("[IVE] ive hw_profiling proc creation failed\n");
	g_enable_usage_profiling = 1;

	//clk init
	for (i = 0; i < IVE_DEV_MAX; i++) {
		ndev->core[i].clk_src = devm_clk_get(dev, ive_clk_name[i][0]);
		if (IS_ERR(ndev->core[i].clk_src)) {
			pr_err("Cannot get vi_sys3 clk for ive%d\n", i);
			ndev->core[i].clk_src = NULL;
		}

		ndev->core[i].clk_ive = devm_clk_get(dev, ive_clk_name[i][1]);
		if (IS_ERR(ndev->core[i].clk_ive)) {
			pr_err("Cannot get ive clk for ive%d\n", i);
			ndev->core[i].clk_ive = NULL;
		}
	}

	instance_init(pdev);

	// stcandicorner_workaround(ndev);
	return 0;
}

static int cvi_ive_remove(struct platform_device *pdev)
{
	// Get drvdata(global variables)
	struct cvi_ive_device *ndev = platform_get_drvdata(pdev);

	cvi_ive_sw_deinit(ndev);

	device_destroy(class_id, cdev_id);

	cdev_del(&ndev->cdev);

	unregister_chrdev_region(cdev_id, 1);

	//misc_deregister(&ndev->miscdev);

	class_destroy(class_id);

	platform_set_drvdata(pdev, NULL);


	// remove ive proc
	remove_proc_entry(CVI_IVE_PROC_NAME, NULL);
	return 0;
}

#ifdef CONFIG_PM
static int cvi_ive_suspend(struct device *dev)
{
	//[TODO]
	pr_debug("[IVE] ive_suspend\n");
	return 0;
}

static int cvi_ive_resume(struct device *dev)
{
	//[TODO]
	pr_debug("[IVE] ive_resume\n");
	return 0;
}
#endif
#ifdef CONFIG_PM
static SIMPLE_DEV_PM_OPS(cvi_ive_pm_ops, cvi_ive_suspend, cvi_ive_resume);
#endif
static const struct of_device_id cvi_ive_match[] = {
	{ .compatible = "cvitek,ive" },
	{},
};
MODULE_DEVICE_TABLE(of, cvi_ive_match);

static struct platform_driver cvi_ive_driver = {
	.probe = cvi_ive_probe,
	.remove = cvi_ive_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = CVI_IVE_CDEV_NAME,
#ifdef CONFIG_PM
			.pm = &cvi_ive_pm_ops,
#endif
			.of_match_table = cvi_ive_match,  // add ive to dtsi
		},
};

module_platform_driver(cvi_ive_driver);

MODULE_AUTHOR("Ken Lin<ken.lin@cvitek.com>");
MODULE_DESCRIPTION("Cvitek SoC IVE driver");
MODULE_LICENSE("GPL");
