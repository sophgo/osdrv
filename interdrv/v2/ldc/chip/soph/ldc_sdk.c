
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/comm_video.h>
#include <linux/comm_gdc.h>
#include <linux/ldc_uapi.h>

#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "ldc_debug.h"
#include "ldc_core.h"
#include "ldc_sdk.h"
#include "ldc_common.h"
#include "ldc.h"
#include "cmdq.h"
#include "ion.h"
#include "vbq.h"
#include "ldc_proc.h"
#include "mesh.h"
#include "base_cb.h"

#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))
#define BGCOLOR_GRAY  (0x808080)
#define BGCOLOR_GREEN  (YUV_8BIT(0, 128, 128))
#define LDC_INTR_EN_ALL (0x01)
#define LDC_INTR_EN_NULL (0x0)
#define LDC_INVALID_CORE_ID (LDC_DEV_MAX_CNT + 1)

#define USE_EXCEPTION_HDL 0
#define USE_REAL_CMDQ 0

static struct mutex g_mesh_lock;

static enum enum_modules_id convert_cb_id(mod_id_e mod_id)
{
	if (mod_id == ID_VI)
		return E_MODULE_VI;
	else if (mod_id == ID_VO)
		return E_MODULE_VO;
	else if (mod_id == ID_VPSS)
		return E_MODULE_VPSS;

	return E_MODULE_BUTT;
}

static mod_id_e convert_mod_id(enum enum_modules_id cbModId)
{
	if (cbModId == E_MODULE_VI)
		return ID_VI;
	else if (cbModId == E_MODULE_VO)
		return ID_VO;
	else if (cbModId == E_MODULE_VPSS)
		return ID_VPSS;

	return ID_BUTT;
}

