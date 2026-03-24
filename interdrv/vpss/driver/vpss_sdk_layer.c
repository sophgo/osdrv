#include "vpss_debug.h"
#include "vpss_core.h"
#include "vpss_ip_ctrl.h"
#include "vpss.h"
#include "vpss_hal.h"
#include "vpss_sdk_layer.h"
#include "comm_buffer.h"
#include "ion.h"
#include "bind.h"
#include "base_cb.h"
#include "vi_cb.h"
#include "venc_cb.h"


/**************************************************************************
 *   internal APIs.
 **************************************************************************/
static void _vpss_grp_param_init(vpss_grp grp_id, struct vpss_grp_ctx *grp_ctx)
{
	unsigned char i, j, k;
	struct vpss_csc_matrix mtrx;

	memset(&grp_ctx->grp_crop_info, 0, sizeof(grp_ctx->grp_crop_info));
	memset(&grp_ctx->frame_crop, 0, sizeof(grp_ctx->frame_crop));
	memset(&grp_ctx->grp_work_status, 0, sizeof(grp_ctx->grp_work_status));

	for (i = 0; i < grp_ctx->chn_max_num; ++i) {
		memset(&grp_ctx->chn_ctxs[i], 0, sizeof(grp_ctx->chn_ctxs[i]));
		grp_ctx->chn_ctxs[i].coef = VPSS_SCALE_COEF_BICUBIC;
		grp_ctx->chn_ctxs[i].align = DEFAULT_ALIGN;
		grp_ctx->chn_ctxs[i].y_ratio = YRATIO_SCALE;
		grp_ctx->chn_ctxs[i].vb_pool = VB_INVALID_POOLID;

		for (j = 0; j < RGN_MAX_LAYER_VPSS; ++j)
			for (k = 0; k < RGN_MAX_NUM_VPSS; ++k)
				grp_ctx->chn_ctxs[i].rgn_handle[j][k] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_COVEREX_MAX_NUM; ++j)
			grp_ctx->chn_ctxs[i].cover_ex_handle[j] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_MOSAIC_MAX_NUM; ++j)
			grp_ctx->chn_ctxs[i].mosaic_handle[j] = RGN_INVALID_HANDLE;
	}

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i) {
		grp_ctx->proc_amp[i] = PROC_AMP_DEFAULT_VALUE;
	}

	// use designer provided table
	vpss_get_csc_mtrx(VPSS_CSC_601_LIMIT_YUV2RGB, &mtrx);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			grp_ctx->csc.coef[i][j] = mtrx.coef[i][j];

		grp_ctx->csc.add[i] = mtrx.add[i];
		grp_ctx->csc.sub[i] = mtrx.sub[i];
	}
	grp_ctx->is_copy_upsample = false;
	grp_ctx->is_cfg_changed = true;
}

static int _notify_vc_sb_mode(u8 grp_id, u8 chn_id, u8 is_online, struct vpss_chn_ctx *chn_ctx)
{
	int i;
	struct venc_sbm_info info;
	struct base_exe_m_cb exe_cb;
	mmf_bind_dest_s bind_dest;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	struct sbm_cfg *pst_sbm_cfg = &info.st_sbm_cfg;

	info.venc_chn = -1;
	if (bind_get_dst(&chn, &bind_dest) == 0) {
		for (i = 0; i < bind_dest.num; ++i) {
			if (bind_dest.mmf_chn[i].mod_id == ID_VENC)
				info.venc_chn = bind_dest.mmf_chn[i].chn_id;
		}
	}

	pst_sbm_cfg->is_online = is_online;
	pst_sbm_cfg->slice_num = chn_ctx->buf_wrap.wrap_buffer_size;
	pst_sbm_cfg->line_num = chn_ctx->buf_wrap.buf_line;
	pst_sbm_cfg->slice_mode = chn_ctx->sbm_ctx.sb_mode;
	pst_sbm_cfg->width = chn_ctx->chn_attr.width;
	pst_sbm_cfg->height = chn_ctx->chn_attr.height;
	for (i = 0; i < 3; i++) {
		pst_sbm_cfg->phy_addr[i] = chn_ctx->sbm_ctx.phy_addr[i];
		pst_sbm_cfg->stride[i] = chn_ctx->sbm_ctx.stride[i];
	}
	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VENC_CB_SET_SBM_INFO;
	exe_cb.data = &info;

	return base_exe_module_cb(&exe_cb);

}

static int _notify_vi_motion_resize(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	struct base_exe_m_cb exe_cb;
	struct vi_cb_msp_cfg msp_cfg;
	mmf_chn_s src_chn;
	mmf_chn_s dest_chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};

	grp_ctx = ctx->grp_ctx[grp_id];
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	msp_cfg.width = chn_ctx->chn_attr.width;
	msp_cfg.height = chn_ctx->chn_attr.height;

	if ((grp_ctx->grp_attr.w != chn_ctx->chn_attr.width) ||
		(grp_ctx->grp_attr.h != chn_ctx->chn_attr.height)) {
		msp_cfg.is_resize = true;
	} else {
		msp_cfg.is_resize = false;
	}

	if (grp_ctx->online_from_isp) {
		msp_cfg.vi_pipe = grp_id;
		msp_cfg.vi_chn = 0;
	} else {
		ret = bind_get_src(&dest_chn, &src_chn);
		if (ret)
			return ERR_VPSS_NOT_SUPPORT;
		if (src_chn.mod_id != ID_VI)
			return ERR_VPSS_NOT_SUPPORT;

		msp_cfg.vi_pipe = src_chn.dev_id;
		msp_cfg.vi_chn = src_chn.chn_id;
	}

	exe_cb.callee = E_MODULE_VI;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VI_CB_MSP_CFG;
	exe_cb.data = &msp_cfg;

	return base_exe_module_cb(&exe_cb);
}

static void _clean_vpss_workq(struct vb_jobs_t *jobs)
{
	int i = 0;
	struct vb_s *vb = NULL;

	if (!jobs || !jobs->inited)
		return;

	osal_mutex_lock(&jobs->lock);
	if (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_FOREACH(vb, &jobs->workq, i) {
			vb->buf.frame_flag = 1;
		}
	}
	osal_mutex_unlock(&jobs->lock);
}

static void _release_vpss_doneq(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	mod_id_e mod_id = ID_VPSS;
	if (!jobs || !jobs->inited)
		return;

	while (1) {
		osal_mutex_lock(&jobs->dlock);
		if (FIFO_EMPTY(&jobs->doneq)) {
			osal_mutex_unlock(&jobs->dlock);
			break;
		}
		FIFO_POP(&jobs->doneq, &vb);
		osal_atomic_fetch_and(~BIT(mod_id), &vb->mod_ids);
		osal_mutex_unlock(&jobs->dlock);
		vb_release_block((vb_blk)(uintptr_t)vb);
	}
}

