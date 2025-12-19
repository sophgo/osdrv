#include "osal_types.h"

#include "comm_video.h"
#include "comm_gdc.h"
#include "ldc_uapi.h"
#include "base_cb.h"
#include "sys.h"
#include "ldc_debug.h"
#include "vb.h"
#include "ldc_sdk.h"
#include "ldc_common.h"
#include "ldc_comm_layer.h"
#include "ldc.h"
#include "ldc_core.h"
#include "cmdq.h"
#include "ion.h"
#include "vbq.h"
#include "mesh.h"
#include "base_cb.h"

#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))
#define BGCOLOR_GRAY  (0x808080)
#define BGCOLOR_GREEN  (YUV_8BIT(0, 128, 128))
#define LDC_INTR_EN_ALL (0x01)
#define DWA_INTR_EN_ALL (0x07)
#define LDC_INTR_EN_NULL (0x0)
#define LDC_INVALID_CORE_ID (LDC_DEV_MAX_CNT + 1)
#define SEM_WAIT_TIMEOUT_MS 100

#define USE_EXCEPTION_HDL 0

#undef ALIGN
#define ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))

static osal_mutex g_mesh_lock = {0};
static osal_mutex ldc_reg_lock = {0};
struct ldc_ctx *ctx;

struct ldc_ctx *get_ldc_ctx(void)
{
	return ctx;
}

struct ldc_ctx **get_ldc_ctx_addr(void)
{
	return &ctx;
}

static cb_modules_id convert_cb_id(mod_id_e mod_id)
{
	if (mod_id == ID_VI)
		return E_MODULE_VI;
	else if (mod_id == ID_VO)
		return E_MODULE_VO;
	else if (mod_id == ID_VPSS)
		return E_MODULE_VPSS;

	return E_MODULE_BUTT;
}

static mod_id_e convert_mod_id(cb_modules_id cbModId)
{
	if (cbModId == E_MODULE_VI)
		return ID_VI;
	else if (cbModId == E_MODULE_VO)
		return ID_VO;
	else if (cbModId == E_MODULE_VPSS)
		return ID_VPSS;

	return ID_BUTT;
}

int ldc_exec_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	struct ldc_ctx *ctx = (struct ldc_ctx *)dev;
	struct mesh_gdc_cfg *cfg;
	mod_id_e mod_id;
	int rc = -1;

	switch (cmd) {
		case LDC_CB_MESH_GDC_OP: {
			osal_mutex_lock(&g_mesh_lock);
			cfg = (struct mesh_gdc_cfg *)arg;
			mod_id = convert_mod_id(caller);
			osal_mutex_unlock(&g_mesh_lock);

			if (mod_id == ID_BUTT) {
				TRACE_LDC(DBG_WARN, "invalid mod do GDC_OP\n");
				return -1;
			}
			rc = mesh_gdc_do_op(ctx, cfg->usage, cfg->usage_param
				, cfg->vb_in, cfg->pix_format, cfg->mesh_addr
				, cfg->sync_io, cfg->cb_param, cfg->cb_param_size
				, mod_id, cfg->rotation);
			break;
		}
		default: {
			TRACE_LDC(DBG_WARN, "invalid cb CMD\n");
			break;
		}
	}

	return rc;
}

int ldc_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_LDC);
}

