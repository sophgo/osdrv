#include "vpss_debug.h"
#include "base_cb.h"
#include "base_common.h"
#include "rgn_cb.h"
#include "vi_cb.h"
#include "venc_cb.h"
#include "vpss_core.h"
#include "vpss_rgn_ctrl.h"
#include "vpss_sdk_layer.h"
#include "vpss.h"
#include "bind.h"

#define VPSS_TIMEOUT_US (1000000)


static int _vpss_set_vivpss_mode(struct vpss_cores *cores, const vi_vpss_mode_s *mode)
{
	osal_memcpy(&cores->vi_vpss_mode, mode, sizeof(vi_vpss_mode_s));

	return 0;
}

int _vpss_call_vi_reset(void)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = E_MODULE_VI;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VI_CB_RESET_ISP;
	exe_cb.data   = NULL;

	return base_exe_module_cb(&exe_cb);
}


static int vpss_online_err_cb(u8 snr_num, struct vpss_cores *cores)
{
	int i;
	struct vpss_device *device = NULL;

	for (i = 0; i < VPSS_DEVICE_NUM; i++)
		if (cores->device[i].is_online)
			device = &cores->device[i];

	if (device == NULL) {
		TRACE_VPSS(DBG_DEBUG, "Not device online.\n");
		return -1;
	}
	TRACE_VPSS(DBG_WARN, "grp(%d) online error.\n", snr_num);

	for (i = 0; i < device->core_num; i++) {
		if (device->core_list[i]->is_sbm) {
			mmf_bind_dest_s bind_dest;
			mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = snr_num, .chn_id = i};
			struct base_exe_m_cb exe_cb;
			struct venc_skip_frm_info info;

			info.venc_chn = -1;
			if (bind_get_dst(&chn, &bind_dest) == 0) {
				for (i = 0; i < bind_dest.num; ++i) {
					if (bind_dest.mmf_chn[i].mod_id == ID_VENC)
						info.venc_chn = bind_dest.mmf_chn[i].chn_id;
				}
			}

			exe_cb.callee = E_MODULE_VCODEC;
			exe_cb.caller = E_MODULE_VPSS;
			exe_cb.cmd_id = VENC_CB_SKIP_FRM;
			exe_cb.data = &info;
			base_exe_module_cb(&exe_cb);

			TRACE_VPSS(DBG_WARN, "venc skip frame.\n");
		}
	}

	if (device->job) {
		vpss_hal_reset(device->job, &cores->hal_ctx);
		device->job = NULL;
	}

	return 0;
}

int vpss_overflow_check(struct vpss_reg_info *vpss_info,
		struct venc_reg_info *vc_info, struct vpss_cores *cores)
{
	int i;
	struct base_exe_m_cb exe_cb;
	struct vpss_device *device = NULL;

	for (i = 0; i < VPSS_DEVICE_NUM; i++)
		if (cores->device[i].is_online)
			device = &cores->device[i];

	if (device == NULL) {
		TRACE_VPSS(DBG_DEBUG, "Not device online.\n");
		return -1;
	}

	for (i = 0; i < device->core_num; i++) {
		if (device->core_list[i]->is_sbm) {
			exe_cb.callee = E_MODULE_VCODEC;
			exe_cb.caller = E_MODULE_VPSS;
			exe_cb.cmd_id = VENC_CB_OVERFLOW_CHECK;
			exe_cb.data = (void *)vc_info;
			base_exe_module_cb(&exe_cb);
		}
	}

	//sclr_check_overflow_reg(vpss_info);

	return 0;
}

