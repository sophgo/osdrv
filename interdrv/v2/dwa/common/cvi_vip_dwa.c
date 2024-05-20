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

#include <linux/cvi_comm_video.h>
#include <linux/dwa_uapi.h>

#include <base_cb.h>
#include <vi_sys.h>

#include "cvi_vip_dwa.h"
#include "dwa_debug.h"
#include "cvi_vip_dwa_proc.h"
#include "dwa_sdk.h"
#include "dwa.h"

#define DWA_CLASS_NAME "soph-dwa"
#define DWA_DEV_NAME "soph-dwa"
#define DWA_CLK_NAME "dwa_clk"
#define DWA_CLK_SYS_NAME "clk_sys_3"
#define DWA0_REG_NAME "dwa0"
#define DWA1_REG_NAME "dwa1"
#define DWA0_INTR_NAME "dwa0"
#define DWA1_INTR_NAME "dwa1"

#define DWA_SHARE_MEM_SIZE (0x8000)

#ifndef DEVICE_FROM_DTS
#define DEVICE_FROM_DTS 1
#endif

#ifndef CONFIG_PROC_FS
#define CONFIG_PROC_FS 1
#endif

#ifndef CONFIG_PM_SLEEP
#define CONFIG_PM_SLEEP 1
#endif

static atomic_t dwa_open_count = ATOMIC_INIT(0);
static atomic_t suspend_flag = ATOMIC_INIT(0);

static struct cvi_dwa_vdev *dwa_dev;
static struct fasync_struct *dwa_fasync;

u32 dwa_log_lv = CVI_DBG_WARN;
bool dwa_dump_reg;

module_param(dwa_log_lv, int, 0644);
MODULE_PARM_DESC(dwa_log_lv, "DWA Debug Log Level");

static u32 clk_sys_freq[DWA_DEV_MAX_CNT];
static u32 core_cnt;
module_param_array(clk_sys_freq, int, &core_cnt, S_IRUGO);
MODULE_PARM_DESC(clk_sys_freq, "clk_sys_freq setting by user");

module_param(dwa_dump_reg, bool, 0644);
MODULE_PARM_DESC(dwa_dump_reg, "dwa need dump reg");

bool is_dwa_suspended(void)
{
	return atomic_read(&suspend_flag) == 1 ? true : false;
}

struct cvi_dwa_vdev *dwa_get_dev(void)
{
	return dwa_dev;
}

struct fasync_struct *dwa_get_dev_fasync(void)
{
	return dwa_fasync;
}

void dwa_core_init(int top_id)
{
	dwa_reset(top_id);
	dwa_init(top_id);
	dwa_intr_ctrl(0x00, top_id);//0x7 if you want get mesh tbl id err status
}

void dwa_core_deinit(int top_id)
{
	dwa_intr_ctrl(0x00, top_id);
	dwa_disable(top_id);
	dwa_reset(top_id);
}

void dwa_dev_init(struct cvi_dwa_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < DWA_DEV_MAX_CNT; i++) {
		atomic_set(&dev->core[i].state, DWA_CORE_STATE_IDLE);
		dwa_core_init(i);
	}

	atomic_set(&dev->state, DWA_DEV_STATE_STOP);
}

void dwa_dev_deinit(struct cvi_dwa_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < DWA_DEV_MAX_CNT; i++) {
		atomic_set(&dev->core[i].state, DWA_CORE_STATE_IDLE);
		dwa_core_deinit(i);
	}

	atomic_set(&dev->state, DWA_DEV_STATE_STOP);
}

void dwa_enable_dev_clk(int coreid, bool en)
{
	struct cvi_dwa_vdev *dev = dwa_get_dev();

	if (!dev || !dev->clk_dwa[coreid]) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null dev or null clk_dwa[%d]\n", coreid);
		return;
	}

	if (en) {
		if (!__clk_is_enabled(dev->clk_dwa[coreid]))
			clk_enable(dev->clk_dwa[coreid]);
	} else {
		if (__clk_is_enabled(dev->clk_dwa[coreid]))
			clk_disable(dev->clk_dwa[coreid]);
	}
}