void _clean_chn_vb_jobs(vpss_chn chn_id, struct vpss_grp_ctx *grp_ctx,
			const vpss_chn_attr_s *chn_attr_old, const vpss_chn_attr_s *chn_attr_new)
{
	if ((chn_attr_old->width != chn_attr_new->width) ||
		(chn_attr_old->height != chn_attr_new->height) ||
		(chn_attr_old->video_format != chn_attr_new->video_format) ||
		(chn_attr_old->pixel_format != chn_attr_new->pixel_format) ||
		(chn_attr_old->mirror != chn_attr_new->mirror) ||
		(chn_attr_old->flip != chn_attr_new->flip) ||
		memcmp(&chn_attr_old->aspect_ratio, &chn_attr_new->aspect_ratio, sizeof(aspect_ratio_s)) ||
		memcmp(&chn_attr_old->normalize, &chn_attr_new->normalize, sizeof(vpss_normalize_s))) {
		_clean_vpss_workq(&grp_ctx->vb_jobs.outs[chn_id]);
		_release_vpss_doneq(&grp_ctx->vb_jobs.outs[chn_id]);
	}

	if (chn_attr_old->depth || (chn_attr_new == 0))
		_release_vpss_doneq(&grp_ctx->vb_jobs.outs[chn_id]);
}


/**************************************************************************
 *   Public APIs.
 **************************************************************************/
int vpss_set_mode(const vpss_mode_s *mode, struct vpss_cores *cores)
{
	osal_spin_lock(&cores->lock);
	vpss_core_set_mode(cores, mode);
	osal_spin_unlock(&cores->lock);

	return 0;
}

int vpss_get_mode(vpss_mode_s *mode, struct vpss_cores *cores)
{
	osal_spin_lock(&cores->lock);
	*mode = cores->vpss_mode;
	osal_spin_unlock(&cores->lock);

	return 0;
}

int vpss_set_mod_param(const vpss_mod_param_s *mod_param_info, struct vpss_ctx *ctx)
{
	int i;

	osal_mutex_lock(&ctx->lock);
	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (ctx->grp_ctx[i]) {
			osal_mutex_unlock(&ctx->lock);
			TRACE_VPSS(DBG_ERR, "Setting module param must be the first step of VPSS.\n");
			return ERR_VPSS_NOT_PERM;
		}

	ctx->mod_param = *mod_param_info;
	osal_mutex_unlock(&ctx->lock);

	return 0;
}

int vpss_get_mod_param(vpss_mod_param_s *mod_param_info, struct vpss_ctx *ctx)
{
	osal_mutex_lock(&ctx->lock);
	*mod_param_info = ctx->mod_param;
	osal_mutex_unlock(&ctx->lock);

	return 0;
}

vpss_grp vpss_get_available_grp(struct vpss_cores *cores)
{
	vpss_grp grp = 0;
	vpss_grp ret = VPSS_INVALID_GRP;
	unsigned char i;
	struct vpss_ctx *ctx = &cores->ctx;

	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		if ((cores->vi_vpss_mode.mode[i] == VI_ONLINE_VPSS_ONLINE) ||
			(cores->vi_vpss_mode.mode[i] == VI_SLICE_VPSS_ONLINE) ||
			(cores->vi_vpss_mode.mode[i] == VI_OFFLINE_VPSS_ONLINE))
			grp = VPSS_ONLINE_NUM;
	}

	osal_mutex_lock(&ctx->lock);
	for (; grp < VPSS_MAX_GRP_NUM; ++grp)
		if (!ctx->grp_used[grp]) {
			ctx->grp_used[grp] = true;
			ret = grp;
			break;
		}
	osal_mutex_unlock(&ctx->lock);

	return ret;
}

int vpss_create_grp(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr, struct vpss_cores *cores)
{
	int job_num = 1;
	unsigned char dev_id;
	unsigned char online_from_isp = false;
	struct vpss_job *job;
	int ret, i;
	struct vpss_ctx *ctx = &cores->ctx;
	struct vpss_grp_ctx *grp_ctx = NULL;
	void *job_buffer = NULL;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_fmt(grp_id, grp_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(grp_attr->pixel_format, grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_size(grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	if (grp_attr->frame_rate.src_frame_rate < grp_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, grp_attr->frame_rate.src_frame_rate
				, grp_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&ctx->lock);

	if (ctx->grp_ctx[grp_id]) {
		osal_mutex_unlock(&ctx->lock);
		TRACE_VPSS(DBG_ERR, "Grp(%d) is occupied\n", grp_id);
		return ERR_VPSS_EXIST;
	}

	dev_id = grp_attr->vpss_dev_id;
	if (cores->vpss_mode.mode == VPSS_MODE_SINGLE) {
		dev_id = 1;
	} else {
		if (dev_id >= VPSS_DEVICE_NUM) {
			dev_id = VPSS_DEVICE_NUM - 1;
			TRACE_VPSS(DBG_WARN, "VPSS Dual mode only allow VpssDev 0/1.\n");
		}
	}

	if ((grp_id < VPSS_ONLINE_NUM) &&
		((cores->vi_vpss_mode.mode[grp_id] == VI_OFFLINE_VPSS_ONLINE) ||
		(cores->vi_vpss_mode.mode[grp_id] == VI_SLICE_VPSS_ONLINE) ||
		(cores->vi_vpss_mode.mode[grp_id] == VI_ONLINE_VPSS_ONLINE))) {
		online_from_isp = true;
		job_num = VPSS_ONLINE_JOB_NUM;
	}

	grp_ctx = osal_kzalloc(sizeof(struct vpss_grp_ctx), OSAL_GFP_ATOMIC);
	if (!grp_ctx) {
		osal_mutex_unlock(&ctx->lock);
		TRACE_VPSS(DBG_ERR, "grp_ctx kzalloc fail.\n");
		return ERR_VPSS_NOMEM;
	}

	job_buffer = osal_kzalloc(sizeof(struct vpss_job) * job_num, OSAL_GFP_ATOMIC);
	if (!job_buffer) {
		osal_mutex_unlock(&ctx->lock);
		osal_kfree(grp_ctx);
		TRACE_VPSS(DBG_ERR, "job kzalloc fail.\n");
		return ERR_VPSS_NOMEM;
	}
	ctx->grp_ctx[grp_id] = grp_ctx;
	grp_ctx->job_buffer = job_buffer;

	FIFO_INIT(&grp_ctx->jobq, job_num);

	//todo
	job = (struct vpss_job *)grp_ctx->job_buffer;
	for (i = 0; i < job_num; i++) {
		job[i].grp_id = grp_id;
		job[i].dev_id = dev_id;
		job[i].data = (void *)grp_ctx;
		job[i].job_cb = vpss_wkup_frame_done_handle;
		osal_spin_lock_init(&job[i].lock);
		osal_atomic_set(&job[i].job_state, JOB_INVALID);
		osal_workqueue_init(&job[i].work, vpss_handle_frame_done);
		FIFO_PUSH(&grp_ctx->jobq, job + i);
	}

	grp_ctx->grp_id = grp_id;
	grp_ctx->dev_id = dev_id;
	grp_ctx->online_from_isp = online_from_isp;
	grp_ctx->chn_max_num = (cores->vpss_mode.mode == VPSS_MODE_SINGLE) ?
		VPSS_MAX_CHN_NUM : ((dev_id == 1) ? VPSS_MAX_CHN_NUM - 1 : 1);
	grp_ctx->hal_ctx_ptr = &cores->hal_ctx;
	grp_ctx->hdl_ctx_ptr = &ctx->hdl_ctx;
	grp_ctx->is_created = true;

	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_STOP);
	osal_mutex_init(&grp_ctx->lock);
	osal_memcpy(&grp_ctx->grp_attr, grp_attr, sizeof(*grp_attr));
	if (online_from_isp)
		base_mod_jobs_init(&grp_ctx->vb_jobs.ins, 0, 0, 0);
	else
		base_mod_jobs_init(&grp_ctx->vb_jobs.ins, 1, 1, 0);

	_vpss_grp_param_init(grp_id, grp_ctx);
	ctx->grp_used[grp_id] = true;

	for (i = 0; i < grp_ctx->chn_max_num; ++i) {
		osal_atomic_set(&grp_ctx->chn_ctxs[i].mesh.gdc_flag, 0);
	}

	osal_mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) max_w(%d) max_h(%d) PixelFmt(%d) online_from_isp(%d)\n",
		grp_id, grp_attr->w, grp_attr->h,
		grp_attr->pixel_format, grp_ctx->online_from_isp);
	return 0;
}