int ldc_exec_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	struct ldc_vdev *wdev = (struct ldc_vdev *)dev;
	struct mesh_gdc_cfg *cfg;
	mod_id_e mod_id;
	int rc = -1;

	switch (cmd) {
		case LDC_CB_MESH_GDC_OP: {
			mutex_lock(&g_mesh_lock);
			cfg = (struct mesh_gdc_cfg *)arg;
			mod_id = convert_mod_id(caller);
			mutex_unlock(&g_mesh_lock);

			if (mod_id == ID_BUTT) {
				TRACE_LDC(DBG_WARN, "invalid mod do GDC_OP\n");
				return -1;
			}
			rc = mesh_gdc_do_op(wdev, cfg->usage, cfg->usage_param
				, cfg->vb_in, cfg->pix_format, cfg->mesh_addr
				,cfg->sync_io, cfg->cb_param, cfg->cb_param_size
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

int ldc_reg_cb(struct ldc_vdev *wdev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_LDC;
	reg_cb.dev		= (void *)wdev;
	reg_cb.cb		= ldc_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static void ldc_op_done_cb(mod_id_e mod_id, void *param, vb_blk blk)
{
	struct ldc_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	enum enum_modules_id callee = convert_cb_id(mod_id);

	TRACE_LDC(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, mod_id);
	cfg.param = param;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void ldc_hdl_hw_tsk_cb(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, bool is_last)
{
	bool is_internal;
	vb_blk blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	mod_id_e mod_id;
	unsigned char isLatask;
	struct ldc_vb_done *vb_done = NULL;
	unsigned long flags;

	if (unlikely(!wdev || !job || !tsk)) {
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
			atomic_long_fetch_and(~BIT(ID_GDC), &((struct vb_s *)blk_out)->mod_ids);
			if (isLatask)
				ldc_op_done_cb(mod_id, (void *)(uintptr_t)tsk->attr.private_data[2], blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal ldc sub job.
		// Kernel space:
		//  !isLatask used for internal ldc sub job.
		if (isLatask && blk_in != VB_INVALID_HANDLE) {
			atomic_long_fetch_and(~BIT(ID_GDC), &((struct vb_s *)blk_in)->mod_ids);
			vb_release_block(blk_in);
		} else if (!isLatask) {
			vfree((void *)(uintptr_t)tsk->attr.private_data[2]);
		}
	} else {
		if (!job->identity.sync_io && is_last) {
			vb_done = kzalloc(sizeof(*vb_done), GFP_ATOMIC);

			spin_lock_irqsave(&wdev->job_lock, flags);
			memcpy(&vb_done->img_out, &tsk->attr.img_out, sizeof(vb_done->img_out));
			memcpy(&vb_done->job, job, sizeof(*job));
			list_add_tail(&vb_done->node, &wdev->vb_doneq.doneq);
			spin_unlock_irqrestore(&wdev->job_lock, flags);

			TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
			up(&wdev->vb_doneq.sem);
			TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);

			TRACE_LDC(DBG_DEBUG, "vb_done->img_out[%llx-%d]\n"
				, vb_done->img_out.video_frame.phyaddr[0], vb_done->img_out.video_frame.width);
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name);
		}
	}
}

static void ldc_wkup_cmdq_tsk(struct ldc_core *core) {
#if USE_REAL_CMDQ
	(void)(core);
#else
	TRACE_LDC(DBG_DEBUG, "ldc_wkup_cmdq_tsk\n");

	core->cmdq_evt = true;
	wake_up_interruptible(&core->cmdq_wq);
#endif
}

static void ldc_notify_wkup_evt_kth(void *data, enum ldc_wait_evt evt)
{
	struct ldc_vdev *dev = (struct ldc_vdev *)data;
	unsigned long flags;

	if(!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->evt |= evt;
	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	wake_up_interruptible(&dev->wait);
}

static void ldc_clr_evt_kth(void *data)
{
	struct ldc_vdev *dev = (struct ldc_vdev *)data;
	unsigned long flags;
	enum ldc_wait_evt evt;

	if(!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	evt = dev->evt;
	dev->evt &= ~evt;
	TRACE_LDC(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);
}

static void ldc_work_handle_job_done(struct ldc_vdev *dev, struct ldc_job *job, unsigned char irq_coreid)
{
	unsigned long flags;
	struct fasync_struct *fasync;

	if (!job) {
		TRACE_LDC(DBG_ERR, "null job\n");
		return;
	}

	atomic_set(&job->job_state, LDC_JOB_END);

	ldc_proc_record_job_done(job);

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->job_cnt--;
	spin_unlock_irqrestore(&dev->job_lock, flags);
	ldc_notify_wkup_evt_kth(dev, LDC_EVENT_EOF);

	if (job->identity.sync_io) {
		TRACE_LDC(DBG_INFO, "job[%px]\n", job);
		job->job_done_evt = true;
		wake_up(&job->job_done_wq);
	} else {
		kfree(job);
		job = NULL;
		fasync = ldc_get_dev_fasync();
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}
	TRACE_LDC(DBG_DEBUG, "job done\n");
}

static void ldc_work_handle_tsk_done(struct ldc_vdev *dev
	, struct ldc_task *tsk, struct ldc_job *job, unsigned char irq_coreid, bool is_last_tsk)
{
	unsigned long flags;

	if (!tsk) {
		TRACE_LDC(DBG_ERR, "null tsk\n");
		return;
	}

	if (atomic_read(&tsk->state) == LDC_TASK_STATE_RUNNING) {
		atomic_set(&tsk->state, LDC_TASK_STATE_DONE);
		atomic_dec(&job->task_num);

		ldc_hdl_hw_tsk_cb(dev, job, tsk, is_last_tsk);

		ldc_proc_record_hw_tsk_done(job, tsk);

		spin_lock_irqsave(&dev->job_lock, flags);
		TRACE_LDC(DBG_DEBUG, "tsk[%px]\n", tsk);
		kfree(tsk);
		tsk = NULL;
		spin_unlock_irqrestore(&dev->job_lock, flags);

		TRACE_LDC(DBG_DEBUG, "tsk done, is last tsk[%d]\n", is_last_tsk);
		if (is_last_tsk) {
			ldc_work_handle_job_done(dev, job, irq_coreid);
		}
	} else {
		TRACE_LDC(DBG_ERR, "invalid tsk state(%d).\n"
			, (enum ldc_task_state)atomic_read(&tsk->state));
	}
}

void ldc_work_handle_frm_done(struct ldc_vdev *dev, struct ldc_core *core)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long flags;
	unsigned char irq_coreid;
	bool is_last_tsk = false;

	if(!dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	atomic_set(&core->state, LDC_CORE_STATE_IDLE);
	irq_coreid = (u8)core->dev_type;
	TRACE_LDC(DBG_INFO, "core(%d) state set(%d)\n", irq_coreid, LDC_CORE_STATE_IDLE);

	if (unlikely(irq_coreid) >= DEV_LDC_MAX)
		return;

	spin_lock_irqsave(&dev->job_lock, flags);
	job = list_first_entry_or_null(&dev->list.done_list[irq_coreid], struct ldc_job, node);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	if (unlikely(!job))
		return;

	if (atomic_read(&job->job_state) == LDC_JOB_WORKING) {
		spin_lock_irqsave(&dev->job_lock, flags);
		tsk = list_first_entry_or_null(&dev->core[irq_coreid].list.done_list, struct ldc_task, node);
		if (unlikely(!tsk)) {
			spin_unlock_irqrestore(&dev->job_lock, flags);
			return;
		}
		is_last_tsk = (atomic_read(&job->task_num) > 1 ? false : true);
		list_del(&tsk->node);
		list_del(&job->node);
		spin_unlock_irqrestore(&dev->job_lock, flags);

		ldc_work_handle_tsk_done(dev, tsk, job, irq_coreid, is_last_tsk);

		if (job && job->use_cmdq && !is_last_tsk)
			ldc_wkup_cmdq_tsk(&dev->core[irq_coreid]);
	} else {
		TRACE_LDC(DBG_ERR, "invalid job(%px) state(%d).\n"
			, job, (enum ldc_job_state)atomic_read(&job->job_state));
	}
}

#if LDC_USE_WORKQUEUE
static void ldc_work_frm_done(struct work_struct *work)//intr post handle
{
	struct ldc_core *core = container_of(work, struct ldc_core, work_frm_done);
	struct ldc_vdev *dev = ldc_get_dev();

	if(!core || !dev) {
		TRACE_LDC(DBG_ERR, "ldc vdev or core isn't created yet.\n");
		return;
	}

	ldc_work_handle_frm_done(dev, core);
}
#endif

static void ldc_wkup_frm_done_work(void *data, int top_id)
{
	struct ldc_core *core = (struct ldc_core *)data;
	(void)top_id;

	if (!core) {
		TRACE_LDC(DBG_ERR, "ldc core isn't created yet.\n");
		return;
	}

#if LDC_USE_WORKQUEUE
	//queue_work(dev->workqueue, &dev->work_frm_done);//wakeup post handle
	schedule_work(&core->work_frm_done);
#else
	ldc_work_handle_frm_done(ldc_get_dev(), core);
#endif
}

static void ldc_reset_cur_abord_job(struct ldc_vdev *dev, unsigned char coreid, struct ldc_job *job)
{
#if USE_EXCEPTION_HDL
	struct ldc_task *tsk, tsk_tmp;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	if (unlikely(!job)) {
		TRACE_LDC(DBG_ERR, "err job is null\n");
		return;
	}

	if (list_empty(&job->task_list)) {
		TRACE_LDC(DBG_ERR, "err tsklist is null\n");
		return;
	}

	list_for_each_entry_safe(tsk, tsk_tmp, &job->task_list, node) {
		if (unlikely(!tsk)) {
			TRACE_LDC(DBG_ERR, "cur job(%px) cur tsk is null\n", job);
			continue;
		}

		if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
			TRACE_LDC(DBG_ERR, "cur job(%px) cur tsk id(%d) invalid\n", job, tsk->tsk_id);
			continue;
		}

		ldc_intr_ctrl(LDC_INTR_EN_NULL, coreid);
		ldc_disable(coreid);
		ldc_reset(coreid);

		atomic_set(&dev->core[coreid]->state, LDC_CORE_STATE_IDLE);

		ldc_notify_wkup_evt_kth(dev, LDC_EVENT_RST);
		up(&tsk->sem);
	}

	atomic_set(&dev->state, LDC_DEV_STATE_RUNNING);
#else
	(void) dev;
	(void) coreid;
	(void) job;
	return;
#endif
}

static void ldc_try_reset_abort_job(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	unsigned long flags;
	struct ldc_job *work_job;

	spin_lock_irqsave(&wdev->job_lock, flags);
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		work_job = list_first_entry_or_null(&wdev->list.work_list[coreid], struct ldc_job, node);
		if (unlikely(!work_job)) {
			TRACE_LDC(DBG_NOTICE, "ldc vdev core[%d] select timeout,hw busy\n", coreid);
			ldc_reset_cur_abord_job(wdev, coreid, work_job);
		}
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
}

static void ldc_set_tsk_run_status(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	atomic_set(&wdev->core[top_id].state, LDC_CORE_STATE_RUNNING);
	TRACE_LDC(DBG_INFO, "core(%d) state set(%d)\n", top_id, LDC_CORE_STATE_RUNNING);

	atomic_set(&tsk->state, LDC_TASK_STATE_RUNNING);

	ldc_proc_record_hw_tsk_start(job, tsk, top_id);
}

static void ldc_submit_hw(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	struct ldc_cfg cfg;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, extend_haddr;
	unsigned long flags;
	struct ldc_job *work_job;
	struct ldc_task *work_tsk;

	ldc_set_tsk_run_status(wdev, top_id, job, tsk);

	mesh_addr = tsk->attr.private_data[0];
	mesh_addr = (mesh_addr != DEFAULT_MESH_PADDR) ? mesh_addr : 0;
	in_frame = &tsk->attr.img_in.video_frame;
	out_frame = &tsk->attr.img_out.video_frame;
	pix_format = in_frame->pixel_format;
	rotation = tsk->rotation;

	memset(&cfg, 0, sizeof(cfg));
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
	cfg.bgcolor = (u16)LDC_YUV_BLACK /*ctx->bgcolor*/;
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

	spin_lock_irqsave(&wdev->job_lock, flags);
	//tsk->coreid = top_id;
	job->coreid = top_id;

	list_add_tail(&job->node, &wdev->list.work_list[top_id]);
	work_job = list_first_entry_or_null(&wdev->list.work_list[top_id], struct ldc_job, node);

	list_add_tail(&tsk->node, &wdev->core[top_id].list.work_list);
	work_tsk = list_first_entry_or_null(&wdev->core[top_id].list.work_list, struct ldc_task, node);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	if (unlikely(!work_job || !work_tsk)) {
		TRACE_LDC(DBG_WARN, "core_id:[%d] null work job tsk\n", top_id);
		return;
	}

	ldc_reset(top_id);
	ldc_init(top_id);
	ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
	ldc_engine(&cfg, top_id);

	TRACE_LDC(DBG_INFO, "job[%px]\n", job);
	TRACE_LDC(DBG_DEBUG, "core_id:%d\n", top_id);
	TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg.src_width, cfg.src_height);
	TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n",
		in_frame->phyaddr[0], in_frame->phyaddr[1], in_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n",
		out_frame->phyaddr[0], out_frame->phyaddr[1], out_frame->phyaddr[2]);
	TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg.map_base);
	TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n", cfg.bgcolor, cfg.pix_fmt);
#if 0
	ldc_dump_register(top_id);
#endif
}

static void ldc_submit_hw_cmdq(struct ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *last_tsk, struct ldc_task **tskq)
{
	struct ldc_cfg *cfg_q[LDC_JOB_MAX_TSK_NUM] = {NULL};
	union cmdq_set *cmdq_addr = NULL;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, i, tsk_idx, extend_haddr;
	int tsk_num, cmdq_wq_ret;
	struct ldc_job *work_job;
	struct ldc_task *work_tsk;

	if (unlikely(!wdev || !job || !last_tsk || !tskq))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(last_tsk != tskq[tsk_num - 1])) {
		TRACE_LDC(DBG_ERR, "invalid last_tsk, not match with tskq\n");
		return;
	}

	cmdq_addr = kzalloc(sizeof(*cmdq_addr) * tsk_num * LDC_CMDQ_MAX_REG_CNT, GFP_ATOMIC);

	for (tsk_idx = 0; tsk_idx < tsk_num; tsk_idx++) {
		unsigned long flags;

		ldc_set_tsk_run_status(wdev, top_id, job, tskq[tsk_idx]);

		cfg_q[tsk_idx] = kzalloc(sizeof(struct ldc_cfg), GFP_ATOMIC);

		mesh_addr = tskq[tsk_idx]->attr.private_data[0];
		mesh_addr = (mesh_addr != DEFAULT_MESH_PADDR) ? mesh_addr : 0;
		in_frame = &tskq[tsk_idx]->attr.img_in.video_frame;
		out_frame = &tskq[tsk_idx]->attr.img_out.video_frame;
		pix_format = in_frame->pixel_format;
		rotation = tskq[tsk_idx]->rotation;

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
		cfg_q[tsk_idx]->bgcolor = (u16)LDC_YUV_BLACK /*ctx->bgcolor*/;
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

		spin_lock_irqsave(&wdev->job_lock, flags);
		//tskq[tsk_idx]->coreid = top_id;
		job->coreid = top_id;

		list_add_tail(&job->node, &wdev->list.work_list[top_id]);
		work_job = list_first_entry_or_null(&wdev->list.work_list[top_id], struct ldc_job, node);

		list_add_tail(&tskq[tsk_idx]->node, &wdev->core[top_id].list.work_list);
		work_tsk = list_first_entry_or_null(&wdev->core[top_id].list.work_list, struct ldc_task, node);

		wdev->core[top_id].cmdq_evt = false;
		spin_unlock_irqrestore(&wdev->job_lock, flags);

#if !USE_REAL_CMDQ //fake cmdq
		ldc_reset(top_id);
		ldc_init(top_id);
		ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);//0x0 cmdq need disable ldc intr
		ldc_engine(cfg_q[tsk_idx], top_id);

		if (tsk_idx != (tsk_num - 1)) {
			cmdq_wq_ret = wait_event_interruptible_timeout(wdev->core[top_id].cmdq_wq
				, wdev->core[top_id].cmdq_evt, LDC_EOF_WAIT_TIMEOUT_MS);
			if (cmdq_wq_ret <= 0) {
				TRACE_LDC(DBG_WARN, "ldc cdmq wait timeout, ret(%d)\n", cmdq_wq_ret);
				tsk_idx++;
				goto FREE_CMDQ_RES;
			}
			TRACE_LDC(DBG_DEBUG, "ldc cdmq wait done, ret(%d)\n", cmdq_wq_ret);
		}
#endif
	}

#if USE_REAL_CMDQ
	(void)cmdq_wq_ret;
	ldc_reset(top_id);
	ldc_init(top_id);
	ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);//0x0 cmdq need disable ldc intr
	ldc_engine_cmdq(top_id, (void *)cmdq_addr, cfg_q, tsk_num);
	goto FREE_CMDQ_RES;
