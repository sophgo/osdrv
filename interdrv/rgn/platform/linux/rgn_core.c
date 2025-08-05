#include <linux/compat.h>
#include "rgn_core.h"
#include "rgn_uapi.h"
#include "rgn_proc.h"

#define CVI_RGN_DEV_NAME            "soph-rgn"

unsigned int rgn_log_lv = RGN_WARN;
module_param(rgn_log_lv, int, 0644);

osal_atomic dev_open_cnt;

static long rgn_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev = file->private_data;
	struct rgn_ext_control p;

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct rgn_ext_control)))
		return ret;

	switch (cmd) {
	case RGN_IOC_S_CTRL:
		ret = _rgn_s_ctrl(rdev, &p);
		break;
	case RGN_IOC_G_CTRL:
		ret = _rgn_g_ctrl(rdev, &p);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct rgn_ext_control)))
		return ret;

	return ret;
}

static long rgn_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return rgn_ioctl(filp, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long rgn_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int rgn_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only open once
	if (!osal_atomic_read(&dev_open_cnt)) {
		struct rgn_dev *rdev =
		container_of(file->private_data, struct rgn_dev, miscdev);

		file->private_data = rdev;
		_rgn_sw_init(rdev);

		TRACE_RGN(RGN_INFO, "-\n");

		ret = 0;
	}

	osal_atomic_inc_return(&dev_open_cnt);

	return ret;
}

static int rgn_core_open(struct inode *inode, struct file *filp)
{
	return rgn_open(inode, filp);
}

int rgn_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	// only exit once
	if (osal_atomic_dec_return(&dev_open_cnt) == 0) {
		struct rgn_dev *rdev =
		container_of(file->private_data, struct rgn_dev, miscdev);

		/* This should move to stop streaming */
		_rgn_release_op(rdev);

		TRACE_RGN(RGN_INFO, "-\n");

		ret = 0;
	}

	if (osal_atomic_read(&dev_open_cnt) < 0)
		osal_atomic_set(&dev_open_cnt, 0);

	return ret;
}

static int rgn_core_release(struct inode *inode, struct file *filp)
{
	return rgn_release(inode, filp);
}

const struct file_operations rgn_fops = {
	.owner = THIS_MODULE,
	.open = rgn_core_open,
	.unlocked_ioctl = rgn_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = rgn_compat_ptr_ioctl,
#endif
	.release = rgn_core_release,
};

int rgn_core_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	return rgn_cb(dev, caller, cmd, arg);
}

static int rgn_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_RGN);
}

static int rgn_core_register_cb(struct rgn_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_RGN;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= rgn_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static int register_rgn_dev(struct device *dev, struct rgn_dev *rdev)
{
	int ret;

	rdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	rdev->miscdev.name = "soph-rgn";
	rdev->miscdev.fops = &rgn_fops;

	ret = misc_register(&rdev->miscdev);
	if (ret) {
		pr_err("rgn_dev: failed to register misc device.\n");
		return ret;
	}

	return ret;
}

static int _rgn_create_proc(struct rgn_dev *rdev)
{
	int ret = -EINVAL;

	if (rgn_proc_init() < 0) {
		TRACE_RGN(RGN_ERR, "rgn proc init failed\n");
		goto err;
	}
	ret = 0;

err:
	return ret;
}

int rgn_create_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	if (_rgn_create_proc(rdev)) {
		TRACE_RGN(RGN_ERR, "Failed to create proc\n");
		goto err;
	}

	ret = 0;

err:
	return ret;
}

static int rgn_probe(struct platform_device *pdev)
{
	struct rgn_dev *rdev;
	int ret = 0;

	rdev = devm_kzalloc(&pdev->dev, sizeof(*rdev), GFP_KERNEL);
	if (!rdev)
		return -ENOMEM;

	/* initialize locks */
	osal_spin_lock_init(&rdev->lock);
	osal_mutex_init(&rdev->mutex);

	dev_set_drvdata(&pdev->dev, rdev);

	ret = rgn_create_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	if (register_rgn_dev(&pdev->dev, rdev)) {
		TRACE_RGN(RGN_ERR, "Failed to register rgn-dev\n");
		goto err_create_instance;
	}

	if (rgn_core_register_cb(rdev)) {
		dev_err(&pdev->dev, "Failed to register rgn cb, err %d\n", ret);
		goto err_create_instance;
	}

	TRACE_RGN(RGN_WARN, "rgn probe done\n");
	return ret;

err_create_instance:
	misc_deregister(&rdev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	TRACE_RGN(RGN_INFO, "failed with rc(%d).\n", ret);
	return ret;
}

static void _rgn_destroy_proc(struct rgn_dev *rdev)
{
	rgn_proc_remove();
}

int rgn_destroy_instance(struct platform_device *pdev)
{
	int ret = -EINVAL;
	struct rgn_dev *rdev;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		TRACE_RGN(RGN_ERR, "invalid data\n");
		goto err;
	}

	_rgn_destroy_proc(rdev);
	ret = 0;

err:
	return ret;
}

static int rgn_remove(struct platform_device *pdev)
{
	struct rgn_dev *rdev = platform_get_drvdata(pdev);
	int ret = 0;

	ret = rgn_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	if (rgn_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm rgn cb, err %d\n", ret);
	}

	misc_deregister(&rdev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	TRACE_RGN(RGN_INFO, "%s -\n", __func__);

	return ret;
}

static const struct of_device_id rgn_dt_match[] = {
	{.compatible = "cvitek,rgn"},
	{},
};

MODULE_DEVICE_TABLE(of, rgn_dt_match);

static struct platform_driver rgn_core_driver = {
	.probe = rgn_probe,
	.remove = rgn_remove,
	.driver = {
		.name = CVI_RGN_DEV_NAME,
		.of_match_table = rgn_dt_match,
	},
};

module_platform_driver(rgn_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek rgn driver");
MODULE_LICENSE("GPL");