static int dwa_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct cvi_dwa_vdev *wdev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = wdev->shared_mem;

	if ((vm_size + offset) > DWA_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE,
				    vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("dwa proc mmap vir(%p) phys(%#llx)\n", pos,
			 (unsigned long long)virt_to_phys((void *)pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static long dwa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_dwa_vdev *wdev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
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
		CVI_TRACE_DWA(CVI_DBG_ERR, "access_ok failed\n");
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size))
		CVI_TRACE_DWA(CVI_DBG_ERR, "access_ok failed\n");
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	//CVI_TRACE_LDC(CVI_DBG_ERR, "ksize-in_size-cmd[%d-%d-%d]\n", ksize, in_size, cmd);
	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
		case CVI_DWA_INIT:
			break;
		case CVI_DWA_DEINIT:
			break;
		case CVI_DWA_BEGIN_JOB: {
			struct dwa_handle_data *data = (struct dwa_handle_data *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_BEGIN_JOB\n");
			ret = dwa_begin_job(wdev, data);
			break;
		}
		case CVI_DWA_END_JOB: {
			struct dwa_handle_data *data = (struct dwa_handle_data *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_END_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			ret = dwa_end_job(wdev, data->handle);
			break;
		}
		case CVI_DWA_CANCEL_JOB: {
			struct dwa_handle_data *data = (struct dwa_handle_data *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_CANCLE_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			ret = dwa_cancel_job(wdev, data->handle);
			break;
		}
		case CVI_DWA_GET_WORK_JOB: {
			struct dwa_handle_data *data = (struct dwa_handle_data *)kdata;

			ret = dwa_get_work_job(wdev, data);
			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVI_DWA_GET_WORK_JOB, handle=0x%llx\n",
				      (unsigned long long)data->handle);
			break;
		}
		case CVI_DWA_ADD_ROT_TASK: {
			struct dwa_task_attr *attr = (struct dwa_task_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_ROT_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			ret = dwa_add_rotation_task(wdev, attr);
			break;
		}
		case CVI_DWA_ADD_LDC_TASK: {
			struct dwa_task_attr *attr = (struct dwa_task_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_LDC_TASK, handle=0x%llx\n",
				      (unsigned long long)attr->handle);
			ret = dwa_add_ldc_task(wdev, attr);
			break;
		}
		case CVI_DWA_ADD_COR_TASK: {
			struct dwa_task_attr *attr = (struct dwa_task_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_COR_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			ret = dwa_add_cor_task(wdev, attr);
			break;
		}
		case CVI_DWA_ADD_WAR_TASK: {
			struct dwa_task_attr *attr = (struct dwa_task_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_WAR_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			ret = dwa_add_warp_task(wdev, attr);
			break;
		}
		case CVI_DWA_ADD_AFF_TASK: {
			struct dwa_task_attr *attr = (struct dwa_task_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVIDWA_ADD_COR_TASK, handle=0x%llx\n",
					  (unsigned long long)attr->handle);
			ret = dwa_add_affine_task(wdev, attr);
			break;
		}
		case CVI_DWA_SET_JOB_IDENTITY: {
			struct dwa_identity_attr *identity = (struct dwa_identity_attr *)kdata;

			CVI_TRACE_DWA(CVI_DBG_DEBUG, "CVI_DWA_SET_JOB_IDENTITY, handle=0x%llx\n",
					  (unsigned long long)identity->handle);
			ret = dwa_set_identity(wdev, identity);
			break;
		}
		case CVI_DWA_GET_CHN_FRM: {
			struct dwa_chn_frm_cfg *cfg = (struct dwa_chn_frm_cfg *)kdata;
			VIDEO_FRAME_INFO_S *VideoFrame = &cfg->VideoFrame;
			struct dwa_identity_attr *identity = &cfg->identity;
			s32 MilliSec = cfg->MilliSec;

			ret = dwa_get_chn_frame(wdev, identity, VideoFrame, MilliSec);
			break;
		}
		case CVI_DWA_SUSPEND: {
			struct cvi_dwa_vdev *dev
				= container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);

			ret = dwa_suspend(dev->miscdev.this_device);
			break;
		}
		case CVI_DWA_RESUME: {
			struct cvi_dwa_vdev *dev
				= container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);

			ret = dwa_resume(dev->miscdev.this_device);
			break;
		}
		default:
			ret = -ENOTTY;
			goto err;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);
	//CVI_TRACE_LDC(CVI_DBG_ERR, "kdata-stack_kdata[%px-%px]\n", kdata, stack_kdata);

	return ret;
}

static int dwa_open(struct inode *inode, struct file *filp)
{
	struct cvi_dwa_vdev *dev =
		container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("cannot find dwa private data\n");
		return -ENODEV;
	}

	i = atomic_inc_return(&dwa_open_count);
	if (i > 1) {
		pr_info("dwa_open: open %d times\n", i);
		return 0;
	}

	CVI_TRACE_DWA(CVI_DBG_INFO, "dwa_open\n");
	return 0;
}

static int dwa_release(struct inode *inode, struct file *filp)
{
	struct cvi_dwa_vdev *dev
		= container_of(filp->private_data, struct cvi_dwa_vdev, miscdev);
	int i;

	if (!dev) {
		pr_err("Cannot find stitch private data\n");
		return -ENODEV;
	}

	i = atomic_dec_return(&dwa_open_count);
	if (i) {
		pr_info("dwa_close: open %d times\n", i);
		return 0;
	}

	CVI_TRACE_DWA(CVI_DBG_INFO, "dwa_release.\n");
	return 0;
}

static int dwa_drv_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &dwa_fasync) >= 0)
		return 0;
	else
		return -EIO;
}