#endif

FREE_CMDQ_RES:
	//last_tsk->coreid = top_id;

	TRACE_LDC(DBG_DEBUG, "cmd_start(%px)\n",cmdq_addr);
	for (i = 0; i < tsk_idx; i++) {
		TRACE_LDC(DBG_DEBUG, "tsk_id[%d]-core_id[%d]\n", i,  top_id);
		TRACE_LDC(DBG_DEBUG, "update size src(%d %d)\n", cfg_q[i]->src_width, cfg_q[i]->src_height);
		TRACE_LDC(DBG_DEBUG, "update src-buf: %#llx-%#llx\n",
			cfg_q[i]->src_y_base, cfg_q[i]->src_c_base);
		TRACE_LDC(DBG_DEBUG, "update dst-buf: %#llx-%#llx\n",
			cfg_q[i]->dst_y_base, cfg_q[i]->dst_c_base);
		TRACE_LDC(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->map_base);
		TRACE_LDC(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
			, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);

		if (cfg_q[i])
			kfree(cfg_q[i]);
	}

if (cmdq_addr)
	kfree(cmdq_addr);

#if 0
	ldc_dump_register(top_id);
	//ldc_dump_cmdq(top_id);
#endif
}

static int ldc_try_submit_hw(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, unsigned char use_cmdq, struct ldc_task **tskq)
{
	int i, top_id, ret = -1;
	enum ldc_core_state state;
	bool finish;

	for (i = 0; i < wdev->core_num; i++) {
		finish = ldc_is_finish(i);
		state = atomic_read(&wdev->core[i].state);

		TRACE_LDC(DBG_INFO, "core[%d]-state[%d]-isfinish[%d]\n", i, state, finish);

		if ((state == LDC_CORE_STATE_IDLE) && finish) {
			top_id = i;
			//ldc_enable_dev_clk(top_id, true);
			if (use_cmdq)
				ldc_submit_hw_cmdq(wdev, top_id, job, tsk, tskq);
			else
				ldc_submit_hw(wdev, top_id, job, tsk);
			ret = 0;
			break;
		}
	}

	if (ret)
		TRACE_LDC(DBG_NOTICE, "ldc_submit_hw fail,hw busy\n");

	return ret;
}

