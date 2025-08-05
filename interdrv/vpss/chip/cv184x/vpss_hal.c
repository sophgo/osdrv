#include "vpss_debug.h"
#include "vpss_core.h"
#include "vpss_hal.h"
#include "ion.h"
#include "base_common.h"
#include "base_cb.h"
#include "vi_cb.h"
#include "venc_cb.h"
#include "bind.h"


#define HAL_WAIT_TIMEOUT_MS  1000

static int job_cheack_hw_ready(struct vpss_device *device)
{
	int i, state;

	for (i = 0; i < device->core_num; i++) {
		state = osal_atomic_read(&device->core_list[i]->state);
		if (state != VPSS_IDLE) {
			TRACE_VPSS(DBG_DEBUG, "vpss%d state(%d).\n", device->core_list[i]->vpss_type, state);
			return -1;
		}
	}

	return 0;
}

static int job_try_schedule(struct vpss_job *job, struct vpss_device *device)
{
	int ret;
	u8 i;
	struct vpss_hw_cfg *cfg = &job->cfg;
	unsigned long flags;
	int first_idx;
	struct vpss_core *core;

	osal_spin_lock_irqsave(&device->dev_lock, &flags);

	ret = job_cheack_hw_ready(device);
	if (ret) {
		TRACE_VPSS(DBG_DEBUG, "dev(%d) not ready.\n", device->id);
		osal_spin_unlock_irqrestore(&device->dev_lock, &flags);
		return -1;
	}

	device->job = (void *)job;
	first_idx = device->core_list[0]->vpss_type;

	for (i = 0; i < device->core_num; i++) {
		core = device->core_list[i];

		core->chn_idx = i;
		if (cfg->chn_cfg[i].sb_cfg.sb_mode)
			core->is_sbm = 1;
		else
			core->is_sbm = 0;
		if (core->reset_sbm) {
			vpss_ip_reset(core->vpss_type, core->reset_sbm, false);
			core->reset_sbm = 0;
		}

		if (i == 0)
			img_update(core->vpss_type, true, &cfg->grp_cfg);
		else
			img_update(core->vpss_type, false, &cfg->grp_cfg); //use dma share

		if (job->cfg.chn_enable[i]) {
			sc_update(core->vpss_type, &cfg->chn_cfg[i]);
			osal_atomic_set(&core->state, VPSS_RUNNING);
			core->start_cnt++;
		} else {
			osal_atomic_set(&core->state, VPSS_END);
		}

		if (i == (device->core_num - 1))
			top_update(core->vpss_type, false, job->cfg.chn_enable[i]); //last chn,not share
		else
			top_update(core->vpss_type, true, job->cfg.chn_enable[i]);
	}

	device->start_cnt++;
	osal_atomic_set(&device->state, VPSS_RUNNING);
	osal_spin_unlock_irqrestore(&device->dev_lock, &flags);

	osal_atomic_set(&job->job_state, JOB_WORKING);

	TRACE_VPSS(DBG_INFO, "Grp(%d) job working, chn enable(%d %d %d %d), dev(%d).\n",
		job->grp_id, cfg->chn_enable[0], cfg->chn_enable[1],
		cfg->chn_enable[2], cfg->chn_enable[3], device->id);

	osal_gettimeofday(&device->ts_start);

	img_start(first_idx, device->core_num);

	return 0;
}

static int notify_isp_retrigger(void)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = E_MODULE_VI;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VI_CB_QBUF_TRIGGER;
	exe_cb.data = NULL;

	return base_exe_module_cb(&exe_cb);

}

static int notify_vc_switch_chn(u8 grp_id, u8 chn_id, u64 frm_num)
{
	int i;
	struct venc_switch_chn info;
	struct base_exe_m_cb exe_cb;
	mmf_bind_dest_s bind_dest;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};

	info.venc_chn = -1;
	if (bind_get_dst(&chn, &bind_dest) == 0) {
		for (i = 0; i < bind_dest.num; ++i) {
			if (bind_dest.mmf_chn[i].mod_id == ID_VENC)
				info.venc_chn = bind_dest.mmf_chn[i].chn_id;
		}
	}

	if (info.venc_chn == -1)
		return -1;

	info.snr_num = grp_id;
	info.frm_num = frm_num;
	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VENC_CB_SWITCH_CHN;
	exe_cb.data = &info;

	return base_exe_module_cb(&exe_cb);

}

