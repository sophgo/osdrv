// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Copyright (C) Sophon Technology Co., Ltd. 2026. All rights reserved.
 *
 * File Name: motionsensor.c
 * Description: motionsensor driver related code
 *
 */
#include <linux/compat.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/iommu.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/sched/signal.h>

#include "motionsensor.h"
#include "motionsensor_cmd.h"
#include "motionsensor_common.h"
#include "msensor_dev.h"

#define __DEVICE_NAME "msensor"

static msensor_dev g_mdev;

static bool g_msensor_init;                 // defaults to false (zero)
static bool g_msensor_start;                // defaults to false (zero)
static msensor_param *g_motionsensor_param; // defaults to NULL (zero)

static int msensor_dev_user_init(unsigned long arg)
{
	int ret = 0;
	u8 *buf_base = NULL;

	if (g_msensor_init) {
		pr_err("[MSENSOR] already init...\n");
		return -1;
	}

	if (copy_from_user(g_motionsensor_param, (msensor_param *)arg, sizeof(msensor_param))) {
		pr_err("[MSENSOR] copy_from_user err...\n");
		return -EFAULT;
	}

	buf_base = (u8 *)phys_to_virt(g_motionsensor_param->buf_attr.phys_addr);

	if (buf_base <= 0) {
		pr_err("[MSENSOR] phys_to_virt err...\n");
		return -1;
	}

	g_motionsensor_param->buf_attr.virt_addr = (u64)buf_base;

	ret = msensor_dev_init(g_motionsensor_param);
	if (ret < 0) {
		pr_err("[MSENSOR] dev init err...\n");
		return ret;
	}

	g_msensor_init = true;

	return 0;
}

static int msensor_dev_user_deinit(unsigned long arg)
{
	if (!g_msensor_init) {
		pr_err("[MSENSOR] not init...\n");
		return -1;
	}

	if (g_msensor_start) {
		pr_err("[MSENSOR] stop first...\n");
		return -1;
	}

	msensor_dev_deinit();

	g_msensor_init = false;

	return 0;
}

static int msensor_dev_user_start(unsigned long arg)
{
	if (!g_msensor_init) {
		pr_err("[MSENSOR] not init...\n");
		return -1;
	}

	if (g_msensor_start) {
		pr_err("[MSENSOR] already start...\n");
		return -1;
	}

	if (msensor_get_triger_mode() == TRIGER_TIMER) {
		pr_info("[MSENSOR] timer triger run...\n");
		msensor_timer_triger_run();
	} else if (msensor_get_triger_mode() == TRIGER_EXTERN_INTERRUPT) {
		pr_info("[MSENSOR] GPIO interrupt triger run...\n");
		msensor_gpio_triger_run();
	} else {
		pr_err("[MSENSOR] not support triger mode: %d\n", msensor_get_triger_mode());
		return -1;
	}

	g_msensor_start = true;

	return 0;
}

static int msensor_dev_user_stop(unsigned long arg)
{
	if (!g_msensor_init) {
		pr_err("[MSENSOR] not init...\n");
		return -1;
	}

	if (!g_msensor_start) {
		pr_err("[MSENSOR] already stop...\n");
		return -1;
	}

	if (msensor_get_triger_mode() == TRIGER_TIMER) {
		pr_info("[MSENSOR] timer triger stop...\n");
		msensor_timer_triger_stop();
	} else if (msensor_get_triger_mode() == TRIGER_EXTERN_INTERRUPT) {
		pr_info("[MSENSOR] GPIO interrupt triger stop...\n");
		msensor_gpio_triger_stop();
	} else {
		pr_err("[MSENSOR] not support triger mode: %d\n", msensor_get_triger_mode());
		return -1;
	}

	g_msensor_start = false;

	return 0;
}

static int msensor_dev_user_get_data(unsigned long arg)
{
	msensor_data_info data_info[2];

	memset(&data_info, 0, sizeof(msensor_data_info) * 2);

	msensor_get_data_info(data_info);

	if (copy_to_user((msensor_data_info *)arg, data_info, 2 * sizeof(msensor_data_info))) {
		pr_err("[MSENSOR] copy_to_user err...\n");
		return -EFAULT;
	}

	return 0;
}

static int msensor_dev_zero_calib(unsigned long arg)
{
	if (!g_msensor_init) {
		pr_err("[MSENSOR] not init...\n");
		return -1;
	}

	if (!g_msensor_start) {
		pr_err("[MSENSOR] not start...\n");
		return -1;
	}

	return msensor_zero_calib(); // only gyro zero calib
}

static const msensor_ioctl_info g_msensor_ioctls[] = {
		{MSENSOR_CMD_INIT, msensor_dev_user_init},
		{MSENSOR_CMD_DEINIT, msensor_dev_user_deinit},
		{MSENSOR_CMD_START, msensor_dev_user_start},
		{MSENSOR_CMD_STOP, msensor_dev_user_stop},
		{MSENSOR_CMD_GET_DATA, msensor_dev_user_get_data},
		{MSENSOR_CMD_ZERO_CALIB, msensor_dev_zero_calib},
};

static int msensor_proc_show(struct seq_file *m, void *v)
{
	char *info = kmalloc(4 * 1024, GFP_KERNEL);

	msensor_get_debug_info(info, 4 * 1024);
	seq_puts(m, info);

	kfree(info);

	return 0;
}