static int vpss_get_sbm_info(struct vpss_sbm_cfg *info, struct vpss_cores *cores)
{
	int i, ret;
	struct vpss_grp_ctx *grp_ctx;
	struct vpss_chn_ctx *chn_ctx;
	vpss_grp grp_id = info->grp_id;
	vpss_chn chn_id = info->chn_id;
	struct sbm_cfg *pst_sbm_cfg = &info->st_sbm_cfg;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, &cores->ctx);
	if (ret != 0)
		return ret;

	grp_ctx = cores->ctx.grp_ctx[grp_id];
	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	chn_ctx = &grp_ctx->chn_ctxs[chn_id];

	pst_sbm_cfg->is_online = grp_ctx->online_from_isp;
	pst_sbm_cfg->slice_num = chn_ctx->buf_wrap.wrap_buffer_size;
	pst_sbm_cfg->line_num = chn_ctx->buf_wrap.buf_line;
	pst_sbm_cfg->slice_mode = chn_ctx->sbm_ctx.sb_mode;
	pst_sbm_cfg->width = chn_ctx->chn_attr.width;
	pst_sbm_cfg->height = chn_ctx->chn_attr.height;
	for (i = 0; i < 3; i++) {
		pst_sbm_cfg->phy_addr[i] = chn_ctx->sbm_ctx.phy_addr[i];
		pst_sbm_cfg->stride[i] = chn_ctx->sbm_ctx.stride[i];
	}

	return 0;
}

static int vpss_reset_sbm(struct vpss_cores *cores)
{
	cores->core[0].reset_sbm = 1;
	return 0;
}

static int _vpss_set_vc_sbm_ready(int venc_chn, struct vpss_cores *cores)
{
	cores->core[0].vc_ready = 1;
	return 0;
}

static void _vpss_set_mlv_info(unsigned char snr_num, struct mlv_i_s *p_m_lv_i)
{
}

static void _vpss_get_mlv_info(unsigned char snr_num, struct mlv_i_s *p_m_lv_i)
{
}

static int _vpss_get_dev_idx(vpss_grp vpss_grp, vpss_chn vpss_chn, struct vpss_cores *cores)
{
	vpss_mode_s vpss_mode = {0};
	u8 dev_id = 0;

	if (cores == NULL) {
		TRACE_VPSS(DBG_ERR, "cores is nulL.\n");
		return ERR_VPSS_NULL_PTR;
	}

	vpss_mode = cores->vpss_mode;
	dev_id = cores->ctx.grp_ctx[vpss_grp]->dev_id;

	if (vpss_mode.mode == VPSS_MODE_SINGLE) {
		return vpss_chn;
	} else if (vpss_mode.mode == VPSS_MODE_DUAL) {
		if (dev_id == 1)
			return vpss_chn;
		else
			return 3;
	}
	return 0;
}

