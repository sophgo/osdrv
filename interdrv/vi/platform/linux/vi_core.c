#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/poll.h>
#include <linux/sched/signal.h>

#include "vi_core.h"
#include "base_cb.h"
#include "vi_common.h"
#include "vi_defines.h"
#include "vi.h"
#include "vi_ioctl.h"
#include "vi_ctx.h"
#include "proc/vi_proc.h"
#include "module/osal_clk.h"

#define VI_IRQ_NAME            "isp"
#define VI_CLASS_NAME          "soph-vi"
#define VI_DEV_NAME            "soph-vi"

/* control vi log level
 */
u32 vi_log_lv = VI_WARN;
osal_module_param(vi_log_lv, int, 0644);

/* vblanking period */
u32 patgen_vblanking = 112;
osal_module_param(patgen_vblanking, int, 0644);

/*csi_be->clk_sys_3, clk_raw->clk_sys_1, clk_isp_top->clk_sys_1*/
const char * const clk_isp_name[] = {
	"reg_clk_csi_be_vip_en"
};

/*clk_csi_mac0->clk_sys_3, clk_csi_mac1->clk_sys_2, clk_csi_mac2->clk_sys_0*/
const char * const clk_mac_name[] = {
	"reg_clk_csi_mac0_vip_en", "reg_clk_csi_mac1_vip_en", "reg_clk_csi_mac2_vip_en",
};

#ifdef PORTING_TEST
int vi_dump_reg;
#endif