static ssize_t msensor_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	u32 user_input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &user_input_param)) {
		pr_err("\n[MSENSOR] input parameter incorrect\n");
		return count;
	}

	pr_info("\n[MSENSOR] get sel %d\n", user_input_param);

	if (user_input_param >= ARRAY_SIZE(g_msensor_ioctls)) {
		pr_err("[MSENSOR] msensor ioctl cmd 0x%x does not exist!\n", user_input_param);
		return -1;
	}

	if (g_msensor_ioctls[user_input_param].func)
		g_msensor_ioctls[user_input_param].func(0);

	return count;
}

static int msensor_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, msensor_proc_show, PDE_DATA(inode));
}

// #if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops msensor_proc_ops = {
		.proc_open = msensor_proc_open,
		.proc_read = seq_read,
		.proc_write = msensor_proc_write,
		.proc_release = single_release,
};

// #else
// static const struct file_operations msensor_proc_ops = {
//		.owner = THIS_MODULE,
//		.open = msensor_proc_open,
//		.read = seq_read,
//		.write = msensor_proc_write,
//		.release = single_release,
// };
// #endif

static int msensor_open(struct inode *inode, struct file *filp) { return 0; }

static int msensor_release(struct inode *inode, struct file *filp) { return 0; }

static long msensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (_IOC_NR(cmd) >= ARRAY_SIZE(g_msensor_ioctls) || cmd != g_msensor_ioctls[_IOC_NR(cmd)].cmd) {
		pr_err("[MSENSOR] msensor ioctl cmd 0x%x does not exist!\n", cmd);
		return -1;
	}

	if (g_msensor_ioctls[_IOC_NR(cmd)].func)
		return g_msensor_ioctls[_IOC_NR(cmd)].func(arg);

	return -1;
}

static const struct file_operations msensor_fops = {
		.open = msensor_open,
		.release = msensor_release,
		.unlocked_ioctl = msensor_ioctl,
};

static int __init msensor_probe(void)
{
	int ret = 0;

	ret = msensor_module_init();
	if (ret < 0) {
		pr_err("[MSENSOR] init err\n");
		return -1;
	}

	/* 1. register dev number */
	if (g_mdev.major) {
		g_mdev.devid = MKDEV(g_mdev.major, 0);
		register_chrdev_region(g_mdev.devid, 1, __DEVICE_NAME);
	} else {
		ret = alloc_chrdev_region(&g_mdev.devid, 0, 1, __DEVICE_NAME);
		g_mdev.major = MAJOR(g_mdev.devid);
		g_mdev.minor = MINOR(g_mdev.devid);
	}

	if (ret < 0) {
		pr_err("[MSENSOR] chr_dev region err\n");
		return -1;
	}

	/* 2. register chardev */
	g_mdev.cdev.owner = THIS_MODULE;
	cdev_init(&g_mdev.cdev, &msensor_fops);
	ret = cdev_add(&g_mdev.cdev, g_mdev.devid, 1);
	if (ret < 0) {
		pr_err("[MSENSOR] cdev_add err\n");
		return -1;
	}

	/* 3. auto create device node */
	g_mdev.class = class_create(THIS_MODULE, __DEVICE_NAME);
	if (IS_ERR(g_mdev.class)) {
		pr_err("[MSENSOR] create class failed\n");
		return PTR_ERR(g_mdev.class);
	}
	g_mdev.device = device_create(g_mdev.class, NULL, g_mdev.devid, NULL, __DEVICE_NAME);
	if (IS_ERR(g_mdev.device))
		return PTR_ERR(g_mdev.device);

	// Create gyro proc descript
	g_mdev.proc_dir = proc_mkdir("msensor", NULL);
	if (!proc_create_data("status", 0644, g_mdev.proc_dir, &msensor_proc_ops, &g_mdev))
		pr_err("[MSENSOR] msensor ioctl proc creation failed\n");

	pr_info("[MSENSOR] msensor registered, major number: %d\n", g_mdev.major);

	g_motionsensor_param = kmalloc(sizeof(msensor_param), GFP_KERNEL);
	if (!g_motionsensor_param)
		return -ENOMEM;

	memset(g_motionsensor_param, 0, sizeof(msensor_param));

	return 0;
}

static void __exit msensor_remove(void)
{
	if (g_msensor_start)
		msensor_dev_user_stop(0);

	if (g_msensor_init)
		msensor_dev_user_deinit(0);

	kfree(g_motionsensor_param);
	g_motionsensor_param = NULL;

	msensor_module_deint();

	/* 1. unregister */
	cdev_del(&g_mdev.cdev);

	unregister_chrdev_region(g_mdev.devid, 1);

	/* 2. destroy device */
	device_destroy(g_mdev.class, g_mdev.devid);

	/* 3. dectroy class */
	class_destroy(g_mdev.class);

	// remove gyro proc
	proc_remove(g_mdev.proc_dir);

	pr_info("[MSENSOR] msensor unregistered...\n");
}

module_init(msensor_probe);
module_exit(msensor_remove);

MODULE_DESCRIPTION("motionsensor driver");
MODULE_AUTHOR("mason.zou@sophgo.com");
MODULE_LICENSE("GPL");
