#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
//#include <linux/module.h>

#include "dpu_debug.h"
#include "../chip/soph/dpu.h"
#include "dpu_ioctl.h"


long dpu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct cvi_dpu_dev *dpuData;
	struct cvi_dpu_handle_info *h_info, *h_node;
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

	dpuData = (struct cvi_dpu_dev *) filp->private_data;
	if (dpuData == NULL){
		return -ENOMEM;
	}

	if (dpu_get_handle_info(dpuData, filp, &h_info)) {
		pr_err("dpudrv: file list is not found!\n");
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_DPU(CVI_DBG_INFO, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case CVI_DPU_CREATE_GROUP:
	{
		struct dpu_grp_attr *attr =
			(struct dpu_grp_attr *)kdata;

		ret = cvi_dpu_create_grp(attr->DpuGrp, &attr->stGrpAttr);
		break;
	}

	case CVI_DPU_DESTROY_GROUP:
	{
		struct dpu_grp_cfg *cfg = ((struct dpu_grp_cfg *)kdata);

		ret = cvi_dpu_destroy_grp(cfg->DpuGrp);

		mutex_lock(&dpuData->dpuLock);
		list_for_each_entry(h_node, &dpuData->handle_list, list) {
			if(h_node->open_pid == h_info->open_pid){
				h_info->useGrp[cfg->DpuGrp] = CVI_FALSE;
			}
		}
		mutex_unlock(&dpuData->dpuLock);
		break;
	}

	case CVI_DPU_GET_AVAIL_GROUP:
	{
		DPU_GRP dpuGrp;

		dpuGrp = cvi_dpu_get_available_grp();
		*((DPU_GRP *)kdata) = dpuGrp;
		ret = 0;

		mutex_lock(&dpuData->dpuLock);
		list_for_each_entry(h_node, &dpuData->handle_list, list) {
			if(h_node->open_pid == h_info->open_pid){
				h_info->useGrp[dpuGrp] = CVI_TRUE;
			}
		}
		mutex_unlock(&dpuData->dpuLock);
		break;
	}

	case CVI_DPU_START_GROUP:
	{
		struct dpu_grp_cfg *cfg = (struct dpu_grp_cfg *)kdata;

		ret = cvi_dpu_start_grp(cfg->DpuGrp);
		break;
	}

	case CVI_DPU_STOP_GROUP:
	{
		struct dpu_grp_cfg *cfg = (struct dpu_grp_cfg *)kdata;

		ret = cvi_dpu_stop_grp(cfg->DpuGrp);
		break;
	}

	case CVI_DPU_SET_GRP_ATTR:
	{
		struct dpu_grp_attr *cfg = (struct dpu_grp_attr *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;

		const DPU_GRP_ATTR_S *pstGrpAttr = &cfg->stGrpAttr;

		ret = cvi_dpu_set_grp_attr(DpuGrp, pstGrpAttr);
		break;
	}

	case CVI_DPU_GET_GRP_ATTR:
	{
		struct dpu_grp_attr *cfg = (struct dpu_grp_attr *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		DPU_GRP_ATTR_S *pstGrpAttr = &cfg->stGrpAttr;

		ret = cvi_dpu_get_grp_attr(DpuGrp, pstGrpAttr);
		break;
	}

	case CVI_DPU_SEND_FRAME:
	{
		struct dpu_set_frame_cfg *cfg = (struct dpu_set_frame_cfg *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		const VIDEO_FRAME_INFO_S *pstSrcLeftFrame = &cfg->stSrcLeftFrame;
		const VIDEO_FRAME_INFO_S *pstSrcRightFrame = &cfg->stSrcRightFrame;
		s32 s32MilliSec = cfg->s32MilliSec;

		ret = cvi_dpu_send_frame(DpuGrp, pstSrcLeftFrame,\
								 pstSrcRightFrame,s32MilliSec);
		break;
	}

	case CVI_DPU_SEND_CHN_FRAME:{
		struct dpu_get_frame_cfg *cfg = (struct dpu_get_frame_cfg *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		DPU_CHN DpuChn = cfg->DpuChn;
		VIDEO_FRAME_INFO_S *pstFrameInfo = &cfg->stFrameInfo;

		s32 s32MilliSec = cfg->s32MilliSec;
		ret =  cvi_dpu_send_chn_frame(DpuGrp, DpuChn, pstFrameInfo, s32MilliSec);

		break;
	}

	case CVI_DPU_GET_FRAME:
	{
		struct dpu_get_frame_cfg *cfg = (struct dpu_get_frame_cfg *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		DPU_CHN DpuChn = cfg->DpuChn;
		VIDEO_FRAME_INFO_S *pstFrameInfo = &cfg->stFrameInfo;

		s32 s32MilliSec = cfg->s32MilliSec;
		ret =  cvi_dpu_get_frame(DpuGrp,DpuChn,pstFrameInfo, s32MilliSec);

		break;
	}

	case CVI_DPU_RELEASE_FRAME:
	{
		struct dpu_release_frame_cfg *cfg = (struct dpu_release_frame_cfg *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		DPU_CHN DpuChn = cfg->DpuChn;
		VIDEO_FRAME_INFO_S *pstFrameInfo = &cfg->stFrameInfo;
		ret =  cvi_dpu_release_frame(DpuGrp,DpuChn, pstFrameInfo);
		break;
	}

	case CVI_DPU_SET_CHN_ATTR:
	{
		struct dpu_chn_attr *attr = (struct dpu_chn_attr *)kdata;
		DPU_GRP DpuGrp = attr->DpuGrp;
		DPU_CHN DpuChn = attr->DpuChn;

		const DPU_CHN_ATTR_S *pstChnAttr = &attr->stChnAttr;

		ret = cvi_dpu_set_chn_attr(DpuGrp, DpuChn, pstChnAttr);
		break;
	}

	case CVI_DPU_GET_CHN_ATTR:
	{
		struct dpu_chn_attr *attr = (struct dpu_chn_attr *)kdata;
		DPU_GRP DpuGrp = attr->DpuGrp;
		DPU_CHN DpuChn = attr->DpuChn;
		DPU_CHN_ATTR_S *pstChnAttr = &attr->stChnAttr;

		ret = cvi_dpu_get_chn_attr(DpuGrp, DpuChn, pstChnAttr);
		break;
	}

	case CVI_DPU_ENABLE_CHN:
	{
		struct dpu_chn_cfg *cfg = (struct dpu_chn_cfg *)kdata;
		DPU_GRP DpuGrp = cfg->DpuGrp;
		DPU_CHN DpuChn = cfg->DpuChn;

		ret = cvi_dpu_enable_chn(DpuGrp, DpuChn);
		break;
	}

	case CVI_DPU_DISABLE_CHN:
	{
		struct dpu_chn_cfg *cfg = (struct dpu_chn_cfg *)kdata;

		ret = cvi_dpu_disable_chn(cfg->DpuGrp, cfg->DpuChn);
		break;
	}

	case CVI_DPU_CHECK_REG_READ:
	{
		dpu_check_reg_read();
		break;
	}

	case CVI_DPU_CHECK_REG_WRITE:
	{
		dpu_check_reg_write();
		break;
	}

	case CVI_DPU_GET_SGBM_STATUS:
	{
		getsgbm_status();
		break;
	}

	case CVI_DPU_GET_FGS_STATUS:
	{
		getfgs_status();
		break;
	}


	default:
		CVI_TRACE_DPU(CVI_DBG_DEBUG, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