static int _vi_clk_ctrl(struct platform_vi_dev *vi_dev, u8 enable)
{
	u8 i = 0;
	int rc = 0;
	struct vi_dev *vdev = &vi_dev->vdev;

	for (i = 0; i < ARRAY_SIZE(vdev->clk_isp); ++i) {
		if (vdev->clk_isp[i]) {
			if (enable) {
				osal_clk_prepare_enable(vdev->clk_isp[i]);
			} else {
				if (osal_clk_is_enabled(vdev->clk_isp[i]))
					osal_clk_disable_unprepare(vdev->clk_isp[i]);
				else
					osal_clk_unprepare(vdev->clk_isp[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_isp(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	for (i = 0; i < ARRAY_SIZE(vdev->clk_mac); ++i) {
		if (vdev->clk_mac[i]) {
			if (enable) {
				osal_clk_prepare_enable(vdev->clk_mac[i]);
			} else {
				if (osal_clk_is_enabled(vdev->clk_mac[i]))
					osal_clk_disable_unprepare(vdev->clk_mac[i]);
				else
					osal_clk_unprepare(vdev->clk_mac[i]);
			}
		} else {
			vi_pr(VI_ERR, "clk_mac(%d) is null\n", i);
			rc = -EAGAIN;
			goto EXIT;
		}
	}

	//Set axi_isp_top_clk_en 1
	//vi_sys_reg_write_mask(VI_SYS_REG_CLK_AXI_ISP_TOP_EN,
	//			VI_SYS_REG_CLK_AXI_ISP_TOP_EN_MASK,
	//			0x1 << VI_SYS_REG_CLK_AXI_ISP_TOP_EN_OFFSET);
EXIT:
	return rc;
}

int vi_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct platform_vi_dev *vi_dev;

	vi_dev = container_of(inode->i_cdev, struct platform_vi_dev, cdev);
	file->private_data = vi_dev;

	if (!atomic_read(&vi_dev->dev_open_cnt)) {

		_vi_clk_ctrl(vi_dev, true);

		vi_sw_reset(&vi_dev->vdev);

		vi_pr(VI_INFO, "-\n");
	}

	atomic_inc(&vi_dev->dev_open_cnt);

	return ret;
}

int vi_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct platform_vi_dev *vi_dev;

	vi_dev = container_of(inode->i_cdev, struct platform_vi_dev, cdev);

	atomic_dec(&vi_dev->dev_open_cnt);

	if (!atomic_read(&vi_dev->dev_open_cnt)) {

		vi_sdk_release(&vi_dev->vdev);

		_vi_clk_ctrl(vi_dev, false);

		vi_pr(VI_INFO, "-\n");
	}

	return ret;
}

int vi_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct platform_vi_dev *vi_dev = file->private_data;
	struct vi_dev *vdev = &vi_dev->vdev;
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos;

	pos = vdev->shared_mem;

	if ((vm_size + offset) > VI_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		vi_pr(VI_DBG, "vi proc mmap vir(%p) phys(%#llx)\n",
		      pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

unsigned int vi_poll(struct file *file, struct poll_table_struct *wait)
{
	struct platform_vi_dev *dev = file->private_data;
	struct vi_dev *vdev = &dev->vdev;
	struct isp_event_q *event_q = &vdev->event_q;
	unsigned long req_events = poll_requested_events(wait);
	unsigned int res = 0;
	unsigned long flags;

	if (req_events & POLLPRI) {
		/*
		 * If event buf is not empty, then notify MW to DQ event.
		 * Otherwise poll_wait.
		 */
		osal_spin_lock_irqsave(&event_q->lock, &flags);
		if (!osal_list_empty(&event_q->list))
			res = POLLPRI;
		else if (vdev->isp_event_wait_q.wait)
			poll_wait(file, vdev->isp_event_wait_q.wait, wait);
		osal_spin_unlock_irqrestore(&event_q->lock, &flags);
	}

	if (req_events & POLLIN) {
		if (osal_atomic_read(&vdev->isp_dbg_flag)) {
			res = POLLIN | POLLRDNORM;
			osal_atomic_set(&vdev->isp_dbg_flag, 0);
		} else if (vdev->isp_event_wait_q.wait) {
			poll_wait(file, vdev->isp_dbg_wait_q.wait, wait);
		}
	}

	return res;
}

static int vi_get_user(void *arg, void *parg, unsigned int cmd)
{
	struct vi_ext_control *ext_ctrl = (struct vi_ext_control *)arg;
	struct vi_ctrl *ctrl = (struct vi_ctrl *)parg;
	void __user *user_ptr = NULL;
	int size = 0;
	int ret = 0;
	int value = 0;

	switch (cmd) {
	case VI_IOC_S_CTRL:
	case VI_IOC_G_CTRL: {
		user_ptr = ext_ctrl->size ? ext_ctrl->ptr : NULL;
		size = ext_ctrl->size;
		value = ext_ctrl->value;
		ctrl->reserved[0] = ext_ctrl->reserved[0];
		break;
	}
	case VI_IOC_SDK_CTRL: {
		user_ptr = ext_ctrl->sdk_cfg.size ? ext_ctrl->sdk_cfg.ptr : NULL;
		size = ext_ctrl->sdk_cfg.size;
		value = ext_ctrl->sdk_cfg.value;
		ctrl->reserved[1] = ext_ctrl->sdk_cfg.reserved[0];
		break;
	}
	default:
		break;
	}

	if (size) {
		ctrl->ptr = kvmalloc(size, OSAL_GFP_KERNEL);
		if (ctrl->ptr == NULL)
			return -ENOMEM;
		if (copy_from_user(ctrl->ptr, (void __user *)user_ptr, size)) {
			kvfree(ctrl->ptr);
			ctrl->ptr = NULL;
			return -EFAULT;
		}
	} else {
		ctrl->val = value;
	}

	ctrl->dev = ext_ctrl->sdk_cfg.dev;
	ctrl->pipe = ext_ctrl->sdk_cfg.pipe;
	ctrl->chn = ext_ctrl->sdk_cfg.chn;
	ctrl->size = size;
	ctrl->id = ext_ctrl->id;

	return ret;
}

static int vi_put_user(void *arg, void *parg, unsigned int cmd)
{
	struct vi_ext_control *ext_ctrl = (struct vi_ext_control *)arg;
	struct vi_ctrl *ctrl = (struct vi_ctrl *)parg;
	void __user *user_ptr = NULL;
	int size = 0;
	int ret = 0;

	switch (cmd) {
	case VI_IOC_S_CTRL:
	case VI_IOC_G_CTRL: {
		user_ptr = ext_ctrl->size ? ext_ctrl->ptr : NULL;
		size = ext_ctrl->size;
		ext_ctrl->value = ctrl->val;
		break;
	}
	case VI_IOC_SDK_CTRL: {
		user_ptr = ext_ctrl->sdk_cfg.size ? ext_ctrl->sdk_cfg.ptr : NULL;
		size = ext_ctrl->sdk_cfg.size;
		ext_ctrl->sdk_cfg.value = ctrl->val;
		break;
	}
	default:
		break;
	}

	if (size && ctrl->ptr) {
		if (copy_to_user((void __user *)user_ptr, ctrl->ptr, size))
			ret = -EFAULT;
		kvfree(ctrl->ptr);
		ctrl->ptr = NULL;
	}

	return ret;
}

static long vi_core_ioctl(struct file *file, u_int cmd, u_long arg)
{
	long	ret = 0;
	struct platform_vi_dev *dev = file->private_data;
	struct vi_dev *vdev = &dev->vdev;
	struct	vi_ext_control ext_ctrl;
	struct	vi_ctrl ctrl;

	memset(&ext_ctrl, 0, sizeof(struct vi_ext_control));
	memset(&ctrl, 0, sizeof(struct vi_ctrl));

	if (copy_from_user(&ext_ctrl, (void __user *)arg, sizeof(struct vi_ext_control))) {
		vi_pr(VI_ERR, "fail to copy from user\n");
		return -EINVAL;
	}

	ret = vi_get_user(&ext_ctrl, &ctrl, cmd);
	if (ret)
		goto out;

	ret = vi_ioctl(vdev, cmd, &ctrl);

	vi_put_user(&ext_ctrl, &ctrl, cmd);

out:
	if (copy_to_user((void __user *)arg, &ext_ctrl, sizeof(struct vi_ext_control))) {
		vi_pr(VI_ERR, "fail to copy to user\n");
		return -EINVAL;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long vi_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int vi_core_open(struct inode *inode, struct file *filp)
{
	return vi_open(inode, filp);
}

static int vi_core_release(struct inode *inode, struct file *filp)
{
	return vi_release(inode, filp);
}

static int vi_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return vi_mmap(filp, vm);
}

static unsigned int vi_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return vi_poll(filp, wait);
}

const struct file_operations vi_fops = {
	.owner = THIS_MODULE,
	.open = vi_core_open,
	.unlocked_ioctl = vi_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vi_compat_ptr_ioctl,
#endif
	.release = vi_core_release,
	.mmap = vi_core_mmap,
	.poll = vi_core_poll,
};

int vi_core_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg)
{
	return vi_cb(dev, caller, cmd, arg);
}

static int vi_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VI);
}

static int vi_core_register_cb(struct vi_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VI;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vi_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static irqreturn_t vi_core_isr(int irq, void *priv)
{
	struct vi_dev *vdev = priv;

	vi_irq_handler(vdev);

	return IRQ_HANDLED;
}

static int vi_core_register_cdev(struct platform_vi_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vi_class = class_create(THIS_MODULE, VI_CLASS_NAME);
	if (IS_ERR(dev->vi_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vi_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, VI_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "allocate chrdev failed\n");
		return err;
	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &vi_fops);

	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "add chrdev failed\n");
		return err;
	}

	dev_t = device_create(dev->vi_class, dev->dev, dev->cdev_id, NULL, "%s", VI_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		return err;
	}

	return err;
}

static int vi_core_clk_init(struct platform_vi_dev *vi_dev)
{
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		vi_dev->vdev.clk_isp[i] = osal_clk_get(vi_dev->dev, clk_isp_name[i]);
		if (vi_dev->vdev.clk_isp[i] == NULL) {
			dev_err(vi_dev->dev, "Cannot get clk for %s\n", clk_isp_name[i]);
			return PTR_ERR(vi_dev->vdev.clk_isp[i]);
		}
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		vi_dev->vdev.clk_mac[i] = osal_clk_get(vi_dev->dev, clk_mac_name[i]);
		if (vi_dev->vdev.clk_mac[i] == NULL) {
			dev_err(vi_dev->dev, "Cannot get clk for %s\n", clk_mac_name[i]);
			return PTR_ERR(vi_dev->vdev.clk_mac[i]);
		}
	}

	return 0;
}

static int vi_core_clk_deinit(struct platform_vi_dev *vi_dev)
{
	u8 i = 0;

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		osal_clk_put(vi_dev->dev, vi_dev->vdev.clk_isp[i]);
		vi_dev->vdev.clk_isp[i] = NULL;
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		osal_clk_put(vi_dev->dev, vi_dev->vdev.clk_mac[i]);
		vi_dev->vdev.clk_mac[i] = NULL;
	}

	return 0;
}