static int check_sb_mode(struct vpss_job *job)
{
	int i;
	int ret = 0;

	for (i = 0; i < VPSS_MAX_CHN_NUM; i++) {
		if (!job->cfg.chn_enable[i])
			continue;
		if (job->cfg.chn_cfg[i].sb_cfg.sb_mode &&
			notify_vc_switch_chn(job->grp_id, i, 0)) {
			TRACE_VPSS(DBG_WARN, "offline grp(%d) chn(%d), vc sbm not ready.\n",
				job->grp_id, i);
			ret = -1;
		}
	}

	return ret;
}

int vpss_hal_init(struct vpss_hal_ctx *hal_ctx)
{
	osal_spin_lock_init(&hal_ctx->task_lock);
	OSAL_INIT_LIST_HEAD(&hal_ctx->job_wait_queue);
	OSAL_INIT_LIST_HEAD(&hal_ctx->job_online_queue);
	hal_ctx->cmdq_buf.cmdq_phy_addr = 0;
	hal_ctx->cmdq_buf.cmdq_vir_addr = NULL;
	hal_ctx->cmdq_buf.cmdq_buf_size = 0;

	return 0;
}

void vpss_hal_deinit(struct vpss_hal_ctx *hal_ctx)
{
	struct vpss_job *job;
	unsigned long flags;

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	while (!osal_list_empty(&hal_ctx->job_wait_queue)) {
		job = osal_list_first_entry(&hal_ctx->job_wait_queue,
				struct vpss_job, list);
		osal_list_del_init(&job->list);
	}
	while (!osal_list_empty(&hal_ctx->job_online_queue)) {
		job = osal_list_first_entry(&hal_ctx->job_online_queue,
				struct vpss_job, list);
		osal_list_del_init(&job->list);
	}
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);
	osal_spin_lock_destroy(&hal_ctx->task_lock);

	if (hal_ctx->cmdq_buf.cmdq_phy_addr) {
		base_ion_free(hal_ctx->cmdq_buf.cmdq_phy_addr);
		hal_ctx->cmdq_buf.cmdq_phy_addr = 0;
		hal_ctx->cmdq_buf.cmdq_vir_addr = NULL;
		hal_ctx->cmdq_buf.cmdq_buf_size = 0;
	}
}

int vpss_hal_push_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx)
{
	unsigned long flags;

	if (check_sb_mode(job))
		return -1;

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	osal_list_add_tail(&job->list, &hal_ctx->job_wait_queue);
	osal_atomic_set(&job->job_state, JOB_WAIT);
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return 0;
}

int vpss_hal_push_online_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx)
{
	unsigned long flags;

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	osal_list_add_tail(&job->list, &hal_ctx->job_online_queue);
	osal_atomic_set(&job->job_state, JOB_WAIT);
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return 0;
}

int vpss_hal_remove_job(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx)
{
	int i, count = 40;
	unsigned long flags, flags_job;
	struct vpss_job *job_item;
	enum job_state job_state;
	struct vpss_cores *cores = osal_container_of(hal_ctx, struct vpss_cores, hal_ctx);
	struct vpss_device *device = &cores->device[job->dev_id];

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	osal_spin_lock_irqsave(&job->lock, &flags_job);

	job_state = osal_atomic_read(&job->job_state);

	if ((job_state == JOB_WORKING) || (job_state == JOB_HALF)) {
		osal_spin_unlock_irqrestore(&job->lock, &flags_job);
		osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

		//wait hw done
		while (--count > 0) {
			if ((osal_atomic_read(&job->job_state) == JOB_END) ||
				(osal_atomic_read(&job->job_state) == JOB_WAIT))
				break;
			TRACE_VPSS(DBG_NOTICE, "wait count(%d)\n", count);
			osal_msleep(1);
		}
		osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
		osal_spin_lock_irqsave(&job->lock, &flags_job);

		//hw hang
		if (count == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) device(%d) Wait timeout, HW hang.\n", job->grp_id, job->dev_id);
			for (i = 0; i < device->core_num; i++) {
				vpss_stauts(device->core_list[i]->vpss_type);
				vpss_ip_reset(device->core_list[i]->vpss_type, device->core_list[i]->is_sbm, true);
				osal_atomic_set(&device->core_list[i]->state, VPSS_IDLE);
			}
			device->job = NULL;
			osal_atomic_set(&device->state, VPSS_IDLE);
		}
	}

	if (osal_atomic_read(&job->job_state) == JOB_WAIT) {
		osal_list_for_each_entry(job_item, &hal_ctx->job_wait_queue, list) {
			if (job_item == job) {
				osal_atomic_set(&job->job_state, JOB_INVALID);
				osal_list_del_init(&job->list);
				goto unlock;
			}
		}
		osal_list_for_each_entry(job_item, &hal_ctx->job_online_queue, list) {
			if (job_item == job) {
				osal_atomic_set(&job->job_state, JOB_INVALID);
				osal_list_del_init(&job->list);
				goto unlock;
			}
		}
	}