int vpss_destroy_grp(vpss_grp grp_id, struct vpss_cores *cores)
{
	vpss_chn chn_id;
	int ret;
	struct vpss_ctx *ctx = &cores->ctx;
	struct vpss_grp_ctx *grp_ctx;
	int job_num = 1, i;
	struct vpss_job *job;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];

	if (!grp_ctx)
		return 0;

	osal_mutex_lock(&ctx->lock);
	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->is_created = false;
	base_mod_jobs_exit(&grp_ctx->vb_jobs.ins);

	for (chn_id = 0; chn_id < grp_ctx->chn_max_num; ++chn_id) {
		grp_ctx->chn_ctxs[chn_id].rotation = ROTATION_0;
		grp_ctx->chn_ctxs[chn_id].ldc_attr.enable = false;

		if (grp_ctx->chn_ctxs[chn_id].mesh.paddr) {
			grp_ctx->chn_ctxs[chn_id].mesh.paddr = 0;
			grp_ctx->chn_ctxs[chn_id].mesh.vaddr = 0;
		}

		if (grp_ctx->chn_ctxs[chn_id].sbm_ctx.phy_addr[0]) {
			base_ion_free(grp_ctx->chn_ctxs[chn_id].sbm_ctx.phy_addr[0]);
			grp_ctx->chn_ctxs[chn_id].sbm_ctx.phy_addr[0] = 0;
		}
	}

	job_num = grp_ctx->online_from_isp ? VPSS_ONLINE_JOB_NUM : 1;
	job = (struct vpss_job *)grp_ctx->job_buffer;
	for (i = 0; i < job_num; i++) {
		osal_spin_lock_destroy(&job[i].lock);
		osal_workqueue_destroy(&job[i].work);
	}
	FIFO_EXIT(&grp_ctx->jobq);
	osal_kfree(grp_ctx->job_buffer);
	osal_mutex_unlock(&grp_ctx->lock);
	osal_mutex_destroy(&grp_ctx->lock);
	osal_kfree(grp_ctx);

	ctx->grp_ctx[grp_id] = NULL;
	ctx->grp_used[grp_id] = false;
	osal_mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

int vpss_start_grp(vpss_grp grp_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	if (grp_ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) already started.\n", grp_id);
		return 0;
	}

	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->is_started = true;
	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_STOP);
	osal_mutex_unlock(&grp_ctx->lock);

	if (grp_ctx->online_from_isp) {
		ret = vpss_online_prepare(grp_id, ctx);
		if (ret != 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) vpss_online_prepare failed.\n", grp_id);
			grp_ctx->is_started = false;
			return ret;
		}
	} else {
		osal_atomic_inc_return(&ctx->hdl_ctx.active_cnt);
	}
	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

int vpss_stop_grp(vpss_grp grp_id, struct vpss_ctx *ctx)
{
	int ret, i; //Todo: online ???
	struct vpss_job *job;
	unsigned int job_num;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_cores *cores = osal_container_of(ctx, struct vpss_cores, ctx);

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	if (!grp_ctx)
		return 0;
	if (!grp_ctx->is_started)
		return 0;

	if (!grp_ctx->online_from_isp)
		osal_atomic_dec_return(&ctx->hdl_ctx.active_cnt);

	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->is_started = false;

	job_num = grp_ctx->online_from_isp ? VPSS_ONLINE_JOB_NUM : 1;
	job = (struct vpss_job *)grp_ctx->job_buffer;

	for (i = 0; i < job_num; i++) {
		if ((osal_atomic_read(&job[i].job_state) == JOB_WAIT) ||
			(osal_atomic_read(&job[i].job_state) == JOB_WORKING)) {
			vpss_hal_remove_job(job + i, &cores->hal_ctx);
			FIFO_PUSH(&grp_ctx->jobq, job + i);
			release_buffers(grp_ctx);
		}
		osal_atomic_set(&job[i].job_state, JOB_INVALID);
	}

	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_STOP);
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d)\n", grp_id);

	return 0;
}

int vpss_reset_grp(vpss_grp grp_id, struct vpss_ctx *ctx)
{
	int ret;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	return 0;
}

int vpss_get_grp_attr(vpss_grp grp_id, vpss_grp_attr_s *grp_attr, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	osal_mutex_lock(&grp_ctx->lock);
	*grp_attr = grp_ctx->grp_attr;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_grp_attr(vpss_grp grp_id, const vpss_grp_attr_s *grp_attr, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_fmt(grp_id, grp_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(grp_attr->pixel_format, grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_size(grp_attr->w, grp_attr->h);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];

	if (grp_ctx->online_from_isp && grp_ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) operation not allowed if vi-2-vpss online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}
	if (grp_attr->frame_rate.src_frame_rate < grp_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, grp_attr->frame_rate.src_frame_rate
				, grp_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (grp_attr->vpss_dev_id != grp_ctx->grp_attr.vpss_dev_id) {
		TRACE_VPSS(DBG_ERR, "The device id cannot be changed.\n");
		return ERR_VPSS_NOT_PERM;
	}

	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->grp_attr = *grp_attr;
	grp_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) max_w(%d) max_h(%d) PixelFmt(%d) online_from_isp(%d)\n",
		grp_id, grp_attr->w, grp_attr->h,
		grp_attr->pixel_format, grp_ctx->online_from_isp);

	return 0;
}

