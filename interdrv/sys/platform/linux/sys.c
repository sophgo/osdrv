#include <linux/types.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/io.h>

#include "defines.h"
#include "sys_uapi.h"
#include "comm_sys.h"
#include "base_ctx.h"
#include "base_cb.h"
#include "vi_cb.h"
#include "vpss_cb.h"
#include "sys.h"
#include "sys_common.h"
#include "vi_sys.h"
#include "sys_debug.h"
#include "osal.h"

#define SYS_DEV_NAME   "soph-sys"
#define SYS_CLASS_NAME "soph-sys"

u32 sys_log_lv = DBG_WARN;

struct sys_device {
	struct device *dev;
	struct miscdevice miscdev;
	osal_mutex dev_lock;
	osal_atomic open_count;
	vi_vpss_mode_s vivpss_mode;
};

module_param(sys_log_lv, int, 0644);

static int _sys_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_SYS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static long sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct sys_device *dev = filp->private_data;

	switch (cmd) {
	case SYS_IOC_SET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);

		osal_mutex_lock(&dev->dev_lock);
		if (copy_from_user(&dev->vivpss_mode, (void __user *)arg, sizeof(vi_vpss_mode_s)) != 0) {
			osal_mutex_unlock(&dev->dev_lock);
			TRACE_SYS(DBG_ERR, "SYS_IOCTL_SET_VIVPSSMODE, copy_from_user failed.\n");
			return -EFAULT;
		}

		osal_mutex_unlock(&dev->dev_lock);

		if (_sys_call_cb(E_MODULE_VI, VI_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VI_CB_SET_VIVPSSMODE failed\n");
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VPSS_CB_SET_VIVPSSMODE failed\n");
		}

		break;
	}
	case SYS_IOC_GET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);

		osal_mutex_lock(&dev->dev_lock);
		if (copy_to_user((void __user *)arg, &dev->vivpss_mode, sizeof(vi_vpss_mode_s)) != 0) {
			osal_mutex_unlock(&dev->dev_lock);
			TRACE_SYS(DBG_ERR, "SYS_IOCTL_GET_VIVPSSMODE, copy_to_user failed.\n");
			return -EFAULT;
		}
		osal_mutex_unlock(&dev->dev_lock);
		break;
	}
	case SYS_IOC_READ_CHIP_ID: {
		unsigned int chip_id = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_id = sys_comm_read_chip_id();
		if (copy_to_user((void __user *)arg, &chip_id, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_READ_CHIP_VERSION: {
		unsigned int chip_version = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_version = sys_comm_read_chip_version();
		if (copy_to_user((void __user *)arg, &chip_version, sizeof(unsigned int)) != 0) {
			TRACE_SYS(DBG_ERR, "SYS_IOC_READ_CHIP_VERSION, copy_to_user failed.%d\n", chip_version);
			return -EFAULT;
		}
		break;
	}
	case SYS_IOC_READ_CHIP_PWR_ON_REASON: {
		unsigned int reason = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		reason = sys_comm_read_chip_pwr_on_reason();
		if (copy_to_user((void __user *)arg, &reason, sizeof(unsigned int))) {
			TRACE_SYS(DBG_ERR, "SYS_IOC_READ_CHIP_PWR_ON_REASON, copy_to_user failed.\n");
			return -EFAULT;
		}
		break;
	}
	default:
		TRACE_SYS(DBG_ERR, "cmd not find*****\n");
		return -ENOTTY;
	}

	return ret;
}

static int sys_open(struct inode *inode, struct file *filp)
{
	int cnt;
	struct sys_device *ndev = container_of(filp->private_data, struct sys_device, miscdev);

	if (!ndev) {
		TRACE_SYS(DBG_ERR, "cannot find sys private data\n");
		return -ENODEV;
	}

	filp->private_data = ndev;

	cnt = osal_atomic_inc_return(&ndev->open_count);
	if (cnt > 1) {
		TRACE_SYS(DBG_INFO, "sys_open: open %d times\n", cnt);
		return 0;
	}

	return 0;
}

static int sys_close(struct inode *inode, struct file *filp)
{
	int cnt = 0;
	struct sys_device *ndev = filp->private_data;

	filp->private_data = NULL;

	cnt = osal_atomic_dec_return(&ndev->open_count);
	if (cnt) {
		TRACE_SYS(DBG_INFO, "sys_close: open %d times\n", cnt);
		return 0;
	}

	return 0;
}

#ifdef CONFIG_COMPAT
static long sys_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations sys_fops = {
	.owner = THIS_MODULE,
	.open = sys_open,
	.release = sys_close,
	.unlocked_ioctl = sys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sys_compat_ptr_ioctl,
#endif
};

int sys_register_misc(struct sys_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = SYS_DEV_NAME;
	ndev->miscdev.fops = &sys_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		TRACE_SYS(DBG_ERR, "sys: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int sys_init_resources(struct platform_device *pdev, struct sys_device *ndev)
{
	int32_t ret, i;
	void __iomem *reg_base;
	struct resource *res = NULL;
	void __iomem *ap_base[2];
	struct resource *ap_res[2];
	char *ap_reg_name[2] = {"top_base", "rtc_base"};

	/* Get vip_sys base address */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "vi_sys");
	if (!res) {
		TRACE_SYS(DBG_ERR, "vi_sys no memory resource defined\n");
		return -ENODEV;
	}
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}

	TRACE_SYS(DBG_INFO, "vi_sys res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	vi_sys_set_base_addr(reg_base);

	/* Get ap sys base address */
	for (i = 0; i < 2; i++) {
		ap_res[i] = platform_get_resource_byname(pdev, IORESOURCE_MEM, ap_reg_name[i]);
		if (!ap_res[i]) {
			TRACE_SYS(DBG_ERR, "%s no memory resource defined\n", ap_reg_name[i]);
			return -ENODEV;
		}
		ap_base[i] = devm_ioremap(&pdev->dev, ap_res[i]->start, resource_size(ap_res[i]));
		if (IS_ERR(ap_base[i])) {
			ret = PTR_ERR(ap_base[i]);
			return ret;
		}
		TRACE_SYS(DBG_INFO, "%s res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
				ap_reg_name[i], ap_res[i]->start, ap_res[i]->end, ap_base[i]);
	}

	sys_comm_set_base_addr(ap_base[0], ap_base[1]);

	return 0;
}

static int sys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sys_device *ndev;
	int32_t ret;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	osal_mutex_init(&ndev->dev_lock);
	osal_atomic_set(&ndev->open_count, 0);

	// get hw-resources
	ret = sys_init_resources(pdev, ndev);
	if (ret) {
		TRACE_SYS(DBG_ERR, "Failed to sys init resource\n");
		return ret;
	}

	ret = sys_register_misc(ndev);
	if (ret < 0) {
		TRACE_SYS(DBG_ERR, "register misc error\n");
		return ret;
	}

	platform_set_drvdata(pdev, ndev);

	TRACE_SYS(DBG_WARN, "sys probe done\n");
	return 0;
}

static int sys_remove(struct platform_device *pdev)
{
	struct sys_device *ndev = platform_get_drvdata(pdev);

	vi_sys_set_base_addr(NULL);

	osal_mutex_destroy(&ndev->dev_lock);

	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id sys_match[] = {
	{ .compatible = "cvitek,sys" },
	{},
};
MODULE_DEVICE_TABLE(of, sys_match);

static struct platform_driver sys_driver = {
	.probe = sys_probe,
	.remove = sys_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = SYS_DEV_NAME,
			.of_match_table = sys_match,
		},
};
module_platform_driver(sys_driver);

MODULE_DESCRIPTION("Soph SoC SYS driver");
MODULE_LICENSE("GPL");
