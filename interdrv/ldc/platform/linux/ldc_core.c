#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/compat.h>

#include "comm_video.h"
#include "ldc_uapi.h"
#include "base_cb.h"
#include "vi_sys.h"
#include "ldc_proc.h"
#include "ldc_debug.h"
#include "ldc_proc.h"
#include "ldc_sdk.h"
#include "ldc.h"
#include "ldc_comm_layer.h"

#define LDC_CLASS_NAME "soph-ldc"
#define LDC_DEV_NAME "soph-ldc"
#define LDC_CLK_NAME "ldc_clk"
#define LDC_CLK_SYS_NAME "clk_sys_3"
#define LDC0_REG_NAME "ldc0"
#define LDC1_REG_NAME "ldc1"
#define LDC2_REG_NAME "dwa0"
#define LDC3_REG_NAME "dwa1"
#define LDC0_INTR_NAME "ldc0"
#define LDC1_INTR_NAME "ldc1"
#define LDC2_INTR_NAME "dwa0"
#define LDC3_INTR_NAME "dwa1"

#define LDC_SHARE_MEM_SIZE (0x8000)

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#ifndef CONFIG_PROC_FS
#define CONFIG_PROC_FS 1
#endif

#ifndef CONFIG_PM_SLEEP
#define CONFIG_PM_SLEEP 1
#endif

static osal_atomic ldc_open_count;

static struct ldc_vdev *ldc_dev;

unsigned int ldc_log_lv = DBG_WARN;
bool ldc_dump_reg;
module_param(ldc_log_lv, int, 0644);
MODULE_PARM_DESC(ldc_log_lv, "LDC Debug Log Level");

static unsigned int clk_sys_freq[LDC_DEV_MAX_CNT];
static unsigned int core_cnt;
module_param_array(clk_sys_freq, int, &core_cnt, 0644);
MODULE_PARM_DESC(clk_sys_freq, "clk_sys_freq setting by user");

module_param(ldc_dump_reg, bool, 0644);
MODULE_PARM_DESC(ldc_dump_reg, "ldc need dump reg");