static const struct file_operations dwa_fops = {
	.owner = THIS_MODULE,
	.open = dwa_open,
	.release = dwa_release,
	.mmap = dwa_mmap,
	.unlocked_ioctl = dwa_ioctl,
	.fasync  = dwa_drv_fasync,
};

static void dwa_tsk_finish(struct cvi_dwa_vdev *dev, int top_id)
{
	struct dwa_core *core = &dev->core[top_id];
	struct dwa_task *done_tsk;
	unsigned long flags;
	struct dwa_job *done_job;

	if (atomic_read(&core->state) != DWA_CORE_STATE_END)
		return;

	spin_lock_irqsave(&dev->job_lock, flags);
	done_tsk = list_first_entry_or_null(&core->list.done_list, struct dwa_task, node);
	done_job = list_first_entry_or_null(&dev->list.done_list[top_id], struct dwa_job, node);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "job [%px]\n", done_job);
	if (unlikely(!done_tsk)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null core[%d] done tsk\n", top_id);
		return;
	}

	if (atomic_read(&done_tsk->state) == DWA_TASK_STATE_RUNNING) {
		if (done_tsk->pfnTskCB)
			done_tsk->pfnTskCB(core, top_id);
		else
			CVI_TRACE_DWA(CVI_DBG_WARN, "null pfntskCB\n");
	}
}

#if DWA_USE_THREADED_IRQ
static irqreturn_t dwa_tsk_finish_thread_func(int irq, void *data)
{
	struct cvi_dwa_vdev *dev = (struct cvi_dwa_vdev *)data;
	struct dwa_core *core;
	int i, top_id = -1;

	if (!dev) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid dwa_dev\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < dev->core_num; i++) {
		core = &dev->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			break;
		}
	}

	if (top_id >= 0 && top_id < dev->core_num)
		dwa_tsk_finish(dev, top_id);
	else
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid core[%d]\n", top_id);
	return IRQ_HANDLED;
}
#endif

