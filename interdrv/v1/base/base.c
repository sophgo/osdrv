/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: base.c
 * Description: vip kernel space base driver code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <linux/suspend.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/ctype.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#include <linux/sched/signal.h>
#endif
#include <linux/security.h>
#include <linux/cred.h>
#include "tee_cv_private.h"
#include "base.h"
#include "cvi_vb_proc.h"
#include "cvi_log_proc.h"
#include "cvi_sys_proc.h"
#include "mw/mw_base.h"
#include <linux/semaphore.h>

#define IS_CHIP_CV183X(x) (((x) == E_CHIPID_CV1829) || ((x) == E_CHIPID_CV1832) \
						|| ((x) == E_CHIPID_CV1835) || ((x) == E_CHIPID_CV1838))

#define IS_CHIP_CV182X(x) (((x) == E_CHIPID_CV1820) || ((x) == E_CHIPID_CV1821) \
						|| ((x) == E_CHIPID_CV1822) || ((x) == E_CHIPID_CV1823) \
						|| ((x) == E_CHIPID_CV1825) || ((x) == E_CHIPID_CV1826))

#define BASE_CLASS_NAME "cvi-base"
#define BASE_DEV_NAME "cvi-base"

/* register bank */
#define TOP_BASE 0x03000000
#define TOP_REG_BANK_SIZE 0x10000
#define GP_REG0_OFFSET 0x80
#define GP_REG3_OFFSET 0x8C
#define GP_REG_CHIP_ID_MASK 0xFFFF

#define CV183X_RTC_BASE 0x03005000
#define CV182X_RTC_BASE 0x05026000
#define RTC_REG_BANK_SIZE 0x140
#define RTC_ST_ON_REASON 0xF8

struct base_device {
	struct device *dev;
	struct miscdevice miscdev;
	void *shared_mem;
	u16 mmap_count;
	struct list_head ps_list;
	spinlock_t lock;
	enum base_state_e state;
	struct completion done;
	struct notifier_block notifier;
	u8 sig_hook;
};

struct base_state {
	struct list_head list;      /* state list */
	struct base_device *ndev;
	struct file *file;
	unsigned int state_signr;
	struct pid *state_pid;
	const struct cred *cred;
	void __user *state_context;
	u32 secid;
};

static void __iomem *top_base;
static void __iomem *rtc_base;
static struct proc_dir_entry *proc_dir;
struct class *pbase_class;

#ifdef CONFIG_ARCH_CV182X_FPGA
#define FPGA_EARLY_PORTING_CHIP_ID E_CHIPID_CV1822
#endif

unsigned int vb_max_pools = 512;
module_param(vb_max_pools, uint, 0644);
unsigned int vb_pool_max_blk = 128;
module_param(vb_pool_max_blk, uint, 0644);

static int __init base_init(void);
static void __exit base_exit(void);

static ssize_t base_efuse_shadow_show(struct class *class,
				      struct class_attribute *attr, char *buf)
{
	int ret;
#ifndef __riscv
	ret = tee_cv_efuse_read(0, PAGE_SIZE, buf);
#endif
	return ret;
}

static ssize_t base_efuse_shadow_store(struct class *class,
				       struct class_attribute *attr,
				       const char *buf, size_t count)
{
	unsigned long addr;
	uint32_t value = 0xDEAFBEEF;
	int ret;

	ret = kstrtoul(buf, 0, &addr);
	if (ret < 0) {
		pr_err("efuse_read: ret=%d\n", ret);
		return ret;
	}
#ifndef __riscv
	ret = tee_cv_efuse_read(addr, 4, &value);
	pr_info("efuse_read: 0x%04lx=0x%08x ret=%d\n", addr, value, ret);
#endif
	return count;
}

static ssize_t base_efuse_prog_show(struct class *class,
				    struct class_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", "PROG_SHOW");
}