int ldc_reg_cb(struct ldc_ctx *ctx)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_LDC;
	reg_cb.dev		= (void *)ctx;
	reg_cb.cb		= ldc_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static void ldc_op_done_cb(mod_id_e mod_id, void *param, vb_blk blk)
{
	struct ldc_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	cb_modules_id callee = convert_cb_id(mod_id);

	TRACE_LDC(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, mod_id);
	cfg.param = param;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void ldc_hdl_hw_tsk_cb(struct ldc_ctx *ctx, struct ldc_job *job
	, struct ldc_task *tsk, bool is_last)
{
	bool is_internal;
	vb_blk blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	mod_id_e mod_id;
	unsigned char isLatask;
	struct ldc_vb_done *vb_done = NULL;
	unsigned long flags;

	if ((!ctx || !job || !tsk)) {
		TRACE_LDC(DBG_ERR, "invalid param\n");
		return;
	}
	is_internal = (tsk->attr.reserved == GDC_MAGIC);
	mod_id = job->identity.mod_id;

	TRACE_LDC(DBG_DEBUG, "is_internal=%d\n", is_internal);

	/* []internal module]:
	 *  sync_io or async_io
	 *  release imgin vb_blk at task done.
	 *  mesh prepared by module.
	 *
	 * []user case]:
	 *  always sync_io.
	 *  Don't care imgin/imgout vb_blk. User release by themselves.
	 *  mesh prepared by ldc at AddXXXTask.
	 *
	 */
	if (is_internal) {
		// for internal module handshaking. such as vi/vpss rotation.
		isLatask = (u8)tsk->attr.private_data[1];

		TRACE_LDC(DBG_DEBUG, "isLatask=%d, blk_in(pa=0x%llx), blk_out(pa=0x%llx)\n",
				isLatask,
				(unsigned long long)tsk->attr.img_in.video_frame.phyaddr[0],
				(unsigned long long)tsk->attr.img_out.video_frame.phyaddr[0]);

		blk_in = vb_phys_addr2handle(tsk->attr.img_in.video_frame.phyaddr[0]);
		blk_out = vb_phys_addr2handle(tsk->attr.img_out.video_frame.phyaddr[0]);
		if (blk_out == VB_INVALID_HANDLE)
			TRACE_LDC(DBG_ERR, "blk_out is invalid vb_blk, no callback to(%d)\n", mod_id);
		else {
			osal_atomic_fetch_and(~BIT(ID_GDC), &((struct vb_s *)(uintptr_t)blk_out)->mod_ids);
			if (isLatask)
				ldc_op_done_cb(mod_id, (void *)(uintptr_t)tsk->attr.private_data[2], blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal ldc sub job.
		// Kernel space:
		//  !isLatask used for internal ldc sub job.
		if (isLatask && blk_in != VB_INVALID_HANDLE) {
			osal_atomic_fetch_and(~BIT(ID_GDC), &((struct vb_s *)(uintptr_t)blk_in)->mod_ids);
			vb_release_block(blk_in);
		} else if (!isLatask) {
			osal_vfree((void *)(uintptr_t)tsk->attr.private_data[2]);
		}
	} else {
		if (!job->identity.sync_io && is_last) {
			vb_done = osal_kzalloc(sizeof(*vb_done), OSAL_GFP_ATOMIC);

			osal_memcpy(&vb_done->img_out, &tsk->attr.img_out, sizeof(vb_done->img_out));
			osal_memcpy(&vb_done->job, job, sizeof(*job));

			osal_spin_lock_irqsave(&ctx->vb_doneq.lock, &flags);
			osal_list_add_tail(&vb_done->node, &ctx->vb_doneq.doneq);
			osal_spin_unlock_irqrestore(&ctx->vb_doneq.lock, &flags);

			osal_sem_up(&ctx->vb_doneq.sem);

			TRACE_LDC(DBG_DEBUG, "vb_done->img_out[%llx-%d]\n"
				, vb_done->img_out.video_frame.phyaddr[0], vb_done->img_out.video_frame.width);
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name);
		}
	}
}

static void ldc_wkup_cmdq_tsk(struct ldc_core *core)
{
	unsigned long flags;

	osal_spin_lock_irqsave(&core->core_lock, &flags);
	core->cmdq_evt = true;
	osal_spin_unlock_irqrestore(&core->core_lock, &flags);

	osal_wait_wakeup(&core->cmdq_wq);

	TRACE_LDC(DBG_DEBUG, "ldc_wkup_cmdq_tsk\n");
}

static void ldc_notify_wkup_evt_kth(void *data, enum ldc_wait_evt evt)
{
	struct ldc_ctx *dev = (struct ldc_ctx *)data;
	unsigned long flags;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	osal_spin_lock_irqsave(&dev->ctx_lock, &flags);
	dev->evt |= evt;
	osal_spin_unlock_irqrestore(&dev->ctx_lock, &flags);

	osal_wait_wakeup(&dev->wait);

	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
}

static void ldc_clr_evt_kth(void *data)
{
	struct ldc_ctx *dev = (struct ldc_ctx *)data;
	unsigned long flags;
	enum ldc_wait_evt evt;

	if (!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	evt = dev->evt;
	osal_spin_lock_irqsave(&dev->ctx_lock, &flags);
	dev->evt &= ~evt;
	osal_spin_unlock_irqrestore(&dev->ctx_lock, &flags);
	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
}

static void ldc_work_handle_job_done(struct ldc_ctx *dev, struct ldc_job *job)
{
	unsigned long flags;
	struct ldc_core *core;
	int coreid;

	coreid = job->coreid;
	core = &dev->core[coreid];

	osal_atomic_set(&job->job_state, LDC_JOB_END);

	ldc_proc_record_job_done(job);

	osal_spin_lock_irqsave(&dev->ctx_lock, &flags);
	osal_list_del(&job->node);
	osal_spin_unlock_irqrestore(&dev->ctx_lock, &flags);

	TRACE_LDC(DBG_INFO, "job [%px] done\n", job);

	osal_atomic_set(&core->state, LDC_CORE_STATE_IDLE);

	if (job->identity.sync_io) {
		TRACE_LDC(DBG_INFO, "job[%px] wake endjob\n", job);
		osal_sem_up(&job->job_done_sem);
	} else {
		osal_sem_destroy(&job->job_done_sem);
		osal_spin_lock_destroy(&job->lock);
		osal_kfree(job);
	}

	ldc_notify_wkup_evt_kth(dev, LDC_EVENT_EOF);
}

static void ldc_work_handle_tsk_done(struct ldc_ctx *dev
	, struct ldc_task *tsk, struct ldc_job *job, bool is_last_tsk)
{
	unsigned long flags;

	if ((!tsk)) {
		TRACE_LDC(DBG_ERR, "null tsk\n");
		return;
	}

	if (osal_atomic_read(&tsk->state) == LDC_TASK_STATE_RUNNING) {
		osal_atomic_set(&tsk->state, LDC_TASK_STATE_DONE);
		osal_atomic_dec_return(&job->task_num);

		ldc_hdl_hw_tsk_cb(dev, job, tsk, is_last_tsk);

		ldc_proc_record_hw_tsk_done(job, tsk);

		TRACE_LDC(DBG_INFO, "tsk[%px] done, is last tsk[%d]\n", tsk, is_last_tsk);

		osal_spin_lock_irqsave(&job->lock, &flags);
		osal_list_del(&tsk->node);
		osal_spin_unlock_irqrestore(&job->lock, &flags);
		osal_kfree(tsk);

		if (is_last_tsk) {
			ldc_work_handle_job_done(dev, job);
		}
	} else {
		TRACE_LDC(DBG_ERR, "invalid tsk state(%d).\n"
			, (enum ldc_task_state)osal_atomic_read(&tsk->state));
	}
}

static void ldc_work_handle_frm_done(int coreid)
{
	struct ldc_ctx *dev = get_ldc_ctx();
	struct ldc_core *core = &dev->core[coreid];
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long flags;
	bool is_last_tsk;

	TRACE_LDC(DBG_INFO, "core[%d]", coreid);
	TRACE_LDC(DBG_INFO, "core[%px]", core);
	TRACE_LDC(DBG_INFO, "core lock[%px]", &core->core_lock);

	osal_spin_lock_irqsave(&core->core_lock, &flags);
	job = osal_list_first_entry(&core->list, struct ldc_job, node);
	osal_spin_unlock_irqrestore(&core->core_lock, &flags);

	if ((!job)) {
		TRACE_LDC(DBG_ERR, "core[%d] null job", coreid);
		return;
	}
	TRACE_LDC(DBG_INFO, "core[%d]", coreid);

	if (osal_atomic_read(&job->job_state) == LDC_JOB_WORKING) {
		osal_spin_lock_irqsave(&job->lock, &flags);
		tsk = osal_list_first_entry(&job->task_list, struct ldc_task, node);
		osal_spin_unlock_irqrestore(&job->lock, &flags);

		is_last_tsk = ((osal_atomic_read(&job->task_num) > 1) ? false : true);

		ldc_work_handle_tsk_done(dev, tsk, job, is_last_tsk);

		if (job && job->use_cmdq && !is_last_tsk)
			ldc_wkup_cmdq_tsk(core);
	} else {
		TRACE_LDC(DBG_ERR, "invalid job(%px) state(%d).\n"
			, job, (enum ldc_job_state)osal_atomic_read(&job->job_state));
	}
}

#if LDC_USE_WORKQUEUE
static void ldc_work_frm_done(osal_workqueue *work)//intr post handle
{
	struct ldc_core *core = osal_container_of(work, struct ldc_core, work_frm_done);

	ldc_work_handle_frm_done((int)core->dev_type);
}
#endif

//wakeup post handle
void ldc_wkup_frm_done_work(void *data)
{
	struct ldc_core *core = (struct ldc_core *)data;

#if LDC_USE_WORKQUEUE
	//queue_work(dev->workqueue, &dev->work_frm_done);
	osal_workqueue_schedule(&core->work_frm_done);
#else
	ldc_work_handle_frm_done((int)core->dev_type);
#endif
}


static void ldc_set_tsk_run_status(struct ldc_ctx *ctx, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	osal_atomic_set(&ctx->core[top_id].state, LDC_CORE_STATE_RUNNING);

	osal_atomic_set(&tsk->state, LDC_TASK_STATE_RUNNING);

	ldc_proc_record_hw_tsk_start(job, tsk, top_id);
}

static void ldc_submit_hw(struct ldc_ctx *ctx, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	struct ldc_cfg cfg;
	video_frame_s *in_frame, *out_frame;
	// pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, extend_haddr, i;
	unsigned long flags;
	unsigned int bg_color;

	ldc_set_tsk_run_status(ctx, top_id, job, tsk);

	bg_color = tsk->attr.private_data[3];
	mesh_addr = (tsk->attr.private_data[0] != DEFAULT_MESH_PADDR)
		     ? tsk->attr.private_data[0] : 0;
	in_frame = &tsk->attr.img_in.video_frame;
	out_frame = &tsk->attr.img_out.video_frame;
	// pix_format = in_frame->pixel_format;
	rotation = tsk->rotation;

	osal_memset(&cfg, 0, sizeof(cfg));

	if (!job->devs_type)	{
		switch (out_frame->pixel_format) {
		case PIXEL_FORMAT_YUV_400:
			cfg.pix_fmt = YUV400;
			num_of_plane = 1;
			break;
		case PIXEL_FORMAT_NV12:
		case PIXEL_FORMAT_NV21:
		default:
			cfg.pix_fmt = NV21;
			num_of_plane = 2;
			break;
		};

		switch (rotation) {
		case ROTATION_90:
			cfg.dst_mode = LDC_DST_ROT_270;
			break;
		case ROTATION_270:
			cfg.dst_mode = LDC_DST_ROT_90;
			break;
		case ROTATION_XY_FLIP:
			cfg.dst_mode = LDC_DST_XY_FLIP;
			break;
		default:
			cfg.dst_mode = LDC_DST_FLAT;
			break;
		}

		cfg.map_base = mesh_addr;
		cfg.bgcolor = LDC_YUV_BLACK /*ctx->bgcolor*/;
		cfg.src_width = ALIGN(in_frame->width, 64);
		cfg.src_height = ALIGN(in_frame->height, 64);
		cfg.ras_width = cfg.src_width;
		cfg.ras_height = cfg.src_height;

		if (cfg.map_base == 0)
			cfg.map_bypass = true;
		else
			cfg.map_bypass = false;

		cfg.src_xstart = 0;
		cfg.src_xend = in_frame->width - 1;

		extend_haddr = in_frame->phyaddr[0] >> 33;
		cfg.extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

		cfg.src_y_base = in_frame->phyaddr[0];
		cfg.dst_y_base = out_frame->phyaddr[0];
		if (num_of_plane == 2) {
			cfg.src_c_base = in_frame->phyaddr[1];
			cfg.dst_c_base = out_frame->phyaddr[1];
		}
	} else {
		switch (out_frame->pixel_format) {
		case PIXEL_FORMAT_YUV_PLANAR_420:
			cfg.pix_fmt = YUV420p;
			num_of_plane = 3;
		break;
		case PIXEL_FORMAT_YUV_400:
			cfg.pix_fmt = 2;
			num_of_plane = 1;
		break;
		case PIXEL_FORMAT_RGB_888_PLANAR:
		case PIXEL_FORMAT_YUV_PLANAR_444:
		default:
			cfg.pix_fmt = RGB888p;
			num_of_plane = 3;
		break;
		};

		cfg.mesh_id = mesh_addr;
		cfg.map_base = mesh_addr;
		cfg.output_target = 1;//dram or sclr
		cfg.bgcolor = bg_color;
		cfg.bdcolor = (u32)BGCOLOR_GREEN;
		//cfg.bgcolor_dwa = tsk->bgcolor;
		//cfg.bdcolor_dwa = tsk->bdcolor;
		cfg.src_width  = in_frame->width;
		cfg.src_height = in_frame->height;
		cfg.dst_width  = out_frame->width;
		cfg.dst_height = out_frame->height;

		for (i = 0; i < num_of_plane; ++i) {
			unsigned long long addr = in_frame->phyaddr[i];

			cfg.src_buf[i].addrl    = addr;
			cfg.src_buf[i].addrh    = addr >> 32;
			cfg.src_buf[i].pitch    = in_frame->stride[i];
			cfg.src_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;

			addr = out_frame->phyaddr[i];

			cfg.dst_buf[i].addrl    = addr;
			cfg.dst_buf[i].addrh    = addr >> 32;
			cfg.dst_buf[i].pitch    = out_frame->stride[i];
			cfg.dst_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;
		}
	}

	osal_spin_lock_irqsave(&job->lock, &flags);
	job->coreid = top_id;
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	osal_spin_lock_irqsave(&ctx->core[top_id].core_lock, &flags);
	osal_list_add_tail(&job->node, &ctx->core[top_id].list);
	osal_spin_unlock_irqrestore(&ctx->core[top_id].core_lock, &flags);

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	ctx->job_cnt--;
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	TRACE_LDC(DBG_INFO, "job[%px]\n", job);
	TRACE_LDC(DBG_INFO, "core_id:%d\n", top_id);
	TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg.src_width, cfg.src_height);
	TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n",
		in_frame->phyaddr[0], in_frame->phyaddr[1], in_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n",
		out_frame->phyaddr[0], out_frame->phyaddr[1], out_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg.mesh_id);
	TRACE_LDC(DBG_DEBUG, "update map_base(%#llx)\n", cfg.map_base);
	TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n", cfg.bgcolor, cfg.pix_fmt);

	ldc_reset(top_id);
	ldc_init(top_id);
	if (top_id >= DEV_DWA_0)
		ldc_intr_ctrl(DWA_INTR_EN_ALL, top_id);
	else
		ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
	ldc_engine(&cfg, top_id);
#if 0
	ldc_dump_register(top_id);
#endif
}

static int ldc_wait_cmdq_evt_func(const void *param)
{
	struct ldc_core *core = (struct ldc_core *)param;

	return core->cmdq_evt;
}

static void ldc_submit_hw_cmdq(struct ldc_ctx *ctx, int top_id
	, struct ldc_job *job, struct ldc_task *last_tsk, struct ldc_task **tskq)
{
	struct ldc_cfg *cfg_q[LDC_JOB_MAX_TSK_NUM] = {NULL};
	union cmdq_set *cmdq_addr = NULL;
	video_frame_s *in_frame, *out_frame;
	// pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, i, tsk_idx, extend_haddr;
	int tsk_num, cmdq_wq_ret;
	unsigned long flags;
	unsigned int bg_color;

	if ((!ctx || !job || !last_tsk || !tskq))
		return;

	tsk_num = osal_atomic_read(&job->task_num);
	if ((last_tsk != tskq[tsk_num - 1])) {
		TRACE_LDC(DBG_ERR, "invalid last_tsk, not match with tskq\n");
		return;
	}

	cmdq_addr = osal_kzalloc(sizeof(*cmdq_addr) * tsk_num * LDC_CMDQ_MAX_REG_CNT, OSAL_GFP_ATOMIC);

	osal_spin_lock_irqsave(&job->lock, &flags);
	job->coreid = top_id;
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	osal_spin_lock_irqsave(&ctx->core[top_id].core_lock, &flags);
	osal_list_add_tail(&job->node, &ctx->core[top_id].list);
	ctx->core[top_id].cmdq_evt = false;
	osal_spin_unlock_irqrestore(&ctx->core[top_id].core_lock, &flags);

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	ctx->job_cnt--;
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	for (tsk_idx = 0; tsk_idx < tsk_num; tsk_idx++) {

		ldc_set_tsk_run_status(ctx, top_id, job, tskq[tsk_idx]);

		cfg_q[tsk_idx] = osal_kzalloc(sizeof(struct ldc_cfg), OSAL_GFP_ATOMIC);

		bg_color = tskq[tsk_idx]->attr.private_data[3];
		mesh_addr = (tskq[tsk_idx]->attr.private_data[0] != DEFAULT_MESH_PADDR)
			     ? tskq[tsk_idx]->attr.private_data[0] : 0;
		in_frame = &tskq[tsk_idx]->attr.img_in.video_frame;
		out_frame = &tskq[tsk_idx]->attr.img_out.video_frame;
		// pix_format = in_frame->pixel_format;
		rotation = tskq[tsk_idx]->rotation;

		if (!job->devs_type) {
			switch (out_frame->pixel_format) {
			case PIXEL_FORMAT_YUV_400:
				cfg_q[tsk_idx]->pix_fmt = YUV400;
				num_of_plane = 1;
				break;
			case PIXEL_FORMAT_NV12:
			case PIXEL_FORMAT_NV21:
			default:
				cfg_q[tsk_idx]->pix_fmt = NV21;
				num_of_plane = 2;
				break;
			};

			switch (rotation) {
			case ROTATION_90:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_ROT_270;
				break;
			case ROTATION_270:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_ROT_90;
				break;
			case ROTATION_XY_FLIP:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_XY_FLIP;
				break;
			default:
				cfg_q[tsk_idx]->dst_mode = LDC_DST_FLAT;
				break;
			}

			cfg_q[tsk_idx]->map_base = mesh_addr;
			cfg_q[tsk_idx]->bgcolor = LDC_YUV_BLACK /*ctx->bgcolor*/;
			cfg_q[tsk_idx]->src_width = ALIGN(in_frame->width, LDC_SIZE_ALIGN);
			cfg_q[tsk_idx]->src_height = ALIGN(in_frame->height, LDC_SIZE_ALIGN);
			cfg_q[tsk_idx]->ras_width = cfg_q[tsk_idx]->src_width;
			cfg_q[tsk_idx]->ras_height = cfg_q[tsk_idx]->src_height;

			if (cfg_q[tsk_idx]->map_base == 0)
				cfg_q[tsk_idx]->map_bypass = true;
			else
				cfg_q[tsk_idx]->map_bypass = false;

			cfg_q[tsk_idx]->src_xstart = 0;
			cfg_q[tsk_idx]->src_xend = in_frame->width - 1;

			extend_haddr = in_frame->phyaddr[0] >> 33;
			cfg_q[tsk_idx]->extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

			cfg_q[tsk_idx]->src_y_base = in_frame->phyaddr[0];
			cfg_q[tsk_idx]->dst_y_base = out_frame->phyaddr[0];
			if (num_of_plane == 2) {
				cfg_q[tsk_idx]->src_c_base = in_frame->phyaddr[1];
				cfg_q[tsk_idx]->dst_c_base = out_frame->phyaddr[1];
			}
		} else {
			switch (out_frame->pixel_format) {
			case PIXEL_FORMAT_YUV_PLANAR_420:
				cfg_q[tsk_idx]->pix_fmt = YUV420p;
				num_of_plane = 3;
			break;
			case PIXEL_FORMAT_YUV_400:
				cfg_q[tsk_idx]->pix_fmt = 2;
				num_of_plane = 1;
			break;
			case PIXEL_FORMAT_RGB_888_PLANAR:
			case PIXEL_FORMAT_YUV_PLANAR_444:
			default:
				cfg_q[tsk_idx]->pix_fmt = RGB888p;
				num_of_plane = 3;
			break;
			};

			cfg_q[tsk_idx]->mesh_id = mesh_addr;
			cfg_q[tsk_idx]->output_target = 1;//dram or sclr
			cfg_q[tsk_idx]->bgcolor = bg_color;
			cfg_q[tsk_idx]->bdcolor = (u32)BGCOLOR_GREEN;
			//cfg_q[tsk_idx]->bgcolor = tsk->bgcolor;
			//cfg_q[tsk_idx]->bdcolor = tsk->bdcolor;
			cfg_q[tsk_idx]->src_width  = in_frame->width;
			cfg_q[tsk_idx]->src_height = in_frame->height;
			cfg_q[tsk_idx]->dst_width  = out_frame->width;
			cfg_q[tsk_idx]->dst_height = out_frame->height;

			for (i = 0; i < num_of_plane; ++i) {
				unsigned long long addr = in_frame->phyaddr[i];

				cfg_q[tsk_idx]->src_buf[i].addrl    = addr;
				cfg_q[tsk_idx]->src_buf[i].addrh    = addr >> 32;
				cfg_q[tsk_idx]->src_buf[i].pitch    = in_frame->stride[i];
				cfg_q[tsk_idx]->src_buf[i].offset_x = cfg_q[tsk_idx]->src_buf[i].offset_y = 0;

				addr = out_frame->phyaddr[i];

				cfg_q[tsk_idx]->dst_buf[i].addrl    = addr;
				cfg_q[tsk_idx]->dst_buf[i].addrh    = addr >> 32;
				cfg_q[tsk_idx]->dst_buf[i].pitch    = out_frame->stride[i];
				cfg_q[tsk_idx]->dst_buf[i].offset_x = cfg_q[tsk_idx]->src_buf[i].offset_y = 0;
			}
		}

		ldc_reset(top_id);
		ldc_init(top_id);
		if (top_id >= DEV_DWA_0)
			ldc_intr_ctrl(DWA_INTR_EN_ALL, top_id);
		else
			ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
		ldc_engine(cfg_q[tsk_idx], top_id);

		if (tsk_idx != (tsk_num - 1)) {
			cmdq_wq_ret = osal_wait_timeout_uninterruptible(&ctx->core[top_id].cmdq_wq,
				ldc_wait_cmdq_evt_func,
				&ctx->core[top_id],
				LDC_EOF_WAIT_TIMEOUT_MS);
			if (cmdq_wq_ret <= 0) {
				TRACE_LDC(DBG_WARN, "ldc cdmq wait timeout, ret(%d)\n", cmdq_wq_ret);
				goto FREE_CMDQ_RES;
			}
			TRACE_LDC(DBG_DEBUG, "ldc cdmq wait done, ret(%d)\n", cmdq_wq_ret);
		}
	}

FREE_CMDQ_RES:
	TRACE_LDC(DBG_DEBUG, "cmd_start(%px)\n", cmdq_addr);
	for (i = 0; i < tsk_idx; i++) {
		TRACE_LDC(DBG_DEBUG, "tsk_id[%d]-core_id[%d]\n", i,  top_id);
		if (top_id >= DEV_DWA_0) {
			TRACE_LDC(DBG_DEBUG, "update size src(%d %d) dst(%d %d)\n",
				cfg_q[i]->src_width, cfg_q[i]->src_height
				, cfg_q[i]->dst_width, cfg_q[i]->dst_height);
			TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n"
				, tskq[i]->attr.img_in.video_frame.phyaddr[0]
				, tskq[i]->attr.img_in.video_frame.phyaddr[1]
				, tskq[i]->attr.img_in.video_frame.phyaddr[2]);
			TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n"
				, tskq[i]->attr.img_out.video_frame.phyaddr[0]
				, tskq[i]->attr.img_out.video_frame.phyaddr[1]
				, tskq[i]->attr.img_out.video_frame.phyaddr[2]);
			TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->mesh_id);
			TRACE_LDC(DBG_DEBUG, "update output_target(%d)\n", cfg_q[i]->output_target);
			TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
				, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);
			TRACE_LDC(DBG_DEBUG, "update src pitch(%d %d %d)\n"
				, cfg_q[i]->src_buf[0].pitch, cfg_q[i]->src_buf[1].pitch, cfg_q[i]->src_buf[2].pitch);
			TRACE_LDC(DBG_DEBUG, "update dst pitch(%d %d %d)\n"
				, cfg_q[i]->dst_buf[0].pitch, cfg_q[i]->dst_buf[1].pitch, cfg_q[i]->dst_buf[2].pitch);
		} else {
			TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg_q[i]->src_width, cfg_q[i]->src_height);
			TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx\n",
				cfg_q[i]->src_y_base, cfg_q[i]->src_c_base);
			TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx\n",
				cfg_q[i]->dst_y_base, cfg_q[i]->dst_c_base);
			TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->map_base);
			TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
				, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);
		}

		osal_kfree(cfg_q[i]);
	}

	osal_kfree(cmdq_addr);
}

