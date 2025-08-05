#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "comm_errno.h"
#include "tde_debug.h"
#include "tde_sdk_layer.h"
#include "tde_uapi.h"
#include "base_ctx.h"


long tde_ioctl(struct tde_core *core, unsigned int cmd, unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	in_size = out_size = ksize = _IOC_SIZE(cmd);
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = osal_kmalloc(ksize, OSAL_GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}

	if (in_size && copy_from_user(kdata, (void __user *)arg, in_size)) {
		TRACE_TDE(DBG_INFO, "copy_from_user failed.\n");
		ret = -EFAULT;
		goto err;
	}

	switch (cmd) {
	case TDE_BEJIN_JOB:
	{
		struct tde_begin_job_cfg *cfg = (struct tde_begin_job_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_begin_job_cfg);
		cfg->handle = tde_begin_job(core);
		if (cfg->handle == TDE_INVALID_HANDLE)
			ret = TDE_INVALID_HANDLE;
		break;
	}

	case TDE_END_JOB:
	{
		struct tde_end_job_cfg *cfg = (struct tde_end_job_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_end_job_cfg);
		ret = tde_end_job(core, cfg->handle, cfg->is_sync, cfg->is_block, cfg->timeout);
		break;
	}

	case TDE_WAIT_ALL_DONE:
	{
		ret = tde_wait_all_done(core);
		break;
	}

	case TDE_CANCEL_JOB:
	{
		struct tde_cancel_job_cfg *cfg = (struct tde_cancel_job_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_cancel_job_cfg);
		ret = tde_cancel_job(core, cfg->handle);
		break;
	}


	case TDE_ROTATE:
	{
		struct tde_rotate_cfg *cfg = (struct tde_rotate_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_rotate_cfg);
		ret = tde_rotate(core, cfg->handle, &cfg->src, &cfg->dst, cfg->angle);
		break;
	}

	case TDE_DRAW_LINE:
	{
		struct tde_draw_line_cfg *cfg = (struct tde_draw_line_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_draw_line_cfg);
		ret = tde_draw_line(core, cfg->handle, &cfg->src, &cfg->dst, &cfg->line);
		break;
	}

	case TDE_QUICK_COPY:
	{
		struct tde_quick_copy_cfg *cfg = (struct tde_quick_copy_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct tde_quick_copy_cfg);
		ret = tde_quick_copy(core, cfg->handle, &cfg->src, &cfg->dst);
		break;
	}

	default:
		TRACE_TDE(DBG_ERR, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (out_size && copy_to_user((void __user *)arg, kdata, out_size))
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