static int ldc_try_submit_hw_cmdq(struct ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task **tskq, struct ldc_task *last_tsk)
{
	return ldc_try_submit_hw(wdev, job, last_tsk, true, tskq);
}

static unsigned char ldc_is_tsk_ready(struct ldc_task *tsk)
{
	if (unlikely(!tsk)) {
		TRACE_LDC(DBG_ERR, "cur tsk is null\n");
		return false;
	}

	if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
		TRACE_LDC(DBG_ERR, "cur tsk id(%d) invalid\n", tsk->tsk_id);
		return false;
	}

	if (unlikely(down_trylock(&tsk->sem))) {
		TRACE_LDC(DBG_ERR, "cur tsk(%px) get sem fail\n", tsk);
		return false;
	}
	return true;
}

static unsigned char ldc_get_idle_coreid(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			break;
	}
	return coreid;
}

static bool ldc_have_idle_core(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			return true;
	}
	return false;
}

static void ldc_try_commit_job(struct ldc_vdev *wdev, struct ldc_job *job)
{
	unsigned long flags;
	unsigned char use_cmdq = false, is_ready = true;
	unsigned char i = 0, tsk_num;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL, *tskq[LDC_JOB_MAX_TSK_NUM], *last_tsk = NULL;

	if (!wdev || !job)
		return;

	if (atomic_read(&job->job_state) != LDC_JOB_WORKING)
		return;

	if (!ldc_have_idle_core(wdev))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(tsk_num <= 0)) {
		TRACE_LDC(DBG_ERR, "cur job(%px) tsk_num(%d) invalid\n"
			, job, tsk_num);
		return;
	}

	if (tsk_num == 1) {
		use_cmdq = false;
		spin_lock_irqsave(&wdev->job_lock, flags);
		tsk = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		is_ready = ldc_is_tsk_ready(tsk);
	} else {
		use_cmdq = true;
		i = 0;
		spin_lock_irqsave(&wdev->job_lock, flags);
		list_for_each_entry_safe(tskq[i], tmp_tsk, &job->task_list, node) {
			is_ready &= ldc_is_tsk_ready(tskq[i]);
			last_tsk = tskq[i];
			i++;
		}
		spin_unlock_irqrestore(&wdev->job_lock, flags);
	}

	if (!is_ready) {
		TRACE_LDC(DBG_ERR, "cur job(%px) cur tsk(%px) is not ready\n", job, tsk);
		return;
	}

	job->use_cmdq = use_cmdq;
	if (use_cmdq) {
		if (unlikely(!last_tsk)) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is null\n");
			return;
		}
		if (unlikely(!list_is_last(&last_tsk->node, &job->task_list))) {
			TRACE_LDC(DBG_ERR, "invalid last_tsk,is not last node\n");
			return;
		}