static int ldc_try_submit_hw(struct ldc_ctx *ctx, struct ldc_job *job
	, struct ldc_task *tsk, unsigned char use_cmdq, struct ldc_task **tskq)
{
	int coreid, top_id, ret = -1;
	enum ldc_core_state state;
	unsigned char core_num;

	if (!job->devs_type) {
		coreid = DEV_LDC_0;
		core_num = ctx->core_num;
	} else {
		coreid = DEV_DWA_0;
		core_num = ctx->core_num;
	}

	for (; coreid < core_num; coreid++) {
		state = osal_atomic_read(&ctx->core[coreid].state);

		if (state == LDC_CORE_STATE_IDLE) {
			top_id = coreid;
			if (use_cmdq)
				ldc_submit_hw_cmdq(ctx, top_id, job, tsk, tskq);
			else
				ldc_submit_hw(ctx, top_id, job, tsk);
			ret = 0;
			break;
		}
	}

	if (ret)
		TRACE_LDC(DBG_NOTICE, "ldc_submit_hw fail,hw busy\n");

	return ret;
}

static int ldc_try_submit_hw_cmdq(struct ldc_ctx *ctx, struct ldc_job *job
	, struct ldc_task **tskq, struct ldc_task *last_tsk)
{
	return ldc_try_submit_hw(ctx, job, last_tsk, true, tskq);
}

