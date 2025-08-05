#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "vpss_debug.h"
#include "vpss_sdk_layer.h"
#include "vpss_rgn_ctrl.h"
#include "comm_errno.h"

long vpss_ioctl(struct vpss_cores *cores, unsigned int cmd, unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, ksize;
	struct vpss_ctx *ctx = &cores->ctx;

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
		TRACE_VPSS(DBG_INFO, "copy_from_user failed.\n");
		ret = -EFAULT;
		goto err;
	}

	switch (cmd) {
	case VPSS_SET_MODE:
	{
		const vpss_mode_s *cfg = (vpss_mode_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mode_s);
		ret = vpss_set_mode(cfg, cores);
		break;
	}

	case VPSS_GET_MODE:
	{
		vpss_mode_s *cfg = (vpss_mode_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mode_s);
		ret = vpss_get_mode(cfg, cores);
		break;
	}

	case VPSS_SET_MOD_PARAM:
	{
		const vpss_mod_param_s *cfg = (vpss_mod_param_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mod_param_s);
		ret = vpss_set_mod_param(cfg, ctx);
		break;
	}

	case VPSS_GET_MOD_PARAM:
	{
		vpss_mod_param_s *cfg = (vpss_mod_param_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_mod_param_s);
		ret = vpss_get_mod_param(cfg, ctx);
		break;
	}

	case VPSS_GET_AVAIL_GROUP:
	{
		vpss_grp grp_id;

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		grp_id = vpss_get_available_grp(cores);
		*((vpss_grp *)kdata) = grp_id;
		ret = 0;
		break;
	}

	case VPSS_CREATE_GROUP:
	{
		struct vpss_grp_cfg *cfg = (struct vpss_grp_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_cfg);
		ret = vpss_create_grp(cfg->vpss_grp, &cfg->grp_attr, cores);
		break;
	}

	case VPSS_DESTROY_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_destroy_grp(grp_id, cores);
		break;
	}

	case VPSS_START_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_start_grp(grp_id, ctx);
		break;
	}

	case VPSS_STOP_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_stop_grp(grp_id, ctx);
		break;
	}

	case VPSS_RESET_GROUP:
	{
		vpss_grp grp_id = *((vpss_grp *)kdata);

		CHECK_IOCTL_CMD(cmd, vpss_grp);
		ret = vpss_reset_grp(grp_id, ctx);
		break;
	}

	case VPSS_SET_GRP_ATTR:
	{
		struct vpss_grp_cfg *cfg = (struct vpss_grp_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		const vpss_grp_attr_s *grp_attr = &cfg->grp_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_cfg);
		ret = vpss_set_grp_attr(vpss_grp, grp_attr, ctx);
		break;
	}

	case VPSS_GET_GRP_ATTR:
	{
		struct vpss_grp_cfg *cfg = (struct vpss_grp_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_grp_attr_s *grp_attr = &cfg->grp_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_cfg);
		ret = vpss_get_grp_attr(vpss_grp, grp_attr, ctx);
		break;
	}

	case VPSS_SET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		const vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_crop_cfg);
		ret = vpss_set_grp_crop(vpss_grp, crop_info, ctx);
		break;
	}

	case VPSS_GET_GRP_CROP:
	{
		struct vpss_grp_crop_cfg *cfg = (struct vpss_grp_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_crop_cfg);
		ret = vpss_get_grp_crop(vpss_grp, crop_info, ctx);
		break;
	}

	case VPSS_GET_GRP_FRAME:
	{
		TRACE_VPSS(DBG_WARN, "Not support.\n");
		ret = ERR_VPSS_NOT_SUPPORT;
		break;
	}

	case VPSS_SET_RELEASE_GRP_FRAME:
	{
		TRACE_VPSS(DBG_WARN, "Not support.\n");
		ret = ERR_VPSS_NOT_SUPPORT;
		break;
	}

	case VPSS_SEND_FRAME:
	{
		struct vpss_snd_frm_cfg *cfg = (struct vpss_snd_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		const video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_snd_frm_cfg);
		ret = vpss_send_frame(vpss_grp, video_frame, milli_sec, ctx);
		break;
	}

	case VPSS_SET_GRP_CSC_CFG:
	{
		struct vpss_grp_csc_cfg *cfg = (struct vpss_grp_csc_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_grp_csc_cfg);
		ret = vpss_set_grp_csc(cfg, ctx);
		break;
	}

	case VPSS_SET_CHN_CSC_CFG:
	{
		struct vpss_chn_csc_cfg *cfg = (struct vpss_chn_csc_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_csc_cfg);
		ret = vpss_set_chn_csc(cfg, ctx);
		break;
	}

	case VPSS_SEND_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_send_chn_frame(vpss_grp, vpss_chn, video_frame, milli_sec, ctx);
		break;
	}

	case VPSS_SET_CHN_ATTR:
	{
		struct vpss_chn_cfg *attr = (struct vpss_chn_cfg *)kdata;
		vpss_grp vpss_grp = attr->vpss_grp;
		vpss_chn vpss_chn = attr->vpss_chn;
		const vpss_chn_attr_s *chn_attr_s = &attr->chn_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_cfg);
		ret = vpss_set_chn_attr(vpss_grp, vpss_chn, chn_attr_s, ctx);
		break;
	}

	case VPSS_GET_CHN_ATTR:
	{
		struct vpss_chn_cfg *attr = (struct vpss_chn_cfg *)kdata;
		vpss_grp vpss_grp = attr->vpss_grp;
		vpss_chn vpss_chn = attr->vpss_chn;
		vpss_chn_attr_s *chn_attr_s = &attr->chn_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_cfg);
		ret = vpss_get_chn_attr(vpss_grp, vpss_chn, chn_attr_s, ctx);
		break;
	}

	case VPSS_ENABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_enable_chn(vpss_grp, vpss_chn, ctx);
		break;
	}

	case VPSS_DISABLE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_disable_chn(cfg->vpss_grp, cfg->vpss_chn, ctx);
		break;
	}

	case VPSS_SET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_crop_cfg);
		ret = vpss_set_chn_crop(vpss_grp, vpss_chn, crop_info, ctx);
		break;
	}

	case VPSS_GET_CHN_CROP:
	{
		struct vpss_chn_crop_cfg *cfg = (struct vpss_chn_crop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_crop_info_s *crop_info = &cfg->crop_info;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_crop_cfg);
		ret = vpss_get_chn_crop(vpss_grp, vpss_chn, crop_info, ctx);
		break;
	}

	case VPSS_SET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		rotation_e rotation = cfg->rotation;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_rot_cfg);
		ret = vpss_set_chn_rotation(vpss_grp, vpss_chn, rotation, ctx);
		break;
	}

	case VPSS_GET_CHN_ROTATION:
	{
		struct vpss_chn_rot_cfg *cfg = (struct vpss_chn_rot_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		rotation_e *rotation = &cfg->rotation;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_rot_cfg);
		ret = vpss_get_chn_rotation(vpss_grp, vpss_chn, rotation, ctx);
		break;
	}

	case VPSS_SET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u64 mesh_addr = cfg->mesh_handle;
		const vpss_ldc_attr_s *ldc_attr = &cfg->ldc_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_ldc_cfg);
		ret = vpss_set_chn_ldc_attr(vpss_grp, vpss_chn, ldc_attr, mesh_addr, ctx);
		break;
	}

	case VPSS_GET_CHN_LDC:
	{
		struct vpss_chn_ldc_cfg *cfg = (struct vpss_chn_ldc_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_ldc_attr_s *ldc_attr = &cfg->ldc_attr;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_ldc_cfg);
		ret = vpss_get_chn_ldc_attr(vpss_grp, vpss_chn, ldc_attr, ctx);
		break;
	}

	case VPSS_GET_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		video_frame_info_s *video_frame = &cfg->video_frame;
		s32 milli_sec = cfg->milli_sec;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_get_chn_frame(vpss_grp, vpss_chn, video_frame, milli_sec, ctx);
		break;
	}

	case VPSS_RELEASE_CHN_FRAME:
	{
		struct vpss_chn_frm_cfg *cfg = (struct vpss_chn_frm_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		const video_frame_info_s *video_frame = &cfg->video_frame;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_frm_cfg);
		ret = vpss_release_chn_frame(vpss_grp, vpss_chn, video_frame);
		break;
	}

	case VPSS_SET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 align = cfg->align;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_align_cfg);
		ret = vpss_set_chn_align(vpss_grp, vpss_chn, align, ctx);
		break;
	}

	case VPSS_GET_CHN_ALIGN:
	{
		struct vpss_chn_align_cfg *cfg = (struct vpss_chn_align_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 *align = &cfg->align;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_align_cfg);
		ret = vpss_get_chn_align(vpss_grp, vpss_chn, align, ctx);
		break;
	}

	case VPSS_SET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 y_ratio = cfg->y_ratio;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_yratio_cfg);
		ret = vpss_set_chn_yratio(vpss_grp, vpss_chn, y_ratio, ctx);
		break;
	}

	case VPSS_GET_CHN_YRATIO:
	{
		struct vpss_chn_yratio_cfg *cfg = (struct vpss_chn_yratio_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 *y_ratio = &cfg->y_ratio;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_yratio_cfg);
		ret = vpss_get_chn_yratio(vpss_grp, vpss_chn, y_ratio, ctx);
		break;
	}

	case VPSS_SET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_scale_coef_e coef = cfg->coef;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_coef_level_cfg);
		ret = vpss_set_chn_scale_coef_level(vpss_grp, vpss_chn, coef, ctx);
		break;
	}

	case VPSS_GET_CHN_SCALE_COEFF_LEVEL:
	{
		struct vpss_chn_coef_level_cfg *cfg = (struct vpss_chn_coef_level_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_scale_coef_e *coef = &cfg->coef;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_coef_level_cfg);
		ret = vpss_get_chn_scale_coef_level(vpss_grp, vpss_chn, coef, ctx);
		break;
	}

	case VPSS_SET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_draw_rect_s *draw_rect = &cfg->draw_rect;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_draw_rect_cfg);
		ret = vpss_set_chn_draw_rect(vpss_grp, vpss_chn, draw_rect, ctx);
		break;
	}

	case VPSS_GET_CHN_DRAW_RECT:
	{
		struct vpss_chn_draw_rect_cfg *cfg = (struct vpss_chn_draw_rect_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_draw_rect_s *draw_rect = &cfg->draw_rect;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_draw_rect_cfg);
		ret = vpss_get_chn_draw_rect(vpss_grp, vpss_chn, draw_rect, ctx);
		break;
	}

	case VPSS_SET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_convert_s *convert = &cfg->convert;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_convert_cfg);
		ret = vpss_set_chn_convert(vpss_grp, vpss_chn, convert, ctx);
		break;
	}

	case VPSS_GET_CHN_CONVERT:
	{
		struct vpss_chn_convert_cfg *cfg = (struct vpss_chn_convert_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vpss_convert_s *convert = &cfg->convert;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_convert_cfg);
		ret = vpss_get_chn_convert(vpss_grp, vpss_chn, convert, ctx);
		break;
	}

	case VPSS_SET_COVEREX_CFG:
	{
		struct vpss_coverex_cfg *cfg = (struct vpss_coverex_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_coverex_cfg);
		ret = vpss_set_rgn_coverex_cfg(vpss_grp, vpss_chn, &cfg->rgn_coverex_cfg, ctx);
		break;
	}

	case VPSS_SET_MOSAIC_CFG:
	{
		struct vpss_mosaic_cfg *cfg = (struct vpss_mosaic_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_mosaic_cfg);
		ret = vpss_set_rgn_mosaic_cfg(vpss_grp, vpss_chn, &cfg->rgn_mosaic_cfg, ctx);
		break;
	}

	case VPSS_SET_GOP_CFG:
	{
		struct vpss_gop_cfg *cfg = (struct vpss_gop_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		u32 layer = cfg->layer;

		CHECK_IOCTL_CMD(cmd, struct vpss_gop_cfg);
		ret = vpss_set_rgn_cfg(vpss_grp, vpss_chn, layer, &cfg->rgn_cfg, ctx);
		break;
	}

	case VPSS_SHOW_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_show_chn(vpss_grp, vpss_chn, ctx);
		break;
	}

	case VPSS_HIDE_CHN:
	{
		struct vpss_en_chn_cfg *cfg = (struct vpss_en_chn_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_en_chn_cfg);
		ret = vpss_hide_chn(vpss_grp, vpss_chn, ctx);
		break;
	}

	case VPSS_ATTACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;
		vb_pool pool_id = (vb_pool)cfg->vb_pool;

		CHECK_IOCTL_CMD(cmd, struct vpss_vb_pool_cfg);
		ret = vpss_attach_vb_pool(vpss_grp, vpss_chn, pool_id, ctx);
		break;
	}

	case VPSS_DETACH_VB_POOL:
	{
		struct vpss_vb_pool_cfg *cfg = (struct vpss_vb_pool_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_vb_pool_cfg);
		ret = vpss_detach_vb_pool(vpss_grp, vpss_chn, ctx);
		break;
	}

	case VPSS_TRIGGER_SNAP_FRAME:
	{
		TRACE_VPSS(DBG_WARN, "Not support.\n");
		ret = ERR_VPSS_NOT_SUPPORT;
		break;
	}

	case VPSS_GET_AMP_CTRL:
	{
		struct vpss_proc_amp_ctrl_cfg *cfg = (struct vpss_proc_amp_ctrl_cfg *)kdata;

		CHECK_IOCTL_CMD(cmd, struct vpss_proc_amp_ctrl_cfg);
		ret = vpss_get_proc_amp_ctrl(cfg->type, &cfg->ctrl);
		break;
	}

	case VPSS_GET_AMP_CFG:
	{
		struct vpss_proc_amp_cfg *cfg = (struct vpss_proc_amp_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;

		CHECK_IOCTL_CMD(cmd, struct vpss_proc_amp_cfg);
		ret = vpss_get_proc_amp(vpss_grp, cfg->proc_amp, ctx);
		break;
	}

	case VPSS_GET_ALL_AMP:
	{
		vpss_all_proc_amp_s *cfg = (vpss_all_proc_amp_s *)kdata;

		CHECK_IOCTL_CMD(cmd, vpss_all_proc_amp_s);
		ret = vpss_get_all_proc_amp(cfg, ctx);
		break;
	}

	case VPSS_SET_CHN_WRAP:
	{
		struct vpss_chn_wrap_cfg *cfg = (struct vpss_chn_wrap_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_wrap_cfg);
		ret = vpss_set_chn_bufwrap_attr(vpss_grp, vpss_chn, &cfg->wrap, ctx);
		break;
	}

	case VPSS_GET_CHN_WRAP:
	{
		struct vpss_chn_wrap_cfg *cfg = (struct vpss_chn_wrap_cfg *)kdata;
		vpss_grp vpss_grp = cfg->vpss_grp;
		vpss_chn vpss_chn = cfg->vpss_chn;

		CHECK_IOCTL_CMD(cmd, struct vpss_chn_wrap_cfg);
		ret = vpss_get_chn_bufwrap_attr(vpss_grp, vpss_chn, &cfg->wrap, ctx);
		break;
	}

	case VPSS_STITCH:
	{
		TRACE_VPSS(DBG_WARN, "Not support.\n");
		ret = ERR_VPSS_NOT_SUPPORT;
		break;
	}

	default:
		TRACE_VPSS(DBG_WARN, "unknown cmd(0x%x)\n", cmd);
		break;
	}

	if (out_size && copy_to_user((void __user *)arg, kdata, out_size))
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