int vpss_get_grp_crop(vpss_grp grp_id, vpss_crop_info_s *crop_info, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	osal_mutex_lock(&grp_ctx->lock);
	*crop_info = grp_ctx->grp_crop_info;
	osal_mutex_unlock(&grp_ctx->lock);
	return 0;
}

int vpss_set_grp_crop(vpss_grp grp_id, const vpss_crop_info_s *crop_info, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];

	if (grp_ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not support crop if online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}

	if (crop_info->enable) {
		if ((crop_info->crop_rect.x < 0) || (crop_info->crop_rect.y < 0)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop start-point(%d %d) illegal\n"
				, grp_id, crop_info->crop_rect.x, crop_info->crop_rect.y);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((crop_info->crop_rect.width < VPSS_MIN_IMAGE_WIDTH) ||
			(crop_info->crop_rect.height < VPSS_MIN_IMAGE_HEIGHT)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop size(%d %d) can't smaller than %dx%d\n"
				, grp_id, crop_info->crop_rect.width, crop_info->crop_rect.height
				, VPSS_MIN_IMAGE_WIDTH, VPSS_MIN_IMAGE_HEIGHT);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((crop_info->crop_rect.y + crop_info->crop_rect.height)
			> grp_ctx->grp_attr.h
		 || (crop_info->crop_rect.x + crop_info->crop_rect.width)
			 > grp_ctx->grp_attr.w) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) crop rect(%d %d %d %d) out of grp size(%d %d)\n"
				, grp_id, crop_info->crop_rect.x, crop_info->crop_rect.y
				, crop_info->crop_rect.width, crop_info->crop_rect.height
				, grp_ctx->grp_attr.w, grp_ctx->grp_attr.h);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->grp_crop_info = *crop_info;
	if (crop_info->enable) {
		bool chk_width_even = IS_FMT_YUV420(grp_ctx->grp_attr.pixel_format) ||
				      IS_FMT_YUV422(grp_ctx->grp_attr.pixel_format);
		bool chk_height_even = IS_FMT_YUV420(grp_ctx->grp_attr.pixel_format);

		if (chk_width_even && (crop_info->crop_rect.width & 0x01)) {
			grp_ctx->grp_crop_info.crop_rect.width &= ~(0x0001);
			TRACE_VPSS(DBG_WARN, "Grp(%d) crop_rect.width(%d) to even(%d) due to YUV\n",
				       grp_id, crop_info->crop_rect.width,
				       grp_ctx->grp_crop_info.crop_rect.width);
		}
		if (chk_height_even && (crop_info->crop_rect.height & 0x01)) {
			grp_ctx->grp_crop_info.crop_rect.height &= ~(0x0001);
			TRACE_VPSS(DBG_WARN, "Grp(%d) crop_rect.height(%d) to even(%d) due to YUV\n",
				       grp_id, crop_info->crop_rect.height,
				       grp_ctx->grp_crop_info.crop_rect.height);
		}
	}

	grp_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d), enable=%d, rect(%d %d %d %d)\n",
		grp_id, crop_info->enable,
		crop_info->crop_rect.x, crop_info->crop_rect.y,
		crop_info->crop_rect.width, crop_info->crop_rect.height);

	return 0;
}

int vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg, struct vpss_ctx *ctx)
{
	int ret;
	vpss_grp grp_id = cfg->vpss_grp;
	unsigned char i, j;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];

	osal_mutex_lock(&grp_ctx->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			grp_ctx->csc.coef[i][j] = cfg->coef[i][j];

		grp_ctx->csc.add[i] = cfg->add[i];
		grp_ctx->csc.sub[i] = cfg->sub[i];
	}
	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		grp_ctx->proc_amp[i] = cfg->proc_amp[i];
	grp_ctx->is_copy_upsample = cfg->is_copy_upsample;
	grp_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d)\n", grp_id);

	return 0;
}

int vpss_get_proc_amp_ctrl(proc_amp_e type, proc_amp_ctrl_s *ctrl)
{
	if (type >= PROC_AMP_MAX) {
		TRACE_VPSS(DBG_ERR, "ProcAmp type(%d) invalid.\n", type);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	ctrl->minimum = PROC_AMP_MIN_VALUE;
	ctrl->maximum = PROC_AMP_MAX_VALUE;
	ctrl->step = PROC_AMP_STEP;
	ctrl->default_value = PROC_AMP_DEFAULT_VALUE;

	return 0;
}

int vpss_get_proc_amp(vpss_grp grp_id, int *proc_amp, struct vpss_ctx *ctx)
{
	int ret;
	unsigned char i;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		proc_amp[i] = ctx->grp_ctx[grp_id]->proc_amp[i];

	return 0;
}

int vpss_get_all_proc_amp(vpss_all_proc_amp_s *cfg, struct vpss_ctx *ctx)
{
	unsigned char i, j;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j) {
			if (ctx->grp_ctx[i])
				cfg->proc_amp[i][j] = ctx->grp_ctx[i]->proc_amp[j];
			else
				cfg->proc_amp[i][j] = PROC_AMP_DEFAULT_VALUE;
		}

	return 0;
}