static unsigned char ldc_is_tsk_ready(struct ldc_task *tsk)
{
	if ((!tsk)) {
		TRACE_LDC(DBG_ERR, "cur tsk is null\n");
		return false;
	}

	if ((tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
		TRACE_LDC(DBG_ERR, "cur tsk id(%d) invalid\n", tsk->tsk_id);
		return false;
	}

	return true;
}

static bool ldc_have_idle_core(struct ldc_ctx *ctx, enum ldc_job_devs_type devs_type)
{
	unsigned char coreid;
	unsigned char core_num;
	enum ldc_core_state state;

	if (devs_type == DEVS_LDC) {
		coreid = DEV_LDC_0;
		core_num = LDC_DEV_MAX_CNT;
	}

	for (; coreid < core_num; coreid++) {
		state = osal_atomic_read(&ctx->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			return true;
	}
	return false;
}

#if 0
static bool ldc_ctx_idle(struct ldc_ctx *ctx)
{
	unsigned char coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < LDC_DEV_MAX_CNT; coreid++) {
		state = osal_atomic_read(&ctx->core[coreid].state);
		if (state != LDC_CORE_STATE_IDLE)
			return false;
	}

	return true;
}
#endif

static void ldc_try_commit_job(struct ldc_ctx *ctx, struct ldc_job *job)
{
	unsigned long flags;
	unsigned char use_cmdq = false, is_ready = true;
	unsigned char i = 0, tsk_num;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL, *tskq[LDC_JOB_MAX_TSK_NUM], *last_tsk = NULL;

	if (!ctx || !job)
		return;

	if (osal_atomic_read(&job->job_state) != LDC_JOB_WORKING)
		return;

	tsk_num = osal_atomic_read(&job->task_num);

	if (tsk_num == 1) {
		use_cmdq = false;
		osal_spin_lock_irqsave(&job->lock, &flags);
		tsk = osal_list_first_entry(&job->task_list, struct ldc_task, node);
		osal_spin_unlock_irqrestore(&job->lock, &flags);
		is_ready = ldc_is_tsk_ready(tsk);
	} else {
		use_cmdq = true;
		i = 0;
		osal_spin_lock_irqsave(&job->lock, &flags);
		osal_list_for_each_entry_safe(tskq[i], tmp_tsk, &job->task_list, node) {
			is_ready &= ldc_is_tsk_ready(tskq[i]);
			last_tsk = tskq[i];
			i++;
		}
		osal_spin_unlock_irqrestore(&job->lock, &flags);
	}

	if (!is_ready) {
		TRACE_LDC(DBG_ERR, "cur job(%px) cur tsk(%px) is not ready\n", job, tsk);
		return;
	}

	job->use_cmdq = use_cmdq;
	if (use_cmdq) {
		if ((!last_tsk)) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is null\n");
			return;
		}
		if ((!osal_list_is_last(&last_tsk->node, &job->task_list))) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is not last node\n");
			return;
		}
		ldc_try_submit_hw_cmdq(ctx, job, tskq, last_tsk);
	} else {
		ldc_try_submit_hw(ctx, job, tsk, false, NULL);
	}
}