static int vi_core_probe(struct platform_device *pdev)
{
	struct platform_vi_dev *vi_dev;
	struct vi_dev *vdev;
	struct resource *res;
	int ret = 0;

	vi_dev = devm_kzalloc(&pdev->dev, sizeof(struct platform_vi_dev), GFP_KERNEL);
	if (!vi_dev)
		return -ENOMEM;

	vi_dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, vi_dev);

	vdev = &vi_dev->vdev;

	/* vi proc setup */
	vdev->shared_mem = osal_kzalloc(VI_SHARE_MEM_SIZE, OSAL_GFP_ATOMIC);
	if (!vdev->shared_mem) {
		dev_err(&pdev->dev, "Failed to allocate shared memory\n");
		ret = -ENOMEM;
		goto err_shared_mem;
	}

	/* IP register base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vdev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	vi_pr(VI_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
			res->start, res->end, vdev->reg_base);
	if (IS_ERR(vdev->reg_base)) {
		ret = PTR_ERR(vdev->reg_base);
		goto err_ioremap;
	}

	/* Interrupt */
	vdev->irq_num = platform_get_irq_byname(pdev, VI_IRQ_NAME);
	if (vdev->irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", VI_IRQ_NAME);
		goto err_get_irq;
	}
	vi_pr(VI_INFO, "irq(%d) for %s get from platform driver.\n",
			vdev->irq_num, VI_IRQ_NAME);

	ret = vi_core_clk_init(vi_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init clk, err %d\n", ret);
		goto err_clk_init;
	}

	ret = vi_core_register_cdev(vi_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register dev, err %d\n", ret);
		goto err_dev_register;
	}

	ret = vi_create_instance(vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	ret = vi_create_proc(vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create proc, err %d\n", ret);
		goto err_create_proc;
	}

	ret = devm_request_irq(&pdev->dev, vdev->irq_num, vi_core_isr, 0,
				pdev->name, vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq_num(%d) ret(%d)\n",
				vdev->irq_num, ret);
		ret = -EINVAL;
		goto err_req_irq;
	}

	ret = vi_core_register_cb(vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register vi cb, err %d\n", ret);
		goto err_register_cb;
	}

	vi_pr(VI_INFO, "isp registered as %s\n", VI_DEV_NAME);

	return ret;

