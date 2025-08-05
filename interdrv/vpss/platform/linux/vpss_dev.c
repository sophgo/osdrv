#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>

#include "vpss_debug.h"
#include "vpss_dev.h"
#include "vpss_ioctl.h"
#include "vpss_core.h"
#include "vpss_proc.h"

#define VPSS_DEV_NAME "soph-vpss"


static const char *const vpss_name[VPSS_MAX] = {"vpss_v0", "vpss_v1", "vpss_v2", "vpss_v3"};
static const char *const vpss_clk_name[VPSS_MAX] = {"reg_clk_vpss0_vip_en",
													"reg_clk_vpss1_vip_en",
													"reg_clk_vpss2_vip_en",
													"reg_clk_vpss3_vip_en"};
u32 vpss_log_lv = DBG_WARN;
module_param(vpss_log_lv, int, 0644);

static irqreturn_t vpss_isr(int irq, void *data)
{
	vpss_core_isr(irq, data);
	return IRQ_HANDLED;
}

static int vpss_open(struct inode *inode, struct file *filep)
{
	int i;
	struct vpss_dev_data *dev_data =
		container_of(filep->private_data, struct vpss_dev_data, miscdev);

	if (!dev_data) {
		TRACE_VPSS(DBG_ERR, "Cannot find vpss private data\n");
		return -ENODEV;
	}

	filep->private_data = dev_data;

	i = osal_atomic_inc_return(&dev_data->open_count);
	if (i > 1) {
		TRACE_VPSS(DBG_INFO, "%s: open %d times\n", __func__, i);
		return 0;
	}
	vpss_core_open(&dev_data->cores);

	TRACE_VPSS(DBG_INFO, "%s\n", __func__);

	return 0;
}

static int vpss_release(struct inode *inode, struct file *filep)
{
	int i;
	struct vpss_dev_data *dev_data = filep->private_data;

	filep->private_data = NULL;

	i = osal_atomic_dec_return(&dev_data->open_count);
	if (i) {
		TRACE_VPSS(DBG_INFO, "%s: open %d times\n", __func__, i);
		return 0;
	}
	vpss_core_release(&dev_data->cores);

	TRACE_VPSS(DBG_INFO, "%s\n", __func__);

	return 0;
}

static long __vpss_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct vpss_dev_data *dev_data = filp->private_data;

	return vpss_ioctl(&dev_data->cores, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long vpss_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations vpss_fops = {
	.owner = THIS_MODULE,
	.open = vpss_open,
	.release = vpss_release,
	.unlocked_ioctl = __vpss_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vpss_compat_ptr_ioctl,
#endif
};

static int vpss_init_resources(struct platform_device *pdev)
{
	int rc = 0;
	int irq_num;
	struct resource *res = NULL;
	void *reg_base;
	struct vpss_dev_data *dev;
	int i;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		TRACE_VPSS(DBG_ERR, "Can not get vpss drvdata\n");
		return -EINVAL;
	}

	/* register ioremap */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vpss");
	if (!res) {
		TRACE_VPSS(DBG_ERR, "vpss no memory resource defined\n");
		return -EINVAL;
	}
	reg_base = devm_ioremap(&pdev->dev, res->start,
					res->end - res->start);

	TRACE_VPSS(DBG_INFO, "vpss res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
		res->start, res->end, reg_base);
	if (!reg_base) {
		TRACE_VPSS(DBG_ERR, "Failed to get reg_base[%d]\n", i);
		return -EINVAL;
	}
	vpss_set_base_addr(reg_base);

	/* Interrupt */
	for (i = 0; i < VPSS_MAX; ++i) {
		irq_num = platform_get_irq_byname(pdev, vpss_name[i]);
		if (irq_num < 0) {
			TRACE_VPSS(DBG_ERR, "No IRQ resource for [%d]%s\n", i, vpss_name[i]);
			return -ENODEV;
		}
		TRACE_VPSS(DBG_INFO, "irq(%d) for %s get from platform driver.\n",
			irq_num, vpss_name[i]);
		dev->cores.core[i].irq_num = irq_num;
	}

	/* clk */
	for (i = 0; i < VPSS_MAX; ++i) {
		dev->cores.core[i].clk = osal_clk_get(&pdev->dev, vpss_clk_name[i]);
		if (dev->cores.core[i].clk == NULL) {
			TRACE_VPSS(DBG_ERR, "Cannot get source clk for vpss%d\n", i);
		}
	}

	return rc;
}

