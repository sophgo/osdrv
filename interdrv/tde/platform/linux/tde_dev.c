#include <linux/module.h>
#include <linux/kernel.h>
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
#include <linux/version.h>

#include "tde_debug.h"
#include "tde_dev.h"
#include "tde_ioctl.h"
#include "tde_core.h"
#include "tde_proc.h"
#include "tde_reg.h"
#include "base_cb.h"
#include "tde_cb.h"
#include "tde_inter_cb.h"

#define TDE_DEV_NAME "soph-tde"
#define TDE_REG_NAME "tde"
#define TDE_INTERRUPT_NAME "tde"

u32 tde_log_lv = DBG_WARN;
module_param(tde_log_lv, int, 0644);

static int tde_exec_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	struct tde_dev_data *dev_data = (struct tde_dev_data *)dev;
	struct tde_inter_cfg *cfg;
	int rc = -1;

	switch (cmd) {
		case TDE_CB_OP: {
			cfg = (struct tde_inter_cfg *)arg;

			if (osal_atomic_inc_return(&dev_data->open_count) == 1)
				tde_core_open(&dev_data->core);
			rc = tde_do_op(&dev_data->core, cfg->usage, cfg->usage_param
				, cfg->width, cfg->height, cfg->src_addr
				, cfg->dst_addr, cfg->sync_io, cfg->block
				, (enum tde_task_mode_e)cfg->task_mode);
			if (osal_atomic_dec_return(&dev_data->open_count) == 0)
				tde_core_release(&dev_data->core);
			break;
		}
		default: {
			TRACE_TDE(DBG_WARN, "invalid cb CMD\n");
			break;
		}
	}

	return rc;
}

static int tde_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_TDE);
}

static int tde_reg_cb(struct tde_dev_data *dev_data)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_TDE;
	reg_cb.dev		= (void *)dev_data;
	reg_cb.cb		= tde_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static irqreturn_t tde_isr(int irq, void *data)
{
	struct tde_dev_data *dev_data = (struct tde_dev_data *)data;

	if (irq != dev_data->irq_num) {
		TRACE_TDE(DBG_ERR, "irq num error.\n");
		return IRQ_HANDLED;
	}

	tde_core_isr(&dev_data->core);
	return IRQ_HANDLED;
}

static int tde_open(struct inode *inode, struct file *filep)
{
	int i;
	struct tde_dev_data *dev_data =
		container_of(filep->private_data, struct tde_dev_data, miscdev);

	if (!dev_data) {
		TRACE_TDE(DBG_ERR, "Cannot find tde private data\n");
		return -ENODEV;
	}

	filep->private_data = dev_data;

	i = osal_atomic_inc_return(&dev_data->open_count);
	if (i > 1) {
		TRACE_TDE(DBG_INFO, "%s: open %d times\n", __func__, i);
		return 0;
	}
	tde_core_open(&dev_data->core);

	TRACE_TDE(DBG_INFO, "%s\n", __func__);

	return 0;
}

static int tde_release(struct inode *inode, struct file *filep)
{
	int i;
	struct tde_dev_data *dev_data = filep->private_data;

	filep->private_data = NULL;

	i = osal_atomic_dec_return(&dev_data->open_count);
	if (i) {
		TRACE_TDE(DBG_INFO, "%s: open %d times\n", __func__, i);
		return 0;
	}
	tde_core_release(&dev_data->core);

	TRACE_TDE(DBG_INFO, "%s\n", __func__);

	return 0;
}

static long __tde_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct tde_dev_data *dev_data = filp->private_data;

	return tde_ioctl(&dev_data->core, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long tde_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations tde_fops = {
	.owner = THIS_MODULE,
	.open = tde_open,
	.release = tde_release,
	.unlocked_ioctl = __tde_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tde_compat_ptr_ioctl,
#endif
};

static int tde_init_resources(struct platform_device *pdev)
{
	int irq_num;
	struct resource *res = NULL;
	void *reg_base;
	struct tde_dev_data *dev;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get vip drvdata\n");
		return -EINVAL;
	}

	/* register ioremap */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, TDE_REG_NAME);
	if (!res) {
		TRACE_TDE(DBG_ERR, "tde no memory resource defined\n");
		return -EINVAL;
	}
#if (KERNEL_VERSION(5, 10, 0) < LINUX_VERSION_CODE)
	reg_base = devm_ioremap(&pdev->dev, res->start,
					res->end - res->start);
#else
	reg_base = devm_ioremap_nocache(&pdev->dev, res->start,
					res->end - res->start);
#endif
	TRACE_TDE(DBG_INFO, "tde res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
		res->start, res->end, reg_base);
	if (!reg_base) {
		TRACE_TDE(DBG_ERR, "Failed to get reg_base\n");
		return -EINVAL;
	}
	tde_set_base_addr(reg_base);

	/* Interrupt */
	irq_num = platform_get_irq_byname(pdev, TDE_INTERRUPT_NAME);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", TDE_INTERRUPT_NAME);
		return -ENODEV;
	}
	TRACE_TDE(DBG_INFO, "irq(%d) for %s get from platform driver.\n",
		irq_num, TDE_INTERRUPT_NAME);
	dev->irq_num = irq_num;

	/* clk */
	dev->core.clk = osal_clk_get(&pdev->dev, "reg_clk_2de_vip_en");
	if (dev->core.clk == NULL) {
		dev_err(&pdev->dev, "Cannot get source clk for 2de\n");
	}

	return 0;
}

