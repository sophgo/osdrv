#include "cif_comm.h"
#include "cif_l.h"
#include "cif_ioctl.h"
#include "cif.h"

static struct proc_dir_entry *cif_proc_entry;
const struct proc_ops cif_proc_fops;

static int cif_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int cif_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations cif_fops = {
	.owner = THIS_MODULE,
	.open = cif_open,
	.release = cif_release,
	.unlocked_ioctl = cif_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cif_compat_ptr_ioctl,
#endif
};

int cif_init_miscdev(struct platform_device *pdev, struct cif_dev *dev)
{
	int rc;

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = MIPI_RX_DEV_NAME;
	dev->miscdev.fops = &cif_fops;

	rc = misc_register(&dev->miscdev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to register misc device.\n");
		return rc;
	}

	rc = cif_init_register(dev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to init register.\n");
		return rc;
	}

	return rc;
}

static int cif_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cif_dev *dev;
	/* allocate main cif state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);
	platform_set_drvdata(pdev, dev);

	rc = cif_init_miscdev(pdev, dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init miscdev for cif, %d\n", rc);
		return rc;
	}

	/* register cif_cb */
	rc = cif_register_cb(dev);
	if (rc) {
		dev_err(&pdev->dev, "cif: failed to register callbacks.\n");
	}

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for cif, %d\n", rc);
		return rc;
	}

#ifdef CONFIG_PROC_FS
	cif_proc_entry = proc_create_data(CIF_PROC_NAME, 0, NULL,
					  &cif_proc_fops, dev);
	if (!cif_proc_entry)
		dev_err(&pdev->dev, "cif: can't init procfs.\n");
#endif

	return 0;
}

static int cif_remove(struct platform_device *pdev)
{
	struct cif_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	/* rm cif_cb */
	if (cif_rm_cb())
		dev_err(&pdev->dev, "cif: failed to rm cb.\n");

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cif drvdata");
		return 0;
	}

	misc_deregister(&dev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

#ifdef CONFIG_PROC_FS
	proc_remove(cif_proc_entry);
#endif
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cvi_cif_suspend(struct platform_device *pdev)
{
	struct cif_dev *dev;
	int i;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);

	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata");
		return 0;
	}

	for (i = 0; i < MAX_LINK_NUM; i++) {
		dev->is_mac_on[i] = dev->link[i].is_on;
		if (dev->is_mac_on[i]) {
			cif_reset_mipi(dev, i);
		}
	}
	proc_remove(cif_proc_entry);
	dev_info(&pdev->dev, "cif suspend end");
	return 0;
}

static int cvi_cif_resume(struct platform_device *pdev)
{
	struct cif_dev *dev;
	int i;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);

	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata");
		return 0;
	}

	cif_proc_entry = proc_create_data("mipi-rx", 0, NULL, &cif_proc_fops, dev);
	for (i = 0; i < MAX_LINK_NUM; i++) {
		if (dev->is_mac_on[i]) {
			cif_set_dev_attr(dev, &dev->link[i].attr);
		}
	}
	dev_info(&pdev->dev, "cif resume end");
	return 0;
}

static int cif_pm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	cvi_cif_resume(pdev);
	return 0;
}

static int cif_pm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	cvi_cif_suspend(pdev);
	return 0;
}

#endif

static SIMPLE_DEV_PM_OPS(cvi_cif_pm_ops, cif_pm_suspend,
				cif_pm_resume);

static const struct of_device_id cif_dt_match[] = {
	{.compatible = "cvitek,cif"},
	{}
};

#if (!DEVICE_FROM_DTS)
static void cif_pdev_release(struct device *dev)
{
}

static struct platform_device cif_pdev = {
	.name		= "cif",
	.dev.release	= cif_pdev_release,
};
#endif


static struct platform_driver cif_pdrv = {
	.probe      = cif_probe,
	.remove     = cif_remove,
	.driver     = {
		.name		= "cif",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cif_dt_match,
#endif
		.pm = &cvi_cif_pm_ops,
	},
};

static int proc_cif_open(struct inode *inode, struct file *file)
{
	struct cif_dev *dev = PDE_DATA(inode);

	return single_open(file, proc_cif_show, dev);
}

const struct proc_ops cif_proc_fops = {
	.proc_open		= proc_cif_open,
	.proc_read		= seq_read,
	.proc_write		= cif_proc_write,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};

static int __init _cif_init(void)
{
	int rc;

#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cif_pdrv);
#else
	rc = platform_device_register(&cif_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cif_pdrv);
	if (rc)
		platform_device_unregister(&cif_pdev);
#endif

	return rc;
}

static void __exit cif_exit(void)
{
	platform_driver_unregister(&cif_pdrv);
#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cif_pdev);
#endif
}

MODULE_DESCRIPTION("Cvitek Camera Interface Driver");
MODULE_AUTHOR("Saxen Ko");
MODULE_LICENSE("GPL");
module_init(_cif_init);
module_exit(cif_exit);