#if USE_REAL_CMDQ
		last_tsk->fn_tsk_cb = ldc_wkup_frm_done_work;
#else
		for (i = 0; i < atomic_read(&job->task_num); i++)
			tskq[i]->fn_tsk_cb = ldc_wkup_frm_done_work;
#endif
		ldc_try_submit_hw_cmdq(wdev, job, tskq, last_tsk);
	} else {
		tsk->fn_tsk_cb = ldc_wkup_frm_done_work;
		ldc_try_submit_hw(wdev, job, tsk, false, NULL);
	}
}

static int ldc_event_handler_th(void *data)
{
	struct ldc_vdev *wdev = (struct ldc_vdev *)data;
	unsigned long flags;
	struct ldc_job *job, *job_tmp;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(LDC_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	unsigned char idle_coreid;
	bool have_idle_job;

	if (!wdev) {
		TRACE_LDC(DBG_ERR, "ldc vdev isn't created yet.\n");
		return -1;
	}

	while (!kthread_should_stop()) {
		if (!wdev)
			break;

		ret = wait_event_interruptible_timeout(wdev->wait,
			wdev->evt || kthread_should_stop(), timeout);

		/* -ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		//suspend
		if (is_ldc_suspended())
			continue;

		/* timeout */
		if (!ret) {
			if (atomic_read(&wdev->state) == LDC_DEV_STATE_STOP) {
				timeout = idle_timeout;
				continue;
			} else if((atomic_read(&wdev->state) == LDC_DEV_STATE_RUNNING)
				&& list_empty(&wdev->job_list)) {
				timeout = idle_timeout;
				atomic_set(&wdev->state, LDC_DEV_STATE_STOP);
				up(&wdev->sem);
				continue;
			} else {
				ldc_try_reset_abort_job(wdev);
				continue;
			}
		}

		idle_coreid = ldc_get_idle_coreid(wdev);
		if (idle_coreid >= wdev->core_num) {
			TRACE_LDC(DBG_INFO, "ldc vdev hw busy, no idle core\n");
			goto continue_th;
		}

		if (list_empty(&wdev->job_list)) {
			TRACE_LDC(DBG_DEBUG, "job list empty\n");
			atomic_set(&wdev->state, LDC_DEV_STATE_STOP);
			up(&wdev->sem);
			goto continue_th;
		}

		have_idle_job = false;
		spin_lock_irqsave(&wdev->job_lock, flags);
		list_for_each_entry_safe(job, job_tmp, &wdev->job_list, node) {
			if (job->coreid == LDC_INVALID_CORE_ID) {
				TRACE_LDC(DBG_DEBUG, "got idle job[%px]\n", job);
				have_idle_job = true;
				break;
			}
		}

		if (!have_idle_job) { //have idle core but cur job is doing
			TRACE_LDC(DBG_INFO, "no idle job, job[%px] job_tmp[%px] coreid[%d]\n"
				, job, job_tmp, job_tmp->coreid);
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			goto continue_th;
		}

		atomic_set(&job->job_state, LDC_JOB_WORKING);
		list_del(&job->node);
		spin_unlock_irqrestore(&wdev->job_lock, flags);

		ldc_proc_record_job_start(job);
		atomic_set(&wdev->state, LDC_DEV_STATE_RUNNING);
		ldc_try_commit_job(wdev, job);

continue_th:
		ldc_clr_evt_kth(wdev);

		/* Adjust timeout */
		timeout = list_empty(&wdev->job_list) ? idle_timeout : eof_timeout;
	}

	return 0;
}

static void ldc_cancel_cur_tsk(struct ldc_job *job)
{
	struct ldc_task *tsk = NULL;
	unsigned char i;

	if (unlikely(!job))
		return;
	if (unlikely(list_empty(&job->task_list))) {
		TRACE_LDC(DBG_NOTICE, "null tsk list\n");
		return;
	}

	for (i = 0; i < atomic_read(&job->task_num); i++) {
		tsk = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
		if (tsk) {
			TRACE_LDC(DBG_DEBUG, "free tsk[%px]\n", tsk);
			list_del(&tsk->node);
			kfree(tsk);
			tsk = NULL;
		}
	}
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
int ldc_begin_job(struct ldc_vdev *wdev, struct gdc_handle_data *data)
{
	struct ldc_job *job;
	unsigned long flags;
	int ret = 0;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	if (is_ldc_suspended()) {
		TRACE_LDC(DBG_ERR, "ldc dev suspend\n");
		return ERR_GDC_NOT_PERMITTED;
	}

	job = kzalloc(sizeof(struct ldc_job), GFP_ATOMIC);
	if (job == NULL) {
		TRACE_LDC(DBG_ERR, "malloc failed.\n");
		return ERR_GDC_NOBUF;
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	INIT_LIST_HEAD(&job->task_list);
	atomic_set(&job->job_state, LDC_JOB_CREAT);
	atomic_set(&job->task_num, 0);
	job->identity.sync_io = true;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]++\n", job);

	return ret;
}

int ldc_end_job(struct ldc_vdev *wdev, unsigned long long handle)
{
	int ret = 0, sync_io_ret = 1;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)handle;
	unsigned long flags;
	int tsk_num = 0;
	unsigned long timeout = msecs_to_jiffies(LDC_SYNC_IO_WAIT_TIMEOUT_MS);
	struct ldc_task *tsk, *tmp_tsk;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(job);
	if (ret)
		return ret;

	if (list_empty(&job->task_list)) {
		TRACE_LDC(DBG_DEBUG, "no task in job.\n");
		return ERR_GDC_NOT_PERMITTED;
	}
	TRACE_LDC(DBG_INFO, "job[%px]\n", job);

	spin_lock_irqsave(&wdev->job_lock, flags);
	atomic_set(&job->job_state, LDC_JOB_WAIT);
	list_add_tail(&job->node, &wdev->job_list);
	wdev->job_cnt++;

	list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
		up(&tsk->sem);
		tsk_num++;
		tsk->tsk_id = tsk_num - 1;
	}
	atomic_set(&job->task_num, tsk_num);
	job->coreid = LDC_INVALID_CORE_ID;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	ldc_proc_commit_job(job);

	ldc_notify_wkup_evt_kth(wdev, LDC_EVENT_WKUP);

	if (job->identity.sync_io) {
		init_waitqueue_head(&job->job_done_wq);
		job->job_done_evt = false;
		sync_io_ret = wait_event_timeout(job->job_done_wq, job->job_done_evt, timeout);
		if (sync_io_ret <= 0) {
			TRACE_LDC(DBG_WARN, "end job[%px] fail,timeout, ret(%d)\n", job, sync_io_ret);
			ret = -1;
			spin_lock_irqsave(&wdev->job_lock, flags);
			wdev->job_cnt--;
			spin_unlock_irqrestore(&wdev->job_lock, flags);
		} else
			ret = 0;
	}

	TRACE_LDC(DBG_INFO, "jobname[%s] sync_io=%d, ret=%d\n", job->identity.name, job->identity.sync_io, ret);

	if (job->identity.sync_io) {
		kfree(job);
		job = NULL;
	}

	return ret;
}