int vpss_send_frame(vpss_grp grp_id, const video_frame_info_s *video_frame, int milli_sec, struct vpss_ctx *ctx)
{
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vb_blk blk;
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	if (grp_ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not support if online\n", grp_id);
		return ERR_VPSS_NOT_PERM;
	}

	if (!grp_ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (grp_ctx->grp_attr.pixel_format != video_frame->video_frame.pixel_format) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) PixelFormat(%d) mismatch.\n"
			, grp_id, video_frame->video_frame.pixel_format);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((grp_ctx->grp_attr.w != video_frame->video_frame.width)
	 || (grp_ctx->grp_attr.h != video_frame->video_frame.height)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Size(%d * %d) mismatch.\n"
			, grp_id, video_frame->video_frame.width, video_frame->video_frame.height);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(video_frame->video_frame)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) frame offset (%d %d %d %d) invalid\n",
			grp_id, video_frame->video_frame.offset_left, video_frame->video_frame.offset_right,
			video_frame->video_frame.offset_top, video_frame->video_frame.offset_bottom);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FMT_YUV420(grp_ctx->grp_attr.pixel_format)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid width\n", grp_id);
			TRACE_VPSS(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				video_frame->video_frame.width, video_frame->video_frame.offset_left,
				video_frame->video_frame.offset_right);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if ((video_frame->video_frame.height - video_frame->video_frame.offset_top -
		     video_frame->video_frame.offset_bottom) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid height\n", grp_id);
			TRACE_VPSS(DBG_ERR, "height(%d) offset_top(%d) offset_bottom(%d)\n",
				video_frame->video_frame.height, video_frame->video_frame.offset_top,
				video_frame->video_frame.offset_bottom);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(grp_ctx->grp_attr.pixel_format)) {
		if ((video_frame->video_frame.width - video_frame->video_frame.offset_left -
		     video_frame->video_frame.offset_right) & 0x01) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) YUV422 can't accept odd frame valid width\n", grp_id);
			TRACE_VPSS(DBG_ERR, "width(%d) offset_left(%d) offset_right(%d)\n",
				video_frame->video_frame.width, video_frame->video_frame.offset_left,
				video_frame->video_frame.offset_right);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(video_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) no space for malloc.\n", grp_id);
			return ERR_VPSS_NOMEM;
		}
	} else {
		unsigned int usr_cnt;

		vb_inquire_user_cnt(blk, &usr_cnt);
		if (usr_cnt == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d), The released frame cannot be used.\n", grp_id);
			return ERR_VPSS_NOT_PERM;
		}
	}

	if (base_fill_videoframe2buffer(chn, video_frame, &((struct vb_s *)(uintptr_t)blk)->buf) != 0) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Invalid parameter\n", grp_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpss_grp_qbuf(chn, blk, ctx) != 0) {
		TRACE_VPSS(DBG_ERR, "vpss_grp_qbuf(%d) failed\n", grp_id);
		return ERR_VPSS_BUSY;
	}

	TRACE_VPSS(DBG_DEBUG, "Grp(%d), phy-address(0x%llx)\n",
			grp_id, video_frame->video_frame.phyaddr[0]);

	return 0;
}

//chn api
int vpss_set_chn_attr(vpss_grp grp_id, vpss_chn chn_id, const vpss_chn_attr_s *chn_attr, struct vpss_ctx *ctx)
{
	vb_cal_config_s vb_cal_config;
	struct vpss_csc_matrix mtrx;
	int ret;
	unsigned char i, j;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_fmt(grp_id, chn_id, chn_attr->pixel_format);
	if (ret != 0)
		return ret;

	ret = check_yuv_param(chn_attr->pixel_format, chn_attr->width, chn_attr->height);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_size(chn_id, grp_ctx->dev_id, chn_attr->width, chn_attr->height);
	if (ret != 0)
		return ret;

	if (chn_attr->aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
		const rect_s *rect = &chn_attr->aspect_ratio.video_rect;

		if (!chn_attr->aspect_ratio.enable_bgcolor) {
			ret = check_yuv_param(chn_attr->pixel_format, rect->width, rect->height);
			if (ret != 0)
				return ret;
			if ((IS_FMT_YUV420(chn_attr->pixel_format) || IS_FMT_YUV422(chn_attr->pixel_format))
				&& (rect->x & 0x01)) {
				TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
				TRACE_VPSS(DBG_ERR, "YUV_420/YUV_422 rect x(%d) should be even.\n",
					rect->x);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		}

		if ((rect->x < 0) || (rect->y < 0)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect pos(%d %d) can't be negative.\n"
				, grp_id, chn_id, rect->x, rect->y);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->width < VPSS_MIN_IMAGE_WIDTH) || (rect->height < VPSS_MIN_IMAGE_HEIGHT)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect size(%d %d) can't smaller than %dx%d\n"
				, grp_id, chn_id, rect->width, rect->height
				, VPSS_MIN_IMAGE_WIDTH, VPSS_MIN_IMAGE_HEIGHT);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->x + rect->width > chn_attr->width)
		|| (rect->y + rect->height > chn_attr->height)) {
			TRACE_VPSS(DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) rect(%d %d %d %d) output-size(%d %d).\n"
					, grp_id, chn_id
					, rect->x, rect->y, rect->width, rect->height
					, chn_attr->width, chn_attr->height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	if (chn_attr->frame_rate.src_frame_rate < chn_attr->frame_rate.dst_frame_rate) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) Chn(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, grp_id, chn_id, chn_attr->frame_rate.src_frame_rate
				, chn_attr->frame_rate.dst_frame_rate);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	common_getpicbufferconfig(chn_attr->width, chn_attr->height,
		chn_attr->pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);

	osal_mutex_lock(&grp_ctx->lock);
	_clean_chn_vb_jobs(chn_id, grp_ctx, &grp_ctx->chn_ctxs[chn_id].chn_attr, chn_attr);
	osal_memcpy(&grp_ctx->chn_ctxs[chn_id].chn_attr, chn_attr,
		sizeof(grp_ctx->chn_ctxs[chn_id].chn_attr));
	grp_ctx->chn_ctxs[chn_id].blk_size = vb_cal_config.vb_size;
	grp_ctx->chn_ctxs[chn_id].align = DEFAULT_ALIGN;
	grp_ctx->chn_ctxs[chn_id].is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	// use designer provided table
	if (chn_attr->pixel_format == PIXEL_FORMAT_YUV_400)
		vpss_get_csc_mtrx(VPSS_CSC_NONE, &mtrx);
	else
		vpss_get_csc_mtrx(VPSS_CSC_601_LIMIT_RGB2YUV, &mtrx);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			grp_ctx->chn_ctxs[chn_id].csc.coef[i][j] = mtrx.coef[i][j];

		grp_ctx->chn_ctxs[chn_id].csc.add[i] = mtrx.add[i];
		grp_ctx->chn_ctxs[chn_id].csc.sub[i] = mtrx.sub[i];
	}

	//Only chn1 support motion resize
	if (chn_id == 1)
		_notify_vi_motion_resize(grp_id, chn_id, ctx);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d) width(%d), height(%d)\n"
		, grp_id, chn_id, chn_attr->width, chn_attr->height);

	return 0;
}