static int _vpss_core_cb(void *dev, cb_modules_id caller, u32 cmd, void *arg)
{
	struct vpss_cores *cores = (struct vpss_cores *)dev;
	int rc = -1;

	switch (cmd) {
	case VPSS_CB_VI_ONLINE_TRIGGER:
	{
		struct vpss_online_cb_info *post_para = (struct vpss_online_cb_info *)arg;
		struct vpss_online_cb param;

		param.snr_num = post_para->snr_num;
		param.is_tile = post_para->is_tile;
		param.is_left_tile = post_para->is_left_tile;
		param.frm_num = post_para->frm_num;
		param.l_in = post_para->l_in;
		param.l_out = post_para->l_out;
		param.r_in = post_para->r_in;
		param.r_out = post_para->r_out;
		param.ts = post_para->ts;

		_vpss_set_mlv_info(post_para->snr_num, &post_para->m_lv_i);
		rc = vpss_hal_online_run(&param, &cores->hal_ctx);
		break;
	}

	case VPSS_CB_SET_VIVPSSMODE:
	{
		const vi_vpss_mode_s *vi_vpss_mode = (const vi_vpss_mode_s *)arg;

		TRACE_VPSS(DBG_INFO, "VPSS_CB_SET_VIVPSSMODE\n");
		rc = _vpss_set_vivpss_mode(cores, vi_vpss_mode);
		break;
	}

	case VPSS_CB_GET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		vpss_grp grp = attr->chn.dev_id;
		vpss_chn chn = attr->chn.chn_id;
		rgn_handle *handle = attr->hdls;
		rgn_type_e type = attr->type;
		u32 layer = attr->layer;
		u8 dev_idx = _vpss_get_dev_idx(grp, chn, cores);

		if (dev_idx > 1)
			layer = 0;

		rc = vpss_get_rgn_hdls(grp, chn, layer, type, handle, &cores->ctx);
		break;
	}

	case VPSS_CB_SET_RGN_HDLS:
	{
		struct _rgn_hdls_cb_param *attr = (struct _rgn_hdls_cb_param *)arg;
		vpss_grp grp = attr->chn.dev_id;
		vpss_chn chn = attr->chn.chn_id;
		rgn_handle *handle = attr->hdls;
		rgn_type_e type = attr->type;
		u32 layer = attr->layer;
		u8 dev_idx = _vpss_get_dev_idx(grp, chn, cores);

		if (dev_idx > 1)
			layer = 0;

		rc = vpss_set_rgn_hdls(grp, chn, layer, type, handle, &cores->ctx);
		break;
	}

	case VPSS_CB_SET_RGN_CFG:
	{
		struct _rgn_cfg_cb_param *attr = (struct _rgn_cfg_cb_param *)arg;
		struct rgn_cfg *rgn_cfg = &attr->rgn_cfg;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;
		u32 layer = attr->layer;
		u8 dev_idx = _vpss_get_dev_idx(vpss_grp, vpss_chn, cores);

		if (dev_idx > 1)
			layer = 0;

		rc = vpss_set_rgn_cfg(vpss_grp, vpss_chn, layer, rgn_cfg, &cores->ctx);
		break;
	}

	case VPSS_CB_SET_RGNEX_CFG:
	{
		TRACE_VPSS(DBG_ERR, "Not support rgn_ex\n");
		rc = -1;
		break;
	}

	case VPSS_CB_SET_COVEREX_CFG:
	{
		struct _rgn_coverex_cfg_cb_param *attr = (struct _rgn_coverex_cfg_cb_param *)arg;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;

		rc = vpss_set_rgn_coverex_cfg(vpss_grp, vpss_chn, &attr->rgn_coverex_cfg, &cores->ctx);
		break;
	}

	case VPSS_CB_SET_MOSAIC_CFG:
	{
		struct _rgn_mosaic_cfg_cb_param *attr = (struct _rgn_mosaic_cfg_cb_param *)arg;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;

		rc = vpss_set_rgn_mosaic_cfg(vpss_grp, vpss_chn, &attr->rgn_mosaic_cfg, &cores->ctx);
		break;
	}

	case VPSS_CB_SET_RGN_LUT_CFG:
	{
		struct _rgn_lut_cb_param *attr = (struct _rgn_lut_cb_param *)arg;
		struct rgn_lut_cfg *rgn_lut_cfg = &attr->lut_cfg;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;

		rc = vpss_set_rgn_lut_cfg(vpss_grp, vpss_chn, rgn_lut_cfg, &cores->ctx);
		break;
	}

	case VPSS_CB_GET_RGN_OW_ADDR:
	{
		struct _rgn_get_ow_addr_cb_param *attr = (struct _rgn_get_ow_addr_cb_param *)arg;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;
		u32 layer = attr->layer;
		u8 dev_idx = _vpss_get_dev_idx(vpss_grp, vpss_chn, cores);

		if (dev_idx > 1)
			layer = 0;

		rc = vpss_get_rgn_ow_addr(vpss_grp, vpss_chn, layer, attr->handle, &attr->addr, &cores->ctx, dev_idx);
		break;
	}

	case VPSS_CB_GET_CHN_SIZE:
	{
		struct _rgn_chn_size_cb_param *attr = (struct _rgn_chn_size_cb_param *)arg;
		vpss_chn_attr_s chn_attr;
		vpss_grp vpss_grp = attr->chn.dev_id;
		vpss_chn vpss_chn = attr->chn.chn_id;

		rc = vpss_get_chn_attr(vpss_grp, vpss_chn, &chn_attr, &cores->ctx);
		attr->rect.width = chn_attr.width;
		attr->rect.height = chn_attr.height;
		break;
	}

	case VPSS_CB_ONLINE_ERR_HANDLE:
	{
		struct vpss_online_err_handle_info *err_cb_info =
			(struct vpss_online_err_handle_info *)arg;

		rc = vpss_online_err_cb(err_cb_info->snr_num, cores);
		break;
	}

	case VPSS_CB_OVERFLOW_CHECK:
	{
		struct overflow_info *info = (struct overflow_info *)arg;

		rc = vpss_overflow_check(&info->vpss_info, &info->vc_info, cores);
		break;
	}

	case VPSS_CB_GET_SBM_INFO:
	{
		struct vpss_sbm_cfg *info = (struct vpss_sbm_cfg *)arg;

		rc = vpss_get_sbm_info(info, cores);
		break;
	}

	case VPSS_CB_RESET_SBM:
	{
		rc = vpss_reset_sbm(cores);
		break;
	}

	case VPSS_CB_GET_MLV_INFO:
	{
		struct vpss_grp_mlv_info *mlv_info = (struct vpss_grp_mlv_info *)arg;
		vpss_grp vpss_grp = mlv_info->vpss_grp;

		_vpss_get_mlv_info(vpss_grp, &mlv_info->m_lv_i);
		break;
	}

	case VPSS_CB_STITCH:
	{
		struct vpss_stitch_cfg *stitch_cfg = (struct vpss_stitch_cfg *)arg;

		rc = vpss_hal_stitch_schedule(stitch_cfg, &cores->hal_ctx);
		break;
	}

	case VPSS_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg =
			(struct ldc_op_done_cfg *)arg;

		vpss_gdc_callback(cfg->param, cfg->blk, &cores->ctx);
		rc = 0;
		break;
	}

	case VPSS_CB_GDC_GET_CHN_ATTR:
	{
		struct ldc_op_done_cfg *ldc_cfg =
			(struct ldc_op_done_cfg *)arg;
		vpss_chn_attr_s chn_attr;

		struct vpss_ldc_internal_chn_attr *attr = (struct vpss_ldc_internal_chn_attr *)ldc_cfg->param;

		rc = vpss_get_chn_attr(attr->vpss_chn_attr.vpss_grp, attr->vpss_chn_attr.vpss_chn,
					&chn_attr, &cores->ctx);
		attr->vpss_chn_attr.size.width = chn_attr.width;
		attr->vpss_chn_attr.size.height = chn_attr.height;
		break;
	}

	case VPSS_CB_GDC_SET_CHN_CFG:
	{
		struct ldc_op_done_cfg *ldc_cfg =
			(struct ldc_op_done_cfg *)arg;

		struct vpss_ldc_internal_chn_ldc_cfg *cfg = (struct vpss_ldc_internal_chn_ldc_cfg *)ldc_cfg->param;

		rc = vpss_set_chn_ldc_attr(cfg->vpss_cfg.vpss_grp, cfg->vpss_cfg.vpss_chn,
				&cfg->vpss_cfg.ldc_attr, cfg->vpss_cfg.mesh_handle, &cores->ctx);
		break;
	}

	case VPSS_CB_SBM_FRM_DONE:
	{
		int *venc_chn = (int *)arg;

		rc = _vpss_set_vc_sbm_ready(*venc_chn, cores);
		break;
	}

	default:
		break;
	}

	return rc;
}

