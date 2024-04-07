#include <vo_core.h>
#include <base_cb.h>

#define CVI_VO_DEV_NAME            "soph-vo"
#define CVI_VO_CLASS_NAME          "soph-vo"

static const char *const disp_irq_name[DISP_MAX_INST] = {"disp0", "disp1"};
static const char *const clk_sys_name[] = {
	"clk_vo_mipimpll0", "clk_vo_mipimpll1"
};
static const char *const clk_vo_name[] = {
	"clk_vo_disp0", "clk_vo_mac0", "clk_vo_dsi_mac0",
	"clk_vo_disp1", "clk_vo_mac1", "clk_vo_dsi_mac1"
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

const struct file_operations vo_fops = {
	.owner = THIS_MODULE,
	.open = vo_core_open,
	.unlocked_ioctl = vo_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vo_core_ioctl,
#endif
	.release = vo_core_release,
	.poll = vo_core_poll,
};

int vo_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	return vo_cb(dev, caller, cmd, arg);
}

static int vo_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VO);
}

static int vo_core_register_cb(struct cvi_vo_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VO;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vo_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static int vo_core_register_cdev(struct cvi_vo_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vo_class = class_create(THIS_MODULE, CVI_VO_CLASS_NAME);
	if (IS_ERR(dev->vo_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vo_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, CVI_VO_DEV_NAME)) < 0) {
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

	dev_t = device_create(dev->vo_class, dev->dev, dev->cdev_id, NULL, "%s", CVI_VO_DEV_NAME);
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

static int vo_core_unregister_cdev(struct cvi_vo_dev *dev)
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
	struct cvi_vo_dev *dev;
	u8 i = 0;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get vo drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		dev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(dev->clk_sys[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_sys_name[i]);
			return PTR_ERR(dev->clk_sys[i]);
		}
		clk_prepare_enable(dev->clk_sys[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_vo_name); ++i) {
		dev->clk_vo[i] = devm_clk_get(&pdev->dev, clk_vo_name[i]);
		if (IS_ERR(dev->clk_vo[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_vo_name[i]);
			return PTR_ERR(dev->clk_vo[i]);
		}
		clk_prepare_enable(dev->clk_vo[i]);
	}

	return 0;
}

static int _init_resources(struct platform_device *pdev)
{
	int ret = 0, i;
	struct cvi_vo_dev *dev;
	struct resource *res;

	dev = dev_get_drvdata(&pdev->dev);
	/* IP register base address */

	for (i = 0; i < 10; ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		dev->reg_base[i] = devm_ioremap_resource(&pdev->dev, res);
		CVI_TRACE_VO(CVI_DBG_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
				res->start, res->end, dev->reg_base[i]);

		if (IS_ERR(dev->reg_base[i])) {
			ret = PTR_ERR(dev->reg_base[i]);
			return ret;
		}
	}

	for (i = 0; i < DISP_MAX_INST; ++i) {
		disp_set_vo_mac_base_addr(i, dev->reg_base[i * 5]);
		disp_set_disp_base_addr(i, dev->reg_base[i * 5 + 1]);
		disp_set_dsi_mac_base_addr(i, dev->reg_base[i * 5 + 2]);
		dphy_set_base_addr(i, dev->reg_base[i * 5 + 3]);
		disp_set_oenc_base_addr(i, dev->reg_base[i * 5 + 4]);
	}

	for (i = 0; i < DISP_MAX_INST; ++i) {
		/* Interrupts */
		dev->vo_core[i].irq_num = platform_get_irq_byname(pdev, disp_irq_name[i]);
		if (dev->vo_core[i].irq_num < 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n",  disp_irq_name[i]);
			return -ENOENT;
		}
		dev_info(&pdev->dev, "irq(%d) for %s get from platform driver.\n",
				dev->vo_core[i].irq_num,  disp_irq_name[i]);
	}

	ret = vo_core_clk_init(pdev);

	return ret;
}

static void _init_parameters(struct cvi_vo_dev *dev)
{
	int i;
	struct cvi_vo_core *pCore;

	for (i = 0; i < DISP_MAX_INST; ++i) {
		pCore = &dev->vo_core[i];

		pCore->core_id = i;
		pCore->disp_online = false;
	}
}

static int vo_core_probe(struct platform_device *pdev)
{
	struct cvi_vo_dev *dev;
	int ret = 0;
	int i;

	/* in alios, dev should be free */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		dev_err(&pdev->dev, "devm_kzalloc failed!\n");
		return -ENOMEM;
	}

	dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, dev);

	// get resources
	ret = _init_resources(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Get resources failed, err %d\n", ret);
		return ret;
	}
	_init_parameters(dev);
	disp_ctrl_init(false);

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

	for (i = 0; i < DISP_MAX_INST; ++i) {
		ret = devm_request_irq(&pdev->dev, dev->vo_core[i].irq_num, vo_irq_handler, 0,
					disp_irq_name[i], (void *)&dev->vo_core[i]);
		if (ret) {
			dev_err(&pdev->dev, "Failed to request irq_num(%d) ret(%d)\n",
					dev->vo_core[i].irq_num, ret);
			ret = -EINVAL;
			goto err_req_irq;
		}
	}

	ret = vo_core_register_cb(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register vo cb, err %d\n", ret);
		goto vo_core_register_cb_err;
	}

	return ret;

err_req_irq:
	for (--i; i >= 0; --i)
		devm_free_irq(&pdev->dev, dev->vo_core[i].irq_num, (void *)&dev->vo_core[i]);
vo_core_register_cb_err:
	vo_destroy_instance(pdev);
vo_create_instance_err:
	vo_core_unregister_cdev(dev);
	return ret;
}

static int vo_core_remove(struct platform_device *pdev)
{
	int ret = 0, i;

	struct cvi_vo_dev *dev = dev_get_drvdata(&pdev->dev);

	ret = vo_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vo_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vo cb, err %d\n", ret);
	}

	for (i = 0; i < ARRAY_SIZE(dev->clk_sys); ++i) {
		if ((dev->clk_sys[i]) && __clk_is_enabled(dev->clk_sys[i]))
			clk_disable_unprepare(dev->clk_sys[i]);
	}
	for (i = 0; i < ARRAY_SIZE(dev->clk_vo); ++i) {
		if ((dev->clk_vo[i]) && __clk_is_enabled(dev->clk_vo[i]))
			clk_disable_unprepare(dev->clk_vo[i]);
	}

	device_destroy(dev->vo_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vo_class);
	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	CVI_TRACE_VO(CVI_DBG_INFO, "%s -\n", __func__);

	return ret;
}


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
		.name = CVI_VO_DEV_NAME,
		.of_match_table = vo_core_match,
	},
};

module_platform_driver(vo_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video output driver");
MODULE_LICENSE("GPL");