unlock:
	osal_spin_unlock_irqrestore(&job->lock, &flags_job);
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return 0;
}

int vpss_hal_try_schedule(struct vpss_hal_ctx *hal_ctx)
{
	struct vpss_job *job = NULL;
	unsigned long flags, flags_job;
	int ret = -1;
	struct vpss_cores *cores = osal_container_of(hal_ctx, struct vpss_cores, hal_ctx);

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	while (!osal_list_empty(&hal_ctx->job_wait_queue)) {
		job = osal_list_first_entry(&hal_ctx->job_wait_queue,
			struct vpss_job, list);

		if (job->dev_id >= VPSS_DEVICE_NUM) {
			TRACE_VPSS(DBG_ERR, "Grp(%d), dev_id error.\n", job->grp_id);
			break;
		}
		if (cores->device[job->dev_id].is_online) {
			TRACE_VPSS(DBG_ERR, "device(%d) online, Please use the correct device number.\n", job->dev_id);
			break;
		}

		osal_spin_lock_irqsave(&job->lock, &flags_job);
		if (job_try_schedule(job, &cores->device[job->dev_id])) {
			//TRACE_VPSS(DBG_DEBUG, "Grp(%d), try schedule fail, wait for next time.\n",
			//	job->grp_id);
			osal_spin_unlock_irqrestore(&job->lock, &flags_job);
			break;
		}
		osal_list_del_init(&job->list);
		osal_spin_unlock_irqrestore(&job->lock, &flags_job);
		ret = 0;
	}
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return ret;
}

int vpss_hal_stitch_schedule(struct vpss_stitch_cfg *cfg, struct vpss_hal_ctx *hal_ctx)
{
	return 0;
}

int vpss_hal_online_run(struct vpss_online_cb *param, struct vpss_hal_ctx *hal_ctx)
{
	int i, ret;
	unsigned long flags, flags_job;
	struct vpss_job *work_job = NULL;
	struct vpss_job *job_item;
	struct vpss_device *device;
	struct vpss_cores *cores = osal_container_of(hal_ctx, struct vpss_cores, hal_ctx);

	if (param->snr_num >= VPSS_ONLINE_NUM) {
		TRACE_VPSS(DBG_WARN, "param->snr_num(%d) err.\n", param->snr_num);
		return -1;
	}

	TRACE_VPSS(DBG_INFO, "online trigger vpss, snr_num(%d).\n", param->snr_num);

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);

	osal_list_for_each_entry(job_item, &hal_ctx->job_online_queue, list) {
		if (job_item->grp_id == param->snr_num) {
			work_job = job_item;
			break;
		}
	}

	if (!work_job) {
		TRACE_VPSS(DBG_WARN, "online grp(%d), Empty job.\n", param->snr_num);
		goto err0;
	}

	osal_spin_lock_irqsave(&work_job->lock, &flags_job);
	for (i = 0; i < VPSS_MAX_CHN_NUM; i++) {
		if (!work_job->cfg.chn_enable[i])
			continue;
		if (work_job->cfg.chn_cfg[i].addr[0] == 0) {
			TRACE_VPSS(DBG_WARN, "online grp(%d) chn(%d), Not buffer.\n", param->snr_num, i);
			goto err1;
		}
		//sbm
		if (work_job->cfg.chn_cfg[i].sb_cfg.sb_mode &&
			notify_vc_switch_chn(param->snr_num, i, param->frm_num)) {
			TRACE_VPSS(DBG_WARN, "online grp(%d) chn(%d), vc sbm not ready.\n",
				param->snr_num, i);
			goto err1;
		}
	}
	osal_memcpy(&work_job->online_param, param, sizeof(struct vpss_online_cb));

	if (work_job->dev_id >= VPSS_DEVICE_NUM) {
		TRACE_VPSS(DBG_ERR, "Grp(%d), dev_id error.\n", work_job->grp_id);
		goto err1;
	}
	device = &cores->device[work_job->dev_id];
	if (!device->is_online) {
		TRACE_VPSS(DBG_ERR, "device(%d) offline, Please use the correct device number.\n", work_job->dev_id);
		goto err1;
	}

	ret = job_try_schedule(work_job, device);
	if (ret) {
		device->isp_triggered = true;
		goto err1;
	}

	osal_list_del_init(&work_job->list);
	osal_spin_unlock_irqrestore(&work_job->lock, &flags_job);
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return 0;

