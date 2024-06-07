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
#include <linux/compat.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/base_uapi.h>
#include "vb.h"
#include "bind.h"
#include "ion.h"
#include "cvi_vb_proc.h"
#include "cvi_log_proc.h"
#include "cvi_sys_proc.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define BASE_CLASS_NAME "soph-base"
#define BASE_DEV_NAME "soph-base"


struct base_device {
	//struct device *dev;
	struct miscdevice miscdev;
	void *shared_mem;
	//u16 mmap_count;
	//u16 use_count;
};


static struct proc_dir_entry *proc_dir;
static struct class *pbase_class;


static int base_open(struct inode *inode, struct file *filp)
{
	struct base_device *ndev = container_of(filp->private_data, struct base_device, miscdev);
	int ret = 0;
	UNUSED(inode);

	if (!ndev) {
		pr_err("cannot find base private data\n");
		return -ENODEV;
	}

	filp->private_data = ndev;
	pr_debug("base open ok\n");

	return ret;
}

static int base_release(struct inode *inode, struct file *filp)
{
	UNUSED(inode);

	vb_release();
	bind_deinit();
	pr_debug("base release ok\n");

	return 0;
}

static int base_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct base_device *ndev = filp->private_data;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = ndev->shared_mem;

	if ((vm_size + offset) > BASE_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("mmap vir(%p) phys(%#llx)\n", pos, (u64)virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long base_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	switch (cmd) {
	case BASE_VB_CMD:
	{
		ret = vb_ctrl(arg);
		break;
	}

	case BASE_SET_BINDCFG:
	{
		ret = bind_set_cfg_user(arg);
		break;
	}

	case BASE_GET_BINDCFG:
	{
		ret = bind_get_cfg_user(arg);
		break;
	}

	case BASE_ION_ALLOC:
	{
		struct sys_ion_data stIonDate;
		void *addr_v = NULL;

		if (copy_from_user(&stIonDate, (struct sys_ion_data __user *)arg,
					sizeof(struct sys_ion_data)))
			return -EINVAL;

		ret = base_ion_alloc(&stIonDate.addr_p, &addr_v, stIonDate.name,
				stIonDate.size, stIonDate.cached);

		if (copy_to_user((struct sys_ion_data __user *)arg, &stIonDate,
					sizeof(struct sys_ion_data)))
			return -EINVAL;

		break;
	}

	case BASE_ION_FREE:
	{
		struct sys_ion_data stIonDate;

		if (copy_from_user(&stIonDate, (struct sys_ion_data __user *)arg,
					sizeof(struct sys_ion_data)))
			return -EINVAL;

		ret = base_ion_free(stIonDate.addr_p);
		if (ret < 0) {
			pr_err("base_ion_free fail\n");
			return -EINVAL;
		} else {
			stIonDate.size = ret;
		}
		if (copy_to_user((struct sys_ion_data __user *)arg, &stIonDate,
					sizeof(struct sys_ion_data)))
			return -EINVAL;
		break;
	}

	case BASE_CACHE_INVLD:
	{
		struct sys_cache_op stCacheOp;

		if (copy_from_user(&stCacheOp, (struct sys_cache_op __user *)arg,
			sizeof(struct sys_cache_op)))
			return -EINVAL;

		ret = base_ion_cache_invalidate(stCacheOp.addr_p, stCacheOp.addr_v, stCacheOp.size);
		break;
	}

	case BASE_CACHE_FLUSH:
	{
		struct sys_cache_op stCacheOp;

		if (copy_from_user(&stCacheOp, (struct sys_cache_op __user *)arg,
			sizeof(struct sys_cache_op)))
			return -EINVAL;

		ret = base_ion_cache_flush(stCacheOp.addr_p, stCacheOp.addr_v, stCacheOp.size);
		break;
	}

	default:
		pr_err("Not support functions");
		return -ENOTTY;
	}
	return ret;
}


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

static int base_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct base_device *ndev;
	int ret;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->shared_mem = kzalloc(BASE_SHARE_MEM_SIZE, GFP_KERNEL);
	if (!ndev->shared_mem)
		return -ENOMEM;

	proc_dir = proc_mkdir("soph", NULL);
	if (vb_proc_init(proc_dir) < 0)
		pr_err("vb proc init failed\n");

	if (log_proc_init(proc_dir, ndev->shared_mem) < 0)
		pr_err("log proc init failed\n");

	if (sys_proc_init(proc_dir, ndev->shared_mem) < 0)
		pr_err("sys proc init failed\n");

	if (vb_create_instance()) {
		pr_err("vb_create_instance failed\n");
		return -ENOMEM;
	}

	bind_init();
	ndev->miscdev.name = BASE_DEV_NAME;
	ndev->miscdev.fops = &base_fops;

	ret = misc_register(&ndev->miscdev);
	if (ret) {
		dev_err(dev, "cvi_base: failed to register misc device.\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);
	pr_info("base probe done\n");

	return 0;
}

static int base_remove(struct platform_device *pdev)
{
	struct base_device *ndev = platform_get_drvdata(pdev);

	bind_deinit();
	vb_destroy_instance();

	vb_proc_remove(proc_dir);
	log_proc_remove(proc_dir);
	sys_proc_remove(proc_dir);
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

static int __init base_init(void)
{
	int rc;

	pbase_class = class_create(THIS_MODULE, BASE_CLASS_NAME);
	if (IS_ERR(pbase_class)) {
		pr_err("create class failed\n");
		rc = PTR_ERR(pbase_class);
		goto cleanup;
	}

	rc = platform_driver_register(&base_driver);

	return 0;

cleanup:
	class_destroy(pbase_class);

	return rc;
}

static void __exit base_exit(void)
{
	platform_driver_unregister(&base_driver);
	vb_cleanup();
	class_destroy(pbase_class);
}


MODULE_DESCRIPTION("Cvitek base driver");
MODULE_LICENSE("GPL");
module_init(base_init);
module_exit(base_exit);