int ldc_cancel_job(struct ldc_vdev *wdev, unsigned long long handle)
{
	int ret = 0;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)handle;
	struct ldc_job *job_tmp, *work_job, *wait_job;
	unsigned long flags;
	unsigned char coreid;
	bool needfreeJob = false;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(job);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (atomic_read(&job->job_state) == LDC_JOB_CREAT) {
		needfreeJob = true;
		goto FREE_JOB;
	}

	list_for_each_entry_safe(wait_job, job_tmp, &wdev->job_list, node) {
		if (job == wait_job) {
			TRACE_LDC(DBG_DEBUG, "cancel wait job(%px)\n", wait_job);
			atomic_set(&wdev->state, LDC_DEV_STATE_RUNNING);
			needfreeJob = true;
			ldc_cancel_cur_tsk(job);
			list_del(&job->node);
			break;
		}
	}

	for (coreid = 0; coreid < LDC_DEV_MAX_CNT; coreid++) {
		list_for_each_entry_safe(work_job, job_tmp, &wdev->list.work_list[coreid], node) {
			if (job == work_job) {
				TRACE_LDC(DBG_DEBUG, "cancel work job(%px)\n", wait_job);
				atomic_set(&wdev->state, LDC_DEV_STATE_RUNNING);
				atomic_set(&wdev->core[coreid].state, LDC_CORE_STATE_IDLE);
				ldc_core_deinit(coreid);
				needfreeJob = true;
				ldc_cancel_cur_tsk(job);
				list_del(&job->node);
				break;
			}
		}
	}