static int ldc_wait_cond_func(const void *param)
{
	struct ldc_ctx *ctx = (struct ldc_ctx *)param;

	return ctx->evt;
}

static int ldc_event_handler_th(void *data)
{
	struct ldc_ctx *ctx = (struct ldc_ctx *)data;
	unsigned long flags;
	struct ldc_job *job;
	int ret;
	unsigned long idle_timeout = LDC_IDLE_WAIT_TIMEOUT_MS;
	unsigned long eof_timeout = LDC_EOF_WAIT_TIMEOUT_MS;
	unsigned long timeout = idle_timeout;

	if (!ctx) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return -1;
	}

	while (!osal_kthread_should_stop()) {
		if (!ctx)
			break;

		ret = osal_wait_timeout_interruptible(&ctx->wait, ldc_wait_cond_func,
			ctx, timeout);

		/* -ERESTARTSYS */
		if (ret < 0)
			break;

		/* timeout */
		if (!ret) {
			if (osal_list_empty(&ctx->job_list)) {
				timeout = idle_timeout;
				continue;
			} else
				TRACE_LDC(DBG_NOTICE, "timeout but job list not empty\n");
		}

		if (osal_list_empty(&ctx->job_list)) {
			TRACE_LDC(DBG_DEBUG, "job list empty\n");
			goto continue_th;
		}

		if (ctx->suspend == true) {
			goto continue_th;
		}

		osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
		job = osal_list_first_entry(&ctx->job_list, struct ldc_job, node);

		if (!ldc_have_idle_core(ctx, job->devs_type)) {
			TRACE_LDC(DBG_INFO, "core busy, not have idle core\n");
			osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);
			goto continue_th;
		}

		osal_list_del(&job->node);
		osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

		TRACE_LDC(DBG_INFO, "send job[[%px]]\n", job);

		osal_atomic_set(&job->job_state, LDC_JOB_WORKING);
		osal_atomic_set(&ctx->state, LDC_DEV_STATE_RUNNING);

		ldc_proc_record_job_start(job);
		ldc_try_commit_job(ctx, job);
continue_th:
		ldc_clr_evt_kth(ctx);

		/* Adjust timeout */
		timeout = osal_list_empty(&ctx->job_list) ? idle_timeout : eof_timeout;
	}

	return 0;
}

static s32 ldc_stop_handler(void)
{
	int ret = 0;
	struct ldc_ctx *ctx = get_ldc_ctx();

	if(ctx->thread_created == false)
		return 0;
	osal_kthread_destroy(ctx->thread, ctx->thread_created);
	if (ret)
		TRACE_LDC(DBG_ERR, "fail to stop gdc thread, err=%d\n", ret);
	ctx->thread_created = false;
	return ret;

}

