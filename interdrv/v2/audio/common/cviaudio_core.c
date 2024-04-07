#include <cviaudio_core.h>
#include <cviaudio_interface.h>//internal api declaration
#include <cviaudio_ioctl_cmd.h>//share struct & cmd between userspace and kernel space

//static function declaration -------------------start
static int cviaudio_core_probe(struct platform_device *pdev);
static int cviaudio_core_remove(struct platform_device *pdev);
//static function declaration --------------------end


int cviaudio_dump_reg;
module_param(cviaudio_dump_reg, int, 0644);

static const struct of_device_id cviaudio_core_match_tbl[] = {
	{
		.compatible = "cvitek,audio",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, cviaudio_core_match_tbl);

static struct platform_driver cviaudio_core_driver = {
	.probe = cviaudio_core_probe,
	.remove = cviaudio_core_remove,
	.driver = {
		.name = CVIAUDIO_DRV_PLATFORM_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cviaudio_core_match_tbl,
	},
};



static long cviaudio_device_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return cviaudio_device_ioctl(filp, cmd, arg);
}


#ifdef CONFIG_COMPAT
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}

static long compat_device_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif


static int cviaudio_device_core_open(struct inode *inode, struct file *filp)
{
	return cviaudio_device_open(inode, filp);
}

static int cviaudio_device_core_release(struct inode *inode, struct file *filp)
{
	return cviaudio_device_release(inode, filp);
}


static int cviaudio_device_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return cviaudio_device_mmap(filp, vm);
}

static unsigned int cviaudio_device_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return cviaudio_device_poll(filp, wait);
}


const struct file_operations cviaudio_device_fops = {
	.owner = THIS_MODULE,
	.open = cviaudio_device_core_open,
	.unlocked_ioctl = cviaudio_device_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_device_ptr_ioctl,
#endif
	.release = cviaudio_device_core_release,
	.mmap = cviaudio_device_core_mmap,
	.poll = cviaudio_device_core_poll,
};


int cviaudio_core_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	return cviaudio_device_cb(dev, caller, cmd, arg);
}

static int cviaudio_core_register_cb(struct cviaudio_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VI;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= cviaudio_core_cb;
#if 0
	return base_reg_module_cb(&reg_cb);
#else
	return 0;
#endif
}


static int cviaudio_core_register_cdev(struct cviaudio_dev *adev)
{
	int err = 0;
	char devName[256];
	dev_t subDevice;

	adev->cviaudio_class = class_create(THIS_MODULE, CVIAUDIO_DRV_CLASS_NAME);
	if (IS_ERR(adev->cviaudio_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(adev->cviaudio_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&adev->cdev_id, 0, CVIAUDIO_DRV_MAX_ALL_NUM, CVIAUDIO_DRV_CLASS_NAME)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate major number\n");
		return err;
	}
	adev->cviaudio_major = MAJOR(adev->cdev_id);
	pr_err("SUCCESS alloc_chrdev_region major %d\n", adev->cviaudio_major);

	subDevice = MKDEV(adev->cviaudio_major, 1);
	cdev_init(&adev->cdev_ceo, &cviaudio_device_fops);
	adev->cdev_ceo.owner = THIS_MODULE;
	if (cdev_add(&adev->cdev_ceo, subDevice, 1) < 0) {
		pr_err("cound not allocate chrdev for cdev_ceo\n");
		return -EBUSY;
	}
	sprintf(devName, "%s", CVIAUDIO_CEO_DEV_NAME);
	device_create(adev->cviaudio_class, NULL, subDevice, NULL, "%s", devName);

	return err;

}

static int cviaudio_core_probe(struct platform_device *pdev)
{
	int ret = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct cviaudio_dev *adev;

	audio_pr(AUDIO_ERR, "Enter cviaudio_core_probe!\n");
	audio_pr(AUDIO_DBG, "[%s][%d]!\n", __func__, __LINE__);
	audio_pr(AUDIO_INFO, "[%s][%d]!\n", __func__, __LINE__);
	adev = devm_kzalloc(&pdev->dev, sizeof(*adev), GFP_KERNEL);
	if (!adev) {
		pr_err("out of memory in cviaudio_core_probe\n");
		return -ENOMEM;
	}
	memset(adev, 0, sizeof(*adev));
	adev->dev = dev;
	/*check match*/
	match = of_match_device(cviaudio_core_match_tbl, &pdev->dev);
	if (!match) {
		pr_err("of match device in cviaudio_core_probe\n");
		return -EINVAL;
	}
	adev->pdata = match->data;
	/* register device and create device nodes by channels*/
	ret = cviaudio_core_register_cdev(adev);
	if (ret < 0) {
		pr_err("adev failed to register dev, err[%d]\n", ret);
		unregister_chrdev_region(adev->cviaudio_major, CVIAUDIO_DRV_MAX_ALL_NUM);
		return -EINVAL;
	}
	/* register callback function: None*/
	cviaudio_core_register_cb(adev);
	platform_set_drvdata(pdev, adev);
	//ret = vi_create_instance(pdev);
	ret = cviaudio_create_instance(pdev);
	if (ret)
		pr_err("adev failure to create instance[%d]\n", ret);
	else
		pr_err("cviaudio_create_instance success!!\n");
	return ret;
}

static int cviaudio_core_remove(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	struct cviaudio_dev *adev = platform_get_drvdata(pdev);

	audio_pr(AUDIO_INFO, "[%s][%d]\n", __func__, __LINE__);
	//TODO: should destroy instance as probe create the instance
	//the destroy instance should written in the cviaudio_interface.c
	#if 0
	ret = vi_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}
	#endif
	if (adev->cviaudio_major <= 0) {
		pr_err("cviudio error cviaudio_core_remove error in major\n");
		return -1;
	}
	//do the device destroy
	pr_err("[%s][%d]\n", __func__, __LINE__);
	for (i = 0; i < CVIAUDIO_DRV_MAX_ALL_NUM; i++) {
		dev_t subDevice;

		subDevice = MKDEV(adev->cviaudio_major, i);
		cdev_del(&adev->cdev_ceo);
		device_destroy(adev->cviaudio_class, subDevice);
	}
	//class destroy and chrdev_region unregister
	class_destroy(adev->cviaudio_class);
	unregister_chrdev_region(adev->cviaudio_major, CVIAUDIO_DRV_MAX_ALL_NUM);
	adev->cviaudio_major = 0;
	pr_err("[%s][%d]\n", __func__, __LINE__);
	return ret;
}


module_platform_driver(cviaudio_core_driver);
MODULE_AUTHOR("CVITEK Audio Inc.");
MODULE_DESCRIPTION("Cvitek Audio driver");
MODULE_LICENSE("GPL");