static int tde_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct tde_dev_data *dev_data;

	/* allocate main structure */
	dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		TRACE_TDE(DBG_ERR, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&pdev->dev, dev_data);

	// get hw-resources
	rc = tde_init_resources(pdev);
	if (rc) {
		TRACE_TDE(DBG_ERR, "Failed to init resource\n");
		goto err0;
	}

	dev_data->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev_data->miscdev.name = devm_kasprintf(&pdev->dev, GFP_KERNEL, TDE_DEV_NAME);
	dev_data->miscdev.fops = &tde_fops;
	rc = misc_register(&dev_data->miscdev);
	if (rc) {
		TRACE_TDE(DBG_ERR, "failed to register misc device.\n");
		goto err0;
	}

	rc = tde_proc_init(&dev_data->core);
	if (rc) {
		TRACE_TDE(DBG_ERR, "Failed to init tde proc\n");
		goto err1;
	}

	if (devm_request_irq(&pdev->dev, dev_data->irq_num, tde_isr, IRQF_SHARED, TDE_INTERRUPT_NAME, dev_data)) {
		TRACE_TDE(DBG_ERR, "Unable to request tde IRQ(%d)\n",
				dev_data->irq_num);
		goto err2;
	}

	rc = tde_core_init(&dev_data->core);
	if (rc) {
		TRACE_TDE(DBG_ERR, "Failed to init tde core\n");
		goto err3;
	}
	rc = tde_reg_cb(dev_data);
	if (rc) {
		TRACE_TDE(DBG_ERR, "Failed to register callback\n");
		goto err4;
	}

	osal_atomic_set(&dev_data->open_count, 0);

	TRACE_TDE(DBG_WARN, "tde probe done\n");

	return rc;

err4:
	tde_core_deinit(&dev_data->core);
err3:
	devm_free_irq(&pdev->dev, dev_data->irq_num, dev_data);
err2:
	tde_proc_remove(&dev_data->core);
err1:
	misc_deregister(&dev_data->miscdev);
err0:
	dev_set_drvdata(&pdev->dev, NULL);
	TRACE_TDE(DBG_ERR, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * bmd_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int tde_remove(struct platform_device *pdev)
{
	struct tde_dev_data *dev_data;

	if (!pdev) {
		TRACE_TDE(DBG_ERR, "invalid param\n");
		return -EINVAL;
	}

	dev_data = dev_get_drvdata(&pdev->dev);
	if (!dev_data) {
		TRACE_TDE(DBG_ERR, "Can not get tde drvdata\n");
		return -EINVAL;
	}

	tde_rm_cb();
	tde_core_deinit(&dev_data->core);

	devm_free_irq(&pdev->dev, dev_data->irq_num, dev_data);

	tde_proc_remove(&dev_data->core);
	misc_deregister(&dev_data->miscdev);
	osal_clk_put(&pdev->dev, dev_data->core.clk);
	dev_set_drvdata(&pdev->dev, NULL);

	TRACE_TDE(DBG_WARN, "tde remove done\n");

	return 0;
}

int tde_suspend(struct device *dev)
{
	struct tde_dev_data *dev_data;

	if (!dev) {
		TRACE_TDE(DBG_ERR, "invalid param\n");
		return -EINVAL;
	}

	dev_data = dev_get_drvdata(dev);
	if (!dev_data) {
		TRACE_TDE(DBG_ERR, "Can not get tde drvdata\n");
		return -EINVAL;
	}

	TRACE_TDE(DBG_WARN, "tde suspended + \n");
	tde_core_suspend(&dev_data->core);
	TRACE_TDE(DBG_WARN, "tde suspended - \n");

	return 0;
}

int tde_resume(struct device *dev)
{
	struct tde_dev_data *dev_data;

	if (!dev) {
		TRACE_TDE(DBG_ERR, "invalid param\n");
		return -EINVAL;
	}

	dev_data = dev_get_drvdata(dev);
	if (!dev_data) {
		TRACE_TDE(DBG_ERR, "Can not get tde drvdata\n");
		return -EINVAL;
	}

	TRACE_TDE(DBG_WARN, "tde resumed + \n");
	tde_core_resume(&dev_data->core);
	TRACE_TDE(DBG_WARN, "tde resumed - \n");

	return 0;
}

static const struct of_device_id tde_dt_match[] = {
	{.compatible = "cvitek,tde"},
	{}
};

static SIMPLE_DEV_PM_OPS(tde_pm_ops, tde_suspend,
				tde_resume);

static struct platform_driver tde_pdrv = {
	.probe      = tde_probe,
	.remove     = tde_remove,
	.driver     = {
		.name		= "tde",
		.owner		= THIS_MODULE,
		.of_match_table	= tde_dt_match,
		.pm		= &tde_pm_ops,
	},
};

static int __init tde_dev_init(void)
{
	int rc;

	TRACE_TDE(DBG_INFO, " +\n");
	rc = platform_driver_register(&tde_pdrv);

	return rc;
}

static void __exit tde_dev_exit(void)
{
	TRACE_TDE(DBG_INFO, " +\n");

	platform_driver_unregister(&tde_pdrv);
}

MODULE_DESCRIPTION("Cvitek Video Driver");
MODULE_AUTHOR("weiyong.luo");
MODULE_LICENSE("GPL");
module_init(tde_dev_init);
module_exit(tde_dev_exit);