int vpss_get_chn_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_chn_attr_s *chn_attr, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(chn_attr, &grp_ctx->chn_ctxs[chn_id].chn_attr, sizeof(*chn_attr));
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_enable_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	unsigned int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (chn_ctx->is_enabled) {
		TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) already enabled\n", grp_id, chn_id);
		return 0;
	}

	osal_mutex_lock(&ctx->lock);
	if (grp_ctx->online_from_isp)
		base_mod_jobs_init(&grp_ctx->vb_jobs.outs[chn_id], 1, VPSS_ONLINE_JOB_NUM, chn_ctx->chn_attr.depth);
	else
		base_mod_jobs_init(&grp_ctx->vb_jobs.outs[chn_id], 1, 1, chn_ctx->chn_attr.depth);
	chn_ctx->is_enabled = true;
	osal_mutex_unlock(&ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

int vpss_disable_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	struct vpss_chn_work_status_s *chn_status;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (!chn_ctx->is_enabled) {
		TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) not enabled yet\n", grp_id, chn_id);
		return 0;
	}

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->is_enabled = false;
	chn_ctx->is_drop = false;
	chn_status = &chn_ctx->chn_work_status;
	chn_status->send_ok = 0;
	chn_status->prev_time = 0;
	chn_status->frame_num = 0;
	chn_status->real_frame_rate = 0;
	if (!chn_ctx->buf_wrap.enable)
		vpss_chn_cancel_block(grp_ctx, chn_id);
	base_mod_jobs_exit(&grp_ctx->vb_jobs.outs[chn_id]);
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

int vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg, struct vpss_ctx *ctx)
{
	int ret;
	vpss_grp grp_id = cfg->vpss_grp;
	vpss_chn chn_id = cfg->vpss_chn;
	unsigned char i, j;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			chn_ctx->csc.coef[i][j] = cfg->coef[i][j];

		chn_ctx->csc.add[i] = cfg->add[i];
		chn_ctx->csc.sub[i] = cfg->sub[i];
	}
	grp_ctx->chn_ctxs[chn_id].is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);

	return 0;
}

int vpss_set_chn_crop(vpss_grp grp_id, vpss_chn chn_id, const vpss_crop_info_s *crop_info, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (crop_info->enable) {
		if ((crop_info->crop_rect.width < 4) || (crop_info->crop_rect.height < 1)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) crop size(%d %d) can't smaller than 4x1\n"
				, grp_id, chn_id, crop_info->crop_rect.width
				, crop_info->crop_rect.height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if (crop_info->crop_rect.x + (int)crop_info->crop_rect.width < 4
			|| crop_info->crop_rect.y + (int)crop_info->crop_rect.height < 1) {
			TRACE_VPSS(DBG_ERR
				, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
				, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
				, crop_info->crop_rect.width, crop_info->crop_rect.height);
			return ERR_VPSS_ILLEGAL_PARAM;
		}

		if (grp_ctx->grp_crop_info.enable) {
			if ((int)grp_ctx->grp_crop_info.crop_rect.height
				- crop_info->crop_rect.y < 1
				|| (int)grp_ctx->grp_crop_info.crop_rect.width
				- crop_info->crop_rect.x < 4) {
				TRACE_VPSS(DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
					, crop_info->crop_rect.width, crop_info->crop_rect.height);
				TRACE_VPSS(DBG_ERR, "grp crop size(%d %d)\n"
					, grp_ctx->grp_crop_info.crop_rect.width
					, grp_ctx->grp_crop_info.crop_rect.height);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		} else {
			if ((int)grp_ctx->grp_attr.h - crop_info->crop_rect.y < 1
				|| (int)grp_ctx->grp_attr.w - crop_info->crop_rect.x < 4) {
				TRACE_VPSS(DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, grp_id, chn_id, crop_info->crop_rect.x, crop_info->crop_rect.y
					, crop_info->crop_rect.width, crop_info->crop_rect.height);
				TRACE_VPSS(DBG_ERR, "out of grp size(%d %d)\n"
					, grp_ctx->grp_attr.w, grp_ctx->grp_attr.h);
				return ERR_VPSS_ILLEGAL_PARAM;
			}
		}
	}

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->crop_info = *crop_info;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);
	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d), enable=%d, rect(%d %d %d %d)\n",
		grp_id, chn_id, crop_info->enable,
		crop_info->crop_rect.x, crop_info->crop_rect.y,
		crop_info->crop_rect.width, crop_info->crop_rect.height);

	return 0;
}

int vpss_get_chn_crop(vpss_grp grp_id, vpss_chn chn_id, vpss_crop_info_s *crop_info, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*crop_info = chn_ctx->crop_info;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_show_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->is_muted = false;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_hide_chn(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->is_muted = true;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_send_chn_frame(vpss_grp grp_id, vpss_chn chn_id
	, const video_frame_info_s *video_frame, int milli_sec, struct vpss_ctx *ctx)
{
	int ret;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	vb_blk blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (!grp_ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (!chn_ctx->is_enabled) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", grp_id, chn_id);
		return ERR_VPSS_NOTREADY;
	}
	if (chn_ctx->chn_attr.pixel_format != video_frame->video_frame.pixel_format) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) PixelFormat(%d) mismatch.\n"
			, grp_id, chn_id, video_frame->video_frame.pixel_format);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((chn_ctx->chn_attr.width != video_frame->video_frame.width)
	 || (chn_ctx->chn_attr.height != video_frame->video_frame.height)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Size(%d * %d) mismatch.\n"
			, grp_id, chn_id, video_frame->video_frame.width, video_frame->video_frame.height);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	UNUSED(milli_sec);

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		blk = vb_create_block(video_frame->video_frame.phyaddr[0], NULL, VB_EXTERNAL_POOLID, true);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) no space for malloc.\n", grp_id);
			return ERR_VPSS_NOMEM;
		}
	} else {
		unsigned int usr_cnt;

		vb_inquire_user_cnt(blk, &usr_cnt);
		if (usr_cnt == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), The released frame cannot be used.\n", grp_id, chn_id);
			return ERR_VPSS_NOT_PERM;
		}
	}

	if (base_fill_videoframe2buffer(chn, video_frame, &((struct vb_s *)(uintptr_t)blk)->buf) != 0) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	jobs = &grp_ctx->vb_jobs.outs[chn_id];
	vb = (struct vb_s *)(uintptr_t)blk;
	osal_mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", grp_id, chn_id);
		osal_mutex_unlock(&jobs->lock);
		return -1;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	vb_add_tag(blk, chn.mod_id);
	osal_atomic_inc_return(&vb->usr_cnt);
	osal_mutex_unlock(&jobs->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return ret;
}