static s32 ldc_start_handler(void)
{
	int ret = 0;
	struct ldc_ctx *ctx = get_ldc_ctx();

	if(ctx->thread_created == true)
		return 0;

	ctx->thread = osal_kthread_create(ldc_event_handler_th, (void *)ctx, "ldc_event_handler_th", DEFAULT_STACK_SIZE);
	if (!(ctx->thread)) {
		TRACE_LDC(DBG_ERR, "failed to create ldc kthread\n");
		return -1;
	}
	osal_kthread_set_priority(ctx->thread, OSAL_TASK_PRIORITY_HIGH);
	ctx->thread_created = true;
	return ret;
}
/**************************************************************************
 *   Public APIs.
 **************************************************************************/
int ldc_suspend(void)
{
	int ret = 0;
	int cnt;
	unsigned char coreid;
	struct ldc_ctx *ctx = get_ldc_ctx();

	/* Step 1: stop all event handlers */
	ret = ldc_stop_handler();
	if (ret) {
		TRACE_LDC(DBG_ERR, "fail to stop gdc thread.\n");
		return ret;
	}
	TRACE_LDC(DBG_ERR, "gdc thread stopped\n");

	/* Step 2: turn off clock after ensuring all cores are idle */
	osal_mutex_lock(&ldc_reg_lock);

	for (cnt = 0; cnt <= 500; cnt++) {
		bool all_idle = true;
		for (coreid = 0; coreid < ctx->core_num; coreid++) {
			if (osal_atomic_read(&ctx->core[coreid].state) != LDC_CORE_STATE_IDLE) {
				all_idle = false;
				break;
			}
		}
		if (all_idle)
			break;
		osal_msleep(2);
	}

	if (cnt > 500) {
		osal_mutex_unlock(&ldc_reg_lock);
		TRACE_LDC(DBG_ERR, "not all cores are idle, can't suspend\n");
		return -1;
	}

	ldc_clk_deinit(ctx);
	osal_mutex_unlock(&ldc_reg_lock);

	TRACE_LDC(DBG_ERR, "gdc suspended!\n");
	return ret;
}

int ldc_core_suspend(void)
{
	int ret = 0;
	int cnt;
	unsigned char coreid;
	struct ldc_ctx *ctx = get_ldc_ctx();

	osal_mutex_lock(&ldc_reg_lock);

	ctx->suspend = true;

	for (cnt = 0; cnt <= 500; cnt++) {
		bool all_idle = true;
		for (coreid = 0; coreid < ctx->core_num; coreid++) {
			if (osal_atomic_read(&ctx->core[coreid].state) != LDC_CORE_STATE_IDLE) {
				all_idle = false;
				break;
			}
		}
		if (all_idle)
			break;
		osal_msleep(2);
	}

	if (cnt > 500) {
		osal_mutex_unlock(&ldc_reg_lock);
		TRACE_LDC(DBG_ERR, "not all cores are idle, can't suspend\n");
		return -1;
	}

	osal_mutex_unlock(&ldc_reg_lock);

	return ret;
}

s32 ldc_resume(void)
{
	int ret = 0;
	struct ldc_ctx *ctx = get_ldc_ctx();

	/*step 1 start all envent_handlers*/
    	ret = ldc_start_handler();
	if (ret) {
		TRACE_LDC(DBG_ERR, "fail to restart gdc thread.\n");
		return ret;
	} else
		TRACE_LDC(DBG_ERR, "gdc thread restarted\n");

	/*step 2 turn on clock*/
	osal_mutex_lock(&ldc_reg_lock);
	ldc_clk_init(ctx);
	osal_mutex_unlock(&ldc_reg_lock);

	TRACE_LDC(DBG_ERR, "gdc resume!\n");
	return ret;
}

s32 ldc_core_resume(void)
{
	int ret = 0;

	ctx->suspend = false;

	return ret;
}

int ldc_begin_job(struct ldc_ctx *ctx, struct gdc_handle_data *data)
{
	struct ldc_job *job;
	int ret = 0;

	ret = ldc_check_null_ptr(ctx) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	job = osal_kzalloc(sizeof(struct ldc_job), OSAL_GFP_ATOMIC);
	if (job == NULL) {
		TRACE_LDC(DBG_ERR, "malloc failed.\n");
		return ERR_GDC_NOBUF;
	}

	OSAL_INIT_LIST_HEAD(&job->task_list);
	osal_spin_lock_init(&job->lock);

	osal_atomic_set(&job->job_state, LDC_JOB_CREAT);
	osal_atomic_set(&job->task_num, 0);
	job->identity.sync_io = true;
	job->devs_type = DEVS_MAX;
	osal_sem_init(&job->job_done_sem, 0);

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]++\n", job);

	return ret;
}

int ldc_end_job(struct ldc_ctx *ctx, unsigned long long handle)
{
	int ret = 0, sync_io_ret;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)handle;
	unsigned long flags;
	int tsk_num = 0;
	unsigned long timeout = LDC_SYNC_IO_WAIT_TIMEOUT_MS;
	struct ldc_task *tsk, *tmp_tsk;

	ret = ldc_check_null_ptr(ctx) ||
		ldc_check_null_ptr(job);
	if (ret)
		return ret;

	if (osal_list_empty(&job->task_list)) {
		TRACE_LDC(DBG_DEBUG, "no task in job.\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	if (ctx->job_cnt >= END_JOB_MAX_LEN) {
		TRACE_LDC(DBG_ERR, "job_cnt is full.\n");
		osal_sem_destroy(&job->job_done_sem);
		osal_spin_lock_destroy(&job->lock);
		osal_kfree(job);
		return ERR_GDC_BUF_FULL;
	}

	//mutex_lock(&g_io_lock);
	osal_spin_lock_irqsave(&job->lock, &flags);

	osal_list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
		tsk_num++;
		tsk->tsk_id = tsk_num - 1;
	}

	job->coreid = LDC_INVALID_CORE_ID;
	osal_atomic_set(&job->job_state, LDC_JOB_WAIT);
	osal_atomic_set(&job->task_num, tsk_num);
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	ldc_proc_commit_job(job);

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	osal_list_add_tail(&job->node, &ctx->job_list);
	ctx->job_cnt++;
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	ldc_notify_wkup_evt_kth(ctx, LDC_EVENT_WKUP);
	//mutex_unlock(&g_io_lock);

	TRACE_LDC(DBG_INFO, "job[%px] name[%s] sync_io=%d\n", job, job->identity.name, job->identity.sync_io);

	if (job->identity.sync_io) {
		sync_io_ret = osal_sem_down_timeout(&job->job_done_sem, timeout);
		if (sync_io_ret) {
			TRACE_LDC(DBG_WARN, "end job[%px] fail,timeout, ret(%d)\n", job, sync_io_ret);
			return sync_io_ret;
		}
		osal_sem_destroy(&job->job_done_sem);
		osal_spin_lock_destroy(&job->lock);
		osal_kfree(job);
	}

	return ret;
}

static int ldc_cancel_wait_job(struct ldc_ctx *ctx, struct ldc_job *job_handle)
{
	struct ldc_job *tmp, *job;
	bool wait_flag = false;
	unsigned long flags;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL;

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	osal_list_for_each_entry_safe(job, tmp, &ctx->job_list, node) {
		if (job == job_handle) {
			wait_flag = true;
			break;
		}
	}
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	if (wait_flag) {
		osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
		osal_list_del(&job_handle->node);
		ctx->job_cnt--;
		osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

		osal_sem_destroy(&job->job_done_sem);
		osal_spin_lock_destroy(&job->lock);

		osal_list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
			osal_kfree(tsk);
		}

		osal_kfree(job_handle);
		TRACE_LDC(DBG_NOTICE, "cancel ctx job_list job[%px]\n", job_handle);
		return 0;
	}

	return -1;
}