static int _vpss_core_register_cb(struct vpss_cores *cores)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VPSS;
	reg_cb.dev		= (void *)cores;
	reg_cb.cb		= _vpss_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static int _vpss_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VPSS);
}

static void _vpss_timer_core_update(struct vpss_cores *cores, unsigned int duration_us)
{
	u8 i;

	for (i = 0; i < VPSS_DEVICE_NUM; ++i) {
		cores->device[i].duty_ratio = 100 * cores->device[i].hw_duration_total / duration_us;
		cores->device[i].hw_duration_total = 0;
	}
}

static void _vpss_timer_callback(unsigned long data)
{
	int i;
	unsigned int diff_us;
	osal_timeval now;
	static osal_timeval prev_time = {0, 0};
	struct vpss_cores *cores = (struct vpss_cores *)osal_timer_get_private_data((void *)data);

	osal_gettimeofday(&now);

	diff_us = get_diff_in_us(prev_time, now);
	_update_vpss_chn_real_frame_rate(&cores->ctx, diff_us);
	_vpss_timer_core_update(cores, diff_us);

	//timout reset
	for (i = 0; i < VPSS_DEVICE_NUM; ++i) {
		if (osal_atomic_read(&cores->device[i].state) == VPSS_RUNNING) {
			diff_us = get_diff_in_us(cores->device[i].ts_start, now);
			if (diff_us > VPSS_TIMEOUT_US) {
				vpss_hal_reset(cores->device[i].job, &cores->hal_ctx);
				cores->device[i].job = NULL;
				if (cores->device[i].is_online) {
					_vpss_call_vi_reset();
				} else {
					vpss_hal_try_schedule(&cores->hal_ctx);
				}
				TRACE_VPSS(DBG_NOTICE, "device-%d %s timeout...\n",
					i, cores->device[i].is_online ? "online" : "offline");
			}
		}
	}
	prev_time = now;

	osal_timer_mod(&cores->timer, 1000);
}