FREE_JOB:
	if (needfreeJob) {
		TRACE_LDC(DBG_DEBUG, "free job[%px]\n", job);
		// kfree(job);
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
	TRACE_LDC(DBG_DEBUG, "++\n");

	return ret;
}

int ldc_get_work_job(struct ldc_vdev *wdev, struct gdc_handle_data *data)
{
	int ret = 0;
	struct ldc_job *job = NULL;
	unsigned long flags;
	unsigned char coreid;

	ret = ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(data);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		job = list_first_entry_or_null(&wdev->list.work_list[coreid], struct ldc_job, node);
		if (job) {
			break;
		}
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	data->handle = (u64)(uintptr_t)job;

	TRACE_LDC(DBG_DEBUG, "job[%px]\n", job);

	return ret;
}

int ldc_set_identity(struct ldc_vdev *wdev,
			  struct gdc_identity_attr *identity)
{
	struct ldc_job *job;
	unsigned long flags;
	unsigned long long handle;

	if( ldc_check_null_ptr(wdev) ||
		ldc_check_null_ptr(identity)) {
		TRACE_LDC(DBG_ERR, "null dev or identity_attr\n");
		return -1;
	}
	handle = identity->handle;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	if (!job) {
		TRACE_LDC(DBG_ERR, "null job handle\n");
		return -1;
	}

	memcpy(&job->identity, &identity->attr, sizeof(identity->attr));
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "sync_io:%d, name:%s, id:%d\n"
		, job->identity.sync_io, job->identity.name, job->identity.id);
	return 0;
}