static int vpss_probe(struct platform_device *pdev)
{
	int rc = 0;
	int i;
	struct vpss_dev_data *dev_data;

	/* allocate main structure */
	dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		TRACE_VPSS(DBG_ERR, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, dev_data);

	// get hw-resources
	rc = vpss_init_resources(pdev);
	if (rc) {
		TRACE_VPSS(DBG_ERR, "Failed to init resource\n");
		goto err0;
	}

	dev_data->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev_data->miscdev.name = devm_kasprintf(&pdev->dev, GFP_KERNEL, VPSS_DEV_NAME);
	dev_data->miscdev.fops = &vpss_fops;
	rc = misc_register(&dev_data->miscdev);
	if (rc) {
		TRACE_VPSS(DBG_ERR, "failed to register misc device.\n");
		goto err0;
	}

	rc = vpss_proc_init(&dev_data->cores);
	if (rc) {
		TRACE_VPSS(DBG_ERR, "Failed to init vpss proc\n");
		goto err1;
	}

	for (i = 0; i < VPSS_MAX; ++i) {
		if (devm_request_irq(&pdev->dev, dev_data->cores.core[i].irq_num, vpss_isr, IRQF_SHARED,
			vpss_name[i], &dev_data->cores.core[i])) {
			TRACE_VPSS(DBG_ERR, "Unable to request vpss IRQ(%d)\n",
					dev_data->cores.core[i].irq_num);
			goto err2;
		}
	}

	vpss_core_init(&dev_data->cores);
	osal_atomic_set(&dev_data->open_count, 0);
	TRACE_VPSS(DBG_WARN, "vpss probe done\n");

	return rc;

err2:
	vpss_proc_remove(&dev_data->cores);
err1:
	misc_deregister(&dev_data->miscdev);
err0:
	dev_set_drvdata(&pdev->dev, NULL);
	TRACE_VPSS(DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * bmd_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int vpss_remove(struct platform_device *pdev)
{
	struct vpss_dev_data *dev_data;
	int i;

	if (!pdev) {
		TRACE_VPSS(DBG_ERR, "invalid param\n");
		return -EINVAL;
	}

	dev_data = dev_get_drvdata(&pdev->dev);
	if (!dev_data) {
		TRACE_VPSS(DBG_ERR, "Can not get vpss drvdata\n");
		return -EINVAL;
	}

	vpss_core_deinit(&dev_data->cores);

	for (i = 0; i < VPSS_MAX; ++i) {
		osal_clk_put(&pdev->dev, dev_data->cores.core[i].clk);
		dev_data->cores.core[i].clk = NULL;
		devm_free_irq(&pdev->dev, dev_data->cores.core[i].irq_num, &dev_data->cores.core[i]);
	}

	vpss_proc_remove(&dev_data->cores);
	misc_deregister(&dev_data->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	TRACE_VPSS(DBG_WARN, "vpss remove done\n");

	return 0;
}

int vpss_suspend(struct device *dev)
{
	TRACE_VPSS(DBG_WARN, "vpss suspended\n");

	return 0;
}

int vpss_resume(struct device *dev)
{
	TRACE_VPSS(DBG_WARN, "vpss resumed\n");

	return 0;
}

static const struct of_device_id vpss_dt_match[] = {
	{.compatible = "cvitek,vpss"},
	{}
};

static SIMPLE_DEV_PM_OPS(vpss_pm_ops, vpss_suspend,
				vpss_resume);

static struct platform_driver vpss_pdrv = {
	.probe      = vpss_probe,
	.remove     = vpss_remove,
	.driver     = {
		.name		= "vpss",
		.owner		= THIS_MODULE,
		.of_match_table	= vpss_dt_match,
		.pm		= &vpss_pm_ops,
	},
};

static int __init vpss_dev_init(void)
{
	int rc;

	TRACE_VPSS(DBG_INFO, " +\n");
	rc = platform_driver_register(&vpss_pdrv);

	return rc;
}

static void __exit vpss_dev_exit(void)
{
	TRACE_VPSS(DBG_INFO, " +\n");

	platform_driver_unregister(&vpss_pdrv);
}

MODULE_DESCRIPTION("Cvitek Video Driver");
MODULE_AUTHOR("weiyong.luo");
MODULE_LICENSE("GPL");
module_init(vpss_dev_init);
module_exit(vpss_dev_exit);