static void dwa_irq_handler(u8 intr_status, struct cvi_dwa_vdev *dev, int top_id, bool use_cmdq)
{
	if (!dev)
		return;

	if (top_id < 0 || top_id >= DWA_DEV_MAX_CNT) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid dwa_dev_%d\n", top_id);
		return;
	}

	if (use_cmdq) {
		CVI_TRACE_DWA(CVI_DBG_DEBUG, "core(%d) cmdq status(%#x)\n", top_id, intr_status);
	} else {
		if (!(intr_status & BIT(0))) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "core(%d) axi read err, status(%#x)\n", top_id, intr_status);
			return;
		}
	}
	atomic_set(&dev->core[top_id].state, DWA_CORE_STATE_END);
	CVI_TRACE_DWA(CVI_DBG_INFO, "core(%d) state set(%d)\n", top_id, DWA_CORE_STATE_END);

#if !DWA_USE_THREADED_IRQ
	dwa_tsk_finish(dev, top_id);
#endif
}

static irqreturn_t dwa_isr(int irq, void *data)
{
	struct cvi_dwa_vdev *dev = (struct cvi_dwa_vdev *)data;
	struct dwa_core *core;
	int top_id, i;
	u8 intr_status;
	unsigned long flags;
	struct dwa_job *done_job;
	struct dwa_task *done_tsk;

	if (!dev) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid dwa_dev\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < dev->core_num; i++) {
		core = &dev->core[i];
		if (core->irq_num == irq) {
			top_id = i;
			spin_lock_irqsave(&dev->job_lock, flags);
			done_job = list_first_entry_or_null(&dev->list.work_list[top_id], struct dwa_job, node);
			done_tsk = list_first_entry_or_null(&dev->core[top_id].list.work_list, struct dwa_task, node);
			spin_unlock_irqrestore(&dev->job_lock, flags);

			if (unlikely(!done_job || !done_tsk)) {
				CVI_TRACE_DWA(CVI_DBG_ERR, "null done_job done_tsk\n");
				intr_status = dwa_intr_status(top_id);
				dwa_intr_clr(intr_status, top_id);
				dwa_intr_ctrl(0x00, top_id);
				dwa_disable(top_id);
				//dwa_enable_dev_clk(top_id, false);
				return IRQ_HANDLED;
			}

			spin_lock_irqsave(&dev->job_lock, flags);
			list_del(&done_job->node);
			list_add_tail(&done_job->node, &dev->list.done_list[top_id]);

			list_del(&done_tsk->node);
			list_add_tail(&done_tsk->node, &dev->core[top_id].list.done_list);
			spin_unlock_irqrestore(&dev->job_lock, flags);

			if (done_job->use_cmdq) {
				intr_status = dwa_cmdq_intr_status(top_id);
				if (intr_status) {
					dwa_cmdq_intr_clr(top_id, intr_status);
					CVI_TRACE_DWA(CVI_DBG_DEBUG, "dwa_%d irqn(%d) cmdq-status(0x%x)\n"
						, top_id, irq, intr_status);

					if ((intr_status & 0x04) && dwa_cmdq_is_sw_restart(top_id)) {
						CVI_TRACE_DWA(CVI_DBG_DEBUG, "cmdq-sw restart\n");
						dwa_cmdq_sw_restart(top_id);
					}
				} else {
					goto CMDQ_STATUS_UNKOWN;
				}
			} else {
CMDQ_STATUS_UNKOWN:
				intr_status = dwa_intr_status(top_id);
				CVI_TRACE_DWA(CVI_DBG_INFO, "dwa_%d, irq(%d), status(0x%x)\n"
					, top_id, irq, intr_status);
				dwa_intr_clr(intr_status, top_id);
			}
			dwa_intr_ctrl(0x00, top_id);
			dwa_disable(top_id);
			//dwa_enable_dev_clk(top_id, false);

			dwa_irq_handler(intr_status, dev, top_id, done_job->use_cmdq);
		}
	}
#if DWA_USE_THREADED_IRQ
	return IRQ_WAKE_THREAD;
#else
	return IRQ_HANDLED;