static int ldc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ldc_vdev *wdev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = wdev->ctx.shared_mem;

	if ((vm_size + offset) > LDC_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE,
				    vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("ldc proc mmap vir(%p) phys(%#llx)\n", pos,
			 (unsigned long long)virt_to_phys((void *)pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long ldc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct ldc_vdev *wdev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, drv_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(cmd);
	out_size = _IOC_SIZE(cmd);
	in_size = out_size;
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size))
		TRACE_LDC(DBG_ERR, "access_ok failed\n");
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size))
		TRACE_LDC(DBG_ERR, "access_ok failed\n");
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		TRACE_LDC(DBG_ERR, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	//TRACE_LDC(DBG_ERR, "ksize-in_size-cmd[%d-%d-%d]\n", ksize, in_size, cmd);
	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		osal_memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case LDC_BEGIN_JOB: {
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_BEGIN_JOB\n");
		CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

		ret = ldc_begin_job(&wdev->ctx, data);
		break;
	}
	case LDC_END_JOB: {
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_END_JOB, handle=0x%llx\n",
			      (unsigned long long)data->handle);
		CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

		ret = ldc_end_job(&wdev->ctx, data->handle);
		break;
	}
	case LDC_CANCEL_JOB: {
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_CANCLE_JOB, handle=0x%llx\n",
			      (unsigned long long)data->handle);
		CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

		ret = ldc_cancel_job(&wdev->ctx, data->handle);
		break;
	}
	case LDC_GET_WORK_JOB: {
		struct gdc_handle_data *data = (struct gdc_handle_data *)kdata;

		CHECK_IOCTL_CMD(cmd, struct gdc_handle_data);

		ret = ldc_get_work_job(&wdev->ctx, data);
		TRACE_LDC(DBG_DEBUG, "LDC_GET_WORK_JOB, handle=0x%llx\n",
			      (unsigned long long)data->handle);
		break;
	}
	case LDC_ADD_ROT_TASK: {
		struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_ADD_ROT_TASK, handle=0x%llx\n",
			      (unsigned long long)attr->handle);
		CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

		ret = ldc_add_rotation_task(&wdev->ctx, attr);
		break;
	}
	case LDC_ADD_LDC_TASK: {
		struct gdc_task_attr *attr = (struct gdc_task_attr *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_ADD_LDC_TASK, handle=0x%llx\n",
			      (unsigned long long)attr->handle);
		CHECK_IOCTL_CMD(cmd, struct gdc_task_attr);

		ret = ldc_add_ldc_task(&wdev->ctx, attr);
		break;
	}
	case LDC_SET_JOB_IDENTITY: {
		struct gdc_identity_attr *identity = (struct gdc_identity_attr *)kdata;

		TRACE_LDC(DBG_DEBUG, "LDC_SET_JOB_IDENTITY, handle=0x%llx\n",
				  (unsigned long long)identity->handle);
		CHECK_IOCTL_CMD(cmd, struct gdc_identity_attr);

		ret = ldc_set_identity(&wdev->ctx, identity);
		break;
	}
	case LDC_GET_CHN_FRM: {
		struct gdc_chn_frm_cfg *cfg = (struct gdc_chn_frm_cfg *)kdata;
		video_frame_info_s *video_frame = &cfg->video_frame;
		struct gdc_identity_attr *identity = &cfg->identity;
		int milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct gdc_chn_frm_cfg);

		ret = ldc_get_chn_frame(&wdev->ctx, identity, video_frame, milli_sec);
		break;
	}
	case LDC_ATTACH_VB_POOL: {
		struct ldc_vb_pool_cfg *cfg = (struct ldc_vb_pool_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct ldc_vb_pool_cfg);

		ret = ldc_attach_vb_pool(cfg);
		break;
	}
	case LDC_DETACH_VB_POOL: {
		struct ldc_vb_pool_cfg *cfg = (struct ldc_vb_pool_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct ldc_vb_pool_cfg);

		ret = ldc_detach_vb_pool(cfg);
		break;
	}
	case LDC_SUSPEND: {
		ret = ldc_suspend();
		break;
	}
	case LDC_RESUME: {
		ret = ldc_resume();
		break;
	}
	case LDC_GET_INTER_CHN_ATTR: {
		struct ldc_internal_chn_attr *attr = (struct ldc_internal_chn_attr *)kdata;

		CHECK_IOCTL_CMD(cmd, struct ldc_internal_chn_attr);

		ret = ldc_get_internal_chn_attr(&wdev->ctx, attr);
		break;
	}

	case CVI_LDC_SET_CHN_LDC_CFG: {
		struct ldc_internal_chn_ldc_cfg *cfg = (struct ldc_internal_chn_ldc_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct ldc_internal_chn_ldc_cfg);

		ret = ldc_set_internal_chn_ldc_cfg(&wdev->ctx, cfg);
		break;
	}

	case LDC_INIT:
		break;
	case LDC_DEINIT:
		break;
	default:
		ret = -ENOTTY;
		goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);
	//TRACE_LDC(DBG_ERR, "kdata-stack_kdata[%px-%px]\n", kdata, stack_kdata);

	return ret;
}

static int ldc_open(struct inode *inode, struct file *filp)
{
	struct ldc_vdev *dev =
		container_of(filp->private_data, struct ldc_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("cannot find ldc private data\n");
		return -ENODEV;
	}

	i = osal_atomic_inc_return(&ldc_open_count);
	if (i > 1) {
		pr_info("ldc_open: open %d times\n", i);
		return 0;
	}

	TRACE_LDC(DBG_INFO, "ldc_open\n");

	return 0;
}

static int ldc_release(struct inode *inode, struct file *filp)
{
	struct ldc_vdev *dev
		= container_of(filp->private_data, struct ldc_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("Cannot find stitch private data\n");
		return -ENODEV;
	}

	i = osal_atomic_dec_return(&ldc_open_count);
	if (i) {
		pr_info("ldc_close: open %d times\n", i);
		return 0;
	}

	TRACE_LDC(DBG_INFO, "ldc_release.\n");
	return 0;
}

