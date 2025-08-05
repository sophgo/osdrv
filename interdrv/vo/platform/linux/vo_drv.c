#include <linux/module.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "vo_drv.h"
#include "vo_ctx.h"
#include "vo_process.h"
#include "base_cb.h"
#include "vo_debug.h"
#include "vo_platform.h"
#include "dsi_mac.h"
#include "dsi_phy.h"
#include "vo_mac.h"

#define VO_DEV_NAME            "soph-vo"
#define VO_CLASS_NAME          "soph-vo"

static const char *const clk_vo_name[] = {
	"clk_vo_disp", "clk_vo_mac",
};

static long vo_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	long ret = 0;

	ret = vo_ioctl(filp, cmd, arg);
	return ret;
}

static int vo_core_open(struct inode *inode, struct file *filp)
{
	return vo_open(inode, filp);
}

static int vo_core_release(struct inode *inode, struct file *filp)
{
	return vo_release(inode, filp);
}

static unsigned int vo_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return vo_poll(filp, wait);
}

#ifdef CONFIG_COMPAT
static long vo_core_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations vo_fops = {
	.owner = THIS_MODULE,
	.open = vo_core_open,
	.unlocked_ioctl = vo_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vo_core_compat_ptr_ioctl,
#endif
	.release = vo_core_release,
	.poll = vo_core_poll,
};

static int vo_core_register_cdev(struct vo_core_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vo_class = class_create(THIS_MODULE, VO_CLASS_NAME);
	if (IS_ERR(dev->vo_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vo_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, VO_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "allocate chrdev failed\n");
		goto alloc_chrdev_region_err;
	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &vo_fops);

	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "add chrdev failed\n");
		goto cdev_add_err;
	}

	dev_t = device_create(dev->vo_class, dev->dev, dev->cdev_id, NULL, "%s", VO_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		goto device_create_err;
	}

	return err;

device_create_err:
	cdev_del(&dev->cdev);
cdev_add_err:
	unregister_chrdev_region(dev->cdev_id, 1);
alloc_chrdev_region_err:
	class_destroy(dev->vo_class);
	return err;
}

static int vo_core_unregister_cdev(struct vo_core_dev *dev)
{
	device_destroy(dev->vo_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vo_class);
	dev_set_drvdata(dev->dev, NULL);

	return 0;
}

static int vo_core_clk_init(struct platform_device *pdev)
{
	struct vo_core_dev *dev;
	// unsigned char i = 0;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get vo drvdata\n");
		return -EINVAL;
	}

	// for (i = 0; i < ARRAY_SIZE(clk_vo_name); ++i) {
	//	dev->clk_vo[i] = devm_clk_get(&pdev->dev, clk_vo_name[i]);
	//	if (IS_ERR(dev->clk_vo[i])) {
	//		dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_vo_name[i]);
	//		return PTR_ERR(dev->clk_vo[i]);
	//	}
	//	clk_prepare_enable(dev->clk_vo[i]);
	// }

	return 0;
}

static int vo_core_resources_init(struct platform_device *pdev)
{
	int ret = 0, i;
	struct vo_core_dev *dev;
	struct resource *res;

	dev = dev_get_drvdata(&pdev->dev);
	/* IP register base address */

	for (i = 0; i < 4; ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		dev->reg_base[i] = devm_ioremap_resource(&pdev->dev, res);
		TRACE_VO(DBG_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			 res->start, res->end, dev->reg_base[i]);

		if (IS_ERR(dev->reg_base[i])) {
			ret = PTR_ERR(dev->reg_base[i]);
			return ret;
		}
	}

	for (i = 0; i < DISP_MAX_INST; ++i) {
		disp_set_disp_base_addr(i, dev->reg_base[i * 4]);
		dsi_mac_set_base_addr(i, dev->reg_base[i * 4 + 1]);
		dphy_set_base_addr(i, dev->reg_base[i * 4 + 2]);
		vo_mac_set_base_addr(i, dev->reg_base[i * 4 + 3]);
	}

	ret = vo_core_clk_init(pdev);

	return ret;
}

static int vo_core_probe(struct platform_device *pdev)
{
	struct vo_core_dev *dev;

	int ret = 0;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return -ENOMEM;
	}

	dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, dev);

	ret = vo_core_resources_init(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Get resources failed, err %d\n", ret);
		return ret;
	}

	ret = vo_core_register_cdev(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register dev, err %d\n", ret);
		return ret;
	}

	ret = vo_create_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto vo_create_instance_err;
	}

	ret = vo_core_register_cb(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register vo cb, err %d\n", ret);
		goto vo_core_register_cb_err;
	}

	return ret;

vo_core_register_cb_err:
	vo_destroy_instance(pdev);
vo_create_instance_err:
	vo_core_unregister_cdev(dev);
	return ret;
}

static int vo_core_remove(struct platform_device *pdev)
{
	int ret = 0;

	struct vo_core_dev *dev = dev_get_drvdata(&pdev->dev);

	ret = vo_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vo_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vo cb, err %d\n", ret);
	}

	// for (i = 0; i < ARRAY_SIZE(dev->clk_vo); ++i) {
	//	if ((dev->clk_vo[i]) && __clk_is_enabled(dev->clk_vo[i]))
	//		clk_disable_unprepare(dev->clk_vo[i]);
	// }

	device_destroy(dev->vo_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vo_class);
	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	TRACE_VO(DBG_INFO, "%s -\n", __func__);

	return ret;
}

#if defined(CONFIG_PM)
int vo_core_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = -1;
	vo_layer layer;
	vo_dev dev = 0;

	g_vo_ctx->suspend = true;

	for (layer = 0; layer < VO_MAX_VIDEO_LAYER_NUM; ++layer)
		if (g_vo_ctx->layer_ctx[layer].is_layer_enable) {
			ret = vo_destroy_thread(layer);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo destory thread\n");
			}
		}

	for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev)
		if (g_vo_ctx->dev_ctx[dev].is_dev_enable) {
			ret = vo_stop_streaming(dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo stop streaming\n");
			}
		}

	TRACE_VO(DBG_WARN, "vo suspended\n");

	return 0;
}

int vo_core_resume(struct platform_device *pdev)
{
	int ret = -1;
	vo_layer layer;
	vo_dev dev = 0;

	for (layer = 0; layer < VO_MAX_VIDEO_LAYER_NUM; ++layer)
		if (g_vo_ctx->layer_ctx[layer].is_layer_enable && g_vo_ctx->suspend) {
			ret = vo_create_thread(layer);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo create thread\n");
			}
		}

	for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev)
		if (g_vo_ctx->dev_ctx[dev].is_dev_enable && g_vo_ctx->suspend) {
			ret = vo_start_streaming(dev);
			if (ret) {
				TRACE_VO(DBG_ERR, "Failed to vo start streaming\n");
			}
		}

	g_vo_ctx->suspend = false;
	TRACE_VO(DBG_WARN, "vo resumed\n");

	return 0;
}
#endif

static const struct of_device_id vo_core_match[] = {
	{
		.compatible = "cvitek,vo",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, vo_core_match);

static struct platform_driver vo_core_driver = {
	.probe = vo_core_probe,
	.remove = vo_core_remove,
	.driver = {
		.name = VO_DEV_NAME,
		.of_match_table = vo_core_match,
	},
#if defined(CONFIG_PM)
	.suspend = vo_core_suspend,
	.resume = vo_core_resume,
#endif
};

module_platform_driver(vo_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video output driver");
MODULE_LICENSE("GPL");