int vpss_get_chn_frame(vpss_grp grp_id, vpss_chn chn_id, video_frame_info_s *frame_info,
			   int milli_sec, struct vpss_ctx *ctx)
{
	int ret, i;
	vb_blk blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (!grp_ctx->is_started) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) not yet started.\n", grp_id);
		return ERR_VPSS_NOTREADY;
	}
	if (!chn_ctx->is_enabled) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", grp_id, chn_id);
		return ERR_VPSS_NOTREADY;
	}

	memset(frame_info, 0, sizeof(*frame_info));
	ret = base_get_chn_buffer(chn, &grp_ctx->vb_jobs.outs[chn_id], &blk, milli_sec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) Chn(%d) get chn frame null, milli_sec=%d, ret=%d\n",
				grp_id, chn_id, milli_sec, ret);
		return ERR_VPSS_BUF_EMPTY;
	}

	vb = (struct vb_s *)(uintptr_t)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.width) {
		TRACE_VPSS(DBG_ERR, "buf already released\n");
		return ERR_VPSS_BUF_EMPTY;
	}

	frame_info->video_frame.pixel_format = chn_ctx->chn_attr.pixel_format;
	frame_info->video_frame.width = vb->buf.size.width;
	frame_info->video_frame.height = vb->buf.size.height;
	frame_info->video_frame.time_ref = vb->buf.frm_num;
	frame_info->video_frame.pts = vb->buf.pts;
	frame_info->video_frame.frame_flag = vb->buf.frame_flag;
	for (i = 0; i < 3; ++i) {
		frame_info->video_frame.phyaddr[i] = vb->buf.phy_addr[i];
		frame_info->video_frame.length[i] = vb->buf.length[i];
		frame_info->video_frame.stride[i] = vb->buf.stride[i];
	}

	frame_info->video_frame.offset_top = vb->buf.offset_top;
	frame_info->video_frame.offset_bottom = vb->buf.offset_bottom;
	frame_info->video_frame.offset_left = vb->buf.offset_left;
	frame_info->video_frame.offset_right = vb->buf.offset_right;
	frame_info->video_frame.private_data = vb;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) end to set frame_info width:%d height:%d buf:0x%llx\n"
			, grp_id, chn_id, frame_info->video_frame.width, frame_info->video_frame.height,
			frame_info->video_frame.phyaddr[0]);
	return 0;
}

int vpss_release_chn_frame(vpss_grp grp_id, vpss_chn chn_id,
		const video_frame_info_s *video_frame)
{
	vb_blk blk;

	blk = vb_phys_addr2handle(video_frame->video_frame.phyaddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (video_frame->video_frame.private_data == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , grp_id, chn_id, (unsigned long long)video_frame->video_frame.phyaddr[0]);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (vb_blk)(uintptr_t)video_frame->video_frame.private_data;
	}

	if (vb_release_block(blk) != 0)
		return -1;

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) buf:0x%llx\n",
			grp_id, chn_id, video_frame->video_frame.phyaddr[0]);
	return 0;
}

int vpss_set_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e rotation, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	struct gdc_mesh *pmesh;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];
	pmesh = &chn_ctx->mesh;

	ret = check_vpss_gdc_fmt(grp_id, chn_id, chn_ctx->chn_attr.pixel_format);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&grp_ctx->lock);
	pmesh->paddr = DEFAULT_MESH_PADDR;
	chn_ctx->rotation = rotation;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) rotation(%d).\n",
			grp_id, chn_id, rotation);

	return 0;
}

int vpss_get_chn_rotation(vpss_grp grp_id, vpss_chn chn_id, rotation_e *rotation, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*rotation = chn_ctx->rotation;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int align, struct vpss_ctx *ctx)
{
	int ret;
	vb_cal_config_s vb_cal_config;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	if (align > MAX_ALIGN) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) alignment(%d) exceeding the maximum value %d\n",
			grp_id, chn_id, align, MAX_ALIGN);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	common_getpicbufferconfig(chn_ctx->chn_attr.width,
		chn_ctx->chn_attr.height,
		chn_ctx->chn_attr.pixel_format,
		DATA_BITWIDTH_8, COMPRESS_MODE_NONE, align, &vb_cal_config);

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->blk_size = vb_cal_config.vb_size;
	chn_ctx->align = align;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) align:%d\n", grp_id, chn_id, align);
	return 0;
}

int vpss_get_chn_align(vpss_grp grp_id, vpss_chn chn_id, unsigned int *palign, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*palign = chn_ctx->align;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id,
		vpss_scale_coef_e coef, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	if (coef >= VPSS_SCALE_COEF_MAX) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) undefined scale_coef type(%d)\n"
			, grp_id, chn_id, coef);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->coef = coef;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_get_chn_scale_coef_level(vpss_grp grp_id, vpss_chn chn_id,
		vpss_scale_coef_e *coef, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*coef = chn_ctx->coef;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_draw_rect_s *draw_rect, struct vpss_ctx *ctx)
{
	int i, ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		if (!draw_rect->rects[i].enable)
			continue;
		if (draw_rect->rects[i].rect.width < (2 * draw_rect->rects[i].thick)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), Width less than 2 times thickness.\n",
				grp_id, chn_id);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
		if (draw_rect->rects[i].rect.height < (2 * draw_rect->rects[i].thick)) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), Height less than 2 times thickness.\n",
				grp_id, chn_id);
			return ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->draw_rect = *draw_rect;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_get_chn_draw_rect(vpss_grp grp_id, vpss_chn chn_id,
		vpss_draw_rect_s *draw_rect, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*draw_rect = chn_ctx->draw_rect;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_chn_convert(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_convert_s *convert, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->convert = *convert;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_get_chn_convert(vpss_grp grp_id, vpss_chn chn_id,
		vpss_convert_s *convert, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*convert = chn_ctx->convert;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

/* VPSS_SetChnYRatio: Modify the y ratio of chn output. Only work for yuv format.
 *
 * @param grp_id: The Vpss Grp to work.
 * @param chn_id: The Vpss Chn to work.
 * @param y_ratio: Output's Y will be sacled by this ratio.
 * @return: 0 if OK.
 */
int vpss_set_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int y_ratio, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (!IS_FMT_YUV(chn_ctx->chn_attr.pixel_format)) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) isn't YUV format. Can't apply this setting.\n"
			, grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (chn_ctx->chn_attr.normalize.enable) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Y-ratio adjustment can't work with normalize.\n"
			, grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (y_ratio > YRATIO_SCALE) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) y_ratio(%d) out of range(0-%d).\n"
			, grp_id, chn_id, y_ratio, YRATIO_SCALE);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->y_ratio = y_ratio;
	chn_ctx->is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}

int vpss_get_chn_yratio(vpss_grp grp_id, vpss_chn chn_id, unsigned int *y_ratio, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*y_ratio = chn_ctx->y_ratio;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

int vpss_set_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id,
				const vpss_ldc_attr_s *ldc_attr, unsigned long long mesh_addr, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	struct gdc_mesh *pmesh;
	unsigned long long paddr_old;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];
	pmesh = &chn_ctx->mesh;

	ret = check_vpss_gdc_fmt(grp_id, chn_id, chn_ctx->chn_attr.pixel_format);
	if (ret != 0)
		return ret;

	osal_mutex_lock(&grp_ctx->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = mesh_addr;
	}
	pmesh->paddr = mesh_addr;
	pmesh->vaddr = NULL;

	chn_ctx->ldc_attr = *ldc_attr;
	osal_mutex_unlock(&grp_ctx->lock);

	(void)paddr_old;
	//if (paddr_old)
	//	SYS_IonFree(paddr_old, vaddr_old);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
			, grp_id, chn_id, (unsigned long long)mesh_addr);
	TRACE_VPSS(DBG_DEBUG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d, rotation=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.xy_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio,
			ldc_attr->attr.rotation);

	return 0;
}