err1:
	osal_spin_unlock_irqrestore(&work_job->lock, &flags_job);
err0:
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);
	return -1;
}

void vpss_hal_job_finish(struct vpss_device *device)
{
	u8 i;
	unsigned long flags_job;
	struct vpss_job *job = (struct vpss_job *)device->job;
	struct vpss_cores *cores = osal_container_of(device, struct vpss_cores, device[device->id]);

	if (!job) {
		TRACE_VPSS(DBG_INFO, "dev(%d), job canceled.\n", device->id);
		return;
	}

	osal_spin_lock_irqsave(&device->dev_lock, &flags_job);

	for (i = 0; i < device->core_num; i++) {
		if (osal_atomic_read(&device->core_list[i]->state) != VPSS_END) {
			osal_spin_unlock_irqrestore(&device->dev_lock, &flags_job);
			return;
		}
	}

	//job finish
	osal_gettimeofday(&device->ts_end);
	device->hw_duration = get_diff_in_us(device->ts_start, device->ts_end);
	device->hw_duration_total += device->hw_duration;
	device->int_cnt++;
	device->job = NULL;
	osal_atomic_set(&device->state, VPSS_IDLE);

	job->info.hw_duration = device->hw_duration;
	for (i = 0; i < device->core_num; ++i) {
		job->info.checksum[i] = device->core_list[i]->checksum;
		osal_atomic_set(&device->core_list[i]->state, VPSS_IDLE);
	}

	TRACE_VPSS(DBG_INFO, "***vpss grp(%d) Hw Duration (%d)***.\n", job->grp_id, device->hw_duration);
	osal_atomic_set(&job->job_state, JOB_END);
	job->job_cb(job);

	osal_spin_unlock_irqrestore(&device->dev_lock, &flags_job);

	if (device->is_online) {
		if (device->isp_triggered) {
			device->isp_triggered = false;
			notify_isp_retrigger();
		}
	} else {
		vpss_hal_try_schedule(&cores->hal_ctx);
	}
}

int vpss_hal_reset(struct vpss_job *job, struct vpss_hal_ctx *hal_ctx)
{
	int i;
	struct vpss_cores *cores = osal_container_of(hal_ctx, struct vpss_cores, hal_ctx);
	struct vpss_device *device;
	unsigned long flags;

	device = &cores->device[job->dev_id];

	for (i = 0; i < device->core_num; i++) {
		vpss_stauts(device->core_list[i]->vpss_type);
		vpss_ip_reset(device->core_list[i]->vpss_type, device->core_list[i]->is_sbm, true);
		osal_atomic_set(&device->core_list[i]->state, VPSS_IDLE);
	}

	osal_atomic_set(&device->state, VPSS_IDLE);

	osal_spin_lock_irqsave(&hal_ctx->task_lock, &flags);
	if (device->is_online)
		osal_list_add(&job->list, &hal_ctx->job_online_queue);
	else
		osal_list_add(&job->list, &hal_ctx->job_wait_queue);
	osal_atomic_set(&job->job_state, JOB_WAIT);
	osal_spin_unlock_irqrestore(&hal_ctx->task_lock, &flags);

	return 0;
}