static int ldc_drv_fasync(int fd, struct file *file, int on)
{
#if 0
	if (fasync_helper(fd, file, on, &ldc_fasync) >= 0)
		return 0;
	else
		return -EIO;
#endif
	return 0;
}

#ifdef CONFIG_COMPAT
static long ldc_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations ldc_fops = {
	.owner = THIS_MODULE,
	.open = ldc_open,
	.release = ldc_release,
	.mmap = ldc_mmap,
	.unlocked_ioctl = ldc_ioctl,
	.fasync  = ldc_drv_fasync,
#ifdef CONFIG_COMPAT
	.compat_ioctl = ldc_compat_ptr_ioctl,
#endif
};

static void ldc_tsk_finish(struct ldc_ctx *ctx, int top_id)
{
	struct ldc_core *core = &ctx->core[top_id];
	ldc_wkup_frm_done_work(core);
}


static void ldc_irq_handler(unsigned char intr_status, struct ldc_ctx *ctx, int top_id, struct ldc_job *done_job)
{
	if (!ctx || !done_job)
		return;

	if (top_id < 0 || top_id >= LDC_DEV_MAX_CNT) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev_%d\n", top_id);
		return;
	}

	if (done_job->use_cmdq) {
		TRACE_LDC(DBG_DEBUG, "core(%d) cmdq status(%#x)\n", top_id, intr_status);
	} else {
		if (!(intr_status & BIT(0))) {
			TRACE_LDC(DBG_ERR, "core(%d) axi read err, status(%#x)\n", top_id, intr_status);
			return;
		}
	}

	osal_atomic_set(&ctx->core[top_id].state, LDC_CORE_STATE_END);
	TRACE_LDC(DBG_INFO, "core(%d) state set(%d)\n", top_id, LDC_CORE_STATE_END);

	ldc_tsk_finish(ctx, top_id);
}

static irqreturn_t ldc_isr(int irq, void *data)
{
	struct ldc_ctx *ctx = (struct ldc_ctx *)data;
	struct ldc_core *core;
	int top_id, i;
	unsigned char intr_status;
	struct ldc_job *done_job;

	if (!ctx) {
		TRACE_LDC(DBG_ERR, "invalid ldc_dev\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < ctx->core_num; i++) {
		core = &ctx->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			osal_spin_lock(&core->core_lock);
			done_job = osal_list_first_entry(&core->list, struct ldc_job, node);
			osal_spin_unlock(&core->core_lock);

			if (unlikely(!done_job)) {
				TRACE_LDC(DBG_NOTICE, "null done job\n");
				goto CMDQ_STATUS_UNKOWN;
			}

			if (unlikely(done_job->coreid != top_id)) {
				TRACE_LDC(DBG_NOTICE, "done job core[%d] not match with [%d]\n"
					, done_job->coreid, top_id);
				goto CMDQ_STATUS_UNKOWN;
			}

			if (done_job->use_cmdq) {
				intr_status = ldc_cmdq_intr_status(top_id);
				if (intr_status) {
					ldc_cmdq_intr_clr(top_id, intr_status);
					TRACE_LDC(DBG_DEBUG, "ldc_%d irqn(%d) cmdq-status(0x%x)\n"
						, top_id, irq, intr_status);

					if ((intr_status & 0x04) && ldc_cmdq_is_sw_restart(top_id)) {
						TRACE_LDC(DBG_DEBUG, "cmdq-sw restart\n");
						ldc_cmdq_sw_restart(top_id);
					}
				} else {
					goto CMDQ_STATUS_UNKOWN;
				}
			} else {
CMDQ_STATUS_UNKOWN:
				intr_status = ldc_intr_status(top_id);
				TRACE_LDC(DBG_INFO, "ldc_%d, irq(%d), status(0x%x)\n"
					, top_id, irq, intr_status);
				ldc_intr_clr(intr_status, top_id);
			}
			ldc_intr_ctrl(0x00, top_id);
			ldc_disable(top_id);
			ldc_irq_handler(intr_status, ctx, top_id, done_job);
		}
	}

	return IRQ_HANDLED;
}