static ssize_t base_efuse_prog_store(struct class *class,
				     struct class_attribute *attr,
				     const char *buf, size_t count)
{
	int err = 0;
	uint32_t addr = 0, value = 0;

	if (sscanf(buf, "0x%x=0x%x", &addr, &value) != 2)
		return -ENOMEM;

	pr_info("addr=%x value=%x\n", addr, value);
#ifndef __riscv
	err = tee_cv_efuse_write(addr, value);
#endif
	if (err < 0)
		return err;

	return count;
}

static ssize_t base_uid_show(struct class *class,
			     struct class_attribute *attr, char *buf)
{
	uint32_t uid_3 = 0xDEAFBEEF;
	uint32_t uid_4 = 0xDEAFBEEF;
#ifndef __riscv
	tee_cv_efuse_read(0x0c, 4, &uid_3);
	tee_cv_efuse_read(0x10, 4, &uid_4);
#endif
	return scnprintf(buf, PAGE_SIZE, "UID: %08x_%08x\n", uid_3, uid_4);
}

static ssize_t base_rosc_show(struct class *class,
			      struct class_attribute *attr, char *buf)
{
	int count = 0;
	void __iomem *rosc_base;

	rosc_base = ioremap(0x030D0000, 0x2028);

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0010]=0x%08X\n", ioread32(rosc_base + 0x0010));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0024]=0x%08X\n", ioread32(rosc_base + 0x0024));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D0028]=0x%08X\n", ioread32(rosc_base + 0x0028));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2010]=0x%08X\n", ioread32(rosc_base + 0x2010));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2024]=0x%08X\n", ioread32(rosc_base + 0x2024));
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "ROSC[0x030D2028]=0x%08X\n", ioread32(rosc_base + 0x2028));

	iounmap(rosc_base);

	return count;
}

static ssize_t base_rosc_store(struct class *class,
			       struct class_attribute *attr,
			       const char *buf, size_t count)
{
	uint32_t chip_id;
	void __iomem *rosc_base;

	if (sysfs_streq(buf, "set")) {
		chip_id = cvi_base_read_chip_id();
		rosc_base = ioremap(0x030D0000, 0x2028);
		if (IS_CHIP_CV182X(chip_id)) {
			iowrite32(0x00000000, rosc_base + 0x0004);
			iowrite32(0x00000040, rosc_base + 0x000C);
			iowrite32(0x01020009, rosc_base + 0x0014);
			iowrite32(0x00FFFFFF, rosc_base + 0x001C);
			iowrite32(0x00000001, rosc_base + 0x0004);

			iowrite32(0x00000000, rosc_base + 0x2004);
			iowrite32(0x00000040, rosc_base + 0x200C);
			iowrite32(0x01020009, rosc_base + 0x2014);
			iowrite32(0x00FFFFFF, rosc_base + 0x201C);
			iowrite32(0x00000001, rosc_base + 0x2004);
		} else {
			iowrite32(0x01020009, rosc_base + 0x0014);
			iowrite32(0x00FFFFFF, rosc_base + 0x001C);
			iowrite32(0x00000000, rosc_base + 0x0004);
			iowrite32(0x00000001, rosc_base + 0x0004);

			iowrite32(0x01020009, rosc_base + 0x2014);
			iowrite32(0x00FFFFFF, rosc_base + 0x201C);
			iowrite32(0x00000000, rosc_base + 0x2004);
			iowrite32(0x00000001, rosc_base + 0x2004);
		}
		iounmap(rosc_base);
	}

	return count;
}

CLASS_ATTR_RW(base_efuse_shadow);
CLASS_ATTR_RW(base_efuse_prog);
CLASS_ATTR_RO(base_uid);
CLASS_ATTR_RW(base_rosc);