err_register_cb:
	devm_free_irq(&pdev->dev, vdev->irq_num, vdev);
err_req_irq:
	vi_destroy_proc(vdev);
err_create_proc:
	vi_destroy_instance(vdev);
err_create_instance:
	device_destroy(vi_dev->vi_class, vi_dev->cdev_id);
	cdev_del(&vi_dev->cdev);
	unregister_chrdev_region(vi_dev->cdev_id, 1);
	class_destroy(vi_dev->vi_class);
err_dev_register:
	vi_core_clk_deinit(vi_dev);
err_clk_init:
err_get_irq:
	if (vdev->reg_base) {
		devm_iounmap(&pdev->dev, vdev->reg_base);
		vdev->reg_base = NULL;
	}
err_ioremap:
	if (vdev->shared_mem) {
		osal_kfree(vdev->shared_mem);
	}
err_shared_mem:
	devm_kfree(&pdev->dev, vi_dev);
	vi_pr(VI_INFO, "%s -\n", __func__);

	return ret;
}

static int vi_core_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct platform_vi_dev *vi_dev = dev_get_drvdata(&pdev->dev);
	struct vi_dev *vdev = &vi_dev->vdev;

	ret = vi_destroy_instance(vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vi_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vi cb, err %d\n", ret);
	}

	vi_destroy_proc(vdev);

	if (vdev->shared_mem) {
		osal_kfree(vdev->shared_mem);
		vdev->shared_mem = NULL;
	}

	device_destroy(vi_dev->vi_class, vi_dev->cdev_id);
	cdev_del(&vi_dev->cdev);
	unregister_chrdev_region(vi_dev->cdev_id, 1);
	class_destroy(vi_dev->vi_class);

	vi_core_clk_deinit(vi_dev);
	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	vi_pr(VI_INFO, "%s -\n", __func__);

	return ret;
}

static int vi_core_suspend(struct platform_device *pdev, pm_message_t state)
{
	int ret = 0;
	struct platform_vi_dev *dev = dev_get_drvdata(&pdev->dev);

	if (!dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	osal_atomic_set(&dev->vdev.state, E_STATE_SUSPEND);

	ret = vi_suspend(&dev->vdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to suspend vi, err %d\n", ret);
		goto err_suspend;
	}

	_vi_clk_ctrl(dev, false);

	vi_pr(VI_INFO, "-\n");

	return ret;
err_suspend:
	vi_resume(&dev->vdev);

	osal_atomic_set(&dev->vdev.state, E_STATE_DEFAULT);

	return 0;
}

static int vi_core_resume(struct platform_device *pdev)
{
	int ret = 0;
	struct platform_vi_dev *dev = dev_get_drvdata(&pdev->dev);
	struct vi_dev *vdev = NULL;

	if (!dev) {
		vi_pr(VI_ERR, "VI device is not initialized!\n");
		return OSAL_EINVAL;
	}

	vdev = &dev->vdev;
	_vi_clk_ctrl(dev, true);

	ret = vi_resume(vdev);
	if (ret) {
		vi_pr(VI_ERR, "Failed to resume vi, err %d\n", ret);
		goto err_resume;
	}

	vi_pr(VI_INFO, "-\n");

	osal_atomic_set(&dev->vdev.state, E_STATE_DEFAULT);

	return ret;

err_resume:
	return ret;
}

static const struct of_device_id vi_core_match[] = {
	{
		.compatible = "cvitek,vi",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, vi_core_match);

static struct platform_driver vi_core_driver = {
	.probe = vi_core_probe,
	.remove = vi_core_remove,
	.suspend = vi_core_suspend,
	.resume = vi_core_resume,
	.driver = {
		.name = VI_DEV_NAME,
		.of_match_table = vi_core_match,
	},
};

module_platform_driver(vi_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video input driver");
MODULE_LICENSE("GPL");