static int ldc_register_dev(struct ldc_vdev *wdev)
{
	int rc;

	wdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	wdev->miscdev.name = LDC_DEV_NAME;
	wdev->miscdev.fops = &ldc_fops;

	rc = misc_register(&wdev->miscdev);
	if (rc) {
		pr_err("ldc: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int ldc_init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	int i;
	int irq_num[LDC_DEV_MAX_CNT];
	const char * const irq_name[LDC_DEV_MAX_CNT] = {LDC0_INTR_NAME};
	const char ldc_clk_name[LDC_DEV_MAX_CNT][20] = {"reg_clk_ldc_vip_en"};

	struct resource *res[LDC_DEV_MAX_CNT];
	void __iomem *reg_base[LDC_DEV_MAX_CNT];
	struct ldc_vdev *dev;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get ldc drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(reg_base); ++i) {
		res[i] = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (unlikely(res[i] == NULL)) {
			dev_err(&pdev->dev, "invalid resource\n");
			return -EINVAL;
		}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res[i]->start,
			res[i]->end - res[i]->start);
		//reg_base[i] = devm_ioremap_resource(&pdev->dev, res[i]);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res[i]->start,
						res[i]->end - res[i]->start);
#endif
		TRACE_LDC(DBG_DEBUG, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px)\n"
			, i, res[i]->start, res[i]->end, reg_base[i]);
		ldc_set_base_addr(reg_base[i], i);

		irq_num[i] = platform_get_irq_byname(pdev, irq_name[i]);
		if (irq_num[i] < 0) {
			dev_err(&pdev->dev, "(%d)No IRQ resource for %s\n", i, irq_name[i]);
			return -ENODEV;
		}

		TRACE_LDC(DBG_DEBUG, "(%d)irq(%d) for %s get from platform driver.\n"
			, i, irq_num[i], irq_name[i]);

		dev->ctx.core[i].ldc_clk = osal_clk_get(&pdev->dev, ldc_clk_name[i]);
		if (IS_ERR(dev->ctx.core[i].ldc_clk->clk)) {
			TRACE_LDC(DBG_ERR, "Cannot get clk for clk_ldc[%d]\n", i);
			dev->ctx.core[i].ldc_clk = NULL;
		}

		if (devm_request_irq(&pdev->dev, irq_num[i], ldc_isr, IRQF_SHARED, irq_name[i], (void *)&dev->ctx)) {
			dev_err(&pdev->dev, "Unable to request ldc_%d IRQ(%d)\n", i, irq_num[i]);
			return -EINVAL;
		}

		dev->ctx.core[i].irq_num = irq_num[i];
	}

#else
	TRACE_LDC(DBG_INFO, "cannot get res for ldc\n");
	rc = -1;
#endif

	return rc;
}

static int ldc_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	int i;
	struct ldc_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("ldc cannot get drv data for ldc\n");
		return -EINVAL;
	}

	if (ldc_register_dev(wdev))
		goto err_dev;

#ifdef CONFIG_PROC_FS
	wdev->ctx.shared_mem = kzalloc(sizeof(struct ldc_proc_ctx), GFP_ATOMIC);
	if (ldc_proc_init(wdev->ctx.shared_mem) < 0) {
		pr_err("ldc proc init failed\n");
		goto err_proc;
	}
#endif

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		osal_atomic_set(&wdev->ctx.core[i].clk_en, false);
	}

	ldc_dev_init(&wdev->ctx);

	if (ldc_sw_init(&wdev->ctx)) {
		pr_err("ldc sw init fail\n");
		goto err_sw_init;
	}

	return rc;

err_sw_init:
	ldc_sw_deinit(&wdev->ctx);
#ifdef CONFIG_PROC_FS
err_proc:
	kfree(wdev->ctx.shared_mem);
#endif

misc_deregister(&wdev->miscdev);

err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	TRACE_LDC(DBG_ERR, "failed with rc(%d).\n", rc);

	return rc;
}

static int ldc_destroy_instance(struct platform_device *pdev)
{
	struct ldc_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("invalid data\n");
		return -EINVAL;
	}

	ldc_sw_deinit(&wdev->ctx);
	ldc_dev_deinit(&wdev->ctx);