unsigned int cvi_base_read_chip_id(void)
{
#ifndef FPGA_EARLY_PORTING_CHIP_ID
	unsigned int chip_id = ioread32(top_base + GP_REG3_OFFSET) & GP_REG_CHIP_ID_MASK;

	pr_debug("chip_id=0x%x\n", chip_id);

	switch (chip_id) {
	case 0x1821:
		return E_CHIPID_CV1821;
	case 0x1822:
		return E_CHIPID_CV1822;
	case 0x1826:
		return E_CHIPID_CV1826;
	case 0x1832:
		return E_CHIPID_CV1832;
	case 0x1838:
		return E_CHIPID_CV1838;
	case 0x1829:
		return E_CHIPID_CV1829;
	case 0x1820:
		return E_CHIPID_CV1820;
	case 0x1823:
		return E_CHIPID_CV1823;
	case 0x1825:
		return E_CHIPID_CV1825;
	case 0x1835:
	default:
		return E_CHIPID_CV1835;
	}
#else
	return FPGA_EARLY_PORTING_CHIP_ID;
#endif
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_id);

uint32_t cvi_base_read_chip_version(void)
{
	uint32_t chip_version = 0;
	uint32_t chip_version_ex = 0;

	chip_version = ioread32(top_base);
	chip_version_ex = ioread32(top_base + GP_REG0_OFFSET);

	pr_debug("chip_version=0x%x\n", chip_version);

#ifdef CONFIG_ARCH_CV182X
	switch (chip_version) {
	case 0x18220000:
		return E_CHIPVERSION_U01;
	case 0x18220001:
		return E_CHIPVERSION_U02;
	default:
		return E_CHIPVERSION_U02;
	}
#elif defined(CONFIG_ARCH_CV183X)
	switch (chip_version) {
	case 0x18802000:
		return E_CHIPVERSION_U01;
	case 0x18802001:
		if (chip_version_ex == 0x18802002)
			return E_CHIPVERSION_U03;
		else
			return E_CHIPVERSION_U02;
	default:
		return E_CHIPVERSION_U03;
	}
#else
#error "Unknown chip arch"
#endif
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_version);

unsigned int cvi_base_read_chip_pwr_on_reason(void)
{
	unsigned int reason = 0;

	reason = ioread32(rtc_base + RTC_ST_ON_REASON);

	pr_debug("pwr on reason = 0x%x\n", reason);

	switch (reason) {
	case 0x800d0000:
	case 0x800f0000:
		return E_CHIP_PWR_ON_COLDBOOT;
	case 0x880d0003:
	case 0x880f0003:
		return E_CHIP_PWR_ON_WDT;
	case 0x80050009:
	case 0x800f0009:
		return E_CHIP_PWR_ON_SUSPEND;
	case 0x840d0003:
	case 0x840f0003:
		return E_CHIP_PWR_ON_WARM_RST;
	default:
		return E_CHIP_PWR_ON_COLDBOOT;
	}
}
EXPORT_SYMBOL_GPL(cvi_base_read_chip_pwr_on_reason);

static int base_open(struct inode *inode, struct file *filp)
{
	struct base_device *ndev = container_of(filp->private_data, struct base_device, miscdev);
	struct base_state *ps;
	int ret;

	if (!ndev) {
		pr_err("cannot find base private data\n");
		return -ENODEV;
	}
	ps = kzalloc(sizeof(struct base_state), GFP_KERNEL);
	if (!ps) {
		ret = -ENOMEM;
		goto out_free_ps;
	}

	device_lock(ndev->miscdev.this_device);
	ps->ndev = ndev;
	INIT_LIST_HEAD(&ps->list);
	ps->state_pid = get_pid(task_pid(current));
	ps->cred = get_current_cred();
	security_task_getsecid(current, &ps->secid);
	/* memory barrier in smp case. */
	smp_wmb();
	/* replace the private data with base state */
	filp->private_data = ps;
	list_add_tail(&ps->list, &ndev->ps_list);

	device_unlock(ndev->miscdev.this_device);
	pr_debug("base open ok\n");

	return 0;

 out_free_ps:
	kfree(ps);
	return ret;
}