#endif
}

static int dwa_register_dev(struct cvi_dwa_vdev *wdev)
{
	int rc;

	wdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	wdev->miscdev.name = DWA_DEV_NAME;
	wdev->miscdev.fops = &dwa_fops;

	rc = misc_register(&wdev->miscdev);
	if (rc) {
		pr_err("cvi_dwa: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int dwa_init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	int i;
	int irq_num[DWA_DEV_MAX_CNT];
	const char * const irq_name[DWA_DEV_MAX_CNT] = {DWA0_INTR_NAME, DWA1_INTR_NAME};
	const char dwa_clk_name[DWA_DEV_MAX_CNT][16] = {"clk_dwa0", "clk_dwa1"};
	const char dwa_clk_sys_name[DWA_DEV_MAX_CNT][16] = {"clk_sys_3", "clk_sys_3"};

	struct resource *res[DWA_DEV_MAX_CNT];
	void __iomem *reg_base[DWA_DEV_MAX_CNT];
	struct cvi_dwa_vdev *dev;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get dwa drvdata\n");
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
		CVI_TRACE_DWA(CVI_DBG_DEBUG, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px)\n"
			, i, res[i]->start, res[i]->end, reg_base[i]);
		dwa_set_base_addr(reg_base[i], i);

		irq_num[i] = platform_get_irq_byname(pdev, irq_name[i]);
		if (irq_num[i] < 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n", irq_name[i]);
			return -ENODEV;
		}

		CVI_TRACE_DWA(CVI_DBG_DEBUG, "(%d)irq(%d) for %s get from platform driver.\n"
			, i, irq_num[i], irq_name[i]);

		//clk res
		dev->clk_src[i] = devm_clk_get(&pdev->dev, dwa_clk_sys_name[i]);
		if (IS_ERR(dev->clk_src[i])) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "Cannot get clk for clk_sys_3 for stitch\n");
			dev->clk_src[i] = NULL;
		}
		dev->clk_dwa[i] = devm_clk_get(&pdev->dev, dwa_clk_name[i]);
		if (IS_ERR(dev->clk_dwa[i])) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "Cannot get clk for clk_stitch\n");
			dev->clk_dwa[i] = NULL;
		}
		if (clk_sys_freq[i])
			dev->clk_sys_freq[i] = clk_sys_freq[i];

