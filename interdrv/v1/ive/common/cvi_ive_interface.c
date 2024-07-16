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

#include <asm/cacheflush.h>
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
#include <linux/sched/signal.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "cvi_ive_interface.h"
#include "cvi_ive_platform.h"

#define CVI_IVE_CDEV_NAME "cvi-ive"
#define CVI_IVE_CLASS_NAME "cvi-ive"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static uint32_t get_duration_us(const struct timespec64 *start,
				const struct timespec64 *end)
#else
static uint32_t get_duration_us(const struct timespec *start,
				const struct timespec *end)
#endif
{
	uint64_t dividend = end->tv_nsec - start->tv_nsec;
	uint32_t event_duration_us;

	do_div(dividend, 1000);
	event_duration_us = (uint32_t)dividend;
	event_duration_us += (end->tv_sec - start->tv_sec) * 1000000;

	return event_duration_us;
}

struct ive_profiling_info {
	char op_name[32];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 time_start;
	struct timespec64 time_end;
#else
	struct timespec time_start;
	struct timespec time_end;
#endif
	int32_t time_diff_us;
};

struct class *class_id;
static dev_t cdev_id;

static int gtime_count;
static uint32_t enable_usage_profiling;
static struct ive_profiling_info time_infos[20];

// proc_operations function
static int ive_proc_open(struct inode *inode, struct file *file);
static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos);
// file_operations function
static int cvi_ive_open(struct inode *inode, struct file *filp);
static int cvi_ive_close(struct inode *inode, struct file *filp);
static long cvi_ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
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
	.unlocked_ioctl = cvi_ive_ioctl,
	.compat_ioctl = cvi_ive_ioctl,
};

static void start_time(struct ive_profiling_info *pinfo, char *name)
{
	if (enable_usage_profiling) {
		strcpy(pinfo->op_name, name);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ktime_get_real_ts64(&pinfo->time_start);
#else
		getnstimeofday(&pinfo->time_start);
#endif
	}
}

static void stop_time(struct ive_profiling_info *pinfo)
{
	if (enable_usage_profiling) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		ktime_get_real_ts64(&pinfo->time_end);
#else
		getnstimeofday(&pinfo->time_end);
#endif
		pinfo->time_diff_us =
			get_duration_us(&pinfo->time_start, &pinfo->time_end);
	}
}

static irqreturn_t cvi_ive_irq_handler(int irq, void *data)
{
	struct cvi_ive_device *ndev = data;
	irqreturn_t ret;

	spin_lock(&ndev->close_lock);
	pr_info("[IVE] ive use_count %d\n", ndev->use_count);
	if (ndev->use_count == 0) {
		spin_unlock(&ndev->close_lock);
		return IRQ_HANDLED;
	}

	ret = platform_ive_irq(ndev);

	spin_unlock(&ndev->close_lock);

	return ret;
}

static int ive_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
	int index = gtime_count % 20;

	if (enable_usage_profiling) {
		seq_puts(m, "[IVE] ive profiling is running\n");
		for (i = index; i < 20 - index; i++) {
			if (strlen(time_infos[i].op_name) > 0) {
				seq_printf(
					m,
					"[IVE] %s, stime = %ld, interval = %d us\n",
					time_infos[i].op_name,
					time_infos[i].time_start.tv_sec,
					time_infos[i].time_diff_us);
			}
		}
		for (i = 0; i < index; i++) {
			if (strlen(time_infos[i].op_name) > 0) {
				seq_printf(
					m,
					"[IVE] %s, stime = %ld, interval = %d us\n",
					time_infos[i].op_name,
					time_infos[i].time_start.tv_sec,
					time_infos[i].time_diff_us);
			}
		}
	} else {
		seq_puts(m, "[IVE] ive profiling is disabled\n");
	}
	return 0;
}

static ssize_t ive_proc_write(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	uint32_t input_param = 0, i = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &input_param)) {
		pr_err("[IVE] input parameter incorrect\n");
		return count;
	}

	// reset related info
	enable_usage_profiling = input_param;
	for (i = 0; i < 20; i++)
		memset(&time_infos[i], 0, sizeof(struct ive_profiling_info));
	pr_debug("[IVE] enable_usage_profiling = %d\n", enable_usage_profiling);
	if (enable_usage_profiling) {
		gtime_count = 0;
		pr_err("[IVE] ive usage profiline is started\n");
	} else {
		pr_err("[IVE] ive usage profiling is ended\n");
	}

	return count;
}