static int ldc_cancel_work_job(struct ldc_ctx *ctx, struct ldc_job *job_handle)
{
	unsigned long flags;
	bool work_flag = false;
	struct ldc_core *core;
	struct ldc_job *tmp, *job;
	int coreid;
	int state;
	int count = 5000;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL;

	for (coreid = 0; coreid < LDC_DEV_MAX_CNT; coreid++) {
		core = &ctx->core[coreid];
		if (osal_list_empty(&core->list))
			continue;

		osal_spin_lock_irqsave(&core->core_lock, &flags);
		osal_list_for_each_entry_safe(job, tmp, &core->list, node) {
			if (job == job_handle) {
				work_flag = true;
				break;
			}
		}
		osal_spin_unlock_irqrestore(&core->core_lock, &flags);

		if (work_flag)
			break;
	}

	if (work_flag) {
		while (--count > 0) {
			state = osal_atomic_read(&core->state);
			if (state == LDC_CORE_STATE_RUNNING)
				osal_usleep_range(100, 200);
			else
				break;
		}

		if (state == LDC_CORE_STATE_RUNNING) {
			osal_atomic_set(&core->state, LDC_CORE_STATE_IDLE);

			osal_spin_lock_irqsave(&core->core_lock, &flags);
			osal_list_del(&job_handle->node);
			osal_spin_unlock_irqrestore(&core->core_lock, &flags);

			osal_sem_destroy(&job->job_done_sem);
			osal_spin_lock_destroy(&job->lock);

			osal_list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
				osal_kfree(tsk);
			}

			osal_kfree(job_handle);
			ldc_core_deinit(coreid);
			TRACE_LDC(DBG_NOTICE, "cancel core[%d] workjob[%px]\n", job_handle->coreid, job_handle);
			TRACE_LDC(DBG_NOTICE, "cur core if timeout, need reset\n");
		} else
			TRACE_LDC(DBG_NOTICE, "core[%d] irq have cancel this job[%px] done\n",
				job_handle->coreid, job_handle);
	}

	return 0;
}

int ldc_cancel_job(struct ldc_ctx *ctx, unsigned long long handle)
{
	int ret = 0;
	struct ldc_job *job_handle = (struct ldc_job *)(uintptr_t)handle;

	TRACE_LDC(DBG_NOTICE, "==enter\n");
	ret = ldc_check_null_ptr(ctx) ||
		ldc_check_null_ptr(job_handle);
	if (ret)
		return ret;

	if (ldc_cancel_wait_job(ctx, job_handle) == 0)
		return ret;

	ret = ldc_cancel_work_job(ctx, job_handle);
	return ret;
}

int ldc_get_work_job(struct ldc_ctx *ctx, struct gdc_handle_data *data)
{
	int ret = 0;
	struct ldc_job *job = NULL;
	unsigned long flags;
	unsigned char coreid;

	ret = ldc_check_null_ptr(ctx) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	for (coreid = 0; coreid < ctx->core_num; coreid++) {
		osal_spin_lock_irqsave(&ctx->core[coreid].core_lock, &flags);
		job = osal_list_first_entry(&ctx->core[coreid].list, struct ldc_job, node);
		osal_spin_unlock_irqrestore(&ctx->core[coreid].core_lock, &flags);

		if (job)
			break;
	}

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]\n", job);

	return ret;
}

int ldc_set_identity(struct ldc_ctx *ctx,
			  struct gdc_identity_attr *identity)
{
	struct ldc_job *job;
	unsigned long flags;
	unsigned long long handle;

	if (ldc_check_null_ptr(ctx) || ldc_check_null_ptr(identity)) {
		TRACE_LDC(DBG_ERR, "null dev or identity_attr\n");
		return -1;
	}
	handle = identity->handle;

	job = (struct ldc_job *)(uintptr_t)handle;
	if (!job) {
		TRACE_LDC(DBG_ERR, "null job handle\n");
		return -1;
	}

	osal_spin_lock_irqsave(&job->lock, &flags);
	osal_memcpy(&job->identity, &identity->attr, sizeof(identity->attr));
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	TRACE_LDC(DBG_DEBUG, "sync_io:%d, name:%s, id:%d\n"
		, job->identity.sync_io, job->identity.name, job->identity.id);
	return 0;
}

int ldc_add_rotation_task(struct ldc_ctx *ctx,
			  struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(ctx, attr))
		return -1;
	handle = attr->handle;

	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = osal_kzalloc(sizeof(*tsk), OSAL_GFP_ATOMIC);

	job->devs_type = DEVS_LDC;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	osal_memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_ROT;
	tsk->rotation = attr->rotation;
	osal_atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	osal_atomic_inc_return(&job->task_num);

	osal_spin_lock_irqsave(&job->lock, &flags);
	osal_list_add_tail(&tsk->node, &job->task_list);
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_ldc_task(struct ldc_ctx *ctx, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(ctx, attr))
		return -1;
	handle = attr->handle;

#if 0
	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;
#endif

	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = osal_kzalloc(sizeof(*tsk), OSAL_GFP_ATOMIC);

	job->devs_type = DEVS_LDC;
	TRACE_LDC(DBG_INFO, "job[%px], job->devs_type is %d\n", job, job->devs_type);

	osal_memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_LDC;
	tsk->rotation = attr->rotation;
	osal_atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	osal_atomic_inc_return(&job->task_num);

	osal_spin_lock_irqsave(&job->lock, &flags);
	osal_list_add_tail(&tsk->node, &job->task_list);
	osal_spin_unlock_irqrestore(&job->lock, &flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_get_chn_frame(struct ldc_ctx *ctx, struct gdc_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	int ret;
	unsigned long flags;
	struct ldc_vb_done *vb_done = NULL, *vb_done_tmp = NULL;
	bool ismatch = false;

	TRACE_LDC(DBG_DEBUG, "++\n");
	ret = ldc_check_null_ptr(pstvideo_frame)
		|| ldc_check_null_ptr(ctx)
		|| ldc_check_null_ptr(identity);
	if (ret)
		return ret;

	osal_memset(pstvideo_frame, 0, sizeof(*pstvideo_frame));
	if (s32milli_sec <= 0) {
		if (osal_sem_trydown(&ctx->vb_doneq.sem)) {
			TRACE_LDC(DBG_ERR, "cannot get sem, doneq not ready\n");
			return ERR_GDC_SYS_NOTREADY;
		}
	} else {
		ret = osal_sem_down_timeout(&ctx->vb_doneq.sem, s32milli_sec);
		if (ret) {
			TRACE_LDC(DBG_ERR, "get sem timeout, doneq not ready\n");
			return ret;
		}
	}

	if (osal_list_empty(&ctx->vb_doneq.doneq)) {
		TRACE_LDC(DBG_ERR, "vb_doneq is empty\n");
		return ERR_GDC_NOBUF;
	}

	osal_spin_lock_irqsave(&ctx->vb_doneq.lock, &flags);
	osal_list_for_each_entry_safe(vb_done, vb_done_tmp, &ctx->vb_doneq.doneq, node) {
		if (ldc_identity_is_match(&vb_done->job.identity, &identity->attr)) {
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s] is match [%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name
				, identity->attr.mod_id, identity->attr.id, identity->attr.name);
			ismatch = true;
			break;
		}
	}
	if (ismatch)
		osal_list_del(&vb_done->node);
	osal_spin_unlock_irqrestore(&ctx->vb_doneq.lock, &flags);

	if (!ismatch) {
		TRACE_LDC(DBG_DEBUG, "vb_doneq[%px] identity[%d-%d-%s] not match [%d-%d-%s]\n"
			, vb_done_tmp, vb_done_tmp->job.identity.mod_id, vb_done_tmp->job.identity.id
			, vb_done_tmp->job.identity.name
			, identity->attr.mod_id, identity->attr.id, identity->attr.name);
		return ERR_GDC_NOBUF;
	}

	osal_memcpy(pstvideo_frame, &vb_done->img_out, sizeof(*pstvideo_frame));
	osal_kfree(vb_done);

	TRACE_LDC(DBG_DEBUG, "end to get pstvideo_frame width:%d height:%d buf:0x%llx\n"
		, pstvideo_frame->video_frame.width
		, pstvideo_frame->video_frame.height
		, pstvideo_frame->video_frame.phyaddr[0]);
	TRACE_LDC(DBG_DEBUG, "--\n");

	return 0;
}