#if DWA_USE_WORKQUEUE
		if (devm_request_irq(&pdev->dev, irq_num[i], dwa_isr, IRQF_SHARED
#elif DWA_USE_THREADED_IRQ
		if (devm_request_threaded_irq(&pdev->dev, irq_num[i], dwa_isr, dwa_tsk_finish_thread_func, IRQF_SHARED
#else
		if (devm_request_irq(&pdev->dev, irq_num[i], dwa_isr, IRQF_SHARED
#endif
			, irq_name[i], (void*)dev)) {
			dev_err(&pdev->dev, "Unable to request dwa_%d IRQ(%d)\n", i, irq_num[i]);
			return -EINVAL;
		}

		dev->core[i].irq_num = irq_num[i];
		dev->core[i].dev_type = (enum cvi_dwa_type)i;
		atomic_set(&dev->core[i].state, DWA_CORE_STATE_IDLE);
	}

	// clk_dwa_src_sel default 1(clk_src_vip_sys_2), 600 MHz
	// set 0(clk_src_vip_sys_4), 400MHz for ND
#if defined(__CV186X__)
		/*vi_sys_reg_write_mask(VI_SYS_CLK_DWA0_SRC_SEL,
					VI_SYS_CLK_DWA0_SRC_SEL_MASK,
					0 << VI_SYS_CLK_DWA0_SRC_SEL_OFFSET);
		*/
#else
		vip_sys_reg_write_mask(VIP_SYS_VIP_CLK_CTRL1, BIT(20), 0);
#endif

#else
	CVI_TRACE_DWA(CVI_DBG_INFO, "cannot get res for stitch\n");
	rc = -1;
#endif

	return rc;
}

static void dwa_clk_init(struct cvi_dwa_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < DWA_DEV_MAX_CNT; i++) {
		if (dev->clk_src[i])
			clk_prepare_enable(dev->clk_src[i]);
		if (dev->clk_apb[i])
			clk_prepare_enable(dev->clk_apb[i]);
		if (dev->clk_dwa[i] && !__clk_is_enabled(dev->clk_dwa[i]))
			clk_prepare_enable(dev->clk_dwa[i]);

		dwa_enable_dev_clk(i, true);

		if (clk_sys_freq[i])
			dev->clk_sys_freq[i] = clk_sys_freq[i];
	}
}

static void dwa_clk_deinit(struct cvi_dwa_vdev *dev)
{
	int i;

	if (unlikely(!dev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "null dev.\n");
		return;
	}

	for (i = 0; i < DWA_DEV_MAX_CNT; i++) {
		if (dev->clk_dwa[i] && __clk_is_enabled(dev->clk_dwa[i]))
			clk_disable_unprepare(dev->clk_dwa[i]);
		if (dev->clk_apb[i])
			clk_disable_unprepare(dev->clk_apb[i]);
		if (dev->clk_src[i])
			clk_disable_unprepare(dev->clk_src[i]);

		if (clk_sys_freq[i]) {
			dev->clk_sys_freq[i] = clk_sys_freq[i];
			clk_sys_freq[i] = 0;
		}
	}
}

static int dwa_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_dwa_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("dwa cannot get drv data for dwa\n");
		return -EINVAL;
	}

	if (dwa_register_dev(wdev))
		goto err_dev;

#ifdef CONFIG_PROC_FS
	wdev->shared_mem = kzalloc(sizeof(struct dwa_proc_ctx), GFP_ATOMIC);
	if (!wdev->shared_mem) {
		pr_err("dwa shared mem alloc fail\n");
		rc = -ENOMEM;
		goto err_proc;
	}

	if (dwa_proc_init(wdev->shared_mem) < 0) {
		pr_err("dwa proc init failed\n");
		goto err_proc;
	}
#endif

	dwa_clk_init(wdev);
	dwa_dev_init(wdev);

	if (cvi_dwa_sw_init(wdev)) {
		pr_err("dwa sw init fail\n");
		goto err_sw_init;
	}

	return rc;

err_sw_init:
	cvi_dwa_sw_deinit(wdev);
#ifdef CONFIG_PROC_FS
err_proc:
	if (wdev->shared_mem) {
		kfree(wdev->shared_mem);
		wdev->shared_mem = NULL;
	}
#endif

misc_deregister(&wdev->miscdev);

err_dev:
	dev_set_drvdata(&pdev->dev, NULL);
	CVI_TRACE_DWA(CVI_DBG_ERR, "failed with rc(%d).\n", rc);

	return rc;
}

static int dwa_destroy_instance(struct platform_device *pdev)
{
	struct cvi_dwa_vdev *wdev;

	wdev = dev_get_drvdata(&pdev->dev);
	if (!wdev) {
		pr_err("invalid data\n");
		return -EINVAL;
	}

	cvi_dwa_sw_deinit(wdev);
	dwa_dev_deinit(wdev);
	dwa_clk_deinit(wdev);

#ifdef CONFIG_PROC_FS
	dwa_proc_remove();

	if (wdev->shared_mem) {
		kfree(wdev->shared_mem);
		wdev->shared_mem = NULL;
	}
#endif

	misc_deregister(&wdev->miscdev);

	return 0;
}

static int cvi_dwa_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_dwa_vdev *wdev;

	wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	dwa_dev = wdev;
	dev_set_drvdata(&pdev->dev, wdev);

	rc = dwa_init_resources(pdev);
	if (rc)
		goto err_res;

	rc = dwa_create_instance(pdev);
	if (rc) {
		pr_err("Failed to create dwa instance\n");
		goto err_dwa_creat_ins;
	}

	if (dwa_reg_cb(wdev)) {
		dev_err(&pdev->dev, "Failed to register dwa cb, err %d\n", rc);
		return -EINVAL;
	}

	CVI_TRACE_DWA(CVI_DBG_INFO, "done with rc(%d).\n", rc);

	return rc;