static int ive_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ive_proc_show, PDE_DATA(inode));
}

static long cvi_ive_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct cvi_ive_device *ndev = filp->private_data;
	CVI_S32 ret = -1;

	pr_info("[IVE] %s\n", __func__);
	switch (cmd) {
	case CVI_IVE_IOC_TEST: {
		int index = (gtime_count++) % 20;
		struct IVE_KEN_TEST_S val;

		if (copy_from_user(&val, (struct IVE_KEN_TEST_S *)arg,
				   sizeof(struct IVE_KEN_TEST_S)) == 0) {
			start_time(&time_infos[index], "Test");
			ret = cvi_ive_test(ndev, val.pAddr, &val.u16Width,
					   &val.u16Height);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_test fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_add ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user((struct IVE_KEN_TEST_S __user *)arg,
					   &val, sizeof(struct IVE_KEN_TEST_S));
		}
	} break;
	case CVI_IVE_IOC_Add: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_add_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_add_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_add_arg)) == 0) {
			start_time(&time_infos[index], "Add");
			ret = cvi_ive_Add(ndev, val.pstSrc1, val.pstSrc2,
					  val.pstDst, val.pstAddCtrl,
					  val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Add fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_add ok ...\n");
			stop_time(&time_infos[index]);

			ret = copy_to_user(
				(struct cvi_ive_ioctl_add_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_add_arg));
		}
	} break;
	case CVI_IVE_IOC_DMA:
		// ret = CVI_MPI_IVE_DMA(ndev, arg);
		break;
	case CVI_IVE_IOC_Filter:
		// ret = cvi_ive_Filter(ndev, arg);
		break;
	case CVI_IVE_IOC_CSC:
		// ret = cvi_ive_CSC(ndev, arg);
		break;
	case CVI_IVE_IOC_FilterAndCSC:
		// ret = cvi_ive_FilterAndCSC(ndev, arg);
		break;
	case CVI_IVE_IOC_Sobel:
		// ret = cvi_ive_Sobel(ndev, arg);
		break;
	case CVI_IVE_IOC_MagAndAng:
		// ret = cvi_ive_MagAndAng(ndev, arg);
		break;
	case CVI_IVE_IOC_MatchBgModel: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_bgmodel_arg val;

		if (copy_from_user(
			    &val, (struct cvi_ive_ioctl_bgmodel_arg *)arg,
			    sizeof(struct cvi_ive_ioctl_bgmodel_arg)) == 0) {
			start_time(&time_infos[index], "Dilate");
			ret = cvi_ive_MatchBgModel(
				ndev, val.pstCurImg, val.pstBgModel,
				val.pstFgFlag, val.pstBgDiffFg,
				val.pstFrmDiffFg, val.pstStatData, val.pstCtrl,
				val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_MatchBgModel fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_MatchBgModel ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_bgmodel_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_bgmodel_arg));
		}
	} break;
	case CVI_IVE_IOC_Dilate: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_dilate_erode_arg val;

		if (copy_from_user(
			    &val, (struct cvi_ive_ioctl_dilate_erode_arg *)arg,
			    sizeof(struct cvi_ive_ioctl_dilate_erode_arg)) ==
		    0) {
			start_time(&time_infos[index], "Dilate");
			ret = cvi_ive_Dilate(ndev, val.pstSrc, val.pstDst,
					     (IVE_DILATE_CTRL_S *)val.pstCtrl,
					     val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Dilate fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_Dilate ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_dilate_erode_arg __user *)
					arg,
				&val,
				sizeof(struct cvi_ive_ioctl_dilate_erode_arg));
		}
	} break;
	case CVI_IVE_IOC_Erode: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_dilate_erode_arg val;

		if (copy_from_user(
			    &val, (struct cvi_ive_ioctl_dilate_erode_arg *)arg,
			    sizeof(struct cvi_ive_ioctl_dilate_erode_arg)) ==
		    0) {
			start_time(&time_infos[index], "Erode");
			ret = cvi_ive_Erode(ndev, val.pstSrc, val.pstDst,
					    (IVE_ERODE_CTRL_S *)val.pstCtrl,
					    val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Erode fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_Erode ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_dilate_erode_arg __user *)
					arg,
				&val,
				sizeof(struct cvi_ive_ioctl_dilate_erode_arg));
		}
	} break;
	case CVI_IVE_IOC_Thresh: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_thresh_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_thresh_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_thresh_arg)) ==
		    0) {
			start_time(&time_infos[index], "Thresh");
			ret = cvi_ive_Thresh(ndev, val.pstSrc, val.pstDst,
					     val.pstCtrl, val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl CVI_IVE_IOC_Thresh fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl CVI_IVE_IOC_Thresh ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_thresh_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_thresh_arg));
		}
	} break;
	case CVI_IVE_IOC_GMM: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_gmm_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_gmm_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_gmm_arg)) == 0) {
			start_time(&time_infos[index], "GMM");
			ret = cvi_ive_GMM(ndev, val.pstSrc, val.pstFg,
					  val.pstBg, val.pstModel, val.pstCtrl,
					  val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_GMM fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_GMM ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_gmm_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_gmm_arg));
		}
	} break;
	case CVI_IVE_IOC_GMM2: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_gmm2_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_gmm2_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_gmm2_arg)) ==
		    0) {
			start_time(&time_infos[index], "GMM2");
			ret = cvi_ive_GMM2(ndev, val.pstSrc, val.pstFactor,
					   val.pstFg, val.pstBg,
					   val.pstMatchModelInfo, val.pstModel,
					   val.pstCtrl, val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_GMM2 fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_GMM2 ok ...\n");
			stop_time(&time_infos[index]);
			ret = copy_to_user(
				(struct cvi_ive_ioctl_gmm2_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_gmm2_arg));
		}
	} break;
	case CVI_IVE_IOC_And: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_and_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_and_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_and_arg)) == 0) {
			start_time(&time_infos[index], "And");
			ret = cvi_ive_And(ndev, val.pstSrc1, val.pstSrc2,
					  val.pstDst, val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_sub fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_And ok ...\n");
			stop_time(&time_infos[index]);

			ret = copy_to_user(
				(struct cvi_ive_ioctl_and_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_and_arg));
		}
	} break;
	case CVI_IVE_IOC_Sub: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_sub_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_sub_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_sub_arg)) == 0) {
			start_time(&time_infos[index], "Sub");
			ret = cvi_ive_Sub(ndev, val.pstSrc1, val.pstSrc2,
					  val.pstDst, val.pstSubCtrl,
					  val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Sub fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_Sub ok ...\n");
			stop_time(&time_infos[index]);

			ret = copy_to_user(
				(struct cvi_ive_ioctl_sub_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_sub_arg));
		}
	} break;
	case CVI_IVE_IOC_Or: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_or_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_or_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_or_arg)) == 0) {
			start_time(&time_infos[index], "Or");
			ret = cvi_ive_Or(ndev, val.pstSrc1, val.pstSrc2,
					 val.pstDst, val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Or fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_Or ok ...\n");
			stop_time(&time_infos[index]);

			ret = copy_to_user(
				(struct cvi_ive_ioctl_or_arg __user *)arg, &val,
				sizeof(struct cvi_ive_ioctl_or_arg));
		}
	} break;
	//[TODO] more op need to implement
	case CVI_IVE_IOC_Xor: {
		int index = (gtime_count++) % 20;
		struct cvi_ive_ioctl_xor_arg val;

		if (copy_from_user(&val, (struct cvi_ive_ioctl_xor_arg *)arg,
				   sizeof(struct cvi_ive_ioctl_xor_arg)) == 0) {
			start_time(&time_infos[index], "Xor");
			ret = cvi_ive_Xor(ndev, val.pstSrc1, val.pstSrc2,
					  val.pstDst, val.bInstant);
			if (ret) {
				dev_err(ndev->dev,
					"[IVE] ioctl cvi_ive_Xor fail\n");
				return ret;
			}
			pr_info("[IVE] Debug ioctl cvi_ive_Xor ok ...\n");
			stop_time(&time_infos[index]);

			ret = copy_to_user(
				(struct cvi_ive_ioctl_xor_arg __user *)arg,
				&val, sizeof(struct cvi_ive_ioctl_xor_arg));
		}
	} break;
	//[TODO] more op need to implement
	default:
		return -ENOTTY;
	}
	// pr_debug("[IVE] [DIR=%02d, MAGIC=%c, OP=0x%02x] use_count = %d, ret = %ld\n",
	//		 _IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), ndev->use_count, ret);
	return ret;
}