int ldc_attach_vb_pool(struct ldc_vb_pool_cfg *cfg)
{
	unsigned long flags;
	struct ldc_ctx *ctx = get_ldc_ctx();
	vb_pool vb_pool = (unsigned int)cfg->vb_pool;
	mmf_chn_s *chn = (mmf_chn_s *)(&(cfg->mmf_chn));

	if (!ctx)
		return -1;

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	ctx->vb_pool[chn->mod_id == ID_VI ? 0 : chn->mod_id == ID_VPSS ? 1 : 2][chn->dev_id][chn->chn_id] = vb_pool;
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	TRACE_LDC(DBG_DEBUG, "attach vb pool(%d) mod %d dev %d chn %d done\n", vb_pool, chn->mod_id, chn->dev_id, chn->chn_id);

	return 0;
}

int ldc_detach_vb_pool(struct ldc_vb_pool_cfg *cfg)
{
	unsigned long flags;
	struct ldc_ctx *ctx = get_ldc_ctx();
	mmf_chn_s *chn = (mmf_chn_s *)(&(cfg->mmf_chn));

	if (!ctx)
		return -1;

	osal_spin_lock_irqsave(&ctx->ctx_lock, &flags);
	ctx->vb_pool[chn->mod_id == ID_VI ? 0 : chn->mod_id == ID_VPSS ? 1 : 2][chn->dev_id][chn->chn_id] = VB_INVALID_POOLID;
	osal_spin_unlock_irqrestore(&ctx->ctx_lock, &flags);

	TRACE_LDC(DBG_DEBUG, "dettach vb pool mod %d dev %d chn %d done\n", chn->mod_id, chn->dev_id, chn->chn_id);
	return 0;
}

int ldc_get_internal_chn_attr(struct ldc_ctx *wdev, struct ldc_internal_chn_attr *attr)
{
	int ret = 0;
	struct ldc_op_done_cfg ldc_cfg;
	struct base_exe_m_cb exe_cb;

	cb_modules_id callee = convert_cb_id(attr->mod);

	TRACE_LDC(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, attr->mod);
	ldc_cfg.param = (void *)attr;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_GET_CHN_ATTR;
	exe_cb.data   = &ldc_cfg;
	ret = base_exe_module_cb(&exe_cb);
	return ret;
}

int ldc_set_internal_chn_ldc_cfg(struct ldc_ctx *wdev, struct ldc_internal_chn_ldc_cfg *cfg)
{
	int ret = 0;
	struct ldc_op_done_cfg ldc_cfg;
	struct base_exe_m_cb exe_cb;

	cb_modules_id callee = convert_cb_id(cfg->mod);

	TRACE_LDC(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, cfg->mod);
	ldc_cfg.param = (void *)cfg;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_SET_CHN_CFG;
	exe_cb.data   = &ldc_cfg;
	ret = base_exe_module_cb(&exe_cb);
	return ret;
}
/**************************************************************************
 *   internal APIs.
 **************************************************************************/
int ldc_sw_init(struct ldc_ctx *ctx)
{
	int ret = 0, i, j, k;
	unsigned char coreid;

	ret = ldc_check_null_ptr(ctx);
	if (ret)
		return ret;

	OSAL_INIT_LIST_HEAD(&ctx->vb_doneq.doneq);
	osal_sem_init(&ctx->vb_doneq.sem, 0);
	osal_spin_lock_init(&ctx->vb_doneq.lock);
	osal_wait_init(&ctx->wait);
	osal_spin_lock_init(&ctx->ctx_lock);
	OSAL_INIT_LIST_HEAD(&ctx->job_list);
	osal_mutex_init(&ldc_reg_lock);
	osal_mutex_init(&g_mesh_lock);
	ctx->core_num = LDC_DEV_MAX_CNT;
	ctx->evt = LDC_EVENT_BUSY_OR_NOT_STAT;
	for (i = 0; i < MAX_CB_MOD_NUM; i++) {
		for (j = 0; j < MAX_CB_DEV_NUM; j++) {
			for (k = 0; k < MAX_CB_CHN_NUM; k++) {
				ctx->vb_pool[i][j][k] = VB_INVALID_POOLID;
			}
		}
	}
	ctx->job_cnt = 0;
	for (coreid = 0; coreid < ctx->core_num; coreid++) {
#if LDC_USE_WORKQUEUE
		osal_workqueue_init(&ctx->core[coreid].work_frm_done, ldc_work_frm_done);
#endif
		OSAL_INIT_LIST_HEAD(&ctx->core[coreid].list);
		osal_wait_init(&ctx->core[coreid].cmdq_wq);
		osal_spin_lock_init(&ctx->core[coreid].core_lock);
	}

	ctx->evt = 0;
	ctx->stop_flag = 0;

	ctx->thread = osal_kthread_create(ldc_event_handler_th, (void *)ctx, "ldc_event_handler_th", DEFAULT_STACK_SIZE);
	if (!(ctx->thread)) {
		TRACE_LDC(DBG_ERR, "failed to create ldc kthread\n");
		return -1;
	}
	ctx->thread_created = true;
	osal_kthread_set_priority(ctx->thread, OSAL_TASK_PRIORITY_HIGH);
	return ret;
}

void ldc_sw_deinit(struct ldc_ctx *ctx)
{
	unsigned char coreid;
	if (ldc_check_null_ptr(ctx) != 0)
		return;

#if LDC_USE_WORKQUEUE
	for (coreid = 0; coreid < ctx->core_num; coreid++) {
		osal_wait_destroy(&ctx->wait);
		osal_workqueue_destroy(&ctx->core[coreid].work_frm_done);
		osal_list_del_init(&ctx->core[coreid].list);
	}
#endif
	ctx->stop_flag = 1;
	osal_kthread_destroy(ctx->thread, ctx->stop_flag);

	osal_sem_destroy(&ctx->vb_doneq.sem);
	osal_spin_lock_destroy(&ctx->vb_doneq.lock);
	osal_spin_lock_destroy(&ctx->ctx_lock);
	osal_mutex_destroy(&ldc_reg_lock);
	osal_mutex_destroy(&g_mesh_lock);

	// osal_list_del_init(&ctx->job_list);
	osal_list_del_init(&ctx->vb_doneq.doneq);
}
