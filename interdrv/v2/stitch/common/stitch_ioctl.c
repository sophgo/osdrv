#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>

#include "stitch_debug.h"
#include "stitch_core.h"
#include "stitch.h"
#include "stitch_ioctl.h"

long stitch_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
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
	if (!access_ok((void __user *)arg, in_size)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		CVI_TRACE_STITCH(CVI_DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_STITCH(CVI_DBG_INFO, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
		case CVI_STITCH_INIT: {
			ret = stitch_init();
			break;
		}
		case CVI_STITCH_DEINIT: {
			ret = stitch_deinit();
			break;
		}
		case CVI_STITCH_RST: {
			ret = stitch_reset();
			break;
		}
		case CVI_STITCH_SET_SRC_ATTR: {
			STITCH_SRC_ATTR_S *src_attr = (STITCH_SRC_ATTR_S *)kdata;

			ret = stitch_set_src_attr(src_attr);
			break;
		}
		case CVI_STITCH_GET_SRC_ATTR: {
			STITCH_SRC_ATTR_S *src_attr = (STITCH_SRC_ATTR_S *)kdata;

			stitch_get_src_attr(src_attr);
			break;
		}
		case CVI_STITCH_SET_CHN_ATTR: {
			STITCH_CHN_ATTR_S *chn_attr = (STITCH_CHN_ATTR_S *)kdata;

			ret = stitch_set_chn_attr(chn_attr);
			break;
		}
		case CVI_STITCH_GET_CHN_ATTR: {
			STITCH_CHN_ATTR_S *chn_attr = (STITCH_CHN_ATTR_S *)kdata;

			ret = stitch_get_chn_attr(chn_attr);
			break;
		}
		case CVI_STITCH_SET_OP_ATTR: {
			STITCH_OP_ATTR_S *op_attr = (STITCH_OP_ATTR_S *)kdata;

			ret = stitch_set_op_attr(op_attr);
			break;
		}
		case CVI_STITCH_GET_OP_ATTR: {
			STITCH_OP_ATTR_S *op_attr = (STITCH_OP_ATTR_S *)kdata;

			ret = stitch_get_op_attr(op_attr);
			break;
		}
		case CVI_STITCH_SET_WGT_ATTR: {
			STITCH_WGT_ATTR_S *wgt_attr = (STITCH_WGT_ATTR_S *)kdata;

			ret = stitch_set_wgt_attr(wgt_attr);
			break;
		}
		case CVI_STITCH_GET_WGT_ATTR: {
			STITCH_WGT_ATTR_S *wgt_attr = (STITCH_WGT_ATTR_S *)kdata;

			ret = stitch_get_wgt_attr(wgt_attr);
			break;
		}
		case CVI_STITCH_SET_REGX: {
			u8 regx = *(u8 *)(kdata);

			ret = stitch_set_reg_x(regx);
			break;
		}
		//case CVI_STITCH_GET_REGX: {
			//break;
		//}
		case CVI_STITCH_DEV_ENABLE: {
			ret = stitch_enable_dev();
			break;
		}
		case CVI_STITCH_DEV_DISABLE: {
			ret = stitch_disable_dev();
			break;
		}
		case CVI_STITCH_SEND_SRC_FRM: {
			struct stitch_src_frm_cfg *cfg = (struct stitch_src_frm_cfg *)kdata;
			VIDEO_FRAME_INFO_S *VideoFrame = &cfg->VideoFrame;
			s32 MilliSec = cfg->MilliSec;
			STITCH_SRC_IDX src_idx = cfg->src_id;

			ret = stitch_send_frame(src_idx, VideoFrame, MilliSec);
			break;
		}
		case CVI_STITCH_SEND_CHN_FRM: {
			struct stitch_chn_frm_cfg *cfg = (struct stitch_chn_frm_cfg *)kdata;
			VIDEO_FRAME_INFO_S *VideoFrame = &cfg->VideoFrame;
			s32 MilliSec = cfg->MilliSec;

			ret = stitch_send_chn_frame(VideoFrame, MilliSec);
			break;
		}
		case CVI_STITCH_GET_CHN_FRM: {
			struct stitch_chn_frm_cfg *cfg = (struct stitch_chn_frm_cfg *)kdata;
			VIDEO_FRAME_INFO_S *VideoFrame = &cfg->VideoFrame;
			s32 MilliSec = cfg->MilliSec;

			ret = stitch_get_chn_frame(VideoFrame, MilliSec);
			break;
		}
		case CVI_STITCH_RLS_CHN_FRM: {
			struct stitch_chn_frm_cfg *cfg = (struct stitch_chn_frm_cfg *)kdata;
			const VIDEO_FRAME_INFO_S *VideoFrame = &cfg->VideoFrame;

			ret = stitch_release_chn_frame(VideoFrame);
			break;
		}
		case CVI_STITCH_ATTACH_VB_POOL: {
			struct stitch_vb_pool_cfg *cfg = (struct stitch_vb_pool_cfg *)kdata;
			VB_POOL VbPool = (VB_POOL)cfg->VbPool;

			ret = stitch_attach_vb_pool(VbPool);
			break;
		}
		case CVI_STITCH_DETACH_VB_POOL: {
			ret = stitch_detach_vb_pool();
			break;
		}
		case CVI_STITCH_DUMP_REGS: {
			ret = stitch_dump_reg_info();
			break;
		}
		case CVI_STITCH_SUSPEND: {
			struct cvi_stitch_dev *dev
				= container_of(filp->private_data, struct cvi_stitch_dev, miscdev);

			ret = stitch_suspend(dev->miscdev.this_device);
			break;
		}
		case CVI_STITCH_RESUME: {
			struct cvi_stitch_dev *dev
				= container_of(filp->private_data, struct cvi_stitch_dev, miscdev);

			ret = stitch_resume(dev->miscdev.this_device);
			break;
		}
		default: {
			CVI_TRACE_STITCH(CVI_DBG_DEBUG, "unknown cmd(0x%x)\n", cmd);
			break;
		}
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