static int base_release(struct inode *inode, struct file *filp)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;

	if (!ndev) {
		pr_err("cannot find base private data\n");
		return -ENODEV;
	}
	device_lock(ndev->miscdev.this_device);
	list_del_init(&ps->list);
	device_unlock(ndev->miscdev.this_device);
	put_pid(ps->state_pid);
	put_cred(ps->cred);
	kfree(ps);

	return 0;
}

static void signal_state_change(struct base_device *ndev, enum base_state_e state)
{
	struct base_state *ps;
	struct siginfo sinfo;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sigval_t addr;
#endif

	spin_lock(&ndev->lock);
	ndev->state = state;
	list_for_each_entry(ps, &ndev->ps_list, list) {
		if (ps->state_signr) {
			memset(&sinfo, 0, sizeof(sinfo));
			sinfo.si_signo = ps->state_signr;
			sinfo.si_errno = 0;
			sinfo.si_code = SI_ASYNCIO;
			sinfo.si_addr = ps->state_context;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			addr.sival_ptr = sinfo.si_addr;
			kill_pid_usb_asyncio(ps->state_signr, sinfo.si_errno, addr,
				ps->state_pid, ps->cred);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
			kill_pid_info_as_cred(ps->state_signr, &sinfo,
				ps->state_pid, ps->cred);
#else
			kill_pid_info_as_cred(ps->state_signr, &sinfo,
				ps->state_pid, ps->cred, ps->secid);
#endif
		}
	}
	spin_unlock(&ndev->lock);
}

