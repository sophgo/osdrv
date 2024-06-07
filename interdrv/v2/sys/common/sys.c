#include <linux/types.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/sys_uapi.h>
#include <linux/cvi_comm_sys.h>
#include <linux/cvi_defines.h>

#include "base_cb.h"
#include "vi_cb.h"
#include "vpss_cb.h"
#include "sys.h"
#include "sys_common.h"
#include "vi_sys.h"
#include "vo_sys.h"
#include "cdma.h"

#define CVI_SYS_DEV_NAME   "soph-sys"
#define CVI_SYS_CLASS_NAME "soph-sys"

struct cvi_sys_device {
	struct device *dev;
	struct miscdevice miscdev;
	struct mutex dev_lock;
	spinlock_t close_lock;
	atomic_t sys_inited;
	VI_VPSS_MODE_S vivpss_mode;
	int cdma_irq_num;
	int use_count;
};

static irqreturn_t cdma_isr(int irq, void *data)
{
	cdma_irq_handler();

	return IRQ_HANDLED;
}

static int _sys_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_SYS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static long cvi_sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct cvi_sys_device *dev = filp->private_data;

	switch (cmd) {
	case SYS_IOC_SET_INIT: {
		atomic_set(&dev->sys_inited, 1);
		break;
	}
	case SYS_IOC_GET_INIT: {
		unsigned int value = 0;

		value = atomic_read(&dev->sys_inited);
		if (copy_to_user((uint32_t *)arg, &value, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_SET_VIVPSSMODE: {
		if (copy_from_user(&dev->vivpss_mode, (void __user *)arg, sizeof(VI_VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_SET_VIVPSSMODE, copy_from_user failed.\n");
			return -EFAULT;
		}

		if (_sys_call_cb(E_MODULE_VI, VI_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			pr_err("VI_CB_SET_VIVPSSMODE failed\n");
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			pr_err("VPSS_CB_SET_VIVPSSMODE failed\n");
		}
		break;
	}
	case SYS_IOC_GET_VIVPSSMODE: {
		if (copy_to_user((void __user *)arg, &dev->vivpss_mode, sizeof(VI_VPSS_MODE_S)) != 0) {
			pr_err("SYS_IOCTL_GET_VIVPSSMODE, copy_to_user failed.\n");
			return -EFAULT;
		}
		break;
	}
	case SYS_IOC_READ_CHIP_ID: {
		unsigned int chip_id = 0;

		chip_id = sys_comm_read_chip_id();
		if (copy_to_user((void __user *)arg, &chip_id, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_READ_CHIP_VERSION: {
		unsigned int chip_version = 0;

		chip_version = sys_comm_read_chip_version();
		if (copy_to_user((void __user *)arg, &chip_version, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_READ_CHIP_PWR_ON_REASON: {
		unsigned int reason = 0;

		reason = sys_comm_read_chip_pwr_on_reason();
		if (copy_to_user((void __user *)arg, &reason, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_CDMA_COPY: {
		struct sys_cdma_copy cdma_cfg;
		struct cdma_1d_param param1d;

		if (copy_from_user(&cdma_cfg, (void __user *)arg, sizeof(cdma_cfg)) != 0) {
			pr_err("SYS_IOC_CDMA_COPY, copy_from_user failed.\n");
			return -EFAULT;
		}
		param1d.data_type = CDMA_DATA_TYPE_8BIT;
		param1d.src_addr = cdma_cfg.u64PhySrc;
		param1d.dst_addr = cdma_cfg.u64PhyDst;
		param1d.len = cdma_cfg.u32Len;

		ret = cvi_cdma_copy1d(&param1d);
		break;
	}
	case SYS_IOC_CDMA_COPY2D: {
		CVI_CDMA_2D_S stCdma2D;
		struct cdma_2d_param param2d;

		if (copy_from_user(&stCdma2D, (void __user *)arg, sizeof(stCdma2D)) != 0) {
			pr_err("SYS_IOC_CDMA_COPY, copy_from_user failed.\n");
			return -EFAULT;
		}
		param2d.data_type = CDMA_DATA_TYPE_8BIT;
		param2d.src_addr = stCdma2D.u64PhyAddrSrc;
		param2d.dst_addr = stCdma2D.u64PhyAddrDst;
		param2d.width = stCdma2D.u16Width;
		param2d.height = stCdma2D.u16Height;
		param2d.src_stride = stCdma2D.u16StrideSrc;
		param2d.dst_stride = stCdma2D.u16StrideDst;
		param2d.isFixed = stCdma2D.bEnableFixed;
		param2d.fixed_value = stCdma2D.u16FixedValue;

		ret = cvi_cdma_copy2d(&param2d);
		break;
	}
	default:
		return -ENOTTY;
	}
	return ret;
}

static int cvi_sys_open(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = container_of(filp->private_data, struct cvi_sys_device, miscdev);

	spin_lock(&ndev->close_lock);
	ndev->use_count++;
	spin_unlock(&ndev->close_lock);
	filp->private_data = ndev;
	return 0;
}

static int cvi_sys_close(struct inode *inode, struct file *filp)
{
	struct cvi_sys_device *ndev = filp->private_data;
	int cnt = 0;

	spin_lock(&ndev->close_lock);
	cnt = --ndev->use_count;
	spin_unlock(&ndev->close_lock);

	if (cnt == 0) {
		atomic_set(&ndev->sys_inited, 0);
	}

	filp->private_data = NULL;
	return 0;
}

static const struct file_operations cvi_sys_fops = {
	.owner = THIS_MODULE,
	.open = cvi_sys_open,
	.release = cvi_sys_close,
	.unlocked_ioctl = cvi_sys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_ptr_ioctl,
#endif
};

int cvi_sys_register_misc(struct cvi_sys_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = CVI_SYS_DEV_NAME;
	ndev->miscdev.fops = &cvi_sys_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "cvi_sys: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int sys_init_resources(struct platform_device *pdev, struct cvi_sys_device *ndev)
{
	int32_t ret;
	void __iomem *reg_base;
	struct resource *res;
	char *irq_name = "vi_cdma";
	int irq_num;

	/* Get vi_sys base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	pr_info("vi_sys res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}
	vi_sys_set_base_addr(reg_base);

	vi_sys_set_offline(VI_SYS_AXI_BUS_VPSS0, true);
	vi_sys_set_offline(VI_SYS_AXI_BUS_VPSS1, true);
	vi_sys_set_offline(VI_SYS_AXI_BUS_VPSS2, true);
	vi_sys_set_offline(VI_SYS_AXI_BUS_VPSS3, true);
	vi_sys_set_offline(VI_SYS_AXI_BUS_ISP_RAW, true);
	vi_sys_set_offline(VI_SYS_AXI_BUS_ISP_YUV, true);
	//VI_SYS_NORM_CLK_RATIO_CONFIG(VAL_VPSS0, 0x1f);


	/* Get cdma base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	pr_info("vi-cdma res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}
	cdma_set_base_addr(reg_base);


	/* Get vo_sys base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	pr_info("vo_sys res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}

	vo_sys_set_base_addr(reg_base);

	vo_sys_set_offline(VO_SYS_AXI_BUS_VPSS0, true);
	vo_sys_set_offline(VO_SYS_AXI_BUS_VPSS1, true);
	VO_SYS_CLK_RATIO_CONFIG(VPSS0, 0x10); //set full-speed clock
	VO_SYS_CLK_RATIO_CONFIG(VPSS1, 0x10);


	/* Get cdma irq num */
	irq_num = platform_get_irq_byname(pdev, irq_name);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", irq_name);
		return -ENODEV;
	}
	pr_info("irq(%d) for %s get from platform driver.\n", irq_num, irq_name);
	ndev->cdma_irq_num = irq_num;

	return 0;
}

static int cvi_sys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_sys_device *ndev;
	int32_t ret;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	// get hw-resources
	ret = sys_init_resources(pdev, ndev);
	if (ret) {
		pr_err("Failed to sys init resource\n");
		return ret;
	}

	ret = cvi_sys_register_misc(ndev);
	if (ret < 0) {
		pr_err("register misc error\n");
		return ret;
	}

	if (devm_request_irq(&pdev->dev, ndev->cdma_irq_num, cdma_isr, IRQF_SHARED,
		"vi_cdma", ndev)) {
		dev_err(&pdev->dev, "Unable to request cdma IRQ(%d)\n", ndev->cdma_irq_num);
		misc_deregister(&ndev->miscdev);
		return -ENODEV;
	}

	platform_set_drvdata(pdev, ndev);
	sys_comm_init();

	pr_info("sys probe done\n");
	return 0;
}

static int cvi_sys_remove(struct platform_device *pdev)
{
	struct cvi_sys_device *ndev = platform_get_drvdata(pdev);

	sys_comm_deinit();

	devm_free_irq(&pdev->dev, ndev->cdma_irq_num, ndev);
	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id cvitek_sys_match[] = {
	{ .compatible = "cvitek,sys" },
	{},
};
MODULE_DEVICE_TABLE(of, cvitek_sys_match);

static struct platform_driver cvitek_sys_driver = {
	.probe = cvi_sys_probe,
	.remove = cvi_sys_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = CVI_SYS_DEV_NAME,
			.of_match_table = cvitek_sys_match,
		},
};
module_platform_driver(cvitek_sys_driver);

MODULE_AUTHOR("Wellken Chen<wellken.chen@cvitek.com.tw>");
MODULE_DESCRIPTION("Cvitek SoC SYS driver");
MODULE_LICENSE("GPL");