err_dwa_creat_ins:
err_res:
	dev_set_drvdata(&pdev->dev, NULL);

	dev_err(&pdev->dev, "failed with rc(%d).\n", rc);

	return rc;
}

static int cvi_dwa_remove(struct platform_device *pdev)
{
	if (dwa_destroy_instance(pdev)) {
		dev_err(&pdev->dev, "dwa_destroy_instance fail\n");
	}

	if (dwa_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm dwa cb\n");
	}

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static const struct of_device_id cvi_dwa_dt_match[] = {
	{ .compatible = "cvitek,dwa" },
	{}
};

#ifdef CONFIG_PM_SLEEP
int dwa_suspend(struct device *dev)
{
	s32 ret = 0;
	struct cvi_dwa_vdev * vdev = dwa_get_dev();

	if (unlikely(!vdev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "dwa_dev is null\n");
		return -1;
	}

	ret = dwa_suspend_handler();
	if (ret) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "fail to suspend dwa thread, err=%d\n", ret);
		return -1;
	}

	dwa_dev_deinit(vdev);
	dwa_clk_deinit(vdev);
	atomic_set(&suspend_flag, 1);

	CVI_TRACE_DWA(CVI_DBG_INFO, "dwa suspend+\n");
	return 0;
}

int dwa_resume(struct device *dev)
{
	s32 ret = 0;
	struct cvi_dwa_vdev * vdev = dwa_get_dev();

	if (unlikely(!vdev)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "dwa_dev is null\n");
		return -1;
	}

	dwa_clk_init(vdev);
	dwa_dev_init(vdev);

	ret = dwa_resume_handler();
	if (ret) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "fail to resume dwa thread, err=%d\n", ret);
		return -1;
	}
	atomic_set(&suspend_flag, 0);

	CVI_TRACE_DWA(CVI_DBG_INFO, "dwa resume+\n");
	return 0;
}



static SIMPLE_DEV_PM_OPS(dwa_pm_ops, dwa_suspend, dwa_resume);
#else
static SIMPLE_DEV_PM_OPS(dwa_pm_ops, NULL, NULL);
#endif

#if (!DEVICE_FROM_DTS)
static void cvi_dwa_pdev_release(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
}

static struct platform_device cvi_dwa_pdev = {
	.name		= DWA_DEV_NAME,
	.dev.release	= cvi_dwa_pdev_release,
};
#endif

static struct platform_driver cvi_dwa_driver = {
	.probe      = cvi_dwa_probe,
	.remove     = cvi_dwa_remove,
	.driver     = {
		.name		= DWA_DEV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= cvi_dwa_dt_match,
		.pm		= &dwa_pm_ops,
	},
};

#if 1
static int __init cvi_dwa_init(void)
{
	int rc;

	CVI_TRACE_DWA(CVI_DBG_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_dwa_driver);
	#else
	rc = platform_device_register(&cvi_dwa_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_dwa_driver);
	if (rc)
		platform_device_unregister(&cvi_dwa_pdev);
	#endif

	return rc;
}

static void __exit cvi_dwa_exit(void)
{
	CVI_TRACE_DWA(CVI_DBG_INFO, " +\n");
	platform_driver_unregister(&cvi_dwa_driver);
	#if (!DEVICE_FROM_DTS)
		platform_device_unregister(&cvi_dwa_pdev);
	#endif
}

module_init(cvi_dwa_init);
module_exit(cvi_dwa_exit);
#else
module_platform_driver(cvi_dwa_driver);
#endif

MODULE_DESCRIPTION("Cvitek Video Driver For DWA");
MODULE_AUTHOR("robin.lee");
MODULE_LICENSE("GPL");