#ifdef CONFIG_PROC_FS
	ldc_proc_remove();
	kfree(wdev->ctx.shared_mem);
#endif

	misc_deregister(&wdev->miscdev);

	return 0;
}

static int ldc_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct ldc_vdev *wdev;
	struct ldc_ctx **ctx = get_ldc_ctx_addr();

	TRACE_LDC(DBG_INFO, "done with rc(%d).\n", rc);

	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	ldc_dev = wdev;
	*ctx = &ldc_dev->ctx;
	dev_set_drvdata(&pdev->dev, wdev);

	rc = ldc_init_resources(pdev);
	if (rc)
		goto err_res;

	rc = ldc_create_instance(pdev);
	if (rc) {
		pr_err("Failed to create ldc instance\n");
		goto err_ldc_creat_ins;
	}

	if (ldc_reg_cb(&wdev->ctx)) {
		dev_err(&pdev->dev, "Failed to register ldc cb, err %d\n", rc);
		return -EINVAL;
	}

	TRACE_LDC(DBG_INFO, "done with rc(%d).\n", rc);

	return rc;

err_ldc_creat_ins:
err_res:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);

	return rc;
}

static int ldc_remove(struct platform_device *pdev)
{
	struct ldc_vdev *dev;
	int i;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get ldc drvdata\n");
		return -EINVAL;
	}

	if (ldc_destroy_instance(pdev)) {
		dev_err(&pdev->dev, "ldc_destroy_instance fail\n");
	}

	for (i = 0; i < LDC_DEV_MAX_CNT; i++) {
		if (dev->ctx.core[i].ldc_clk) {
			osal_clk_put(&pdev->dev, dev->ctx.core[i].ldc_clk);
			dev->ctx.core[i].ldc_clk = NULL;
		}
	}

	if (ldc_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm ldc cb\n");
	}

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id ldc_dt_match[] = {
	{ .compatible = "cvitek,gdc" },
	{}
};

#ifdef CONFIG_PM_SLEEP
int gdc_suspend(struct device *dev)
{
	TRACE_LDC(DBG_WARN, "ldc suspended +\n");
	ldc_core_suspend();
	TRACE_LDC(DBG_WARN, "ldc suspended -\n");
	return 0;
}

int gdc_resume(struct device *dev)
{
	TRACE_LDC(DBG_WARN, "ldc resumed +\n");
	ldc_core_resume();
	TRACE_LDC(DBG_WARN, "ldc resumed -\n");
	return 0;
}

static SIMPLE_DEV_PM_OPS(ldc_pm_ops, gdc_suspend, gdc_resume);
#else
static SIMPLE_DEV_PM_OPS(ldc_pm_ops, NULL, NULL);
#endif

#if (!DEVICE_FROM_DTS)
static void ldc_pdev_release(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
}

static struct platform_device ldc_pdev = {
	.name		= LDC_DEV_NAME,
	.dev.release	= ldc_pdev_release,
};
#endif

static struct platform_driver ldc_driver = {
	.probe      = ldc_probe,
	.remove     = ldc_remove,
	.driver     = {
		.name		= LDC_DEV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= ldc_dt_match,
		.pm		= &ldc_pm_ops,
	},
};

#if 1
static int __init __ldc_init(void)
{
	int rc;

	TRACE_LDC(DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&ldc_driver);
	#else
	rc = platform_device_register(&ldc_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&ldc_driver);
	if (rc)
		platform_device_unregister(&ldc_pdev);
	#endif

	return rc;
}

static void __exit __ldc_exit(void)
{
	TRACE_LDC(DBG_INFO, " +\n");
	platform_driver_unregister(&ldc_driver);
	#if (!DEVICE_FROM_DTS)
		platform_device_unregister(&ldc_pdev);
	#endif
}

module_init(__ldc_init);
module_exit(__ldc_exit);
#else
module_platform_driver(ldc_driver);
#endif

MODULE_DESCRIPTION("Cvitek Video Driver For LDC");
MODULE_AUTHOR("robin.lee");
MODULE_LICENSE("GPL");