int vpss_get_chn_ldc_attr(vpss_grp grp_id, vpss_chn chn_id, vpss_ldc_attr_s *ldc_attr, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	osal_memcpy(ldc_attr, &chn_ctx->ldc_attr, sizeof(*ldc_attr));
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "enable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d\n",
			ldc_attr->enable, ldc_attr->attr.aspect,
			ldc_attr->attr.xy_ratio, ldc_attr->attr.center_x_offset,
			ldc_attr->attr.center_y_offset, ldc_attr->attr.distortion_ratio);

	return 0;
}

int vpss_attach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, vb_pool vb_pool, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->vb_pool = vb_pool;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) attach vb pool(%d)\n",
		grp_id, chn_id, vb_pool);
	return 0;
}

int vpss_detach_vb_pool(vpss_grp grp_id, vpss_chn chn_id, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->vb_pool = VB_INVALID_POOLID;
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d)\n", grp_id, chn_id);
	return 0;
}


int vpss_set_chn_bufwrap_attr(vpss_grp grp_id, vpss_chn chn_id,
		const vpss_chn_buf_wrap_s *buf_wrap, struct vpss_ctx *ctx)
{
	int ret, i;
	void *ion_vaddr = NULL;
	uint64_t ion_paddr = 0, mem_base;
	char ion_name[64];
	int wrap_ion_size;
	unsigned int wrap_height;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	vb_cal_config_s vb_cal_config;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	if (chn_ctx->is_enabled) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d), Channel already enabled\n", grp_id, chn_id);
		return ERR_VPSS_NOT_SUPPORT;
	}

	if (!buf_wrap->enable) {
		osal_mutex_lock(&grp_ctx->lock);
		chn_ctx->buf_wrap = *buf_wrap;
		chn_ctx->is_cfg_changed = true;
		if (chn_ctx->sbm_ctx.phy_addr[0]) {
			base_ion_free(chn_ctx->sbm_ctx.phy_addr[0]);
			chn_ctx->sbm_ctx.phy_addr[0] = 0;
		}
		osal_mutex_unlock(&grp_ctx->lock);
		return 0;
	}

	if (chn_ctx->buf_wrap.enable) {
		TRACE_VPSS(DBG_WARN, "Grp(%d) Chn(%d) sbm already enabled\\n", grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (buf_wrap->buf_line != SB_LINE_64 && buf_wrap->buf_line != SB_LINE_128) {
		TRACE_VPSS(DBG_ERR, "Only support 64 or 128 lines, u32BufLine(%d)\n",
				buf_wrap->buf_line);
		return ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((buf_wrap->wrap_buffer_size < 2) || (buf_wrap->wrap_buffer_size > 32)) {
		TRACE_VPSS(DBG_ERR, "u32WrapBufferSize(%d) range(2, 32)\n",
				buf_wrap->wrap_buffer_size);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	if (chn_ctx->chn_attr.pixel_format != PIXEL_FORMAT_NV12) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) isn't NV12 format. Can't apply this setting.\n"
			, grp_id, chn_id);
		return ERR_VPSS_ILLEGAL_PARAM;
	}

	osal_mutex_lock(&ctx->lock);
	osal_mutex_lock(&grp_ctx->lock);
	chn_ctx->buf_wrap = *buf_wrap;
	chn_ctx->is_cfg_changed = true;

	if (buf_wrap->enable) {
		wrap_height = buf_wrap->buf_line * buf_wrap->wrap_buffer_size;
		common_getpicbufferconfig(chn_ctx->chn_attr.width, wrap_height,
			chn_ctx->chn_attr.pixel_format, DATA_BITWIDTH_8,
			COMPRESS_MODE_NONE, chn_ctx->align, &vb_cal_config);
		wrap_ion_size = vb_cal_config.vb_size;

		sprintf(ion_name, "VpssGrp%dChn%dWrapBuf", grp_id, chn_id);
		ret = base_ion_alloc(&ion_paddr, (void *)&ion_vaddr, (uint8_t *)ion_name, wrap_ion_size, true);
		if (!ion_paddr) {
			TRACE_VPSS(DBG_ERR, "allocate wrap buffer failed\n");
			osal_mutex_unlock(&grp_ctx->lock);
			osal_mutex_unlock(&ctx->lock);
			return ERR_VPSS_NOMEM;
		}

		mem_base = ion_paddr;
		for (i = 0; i < vb_cal_config.plane_num; ++i) {
			chn_ctx->sbm_ctx.phy_addr[i] = mem_base;
			chn_ctx->sbm_ctx.stride[i] = i ? vb_cal_config.c_stride : vb_cal_config.main_stride;
			mem_base += (i == 0) ? vb_cal_config.main_y_size : vb_cal_config.main_c_size;
		}
		chn_ctx->sbm_ctx.sb_mode = SB_FREE_RUN;
		_notify_vc_sb_mode(grp_id, chn_id, grp_ctx->online_from_isp, chn_ctx);

		TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) WrapBuf Size(0x%x) Addr(0x%llx)\n",
				grp_id, chn_id, wrap_ion_size, (unsigned long long)ion_paddr);
	}
	osal_mutex_unlock(&grp_ctx->lock);
	osal_mutex_unlock(&ctx->lock);

	return 0;
}

int vpss_get_chn_bufwrap_attr(vpss_grp grp_id, vpss_chn chn_id,
		vpss_chn_buf_wrap_s *buf_wrap, struct vpss_ctx *ctx)
{
	int ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	osal_mutex_lock(&grp_ctx->lock);
	*buf_wrap = chn_ctx->buf_wrap;
	osal_mutex_unlock(&grp_ctx->lock);

	return 0;
}

void vpss_release_all_grp(struct vpss_ctx *ctx)
{
	int i, j;
	struct vpss_cores *cores = osal_container_of(ctx, struct vpss_cores, ctx);

	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			if (ctx->grp_ctx[i]->is_started)
				vpss_stop_grp(i, ctx);

			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				if (ctx->grp_ctx[i]->chn_ctxs[j].is_enabled)
					vpss_disable_chn(i, j, ctx);
			}
			vpss_destroy_grp(i, cores);
		}
	}
}