void vpss_core_set_mode(struct vpss_cores *cores, const vpss_mode_s *vpss_mode)
{
	//device
	cores->vpss_mode = *vpss_mode;

	if (vpss_mode->mode == VPSS_MODE_SINGLE) {
		cores->device[0].core_num = 0;
		cores->device[0].is_online = false;

		cores->device[1].core_num = 4;
		cores->device[1].is_online = (vpss_mode->input[0] == VPSS_INPUT_ISP) ? true : false;
		cores->device[1].core_list[0] = &cores->core[0];
		cores->device[1].core_list[1] = &cores->core[1];
		cores->device[1].core_list[2] = &cores->core[2];
		cores->device[1].core_list[3] = &cores->core[3];

		cores->core[0].device = &cores->device[1];
		cores->core[1].device = &cores->device[1];
		cores->core[2].device = &cores->device[1];
		cores->core[3].device = &cores->device[1];
	} else {
		cores->device[0].core_num = 1;
		cores->device[0].is_online = (vpss_mode->input[0] == VPSS_INPUT_ISP) ? true : false;
		cores->device[0].core_list[0] = &cores->core[3];

		cores->device[1].core_num = 3;
		cores->device[1].is_online = (vpss_mode->input[1] == VPSS_INPUT_ISP) ? true : false;
		cores->device[1].core_list[0] = &cores->core[0];
		cores->device[1].core_list[1] = &cores->core[1];
		cores->device[1].core_list[2] = &cores->core[2];

		cores->core[0].device = &cores->device[1];
		cores->core[1].device = &cores->device[1];
		cores->core[2].device = &cores->device[1];
		cores->core[3].device = &cores->device[0];
	}
}

void vpss_core_init(struct vpss_cores *cores)
{
	u8 i;
	struct vpss_core *core;

	/* vpss register cb */
	if (_vpss_core_register_cb(cores)) {
		TRACE_VPSS(DBG_INFO, "Failed to register vpss cb\n");
	}

	/* initialize locks */
	osal_spin_lock_init(&cores->lock);


	/* initialize timer */
	cores->timer.handler = _vpss_timer_callback;
	cores->timer.data = (unsigned long)cores;
	cores->timer.interval = 1000;
	osal_timer_init(&cores->timer);

	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		core = &cores->core[i];

		core->vpss_type = i;
		osal_atomic_set(&core->state, VPSS_IDLE);
		osal_spin_lock_init(&core->core_lock);
		vpss_ip_init(i, false);
		TRACE_VPSS(DBG_INFO, "vpss-%d init done\n", i);
	}

	for (i = 0; i < VPSS_DEVICE_NUM; ++i) {
		cores->device[i].id = i;
		cores->device[i].is_online = false;
		osal_spin_lock_init(&cores->device[i].dev_lock);
		osal_atomic_set(&cores->device[i].state, VPSS_IDLE);
	}

	vpss_hal_init(&cores->hal_ctx);
	vpss_init(&cores->ctx);
}