int ldc_add_rotation_task(struct ldc_vdev *wdev,
			  struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_ROT;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_add_ldc_task(struct ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!ldc_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

#if 0
	ret = ldc_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;
#endif

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_LDC;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int ldc_get_chn_frame(struct ldc_vdev *wdev, struct gdc_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	int ret;
	unsigned long flags;
	struct ldc_vb_done *vb_done = NULL, *vb_done_tmp = NULL;
	bool ismatch = false;

	TRACE_LDC(DBG_DEBUG, "++\n");
	ret = ldc_check_null_ptr(pstvideo_frame)
		|| ldc_check_null_ptr(wdev)
		|| ldc_check_null_ptr(identity);
	if (ret != 0)
		return ret;

	memset(pstvideo_frame, 0, sizeof(*pstvideo_frame));
	if (s32milli_sec <= 0) {
		if (down_trylock(&wdev->vb_doneq.sem)) {
			TRACE_LDC(DBG_ERR, "cannot get sem, doneq not ready\n");
			return ERR_GDC_SYS_NOTREADY;
		}
	} else {
		TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
		ret = down_timeout(&wdev->vb_doneq.sem, msecs_to_jiffies(s32milli_sec));
		if (ret == -ETIME) {
			TRACE_LDC(DBG_ERR, "get sem timeout, doneq not ready\n");
			return ret;
		}
		TRACE_LDC(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (list_empty(&wdev->vb_doneq.doneq)) {
		TRACE_LDC(DBG_ERR, "vb_doneq is empty\n");
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		return ERR_GDC_NOBUF;
	}

	list_for_each_entry_safe(vb_done, vb_done_tmp, &wdev->vb_doneq.doneq, node) {
		if (!vb_done) {
			TRACE_LDC(DBG_ERR, "vb_done is null\n");
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			return ERR_GDC_NOBUF;
		}

		if (ldc_identity_is_match(&vb_done->job.identity, &identity->attr)) {
			TRACE_LDC(DBG_DEBUG, "vb_doneq identity[%d-%d-%s] is match [%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name
				, identity->attr.mod_id, identity->attr.id, identity->attr.name);
			ismatch = true;
			break;
		}
	}

	if (!ismatch) {
		TRACE_LDC(DBG_DEBUG, "vb_doneq[%px] identity[%d-%d-%s] not match [%d-%d-%s]\n"
			, vb_done_tmp, vb_done_tmp->job.identity.mod_id, vb_done_tmp->job.identity.id, vb_done_tmp->job.identity.name
			, identity->attr.mod_id, identity->attr.id, identity->attr.name);
		up(&wdev->vb_doneq.sem);
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		return ERR_GDC_NOBUF;
	}

	list_del(&vb_done->node);

	memcpy(pstvideo_frame, &vb_done->img_out, sizeof(*pstvideo_frame));
	kfree(vb_done);
	vb_done = NULL;

	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "end to get pstvideo_frame width:%d height:%d buf:0x%llx\n"
		, pstvideo_frame->video_frame.width
		, pstvideo_frame->video_frame.height
		, pstvideo_frame->video_frame.phyaddr[0]);
	TRACE_LDC(DBG_DEBUG, "--\n");

	return ret;
}

int ldc_attach_vb_pool(vb_pool vb_pool)
{
	unsigned long flags;
	struct ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return -1;

	spin_lock_irqsave(&wdev->job_lock, flags);
	wdev->vb_pool = vb_pool;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "attach vb pool(%d)\n", vb_pool);
	return 0;
}

int ldc_detach_vb_pool(void)
{
	unsigned long flags;
	struct ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return -1;

	spin_lock_irqsave(&wdev->job_lock, flags);
	wdev->vb_pool = VB_INVALID_POOLID;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_LDC(DBG_DEBUG, "dettach vb pool\n");
	return 0;
}

/**************************************************************************
 *   internal APIs.
 **************************************************************************/
int ldc_sw_init(struct ldc_vdev *wdev)
{
	struct sched_param tsk;
	int ret = 0;
	unsigned char coreid;

	ret = ldc_check_null_ptr(wdev);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&wdev->job_list);
	INIT_LIST_HEAD(&wdev->vb_doneq.doneq);
	init_waitqueue_head(&wdev->wait);
	sema_init(&wdev->vb_doneq.sem, 0);
	sema_init(&wdev->sem, 0);

	spin_lock_init(&wdev->job_lock);
	wdev->core_num = LDC_DEV_MAX_CNT;
	wdev->evt = LDC_EVENT_BUSY_OR_NOT_STAT;
	wdev->vb_pool = VB_INVALID_POOLID;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
#if LDC_USE_WORKQUEUE
		INIT_WORK(&wdev->core[coreid].work_frm_done, ldc_work_frm_done);
#endif
		INIT_LIST_HEAD(&wdev->list.work_list[coreid]);
		INIT_LIST_HEAD(&wdev->list.done_list[coreid]);
		INIT_LIST_HEAD(&wdev->core[coreid].list.work_list);
		INIT_LIST_HEAD(&wdev->core[coreid].list.done_list);
	#if !USE_REAL_CMDQ
		init_waitqueue_head(&wdev->core[coreid].cmdq_wq);
	#endif
	}
	wdev->job_cnt = 0;

	wdev->thread = kthread_run(ldc_event_handler_th, (void *)wdev, "ldc_event_handler_th");
	if (IS_ERR(wdev->thread)) {
		TRACE_LDC(DBG_ERR, "failed to create ldc kthread\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(wdev->thread, SCHED_FIFO, &tsk);
	if (ret)
		TRACE_LDC(DBG_WARN, "ldc thread priority update failed: %d\n", ret);

	/*wdev->workqueue = create_singlethread_workqueue("ldc_workqueue");
	if (!wdev->workqueue) {
		TRACE_LDC(DBG_ERR, "ldc dev create_workqueue failed.\n");
		return -2;
	}

	INIT_WORK(&wdev->work_frm_done, ldc_work_frm_done);*/

	return ret;
}

void ldc_sw_deinit(struct ldc_vdev *wdev)
{
	unsigned char coreid;
	if (ldc_check_null_ptr(wdev) != 0)
		return;

	/*if (wdev->workqueue)
		destroy_workqueue(wdev->workqueue);*/

#if LDC_USE_WORKQUEUE
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		flush_work(&wdev->core[coreid].work_frm_done);
		cancel_work_sync(&wdev->core[coreid].work_frm_done);
		list_del_init(&wdev->list.work_list[coreid]);
		list_del_init(&wdev->list.work_list[coreid]);
		list_del_init(&wdev->core[coreid].list.work_list);
		list_del_init(&wdev->core[coreid].list.done_list);
	}
#endif
	(void)coreid;
	if (!IS_ERR(wdev->thread)) {
		if (kthread_stop(wdev->thread))
			TRACE_LDC(DBG_ERR, "fail to stop ldc kthread\n");
	}

	list_del_init(&wdev->job_list);
	list_del_init(&wdev->vb_doneq.doneq);
}

int ldc_suspend_handler(void)
{
	int ret;
	struct ldc_vdev * dev = ldc_get_dev();
	struct ldc_job *job;

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_LDC(DBG_ERR, "ldc thread not initialized yet\n");
		return ERR_GDC_SYS_NOTREADY;
	}

	if (dev->job_cnt > 0 || !list_empty(&dev->job_list) || atomic_read(&dev->state) == LDC_DEV_STATE_RUNNING) {
		sema_init(&dev->sem, 0);
		ret = down_timeout(&dev->sem, msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS));
		if (ret == -ETIME) {
			TRACE_LDC(DBG_ERR, "get sem timeout, dev not idle yet\n");
			return ret;
		}
	}
	atomic_set(&dev->state, LDC_DEV_STATE_STOP);
	spin_lock(&dev->job_lock);
	while(!list_empty(&dev->job_list)) {
		job = list_first_entry_or_null(&dev->job_list, struct ldc_job, node);
		list_del(&job->node);
	}
	dev->job_cnt = 0;
	spin_unlock(&dev->job_lock);

	TRACE_LDC(DBG_WARN, "suspend handler+\n");
	return 0;
}

int ldc_resume_handler(void)
{
	struct ldc_vdev * dev = ldc_get_dev();

	if (unlikely(!dev)) {
		TRACE_LDC(DBG_ERR, "ldc_dev is null\n");
		return ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_LDC(DBG_ERR, "ldc thread not initialized yet\n");
		return ERR_GDC_SYS_NOTREADY;
	}

	atomic_set(&dev->state, LDC_DEV_STATE_RUNNING);

	TRACE_LDC(DBG_WARN, "resume handler+\n");
	return 0;
}