static int cvi_ive_open(struct inode *inode, struct file *filp)
{
	struct cvi_ive_device *ndev =
		container_of(inode->i_cdev, struct cvi_ive_device, cdev);
	unsigned long flags = 0;

	spin_lock_irqsave(&ndev->close_lock, flags);

	if (ndev->use_count == 0) {
		pr_debug("[IVE] first cvi_ive open\n");
	}
	ndev->use_count++;

	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = ndev;
	return 0;
}

static int cvi_ive_close(struct inode *inode, struct file *filp)
{
	unsigned long flags = 0;
	struct cvi_ive_device *ndev =
		container_of(inode->i_cdev, struct cvi_ive_device, cdev);

	spin_lock_irqsave(&ndev->close_lock, flags);
	ndev->use_count--;
	spin_unlock_irqrestore(&ndev->close_lock, flags);

	filp->private_data = NULL;

	return 0;
}

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
	device_create(class_id, ndev->dev, cdev_id, NULL, "%s%d",
		      CVI_IVE_CDEV_NAME, 0);

	return 0;
}

static int cvi_ive_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct cvi_ive_device *ndev;
	struct resource *res;
	int ret;

	pr_info("[IVE] %s\n", __func__);
	dev = &pdev->dev;
	gtime_count = 0;

	// Alloc a zero cvi_ive_device struct, and it will auto free when remod
	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ndev->ive_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(ndev->ive_base))
		return PTR_ERR(ndev->ive_base);

	if (assign_ive_block_addr(ndev))
		return PTR_ERR(ndev->ive_base);

	ndev->ive_irq = platform_get_irq(pdev, 0);
	if (ndev->ive_irq <= 0)
		return -EBUSY;

	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;
	// Get and enable clk
	// ndev->clk = devm_clk_get(&pdev->dev, NULL);
	// if (IS_ERR(ndev->clk))
	// return PTR_ERR(ndev->clk);
	// ret = clk_prepare_enable(ndev->clk);
	// if (ret < 0)
	// return ret;
	// Create character device (cdev)

	ret = devm_request_irq(&pdev->dev, ndev->ive_irq, cvi_ive_irq_handler,
			       IRQF_TRIGGER_NONE, dev_name(&pdev->dev), ndev);
	if (ret) {
		dev_err(&pdev->dev,
			"Unable to request interrupt for device (err=%d).\n",
			ret);
		return -ENXIO;
	}

	init_completion(&ndev->ive_done);

	ret = cvi_ive_register_cdev(ndev);
	if (ret < 0) {
		pr_err("[IVE] register chrdev error\n");
		return ret;
	}
	// Create drvdata(global variables)
	platform_set_drvdata(pdev, ndev);
	// Create ive proc descript
	ndev->proc_dir = proc_mkdir("ive", NULL);
	if (proc_create_data("usage_profiling", 0644, ndev->proc_dir,
			     &ive_proc_ops, ndev) == NULL)
		pr_err("[IVE] ive usage_profiling proc creation failed\n");

	return 0;
}

static int cvi_ive_remove(struct platform_device *pdev)
{
	// Get drvdata(global variables)
	struct cvi_ive_device *ndev = platform_get_drvdata(pdev);

	device_destroy(class_id, cdev_id);

	cdev_del(&ndev->cdev);
	unregister_chrdev_region(cdev_id, 1);

	class_destroy(class_id);

	platform_set_drvdata(pdev, NULL);

	// clk_disable_unprepare(ndev->clk);

	pr_debug("[IVE] ive_remove prdebug()\n");

	// remove ive proc
	proc_remove(ndev->proc_dir);
	return 0;
}

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

static SIMPLE_DEV_PM_OPS(cvi_ive_pm_ops, cvi_ive_suspend, cvi_ive_resume);
// [TODO] # when add ive to dtsi
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
			.name = "cvi-ive",
			.pm = &cvi_ive_pm_ops,
			.of_match_table = cvi_ive_match,  // add ive to dtsi
		},
};

module_platform_driver(cvi_ive_driver);

MODULE_AUTHOR("Ken Lin<ken.lin@cvitek.com>");
MODULE_DESCRIPTION("Cvitek SoC IVE driver");
MODULE_LICENSE("GPL");