void vpss_core_deinit(struct vpss_cores *cores)
{
	int i;

	vpss_deinit(&cores->ctx);
	vpss_hal_deinit(&cores->hal_ctx);

	for (i = 0; i < VPSS_DEVICE_NUM; ++i) {
		osal_spin_lock_destroy(&cores->device[i].dev_lock);
	}
	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		osal_spin_lock_destroy(&cores->core[i].core_lock);
	}
	osal_timer_destroy_sync(&cores->timer);
	osal_spin_lock_destroy(&cores->lock);

	_vpss_core_rm_cb();
}

void vpss_core_open(struct vpss_cores *cores)
{
	int i;
	vpss_mode_s vpss_mode;

	//default single mode
	vpss_mode.mode = VPSS_MODE_SINGLE;
	vpss_mode.input[0] = VPSS_INPUT_MEM;
	vpss_core_set_mode(cores, &vpss_mode);

	vpss_ctx_param_init(&cores->ctx);
	osal_timer_start(&cores->timer);

	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		osal_clk_prepare_enable(cores->core[i].clk);
		vpss_ip_reset(i, true, true);
	}
}

void vpss_core_release(struct vpss_cores *cores)
{
	int i;

	osal_timer_stop(&cores->timer);
	vpss_release_all_grp(&cores->ctx);
	for (i = VPSS_V0; i < VPSS_MAX; ++i) {
		osal_clk_disable_unprepare(cores->core[i].clk);
	}
}

static void vpss_irq_handler(struct vpss_core *core)
{
	struct vpss_device *device = (struct vpss_device *)core->device;

	core->int_cnt++;
	osal_atomic_cmpxchg(&core->state, VPSS_RUNNING, VPSS_END);

	vpss_hal_job_finish(device);
}

static void vpss_cmdq_irq_handler(struct vpss_core *core)
{
	struct vpss_device *device = (struct vpss_device *)core->device;
	struct vpss_stitch_cfg *cfg = (struct vpss_stitch_cfg *)device->job;
	unsigned long flags;

	if (!cfg) {
		TRACE_VPSS(DBG_INFO, "dev(%d), job NULL.\n", device->id);
		osal_atomic_set(&core->state, VPSS_IDLE);
		return;
	}

	osal_spin_lock_irqsave(&device->dev_lock, &flags);
	osal_atomic_set(&core->state, VPSS_IDLE);
	core->int_cnt++;

	osal_gettimeofday(&device->ts_end);
	device->hw_duration = get_diff_in_us(device->ts_start, device->ts_end);
	device->hw_duration_total += device->hw_duration;
	device->job = NULL;
	osal_spin_unlock_irqrestore(&device->dev_lock, &flags);
	TRACE_VPSS(DBG_DEBUG, "dev(%d) Hw Duration (%d).\n", device->id, device->hw_duration);

	cfg->job_cb(cfg->data);
}

void vpss_core_isr(int irq, void *data)
{
	struct vpss_core *core = (struct vpss_core *)data;
	u8 vpss_idx = core->vpss_type;

	if (core->irq_num != irq) {
		TRACE_VPSS(DBG_ERR, "irq(%d) Error.\n", irq);
		return;
	}
	vpss_interrupter_clear(vpss_idx, &core->intr_status);

	if (core->intr_status.cmdq_end) {
		core->intr_status.cmdq_end = false;
		vpss_cmdq_irq_handler(core);
		return;
	}

	if (core->intr_status.sc_end) {
		core->intr_status.sc_end = false;
		core->checksum = vpss_get_checksum(vpss_idx);
		vpss_irq_handler(core);
	}
}

