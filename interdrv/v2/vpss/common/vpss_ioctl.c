#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
//#include <linux/module.h>

#include <vpss_cb.h>

#include "vpss_debug.h"
#include "vpss_core.h"
#include "vpss.h"
#include "vpss_ioctl.h"
#include "vpss_rgn_ctrl.h"

long vpss_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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
		CVI_TRACE_VPSS(CVI_DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
	case CVI_VPSS_CREATE_GROUP:
	{
		struct vpss_crt_grp_cfg *cfg =
			(struct vpss_crt_grp_cfg *)kdata;

		ret = vpss_create_grp(cfg->VpssGrp, &cfg->stGrpAttr);
		break;
	}

	case CVI_VPSS_DESTROY_GROUP:
	{
		VPSS_GRP VpssGrp = *((VPSS_GRP *)kdata);

		ret = vpss_destroy_grp(VpssGrp);
		break;
	}

	case CVI_VPSS_START_GROUP:
	{
		struct vpss_str_grp_cfg *cfg = (struct vpss_str_grp_cfg *)kdata;

		ret = vpss_start_grp(cfg->VpssGrp);
		break;
	}

	case CVI_VPSS_STOP_GROUP:
	{
		VPSS_GRP VpssGrp = *((VPSS_GRP *)kdata);

		ret = vpss_stop_grp(VpssGrp);
		break;
	}

	case CVI_VPSS_RESET_GROUP:
	{
		VPSS_GRP VpssGrp = *((VPSS_GRP *)kdata);

		ret = vpss_reset_grp(VpssGrp);
		break;
	}

	case CVI_VPSS_GET_AVAIL_GROUP:
	{
		VPSS_GRP VpssGrp;

		VpssGrp = vpss_get_available_grp();
		*((VPSS_GRP *)kdata) = VpssGrp;
		ret = 0;
		break;
	}

	case CVI_VPSS_SET_GRP_ATTR:
	{
		struct vpss_grp_attr *cfg = (struct vpss_grp_attr *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;

		const VPSS_GRP_ATTR_S *pstGrpAttr = &cfg->stGrpAttr;

		ret = vpss_set_grp_attr(VpssGrp, pstGrpAttr);
		break;
	}

	case CVI_VPSS_GET_GRP_ATTR:
	{
		struct vpss_grp_attr *cfg = (struct vpss_grp_attr *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_GRP_ATTR_S *pstGrpAttr = &cfg->stGrpAttr;

		ret = vpss_get_grp_attr(VpssGrp, pstGrpAttr);
		break;
	}

	case CVI_VPSS_SET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;

		const VPSS_CROP_INFO_S *pstCropInfo = &cfg->stCropInfo;

		ret = vpss_set_grp_crop(VpssGrp, pstCropInfo);
		break;
	}

	case CVI_VPSS_GET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CROP_INFO_S *pstCropInfo = &cfg->stCropInfo;

		ret = vpss_get_grp_crop(VpssGrp, pstCropInfo);
		break;
	}

	case CVI_VPSS_GET_GRP_FRAME:
	{
		struct vpss_grp_frame_cfg *cfg = (struct vpss_grp_frame_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VIDEO_FRAME_INFO_S *pstVideoFrame = &cfg->stVideoFrame;

		ret = vpss_get_grp_frame(VpssGrp, pstVideoFrame);
		break;
	}

	case CVI_VPSS_SET_RELEASE_GRP_FRAME:
	{
		struct vpss_grp_frame_cfg *cfg = (struct vpss_grp_frame_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		const VIDEO_FRAME_INFO_S *pstVideoFrame = &cfg->stVideoFrame;

		ret = vpss_release_grp_frame(VpssGrp, pstVideoFrame);
		break;
	}

	case CVI_VPSS_SEND_FRAME:
	{
		struct vpss_snd_frm_cfg *cfg = (struct vpss_snd_frm_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		const VIDEO_FRAME_INFO_S *pstVideoFrame = &cfg->stVideoFrame;
		s32 s32MilliSec = cfg->s32MilliSec;

		ret = vpss_send_frame(VpssGrp, pstVideoFrame, s32MilliSec);
		break;
	}

	case CVI_VPSS_SET_GRP_CSC_CFG:
	{
		struct vpss_grp_csc_cfg *cfg = (struct vpss_grp_csc_cfg *)kdata;

		ret = vpss_set_grp_csc(cfg);
		break;
	}

	case CVI_VPSS_SET_CHN_CSC_CFG:
	{
		struct vpss_chn_csc_cfg *cfg = (struct vpss_chn_csc_cfg *)kdata;

		ret = vpss_set_chn_csc(cfg);
		break;
	}

	case CVI_VPSS_SEND_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		const VIDEO_FRAME_INFO_S *pstVideoFrame = &cfg->stVideoFrame;
		s32 s32MilliSec = cfg->s32MilliSec;

		ret = vpss_send_chn_frame(VpssGrp, VpssChn, pstVideoFrame, s32MilliSec);
		break;
	}

	case CVI_VPSS_SET_CHN_ATTR:
	{
		struct vpss_chn_attr *attr = (struct vpss_chn_attr *)kdata;
		VPSS_GRP VpssGrp = attr->VpssGrp;
		VPSS_CHN VpssChn = attr->VpssChn;

		const VPSS_CHN_ATTR_S *pstChnAttr = &attr->stChnAttr;

		ret = vpss_set_chn_attr(VpssGrp, VpssChn, pstChnAttr);
		break;
	}

	case CVI_VPSS_GET_CHN_ATTR:
	{
		struct vpss_chn_attr *attr = (struct vpss_chn_attr *)kdata;
		VPSS_GRP VpssGrp = attr->VpssGrp;
		VPSS_CHN VpssChn = attr->VpssChn;
		VPSS_CHN_ATTR_S *pstChnAttr = &attr->stChnAttr;

		ret = vpss_get_chn_attr(VpssGrp, VpssChn, pstChnAttr);
		break;
	}

	case CVI_VPSS_ENABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_enable_chn(VpssGrp, VpssChn);
		break;
	}

	case CVI_VPSS_DISABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;

		ret = vpss_disable_chn(cfg->VpssGrp, cfg->VpssChn);
		break;
	}

	case CVI_VPSS_SET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		const VPSS_CROP_INFO_S *pstCropInfo = &cfg->stCropInfo;

		ret = vpss_set_chn_crop(VpssGrp, VpssChn, pstCropInfo);
		break;
	}

	case CVI_VPSS_GET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_CROP_INFO_S *pstCropInfo = &cfg->stCropInfo;

		ret = vpss_get_chn_crop(VpssGrp, VpssChn, pstCropInfo);
		break;
	}

	case CVI_VPSS_SET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		ROTATION_E enRotation = cfg->enRotation;

		ret = vpss_set_chn_rotation(VpssGrp, VpssChn, enRotation);
		break;
	}

	case CVI_VPSS_GET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		ROTATION_E *penRotation = &cfg->enRotation;

		ret = vpss_get_chn_rotation(VpssGrp, VpssChn, penRotation);
		break;
	}

	case CVI_VPSS_SET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u64 mesh_addr = cfg->meshHandle;

		const VPSS_LDC_ATTR_S *pstLDCAttr = &cfg->stLDCAttr;

		ret = vpss_set_chn_ldc_attr(VpssGrp, VpssChn, pstLDCAttr, mesh_addr);
		break;
	}

	case CVI_VPSS_GET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_LDC_ATTR_S *pstLDCAttr = &cfg->stLDCAttr;

		// vpss_get_chn_rotation(VpssGrp, VpssChn, &cfg->enRotation);

		ret = vpss_get_chn_ldc_attr(VpssGrp, VpssChn, pstLDCAttr);
		break;
	}

	case CVI_VPSS_SET_CHN_FISHEYE:
	{
		struct vpss_chn_fisheye_cfg *cfg = (struct vpss_chn_fisheye_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		ROTATION_E enRotation = cfg->enRotation;
		u64 mesh_addr = cfg->meshHandle;

		const FISHEYE_ATTR_S *pstFishEyeAttr = &cfg->stFishEyeAttr;

		ret = vpss_set_chn_fisheye_attr(VpssGrp, VpssChn, enRotation, pstFishEyeAttr, mesh_addr);
		break;
	}

	case CVI_VPSS_GET_CHN_FISHEYE:
	{
		struct vpss_chn_fisheye_cfg *cfg = (struct vpss_chn_fisheye_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		FISHEYE_ATTR_S *pstFishEyeAttr = &cfg->stFishEyeAttr;

		vpss_get_chn_rotation(VpssGrp, VpssChn, &cfg->enRotation);

		ret = vpss_get_chn_fisheye_attr(VpssGrp, VpssChn, pstFishEyeAttr);
		break;
	}

	case CVI_VPSS_GET_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VIDEO_FRAME_INFO_S *pstFrameInfo = &cfg->stVideoFrame;
		s32 s32MilliSec = cfg->s32MilliSec;

		ret = vpss_get_chn_frame(VpssGrp, VpssChn, pstFrameInfo, s32MilliSec);
		break;
	}

	case CVI_VPSS_RELEASE_CHN_RAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		const VIDEO_FRAME_INFO_S *pstVideoFrame = &cfg->stVideoFrame;

		ret = vpss_release_chn_frame(VpssGrp, VpssChn, pstVideoFrame);
		break;
	}

	case CVI_VPSS_SET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 u32Align = cfg->u32Align;

		ret = vpss_set_chn_align(VpssGrp, VpssChn, u32Align);
		break;
	}

	case CVI_VPSS_GET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 *pu32Align = &cfg->u32Align;

		ret = vpss_get_chn_align(VpssGrp, VpssChn, pu32Align);
		break;
	}

	case CVI_VPSS_SET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 YRatio = cfg->YRatio;

		ret = vpss_set_chn_yratio(VpssGrp, VpssChn, YRatio);
		break;
	}

	case CVI_VPSS_GET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 *pYRatio = &cfg->YRatio;

		ret = vpss_get_chn_yratio(VpssGrp, VpssChn, pYRatio);
		break;
	}

	case CVI_VPSS_SET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_SCALE_COEF_E enCoef = cfg->enCoef;

		ret = vpss_set_chn_scale_coef_level(VpssGrp, VpssChn, enCoef);
		break;
	}

	case CVI_VPSS_GET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_SCALE_COEF_E *penCoef = &cfg->enCoef;

		ret = vpss_get_chn_scale_coef_level(VpssGrp, VpssChn, penCoef);
		break;
	}

	case CVI_VPSS_SET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_DRAW_RECT_S *pstDrawRect = &cfg->stDrawRect;

		ret = vpss_set_chn_draw_rect(VpssGrp, VpssChn, pstDrawRect);
		break;
	}

	case CVI_VPSS_GET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_DRAW_RECT_S *pstDrawRect = &cfg->stDrawRect;

		ret = vpss_get_chn_draw_rect(VpssGrp, VpssChn, pstDrawRect);
		break;
	}

	case CVI_VPSS_SET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_CONVERT_S *pstConvert = &cfg->stConvert;

		ret = vpss_set_chn_convert(VpssGrp, VpssChn, pstConvert);
		break;
	}

	case CVI_VPSS_GET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VPSS_CONVERT_S *pstConvert = &cfg->stConvert;

		ret = vpss_get_chn_convert(VpssGrp, VpssChn, pstConvert);
		break;
	}

	case CVI_VPSS_SET_COVEREX_CFG:
	{
		struct _vpss_coverex_cfg *cfg = (struct _vpss_coverex_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_set_rgn_coverex_cfg(VpssGrp, VpssChn, &cfg->rgn_coverex_cfg);
		break;
	}

	case CVI_VPSS_SET_MOSAIC_CFG:
	{
		struct _vpss_mosaic_cfg *cfg = (struct _vpss_mosaic_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_set_rgn_mosaic_cfg(VpssGrp, VpssChn, &cfg->rgn_mosaic_cfg);
		break;
	}

	case CVI_VPSS_SET_GOP_CFG:
	{
		struct _vpss_gop_cfg *cfg = (struct _vpss_gop_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 layer = cfg->layer;

		ret = vpss_set_rgn_cfg(VpssGrp, VpssChn, layer, &cfg->rgn_cfg);
		break;
	}

	case CVI_VPSS_SHOW_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_show_chn(VpssGrp, VpssChn);
		break;
	}

	case CVI_VPSS_HIDE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_hide_chn(VpssGrp, VpssChn);
		break;
	}

	case CVI_VPSS_ATTACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		VB_POOL hVbPool = (VB_POOL)cfg->hVbPool;

		ret = vpss_attach_vb_pool(VpssGrp, VpssChn, hVbPool);
		break;
	}

	case CVI_VPSS_DETACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;

		ret = vpss_detach_vb_pool(VpssGrp, VpssChn);
		break;
	}

	case CVI_VPSS_TRIGGER_SNAP_FRAME:
	{
		struct vpss_snap_cfg *cfg = (struct vpss_snap_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;
		VPSS_CHN VpssChn = cfg->VpssChn;
		u32 frame_cnt = cfg->frame_cnt;

		ret = vpss_trigger_snap_frame(VpssGrp, VpssChn, frame_cnt);
		break;
	}

	case CVI_VPSS_SET_MOD_PARAM:
	{
		const VPSS_MOD_PARAM_S *cfg = (VPSS_MOD_PARAM_S *)kdata;

		ret = vpss_set_mod_param(cfg);
		break;
	}

	case CVI_VPSS_GET_MOD_PARAM:
	{
		VPSS_MOD_PARAM_S *cfg = (VPSS_MOD_PARAM_S *)kdata;

		ret = vpss_get_mod_param(cfg);
		break;
	}

	case CVI_VPSS_GET_AMP_CTRL:
	{
		struct vpss_proc_amp_ctrl_cfg *cfg = (struct vpss_proc_amp_ctrl_cfg *)kdata;

		ret = vpss_get_proc_amp_ctrl(cfg->type, &cfg->ctrl);
		break;
	}

	case CVI_VPSS_GET_AMP_CFG:
	{
		struct vpss_proc_amp_cfg *cfg = (struct vpss_proc_amp_cfg *)kdata;
		VPSS_GRP VpssGrp = cfg->VpssGrp;

		ret = vpss_get_proc_amp(VpssGrp, cfg->proc_amp);
		break;
	}

	case CVI_VPSS_GET_ALL_AMP:
	{
		struct vpss_all_proc_amp_cfg *cfg = (struct vpss_all_proc_amp_cfg *)kdata;

		ret = vpss_get_all_proc_amp(cfg);
		break;
	}

	case CVI_VPSS_STITCH:
	{
		u32 u32ChnNum;
		VPSS_STITCH_CHN_ATTR_S *pstInput;
		struct _vpss_stitch_cfg *cfg = (struct _vpss_stitch_cfg *)kdata;

		u32ChnNum = cfg->u32ChnNum;
		pstInput = (VPSS_STITCH_CHN_ATTR_S *)vmalloc(sizeof(VPSS_STITCH_CHN_ATTR_S) * u32ChnNum);

		ret = copy_from_user(pstInput, cfg->pstInput, sizeof(VPSS_STITCH_CHN_ATTR_S) * u32ChnNum);
		if (ret != 0) {
			CVI_TRACE_VPSS(CVI_DBG_INFO, "pstInput copy_from_user failed: ret=%d\n", ret);
			vfree(pstInput);
			break;
		}

		ret = vpss_stitch(u32ChnNum, pstInput, &cfg->stOutput, &cfg->stVideoFrame);

		vfree(pstInput);
		break;
	}

	case CVI_VPSS_BM_SEND_FRAME:
	{
		struct _bm_vpss_cfg *cfg = (struct _bm_vpss_cfg *)kdata;

		ret = vpss_bm_send_frame(cfg);
		break;
	}

	default:
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