static int base_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = ndev->shared_mem;

	if (offset < 0 || (vm_size + offset) > BASE_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long base_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct base_state *ps = filp->private_data;
	struct base_device *ndev = ps->ndev;
	long ret = 0;

	switch (cmd) {
	case IOCTL_READ_CHIP_ID: {
		unsigned long chip_id = 0;

		chip_id = cvi_base_read_chip_id();
		if (copy_to_user((uint32_t *) arg, &chip_id, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_READ_CHIP_VERSION: {
		uint32_t chip_version = 0;

		chip_version = cvi_base_read_chip_version();
		if (copy_to_user((uint32_t *) arg, &chip_version, sizeof(uint32_t)))
			return -EFAULT;
		break;
	}
	case IOCTL_READ_CHIP_PWR_ON_REASON: {
		unsigned long reason = 0;

		reason = cvi_base_read_chip_pwr_on_reason();
		if (copy_to_user((uint32_t *) arg, &reason, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_STATESIG: {
		struct base_statesignal ds;

		if (copy_from_user(&ds, (void __user *)arg, sizeof(ds)))
			return -EFAULT;
		ps->state_signr = ds.signr;
		ps->state_context = ds.context;
		ndev->sig_hook = 1;
		break;
	}
#ifdef CONFIG_COMPAT
	case IOCTL_STATESIG32: {
		struct base_statesignal32 ds;

		if (copy_from_user(&ds, arg, sizeof(ds)))
			return -EFAULT;
		ps->state_signr = ds.signr;
		ps->state_context = compat_ptr(ds.context);
		ndev->sig_hook = 1;
		break;
	}
#endif
	case IOCTL_READ_STATE: {
		unsigned int state;

		spin_lock(&ndev->lock);
		state = ndev->state;
		spin_unlock(&ndev->lock);
		if (copy_to_user((void __user *)arg, &state, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_USER_SUSPEND_DONE: {
		spin_lock(&ndev->lock);
		ndev->state = BASE_STATE_SUSPEND;
		spin_unlock(&ndev->lock);
		/* memory barrier in smp case. */
		smp_wmb();
		/* release the notifier */
		complete(&ndev->done);
		break;
	}
	case IOCTL_USER_RESUME_DONE: {
		spin_lock(&ndev->lock);
		ndev->state = BASE_STATE_NORMAL;
		spin_unlock(&ndev->lock);
		/* memory barrier in smp case. */
		smp_wmb();
		/* release the notifier */
		complete(&ndev->done);
		break;
	}
	case IOCTL_GET_VB_POOLS_MAX_CNT: {
		if (copy_to_user((uint32_t *) arg, &vb_max_pools, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case IOCTL_GET_VB_BLK_MAX_CNT: {
		if (copy_to_user((uint32_t *) arg, &vb_pool_max_blk, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	default:
		pr_err("Not support functions");
		return -ENOTTY;
	}
	return ret;
}

#ifdef CONFIG_COMPAT
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif
#endif

static const struct file_operations base_fops = {
	.owner = THIS_MODULE,
	.open = base_open,
	.release = base_release,
	.mmap = base_mmap,
	.unlocked_ioctl = base_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ptr_ioctl,
#endif
};

static int _register_dev(struct base_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = BASE_DEV_NAME;
	ndev->miscdev.fops = &base_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_base: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

#define POWER_PROC_NAME			"power"
#define POWER_PROC_PERMS		(0644)

static u8 *sel_state[] = {
	"suspend",
	"resume",
};

static int proc_power_show(struct seq_file *m, void *v)
{
	seq_puts(m, "suspend resume\n");

	return 0;
}

static int set_power_hdler(struct base_device *ndev, char const *input)
{
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s", str);
	if (num > 1) {
		return -EINVAL;
	}

	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(sel_state);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, sel_state[i])) {
			t = i;
			break;
		}
	}
	if (i == n)
		return -EINVAL;

	signal_state_change(ndev, t ? BASE_STATE_RESUME : BASE_STATE_SUSPEND_PREPARE);

	return 0;
}

static ssize_t power_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct base_device *ndev = PDE_DATA(file_inode(file));

	set_power_hdler(ndev, user_buf);

	return count;
}

static int proc_power_open(struct inode *inode, struct file *file)
{
	struct base_device *ndev = PDE_DATA(inode);

	return single_open(file, proc_power_show, ndev);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _power_proc_fops = {
	.proc_open = proc_power_open,
	.proc_read = seq_read,
	.proc_write = power_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _power_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_power_open,
	.read		= seq_read,
	.write		= power_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int power_proc_init(struct proc_dir_entry *_proc_dir, void *ndev)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(POWER_PROC_NAME, POWER_PROC_PERMS, _proc_dir, &_power_proc_fops, ndev) == NULL) {
		pr_err("power proc creation failed\n");
		rc = -1;
	}

	return rc;
}

static int power_proc_remove(struct proc_dir_entry *_proc_dir)
{
	remove_proc_entry(POWER_PROC_NAME, _proc_dir);

	return 0;
}

static int base_suspend_user(struct base_device *ndev)
{
	int ret;

	if (!ndev->sig_hook)
		return 0;
	signal_state_change(ndev, BASE_STATE_SUSPEND_PREPARE);
	ret = wait_for_completion_timeout(&ndev->done, usecs_to_jiffies(500000));
	if (ret < 0) {
		pr_info("user space suspend expired, state = %d\n", ndev->state);
		ndev->state = BASE_STATE_SUSPEND;
		return 0;
	}
	if (ndev->state != BASE_STATE_SUSPEND) {
		pr_info("expect suspend(2) but state = %d\n", ndev->state);
		ndev->state = BASE_STATE_SUSPEND;
	}
	return 0;
}

static int base_resume_user(struct base_device *ndev)
{
	int ret;

	if (!ndev->sig_hook)
		return 0;
	signal_state_change(ndev, BASE_STATE_RESUME);
	ret = wait_for_completion_timeout(&ndev->done, usecs_to_jiffies(500000));
	if (ret < 0) {
		pr_info("user space resume expired, state = %d\n", ndev->state);
		ndev->state = BASE_STATE_NORMAL;
		return 0;
	}
	if (ndev->state != BASE_STATE_NORMAL) {
		pr_info("expect normal(0) but state = %d\n", ndev->state);
		ndev->state = BASE_STATE_NORMAL;
	}
	return 0;
}

static int base_pm_notif(struct notifier_block *b, unsigned long v, void *d)
{
	struct base_device *ndev = container_of(b, struct base_device, notifier);

	pr_info("pm notif %lu\n", v);
	switch (v) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
	case PM_RESTORE_PREPARE:
		pr_info("suspending displays\n");
		return base_suspend_user(ndev);

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
		pr_info("resuming displays\n");
		return base_resume_user(ndev);

	default:
		return 0;
	}
}

static int base_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct base_device *ndev;
	int ret;

	pr_debug("base_probe start\n");

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->shared_mem = kzalloc(BASE_SHARE_MEM_SIZE, GFP_KERNEL);
	if (!ndev->shared_mem)
		return -ENOMEM;

	proc_dir = proc_mkdir("cvitek", NULL);
	if (vb_proc_init(proc_dir, ndev->shared_mem) < 0)
		pr_err("vb proc init failed\n");

	if (log_proc_init(proc_dir, ndev->shared_mem) < 0)
		pr_err("log proc init failed\n");

	if (sys_proc_init(proc_dir, ndev->shared_mem) < 0)
		pr_err("sys proc init failed\n");

	if (power_proc_init(proc_dir, ndev) < 0)
		pr_err("power proc init failed\n");

	spin_lock_init(&ndev->lock);
	INIT_LIST_HEAD(&ndev->ps_list);
	ndev->state = BASE_STATE_NORMAL;
	ndev->dev = dev;
	init_completion(&ndev->done);

	ndev->notifier.notifier_call = base_pm_notif;
	register_pm_notifier(&ndev->notifier);
	ret = _register_dev(ndev);
	if (ret < 0) {
		pr_err("regsiter base chrdev error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);
	pr_debug("%s DONE\n", __func__);

	return 0;
}

static int base_remove(struct platform_device *pdev)
{
	struct base_device *ndev = platform_get_drvdata(pdev);

	unregister_pm_notifier(&ndev->notifier);
	vb_proc_remove(proc_dir);
	log_proc_remove(proc_dir);
	sys_proc_remove(proc_dir);
	power_proc_remove(proc_dir);
	proc_remove(proc_dir);
	proc_dir = NULL;
	kfree(ndev->shared_mem);
	ndev->shared_mem = NULL;

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);
	pr_debug("%s DONE\n", __func__);

	return 0;
}

static const struct of_device_id cvi_base_dt_match[] = { { .compatible = "cvitek,base" }, {} };

static struct platform_driver base_driver = {
	.probe = base_probe,
	.remove = base_remove,
	.driver = {
		.name = BASE_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cvi_base_dt_match,
	},
};

static void base_cleanup(void)
{
	class_remove_file(pbase_class, &class_attr_base_efuse_shadow);
	class_remove_file(pbase_class, &class_attr_base_efuse_prog);
	class_remove_file(pbase_class, &class_attr_base_uid);
	class_remove_file(pbase_class, &class_attr_base_rosc);
	class_destroy(pbase_class);
}

static int __init base_init(void)
{
	int rc;
	uint32_t chip_id;

	top_base = ioremap(TOP_BASE, TOP_REG_BANK_SIZE);

	pbase_class = class_create(THIS_MODULE, BASE_CLASS_NAME);
	if (IS_ERR(pbase_class)) {
		pr_err("create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_shadow);
	if (rc) {
		pr_err("base: can't create sysfs base_efuse_shadow file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_efuse_prog);
	if (rc) {
		pr_err("base: can't create sysfs base_efuse_prog) file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_uid);
	if (rc) {
		pr_err("base: can't create sysfs base_uid file\n");
		goto cleanup;
	}

	rc = class_create_file(pbase_class, &class_attr_base_rosc);
	if (rc) {
		pr_err("base: can't create sysfs base_rosc file\n");
		goto cleanup;
	}

	rc = platform_driver_register(&base_driver);
	chip_id = cvi_base_read_chip_id();
	pr_notice("CVITEK CHIP ID = %d\n", chip_id);

	if (IS_CHIP_CV182X(chip_id))
		rtc_base = ioremap(CV182X_RTC_BASE, RTC_REG_BANK_SIZE);
	else
		rtc_base = ioremap(CV183X_RTC_BASE, RTC_REG_BANK_SIZE);

	return 0;

cleanup:
	base_cleanup();

	return rc;
}

static void __exit base_exit(void)
{
	platform_driver_unregister(&base_driver);
	base_cleanup();

	iounmap(top_base);
	iounmap(rtc_base);
}

/* sensor cmm extern function. */
enum vip_sys_cmm {
	VIP_CMM_I2C = 0,
	VIP_CMM_SSP,
	VIP_CMM_BUTT,
};

struct vip_sys_cmm_ops {
	long (*cb)(void *hdlr, unsigned int cmd, void *arg);
};

struct vip_sys_cmm_dev {
	enum vip_sys_cmm		cmm_type;
	void				*hdlr;
	struct vip_sys_cmm_ops		ops;
};
static struct vip_sys_cmm_dev cmm_ssp;
static struct vip_sys_cmm_dev cmm_i2c;

int vip_sys_register_cmm_cb(unsigned long cmm, void *hdlr, void *cb)
{
	struct vip_sys_cmm_dev *cmm_dev;

	if ((cmm >= VIP_CMM_BUTT) || !hdlr || !cb)
		return -1;

	cmm_dev = (cmm == VIP_CMM_I2C) ? &cmm_i2c : &cmm_ssp;

	cmm_dev->cmm_type = cmm;
	cmm_dev->hdlr = hdlr;
	cmm_dev->ops.cb = cb;

	return 0;
}
EXPORT_SYMBOL_GPL(vip_sys_register_cmm_cb);

int vip_sys_cmm_cb_i2c(unsigned int cmd, void *arg)
{
	struct vip_sys_cmm_dev *cmm_dev = &cmm_i2c;

	if (cmm_dev->cmm_type != VIP_CMM_I2C)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vip_sys_cmm_cb_i2c);

int vip_sys_cmm_cb_ssp(unsigned int cmd, void *arg)
{
	struct vip_sys_cmm_dev *cmm_dev = &cmm_ssp;

	if (cmm_dev->cmm_type != VIP_CMM_SSP)
		return -1;

	return (cmm_dev->ops.cb) ? cmm_dev->ops.cb(cmm_dev->hdlr, cmd, arg) : (-1);
}
EXPORT_SYMBOL_GPL(vip_sys_cmm_cb_ssp);

/* cif extern function. */
struct vip_sys_cif_ops {
	long (*cb)(void *hdlr, unsigned int cmd, unsigned long arg);
};

struct vip_sys_cif_dev {
	void				*hdlr;
	struct vip_sys_cif_ops		ops;
};


static struct vip_sys_cif_dev cif_dev;

int vip_sys_register_cif_cb(void *hdlr, void *cb)
{
	if (!hdlr || !cb)
		return -1;

	cif_dev.hdlr = hdlr;
	cif_dev.ops.cb = cb;

	return 0;
}
EXPORT_SYMBOL_GPL(vip_sys_register_cif_cb);

int vip_sys_cif_cb(unsigned int cmd, void *arg)
{
	return (cif_dev.ops.cb) ?
		cif_dev.ops.cb(cif_dev.hdlr, cmd, (unsigned long)arg) : (-1);
}
EXPORT_SYMBOL_GPL(vip_sys_cif_cb);

MODULE_DESCRIPTION("Cvitek base driver");
MODULE_LICENSE("GPL");
module_init(base_init);
module_exit(base_exit);
